/*
 * Copyright (c) 2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup vpstorage Versioned Persistence
 * @{
 *
 * @file vp/internal.c
 * @brief Internal functions for VersionedPersistence
 *
 * @}
 */

#include <ufw/versioned-persistence.h>

#include "internal.h"

/**
 * Update header checksum result
 *
 * The updated datum is stored in `vp->result.header`.
 *
 * @param  vp  VersionedPersistence instance to update
 *
 * @sideeffects Modifies vp->result.
 */
void
vp_calculate_header_checksum(VersionedPersistence *vp)
{
   vp->result.header = vp->chksum.process(vp->chksum.initial,
                                           vp->metablock + sizeof(vp_chksum),
                                           VP_SIZE_META  - sizeof(vp_chksum));
}

/**
 * Update payload checksum result
 *
 * The updated datum is stored in `vp->result.payload`.
 *
 * Since this function has to interact with the memory peripheral used for
 * storage, errors are possible. Therefore the functions return type is int, in
 * contrast to `void` in case of calculate_header_checksum().
 *
 * @param  vp  VersionedPersistence instance to update
 * @param  n   Length of data to consider for the checksum calculation.
 *
 * @return Zero if no errors occured; negative errno otherwise.
 * @sideeffects Modifies vp->result.
 */
int
vp_calculate_payload_checksum(VersionedPersistence *vp, const size_t n)
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

    vp->result.payload = cs;
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
 * @sideeffects Updates `vp->metablock`, and `vp->state`.
 */
int
vp_read_meta(VersionedPersistence *vp)
{
    /* Transfer meta data header into vp->metablock. */
    maybe(source_seek(&vp->data.source, vp->data.address));
    maybe(source_get_chunk(&vp->data.source, vp->metablock, VP_SIZE_META));

    /* Update vp->result.header from vp->metablock.  */
    vp_calculate_header_checksum(vp);

    const uint16_t mask = (VP_STATE_META_CACHE_FROM_STORAGE |
                           VP_STATE_META_CACHE_ACTIVE       |
                           VP_STATE_META_CONSISTENT);

    /* Check that stored header checksum and calculated value match. */
    if (header_intact(vp) == false) {
        BIT_CLEAR(vp->state, mask);
        return -EBADFD;
    }

    BIT_SET(vp->state, mask);
    return 0;
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
 * @sideeffects Updates `vp->result.payload`.
 */
int
vp_verify_payload(VersionedPersistence *vp, const size_t n)
{
    maybe(vp_calculate_payload_checksum(vp, n));
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
int
vp_store_header(VersionedPersistence *vp)
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
int
vp_update_checksums(VersionedPersistence *vp)
{
    const vp_length n = get_length(vp);
    /* Order is important here: First payload checksum, then header. */
    maybe(vp_calculate_payload_checksum(vp, n));
    put_payload_chksum(vp, vp->result.payload);
    vp_calculate_header_checksum(vp);
    put_header_chksum(vp, vp->result.header);
    vp_calculate_header_checksum(vp);
    return 0;
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
    if (BIT_ISSET(vp->state, VP_STATE_META_CACHE_ACTIVE) == false) {
        load_meta_from_spec(vp);
    }
    maybe(vp_update_checksums(vp));
    return vp_store_header(vp);
}
