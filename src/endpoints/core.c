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
 * @file endpoints/core.c
 * @brief Implementation of a Sink and Source abstraction
 */

/**
 * @}
 */

#include <assert.h>
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

/**
 * Initialise a source of kind DATA_KIND_OCTET
 *
 * @param  instance  Pointer to the Source instance to initialise
 * @param  source    ByteSource function that should drive the source
 * @param  driver    Pointer to arbitrary data handed to the driver function
 *
 * @sideeffects Modifies the instance pointer
 */
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

/**
 * Initialise a source of kind DATA_KIND_CHUNK
 *
 * @param  instance  Pointer to the Source instance to initialise
 * @param  source    ChunkSource function that should drive the source
 * @param  driver    Pointer to arbitrary data handed to the driver function
 *
 * @sideeffects Modifies the instance pointer
 */
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

/**
 * Initialise a sink of kind DATA_KIND_OCTET
 *
 * @param  instance  Pointer to the Sink instance to initialise
 * @param  sink      ByteSink function that should drive the sink
 * @param  driver    Pointer to arbitrary data handed to the driver function
 *
 * @sideeffects Modifies the instance pointer
 */
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

/**
 * Initialise a sink of kind DATA_KIND_CHUNK
 *
 * @param  instance  Pointer to the Sink instance to initialise
 * @param  sink      ChunkSink function that should drive the sink
 * @param  driver    Pointer to arbitrary data handed to the driver function
 *
 * @sideeffects Modifies the instance pointer
 */
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

/**
 * Get a single octet from an arbitrary source
 *
 * This function gets a single datum from a source and stores it in the memory
 * pointed to by the data argument. The function can return error condition as
 * negative integers, with negated values from "errno.h".
 *
 * Upon successful completion, the number of transferred bytes (thus 1) is
 * returned.
 *
 * @param  source  Pointer to the source instance to read from
 * @param  data    Pointer to byte datum to store a single byte into
 *
 * @return Negative errno on failure, or the number of bytes read upon success.
 * @sideeffects Any sideeffects performed by the driver of the source instance.
 */
int
source_get_octet(Source *source, void *data)
{
    trace();
    return source->kind == DATA_KIND_OCTET
        ? source->source.octet(source->driver, data)
        : source->source.chunk(source->driver, data, 1U);
}

/**
 * Put a single octet from an arbitrary sink
 *
 * This function puts a single datum into a sink and stores it in the memory.
 * The function can return error condition as negative integers, with negated
 * values from "errno.h".
 *
 * Upon successful completion, the number of transferred bytes (thus 1) is
 * returned.
 *
 * @param  sink    Pointer to the sink instance to write to
 * @param  data    Single byte to put into the sink
 *
 * @return Negative errno on failure; the number of bytes written on success.
 * @sideeffects Any sideeffects performed by the driver of the sink instance.
 */
int
sink_put_octet(Sink *sink, const unsigned char data)
{
    trace();
    return sink->kind == DATA_KIND_OCTET
        ? sink->sink.octet(sink->driver, data)
        : sink->sink.chunk(sink->driver, &data, 1U);
}

/**
 * Implementation of the endpoint-retry machinery
 *
 * Note that this is an internal function!

 * Only call this function if retry->run is a valid callback function!
 *
 * The endpoint system performs automatic read/write retries on common system
 * issues, like EAGAIN, EINTR, and so on. There may be situations, in which it
 * is desirable to perform custom handling of these situations.
 *
 * This function returning something larger than zero means retry whatever
 * we've done before. Any other value will be returned from the endpoint as is.
 * This allows injecting sideeffects (such as waiting), remapping error codes
 * and custom handling of zero data.
 *
 * The set of controlled conditions can be chosen via retry->ctrl.
 *
 * @param  retry  Pointer to retry configuration to use
 * @param  drv    Pointer to endpoint driver
 * @param  rc     Copy of the return code that cause this function to be called
 *
 * @return A positive return value indicates the user wants the IO action that
 *         caused this function to be called to the retried. Other values will
 *         cause the system to abort the current transaction with that value.
 * @sideeffects None in of itself, but the retry system functions may cause
 *              arbitrary side-effects.
 */
