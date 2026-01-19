/*
 * Copyright (c) 2025-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup vpstorage Versioned Persistence
 * @{
 *
 * @file versioned-persistence.c
 * @brief Persistence storage with version information
 *
 * @}
 */

#include <string.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>
#include <ufw/endpoints.h>
#include <ufw/versioned-persistence.h>

/*
 * Low level utilities
 */

#define maybe(expr)                             \
    do {                                        \
        const int vp_retval_ = (expr);          \
        if (vp_retval_ < 0) {                   \
            return vp_retval_;                  \
        }                                       \
    } while (0)

/**
 * Put new header checksum into local meta data copy
 *
 * This is a local-only operation and stores nothing in a connected memory.
 * store_header() has to be called in order to perform such a transfer.
 *
 * Instead of manually doing this, vp_update_meta() should be used.
 *
 * @param  vp  VersionedPersistence instance to modify
 * @param  cs  New checksum value for header portion
 *
 * @sideeffects Modifies vp memory.
 */
static inline void
vp_put_header_chksum(VersionedPersistence *vp, const vp_chksum cs)
{
    (void)vp_set_chksum(vp->metablock + VP_OFFSET_HEADER_CHKSUM, cs);
}

/**
 * Put new payload checksum into local meta data copy
 *
 * This is like vp_put_header_chksum() but for the payload checksum.
 *
 * @param  vp  VersionedPersistence instance to modify
 * @param  cs  New checksum value for payload portion
 *
 * @sideeffects Modifies vp memory.
 */
static inline void
vp_put_payload_chksum(VersionedPersistence *vp, const vp_chksum cs)
{
    (void)vp_set_chksum(vp->metablock + VP_OFFSET_PAYLOAD_CHKSUM, cs);
}

/*
 * Meta data handling
 */

/**
 * Check if header data is consistent
 *
 * This function depends on the fact that the `cache` and the local meta data
 * copy are up-to-date. With this precondition satisfied, the test performed is
 * trivial.
 *
 * @param  vp  VersionedPersistence instance to query
 *
 * @return True if header data checksums agree; false otherwise.
 * @sideeffects None
 */
static inline bool
header_intact(const VersionedPersistence *vp)
{
    return (vp_get_header_chksum(vp) == vp->cache.header);
}

/**
 * Check if payload data is consistent
 *
 * This is like header_intact() but for the payload checksum.
 *
 * @param  vp  VersionedPersistence instance to query
 *
 * @return True if payload data checksums agree; false otherwise.
 * @sideeffects None
 */
static inline bool
payload_intact(const VersionedPersistence *vp)
{
    return (vp_get_payload_chksum(vp) == vp->cache.payload);
}

/**
 * Update header checksum cache
 *
 * The updated datum is stored in `vp->cache.header`.
 *
 * @param  vp  VersionedPersistence instance to update
 *
 * @sideeffects Modifies vp->cache.
 */
static void
calculate_header_checksum(VersionedPersistence *vp)
{
    vp->cache.header = vp->chksum.process(vp->chksum.initial,
                                          vp->metablock + sizeof(vp_chksum),
                                          VP_SIZE_META  - sizeof(vp_chksum));
}

/**
 * Update payload checksum cache
 *
 * The updated datum is stored in `vp->cache.payload`.
 *
 * Since this function has to interact with the memory peripheral used for
 * storage, errors are possible. Therefore the functions return type is int, in
 * contrast to `void` in case of calculate_header_checksum().
 *
 * @param  vp  VersionedPersistence instance to update
 * @param  n   Length of data to consider for the checksum calculation.
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Modifies vp->cache.
 */
static int
calculate_payload_checksum(VersionedPersistence *vp, const size_t n)
{
    unsigned char buf[16];
    size_t rest = n;

    maybe(source_seek(&vp->data.source, vp->data.address + VP_SIZE_META));
    vp_chksum cs = vp->chksum.initial;
    while (rest > 0U) {
        const size_t toread = rest > sizeof(buf) ? sizeof(buf) : rest;
        const ssize_t m = source_get_chunk(&vp->data.source, buf, toread);
        if (m < 0) {
            return (int)m;
        }
        rest -= m;
        cs = vp->chksum.process(cs, buf, m);
    }

    vp->cache.payload = cs;
    return 0;
}

/**
 * Load local copy of meta data block from configured memory and check it
 *
 * This system keeps a copy of the header data of the VersionedPersistence from
 * persistent memory in RAM. Updates to it can then be done locally and be
 * updated via simple, single block transfers.
 *
 * After transferring data, the header checksum is checked against a previously
 * calculated value from actual data in persistent memory. If this test fails,
 * `-EBADFD` is returned. Because the header_intact() is used, the system must
 * ensure that the local header data cache was updated using
 * calculate_header_checksum().
 *
 * @param  vp  VersionedPersistence instance to update
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Updates `vp->metablock`.
 */
