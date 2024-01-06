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
octet_source_init(Source *instance, ByteSource source, void *driver)
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
octet_sink_init(Sink *instance, ByteSink sink, void *driver)
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
source_adapt(ByteSource source, void *driver, void *buf, const size_t n)
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

ssize_t
source_get_chunk_atmost(Source *source, void *buf, const size_t n)
{
    return once_source_get_chunk(source, buf, n);
}

static inline ssize_t
sink_adapt(ByteSink sink, void *driver, const void *buf, const size_t n)
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

ssize_t
sink_put_chunk_atmost(Sink *sink, const void *buf, const size_t n)
{
    return once_sink_put_chunk(sink, buf, n);
}

static inline bool
channel_has_buffer_ext(Source *source, Sink *sink)
{
    return (source->ext.getbuffer != NULL || sink->ext.getbuffer != NULL);
}

static ssize_t
sts_atmost_via_sink(Source *source, Sink *sink, const size_t n)
{
    if (sink->ext.getbuffer == NULL) {
        return -ENOMEM;
    }

    ByteBuffer b = sink->ext.getbuffer(sink);
    void *buf = b.data + b.offset;
    const size_t rest = byte_buffer_rest(&b);
    if (rest == 0) {
        return -ENOMEM;
    }
    const size_t m = (n == 0 || rest < n) ? rest : n;
    return (source->kind == DATA_KIND_CHUNK)
        ? source->source.chunk(source->driver, buf, m)
        : source_get_chunk(source, buf, m);
}

static ssize_t
sts_atmost_via_source(Source *source, Sink *sink, const size_t n)
{
    if (source->ext.getbuffer == NULL) {
        return -EPIPE;
    }

    ByteBuffer b = source->ext.getbuffer(source);
    void *buf = b.data + b.offset;
    const size_t rest = byte_buffer_rest(&b);
    if (rest == 0) {
        /* A source shouldn't offer an empty buffer. */
        return -ENODATA;
    }
    const size_t m = (n == 0 || rest < n) ? rest : n;
    const ssize_t rc = (source->kind == DATA_KIND_CHUNK)
        ? source->source.chunk(source->driver, buf, m)
        : source_get_chunk(source, buf, m);
    return (rc < 0) ? rc : sink_put_chunk(sink, buf, rc);
}

ssize_t
sts_atmost(Source *source, Sink *sink, size_t n)
{
    if (channel_has_buffer_ext(source, sink) == false) {
        /* This works, but has a pretty heavy runtime overhead. Using sinks
         * with exposable buffers is preferable. */
        return sts_cbc(source, sink);
    }
    const ssize_t sinkrc = sts_atmost_via_sink(source, sink, n);
    return (sinkrc >= 0) ? sinkrc : sts_atmost_via_source(source, sink, n);
}

ssize_t
sts_some(Source *source, Sink *sink)
{
    return sts_atmost(source, sink, 0u);
}

ssize_t
sts_n(Source *source, Sink *sink, const size_t n)
{
    size_t rest = n;
    bool shortcut = false;
    while (rest > 0) {
        const ssize_t rc = shortcut
            ? sts_atmost_via_source(source, sink, rest)
            : sts_atmost(source, sink, rest);
        if (rc == -ENOMEM) {
            /* This means that the sink buffer is out of memory. If the source
             * can provide a buffer in the next iteration, we can go on,
             * otherwise we cannot. */
            shortcut = channel_has_buffer_ext(source, sink);
            continue;
        } else if (rc < 0) {
            return rc;
        }
        rest -= rc;
    }

    return n;
}

ssize_t
sts_drain(Source *source, Sink *sink)
{
    ssize_t rc = 0;
    bool shortcut = false;

    for (;;) {
        rc = shortcut
            ? sts_atmost_via_source(source, sink, 0u)
            : sts_atmost(source, sink, 0u);
        if (rc == -ENOMEM) {
            /* This means that the sink buffer is out of memory. If the source
             * can provide a buffer in the next iteration, we can go on,
             * otherwise we cannot. */
            shortcut = true;
            continue;
        } else if (rc < 0) {
            break;
        }
    }

    return rc;
}

ssize_t
sts_some_aux(Source *source, Sink *sink, ByteBuffer *b)
{
    void *buf = b->data + b->offset;
    const size_t n = byte_buffer_rest(b);
    const ssize_t rc = source_get_chunk_atmost(source, buf, n);
    return (rc < 0) ? rc : sink_put_chunk(sink, buf, n);
}

ssize_t
sts_atmost_aux(Source *source, Sink *sink, ByteBuffer *b, const size_t n)
{
    ByteBuffer buffer;
    memcpy(&buffer, b, sizeof(*b));
    if (buffer.size > n) {
        buffer.size = n;
    }
    return sts_some_aux(source, sink, &buffer);
}

ssize_t
sts_n_aux(Source *source, Sink *sink, ByteBuffer *b, const size_t n)
{
    size_t rest = n;

    while (rest > 0) {
        byte_buffer_rewind(b);
        const ssize_t rc = sts_atmost_aux(source, sink, b, rest);
        if (rc < 0) {
            return rc;
        }
        rest -= rc;
    }

    return n;
}

ssize_t
sts_drain_aux(Source *source, Sink *sink, ByteBuffer *b)
{
    const size_t n = b->size;
    ssize_t rc = 0;

    for (;;) {
        byte_buffer_rewind(b);
        rc = sts_atmost_aux(source, sink, b, n);
        if (rc < 0) {
            break;
        }

    }

    return rc;
}

ssize_t
sts_cbc(Source *source, Sink *sink)
{
    unsigned char buf;

    const int rc = source_get_octet(source, &buf);
    if (rc < 0) {
        return (ssize_t)rc;
    }

    return sink_put_octet(sink, buf);
}

ssize_t
sts_n_cbc(Source *source, Sink *sink, const size_t n)
{
    for (size_t i = 0u; i < n; ++i) {
        const ssize_t rc = sts_cbc(source, sink);
        if (rc < 0) {
            return rc;
        }
    }

    return n;
}

ssize_t
sts_drain_cbc(Source *source, Sink *sink)
{
    ssize_t rc = 0;

    for (;;) {
        rc = sts_cbc(source, sink);
        if (rc < 0) {
            break;
        }
    }

    return rc;
}