static inline ssize_t
ep_retry(struct ufw_ep_retry *retry, void *drv, const ssize_t rc)
{
    trace();
    if (rc == -EAGAIN) {
        if (BIT_ISSET(retry->ctrl, EP_RETRY_CTRL_EAGAIN)) {
            return retry->run(drv, retry->data, rc);
        } else {
            return 1;
        }
    }

    if (rc == -EINTR) {
        if (BIT_ISSET(retry->ctrl, EP_RETRY_CTRL_EINTR)) {
            return retry->run(drv, retry->data, rc);
        } else {
            return 1;
        }
    }

    if (rc == 0) {
        if (BIT_ISSET(retry->ctrl, EP_RETRY_CTRL_NOTHING)) {
            return retry->run(drv, retry->data, rc);
        } else {
            return 1;
        }
    }

    if (BIT_ISSET(retry->ctrl, EP_RETRY_CTRL_OTHER)) {
        return retry->run(drv, retry->data, rc);
    }

    return rc;
}

/**
 * Perform chunk reads on an octet source
 *
 * This uses a separate buffer to implement chunk reads for octet sources.
 *
 * @param  source  ByteSource function pointer to adapt
 * @param  driver  Pointer to source driver data
 * @param  buf     Pointer to memory for the adaptation process
 * @param  n       Size of the memory pointed to by buf
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was read.
 * @sideeffects The procedure moves data from the source to the supplied
 *              memory.
 */
static inline ssize_t
source_adapt(ByteSource source, void *driver, void *buf, const size_t n)
{
    trace();
    unsigned char *data = buf;
    size_t rest = n;
    while (rest > 0) {
        const int rc = source(driver, data + n - rest);
        if (rc < 0) {
            return (rest == n) ? (ssize_t)rc : (ssize_t)(n - rest);
        } else if (rc == 0) {
            return (rest == n) ? -EAGAIN : (ssize_t)(n - rest);
        }
        rest -= rc;
    }

    return n;
}

/**
 * Read a chunk of data from a source without retry-logic
 *
 * This is similar to POSIX read() for arbitrary Source instances. There is no
 * retry logic in this function. Use source_get_chunk() for the full endpoint
 * functionality.
 *
 * @param  source  Pointer to the source to read from
 * @param  buf     Pointer to memory to read into
 * @param  n       Size of the memory pointed to by buf
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was read.
 * @sideeffects The procedure moves data from the source to the supplied
 *              memory.
 */
ssize_t
source_read(Source *source, void *buf, const size_t n)
{
    trace();
    return source->kind == DATA_KIND_OCTET
        ? source_adapt(source->source.octet, source->driver, buf, n)
        : source->source.chunk(source->driver, buf, n);
}

struct size_error {
    int error;
    size_t size;
};

/* This is the worker for source_get_chunk() and source_get_chunk_atmost().
 * Both use source_read() multiple times, but they react to incomplete reads
 * differently. That's why this returns both the error condition and the,
 * possibly partial, read count. */
static inline struct size_error
/* NOLINTNEXTLINE(readability-function-cognitive-complexity) */
source_read_multi(Source *source, void *buf, const size_t n)
{
    trace();
    struct size_error rc = { 0, 0U };
    size_t rest = n;

    if (n == 0 || n > SSIZE_MAX) {
        rc.error = -EINVAL;
        goto done;
    }

    if (source->retry.init != NULL) {
        source->retry.init(&source->retry);
    }

    while (rest > 0) {
        unsigned char *dst = buf;
        const size_t done = n - rest;
        const ssize_t get = source_read(source, dst + done, rest);
        if (get <= 0) {
            if (source->retry.run == NULL) {
                if (get == -EINTR || get == -EAGAIN) {
                    continue;
                } else if (get < 0) {
                    rc.error = (int)get;
                    rc.size = done;
                    goto done;
                }
            } else {
                const ssize_t retried =
                    ep_retry(&source->retry, source->driver, get);
                if (retried > 0) {
                    continue;
                } else if (retried == 0) {
                    rc.error = -ENODATA;
                    rc.size = done;
                    goto done;
                }
                /* negative */
                rc.error = (int)retried;
                rc.size = done;
                goto done;
            }
        }
        assert((size_t)get <= rest);
        rest -= get;
    }
    rc.size = n;

done:
    return rc;
}