static int
read_meta(VersionedPersistence *vp)
{
    maybe(source_seek(&vp->data.source, vp->data.address));
    maybe(source_get_chunk(&vp->data.source, vp->metablock, VP_SIZE_META));
    calculate_header_checksum(vp);
    return header_intact(vp) ? 0 : -EBADFD;
}

/**
 * Check if payload data is consistent
 *
 * This checks the payload checksum stored in meta data to a value calculated
 * at runtime, to determine its consistency. This function expects a payload
 * size to be specified. This is important, since the variant of payload in the
 * persistent memory may not match the one in `vp->spec`. The value should be
 * taken from the local meta data block via vp_get_length().
 *
 * If the consistency check fails, `-EBADMSG` is returned.
 *
 * @param  vp  VersionedPersistence instance to check
 * @param  n   Size of payload in bytes.
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Updates `vp->cache.payload`.
 */
static int
verify_payload(VersionedPersistence *vp, const size_t n)
{
    maybe(calculate_payload_checksum(vp, n));
    return payload_intact(vp) ? 0 : -EBADMSG;
}

/**
 * Transfer local meta data copy into configured memory
 *
 * @param  vp  VersionedPersistence instance trigger the transfer for
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Performs IO with the configured data sink.
 */
static int
store_header(VersionedPersistence *vp)
{
    maybe(sink_seek(&vp->data.sink, vp->data.address));
    maybe(sink_put_chunk(&vp->data.sink, vp->metablock, VP_SIZE_META));
    return 0;
}

/**
 * Correctly update checksum values in local meta data block
 *
 * @param  vp  VersionedPersistence instance to update
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Modifies vp in various ways
 */
static int
update_checksums(VersionedPersistence *vp)
{
    const vp_length n = vp_get_length(vp);
    /* Order is important here: First payload checksum, then header. */
    maybe(calculate_payload_checksum(vp, n));
    vp_put_payload_chksum(vp, vp->cache.payload);
    BIT_SET(vp->state, VP_STATE_PAYLOAD_CONSISTENT);
    calculate_header_checksum(vp);
    vp_put_header_chksum(vp, vp->cache.header);
    BIT_SET(vp->state, VP_STATE_META_CONSISTENT);
    return 0;
}

/*
 * Main user API
 */

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
    return vp_get_length(vp) + VP_SIZE_META;
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
 * Update meta data header in persistent memory
 *
 * This procedure does not only write the data into persistent memory. Before
 * doing that, it updates the two checksums of the system. This is therefore a
 * fairly expensive operation.
 *
 * @param  vp  VersionedPersistence instance to update
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Performs IO with the configured data sink.
 */
int
vp_update_meta(VersionedPersistence *vp)
{
    maybe(update_checksums(vp));
    return store_header(vp);
}

/**
 * Initialise a VersionedPersistence instance with specification
 *
 * This function stores the current section specification (version and length)
 * into the local meta data copy, updates all checksums, stores the result into
 * the configured memory storage, and finally refreshes the state of the
 * instance using vp_refresh().
 *
 * This is a useful operation when starting to work with a new bit of
 * persistent storage. In the absence of problems with the memory itself, this
 * will yield a usable VersionedPersistence instance in `vp`. This procedure
 * does not modify the payload part of the memory section.
 *
 * @param  vp  VersionedPersistence instance to initialise
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
int
vp_init(VersionedPersistence *vp)
{
    vp_put_version(vp, vp->spec.version);
    vp_put_length(vp, vp->spec.length);
    maybe(vp_update_meta(vp));
    return vp_refresh(vp);
}

/**
 * Refresh meta data information of VersionedPersistence instance
 *
 * This procedure reads the meta data header of `vp` from the associated memory
 * and verifies its consistency and compatibility with the specification made
 * in `vp->spec`. It then checks if the payload can be verified. This leads to
 * a number of bits to be set in `vp->state`:
 *
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
vp_refresh(VersionedPersistence *vp)
{
    vp->state = 0U;
    /* This reads the meta data block from storage into vp->metablock, verifies
     * its consistency. In the process it fills vp->cache.header. If this
     * returns something negative, it is time to abort, since the header
     * checksum could not be verified, and hence the information cannot be
     * trusted. */
    maybe(read_meta(vp));
    BIT_SET(vp->state, VP_STATE_META_CONSISTENT);

    const vp_version read_version = vp_get_version(vp);
    const vp_length read_length = vp_get_length(vp);

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
    maybe(verify_payload(vp, read_length));

    BIT_SET(vp->state, VP_STATE_PAYLOAD_CONSISTENT);
    return 0;
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
 * Note that this only modifies the memory. In most cases, you will want to use
 * vp_format() instead, to also update checksums in the header of the
 * instance.
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
    if ((n + offset) > vp_get_length(vp)) {
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

    return 0;
}

