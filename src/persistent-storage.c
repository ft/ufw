/*
 * Copyright (c) 2019-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup pstorage Persistent Storage
 * @{
 */

/**
 * @file persistent-storage.c
 * @brief Persistent storage implementation
 */

#include <stdint.h>
#include <string.h>

#include <ufw/persistent-storage.h>

/**
 * Utility type to maybe represent a checksum value
 *
 * This type either encodes success and a checksum value, or an error code an
 * no checksum value.
 */
struct maybe_sum {
    /** Success or error code */
    PersistentAccess access;
    /** Checksum value in case ‘access’ datum shows success */
    PersistentChecksum value;
};

/**
 * Return configured checksum size in bytes
 *
 * @param  store  Pointer to the instance to query
 *
 * @return Checksum size in bytes
 * @sideeffects None
 */
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

/**
 * Set up data address in storage instance
 *
 * In a PersistentStorage store, the data resides behind the checksum. There is
 * no padding for different checksum implementations, so the offset depends on
 * the checksum that is configured. This function sets that offset accordingly.
 *
 * @param  store  Pointer to the instance to configure
 *
 * @return void
 * @sideeffects Modifies the storage instance as described.
 */
static inline void
set_data_address(PersistentStorage *store)
{
    store->data.address = store->checksum.address + store->checksum.size;
}

/**
 * Trivial 16 bit checksum implementation
 *
 * This just sums up all bytes in the buffer into a uint16_t return value.
 *
 * Nobody should use this algorithm in production code! Use
 * persistent_checksum_16bit() or persistent_checksum_32bit() to specify a
 * proper checksumming algorithm.
 *
 * @param  data  Pointer to buffer to sum up
 * @param  n     Length of data buffer
 * @param  init  Offset for sum calculation
 *
 * @return uint16_t sum as described
 * @sideeffects None
 */
static uint16_t
trivialsum(const unsigned char *data, size_t n, uint16_t init)
{
    for (size_t i = 0u; i < n; ++i) {
        /*
         * Clang14's undefined behaviour sanitiser complains about this:
         *
         *   persistent-storage.c:47:14: runtime error: implicit conversion from
         *     type 'int' of value 65536 (32-bit, signed) to type 'uint16_t'
         *     (aka 'unsigned short') changed the value to 0 (16-bit, unsigned)
         *
         * ...when this was: init += data[i];
         *
         * I am not sure why that is, since neither "init" nor "data[*]" are
         * signed values (nor is "i" for that matter). Forcibly limiting the
         * result of the addition to 16 bits removes the error. I am a bit
         * puzzled about this, but nobody should be using "trivialsum()" for
         * anything in production anyway. It's main job is to never leave a
         * PersistentStorage object in an invalid state.
         */
        init = (init + data[i]) & 0xffffu;
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

/**
 * Run checksum algorithm if a storage instance for a memory block
 *
 * This takes a memory block, provided as ‘src’ and runs the checksum
 * algorithm, configured in the storage instance (type, data-size,
 * initial-value) for that piece of memory.
 *
 * The memory block must be fit for this kind of access (i.e. be readable to
 * the configured size).
 *
 * This is a simplified variant of persistent_calculate_checksum(), that relies
 * on the fact that the whole data to be checksummed can be accessed
 * consecutively. The caller has to make sure that is the case. If this cannot
 * be guaranteed, the more complex, but also more general function needs to be
 * used instead.
 *
 * @param  store  Pointer to the storage instance
 * @param  src    Pointer to the memory block to process
 *
 * @return Checksum for the memory block according to the storage instance
 * @sideeffects None
 */
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

/**
 * Calculate checksum for a persistent storage instance
 *
 * This function reads data from its designated source and runs the configured
 * checksum algorithm for it, returning the final result of that process,
 * unless an error occurs, in which case the ‘struct maybe_sum’ return type is
 * set up accordingly.
 *
 * If the storage instance was supplied with an auxiliary buffer, the function
 * uses it in order reduce the number of reading calls to the configured
 * medium. Otherwise, the medium is read byte-by-byte.
 *
 * @param  store  Pointer to the storage instance to process
 *
 * @return Success code and data store checksum; or an error code.
 * @sideeffects Accesses configured media as described.
 */
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
            rv.value.sum16 = store->checksum.process.c16(data, toget,
                                                         rv.value.sum16);
            break;
        case PERSISTENT_CHECKSUM_32BIT: /* FALLTHROUGH */
        default:
            rv.value.sum32 = store->checksum.process.c32(data, toget,
                                                         rv.value.sum32);
            break;
        }

        rest -= toget;
        address += toget;
    }

    rv.access = PERSISTENT_ACCESS_SUCCESS;
    return rv;
}

