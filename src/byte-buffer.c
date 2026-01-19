/*
 * Copyright (c) 2022-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup bytebuffer Byte Buffers
 * @{
 *
 * @file byte-buffer.c
 * @brief Byte Buffer Implementation
 *
 * @}
 */

#include <stddef.h>
#include <string.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/byte-buffer.h>

/**
 * Point byte-buffer at NULL pointer
 *
 * This is a buffer that has no memory. So size, used and offset are all zero.
 * There is not a lot that can be done with such buffers. But they offer a
 * value to use when no other piece of memory is available.
 *
 * @param  b  The ByteBuffer instance to use
 *
 * @sideeffects Modifies byte buffer meta data.
 */
void
byte_buffer_null(ByteBuffer *b)
{
    b->data = NULL;
    b->size = b->used = b->offset = 0U;
}

/**
 * Generic assignment for memory into ByteBuffer abstraction
 *
 * This function assigns a piece of memory into a ByteBuffer abstraction. This
 * function allows control for all meta-data. In most cases it is better to use
 * one of the more specific assignment functions like byte_buffer_space().
 *
 * The input parameters must be sensible: size cannot be zero. data cannot be
 * NULL. used cannot be larger than size. offset cannot be larger than used.
 *
 * @param  b       The ByteBuffer instance to use
 * @param  data    Pointer to memory area to use.
 * @param  size    Size of the memory area pointed at by data.
 * @param  used    Indicata how much of the memory area is currently in use.
 * @param  offset  Process mark into memory area.
 *
 * @return -EINVAL if input values are nonsensical; 0 otherwise.
 * @sideeffects Modifies byte buffer meta data.
 */
int
byte_buffer_set(ByteBuffer *b, void *data,
                size_t size, size_t used, size_t offset)
{
    if (data == NULL || size == 0U || used > size || offset > used) {
        return -EINVAL;
    }

    b->data = data;
    b->size = size;
    b->used = used;
    b->offset = offset;

    return 0;
}

/**
 * Return the number of bytes free for use in a ByteBuffer
 *
 * This is the number of unused bytes in the memory area assigned to a
 * ByteBuffer.
 *
 * @param  b  The ByteBuffer instance to query.
 *
 * @return Number of bytes free for use
 * @sideeffects None
 */
size_t
byte_buffer_avail(const ByteBuffer *b)
{
    return (b->size - b->used);
}

/**
 * Return the number of bytes available after the process mark in a ByteBuffer
 *
 * The offset member of ByteBuffer is a process mark used by the consume type
 * APIs. The rest to consume is the number of bytes from the mark to the number
 * of bytes usd in the ByteBuffer (not its size!).
 *
 * @param  b  The ByteBuffer instance to query.
 *
 * @return Number of bytes available after process mark
 * @sideeffects None
 */
size_t
byte_buffer_rest(const ByteBuffer *b)
{
    return (b->used - b->offset);
}

/**
 * Assign an exising piece of memory to a ByteBuffer
 *
 * This function is useful when an exising buffer, with meaningful data, should
 * be wrapped in a ByteBuffer, for use in APIs using this abstraction. This
 * marks all of the memory as used in the ByteBuffer instance.
 *
 * @param  b     The ByteBuffer instance to use.
 * @param  data  Pointer to memory area to use.
 * @param  size  Size of the memory area pointed at by data.
 *
 * @return -EINVAL if input values are nonsensical; 0 otherwise.
 * @sideeffects Modifies byte buffer meta data.
 */
int
byte_buffer_use(ByteBuffer *b, void *data, size_t size)
{
    return byte_buffer_set(b, data, size, size, 0U);
}

/**
 * Use an exising piece of memory as empty space in a ByteBuffer
 *
 * This function is useful when an exising buffer, should be wrapped in a
 * ByteBuffer, for use in APIs using this abstraction. This marks all of the
 * memry as unused in the ByteBuffer instance.
 *
 * @param  b     The ByteBuffer instance to use.
 * @param  data  Pointer to memory area to use.
 * @param  size  Size of the memory area pointed at by data.
 *
 * @return -EINVAL if input values are nonsensical; 0 otherwise.
 * @sideeffects Modifies byte buffer meta data.
 */
int
byte_buffer_space(ByteBuffer *b, void *data, size_t size)
{
    return byte_buffer_set(b, data, size, 0U, 0U);
}

/**
 * Append data at the end of a ByteBuffer managed piece of memory
 *
 * This copies a piece of memory into a ByteBuffer's piece of memory, at the
 * beginning of the currently unused space (as marked by used). The function
 * returns -ENOMEM if the size of the memory pointed too is too large to fit
 * into the ByteBuffer.
 *
 * @param  b     The ByteBuffer instance to use.
 * @param  data  Pointer to the memory to copy into b.
 * @param  size  Size of memory pointed to by data.
 *
 * @return -ENOMEM if data is too large for buffer; zero on success.
 * @sideeffects Modifies byte buffer meta data and memory.
 */
int
byte_buffer_add(ByteBuffer *b, const void *data, size_t size)
{
    if (b->size < (b->used + size)) {
        return -ENOMEM;
    }

    memcpy(b->data + b->used, data, size);
    b->used += size;
    return 0;
}