/**
 * Read a chunk of exact size from a source
 *
 * This function reads from source until exactly the indicated amount of memory
 * is transferred. This may require a number of actual reads. The function
 * automatically retries reads on well-known error codes like EAGAIN and EINTR.
 * This retry behaviour can be customised by using the endpoint's retry
 * configuration.
 *
 * @param  source  Pointer to the source to read from
 * @param  buf     Pointer to memory to read into
 * @param  n       Exact amount of data to write into buf
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was read.
 * @sideeffects The procedure moves data from the source to the supplied
 *              memory.
 */
ssize_t
source_get_chunk(Source *source, void *buf, size_t n)
{
    trace();
    const struct size_error rc = source_read_multi(source, buf, n);
    /* If error is zero, n bytes where transfered. This is what this function
     * promises to do, so return the size, which will be equal to n. */
    assert(rc.size == n);
    assert(rc.size <= SSIZE_MAX);
    return (rc.error == 0) ? (ssize_t)rc.size : rc.error;
}

/**
 * Get a limited amount of bytes from a source
 *
 * This is very similar to source_get_chunk(), except that being able to read
 * less than the indicated amount of data is not an error. This can have a
 * number of uses, like reading data from a source that has a fixed but unknown
 * number of bytes to give. Reading this chunk by chunk will cause a final read
 * with less data in the general case. An example of such a source would be a
 * regular file on many operating systems.
 *
 * @param  source  Pointer to the source to read from
 * @param  buf     Pointer to memory to read into
 * @param  n       Maximum amount of data to write into buf
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was read.
 * @sideeffects The procedure moves data from the source to the supplied
 *              memory.
 */
ssize_t
source_get_chunk_atmost(Source *source, void *buf, const size_t n)
{
    trace();
    const struct size_error rc = source_read_multi(source, buf, n);
    /* Even if an error was signaled, if data was read, that satisfies the
     * promise of this function, so return the size. If nothing could be read,
     * however, return the error. */
    assert(rc.size <= n);
    assert(rc.size <= SSIZE_MAX);
    return (rc.size == 0) ? rc.error : (ssize_t)rc.size;
}

/**
 * Perform chunk writes on an octet sink
 *
 * This uses a separate buffer to implement writes reads for octet sinks.
 *
 * @param  sink    ByteSink function pointer to adapt
 * @param  driver  Pointer to sink driver data
 * @param  buf     Pointer to memory for the adaptation process
 * @param  n       Size of the memory pointed to by buf
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was written.
 * @sideeffects The procedure moves data from the supplied memory to the sink.
 */
static inline ssize_t
sink_adapt(ByteSink sink, void *driver, const void *buf, const size_t n)
{
    trace();
    const unsigned char *data = buf;
    size_t rest = n;
    while (rest > 0) {
        const int rc = sink(driver, data[n - rest]);
        if (rc < 0) {
            return (rest == n) ? (ssize_t)rc : (ssize_t)(n - rest);
        } else if (rc == 0) {
            return (rest == n) ? -EAGAIN : (ssize_t)(n - rest);
        }
        rest -= rc;
    }

    return n;
}

/**
 * Write a chunk of data to a sink without retry-logic
 *
 * This is similar to POSIX write() for arbitrary Sink instances. There is no
 * retry logic in this function. Use source_put_chunk() for the full endpoint
 * functionality.
 *
 * @param  sink  Pointer to sink instance to write to
 * @param  buf   Pointer to memory to read into
 * @param  n     Size of the memory pointed to by buf
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was written.
 * @sideeffects The procedure moves data from the supplied memory to the sink.
 */
ssize_t
sink_write(Sink *sink, const void *buf, const size_t n)
{
    trace();
    return sink->kind == DATA_KIND_OCTET
        ? sink_adapt(sink->sink.octet, sink->driver, buf, n)
        : sink->sink.chunk(sink->driver, buf, n);
}

