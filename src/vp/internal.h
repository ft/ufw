/*
 * Copyright (c) 2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup vpstorage Versioned Persistence
 * @{
 *
 * @file vp/internal.h
 * @brief Internal utilities for VersionedPersistence implementation
 *
 * These functions could well live in versioned-persistence.c, but they are
 * useful for unit tests also, so we allow the unit test to include this set of
 * internal functions explicitly.
 *
 * @}
 */

#ifndef INC_VERSIONED_PERSISTENCE_PRIVATE_H_545e5959
#define INC_VERSIONED_PERSISTENCE_PRIVATE_H_545e5959

#include <ufw/versioned-persistence.h>

#define maybe(expr)                             \
    do {                                        \
        const int vp_retval_ = (expr);          \
        if (vp_retval_ < 0) {                   \
            return vp_retval_;                  \
        }                                       \
    } while (0)

#define vp_ref_chksum bf_ref_u16n
#define vp_set_chksum bf_set_u16n
#define vp_ref_length bf_ref_u16n
#define vp_set_length bf_set_u16n
#define vp_ref_version bf_ref_u16n
#define vp_set_version bf_set_u16n

/**
 * Change specification version from local meta data header
 *
 * This works on the local copy of the meta data header. In most cases, you
 * probably want to use load_meta_from_spec() or similar instead of using this
 * function directly.
 *
 * @param  vp       VersionedPersistence instance to query
 * @param  version  Version value to be stored
 *
 * @sideeffects Modifies local meta data header copy
 */
static inline void
put_version(VersionedPersistence *vp, const vp_version version)
{
    (void)vp_set_version(vp->metablock + VP_OFFSET_VERSION, version);
}

/**
 * Read specification version from local meta data header
 *
 * This is like vp_get_header_chksum() but for the specification version
 * field.
 *
 * @param  vp  VersionedPersistence instance to query
 *
 * @return Specification version
 * @sideeffects None
 */
static inline vp_version
get_version(const VersionedPersistence *vp)
{
    return vp_ref_version(vp->metablock + VP_OFFSET_VERSION);
}

/**
 * Change specification version from local meta data header
 *
 * This is like vp_put_version() but for the specification length field in
 * the local copy of the meta data header.
 *
 * @param  vp      VersionedPersistence instance to query
 * @param  length  Length value to be stored
 *
 * @sideeffects Modifies local meta data header copy
 */
static inline void
put_length(VersionedPersistence *vp, const vp_length length)
{
    (void)vp_set_length(vp->metablock + VP_OFFSET_LENGTH, length);
}

/**
 * Read specification length from local meta data header
 *
 * This is like vp_get_header_chksum() but for the specification length
 * field.
 *
 * @param  vp  VersionedPersistence instance to query
 *
 * @return Specification length
 * @sideeffects None
 */
static inline vp_length
get_length(const VersionedPersistence *vp)
{
    return vp_ref_length(vp->metablock + VP_OFFSET_LENGTH);
}

/**
 * Initialise a VersionedPersistence instance with specification
 *
 * The specification is spec.version and spec.length. This transfers both into
 * the local cache of the instances meta data section.
 *
 * @param  vp  VersionedPersistence instance to initialise
 *
 * @sideeffects Modifies locally cached meta-data header in `vp`.
 */
static inline void
load_meta_from_spec(VersionedPersistence *vp)
{
    put_version(vp, vp->spec.version);
    put_length(vp, vp->spec.length);
    BIT_SET(vp->state, VP_STATE_META_CACHE_ACTIVE);
    BIT_CLEAR(vp->state, VP_STATE_META_CACHE_FROM_STORAGE);
}

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
put_header_chksum(VersionedPersistence *vp, const vp_chksum cs)
{
    (void)vp_set_chksum(vp->metablock + VP_OFFSET_HEADER_CHKSUM, cs);
}

/**
 * Read header checksum from local meta data header
 *
 * This reads data from the local copy of the meta data header, and does not
 * communicate with the associated memory. Therefore this should only be used
 * when this data was updated, for instance via vp_load().
 *
 * @param  vp  VersionedPersistence instance to query
 *
 * @return Header data checksum
 * @sideeffects None
 */
static inline vp_chksum
get_header_chksum(const VersionedPersistence *vp)
{
    return vp_ref_chksum(vp->metablock + VP_OFFSET_HEADER_CHKSUM);
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
put_payload_chksum(VersionedPersistence *vp, const vp_chksum cs)
{
    (void)vp_set_chksum(vp->metablock + VP_OFFSET_PAYLOAD_CHKSUM, cs);
}

/**
 * Read payload checksum from local meta data header
 *
 * This is like vp_get_header_chksum() but for the payload checksum field.
 *
 * @param  vp  VersionedPersistence instance to query
 *
 * @return Payload data checksum
 * @sideeffects None
 */
static inline vp_chksum
get_payload_chksum(const VersionedPersistence *vp)
{
    return vp_ref_chksum(vp->metablock + VP_OFFSET_PAYLOAD_CHKSUM);
}

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
    return (get_header_chksum(vp) == vp->result.header);
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
    return (get_payload_chksum(vp) == vp->result.payload);
}

/* Prototypes for non-inlined functions */

void vp_calculate_header_checksum(VersionedPersistence *vp);
int vp_calculate_payload_checksum(
    VersionedPersistence *vp, size_t n);
int vp_read_meta(VersionedPersistence *vp);
int vp_verify_payload(VersionedPersistence *vp, size_t n);
int vp_store_header(VersionedPersistence *vp);
int vp_update_checksums(VersionedPersistence *vp);
int vp_update_meta(VersionedPersistence *vp);

#endif /* INC_VERSIONED_PERSISTENCE_PRIVATE_H_545e5959 */
