/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_TYPES_H
#define INC_UFW_TYPES_H

#include <stddef.h>

#include <ufw/compat/ssize-t.h>

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

typedef struct ufw_octet_buffer {
    unsigned char *data;
    size_t size;
    size_t used;
    size_t offset;
} OctetBuffer;

#define OCTET_BUFFER_INIT(DATA, SIZE, LENGTH, OFFSET) { \
        .data = DATA,                                   \
        .size = SIZE,                                   \
        .used = LENGTH,                                 \
        .offset = OFFSET }

int octet_buffer_set(OctetBuffer*, void*, size_t, size_t, size_t);
void octet_buffer_null(OctetBuffer*);

int octet_buffer_use(OctetBuffer*, void*, size_t);
int octet_buffer_space(OctetBuffer*, void*, size_t);

int octet_buffer_add(OctetBuffer*, const void*, size_t);
int octet_buffer_consume(OctetBuffer*, void*, size_t);

int octet_buffer_rewind(OctetBuffer*);
void octet_buffer_clear(OctetBuffer*);
void octet_buffer_repeat(OctetBuffer*);

size_t octet_buffer_avail(OctetBuffer*);

#endif /* INC_UFW_TYPES_H */
