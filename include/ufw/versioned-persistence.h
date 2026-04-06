/*
 * Copyright (c) 2025-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup vpstorage Versioned Persistence
 *
 * The old PersistentStorage module implements a system that can check the
 * consistency of persistently stored data. What it does not concern itself
 * with is the compatibility of the data stored and the firmware system running
 * presently. In order to address both concerns, and improve on some suboptimal
 * API choices in PersistentStorage, this module was introduced.
 *
 * The meta data block used in this system looks like this:
 *
 *     u16: Header Checksum  (a)
 *     u16: Payload Version  (b)
 *     u16: Payload Size     (c)
 *     u16: Payload Checksum (d)
 *
 * `(a)` covers `(b)`, `(c)`, and `(d)`. The payload block `(e)` follows the
 * meta data header immediately, and `(d)` covers the entirety of `(e)`.
 *
 * The choice of 16 bits for specifying payload size, means that the system is
 * limited to memory blocks of 64KiB. For use in current micro-controller based
 * applications, that is more than enough, and should be enough for the
 * foreseeable future. Since the size of the section large determines how
 * expensive a block-write operation into the section is (each store has to
 * update the payload checksum), this should be a reasonable range to offer.
 *
 * The system tries to establish two properties between data stored in a memory
 * device and the idea that an accessing system has about this memory:
 *
 * - Consistency: The data in the memory is consistent with regard to the time
 *                it was last stored. This is implemented by use of checksums.
 *                The default checksum used is CRC-16-ARC.
 *
 * - Compatibility: The data stored in memory and the assumptions about this
 *                  memory in the accessing system have the same semantics.
 *                  This is done by storing two meta-data items: The length of
 *                  the data, and a version number for the semantics of the
 *                  data.
 *
 * If memory is persistent, it is possible for the accessing system to change
 * when the persistent memory does not (think: Software updates). A piece of
 * data in memory can therefore be consistent, but incompatible with an
 * accessing system. If consistency cannot be establshed, no determination on
 * compatibility can be made. Both properties can be queried independently,
 * which allows for users to implement update logic in case consistent, but
 * incompatible memory is encountered.
 *
 * The most important parts of the API are these:
 *
 * - vp_load() — Load an instance from storage
 * - vp_usable() — Check if a stored datum is exactly usable with an instance
 * - vp_format() — Initially format an instance in storage
 * - vp_fetch() — Block read for memory associated with an instance
 * - vp_store() — Block read for memory associated with an instance
 *
 * Let's assume that an instance `vp` of `VersionedPersistence` is associated
 * to some EEPROM peripheral to store a firmware system's configuration data.
 * This association is created by a pair of endpoints (a Source object and a
 * Sink object). Both of these must implement the `seek` extensions to
 * endpoints, since the system needs to be able to access the memory in random
 * order.
 *
 * When starting out, the EEPROM memory will have some value. This can be
 * completely random, but with EEPROM it often is the case that all bits in the
 * memory will be set to one. Let `struct config` be the type of data that can
 * carry a system's configuration. Note that this first example uses vp_save()
 * which is a shorthand for vp_format(), vp_store(), and vp_load(), to force a
 * certain datum in a given specification into storage.
 *
 * @code
 *  #include <ufw/versioned-persistence.h>
 *  // …
 *  #define CFG_ADDRESS 0x10
 *  #define CFG_VERSION 1
 *  #define CFG_SIZE    sizeof(struct config)
 *  // …
 *  VersionedPersistence vp =
 *      VP_SIMPLE_INIT(CFG_ADDRESS, CFG_SIZE, CFG_VERSION,
 *                     eeprom_read, eeprom_write);
 *  // …
 *  struct config cfg = DEFAULT_CONFIGURATION;
 *  const int saverc = vp_save(&vp, &cfg, sizeof(cfg));
 *  // error handling…
 * @endcode
 *
 * This will establish semantics specification of `CFG_VERSION` and
 * `CFG_LENGTH` in the `vp` object at construction time. vp_save() will put
 * this specification into EEPROM via `eeprom_write` (which has to implement
 * `ufw`'s `Sink` interface with its `seek` extension), with the meta data
 * header of the section starting at address `CFG_ADDRESS`. It will also
 * transfer the data behind the `cfg` datum (of type `struct config`) into the
 * payload part of the section. The checksums in the meta data header will be
 * updated to reflect the data that was just stored. `vp` is usable after this
 * call.
 *
 * Obviously, you do not want to call vp_save() unconditionally, since that
 * would always store the data in `DEFAULT_CONFIGURATION` no matter what.
 *
 * @code
 *  VersionedPersistence vp =
 *      VP_SIMPLE_INIT(CFG_ADDRESS, CFG_SIZE, CFG_VERSION,
 *                     eeprom_read, eeprom_write);
 *  // …
 *  struct config cfg;
 *  const int loadrc = vp_load(&vp);
 *  // error handling…
 *  if (vp_usable(&vp) == false) {
 *      struct config def = DEFAULT_CONFIGURATION;
 *      const int saverc = vp_save(&vp, &def);
 *      // error handling…
 *  }
 *  const int fetchrc = vp_fetch(&vp, &cfg, 0U, sizeof(cfg));
 *  // error handling…
 * @endcode
 *
 * This example is a little more sophisticated, and might be enough if the
 * semantics of the configuration block never change. If this does not hold
 * true, the system allows to query compatibility information. With this, it is
 * possible to implement conversion paths from one set of semantics to another.
 *
 * These procedures depend on the nature of the changes made in semantics. Pure
 * extensions are easy to upgrade and even reasonably easy to downgrade with.
 * Complete changes in semantics can be arbitrarily complicated; and downgrades
 * (uploading an older version of firmware that does not know about the newer
 * semantics), can be next to impossible. Here you can still detect the
 * situation and establish the default behaviour of the old system.
 *
 * On the matter of bootstrapping:
 *
 * Bootstrapping is the process of bringing a completely fresh piece of
 * hardware, with all chips in a blank factory state, into a state that can and
 * will run a production system. And checksums are not magic. By their nature
 * they break down an arbitrarily large piece of data into a fixed width datum.
 * This means that in general, two different pieces of data can yield the same
 * checksum. With this system using chained checksums between header and
 * payload data, collisions are fairly unlikely. Users should still consider
 * bootstrap processes, that brings persistent memory into a known state, and
 * only then start putting data into the medium. It is also worth considering
 * to use a section of persistent memory to store the version of the firmware
 * running in a system. That way, up- and downgrades can be detected, and
 * actions can be taken should they be required.
 *
 * @{
 *
 * @file versioned-persistence.h
 * @brief Persistence storage with version information
 *
 * @}
 */

