/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file endpoints/core.c
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
 * Functions implementing endpoints must return the number of octet transmitted
 * (i.e. read or written). In case of an error, the functions must return
 * -ERRNO. Additionally, when sources run out of data permanently, they need to
 * return -ENODATA. When sinks run out of space to store date, they need to
 * return -ENOMEM. Returning zero is allowed, strictly, but will cause the
 * system to retry. If that is not meaningful with your endpoint, return a
 * meaningful error code instead. Drivers returning -EINTR will cause the
 * system to assume the operation was interrupted and it will thus retry.
 *
 * Using a data count of zero, or one bigger than SSIZE_MAX causes the API to
 * return -EINVAL.
 */

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/endpoints.h>

void
octet_source_init(Source *instance, OctetSource source, void *driver)
{
    instance->kind = DATA_KIND_OCTET;
    instance->source.octet = source;
    instance->driver = driver;
    instance->ext.getbuffer = NULL;
}

void
chunk_source_init(Source *instance, ChunkSource source, void *driver)
{
    instance->kind = DATA_KIND_CHUNK;
    instance->source.chunk = source;
    instance->driver = driver;
    instance->ext.getbuffer = NULL;
}

void
octet_sink_init(Sink *instance, OctetSink sink, void *driver)
{
    instance->kind = DATA_KIND_OCTET;
    instance->sink.octet = sink;
    instance->driver = driver;
    instance->ext.getbuffer = NULL;
}

void
chunk_sink_init(Sink *instance, ChunkSink sink, void *driver)
{
    instance->kind = DATA_KIND_CHUNK;
    instance->sink.chunk = sink;
    instance->driver = driver;
    instance->ext.getbuffer = NULL;
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
source_adapt(OctetSource source, void *driver, void *buf, const size_t n)
{
    unsigned char *data = buf;
    size_t rest = n;
    while (rest > 0) {
        const int rc = source(driver, data + n - rest);
        if (rc == -EINTR) {
            continue;
        } else if (rc < 0) {
            return (ssize_t)rc;
        }
        rest -= rc;
    }

    return 0;
}

static inline ssize_t
once_source_get_chunk(Source *source, void *buf, size_t n)
{
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
        if (get == -EINTR) {
            continue;
        } else if (get < 0) {
            return get;
        }
        rest -= get;
    }

    return (ssize_t)n;
}

static inline ssize_t
sink_adapt(OctetSink sink, void *driver, const void *buf, const size_t n)
{
    const unsigned char *data = buf;
    size_t rest = n;
    while (rest > 0) {
        const int rc = sink(driver, data[n - rest]);
        if (rc == -EINTR) {
            continue;
        } else if (rc < 0) {
            return (ssize_t)rc;
        }
        rest -= rc;
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
        if (put == -EINTR) {
            continue;
        } else if (put < 0) {
            return put;
        }
        rest -= put;
    }

    return (ssize_t)n;
}
