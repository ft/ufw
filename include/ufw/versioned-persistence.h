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
    Source source;
    Sink sink;
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

typedef struct versioned_persistence {
    uint16_t state;
    struct vp_meta meta;
    struct vp_cache cache;
    struct vp_access data;
    struct vp_checksum chksum;
    ByteBuffer *buffer;
} VersionedPersistence;

/*
 * The VP_STATE_* macros are bits in the state member of VersionedPersistence.
 * They are set by vp_open(). META_CONSISTENT reflects is the meta data section
 * of a block could be verified by its checksum.
 *
 * LENGTH_COMPATIBLE is set if the length meta field in persistent memory
 * matches the length field in the VersionedPersistence instance.
 * VERSION_COMPATIBLE is like LENGTH_COMPATIBLE, but for the version field.
 *
 * PAYLOAD_CONSISTENT is like META_CONSISTENT, just for the payload section. It
 * can only be set if LENGTH_COMPATIBLE is set.
 *
 * PAYLOAD_COMPATIBLE is set if both LENGTH_COMPATIBLE and VERSION_COMPATIBLE
 * are set, as a convenience to the user.
 */

/** Indicate that a block's meta field could be verified */
#define VP_STATE_META_CONSISTENT    BIT(0)
/** Indicate that a block's payload field could be verified */
#define VP_STATE_PAYLOAD_CONSISTENT BIT(1)
/** A block's payload is compatible in length and version */
#define VP_STATE_PAYLOAD_COMPATIBLE BIT(2)
/** A block's payload is compatible in length */
#define VP_STATE_LENGTH_COMPATIBLE  BIT(3)
/** A block's payload is compatible in version */
#define VP_STATE_VERSION_COMPATIBLE BIT(4)

/** Size of the meta field of a block in bytes */
#define VP_SIZE_META ( (2*sizeof(vp_chksum)) + (2*sizeof(uint16_t)) )

/** Bit mask to address the meta field of a block */
#define VP_DATA_META    BIT(0)
/** Bit mask to address the payload field of a block */
#define VP_DATA_PAYLOAD BIT(1)

#define VP_SECTION_SIZE(n) ((n) + VP_SIZE_META)
#define VP_FULL_INIT(ADDR, SIZE, VERSION, BUF, SOURCE, SINK, CHKSUM, INIT) \
    {   .state = 0u,                                                       \
        .meta.address = (ADDR),                                            \
        .meta.size = VP_SECTION_SIZE(SIZE),                                \
        .meta.version = (VERSION),                                         \
        .data.source = (SOURCE),                                           \
        .data.sink = (SINK),                                               \
        .chksum.initial= (INIT),                                           \
        .chksum.process= (CHKSUM),                                         \
        .buffer = (BUF) }

#define VP_INIT(ADDR, SIZE, VERSION, BUF, SOURCE, SINK) \
    VP_FULL_INIT(                                       \
        ADDR, SIZE, VERSION, BUF, SOURCE, SINK,         \
        ufw_crc16_arc, CRC16_ARC_INITIAL)

#define VP_SIMPLE_INIT(ADDR, SIZE, VERSION, SOURCE, SINK)       \
    VP_INIT(ADDR, SIZE, VERSION, NULL, SOURCE, SINK)

int vp_open(VersionedPersistence *vp);
int vp_format(VersionedPersistence *vp, unsigned char c);
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
