/*
 * Copyright (c) 2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup vpstorage Versioned Persistence
 * @{
 *
 * @file vp/core.c
 * @brief Persistence storage with version information
 *
 * @}
 */

#include <string.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>
#include <ufw/endpoints.h>
#include <ufw/versioned-persistence.h>

#include "internal.h"

/**
 * Return the full size of a VersionedPersistence instance
 *
 * This returns the combined size of the header and payload sections of a
 * VersionedPersistence instance. Note, that this is about the current size in
 * memory, which may differ from `vp->spec`.
 *
 * @param  vp  VersionedPersistence instance to query
 *
 * @return Size of instance in persistent memory
 * @sideeffects None
 */
size_t
vp_section_size(const VersionedPersistence *vp)
{
    return get_length(vp) + VP_SIZE_META;
}

/**
 * Return the full size of a VersionedPersistence specification
 *
 * This is like vp_section_size(), except that it returns the size required
 * by the instance specification, rather than the value of the section found in
 * persistent memory.
 *
 * @param  vp  VersionedPersistence instance to query
 *
 * @return Size of instance size according to its specification.
 * @sideeffects None
 */
size_t
vp_spec_size(const VersionedPersistence *vp)
{
    return vp->spec.length + VP_SIZE_META;
}

/**
 * Fill payload memory with a constant datum
 *
 * This procedure is like memset() from the C standard library, but for the
 * payload part of the memory associated of the VersionedPersistence instance.
 * In contrast to memset(), the function takes an offset parameter, since here
 * it is not possible to manipulate the start of the addressed memory
 * otherwise.
 *
 * @param  vp      VersionedPersistence to modify.
 * @param  c       Datum to write to all indicated addresses.
 * @param  offset  Offset at which to start writing data
 * @param  n       Length of data to override.
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
int
vp_memset(VersionedPersistence *vp, const unsigned char c,
          const size_t offset, const size_t n)
{
    if (BIT_ISSET(vp->state, VP_STATE_META_CACHE_ACTIVE) == false) {
        load_meta_from_spec(vp);
    }

    if ((n + offset) > get_length(vp)) {
        return -EINVAL;
    }

    unsigned char buf[16];
    memset(buf, c, sizeof(buf));
    size_t rest = n;

    maybe(sink_seek(&vp->data.sink, vp->data.address + VP_SIZE_META + offset));
    while (rest > 0U) {
        const size_t towrite = rest > sizeof(buf) ? sizeof(buf) : rest;
        const ssize_t m = sink_put_chunk(&vp->data.sink, buf, towrite);
        if (m < 0) {
            return (int)m;
        }
        rest -= m;
    }

    return vp_update_meta(vp);
}

/**
 * Format VersionedPersistence instance
 *
 * This function installs a value meta-data header in storage, that is
 * compatible with the instance's specification. The payload checksum of the
 * instance is explicitly invalidated.
 *
 * @param  vp  VersionedPersistence to modify.
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
int
vp_format(VersionedPersistence *vp)
{
    load_meta_from_spec(vp);
    /* vp_invalidate() updates checksums before invalidation, so it can create
     * the largest hamming distance possible between the correct and the wrong
     * value. This means, we don't have to initialise checksums here, because
     * vp_invalidate() will do it for us. */
    maybe(vp_invalidate(vp, VP_DATA_PAYLOAD));
    const int rc = vp_read_meta(vp);
    return rc;
}

/**
 * Invalidate checksums in associated memory
 *
 * There may be cases where users want to invalidate checksums in associated
 * memory. This function allows doing this. The `parts` parameter allows
 * addressing checksums to invalidate. If this parameter is zero, the function
 * does nothing. The usable bits are:
 *
 *  - `VP_DATA_META`     Modify meta data header checksum
 *  - `VP_DATA_PAYLOAD`  Modify payload checksum
 *
 * @param  vp     VersionedPersistence to modify.
 * @param  parts  Bits addressing checksums to change
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
int
vp_invalidate(VersionedPersistence *vp, const unsigned int parts)
{
    if (BIT_ISSET(vp->state, VP_STATE_META_CACHE_ACTIVE) == false) {
        load_meta_from_spec(vp);
    }

    if (parts == 0) {
        return 0;
    }

    /* Make sure the current checksums are correct, so we invalidate the
     * correct data. */
    maybe(vp_update_checksums(vp));

    if (BIT_ISSET(parts, VP_DATA_PAYLOAD)) {
        const uint16_t cs = get_payload_chksum(vp);
        put_payload_chksum(vp, cs ^ VP_CHKSUM_MAX);
        BIT_CLEAR(vp->state, VP_STATE_PAYLOAD_CONSISTENT);
        /* Invalidating the payload checksum will invalidate the header
         * checksum. So re-establish it here. If it should be invalidated as
         * well, the next step here will do it. */
        vp_calculate_header_checksum(vp);
        put_header_chksum(vp, vp->result.header);
    }

    if (BIT_ISSET(parts, VP_DATA_META)) {
        const uint16_t cs = get_header_chksum(vp);
        put_header_chksum(vp, cs ^ VP_CHKSUM_MAX);
        BIT_CLEAR(vp->state, VP_STATE_META_CONSISTENT);
    }

    return vp_store_header(vp);
}

/**
 * Block transfer from a VersionedPersistence instance into a buffer
 *
 * This is the main block read procedure of the VersionedPersistence system.
 * The datum read into should be compatible with the specification stored in
 * the associated memory. To change the stored specification, use vp_init(),
 * vp_save() etc.
 *
 * @param  vp      VersionedPersistence instance to modify
 * @param  dst     Pointer to a buffer to write into
 * @param  offset  Offset at which to start reading data
 * @param  n       Size of block to read starting at offset
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Performs IO with associated memory.
 */
