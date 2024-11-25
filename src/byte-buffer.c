/*
 * Copyright (c) 2022-2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
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
 * @return void
 * @sideeffects Modifies byte buffer meta data.
 */
void
byte_buffer_null(ByteBuffer *b)
{
    b->data = NULL;
    b->size = b->used = b->offset = 0u;
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
    if (data == NULL || size == 0u || used > size || offset > used) {
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
    return byte_buffer_set(b, data, size, size, 0u);
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
    return byte_buffer_set(b, data, size, 0u, 0u);
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
    if (rest == 0u) {
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
 * @param  b  TODO: The ByteBuffer instance to use.
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

    if (b->offset == 0u) {
        /* Already rewound */
        return 0;
    }

    memmove(b->data,
            b->data + b->offset,
            b->used - b->offset);

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
 * @return void
 * @sideeffects Modifies byte buffer meta data and memory.
 */
void
byte_buffer_clear(ByteBuffer *b)
{
    b->offset = b->used = 0u;
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
 * @return void
 * @sideeffects Modifies byte buffer meta data.
 */
void
byte_buffer_reset(ByteBuffer *b)
{
    b->offset = b->used = 0u;
}

/**
 * Reset process mark of ByteBuffer instance
 *
 * This is useful if the contents of a buffer should be re-processed.
 *
 * @param  b  The ByteBuffer instance to use.
 *
 * @return void
 * @sideeffects Modifies byte buffer meta data.
 */
void
byte_buffer_repeat(ByteBuffer *b)
{
    b->offset = 0u;
}
