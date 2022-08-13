/*
 * Copyright (c) 2019-2021 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file persistent-storage.h
 * @brief Persistent storage API
 *
 * A common task in embedded systems is to store data in persistent storage
 * (like flash or eeprom memory), for it to survive system reboots. For this to
 * be useful, it is required for a client (the firmware) to decide whether or
 * not the contents of a field of data is valid.
 */

#ifndef INC_UFW_PERSISTENT_STORE_H
#define INC_UFW_PERSISTENT_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Data type for 16-bit checksum processing
 *
 * Arguments are pointer to source buffer, the size of that buffer and an
 * initial value to the checksum calculation.
 */
typedef uint16_t (*PersistentChksum16)(const unsigned char*, size_t, uint16_t);

/** Like PersistentChksum16 but for 32-bit checksum processing */
typedef uint32_t (*PersistentChksum32)(const unsigned char*, size_t, uint32_t);

/**
 * Datatype for reading data from a medium
 *
 * Takes a pointer to a buffer to read into, an address to read from, and the
 * number of characters to read from a data source.
 */
typedef size_t (*PersistentBlockRead)(void*, uint32_t, size_t);

/**
 * Datatype for writing data to a medium
 *
 * Takes an address to write to in a data sink, a pointer to a buffer to read
 * data from, and the number of characters to write.
 */
typedef size_t (*PersistentBlockWrite)(uint32_t, const void*, size_t);

/**
 * Datatype to distinguish between checksum types
 *
 * This has nothing to do with the actual algorithm being used, but rather the
 * width of the resulting checksum.
 */
typedef enum persistent_checksum_type {
    /** Signal 16-bit wide checksums */
    PERSISTENT_CHECKSUM_16BIT,
    /** Signal 32-bit wide checksums */
    PERSISTENT_CHECKSUM_32BIT
} PersistentChecksumType;

/** Container to hold checksums supported by PersistentChecksumType */
typedef union persistent_checksum {
    /** Access to 16-bit checksum value */
    uint16_t sum16;
    /** Access to 32-bit checksum value */
    uint32_t sum32;
} PersistentChecksum;

/**
 * Persistent storage abstraction
 *
 * This data type encapsulates the entirety of an instance of persistent
 * memory. This should be used as an opaque data type. It's contents are
 * subject to change at any point in time.
 */
typedef struct persistent_storage {
    struct {
        /** Address of the data portion of the storage, as passed to the read
         * and write callbacks */
        uint32_t address;
        /** Size of data portion in units of addresses with read and write
         * callbacks */
        size_t size;
    } data;
    struct {
        /** Checksum address as passed to the read and write callbacks */
        uint32_t address;
        /** Checksum size; in units of addresses with read and write
         * callbacks */
        size_t size;
        /** Checksum type used for persistent storage instance */
        PersistentChecksumType type;
        /** Initial value for checksum calculation */
        PersistentChecksum initial;
        /** Checksum processor callback, depending on checksum type */
        union {
            PersistentChksum16 c16;
            PersistentChksum32 c32;
        } process;
    } checksum;
    /** Optional buffer for checksum calculation from medium */
    struct {
        unsigned char *data;
        size_t size;
    } buffer;
    /** Block read and write access to medium */
    struct {
        PersistentBlockRead read;
        PersistentBlockWrite write;
    } block;
} PersistentStorage;

/** Data type encoding error conditions in persistent storage operations */
typedef enum persistent_access {
    /** Signal successful operation */
    PERSISTENT_ACCESS_SUCCESS = 0,
    /** Data portion could not be validated with stored checksum */
    PERSISTENT_ACCESS_INVALID_DATA,
    /** Read or write operation from or to medium failed */
    PERSISTENT_ACCESS_IO_ERROR,
    /** Address and size parameters to operation yielded out of bounds
     * address */
    PERSISTENT_ACCESS_ADDRESS_OUT_OF_RANGE
} PersistentAccess;

/*
 * Initialisation
 */

void persistent_init(PersistentStorage*,
                     size_t,
                     PersistentBlockRead,
                     PersistentBlockWrite);
void persistent_sum16(PersistentStorage*, PersistentChksum16, uint16_t);
void persistent_sum32(PersistentStorage*, PersistentChksum32, uint32_t);
void persistent_place(PersistentStorage*, uint32_t);
void persistent_buffer(PersistentStorage*, unsigned char*, size_t);

/*
 * Validation
 */

PersistentAccess persistent_validate(PersistentStorage*);

/*
 * Transfer
 */

PersistentAccess persistent_fetch(void*, PersistentStorage*);
PersistentAccess persistent_store(PersistentStorage*, const void*);

PersistentAccess persistent_fetch_part(void*, PersistentStorage*,
                                       size_t, size_t);
PersistentAccess persistent_store_part(PersistentStorage*, const void*,
                                       size_t, size_t);
PersistentAccess persistent_reset(PersistentStorage*, unsigned char);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_PERSISTENT_STORE_H */
