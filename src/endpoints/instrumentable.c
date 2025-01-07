/*
 * Copyright (c) 2022-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup endpoints Endpoints
 * @{
 */

/**
 * @file endpoints/instrumentable.c
 * @brief Sources and sinks that can be instrumented for testing purposes.
 *
 * These endpoints are similar to buffer-backed endpoints, except that you can
 * control their error behaviour. Namely at which offset a certain error should
 * occur.
 */

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/bit-operations.h>
#include <ufw/byte-buffer.h>
#include <ufw/compiler.h>
#include <ufw/endpoints.h>
#include <ufw/hexdump.h>

static void
init_ibuf(InstrumentableBuffer *b)
{
    b->chunksize = 0u;
    b->flags = 0u;
    instrumentable_reset_error(&b->read.error);
    instrumentable_reset_stats(&b->read.stat);
    instrumentable_reset_error(&b->write.error);
    instrumentable_reset_stats(&b->write.stat);
    byte_buffer_reset(&b->buffer);
}

static void
debug_trace(const bool sinkp, const void *driver, const unsigned char data)
{
    const InstrumentableBuffer *b = driver;
    const char *label = sinkp ? "(sink)  " : "(source)";
    const size_t count = sinkp
        ? b->write.stat.bytes
        : b->read.stat.bytes;
    printf("# %s 0x%016"PRIXPTR": %7zu 0x%02x '%c' u:%u s:%d\n",
           label, (uintptr_t)driver, count,
           data,
#ifdef UFW_HAVE_CTYPE_ISPRINT
           isprint(data) ? (char)data : '.',
#else
           '.',
#endif /* UFW_HAVE_CTYPE_ISPRINT */
           (unsigned int)data,
           (signed int)data);
}

static int
run_instrumentable_octet_source(void *driver, void *data)
{
    InstrumentableBuffer *b = driver;
    const InstrumentableError *err = &b->read.error;
    InstrumentableAccessStats *stat = &b->read.stat;

    stat->accesses++;

    if (BIT_ISSET(err->flags, INSTRUMENTABLE_UNTIL_SUCCESS)) {
        if (stat->accesses <= err->at) {
            return err->number;
        }
    }

    if (BIT_ISSET(err->flags, INSTRUMENTABLE_UNTIL_FAILURE)) {
        if (b->buffer.offset >= err->at) {
            return err->number;
        }
    }

    if (b->buffer.offset >= b->buffer.used) {
        return -ENODATA;
    }

    unsigned char *dest = data;

    *dest = b->buffer.data[b->buffer.offset];
    b->buffer.offset++;
    stat->bytes++;

    if (BIT_ISSET(b->flags, INSTRUMENTABLE_COMMON_ENABLE_TRACE)) {
        debug_trace(false, driver, *dest);
    }

    return 1;
}

static int
run_instrumentable_octet_sink(void *driver, unsigned char data)
{
    InstrumentableBuffer *b = driver;
    const InstrumentableError *err = &b->write.error;
    InstrumentableAccessStats *stat = &b->write.stat;

    stat->accesses++;

    if (BIT_ISSET(err->flags, INSTRUMENTABLE_UNTIL_SUCCESS)) {
        if (stat->accesses <= err->at) {
            return err->number;
        }
    }

    if (BIT_ISSET(err->flags, INSTRUMENTABLE_UNTIL_FAILURE)) {
        if (b->buffer.used >= err->at) {
            return err->number;
        }
    }

    if (b->buffer.used == b->buffer.size) {
        return -ENOMEM;
    }

    if (BIT_ISSET(b->flags, INSTRUMENTABLE_COMMON_ENABLE_TRACE)) {
        debug_trace(true, driver, data);
    }

    b->buffer.data[b->buffer.used] = data;
    b->buffer.used++;
    stat->bytes++;

    return 1;
}

