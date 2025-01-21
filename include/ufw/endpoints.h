/*
 * Copyright (c) 2022-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_SOURCES_AND_SINKS_H
#define INC_UFW_SOURCES_AND_SINKS_H

/**
 * @addtogroup endpoints Endpoints
 *
 * Abstraction for generic source and sink endpoints
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
 * `-ERRNO`. Additionally, when sources run out of data permanently, they need
 * to return `-ENODATA`. When sinks run out of space to store date, they need
 * to return `-ENOMEM`. Returning zero is allowed, strictly, but will cause the
 * system to retry. If that is not meaningful with your endpoint, return a
 * meaningful error code instead. Drivers returning `-EINTR` will cause the
 * system to assume the operation was interrupted and it will thus retry.
 * Similarly, drivers returning -EAGAIN are assumed to be in a temporary
 * situation that prevented the previous call from performing any work, and
 * thus the system will retry as well.
 *
 * The retry-logic for values like `EAGAIN` and `EINTR` can be customised. This
 * is done by way of the "retry" member, that contains two function pointers,
 * one for initialisation purposes (run each time a transaction is performed)
 * and one for each retry step done within such a transaction. The cases in
 * which customisation happens can be selected using the "ctrl" datum, which is
 * a bit mask that should be or'ed `EP_RETRY_CTRL_*` macros. Finally, an
 * arbitrary data pointer is passed to the retry runner function. This can be
 * initialised by the init function and used by the run function to achieve
 * altered behaviour of the runner depending on the retry state.
 *
 * Using a data count of zero, or one bigger than `SSIZE_MAX` causes the API to
 * return `-EINVAL`.
 *
 * Some of the naming conventions used herein:
 *
 * - `sts_`: Source to Sink plumbing APIs
 *
 * - `_cbc`: Character by Character transfer APIs
 *
 * - `_aux`: APIs using user-supplied auxiliary buffers
 *
 * - `_some`: APIs that transfer some, greater than zero bytes, amount of data,
 *   determined by the system
 *
 * - `_atmost`: APIs that transfer some, greater than zero bytes, amount of
 *    data, determined by the user.
 *
 * - `_n`: APIs that transfer an exact amount of data; exactly n bytes.
 *
 * - `_drain`: APIs that transfer all data available in a source into a sink.
 *
 * The module offers the low-level APIs `source_read()` and `sink_write()`,
 * which implement similar functionality as POSIX `read()` and `write()`, but
 * for arbitrary sources and sinks. In most cases it is advisable to use the
 * higher level functions, that implement common issue handling on top of the
 * these low level functions.
 *
 * @{
 *
 * @file ufw/endpoints.h
 * @brief Endpoints API
 *
 * @}
 */

#include <stdbool.h>
#include <stddef.h>

#include <ufw/compat/ssize-t.h>
#include <ufw/toolchain.h>

#include <ufw/bit-operations.h>
#include <ufw/byte-buffer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Core Data Types
 */

typedef struct ufw_sink Sink;
typedef struct ufw_source Source;

/**
 * Function type that accepts an octet
 *
 * These functions return integer values and accept an arbitrary data pointer,
 * which the individual application may optionally use. The byte parameter is
 * the data to be transfered into the sink.
 */
typedef int (*ByteSink)(void*, unsigned char);

/**
 * Function type that produces an octet
 *
 * These functions return integer values (negative values indicating an error)
 * and accept an arbitrary data pointer, which the individual application may
 * optionally use. The second byte parameter pointer is used to return the
 * produced octet.
 */
typedef int (*ByteSource)(void*, void*);

/** Function type that accepts a buffer of octets */
typedef ssize_t (*ChunkSink)(void*, const void*, size_t);

/** Function type that produces a buffer of octets */
typedef ssize_t (*ChunkSource)(void*, void*, size_t);

typedef enum ufw_data_kind {
    DATA_KIND_OCTET,
    DATA_KIND_CHUNK
} DataKind;

typedef ByteBuffer (*SinkGetBuffer)(Sink*);
typedef ByteBuffer (*SourceGetBuffer)(Source*);

struct ufw_ep_retry;
typedef void (*EndpointRetryInit)(struct ufw_ep_retry*);
typedef ssize_t (*EndpointRetry)(void*, void*, ssize_t);
typedef int (*EndpointSeek)(void*, size_t);

