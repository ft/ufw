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
 * Similarly, drivers returning -EAGAIN are assumed to be in a temporary
 * situation that prevented the previous call from performing any work, and
 * thus the system will retry as well.
 *
 * The retry-logic for values like EAGAIN and EINTR can be customised. This is
 * done by way of the "retry" member, that contains two function pointers, one
 * for initialisation purposes (run each time a transaction is performed) and
 * one for each retry step done within such a transaction. The cases in which
 * customisation happens can be selected using the "ctrl" datum, which is a bit
 * mask that should be or'ed EP_RETRY_CTRL_* macros. Finally, an arbitrary data
 * pointer is passed to the retry runner function. This can be initialised by
 * the init function and used by the run function to achieve altered behaviour
 * of the runner depending on the retry state.
 *
 * Using a data count of zero, or one bigger than SSIZE_MAX causes the API to
 * return -EINVAL.
 */

#include <stdbool.h>
#include <stddef.h>
#ifdef UFW_WITH_EP_CORE_TRACE
#include <stdio.h>
#endif /* UFW_WITH_EP_CORE_TRACE */
#include <string.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/endpoints.h>

#ifdef UFW_WITH_EP_CORE_TRACE
#define trace() printf("# trace %s\n", __func__)
#else
#define trace() ((void)0)
#endif

void
octet_source_init(Source *instance, ByteSource source, void *driver)
{
    trace();
    instance->kind = DATA_KIND_OCTET;
    instance->source.octet = source;
    instance->driver = driver;
    instance->retry.run = NULL;
    instance->retry.init = NULL;
    instance->ext.getbuffer = NULL;
}

void
chunk_source_init(Source *instance, ChunkSource source, void *driver)
{
    trace();
    instance->kind = DATA_KIND_CHUNK;
    instance->source.chunk = source;
    instance->driver = driver;
    instance->retry.run = NULL;
    instance->retry.init = NULL;
    instance->ext.getbuffer = NULL;
}

void
octet_sink_init(Sink *instance, ByteSink sink, void *driver)
{
    trace();
    instance->kind = DATA_KIND_OCTET;
    instance->sink.octet = sink;
    instance->driver = driver;
    instance->retry.run = NULL;
    instance->retry.init = NULL;
    instance->ext.getbuffer = NULL;
}

void
chunk_sink_init(Sink *instance, ChunkSink sink, void *driver)
{
    trace();
    instance->kind = DATA_KIND_CHUNK;
    instance->sink.chunk = sink;
    instance->driver = driver;
    instance->retry.run = NULL;
    instance->retry.init = NULL;
    instance->ext.getbuffer = NULL;
}

int
source_get_octet(Source *source, void *data)
{
    trace();
    return source->kind == DATA_KIND_OCTET
        ? source->source.octet(source->driver, data)
        : source->source.chunk(source->driver, data, 1u);
}

int
sink_put_octet(Sink *sink, const unsigned char data)
{
    trace();
    return sink->kind == DATA_KIND_OCTET
        ? sink->sink.octet(sink->driver, data)
        : sink->sink.chunk(sink->driver, &data, 1u);
}

static inline ssize_t
ep_retry(struct ufw_ep_retry *retry, const DataKind kind,
         void *drv, const ssize_t rc)
{
    trace();
    /*
     * Only call this function if retry->run is a valid callback function!
     *
     * This function returning something larger than zero means retry whatever
     * we've done before. Any other value will be returned from the endpoint as
     * is. This allows injecting sideeffects (such as waiting), remapping error
     * codes and custom handling of zero data.
     *
     * The set of controlled conditions can be chosed via retry->ctrl.
     */
    if (rc == -EAGAIN) {
        if (BIT_ISSET(retry->ctrl, EP_RETRY_CTRL_EAGAIN)) {
            return retry->run(kind, drv, retry->data, rc);
        } else {
            return 1;
        }
    }

    if (rc == -EINTR) {
        if (BIT_ISSET(retry->ctrl, EP_RETRY_CTRL_EINTR)) {
            return retry->run(kind, drv, retry->data, rc);
        } else {
            return 1;
        }
    }

    if (rc == 0) {
        if (BIT_ISSET(retry->ctrl, EP_RETRY_CTRL_NOTHING)) {
            return retry->run(kind, drv, retry->data, rc);
        } else {
            return 1;
        }
    }

    if (BIT_ISSET(retry->ctrl, EP_RETRY_CTRL_OTHER)) {
        return retry->run(kind, drv, retry->data, rc);
    }

    return rc;
}

