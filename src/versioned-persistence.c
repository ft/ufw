/*
 * Copyright (c) 2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file versioned-persistence.c
 * @brief Persistence storage with version information
 */

#include <ufw/binary-format.h>
#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>
#include <ufw/endpoints.h>
#include <ufw/versioned-persistence.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push

static int
read_meta(VersionedPersistence *vp)
{
    unsigned char buf[VP_SIZE_META];
    const ssize_t rc = source_get_chunk(&vp->data.fetch, buf, VP_SIZE_META);
    if (rc < 0) {
        return (int)rc;
    }

    vp->chksum.meta_read = vp_ref_chksum(buf);
    vp->chksum.meta_calculated =
        vp->chksum.process(vp->chksum.initial, buf+sizeof(vp_chksum),
                           VP_SIZE_META-sizeof(vp_chksum));

    if (vp->chksum.meta_read != vp->chksum.meta_calculated) {
        return -EBADFD;
    }

    return 0;
}

int
vp_open(VersionedPersistence *vp)
{
    Source *src = &vp->data.fetch;
    Sink *snk = &vp->data.store;
    AddressableSource *srcdrv = src->driver;
    AddressableSink *snkdrv = snk->driver;
    srcdrv->address = snkdrv->address = vp->meta.address;
    const int metarc = read_meta(vp);
    if (metarc < 0) {
        return metarc;
    }
    return 0;
}

int
vp_format(VersionedPersistence *vp, const unsigned int parts)
{
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
