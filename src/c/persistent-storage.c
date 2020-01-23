/*
 * Copyright (c) 2019-2020 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file persistent-storage.c
 * @brief Persistent storage implementation
 */

#include <c/persistent-storage.h>

struct maybe_sum {
    PersistentAccess access;
    PersistentChecksum value;
};

static inline size_t
checksum_size(const PersistentStorage *store)
{
    switch (store->checksum.type) {
    case PERSISTENT_CHECKSUM_16BIT:
        return sizeof(uint16_t);
    case PERSISTENT_CHECKSUM_32BIT: /* FALLTHROUGH */
    default:
        return sizeof(uint32_t);
    }
}

static inline void
set_data_address(PersistentStorage *store)
{
    store->data.address = store->checksum.address + store->checksum.size;
}

static uint16_t
trivialsum(const unsigned char *data, size_t n, uint16_t init)
{
    for (size_t i = 0u; i < n; ++i) {
        init += data[i];
    }

    return init;
}


/**
 * Set up PersistentStorage to use a given 16-bit checksum algorithm
 *
 * @param  store   Pointer to the instance to initialise
 * @param  f       Callback function that implements checksum algorithm
 * @param  init    Initial value to feed into the checksum algorithm
 *
 * @return void
 * @sideeffects store is mutated as advertised
 */
void
persistent_sum16(PersistentStorage *store, PersistentChksum16 f, uint16_t init)
{
    store->checksum.initial.sum16 = init;
    store->checksum.type = PERSISTENT_CHECKSUM_16BIT;
    store->checksum.size = checksum_size(store);
    store->checksum.process.c16 = f;
    set_data_address(store);
}

/**
 * Set up PersistentStorage to use a given 32-bit checksum algorithm
 *
 * @param  store   Pointer to the instance to initialise
 * @param  f       Callback function that implements checksum algorithm
 * @param  init    Initial value to feed into the checksum algorithm
 *
 * @return void
 * @sideeffects store is mutated as advertised
 */
void
persistent_sum32(PersistentStorage *store, PersistentChksum32 f, uint32_t init)
{
    store->checksum.initial.sum32 = init;
    store->checksum.type = PERSISTENT_CHECKSUM_32BIT;
    store->checksum.size = checksum_size(store);
    store->checksum.process.c32 = f;
    set_data_address(store);
}

/**
 * Initialise PersistentStorage instance
 *
 * The default checksum is a trivial mod-16 addition across all words stored in
 * managed data portion. You may want to assign a more sophisticated algorithm
 * using ‘persistent_sum16’ or ‘persistent_sum32’.
 *
 * @param  store   Pointer to the instance to initialise
 * @param  size    Size of data portion to manage
 * @param  rd      Callback function for block reads to medium
 * @param  wr      Callback function for block writes to medium
 *
 * @return void
 * @sideeffects store is mutated as advertised
 */
void
persistent_init(PersistentStorage *store,
                const size_t size,
                const PersistentBlockRead rd,
                const PersistentBlockWrite wr)
{
    /* Initialise checksum first; data init needs address and type of it. The
     * initialisation order of the rest doesn't really matter. */
    store->checksum.address = 0u;
    persistent_sum16(store, trivialsum, 0u);

    set_data_address(store);
    store->data.size = size;

    store->block.read = rd;
    store->block.write = wr;

    store->buffer.size = 1;
    store->buffer.data = NULL;
}

static PersistentChecksum
persistent_checksum(const PersistentStorage *store, const void *src)
{
    const unsigned char *from = src;
    PersistentChecksum rv;

    switch (store->checksum.type) {
    case PERSISTENT_CHECKSUM_16BIT:
        rv.sum16 = store->checksum.process.c16(
            from, store->data.size, store->checksum.initial.sum16);
        break;
    case PERSISTENT_CHECKSUM_32BIT: /* FALLTHROUGH */
    default:
        rv.sum32 = store->checksum.process.c32(
            from, store->data.size, store->checksum.initial.sum32);
        break;
    }

    return rv;
}

