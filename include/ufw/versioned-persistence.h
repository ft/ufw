/*
 * Copyright (c) 2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file versioned-persistence.h
 * @brief Persistence storage with version information
 */

#ifndef INC_UFW_VERSIONED_PERSISTENCE_H_91da0966
#define INC_UFW_VERSIONED_PERSISTENCE_H_91da0966

#include <ufw/bit-operations.h>
#include <ufw/byte-buffer.h>
#include <ufw/endpoints.h>
#include <ufw/persistent-storage.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef uint16_t vp_chksum;
typedef vp_chksum (*vp_chksum_fnc)(vp_chksum, const void*, size_t);
#define vp_ref_chksum bf_ref_u16n

struct vp_access {
    Source fetch;
    Sink store;
};

struct vp_meta {
    uint32_t address;
    uint16_t size;
    uint16_t version;
};

struct vp_cache {
    uint16_t size;
    uint16_t version;
};

struct vp_checksum {
    vp_chksum initial;
    vp_chksum_fnc process;
    vp_chksum meta_read;
    vp_chksum meta_calculated;
    vp_chksum payload_read;
    vp_chksum payload_calculated;
};

#define VP_STATE_META_CONSISTENT    BIT(0)
#define VP_STATE_PAYLOAD_CONSISTENT BIT(1)
#define VP_STATE_PAYLOAD_COMPATIBLE BIT(2)

#define VP_SIZE_META ( (2*sizeof(vp_chksum)) + (2*sizeof(uint16_t)) )

#define VP_DATA_META    BIT(0)
#define VP_DATA_PAYLOAD BIT(1)

typedef struct versioned_persistence {
    uint16_t state;
    struct vp_meta meta;
    struct vp_cache cache;
    struct vp_access data;
    struct vp_checksum chksum;
    ByteBuffer *buffer;
} VersionedPersistence;

#define VP_SECTION_SIZE(n) ((n) + VP_SIZE_META)
#define VP_FULL_INIT(ADDR, SIZE, VERSION, BUF, FETCH, STORE, CHKSUM, INIT) \
    {   .state = 0u,                                                       \
        .meta.address = (ADDR),                                            \
        .meta.size = VP_SECTION_SIZE(SIZE),                                \
        .meta.version = (VERSION),                                         \
        .data.fetch = (FETCH),                                             \
        .data.store = (STORE),                                             \
        .chksum.initial= (INIT),                                           \
        .chksum.process= (CHKSUM),                                         \
        .buffer = (BUF) }

#define VP_INIT(ADDR, SIZE, VERSION, BUF, FETCH, STORE) \
    VP_FULL_INIT(                                       \
        ADDR, SIZE, VERSION, BUF, FETCH, STORE,         \
        ufw_crc16_arc, CRC16_ARC_INITIAL)

#define VP_SIMPLE_INIT(ADDR, SIZE, VERSION, FETCH, STORE)       \
    VP_INIT(ADDR, SIZE, VERSION, NULL, FETCH, STORE)

int vp_open(VersionedPersistence *vp);
int vp_invalidate(VersionedPersistence *vp, unsigned int parts);

int vp_store_meta(VersionedPersistence *vp);
int vp_memset(VersionedPersistence *vp, unsigned char c);

int vp_fetch_part(void *dst, VersionedPersistence *vp, size_t offset, size_t n);
int vp_store_part(VersionedPersistence *vp, const void *src, size_t offset, size_t n);

int vp_fetch(void *dst, VersionedPersistence *vp);
int vp_store(VersionedPersistence *vp, const void *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_VERSIONED_PERSISTENCE_H_91da0966 */
