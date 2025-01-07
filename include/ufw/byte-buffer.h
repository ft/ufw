/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_BYTE_BUFFER_H
#define INC_UFW_BYTE_BUFFER_H

/**
 * @addtogroup bytebuffer Byte Buffers
 * @{
 */

/**
 * @file byte-buffer.h
 * @brief Byte Buffer API
 */

#include <stddef.h>

#include <ufw/compat/ssize-t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct ufw_byte_buffer {
    unsigned char *data;
    size_t size;
    size_t used;
    size_t offset;
} ByteBuffer;

typedef struct ufw_byte_chunks {
    size_t chunks;
    size_t active;
    ByteBuffer *chunk;
} ByteChunks;

#define BYTE_BUFFER_INIT(DATA, SIZE, LENGTH, OFFSET) {  \
        .data = (DATA),                                 \
        .size = (SIZE),                                 \
        .used = (LENGTH),                               \
        .offset = (OFFSET) }

#define BYTE_BUFFER(DATA, SIZE) {  \
        .data = (DATA),            \
        .size = (SIZE),            \
        .used = (SIZE),            \
        .offset = 0u }

#define BYTE_BUFFER_EMPTY(DATA, SIZE) {  \
        .data = (DATA),                  \
        .size = (SIZE),                  \
        .used = 0u,                      \
        .offset = 0u }

#define BYTE_CHUNKS(VAR) {                      \
        .chunks = sizeof(VAR)/sizeof(*(VAR)),   \
        .active = 0u,                           \
        .chunk = (VAR)                          }

int byte_buffer_set(ByteBuffer*, void*, size_t, size_t, size_t);
void byte_buffer_null(ByteBuffer*);

int byte_buffer_use(ByteBuffer*, void*, size_t);
int byte_buffer_space(ByteBuffer*, void*, size_t);

int byte_buffer_add(ByteBuffer*, const void*, size_t);
int byte_buffer_consume(ByteBuffer*, void*, size_t);
ssize_t byte_buffer_consume_at_most(ByteBuffer*, void*, size_t);

int byte_buffer_rewind(ByteBuffer*);
void byte_buffer_clear(ByteBuffer*);
void byte_buffer_reset(ByteBuffer*);
void byte_buffer_repeat(ByteBuffer*);

void byte_buffer_fill(ByteBuffer*, unsigned char);
void byte_buffer_fillx(ByteBuffer*, unsigned char, signed char);
void byte_buffer_fill_cb(ByteBuffer*, size_t, int(*)(size_t, unsigned char*));

size_t byte_buffer_avail(const ByteBuffer*);
size_t byte_buffer_rest(const ByteBuffer*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* INC_UFW_BYTE_BUFFER_H */