/**
 * Store the position information of a buffer
 *
 * This function stores a buffer's positional information in separate
 * parameter. This position can be applied using byte_buffer_setpos() later on.
 *
 * @param  b    The ByteBuffer to get the position information from
 * @param  pos  A ByteBufferPos datum to store the infommation in
 *
 * @sideeffects Modifies pos parameter
 */
void
byte_buffer_getpos(const ByteBuffer *b, ByteBufferPos *pos)
{
    pos->offset = b->offset;
    pos->used = b->used;
}


/**
 * Apply positional information to a buffer
 *
 * This function applies the previously acquired positional information of a
 * buffer to a buffer. parameter. Positional information like this can be
 * obtained by using byte_buffer_getpos().
 *
 * @param  b    The ByteBuffer to set the position information in
 * @param  pos  A ByteBufferPos datum holding the desired positional
 *              information
 *
 * @return -EINVAL if the position does not fit with the buffer. 0 otherwise.
 * @sideeffects Modifies pos parameter
 */
int
byte_buffer_setpos(ByteBuffer *b, const ByteBufferPos *pos)
{
    if (pos->offset >= b->size || pos->used > b->size) {
        return -EINVAL;
    }
    b->offset = pos->offset;
    b->used = pos->used;

    return 0;
}

/**
 * Return a pointer at the current read-position of a buffer
 *
 * With some low-level operations it may be useful to get a pointer into the
 * ByteBuffer instance at the position of the read-mark (offset). It is usually
 * a good idea to also query the amount of date left for reading as well (via
 * byte_buffer_rest()).
 *
 * @param  b  The ByteBuffer instance to query.
 *
 * @return A pointer at the read position of the buffer; NULL if no memory is
 *         left to read in the buffer.
 * @sideeffects None
 */
unsigned char*
byte_buffer_readptr(ByteBuffer *b)
{
    if (b->offset >= b->size) {
        return NULL;
    }
    return b->data + b->offset;
}

/**
 * Return a pointer at the current write-position of a buffer
 *
 * With some low-level operations it may be useful to get a pointer into the
 * ByteBuffer instance at the position of the write-mark (used). It is usually
 * a good idea to also query the space left available for writing as well (via
 * byte_buffer_avail()).
 *
 * @param  b  The ByteBuffer instance to query.
 *
 * @return A pointer at the write position of the buffer; NULL if no memory is
 *         left to use in the buffer.
 * @sideeffects None
 */
unsigned char*
byte_buffer_writeptr(ByteBuffer *b)
{
    if (b->used >= b->size) {
        return NULL;
    }
    return b->data + b->used;
}

/**
 * Mark a chunk of bytes as read
 *
 * This moves the read-mark (offset) forward by SIZE bytes. If that would
 * exceed the limits of the buffer, -ENODATA is returned.
 *
 * The function is similar to byte_buffer_consume(), but without the actual
 * copying of data. This function only modifies the meta-data, concerning the
 * read-position into the buffer.
 *
 * @param  b     The ByteBuffer instance to use.
 * @param  size  The number of bytes to mark as read.
 *
 * @return 0 on success; -ENODATA if the operation would exceed the size of the
 *         ByteBuffer instance.
 * @sideeffects Modifies the instances read-mark.
 */
int
byte_buffer_markread(ByteBuffer *b, size_t size)
{
    if (size > (b->used - b->offset)) {
        return -ENOMEM;
    }

    b->offset += size;
    return 0;
}

/**
 * Extract data from ByteBuffer using its process mark
 *
 * The ByteBuffer abstraction has a process mark called offset. This function
 * allows extracting data from the ByteBuffer chunk by chunk with the mark
 * keeping track of the read position in the buffer. This function reads an
 * exact number of bytes from the ByteBuffer or errors out of the request
 * cannot be fulfilled because the buffer does not have enough data left.
 *
 * @param  b     The ByteBuffer instance to use.
 * @param  data  Pointer of memory to copy into.
 * @param  size  Number of bytes to extract from ByteBuffer.
 *
 * @return -ENODATA if ByteBuffer does not have size bytes left in it to
 *         process (that is, beyond the offset member); zero on success.
 * @sideeffects Modifies byte buffer meta data.
 */
int
byte_buffer_consume(ByteBuffer *b, void *data, size_t size)
{
    if (size > (b->used - b->offset)) {
        return -ENODATA;
    }

    memcpy(data, b->data + b->offset, size);
    b->offset += size;

    return 0;
}

/**
 * Extract some data from ByteBuffer using its process mark
 *
 * This is similar to byte_buffer_consume(). The difference is that if it has
 * some data left, even if that is less than requested, that last piece of data
 * is extracted. The function does return -ENODATA of the number of bytes left
 * in a buffer is zero.
 *
 * @param  b     The ByteBuffer instance to use.
 * @param  data  Pointer of memory to copy into.
 * @param  size  Number of bytes to extract from ByteBuffer.
 *
 * @return -ENODATA if ByteBuffer does not have any bytes left in it; the
 *         number of bytes copied is returned on success.
 * @sideeffects Modifies byte buffer meta data.
 */