#ifndef INC_UFW_VERSIONED_PERSISTENCE_H_91da0966
#define INC_UFW_VERSIONED_PERSISTENCE_H_91da0966

#include <stdint.h>

#include <ufw/binary-format.h>
#include <ufw/bit-operations.h>
#include <ufw/byte-buffer.h>
#include <ufw/endpoints.h>
#include <ufw/persistent-storage.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VP_CHKSUM_MAX UINT16_MAX
typedef uint16_t vp_chksum;
typedef uint16_t vp_length;
typedef uint16_t vp_version;
typedef vp_chksum (*vp_chksum_fnc)(vp_chksum, const void*, size_t);

/** Endpoint pair and address to access physical storage */
struct vp_access {
    uint32_t address;
    Source source;
    Sink sink;
};

/** Reference to checksum implementation */
struct vp_checksum {
    vp_chksum initial;
    vp_chksum_fnc process;
};

/** Meta data for storage, size and version */
struct vp_meta {
    vp_length length;
    vp_version version;
};

struct vp_checksum_results {
    vp_chksum header;
    vp_chksum payload;
};

/** Size of the meta field of a block in bytes */
#define VP_SIZE_META                                                    \
    ( (2*sizeof(vp_chksum)) + sizeof(vp_version) + sizeof(vp_length) )

#define VP_OFFSET_HEADER_CHKSUM  0
#define VP_OFFSET_VERSION        (VP_OFFSET_HEADER_CHKSUM + sizeof(vp_chksum))
#define VP_OFFSET_LENGTH         (VP_OFFSET_VERSION       + sizeof(vp_version))
#define VP_OFFSET_PAYLOAD_CHKSUM (VP_OFFSET_LENGTH        + sizeof(vp_length))

/**
 * Control data for versioned persistent memory
 */
typedef struct versioned_persistence {
    /** Raw meta data header block buffer */
    unsigned char metablock[VP_SIZE_META];
    /** Block state determined while initialising physical memory */
    uint16_t state;
    /** Implementation of physical storage access */
    struct vp_access data;
    /**
     * Expected meta-data specification
     *
     * This part is to be specified by the user, and the system will check if
     * the stored data (dumped into "cache"), matches this specification.
     */
    struct vp_meta spec;
    /** Checksum implementation */
    struct vp_checksum chksum;
    /** Checksum results: Header and payload */
    struct vp_checksum_results result;
    /** Optional utility buffer for section */
    ByteBuffer *buffer;
} VersionedPersistence;