/* Similar to source_read_multi(), this is the worker for sink_put_chunk() and
 * sink_put_chunk_atmost(). */
static inline struct size_error
/* NOLINTNEXTLINE(readability-function-cognitive-complexity) */
sink_write_multi(Sink *sink, const void *buf, const size_t n)
{
    trace();
    struct size_error rc = { 0, 0U };

    if (n == 0) {
        /* Can we write a buffer of zero size? Already done. */
        return rc;
    }

    if (n > SSIZE_MAX) {
        rc.error = -EINVAL;
        goto done;
    }

    if (sink->retry.init != NULL) {
        sink->retry.init(&sink->retry);
    }

    size_t rest = n;
    while (rest > 0) {
        const unsigned char *src = buf;
        const size_t done = n - rest;
        const ssize_t get = sink_write(sink, src + done, rest);
        if (get <= 0) {
            if (sink->retry.run == NULL) {
                if (get == -EINTR || get == -EAGAIN) {
                    continue;
                } else if (get < 0) {
                    rc.error = (int)get;
                    rc.size = done;
                    goto done;
                }
            } else {
                const ssize_t retried =
                    ep_retry(&sink->retry, sink->driver, get);
                if (retried > 0) {
                    continue;
                } else if (retried == 0) {
                    rc.error = -ENODATA;
                    rc.size = done;
                    goto done;
                }
                /* negative */
                rc.error = (int)retried;
                rc.size = done;
                goto done;
            }
        }
        assert((size_t)get <= rest);
        rest -= get;
    }
    rc.size = n;

done:
    return rc;
}

/**
 * Write a piece of memory to a Sink instance
 *
 * This function writes to a sink until exactly the indicated amount of memory
 * is transferred. This may require a number of actual writes. The function
 * automatically retries writes on well-known error codes like EAGAIN and
 * EINTR. This retry behaviour can be customised by using the endpoint's retry
 * configuration.
 *
 * @param  sink  Pointer to sink instance to write to
 * @param  buf   Pointer to memory to read into
 * @param  n     Exact amount of data to transfer from buf
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was written.
 * @sideeffects The procedure moves data from the supplied memory to the sink.
 */
ssize_t
sink_put_chunk(Sink *sink, const void *buf, const size_t n)
{
    trace();
    const struct size_error rc = sink_write_multi(sink, buf, n);
    assert(rc.size == n);
    assert(rc.size <= SSIZE_MAX);
    return (rc.error == 0) ? (ssize_t)rc.size : rc.error;
}

/**
 * Write a limited amount of data to a sink
 *
 * This is similar to sink_put_chunk(), with the difference in behaviour that
 * not tranmitting exactly the specified amount of data is not an error.
 *
 * @param  sink  Pointer to sink instance to write to
 * @param  buf   Pointer to memory to read into
 * @param  n     Maximum amount of data to transfer from buf
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was written.
 * @sideeffects The procedure moves data from the supplied memory to the sink.
 */
ssize_t
sink_put_chunk_atmost(Sink *sink, const void *buf, const size_t n)
{
    trace();
    const struct size_error rc = sink_write_multi(sink, buf, n);
    assert(rc.size <= n);
    assert(rc.size <= SSIZE_MAX);
    return (rc.size == 0) ? rc.error : (ssize_t)rc.size;
}

/**
 * Query is a source instance implements the getbuffer extension
 *
 * @param  source  Pointer to the source instance to query
 *
 * @return True of the source implements ths getbuffer extension; false
 *         otherwise.
 * @sideeffects None
 */
static inline bool
source_has_buffer_ext(const Source *source)
{
    trace();
    return (source->ext.getbuffer != NULL);
}

/**
 * Query is a sink instance implements the getbuffer extension
 *
 * @param  sink  Pointer to the source instance to query
 *
 * @return True of the sink implements ths getbuffer extension; false
 *         otherwise.
 * @sideeffects None
 */
static inline bool
sink_has_buffer_ext(const Sink *sink)
{
    trace();
    return (sink->ext.getbuffer != NULL);
}