ssize_t
byte_buffer_consume_at_most(ByteBuffer *b, void *data, size_t size)
{
    const size_t rest = b->used - b->offset;
    if (rest == 0U) {
        return -ENODATA;
    }

    const size_t n = size > rest ? rest : size;
    memcpy(data, b->data + b->offset, n);
    b->offset += n;

    return (ssize_t)n;
}

/**
 * Move unprocessed data in a ByteBuffer to the front of its memory
 *
 * This is useful to move some leftover data to the front of a ByteBuffer so
 * more room is available more more appending to the buffer.
 *
 * @param  b  The ByteBuffer instance to use.
 *
 * @return -EINVAL of the ByteBuffer's memory is NULL; zero otherwise.
 * @sideeffects Modifies byte buffer meta data and memory.
 */
int
byte_buffer_rewind(ByteBuffer *b)
{
    if (b->data == NULL) {
        return -EINVAL;
    }

    if (b->offset == 0U) {
        /* Already rewound */
        return 0;
    }

    const size_t rest = b->used - b->offset;
    memmove(b->data, b->data + b->offset, rest);
    b->used = rest;
    b->offset = 0U;

    return 0;
}

/**
 * Wipe and reset ByteBuffer
 *
 * This sets all bytes of a ByteBuffer instance to zero. It also sets the
 * number of bytes used in the buffer and its processing mark to zero.
 *
 * @param  b  The ByteBuffer instance to use.
 *
 * @sideeffects Modifies byte buffer meta data and memory.
 */
void
byte_buffer_clear(ByteBuffer *b)
{
    b->offset = b->used = 0U;
    memset(b->data, 0, b->size);
}

/**
 * Reset ByteBuffer instance
 *
 * This sets the number of bytes used in the buffer and its processing mark to
 * zero.
 *
 * @param  b  The ByteBuffer instance to use.
 *
 * @sideeffects Modifies byte buffer meta data.
 */
void
byte_buffer_reset(ByteBuffer *b)
{
    b->offset = b->used = 0U;
}

/**
 * Reset process mark of ByteBuffer instance
 *
 * This is useful if the contents of a buffer should be re-processed.
 *
 * @param  b  The ByteBuffer instance to use.
 *
 * @sideeffects Modifies byte buffer meta data.
 */
void
byte_buffer_repeat(ByteBuffer *b)
{
    b->offset = 0U;
}

/**
 * Fill ByteBuffer completely with a fixed datum
 *
 * This is basically memset() for a ByteBuffer. Note that this will mark all
 * memory in the ByteBuffer as used.
 *
 * @param  b      The ByteBuffer instance to use.
 * @param  datum  Datum to write to all bytes in memory.
 *
 * @sideeffects Modifies byte buffer meta data and memory
 */
void
byte_buffer_fill(ByteBuffer *b, const unsigned char datum)
{
    memset(b->data, datum, b->size);
    b->used = b->size;
}

/**
 * Incrementally fill data in a ByteBuffer completely
 *
 * Instead of filling memory with a single datum like byte_buffer_fill(), this
 * allows specifying an initial value to a use and an increment to be applied
 * from byte to byte.
 *
 * @param  b          The ByteBuffer instance to use.
 * @param  init       Initially value at index zero.
 * @param  increment  Increment to apply at each iteration.
 *
 * @sideeffects Modifies byte buffer meta data and memory.
 */
void
byte_buffer_fillx(ByteBuffer *b,
                  const unsigned char init,
                  const signed char increment)
{
    unsigned char datum = init;
    for (size_t i = 0U; i < b->size; ++i) {
        b->data[i] = datum;
        datum += increment;
    }
    b->used = b->size;
}

/**
 * Run a function to produce initialisation values for ByteBuffer memory
 *
 * This is the most flexible kind of memory initialisation for ByteBuffer
 * instances. This function iterates over each byte in memory starting at
 * "offset". For each byte the callback function "cb" is called with the
 * current index into the ByteBuffer and the pointer to the byte initialise.
 *
 * Initialisation stops, if the end of the buffer is reached, or the callback
 * function returns a negative value.
 *
 * Note that the pointer handed to the callback is into the buffer directly, to
 * avoid additional copying. That means if an early iteration termination is
 * signaled using a negative return value, the function should not modify the
 * value this pointer points to.
 *
 * Finally, the process mark is set to the "offset" parameter, and the number
 * of used bytes in the buffer is set to the index at which the iteration
 * stopped.
 *
 * @param  b       The ByteBuffer instance to use.
 * @param  offset  Index at which to start the initialisation process.
 * @param  cb      Function to call to produce initialisation values.
 *
 * @sideeffects Modifies byte buffer meta data and memory.
 */
void
byte_buffer_fill_cb(ByteBuffer *b, const size_t offset,
                    int(*cb)(size_t, unsigned char*))
{
    size_t i;
    for (i = offset; i < b->size; ++i) {
        const int rc = cb(i, b->data + i);
        if (rc < 0) {
            break;
        }
    }
    b->offset = offset;
    b->used = i;
}
