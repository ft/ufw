/*
 * Copyright (c) 2019 ufw workers, All rights reserved.
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

#ifndef INC_PERSISTENT_STORE_H
#define INC_PERSISTENT_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef uint16_t (*PersistentChksum16)(const unsigned char*, size_t, uint16_t);
typedef uint32_t (*PersistentChksum32)(const unsigned char*, size_t, uint32_t);
typedef size_t (*PersistentBlockRead)(void*, uint32_t, size_t);
typedef size_t (*PersistentBlockWrite)(uint32_t, const void*, size_t);

typedef enum persistent_checksum_type {
    PERSISTENT_CHECKSUM_16BIT,
    PERSISTENT_CHECKSUM_32BIT
} PersistentChecksumType;

typedef union persistent_checksum {
    uint16_t sum16;
    uint32_t sum32;
} PersistentChecksum;

typedef struct persistent_storage {
    struct {
        uint32_t address;
        size_t size;
    } data;
    struct {
        uint32_t address;
        PersistentChecksumType type;
        PersistentChecksum initial;
        union {
            PersistentChksum16 c16;
            PersistentChksum32 c32;
        } process;
    } checksum;
    struct {
        unsigned char *data;
        size_t size;
    } buffer;
    struct {
        PersistentBlockRead read;
        PersistentBlockWrite write;
    } block;
} PersistentStorage;

typedef enum persistent_access {
    PERSISTENT_ACCESS_SUCCESS = 0,
    PERSISTENT_ACCESS_INVALID_DATA,
    PERSISTENT_ACCESS_IO_ERROR,
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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_PERSISTENT_STORE_H */