/**
 * Store given checksum in persistent storage medium
 *
 * This takes a checksum datum and stores it in the medium configured in a
 * persistent storage instance.
 *
 * @param  store  Pointer to the storage instance to modify
 * @param  sum    Checksum to store in persistent medium
 *
 * @return PERSISTENT_ACCESS_SUCCESS or PERSISTENT_ACCESS_IO_ERROR.
 * @sideeffects Writes to the configured data storage medium.
 */
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

/**
 * Retrieve checksum from persistent data medium
 *
 * This function does not calculate a checksum, but returns a previously stored
 * checksum from a medium configured in a persistent storage instance.
 *
 * @param  store  Pointer to the persistent instance to read from
 *
 * @return Success code and data store checksum; or an error code.
 * @sideeffects Data is read from persistent data medium.
 */
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

/**
 * Check if two given checksum values match
 *
 * Given two PersistentChecksum values, return true if and only if both values
 * are exactly the same.
 *
 * @param  store  Pointer to the instance to query configuration from
 * @param  a      First checksum value
 * @param  b      Second checksum value
 *
 * @return True if both checksum values match; false otherwise.
 * @sideeffects None
 */
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
        struct maybe_sum tmp = persistent_calculate_checksum(store);
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

/**
 * Set part of the memory in a persistent medium to one value
 *
 * This function takes a persistent storage instance and sets ‘k’ bytes
 * starting at offset ‘address’ to a given value ‘item’.
 *
 * This function is used to reset a persistent storage instance to a known
 * state in persistent_reset().
 *
 * @param  store    Pointer to the instance to write to
 * @param  address  Address at which to start writing
 * @param  item     Datum to write at each address
 * @param  k        Number of bytes to to write
 *
 * @return PERSISTENT_ACCESS_SUCCESS or PERSISTENT_ACCESS_IO_ERROR.
 * @sideeffects Data storage medium is written to as described.
 */
static PersistentAccess
persistent_writen(PersistentStorage *store,
                  uint32_t address, unsigned char item, size_t k)
{
    PersistentAccess rv;
    unsigned char buf;
    unsigned char *data;
    size_t bsize;

    rv = PERSISTENT_ACCESS_IO_ERROR;

    if (store->buffer.data != NULL) {
        data = store->buffer.data;
        bsize = store->buffer.size;
    } else {
        data = &buf;
        bsize = 1u;
    }

    memset(data, item, bsize);
    size_t rest = k;

    while (rest > 0u) {
        const size_t toput = (rest > bsize) ? bsize : rest;
        const size_t n = store->block.write(address, data, toput);

        if (n != toput) {
            return rv;
        }

        rest -= toput;
        address += toput;
    }

    rv = PERSISTENT_ACCESS_SUCCESS;
    return rv;
}

/**
 * Set all memory of a persistency instance to a given value
 *
 * This procedure overrides all memory referenced by a persistency instance.
 * This includes the memory where the instance's checksum is stored!
 *
 * @param  store   Pointer to PersistentStorage instance to use
 * @param  item    Value to set all memory referenced by store to
 *
 * @return Error condition via PersistentAccess data type.
 * @sideeffects Sets all memory of store to item.
 */
PersistentAccess
persistent_reset(PersistentStorage *store, unsigned char item)
{
    const PersistentAccess rv =
        persistent_writen(store, store->checksum.address,
                          item, store->checksum.size);
    if (rv != PERSISTENT_ACCESS_SUCCESS) {
        return rv;
    }
    return persistent_writen(store, store->data.address, item, store->data.size);
}

/**
 * @}
 */