/**
 * Move PersistentStorage inside of target medium
 *
 * By default an instance is placed at address zero in the medium accessed by
 * the read and write callbacks. This function allows the user the define the
 * address to place this particular instance.
 *
 * @param  store    Pointer to the instance to initialise
 * @param  address  Address to place instance in on target medium
 *
 * @return void
 * @sideeffects store is mutated as advertised
 */
void
persistent_place(PersistentStorage *store, uint32_t address)
{
    store->checksum.address = address;
    set_data_address(store);
}

/**
 * Supply PersistentStorage with auxiliary buffer
 *
 * By default, when reading data from the target medium in bulk in order to
 * calculate the checksum of the instance's data part, the system reads data
 * word by word.
 *
 * Often times it is beneficial for performance to read data in larger chunks.
 * This function allows the user to assign a chunk of memory to the system for
 * it to be used in such situations.
 *
 * @param  store    Pointer to the instance to configure
 * @param  buffer   Pointer to the buffer to assign to the instance
 * @param  n        Size of buffer
 *
 * @return void
 * @sideeffects store is mutated as advertised
 */
void
persistent_buffer(PersistentStorage *store, unsigned char *buffer, size_t n)
{
    store->buffer.data = buffer;
    store->buffer.size = n;
}

static struct maybe_sum
persistent_calculate_checksum(PersistentStorage *store)
{
    struct maybe_sum rv;
    unsigned char buf;
    unsigned char *data;
    size_t bsize;

    rv.value = store->checksum.initial;
    rv.access = PERSISTENT_ACCESS_IO_ERROR;

    if (store->buffer.data != NULL) {
        data = store->buffer.data;
        bsize = store->buffer.size;
    } else {
        data = &buf;
        bsize = 1u;
    }

    size_t rest = store->data.size;
    uint32_t address = store->data.address;

    while (rest > 0u) {
        const size_t toget = (rest > bsize) ? bsize : rest;
        const size_t n = store->block.read(data, address, toget);

        if (n != toget) {
            return rv;
        }

        switch (store->checksum.type) {
        case PERSISTENT_CHECKSUM_16BIT:
            rv.value.sum16 = store->checksum.process.c16(data, bsize,
                                                         rv.value.sum16);
            break;
        case PERSISTENT_CHECKSUM_32BIT: /* FALLTHROUGH */
        default:
            rv.value.sum32 = store->checksum.process.c32(data, bsize,
                                                         rv.value.sum32);
            break;
        }

        rest -= toget;
        address += toget;
    }

    rv.access = PERSISTENT_ACCESS_SUCCESS;
    return rv;
}

static PersistentAccess
persistent_store_checksum(PersistentStorage *store, PersistentChecksum sum)
{
    const size_t size = store->checksum.size;
    const uint32_t address = store->checksum.address;

    size_t n;
    if (store->checksum.type == PERSISTENT_CHECKSUM_32BIT) {
        n = store->block.write(address, &sum.sum32, size);
    } else {
        n = store->block.write(address, &sum.sum16, size);
    }

    if (n != size) {
        return PERSISTENT_ACCESS_IO_ERROR;
    }

    return PERSISTENT_ACCESS_SUCCESS;
}

static struct maybe_sum
persistent_fetch_checksum(PersistentStorage *store)
{
    const size_t size = store->checksum.size;
    const uint32_t address = store->checksum.address;
    struct maybe_sum rv;
    rv.access = PERSISTENT_ACCESS_SUCCESS;

    size_t n;
    if (store->checksum.type == PERSISTENT_CHECKSUM_32BIT) {
        n = store->block.read(&rv.value.sum32, address, size);
    } else {
        n = store->block.read(&rv.value.sum16, address, size);
    }

    if (n != size) {
        rv.access = PERSISTENT_ACCESS_IO_ERROR;
    }

    return rv;
}

static bool
persistent_match(const PersistentStorage *store,
                 PersistentChecksum a, PersistentChecksum b)
{
    switch (store->checksum.type) {
    case PERSISTENT_CHECKSUM_16BIT:
        return a.sum16 == b.sum16;
    case PERSISTENT_CHECKSUM_32BIT: /* FALLTHROUGH */
    default:
        return a.sum32 == b.sum32;
    }
}

