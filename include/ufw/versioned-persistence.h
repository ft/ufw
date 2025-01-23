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

struct vp_access {
    Source fetch;
    Sink store;
};

struct vp_meta {
    uint32_t address;
    uint16_t size;
    uint16_t version;
};

struct vp_checksum {
    vp_chksum initial;
    vp_chksum_fnc process;
};

#define VP_STATE_META_CONSISTENT    BIT(0)
#define VP_STATE_PAYLOAD_CONSISTENT BIT(1)
#define VP_STATE_PAYLOAD_COMPATIBLE BIT(2)

typedef struct versioned_persistence {
    uint16_t state;
    struct vp_meta meta;
    struct vp_access data;
    struct vp_checksum chksum;
    ByteBuffer *buffer;
} VersionedPersistence;

#define VP16_INIT(ADDR, SIZE, VERSION, FETCH, STORE, CHKSUM, INIT)  \
    {   .state = 0u,                                                \
        .meta.address = (ADDR),                                     \
        .meta.size = (SIZE),                                        \
        .meta.version = (VERSION),                                  \
        .data.fetch = (FETCH),                                      \
        .data.store = (STORE),                                      \
        .chksum.initial= (INIT),                                    \
        .chksum.process= (CHKSUM),                                  \
        .buffer = NULL }

#if 0
/**
 * Variant of PersistentStorage that adds content versioning
 *
 * PersistentStorage offers a low-level abstraction, that allows to store data
 * in persistent memory, so it can be accessed across many system starts, while
 * ensuring that the data retrieved is consistent with regard to a checksum
 * algorithm.
 *
 * What PersistentStorage does not allow is to tell if the data stored is
 * compatible with the active application: The software accessing the
 * persistent memory may change in the course of time. From version to version,
 * data that needs to be stored across system starts may change content and
 * semantics. Extending PersistentStorage with versioning information is this
 * type's job.
 *
 * In persistent memory, this can be thought of as this:
 *
 * @code
 * struct {
 *     Checksum chksum;   // 16 or 32 bits.
 *     uint16_t size;     // Size of data section.
 *     uint16_t version;  // Number identifying the version of data section.
 *     unsigned char data[size];
 * };
 * @endcode
 *
 * This type is implemented on top of PersistentStorage, and can be accessed
 * through it, even if compatibility cannot be ensured, but consistency can.
 *
 * Consistency is established by PersistentStorage. Compatibility is assumed if
 * `version.expected` equals `version.stored`.
 */
typedef struct versioned_persistence_old {
    /** Underlying PersistentStorage instance */
    PersistentStorage ps;
    struct {
        /** This is specifies the version the software is compatible with. */
        uint16_t expected
        /** This is the version retrieved from the persistent memory. */
        uint16_t stored;
    } version;
    struct {
        /** Calculated size of the data section in memory */
        size_t size;
        /**
         * Offset of data section from the start of the memory the
         * PersistentStorage instance accesses.
         */
        off_t offset;
    } data;
} VersionedPersistenceOld;
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_VERSIONED_PERSISTENCE_H_91da0966 */