struct ufw_ep_retry {
    EndpointRetryInit init;
    EndpointRetry run;
    uint32_t ctrl;
    void *data;
};

#define EP_RETRY_INIT { .init = NULL, .run = NULL, .ctrl = 0u, .data = NULL }
#define EP_RETRY_CTRL_OTHER    BITL(0)
#define EP_RETRY_CTRL_NOTHING  BITL(1)
#define EP_RETRY_CTRL_EAGAIN   BITL(2)
#define EP_RETRY_CTRL_EINTR    BITL(3)

struct ufw_source {
    DataKind kind;
    void *driver;
    union {
        ByteSource octet;
        ChunkSource chunk;
    } source;
    struct ufw_ep_retry retry;
    struct {
        SourceGetBuffer getbuffer;
        EndpointSeek seek;
    } ext;
};

struct ufw_sink {
    DataKind kind;
    void *driver;
    union {
        ByteSink octet;
        ChunkSink chunk;
    } sink;
    struct ufw_ep_retry retry;
    struct {
        SinkGetBuffer getbuffer;
        EndpointSeek seek;
    } ext;
};

#define OCTET_SOURCE_INIT(CB, DRIVER) { \
        .kind = DATA_KIND_OCTET,        \
        .driver = (DRIVER),             \
        .source.octet = (CB),           \
        .retry = EP_RETRY_INIT,         \
        .ext.getbuffer = NULL,          \
        .ext.seek = NULL }

#define CHUNK_SOURCE_INIT(CB, DRIVER) { \
        .kind = DATA_KIND_CHUNK,        \
        .driver = (DRIVER),             \
        .source.chunk = (CB),           \
        .retry = EP_RETRY_INIT,         \
        .ext.getbuffer = NULL,          \
        .ext.seek = NULL }

#define OCTET_SINK_INIT(CB, DRIVER) {   \
        .kind = DATA_KIND_OCTET,        \
        .driver = (DRIVER),             \
        .sink.octet = (CB),             \
        .retry = EP_RETRY_INIT,         \
        .ext.getbuffer = NULL,          \
        .ext.seek = NULL }

#define CHUNK_SINK_INIT(CB, DRIVER) {   \
        .kind = DATA_KIND_CHUNK,        \
        .driver = (DRIVER),             \
        .sink.chunk = (CB),             \
        .retry = EP_RETRY_INIT,         \
        .ext.getbuffer = NULL,          \
        .ext.seek = NULL }

void octet_source_init(Source *instance, ByteSource source, void *driver);
void chunk_source_init(Source *instance, ChunkSource source, void *driver);
void octet_sink_init(Sink *instance, ByteSink sink, void *driver);
void chunk_sink_init(Sink *instance, ChunkSink sink, void *driver);

int source_get_octet(Source *source, void *data);
int sink_put_octet(Sink *sink, unsigned char data);

ssize_t source_read(Source *source, void *buf, size_t n);
ssize_t sink_write(Sink *sink, const void *buf, size_t n);

ssize_t source_get_chunk(Source *source, void *buf, size_t n);
ssize_t source_get_chunk_atmost(Source *source, void *buf, size_t n);
ssize_t sink_put_chunk(Sink *sink, const void *buf, size_t n);
ssize_t sink_put_chunk_atmost(Sink *sink, const void *buf, size_t n);

int source_seek(Source *source, size_t offset);
int sink_seek(Sink *sink, size_t offset);

/*
 * Source to Sink Plumbing
 */

/* These makes sense in cases where either the source or the sink implements
 * the getbuffer extension. */
ssize_t sts_some(Source *source, Sink *sink);
ssize_t sts_atmost(Source *source, Sink *sink, size_t n);
ssize_t sts_n(Source *source, Sink *sink, size_t n);
ssize_t sts_drain(Source *source, Sink *sink);

/* These work for arbitrary sources and sinks, but requires extra copying to
 * and from the auxiliary buffer. */
