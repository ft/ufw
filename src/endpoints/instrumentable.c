/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file endpoints/instrumentable.c
 * @brief Sources and sinks that can be instrumented for testing purposes.
 *
 * These endpoints are similar to buffer-backed endpoints, except that you can
 * control their error behaviour. Namely at which offset a certain error should
 * occur.
 */

#include <stddef.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/endpoints.h>

void
instrumentable_error_at(InstrumentableBuffer *buffer, size_t at, int error)
{
    buffer->flags |= INSTRUMENTABLE_ERROR_AT_COUNT;
    buffer->error.at = at;
    buffer->error.number = error;
}

void
instrumentable_no_error(InstrumentableBuffer *buffer)
{
    buffer->flags = 0u;
    buffer->error.at = 0u;
    buffer->error.number = 0;
}

static int
run_instrumentable_source(void *driver, void *data)
{
    InstrumentableBuffer *b = driver;

    if (BIT_ISSET(b->flags, INSTRUMENTABLE_ERROR_AT_COUNT)) {
        if (b->buffer.offset >= b->error.at) {
            return b->error.number;
        }
    }

    if (b->buffer.offset >= b->buffer.used) {
        return 0;
    }

    unsigned char *dest = data;

    *dest = b->buffer.data[b->buffer.offset];
    b->buffer.offset++;

    return 1;
}

static int
run_instrumentable_sink(void *driver, unsigned char data)
{
    InstrumentableBuffer *b = driver;

    if (BIT_ISSET(b->flags, INSTRUMENTABLE_ERROR_AT_COUNT)) {
        if (b->buffer.used >= b->error.at) {
            return b->error.number;
        }
    }

    if (b->buffer.used == b->buffer.size) {
        return 0;
    }

    b->buffer.data[b->buffer.used] = data;
    b->buffer.used++;

    return 1;
}

void
instrumentable_source(Source *instance, InstrumentableBuffer *buffer)
{
    instrumentable_no_error(buffer);
    octet_source_init(instance, run_instrumentable_source, buffer);
}

void
instrumentable_sink(Sink *instance, InstrumentableBuffer *buffer)
{
    instrumentable_no_error(buffer);
    octet_sink_init(instance, run_instrumentable_sink, buffer);
}