/**
 * Validate the a PersistentStorage instance
 *
 * Read the checksum stored, recalculate over the managed data portion and
 * compare.
 *
 * @return PERSISTENT_ACCESS_SUCCESS in case validation succeeded;
 *         PERSISTENT_ACCESS_INVALID_DATA in case it didn't; and
 *         PERSISTENT_ACCESS_IO_ERROR in case IO failed.
 * @sideeffects None, except the IO to the medium. No mutation of data.
 */
PersistentAccess
persistent_validate(PersistentStorage *store)
{
    const struct maybe_sum stored = persistent_fetch_checksum(store);
    if (stored.access != PERSISTENT_ACCESS_SUCCESS) {
        return stored.access;
    }
    const struct maybe_sum calculated = persistent_calculate_checksum(store);
    if (calculated.access != PERSISTENT_ACCESS_SUCCESS) {
        return calculated.access;
    }
    const bool valid = persistent_match(store, stored.value, calculated.value);
    return valid ? PERSISTENT_ACCESS_SUCCESS : PERSISTENT_ACCESS_INVALID_DATA;
}

/**
 * Fetch part of the data portion of PersistentStorage instance
 *
 * @param  dst     Pointer to destination buffer to read into
 * @param  store   Pointer to PersistentStorage instance to use
 * @param  offset  Offset to start reading at inside of data portion
 * @param  n       Amount of words to read from data portion
 *
 * @return Error condition via PersistentAccess data type.
 * @sideeffects Fills dst as described.
 */
PersistentAccess
persistent_fetch_part(void *dst, PersistentStorage *store,
                      size_t offset, size_t n)
{
    if ((offset + n) > store->data.size) {
        return PERSISTENT_ACCESS_ADDRESS_OUT_OF_RANGE;
    }

    const uint32_t address = store->data.address + offset;
    const size_t read = store->block.read(dst, address, n);
    return (read == n) ? PERSISTENT_ACCESS_SUCCESS : PERSISTENT_ACCESS_IO_ERROR;
}

/**
 * Fetch all of the data portion of PersistentStorage instance
 *
 * @param  dst     Pointer to destination buffer to read into
 * @param  store   Pointer to PersistentStorage instance to use
 *
 * @return Error condition via PersistentAccess data type.
 * @sideeffects Fills dst as described.
 */
PersistentAccess
persistent_fetch(void *dst, PersistentStorage *store)
{
    return persistent_fetch_part(dst, store, 0, store->data.size);
}

/**
 * Store part of the data portion into PersistentStorage instance
 *
 * @param  store   Pointer to PersistentStorage instance to use
 * @param  src     Pointer to source buffer to read from
 * @param  offset  Offset to start storing to inside of data portion
 * @param  n       Amount of words to put into data portion
 *
 * @return Error condition via PersistentAccess data type.
 * @sideeffects Fills data portion of PersistentStorage instance as described.
 */
PersistentAccess
persistent_store_part(PersistentStorage *store, const void *src,
                      size_t offset, size_t n)
{
    if ((offset + n) > store->data.size) {
        return PERSISTENT_ACCESS_ADDRESS_OUT_OF_RANGE;
    }

    const uint32_t address = store->data.address + offset;
    const size_t stored = store->block.write(address, src, n);
    if (stored != n) {
        return PERSISTENT_ACCESS_IO_ERROR;
    }

    PersistentChecksum sum;
    if ((offset == 0) && (n == store->data.size)) {
        sum = persistent_checksum(store, src);
    } else {
        struct maybe_sum tmp = persistent_fetch_checksum(store);
        if (tmp.access != PERSISTENT_ACCESS_SUCCESS) {
            return tmp.access;
        }
        sum = tmp.value;
    }

    return persistent_store_checksum(store, sum);
}

/**
 * Store all of the data portion into PersistentStorage instance
 *
 * @param  store   Pointer to PersistentStorage instance to use
 * @param  src     Pointer to source buffer to read from
 *
 * @return Error condition via PersistentAccess data type.
 * @sideeffects Fills data portion of PersistentStorage instance as described.
 */
PersistentAccess
persistent_store(PersistentStorage *store, const void *src)
{
    return persistent_store_part(store, src, 0, store->data.size);
}
