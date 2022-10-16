/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file source-sink.c
 * @brief Implementation of a Sink and Source abstraction
 *
 * This implements a generic sink and source data type. The main idea here is
 * to be able to reuse implementations of protocols like SLIP for a wide array
 * of applications.
 *
 * The API implements getting and putting octets and buffers of octets from/to
 * sources/sinks. The underlying driver can be either of those access paradigms
 * and the abstraction implements the other on top of it.
 *
 * Sources and sinks make sure the expected amount of data is transmitted,
 * unlike the POSIX read() and write() functions, for instance.
 *
 * The OctetSource/OctetSink and ChunkSource/ChunkSource function types need to
 * therefore behave similar to those functions, while returning negative errno
 * instead of actually setting errno. They do need to return the number of
 * octets transmitted. Returning zero indicates end-of-file. If some condition
 * would cause the function to return zero even though EOF was not reached yet,
 * an appropriate errno has to be used. -EINTR in case the function was
 * interrupted by a signal before an octet could be read/written.
 *
 * If a driver returns zero (indicating end-of-file) while getting/putting a
 * buffer of octets, the API returns zero as well, regardless of any previously
 * transmitted data.
 *
 * The API for sources and sinks return negative errno values to signal errors.
 *
 * For sources, returning zero indicates end of file.
 *
 * Positive return values indicate the number of octets that were transmitted.
 *
 * Using a data count of zero, or one bigger than SSIZE_MAX causes the API to
 * return -EINVAL.
 */

#include <stddef.h>
#include <string.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/types.h>

static ssize_t
read_from_buffer(void *driver, void *data, size_t n)
{
    OctetBuffer *b = driver;
    ssize_t value = n;

    const ssize_t rc = octet_buffer_consume(b, data, n);
    if (rc == -EOVERFLOW) {
        value = 0;
    } else if (rc < 0) {
        value = rc;
    }

    return value;
}

static ssize_t
write_to_buffer(void *driver, const void *data, size_t n)
{
    OctetBuffer *b = driver;
    ssize_t value = n;

    const ssize_t rc = octet_buffer_add(b, data, n);
    if (rc == -EOVERFLOW) {
        value = 0;
    } else if (rc < 0) {
        value = rc;
    }

    return value;
}

void
source_from_buffer(Source *instance, OctetBuffer *buffer)
{
    chunk_source_init(instance, read_from_buffer, buffer);
}

void
sink_to_buffer(Sink *instance, OctetBuffer *buffer)
{
    chunk_sink_init(instance, write_to_buffer, buffer);
}

void
octet_source_init(Source *instance, OctetSource source, void *driver)
{
    instance->kind = DATA_KIND_OCTET;
    instance->source.octet = source;
    instance->driver = driver;
}

void
chunk_source_init(Source *instance, ChunkSource source, void *driver)
{
    instance->kind = DATA_KIND_CHUNK;
    instance->source.chunk = source;
    instance->driver = driver;
}

void
octet_sink_init(Sink *instance, OctetSink sink, void *driver)
{
    instance->kind = DATA_KIND_OCTET;
    instance->sink.octet = sink;
    instance->driver = driver;
}

void
chunk_sink_init(Sink *instance, ChunkSink sink, void *driver)
{
    instance->kind = DATA_KIND_CHUNK;
    instance->sink.chunk = sink;
    instance->driver = driver;
}

int
source_get_octet(Source *source, void *data)
{
    return source->kind == DATA_KIND_OCTET
        ? source->source.octet(source->driver, data)
        : source->source.chunk(source->driver, data, 1u);
}

int
sink_put_octet(Sink *sink, const unsigned char data)
{
    return sink->kind == DATA_KIND_OCTET
        ? sink->sink.octet(sink->driver, data)
        : sink->sink.chunk(sink->driver, &data, 1u);
}

static inline ssize_t
source_adapt(OctetSource source, void *driver, void *buf, size_t n)
{
    unsigned char *data = buf;
    for (size_t i = 0u; i < n; ++i) {
        const int rc = source(driver, data + i);
        if (rc < 0) {
            return (ssize_t)rc;
        }
    }

    return 0;
}

static inline ssize_t
once_source_get_chunk(Source *source, void *buf, size_t n)
{
    if (n == 0 || n > SSIZE_MAX) {
        return -EINVAL;
    }

    return source->kind == DATA_KIND_OCTET
        ? source_adapt(source->source.octet, source->driver, buf, n)
        : source->source.chunk(source->driver, buf, n);
}

ssize_t
source_get_chunk(Source *source, void *buf, size_t n)
{
    if (n == 0 || n > SSIZE_MAX) {
        return -EINVAL;
    }

    size_t rest = n;
    while (rest > 0) {
        const ssize_t get = once_source_get_chunk(source, buf, rest);
        if (get < 0) {
            return get;
        } else if (get == 0) {
            return 0;
        }
        rest -= get;
    }

    return (ssize_t)n;
}

static inline ssize_t
sink_adapt(OctetSink sink, void *driver, const void *buf, size_t n)
{
    const unsigned char *data = buf;
    for (size_t i = 0u; i < n; ++i) {
        const int rc = sink(driver, data[i]);
        if (rc < 0) {
            return (ssize_t)rc;
        }
    }

    return n;
}

static inline ssize_t
once_sink_put_chunk(Sink *sink, const void *buf, size_t n)
{
    return sink->kind == DATA_KIND_OCTET
        ? sink_adapt(sink->sink.octet, sink->driver, buf, n)
        : sink->sink.chunk(sink->driver, buf, n);
}

ssize_t
sink_put_chunk(Sink *sink, const void *buf, size_t n)
{
    if (n == 0 || n > SSIZE_MAX) {
        return -EINVAL;
    }

    size_t rest = n;
    while (rest > 0) {
        const ssize_t put = once_sink_put_chunk(sink, buf, rest);
        if (put < 0) {
            return put;
        } else if (put == 0) {
            return 0;
        }
        rest -= put;
    }

    return (ssize_t)n;
}