/**
 * Query if a pair of source and sink endpoints implement getbuffer
 *
 * This returns true if either the source or the sink implements the getbuffer
 * extension.
 *
 * @param  source  Pointer to the source to query
 * @param  sink    Pointer to the sink to query
 *
 * @return True of either the sink or the source implements ths getbuffer
 *         extension; false otherwise.
 * @sideeffects None
 */
static inline bool
channel_has_buffer_ext(const Source *source, const Sink *sink)
{
    trace();
    return (source_has_buffer_ext(source) || sink_has_buffer_ext(sink));
}

int
source_seek(Source *source, size_t offset)
{
    if (source->ext.seek == NULL) {
        return -ENOTSUP;
    }

    return source->ext.seek(source->driver, offset);
}

int
sink_seek(Sink *sink, size_t offset)
{
    if (sink->ext.seek == NULL) {
        return -ENOTSUP;
    }

    return sink->ext.seek(sink->driver, offset);
}

/*
 * Plumbing API, Source-to-Sink (sts_)
 *
 * Endpoints can be plugged into each other, to form something akin to a
 * channel of some sort. These functions implement such plumbing, to transfer
 * data from sources to sinks. This is the Source->Sink pattern.
 *
 * Another pattern, that is possible is Sink->Source which can be described as
 * a conduit. Here, a sink where data can be written to, immediately also
 * implements a Source interface to be able to read the same data from. This
 * kind of access pattern is not implemented by this module yet.
 */

/**
 * Move data using a sink's exposed buffer
 *
 * This requires the Sink to implement the getbuffer extension.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  n       Limit for the amount of data being transferred
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
static ssize_t
sts_atmost_via_sink(Source *source, Sink *sink, const size_t n)
{
    trace();
    if (sink->ext.getbuffer == NULL) {
        return -EPIPE;
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

/**
 * Move data using a source's exposed buffer
 *
 * This requires the Source to implement the getbuffer extension.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  n       Limit for the amount of data being transferred
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
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

/**
 * Move data from source to sink
 *
 * Note that this will use the character-by-character (cbc) API if neither
 * source nor sink implement the getbuffer extension. The cbc API has a very
 * high overhead when copying large buffers.
 *
 * Consider using the APIs using an auxiliary buffer (aux) APIs that accept an
 * user provided buffer to perform larger transfers in chunks of the size of
 * that auxiliary buffer.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  n       Limit for the amount of data being transferred
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
ssize_t
sts_atmost(Source *source, Sink *sink, size_t n)
{
    trace();
    if (source_has_buffer_ext(source)) {
        return sts_atmost_via_source(source, sink, n);
    }
    if (sink_has_buffer_ext(sink)) {
        return sts_atmost_via_sink(source, sink, n);
    }
    return (n == 0U)
        ? sts_cbc(source, sink)
        : sts_atmost_cbc(source, sink, n);
}

/**
 * Transfer some data from a source to a sink endpoint
 *
 * This transfers some data, the exact amount is determined by the system. This
 * uses sts_atmost() under the hood, and the performance considerations
 * mentioned in that function's descriptions apply to this as well, therefore.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
ssize_t
sts_some(Source *source, Sink *sink)
{
    trace();
    return sts_atmost(source, sink, 0U);
}

/**
 * Transfer exactly n bytes from a source to a sink
 *
 * Note that this uses sts_atmost() under the hood, and the performace note
 * made in its description holds true for this function as well.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  n       Limit for the amount of data being transferred
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
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

/**
 * Completely drain a source into a sink
 *
 * Note that this uses sts_atmost() under the hood, and the performace note
 * made in its description holds true for this function as well.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
ssize_t
sts_drain(Source *source, Sink *sink)
{
    trace();
    ssize_t rc = 0;
    bool shortcut = false;

    for (;;) {
        rc = shortcut
            ? sts_atmost_via_source(source, sink, 0U)
            : sts_atmost(source, sink, 0U);
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

/**
 * Transfer some data from a source to a sink endpoint via aux buffer
 *
 * The value of "some" is up to the implementation.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  b       Pointer to ByteBuffer instance to use as auxiliary buffer
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
ssize_t
sts_some_aux(Source *source, Sink *sink, ByteBuffer *b)
{
    trace();
    byte_buffer_reset(b);
    void *buf = b->data + b->offset;
    /* This is the "implementation" mentioned in the API documentation. We are
     * not making this information part of that documentation, since it might
     * change. Currently the amount of data transferred is minimum of the data
     * available in th e source and the size of the auxiliary buffer. Also, any
     * effect of the _get…_atmost() function may reduce this. */
    const size_t n = byte_buffer_avail(b);
    const ssize_t rc = source_get_chunk_atmost(source, buf, n);
    return (rc == 0) ? -ENODATA
        : (rc < 0) ? rc
        : sink_put_chunk(sink, buf, rc);
}