ssize_t
vp_fetch(VersionedPersistence *vp, void *dst,
         const size_t offset, const size_t n)
{
    if (BIT_ISSET(vp->state, VP_STATE_META_CACHE_ACTIVE) == false) {
        maybe(vp_read_meta(vp));
    }
    /* In fetches, header and payload must have been verified */
    if (BIT_ISSET(vp->state,
                  VP_STATE_PAYLOAD_CONSISTENT |
                  VP_STATE_META_CONSISTENT) == false)
    {
        return -EINVAL;
    }
    if ((offset + n) > get_length(vp)) {
        return -EFAULT;
    }
    const size_t start = vp->data.address + VP_SIZE_META + offset;
    maybe(source_seek(&vp->data.source, start));
    return source_get_chunk(&vp->data.source, dst, n);
}

/**
 * Block transfer from a buffer into a VersionedPersistence instance
 *
 * This is the main block write procedure of the VersionedPersistence system.
 * The data stored should be compatible with the specification stored in the
 * associated memory. To change the stored specification, use vp_init(),
 * vp_save() etc.
 *
 * @param  vp      VersionedPersistence instance to modify
 * @param  src     Pointer to a buffer to write into
 * @param  offset  Offset at which to start reading data
 * @param  n       Size of block to read starting at offset
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
ssize_t
vp_store(VersionedPersistence *vp, const void *src,
         const size_t offset, const size_t n)
{
    if (BIT_ISSET(vp->state, VP_STATE_META_CACHE_ACTIVE) == false) {
        load_meta_from_spec(vp);
    }
    /* In stores, only the meta header must have been verified before. The
     * payload checksum will be updated if this procedure finishes success-
     * fully. */
    if (BIT_ISSET(vp->state, VP_STATE_META_CONSISTENT) == false) {
        return -EINVAL;
    }
    if ((offset + n) > get_length(vp)) {
        return -EFAULT;
    }
    const size_t start = vp->data.address + VP_SIZE_META + offset;
    maybe(sink_seek(&vp->data.sink, start));
    /* This sink_put_chunk() doesn't use "maybe", because we want to return the
     * positive value from it as our own return value. */
    ssize_t rc = sink_put_chunk(&vp->data.sink, src, n);
    if (rc >= 0) {
        /* Therefore this is the successful branch. */
        maybe(vp_update_meta(vp));
    }
    /* …and this is error handling. */
    return rc;
}

/**
 * Load meta data information of VersionedPersistence instance from storage
 *
 * This procedure reads the meta data header of `vp` from the associated memory
 * and verifies its consistency and compatibility with the specification made
 * in `vp->spec`. It then checks if the payload can be verified. This leads to
 * a number of bits to be set in `vp->state`:
 *
 * - `VP_STATE_META_CACHE_ACTIVE`   Specification (version, length) is cached
 * - `VP_STATE_META_CONSISTENT`     Meta data checksum could be verified
 * - `VP_STATE_LENGTH_COMPATIBLE`   Payload length matches specification
 * - `VP_STATE_VERSION_COMPATIBLE`  Payload version matches specification
 * - `VP_STATE_PAYLOAD_COMPATIBLE`  Payload length and version are compatible
 * - `VP_STATE_PAYLOAD_CONSISTENT`  Payload data could be verified
 *
 * Payload can be consistent, but not compatible. The verification is done with
 * the length indication in the meta data header of the associated memory, not
 * the one in the instance's spec. If this is the case, user can decide if they
 * want to implement a conversion from the old storage format to the new one.
 *
 * @param  vp  VersionedPersistence instance to update
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
int
vp_load(VersionedPersistence *vp)
{
    vp->state = 0U;
    /* This reads the meta data block from storage into vp->metablock, verifies
     * its consistency. In the process it fills vp->result.header. If this
     * returns something negative, it is time to abort, since the header
     * checksum could not be verified, and hence the information cannot be
     * trusted. */
    maybe(vp_read_meta(vp));

    const vp_version read_version = get_version(vp);
    const vp_length read_length = get_length(vp);

    if (read_version == vp->spec.version) {
        BIT_SET(vp->state, VP_STATE_VERSION_COMPATIBLE);
    }

    if (read_length == vp->spec.length) {
        BIT_SET(vp->state, VP_STATE_LENGTH_COMPATIBLE);
        if (BIT_ISSET(vp->state, VP_STATE_VERSION_COMPATIBLE)) {
            BIT_SET(vp->state, VP_STATE_PAYLOAD_COMPATIBLE);
        }
    }

    /* Payload verification has to be done with the stored length information.
     * If we're here, then the meta information block was verified, so the data
     * in read_length is good to go. */
    maybe(vp_verify_payload(vp, read_length));

    BIT_SET(vp->state, VP_STATE_PAYLOAD_CONSISTENT);
    return 0;
}

/**
 * High level store function
 *
 * This function takes a datum, and puts it into associated memory, according
 * to the specification in `vp->spec`. The currently stored specification is
 * not consulted. This function is useful to just store a datum, without any
 * possible upgrade paths implemented.
 *
 * @param  vp   VersionedPersistence instance to modify
 * @param  src  Pointer to a buffer to write into
 * @param  n    Size of buffer in src
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
ssize_t
vp_save(VersionedPersistence *vp, const void *src, const size_t n)
{
    maybe(vp_format(vp));
    maybe(vp_store(vp, src, 0, n));
    return vp_load(vp);
}