static inline ssize_t
source_adapt(ByteSource source, void *driver, struct ufw_ep_retry *retry,
             void *buf, const size_t n)
{
    trace();
    if (retry->init != NULL) {
        retry->init(DATA_KIND_OCTET, retry);
    }

    unsigned char *data = buf;
    size_t rest = n;
    while (rest > 0) {
        const int rc = source(driver, data + n - rest);
        if (rc <= 0) {
            if (retry->run == NULL) {
                if (rc == -EINTR || rc == -EAGAIN) {
                    continue;
                } else if (rc < 0) {
                    return (ssize_t)rc;
                }
            } else {
                const ssize_t retried =
                    ep_retry(retry, DATA_KIND_OCTET, driver, rc);
                if (retried > 0) {
                    continue;
                }
                return retried;
            }
        }
        rest -= rc;
    }

    return n;
}

static inline ssize_t
once_source_get_chunk(Source *source, void *buf, size_t n)
{
    trace();
    return source->kind == DATA_KIND_OCTET
        ? source_adapt(source->source.octet, source->driver, &source->retry, buf, n)
        : source->source.chunk(source->driver, buf, n);
}

ssize_t
source_get_chunk(Source *source, void *buf, size_t n)
{
    trace();
    if (n == 0 || n > SSIZE_MAX) {
        return -EINVAL;
    }

    if (source->retry.init != NULL) {
        source->retry.init(DATA_KIND_CHUNK, &source->retry);
    }

    size_t rest = n;
    while (rest > 0) {
        const ssize_t get = once_source_get_chunk(source, buf, rest);
        if (get <= 0) {
            if (source->retry.run == NULL) {
                if (get == -EINTR || get == -EAGAIN) {
                    continue;
                } else if (get < 0) {
                    return (ssize_t)get;
                }
            } else {
                const ssize_t retried =
                    ep_retry(&source->retry, DATA_KIND_CHUNK,
                             source->driver, get);
                if (retried > 0) {
                    continue;
                }
                return retried;
            }
        }
        rest -= get;
    }

    return (ssize_t)n;
}

ssize_t
source_get_chunk_atmost(Source *source, void *buf, const size_t n)
{
    trace();
    if (n == 0 || n > SSIZE_MAX) {
        return -EINVAL;
    }

    if (source->retry.init != NULL) {
        source->retry.init(DATA_KIND_CHUNK, &source->retry);
    }

    size_t got = 0;
    while (got == 0) {
        const ssize_t rc = once_source_get_chunk(source, buf, n);
        if (rc <= 0) {
            if (source->retry.run == NULL) {
                if (rc == -EINTR || rc == -EAGAIN) {
                    continue;
                } else if (rc < 0) {
                    return rc;
                }
            } else {
                const ssize_t retried =
                    ep_retry(&source->retry, DATA_KIND_CHUNK,
                             source->driver, rc);
                if (retried > 0) {
                    continue;
                }
                return retried;
            }
        }
        got = (size_t)rc;
    }

    return got;
}

static inline ssize_t
sink_adapt(ByteSink sink, void *driver, const void *buf,
           struct ufw_ep_retry *retry, const size_t n)
{
    trace();
    if (retry->init != NULL) {
        retry->init(DATA_KIND_OCTET, retry);
    }

