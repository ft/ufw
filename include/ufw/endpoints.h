/*
 * Copyright (c) 2022-2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_SOURCES_AND_SINKS_H
#define INC_UFW_SOURCES_AND_SINKS_H

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

typedef ByteBuffer (*SinkGetBuffer)(Sink*);
typedef ByteBuffer (*SourceGetBuffer)(Source*);

typedef enum ufw_data_kind {
    DATA_KIND_OCTET,
    DATA_KIND_CHUNK
} DataKind;

struct ufw_source {
    DataKind kind;
    void *driver;
    union {
        ByteSource octet;
        ChunkSource chunk;
    } source;
    struct {
        SourceGetBuffer getbuffer;
    } ext;
};

struct ufw_sink {
    DataKind kind;
    void *driver;
    union {
        ByteSink octet;
        ChunkSink chunk;
    } sink;
    struct {
        SinkGetBuffer getbuffer;
    } ext;
};

#define OCTET_SOURCE_INIT(CB, DRIVER) { \
        .kind = DATA_KIND_OCTET,        \
        .driver = (DRIVER),             \
        .source.octet = (CB),           \
        .ext.getbuffer = NULL }

#define CHUNK_SOURCE_INIT(CB, DRIVER) { \
        .kind = DATA_KIND_CHUNK,        \
        .driver = (DRIVER),             \
        .source.chunk = (CB),           \
        .ext.getbuffer = NULL }

#define OCTET_SINK_INIT(CB, DRIVER) {   \
        .kind = DATA_KIND_OCTET,        \
        .driver = (DRIVER),             \
        .sink.octet = (CB),             \
        .ext.getbuffer = NULL }

#define CHUNK_SINK_INIT(CB, DRIVER) {   \
        .kind = DATA_KIND_CHUNK,        \
        .driver = (DRIVER),             \
        .sink.chunk = (CB),             \
        .ext.getbuffer = NULL }

void octet_source_init(Source*, ByteSource, void*);
void chunk_source_init(Source*, ChunkSource, void*);
void octet_sink_init(Sink*, ByteSink, void*);
void chunk_sink_init(Sink*, ChunkSink, void*);

int source_get_octet(Source*, void*);
int sink_put_octet(Sink*, unsigned char);

ssize_t source_get_chunk(Source*, void*, size_t);
ssize_t source_get_chunk_atmost(Source*, void*, size_t);
ssize_t sink_put_chunk(Sink*, const void*, size_t);
ssize_t sink_put_chunk_atmost(Sink*, const void*, size_t);

/*
 * Source to Sink Plumbing
 */

/* These makes sense in cases where either the source or the sink implements
 * the getbuffer extension. */
ssize_t sts_some(Source*, Sink*);
ssize_t sts_atmost(Source*, Sink*, size_t);
ssize_t sts_n(Source*, Sink*, size_t);
ssize_t sts_drain(Source*, Sink*);

/* These work for arbitrary sources and sinks, but requires extra copying to
 * and from the auxiliary buffer. */
ssize_t sts_some_aux(Source*, Sink*, ByteBuffer*);
ssize_t sts_atmost_aux(Source*, Sink*, ByteBuffer*, size_t);
ssize_t sts_n_aux(Source*, Sink*, ByteBuffer*, size_t);
ssize_t sts_drain_aux(Source*, Sink*, ByteBuffer*);

/* Character-by-Character Plumbing */
ssize_t sts_cbc(Source*, Sink*);
ssize_t sts_n_cbc(Source*, Sink*, size_t);
ssize_t sts_drain_cbc(Source*, Sink*);

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
ssize_t run_read(void*, void*, size_t);
void source_from_filedesc(Source*, int*);
#endif /* UFW_HAVE_POSIX_READ */

#ifdef UFW_HAVE_POSIX_WRITE
ssize_t run_write(void*, const void*, size_t);
void sink_to_filedesc(Sink*, int*);
#endif /* UFW_HAVE_POSIX_WRITE */

/*
 * Buffer based Sources and Sinks
 */

void source_from_buffer(Source*, ByteBuffer*);
void source_from_chunks(Source*, ByteChunks*);
void sink_to_buffer(Sink*, ByteBuffer*);

/*
 * Instrumentable Sources and Sinks
 */

#define INSTRUMENTABLE_ERROR_AT_COUNT BITLL(0)
#define INSTRUMENTABLE_ENABLE_TRACE   BITLL(1)

typedef struct ufw_instrumentable_buffer {
    uint64_t flags;
    struct {
        size_t read;
        size_t write;
    } count;
    struct {
        int number;
        size_t at;
    } error;
    ByteBuffer buffer;
} InstrumentableBuffer;

void instrumentable_set_trace(InstrumentableBuffer*, bool);
void instrumentable_no_error(InstrumentableBuffer*);
void instrumentable_error_at(InstrumentableBuffer*, size_t, int);
void instrumentable_source(Source*, InstrumentableBuffer*);
void instrumentable_sink(Sink*, InstrumentableBuffer*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_SOURCES_AND_SINKS_H */
