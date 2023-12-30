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

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/bit-operations.h>
#include <ufw/endpoints.h>
#include <ufw/hexdump.h>

static void
debug_trace(const bool sinkp, const void *driver, const unsigned char data)
{
    const InstrumentableBuffer *b = driver;
    const char *label = sinkp ? "(sink)  " : "(source)";
    const size_t count = sinkp ? b->count.write : b->count.read;
    printf("# %s 0x%016"PRIXPTR": %7zu 0x%02x '%c' u:%u s:%d\n",
           label, (uintptr_t)driver, count,
           data,
#ifdef UFW_HAVE_CTYPE_ISPRINT
           isprint(data) ? data : '.',
#else
           '.',
#endif /* UFW_HAVE_CTYPE_ISPRINT */
           (unsigned int)data,
           (signed int)data);
}

void
instrumentable_error_at(InstrumentableBuffer *buffer, size_t at, int error)
{
    BIT_SET(buffer->flags, INSTRUMENTABLE_ERROR_AT_COUNT);
    buffer->error.at = at;
    buffer->error.number = error;
}

void
instrumentable_no_error(InstrumentableBuffer *buffer)
{
    BIT_CLEAR(buffer->flags, INSTRUMENTABLE_ERROR_AT_COUNT);
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
        return -ENODATA;
    }

    unsigned char *dest = data;

    *dest = b->buffer.data[b->buffer.offset];
    b->buffer.offset++;
    b->count.read++;

    if (BIT_ISSET(b->flags, INSTRUMENTABLE_ENABLE_TRACE)) {
        debug_trace(false, driver, *dest);
    }

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
        return -ENOMEM;
    }

    if (BIT_ISSET(b->flags, INSTRUMENTABLE_ENABLE_TRACE)) {
        debug_trace(true, driver, data);
    }

    b->buffer.data[b->buffer.used] = data;
    b->buffer.used++;
    b->count.write++;

    return 1;
}

void
instrumentable_source(Source *instance, InstrumentableBuffer *buffer)
{
    instrumentable_no_error(buffer);
    buffer->count.read = 0u;
    octet_source_init(instance, run_instrumentable_source, buffer);
}

void
instrumentable_sink(Sink *instance, InstrumentableBuffer *buffer)
{
    instrumentable_no_error(buffer);
    buffer->count.write = 0u;
    octet_sink_init(instance, run_instrumentable_sink, buffer);
}

void
instrumentable_set_trace(InstrumentableBuffer *b, const bool value)
{
    if (value) {
        BIT_SET(b->flags, INSTRUMENTABLE_ENABLE_TRACE);
    } else {
        BIT_CLEAR(b->flags, INSTRUMENTABLE_ENABLE_TRACE);
    }
}
