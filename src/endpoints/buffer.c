/*
 * Copyright (c) 2022-2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file endpoints/buffer.c
 * @brief Sources and sinks interfacing byte buffer.
 */

#include <stddef.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/endpoints.h>

static ssize_t
read_from_buffer(void *driver, void *data, size_t n)
{
    ByteBuffer *b = driver;
    return byte_buffer_consume_at_most(b, data, n);
}

static ssize_t
read_from_chunks(void *driver, void *data, size_t n)
{
    ByteChunks *source = driver;

next:
    if (source->active >= source->chunks) {
        return -ENODATA;
    }

    const ssize_t rc = byte_buffer_consume_at_most(
        source->chunk + source->active, data, n);

    if (rc == -ENODATA) {
        source->active += 1u;
        goto next;
    }

    return rc;
}

static ssize_t
write_to_buffer(void *driver, const void *data, size_t n)
{
    ByteBuffer *b = driver;
    ssize_t value = n;

    const ssize_t rc = byte_buffer_add(b, data, n);
    if (rc < 0) {
        value = rc;
    }

    return value;
}

void
source_from_buffer(Source *instance, ByteBuffer *buffer)
{
    chunk_source_init(instance, read_from_buffer, buffer);
}

void
source_from_chunks(Source *instance, ByteChunks *chunks)
{
    chunk_source_init(instance, read_from_chunks, chunks);
}

void
sink_to_buffer(Sink *instance, ByteBuffer *buffer)
{
    chunk_sink_init(instance, write_to_buffer, buffer);
}