/**
 * Format VersionedPersistence instance with constant data
 *
 * This function uses vp_memset() to initialise all data indicated by the
 * header's length field with a constant datum and updates the meta data header
 * accordingly.
 *
 * Note that this does not modify the specification in the header section. This
 * can be done with vp_init(). There is also vp_save() that stores a datum
 * according to `vp->spec` into the associated memory, without looking at the
 * currently stored format.
 *
 * @param  vp  VersionedPersistence to modify.
 * @param  c   Datum to write to all indicated addresses.
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
int
vp_format(VersionedPersistence *vp, const unsigned char c)
{
    maybe(vp_memset(vp, c, 0U, vp_get_length(vp)));
    return vp_update_meta(vp);
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
    if (parts == 0) {
        return 0;
    }

    /* Make sure the current checksums are correct, so we invalidate the
     * correct data. */
    maybe(update_checksums(vp));

    if (BIT_ISSET(parts, VP_DATA_PAYLOAD)) {
        const uint16_t cs = vp_get_payload_chksum(vp);
        vp_put_payload_chksum(vp, cs ^ VP_CHKSUM_MAX);
        BIT_CLEAR(vp->state, VP_STATE_PAYLOAD_CONSISTENT);
    }

    if (BIT_ISSET(parts, VP_DATA_META)) {
        const uint16_t cs = vp_get_header_chksum(vp);
        vp_put_header_chksum(vp, cs ^ VP_CHKSUM_MAX);
        BIT_CLEAR(vp->state, VP_STATE_META_CONSISTENT);
    }

    return store_header(vp);
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
vp_fetch_part(VersionedPersistence *vp, void *dst,
              const size_t offset, const size_t n)
{
    /* In fetches, header and payload must have been verified */
    if (BIT_ISSET(vp->state,
                  VP_STATE_PAYLOAD_CONSISTENT |
                  VP_STATE_META_CONSISTENT) == false)
    {
        return -EINVAL;
    }
    if ((offset + n) > vp_get_length(vp)) {
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
vp_store_part(VersionedPersistence *vp, const void *src,
              const size_t offset, const size_t n)
{
    /* In stores, only the meta header must have been verified before. The
     * payload checksum will be updated if this procedure finishes success-
     * fully. */
    if (BIT_ISSET(vp->state, VP_STATE_META_CONSISTENT) == false) {
        return -EINVAL;
    }
    if ((offset + n) > vp_get_length(vp)) {
        return -EFAULT;
    }
    const size_t start = vp->data.address + VP_SIZE_META + offset;
    maybe(sink_seek(&vp->data.sink, start));
    const ssize_t rc = sink_put_chunk(&vp->data.sink, src, n);
    if (rc >= 0) {
        maybe(vp_update_meta(vp));
    }
    return rc;
}

/**
 * Block read for the whole payload of a VersionedPersistence instance
 *
 * This is a variant of vp_fetch_part() for convenience, that reads as much
 * data as indicated by the meta data header stored in the associated memory of
 * the object. You must be sure that `dst` can take as much data. Be sure to
 * inspect `vp->state` or use vp_get_length() and vp_get_version() to be sure
 * you know the state of the storage.
 *
 * @param  vp   VersionedPersistence instance to modify
 * @param  dst  Pointer to a buffer to write into
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Performs IO with associated memory.
 */
ssize_t
vp_fetch(VersionedPersistence *vp, void *dst)
{
    return vp_fetch_part(vp, dst, 0, vp_get_length(vp));
}

/**
 * Block write for the whole payload of a VersionedPersistence instance
 *
 * This is a variant of vp_store_part() for convenience, that writes as much
 * data as indicated by the meta data header stored in the associated memory of
 * the object. You must be sure that `src` can source as much data. Be sure to
 * inspect `vp->state` or use vp_get_length() and vp_get_version() to be sure
 * you know the state of the storage.
 *
 * @param  vp   VersionedPersistence instance to modify
 * @param  src  Pointer to a buffer to write into
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
ssize_t
vp_store(VersionedPersistence *vp, const void *src)
{
    return vp_store_part(vp, src, 0, vp_get_length(vp));
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
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Manipulates `vp` and performs IO with associated memory.
 */
ssize_t
vp_save(VersionedPersistence *vp, const void *src)
{
    maybe(vp_init(vp));
    maybe(vp_refresh(vp));
    return vp_store(vp, src);
}