ssize_t sts_some_aux(Source *source, Sink *sink, ByteBuffer *b);
ssize_t sts_atmost_aux(Source *source, Sink *sink, ByteBuffer *b, size_t n);
ssize_t sts_n_aux(Source *source, Sink *sink, ByteBuffer *b, size_t n);
ssize_t sts_drain_aux(Source *source, Sink *sink, ByteBuffer *b);

/* Character-by-Character Plumbing */
ssize_t sts_cbc(Source *source, Sink *sink);
ssize_t sts_atmost_cbc(Source *source, Sink *sink, size_t n);
ssize_t sts_n_cbc(Source *source, Sink *sink, size_t n);
ssize_t sts_drain_cbc(Source *source, Sink *sink);

/*
 * Generic Sources and Sinks
 */

extern Source source_empty;
extern Source source_zero;
extern Sink sink_null;

/*
 * File descriptor based Sources and Sinks
 */

#ifdef UFW_HAVE_POSIX_READ
ssize_t run_read(void *driver, void *data, size_t n);
void source_from_filedesc(Source *instance, int *fd);
#endif /* UFW_HAVE_POSIX_READ */

#ifdef UFW_HAVE_POSIX_WRITE
ssize_t run_write(void *driver, const void *data, size_t n);
void sink_to_filedesc(Sink *instance, int *fd);
#endif /* UFW_HAVE_POSIX_WRITE */

/*
 * Buffer based Sources and Sinks
 */

void source_from_buffer(Source *instance, ByteBuffer *buffer);
void source_from_chunks(Source *instance, ByteChunks *chunks);
void sink_to_buffer(Sink *instance, ByteBuffer *buffer);

/*
 * Instrumentable Sources and Sinks
 */

/* Common flags */
#define INSTRUMENTABLE_COMMON_ENABLE_TRACE   BITLL(0)

/* Error specific flags */
#define INSTRUMENTABLE_UNTIL_FAILURE  BITLL(0)
#define INSTRUMENTABLE_UNTIL_SUCCESS  BITLL(1)

typedef struct ufw_instrumentable_access_stats {
    size_t bytes;
    size_t accesses;
} InstrumentableAccessStats;

#define INSTRUMENTABLE_ACCESS_STATS { .bytes = 0u, .accesses = 0u }

typedef struct ufw_instrumentable_error {
    uint64_t flags;
    int number;
    size_t at;
} InstrumentableError;

#define INSTRUMENTABLE_ERROR { .flags = 0u, .number = 0, .at = 0u }

typedef struct ufw_instrumentable_buffer {
    uint64_t flags;
    struct {
        InstrumentableAccessStats stat;
        InstrumentableError error;
    } read;
    struct {
        InstrumentableAccessStats stat;
        InstrumentableError error;
    } write;
    /**
     * Chunk size of chunk-based endpoints. We're simulating an endpoint making
     * at most chunksize transfers. This is useful for testing the repetition
     * logic.
     */
    size_t chunksize;
    /** The actual bytebuffer, that is enhanced by this data type. */
    ByteBuffer buffer;
} InstrumentableBuffer;

#define INSTRUMENTABLE_BUFFER(DATA, SIZE)               \
    {                                                   \
        .flags = 0u,                                    \
        .read.stat = INSTRUMENTABLE_ACCESS_STATS,       \
        .read.error = INSTRUMENTABLE_ERROR,             \
        .write.stat = INSTRUMENTABLE_ACCESS_STATS,      \
        .write.error = INSTRUMENTABLE_ERROR,            \
        .chunksize = 0u,                                \
        .buffer = BYTE_BUFFER_EMPTY(DATA, SIZE)         \
    }

void instrumentable_set_trace(InstrumentableBuffer *b, bool value);
void instrumentable_source(
    DataKind kind, Source *instance, InstrumentableBuffer *buffer);
void instrumentable_sink(
    DataKind kind, Sink *instance, InstrumentableBuffer *buffer);
void instrumentable_until_error_at(
    InstrumentableError *e, size_t offset, int n);
void instrumentable_until_success_at(
    InstrumentableError *e, size_t offset, int n);
void instrumentable_reset_error(InstrumentableError *e);
void instrumentable_reset_stats(InstrumentableAccessStats *s);

static inline void
instrumentable_chunksize(InstrumentableBuffer *b, const size_t n)
{
    b->chunksize = n;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_SOURCES_AND_SINKS_H */
