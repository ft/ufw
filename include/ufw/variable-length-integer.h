/*
 * Copyright (c) 2022-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup varint Variable Length Integers
 *
 * Google Profobuf-style varint implementation
 *
 * Google's protobuf system specifies a way to implement integers that only
 * occupy as many bytes as their value's most-significant-bit demands. This is
 * an implementation of this encoding.
 *
 * @{
 *
 * @file ufw/variable-length-integer.h
 * @brief Variable Length Integers (Protobuf Style) API
 *
 * @}
 */

#ifndef INC_UFW_VARIABLE_LENGTH_INTEGER_H
#define INC_UFW_VARIABLE_LENGTH_INTEGER_H

#include <stdint.h>

#include <ufw/byte-buffer.h>
#include <ufw/endpoints.h>

#define VARINT_CONTINUATION_MASK 0x80u
#define VARINT_DATA_MASK         0x7fu
#define VARINT_DATA_BITS            7u
#define VARINT_32BIT_MAX_OCTETS     5u
#define VARINT_64BIT_MAX_OCTETS    10u

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int varint_decode_u32(ByteBuffer *b, uint32_t *n);
int varint_decode_s32(ByteBuffer *b, int32_t *n);
int varint_decode_u64(ByteBuffer *b, uint64_t *n);
int varint_decode_s64(ByteBuffer *b, int64_t *n);

int varint_encode_u32(ByteBuffer *b, uint32_t n);
int varint_encode_s32(ByteBuffer *b, int32_t n);
int varint_encode_u64(ByteBuffer *b, uint64_t n);
int varint_encode_s64(ByteBuffer *b, int64_t n);

int varint_u32_from_source(Source *source, uint32_t *n);
int varint_s32_from_source(Source *source, int32_t *n);
int varint_u64_from_source(Source *source, uint64_t *n);
int varint_s64_from_source(Source *source, int64_t *n);

int varint_u32_to_sink(Sink *sink, uint32_t n);
int varint_s32_to_sink(Sink *sink, int32_t n);
int varint_u64_to_sink(Sink *sink, uint64_t n);
int varint_s64_to_sink(Sink *sink, int64_t n);

size_t varint_u32_length(uint32_t n);
size_t varint_s32_length(int32_t n);
size_t varint_u64_length(uint64_t n);
size_t varint_s64_length(int64_t n);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_VARIABLE_LENGTH_INTEGER_H */