static ssize_t
run_instrumentable_chunk_source(void *driver, void *data, size_t n)
{
    InstrumentableBuffer *b = driver;
    const InstrumentableError *err = &b->read.error;
    InstrumentableAccessStats *stat = &b->read.stat;

    stat->accesses++;

    if (BIT_ISSET(err->flags, INSTRUMENTABLE_UNTIL_SUCCESS)) {
        if (stat->accesses <= err->at) {
            return err->number;
        }
    }

    if (BIT_ISSET(err->flags, INSTRUMENTABLE_UNTIL_FAILURE)) {
        if (b->buffer.used >= err->at) {
            return err->number;
        }
    }

    if (n > b->chunksize) {
        n = b->chunksize;
    }

    if (BIT_ISSET(b->flags, INSTRUMENTABLE_COMMON_ENABLE_TRACE)) {
        for (size_t i = 0u; i < n; ++i) {
            debug_trace(false, driver, *b->buffer.data);
        }
    }

    return byte_buffer_consume_at_most(&b->buffer, data, n);
}

static ssize_t
run_instrumentable_chunk_sink(void *driver, const void *data, size_t n)
{
    InstrumentableBuffer *b = driver;
    const InstrumentableError *err = &b->write.error;
    InstrumentableAccessStats *stat = &b->write.stat;

    stat->accesses++;

    if (BIT_ISSET(err->flags, INSTRUMENTABLE_UNTIL_SUCCESS)) {
        if (stat->accesses <= err->at) {
            return err->number;
        }
    }

    if (BIT_ISSET(err->flags, INSTRUMENTABLE_UNTIL_FAILURE)) {
        if (b->buffer.used >= err->at) {
            return err->number;
        }
    }

    if (n > b->chunksize) {
        n = b->chunksize;
    }

    if (BIT_ISSET(b->flags, INSTRUMENTABLE_COMMON_ENABLE_TRACE)) {
        for (size_t i = 0u; i < n; ++i) {
            const unsigned char *ptr = data;
            debug_trace(true, driver, *ptr);
        }
    }

    const ssize_t rc = byte_buffer_add(&b->buffer, data, n);
    return rc < 0 ? rc : (ssize_t)n;
}

void
instrumentable_source(const DataKind kind,
                      Source *instance,
                      InstrumentableBuffer *buffer)
{
    init_ibuf(buffer);
    switch (kind) {
    case DATA_KIND_OCTET:
        octet_source_init(instance, run_instrumentable_octet_source, buffer);
        break;
    case DATA_KIND_CHUNK:
        chunk_source_init(instance, run_instrumentable_chunk_source, buffer);
        break;
    default:
        assert(false);
        break;
    }
}

void
instrumentable_sink(const DataKind kind,
                    Sink *instance,
                    InstrumentableBuffer *buffer)
{
    init_ibuf(buffer);
    switch (kind) {
    case DATA_KIND_OCTET:
        octet_sink_init(instance, run_instrumentable_octet_sink, buffer);
        break;
    case DATA_KIND_CHUNK:
        chunk_sink_init(instance, run_instrumentable_chunk_sink, buffer);
        break;
    default:
        assert(false);
        break;
    }
}

void
instrumentable_set_trace(InstrumentableBuffer *b, const bool value)
{
    if (value) {
        BIT_SET(b->flags, INSTRUMENTABLE_COMMON_ENABLE_TRACE);
    } else {
        BIT_CLEAR(b->flags, INSTRUMENTABLE_COMMON_ENABLE_TRACE);
    }
}

void
instrumentable_until_error_at(InstrumentableError *e,
                              const size_t offset,
                              const int n)
{
    e->flags = INSTRUMENTABLE_UNTIL_FAILURE;
    e->at = offset;
    e->number = n;
}

void
instrumentable_until_success_at(InstrumentableError *e,
                                const size_t offset,
                                const int n)
{
    e->flags = INSTRUMENTABLE_UNTIL_SUCCESS;
    e->at = offset;
    e->number = n;
}

void
instrumentable_reset_error(InstrumentableError *e)
{
    e->at = 0u;
    e->flags = 0u;
    e->number = 0;
}

void
instrumentable_reset_stats(InstrumentableAccessStats *s)
{
    s->accesses = 0u;
    s->bytes= 0u;
}

/**
 * @}
 */
