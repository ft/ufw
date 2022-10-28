/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_SOURCES_AND_SINKS_H
#define INC_UFW_SOURCES_AND_SINKS_H

#include <stddef.h>

#include <ufw/compat/ssize-t.h>
#include <ufw/toolchain.h>

#include <ufw/bit-operations.h>
#include <ufw/octet-buffer.h>

/*
 * Core Data Types
 */

/**
 * Function type that accepts an octet
 *
 * These functions return integer values and accept an arbitrary data pointer,
 * which the individual application may optionally use. The byte parameter is
 * the data to be transfered into the sink.
 */
typedef int (*OctetSink)(void*, unsigned char);

/**
 * Function type that produces an octet
 *
 * These functions return integer values (negative values indicating an error)
 * and accept an arbitrary data pointer, which the individual application may
 * optionally use. The second byte parameter pointer is used to return the
 * produced octet.
 */
typedef int (*OctetSource)(void*, void*);

/** Function type that accepts a buffer of octets */
typedef ssize_t (*ChunkSink)(void*, const void*, size_t);

/** Function type that produces a buffer of octets */
typedef ssize_t (*ChunkSource)(void*, void*, size_t);

typedef enum ufw_data_kind {
    DATA_KIND_OCTET,
    DATA_KIND_CHUNK
} DataKind;

typedef struct ufw_source {
    DataKind kind;
    void *driver;
    union {
        OctetSource octet;
        ChunkSource chunk;
    } source;
} Source;

typedef struct ufw_sink {
    DataKind kind;
    void *driver;
    union {
        OctetSink octet;
        ChunkSink chunk;
    } sink;
} Sink;

#define OCTET_SOURCE_INIT(CB, DRIVER) { \
        .kind = DATA_KIND_OCTET,        \
        .driver = DRIVER,               \
        .source.octet = CB }

#define CHUNK_SOURCE_INIT(CB, DRIVER) { \
        .kind = DATA_KIND_CHUNK,        \
        .driver = DRIVER,               \
        .source.chunk = CB }

#define OCTET_SINK_INIT(CB, DRIVER) {   \
        .kind = DATA_KIND_OCTET,        \
        .driver = DRIVER,               \
        .sink.octet = CB }

#define CHUNK_SINK_INIT(CB, DRIVER) {   \
        .kind = DATA_KIND_CHUNK,        \
        .driver = DRIVER,               \
        .sink.chunk = CB }

void octet_source_init(Source*, OctetSource, void*);
void chunk_source_init(Source*, ChunkSource, void*);
void octet_sink_init(Sink*, OctetSink, void*);
void chunk_sink_init(Sink*, ChunkSink, void*);

int source_get_octet(Source*, void*);
int sink_put_octet(Sink*, unsigned char);

ssize_t source_get_chunk(Source*, void*, size_t);
ssize_t sink_put_chunk(Sink*, const void*, size_t);

/*
 * Generic Sources and Sinks
 */

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

void source_from_buffer(Source*, OctetBuffer*);
void sink_to_buffer(Sink*, OctetBuffer*);

/*
 * Instrumentable Sources and Sinks
 */

#define INSTRUMENTABLE_ERROR_AT_COUNT BITLL(0)

typedef struct ufw_instrumentable_buffer {
    uint64_t flags;
    struct {
        int number;
        size_t at;
    } error;
    OctetBuffer buffer;
} InstrumentableBuffer;

void instrumentable_no_error(InstrumentableBuffer*);
void instrumentable_error_at(InstrumentableBuffer*, size_t, int);
void instrumentable_source(Source*, InstrumentableBuffer*);
void instrumentable_sink(Sink*, InstrumentableBuffer*);

#endif /* INC_UFW_SOURCES_AND_SINKS_H */
