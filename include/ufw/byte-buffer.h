/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_BYTE_BUFFER_H
#define INC_UFW_BYTE_BUFFER_H

/**
 * @addtogroup bytebuffer Byte Buffers
 *
 * Featureful access to memory buffers
 *
 * In C, strings are implemented using memory that is terminated by a `NUL`
 * byte. It is arguable whether or not that is a good design decision. With
 * arbitrary memory buffers this cannot done, because no byte can have special
 * semantics. Pointers in C are just that: They point to an address in memory.
 * There is no length information associated with it. What is worse is that
 * array arguments to functions degrade to pointers, losing any length
 * information that they may have carried at the calling site. This module
 * implements a data type, that embellishes a pointer with size information.
 * Additionally, there are two additional indices into the memory, that can be
 * used as read and write pointers into the memory referenced at by the
 * pointer.
 *
 * @{
 *
 * @file ufw/byte-buffer.h
 * @brief Byte Buffer API
 *
 * @}
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

int byte_buffer_set(ByteBuffer *b, void *data, size_t size,
                    size_t used, size_t offset);
void byte_buffer_null(ByteBuffer *b);

int byte_buffer_use(ByteBuffer *b, void *data, size_t size);
int byte_buffer_space(ByteBuffer *b, void *data, size_t size);

int byte_buffer_add(ByteBuffer *b, const void *data, size_t size);
int byte_buffer_consume(ByteBuffer *b, void *data, size_t size);
ssize_t byte_buffer_consume_at_most(ByteBuffer *b, void *data,
                            size_t size);

int byte_buffer_rewind(ByteBuffer *b);
void byte_buffer_clear(ByteBuffer *b);
void byte_buffer_reset(ByteBuffer *b);
void byte_buffer_repeat(ByteBuffer *b);

void byte_buffer_fill(ByteBuffer *b, unsigned char datum);
void byte_buffer_fillx(ByteBuffer *b, unsigned char init,
                       signed char increment);
void byte_buffer_fill_cb(ByteBuffer *b, size_t offset,
                         int (*cb)(size_t, unsigned char *));

size_t byte_buffer_avail(const ByteBuffer *b);
size_t byte_buffer_rest(const ByteBuffer *b);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_BYTE_BUFFER_H */
