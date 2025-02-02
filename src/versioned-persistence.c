/*
 * Copyright (c) 2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file versioned-persistence.c
 * @brief Persistence storage with version information
 */

#include <string.h>

#include <ufw/binary-format.h>
#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>
#include <ufw/endpoints.h>
#include <ufw/versioned-persistence.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push

#define maybe(expr)                             \
    do {                                        \
        const int vp_retval_ = (expr);          \
        if (vp_retval_ < 0) {                   \
            return vp_retval_;                  \
        }                                       \
    } while (0)

#define new_offset(vp__, s, off__)                              \
    do {                                                        \
    struct addressable_source *cfg__ = (vp__)->data.s.driver;   \
    cfg__->address = (vp__)->meta.address + (off__);            \
    } while (0)

static inline bool
checksum_matches(const VersionedPersistenceChecksums *p)
{
    return (p->from_store == p->calculated);
}

static int
read_meta(VersionedPersistence *vp, VersionedPersistenceChecksums *cs)
{
    unsigned char buf[VP_SIZE_META];
    new_offset(vp, source, 0);
    maybe(source_get_chunk(&vp->data.source, buf, VP_SIZE_META));

    cs->from_store = vp_ref_chksum(buf);
    cs->calculated =
        vp->chksum.process(vp->chksum.initial, buf+sizeof(vp_chksum),
                           VP_SIZE_META-sizeof(vp_chksum));

    return checksum_matches(cs) ? 0 : -EBADFD;
}

static int
verify_payload(VersionedPersistence *vp, VersionedPersistenceChecksums *cs)
{
    return 0;
}

static int
update_checksums(VersionedPersistence *vp)
{
    return 0;
}

static int
store_header(VersionedPersistence *vp)
{
    return 0;
}

int
vp_open(VersionedPersistence *vp)
{
    vp->state = 0U;
    VersionedPersistenceChecksums cs = VP_CHECKSUMS_INIT;
    maybe(read_meta(vp, &cs));
    BIT_SET(vp->state, VP_STATE_META_CONSISTENT);
    const uint16_t read_version = vp_ref_version(
    cs = (VersionedPersistenceChecksums)VP_CHECKSUMS_INIT;
    maybe(verify_payload(vp, &cs));
    BIT_SET(vp->state, VP_STATE_PAYLOAD_CONSISTENT);
    return 0;
}

int
vp_format(VersionedPersistence *vp, const unsigned char c)
{
    maybe(vp_memset(vp, c));
    maybe(vp_store_meta(vp));
    return 0;
}

int
vp_invalidate(VersionedPersistence *vp, const unsigned int parts)
{
    return 0;
}

int
vp_store_meta(VersionedPersistence *vp)
{
    maybe(store_header(vp));
    maybe(update_checksums(vp));
    return 0;
}

int
vp_memset(VersionedPersistence *vp, const unsigned char c)
{
    return 0;
}

int
vp_fetch_part(void *dst, VersionedPersistence *vp, size_t offset, size_t n)
{
    return 0;
}

int
vp_store_part(VersionedPersistence *vp, const void *src, size_t offset, size_t n)
{
    return 0;
}

int
vp_fetch(void *dst, VersionedPersistence *vp)
{
    return 0;
}

int
vp_store(VersionedPersistence *vp, const void *src)
{
    return 0;
}

#pragma GCC diagnostic pop