/*
 * The VP_STATE_* macros are bits in the state member of VersionedPersistence.
 * They are set by vp_load(). META_CONSISTENT reflects is the meta data section
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
 *
 * META_CACHE_ACTIVE is set if version and length in "metablock" is loaded from
 * something useful. META_CACHE_FROM_STORAGE is set if META_CACHE_ACTIVE was
 * set because of reading header information from storage.
 */

/** Indicate that instance's specification is in meta-data cache */
#define VP_STATE_META_CACHE_ACTIVE        BIT(0)
/** Indicate if meta-data cache was loaded from storage, rather than spec */
#define VP_STATE_META_CACHE_FROM_STORAGE  BIT(1)
/** Indicate that a block's meta field could be verified */
#define VP_STATE_META_CONSISTENT          BIT(2)
/** Indicate that a block's payload field could be verified */
#define VP_STATE_PAYLOAD_CONSISTENT       BIT(3)
/** A block's payload is compatible in length and version */
#define VP_STATE_PAYLOAD_COMPATIBLE       BIT(4)
/** A block's payload is compatible in length */
#define VP_STATE_LENGTH_COMPATIBLE        BIT(5)
/** A block's payload is compatible in version */
#define VP_STATE_VERSION_COMPATIBLE       BIT(6)

/** Bit mask to address the meta field of a block */
#define VP_DATA_META    BIT(0)
/** Bit mask to address the payload field of a block */
#define VP_DATA_PAYLOAD BIT(1)

/** Compute the size of a VersionedPersistence section given a payload size */
#define VP_SECTION_SIZE(n) ((n) + VP_SIZE_META)

/** Full static VersionedPersistence instance initialiser */
#define VP_FULL_INIT(ADDR, SIZE, VERSION, BUF, SOURCE, SINK, CHKSUM, INIT) \
    {   .state = 0u,                                                       \
        .data.address = (ADDR),                                            \
        .spec.length = (SIZE),                                             \
        .spec.version = (VERSION),                                         \
        .data.source = (SOURCE),                                           \
        .data.sink = (SINK),                                               \
        .chksum.initial= (INIT),                                           \
        .chksum.process= (CHKSUM),                                         \
        .buffer = (BUF) }

/** Common static VersionedPersistence instance initialiser */
#define VP_INIT(ADDR, SIZE, VERSION, BUF, SOURCE, SINK) \
    VP_FULL_INIT(                                       \
        ADDR, SIZE, VERSION, BUF, SOURCE, SINK,         \
        ufw_crc16_arc, CRC16_ARC_INITIAL)

/** Simple static VersionedPersistence instance initialiser */
#define VP_SIMPLE_INIT(ADDR, SIZE, VERSION, SOURCE, SINK)       \
    VP_INIT(ADDR, SIZE, VERSION, NULL, SOURCE, SINK)

/* Query sizes of VersionedPersistence instances */
size_t vp_section_size(const VersionedPersistence *vp);
size_t vp_spec_size(const VersionedPersistence *vp);

/* Storage initialisation */
int vp_format(VersionedPersistence *vp);
int vp_memset(VersionedPersistence *vp, unsigned char c,
              size_t offset, size_t n);

/* Block transfer */
ssize_t vp_fetch(VersionedPersistence *vp, void *dst, size_t offset, size_t n);
ssize_t vp_store(VersionedPersistence *vp,
                 const void *src, size_t offset, size_t n);

/* Load/save and manual invalidation */
int vp_load(VersionedPersistence *vp);
ssize_t vp_save(VersionedPersistence *vp, const void *src, size_t n);
int vp_invalidate(VersionedPersistence *vp, unsigned int parts);

/**
 * Determine if an instance is usable with its associated memory
 *
 * Usable means, that consistency for the meta data header and the payload
 * parts can be established, and that the semantics in terms of length and
 * version match between the VersionedPersistence instance and the data stored
 * inside of the associated memory section.
 *
 * @param  vp  VersionedPersistence instance to query
 *
 * @return True if the payload in memory is usable with the semantics specified
 *         in the VersionedPersistence instance `vp`; false otherwise.
 * @sideeffects None
 */
static inline bool
vp_usable(const VersionedPersistence *vp)
{
    /*
     * META_CONSISTENT reflects that the meta data header could be verified for
     * consistency. PAYLOAD_COMPATIBLE means that length and version as stored
     * in the meta-data header are compatible with vp->spec. Finally, the
     * PAYLOAD_CONSISTENT flag reflects that the payload data could be verified
     * with its checksum stored in the meta data header.
     */
    return BIT_ISSET(vp->state,
                     VP_STATE_META_CONSISTENT    |
                     VP_STATE_PAYLOAD_COMPATIBLE |
                     VP_STATE_PAYLOAD_CONSISTENT);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_VERSIONED_PERSISTENCE_H_91da0966 */