    const unsigned char *data = buf;
    size_t rest = n;
    while (rest > 0) {
        const int rc = sink(driver, data[n - rest]);
        if (retry->run == NULL) {
            if (rc == -EINTR || rc == -EAGAIN) {
                continue;
            } else if (rc < 0) {
                return (ssize_t)rc;
            }
        } else {
            const ssize_t retried =
                ep_retry(retry, DATA_KIND_OCTET, driver, rc);
            if (retried > 0) {
                continue;
            }
            return retried;
        }
        rest -= rc;
    }

    return n;
}

static inline ssize_t
once_sink_put_chunk(Sink *sink, const void *buf, size_t n)
{
    trace();
    return sink->kind == DATA_KIND_OCTET
        ? sink_adapt(sink->sink.octet, sink->driver, buf, &sink->retry, n)
        : sink->sink.chunk(sink->driver, buf, n);
}

ssize_t
sink_put_chunk(Sink *sink, const void *buf, size_t n)
{
    trace();
    if (n == 0 || n > SSIZE_MAX) {
        return -EINVAL;
    }

    if (sink->retry.init != NULL) {
        sink->retry.init(DATA_KIND_CHUNK, &sink->retry);
    }

    const unsigned char *data = buf;
    size_t rest = n;
    while (rest > 0) {
        const ssize_t put = once_sink_put_chunk(sink, data + n - rest, rest);
        if (put <= 0) {
            if (sink->retry.run == NULL) {
                if (put == -EINTR || put == -EAGAIN) {
                    continue;
                } else if (put < 0) {
                    return (ssize_t)put;
                }
            } else {
                const ssize_t retried =
                    ep_retry(&sink->retry, DATA_KIND_CHUNK, sink->driver, put);
                if (retried > 0) {
                    continue;
                }
                return retried;
            }
        }
        rest -= put;
    }

    return (ssize_t)n;
}

ssize_t
sink_put_chunk_atmost(Sink *sink, const void *buf, const size_t n)
{
    trace();
    return once_sink_put_chunk(sink, buf, n);
}

static inline bool
channel_has_buffer_ext(Source *source, Sink *sink)
{
    trace();
    return (source->ext.getbuffer != NULL || sink->ext.getbuffer != NULL);
}

static ssize_t
sts_atmost_via_sink(Source *source, Sink *sink, const size_t n)
{
    trace();
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
    trace();
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
    trace();
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
    trace();
    return sts_atmost(source, sink, 0u);
}

ssize_t
sts_n(Source *source, Sink *sink, const size_t n)
{
    trace();
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
    trace();
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
    trace();
    byte_buffer_reset(b);
    void *buf = b->data + b->offset;
    const size_t n = byte_buffer_avail(b);
    const ssize_t rc = source_get_chunk_atmost(source, buf, n);
    return (rc < 0) ? rc : sink_put_chunk(sink, buf, rc);
}

ssize_t
sts_atmost_aux(Source *source, Sink *sink, ByteBuffer *b, const size_t n)
{
    trace();
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
    trace();
    size_t rest = n;

    while (rest > 0) {
        byte_buffer_reset(b);
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
    trace();
    byte_buffer_reset(b);
    const size_t n = b->size;
    ssize_t acc = 0;

    for (;;) {
        byte_buffer_reset(b);
        const ssize_t rc = sts_atmost_aux(source, sink, b, n);
        if (rc < 0) {
            if (rc == -ENODATA) {
                break;
            }
            return rc;
        }
        acc += rc;
    }

    return acc;
}

ssize_t
sts_cbc(Source *source, Sink *sink)
{
    trace();
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
    trace();
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
    trace();
    ssize_t rc = 0;

    for (;;) {
        rc = sts_cbc(source, sink);
        if (rc < 0) {
            break;
        }
    }

    return rc;
}
