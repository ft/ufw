/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <stddef.h>
#include <string.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/octet-buffer.h>

void
octet_buffer_null(OctetBuffer *b)
{
    b->data = NULL;
    b->size = b->used = b->offset = 0u;
}

int
octet_buffer_set(OctetBuffer *b, void *data,
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

size_t
octet_buffer_avail(OctetBuffer *b)
{
    return (b->size - b->used);
}

size_t
octet_buffer_rest(OctetBuffer *b)
{
    return (b->used - b->offset);
}

int
octet_buffer_use(OctetBuffer *b, void *data, size_t size)
{
    return octet_buffer_set(b, data, size, size, 0u);
}

int
octet_buffer_space(OctetBuffer *b, void *data, size_t size)
{
    return octet_buffer_set(b, data, size, 0u, 0u);
}

int
octet_buffer_add(OctetBuffer *b, const void *data, size_t size)
{
    if (b->size < (b->used + size)) {
        return -ENOMEM;
    }

    memcpy(b->data + b->used, data, size);
    b->used += size;
    return 0;
}

int
octet_buffer_consume(OctetBuffer *b, void *data, size_t size)
{
    if (size > (b->used - b->offset)) {
        return -ENODATA;
    }

    memcpy(data, b->data + b->offset, size);
    b->offset += size;

    return 0;
}

ssize_t
octet_buffer_consume_at_most(OctetBuffer *b, void *data, size_t size)
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

int
octet_buffer_rewind(OctetBuffer *b)
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

void
octet_buffer_clear(OctetBuffer *b)
{
    b->offset = b->used = 0u;
    memset(b->data, 0, b->size);
}

void
octet_buffer_repeat(OctetBuffer *b)
{
    b->offset = 0u;
}