/**
 * Transfer a limited amount of date from a source to a sink via aux buffer
 *
 * This is similar to sts_some_aux(), but allows for an additional limit to
 * imposed on the transfer.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  b       Pointer to ByteBuffer instance to use as auxiliary buffer
 * @param  n       Limit for the amount of data being transferred
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
ssize_t
sts_atmost_aux(Source *source, Sink *sink, ByteBuffer *b, const size_t n)
{
    trace();
    /* Here, we make a copy of the provided auxiliary buffer, to be able to
     * artificially limit its size to n. If its size is less than or equal to
     * n, the we don't have to do anything, because sts_some_aux() will already
     * do the correct limiting for the promise of this API to be true. */
    ByteBuffer buffer;
    memcpy(&buffer, b, sizeof(*b));
    if (buffer.size > n) {
        buffer.size = n;
    }
    return sts_some_aux(source, sink, &buffer);
}

/**
 * Transfer a fixed amount of data from a source to a sink via aux buffer
 *
 * This repeatedly calls sts_atmost_aux() until the desired amount of data
 * was transferred.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  b       Pointer to ByteBuffer instance to use as auxiliary buffer
 * @param  n       Exact amount of data being transferred
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
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

/**
 * Transfer all data from a source into a sink via auxiliary buffer
 *
 * This procedure transfers data from "source" to "sink" until the source runs
 * out of data. Transfers are conducted through the auxiliary buffer provided.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  b       Pointer to ByteBuffer instance to use as auxiliary buffer
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
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

/**
 * Transfer a character from source to sink
 *
 * This works with any source or sink, and does not require any additional
 * memory (like another buffer). However, transfering data like this has a
 * large overhead, and should only be used in places that can allow for this to
 * be the case.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
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

/**
 * Transfer a fixed amount of data from source to sink by character
 *
 * This uses sts_cbc() to transfer a fixed amount of data. See that function
 * for considerations regarding the function's runtime overhead.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  n       Exact amount of data being transferred
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
ssize_t
sts_n_cbc(Source *source, Sink *sink, const size_t n)
{
    trace();
    for (size_t i = 0U; i < n; ++i) {
        const ssize_t rc = sts_cbc(source, sink);
        if (rc < 0) {
            return rc;
        }
    }

    return n;
}

/**
 * Transfer a limited amount of data data from source to sink by character
 *
 * This uses sts_cbc() to transfer a limited amount of data. See that function
 * for considerations regarding the function's runtime overhead.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 * @param  n       Limit for the amount of data being transferred
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
ssize_t
sts_atmost_cbc(Source *source, Sink *sink, const size_t n)
{
    trace();
    for (size_t i = 0U; i < n; ++i) {
        const ssize_t rc = sts_cbc(source, sink);
        if (rc <= 0) {
            return (i == 0) ? rc : (ssize_t)i;
        }
    }

    return n;
}

/**
 * Transfer all data from a source into a sink by character
 *
 * This uses sts_cbc() to transfer all data from a source into a sink. See that
 * function for considerations regarding the function's runtime overhead.
 *
 * @param  source  Pointer of Source instance to read from
 * @param  sink    Pointer of Sink instance to write to
 *
 * @return Negative values use -errno to encode errors; other values indicate
 *         the amount of data that was transferred.
 * @sideeffects The procedure moves data from source to sink; with most real
 *              world implementations of those, that means IO, which is a side
 *              effect.
 */
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
