/*
 * Copyright (c) 2022-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
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

int varint_decode_u32(ByteBuffer*, uint32_t*);
int varint_decode_s32(ByteBuffer*,  int32_t*);
int varint_decode_u64(ByteBuffer*, uint64_t*);
int varint_decode_s64(ByteBuffer*,  int64_t*);

int varint_encode_u32(ByteBuffer*, uint32_t);
int varint_encode_s32(ByteBuffer*,  int32_t);
int varint_encode_u64(ByteBuffer*, uint64_t);
int varint_encode_s64(ByteBuffer*,  int64_t);

int varint_u32_from_source(Source*, uint32_t*);
int varint_s32_from_source(Source*,  int32_t*);
int varint_u64_from_source(Source*, uint64_t*);
int varint_s64_from_source(Source*,  int64_t*);

int varint_u32_to_sink(Sink*, uint32_t);
int varint_s32_to_sink(Sink*,  int32_t);
int varint_u64_to_sink(Sink*, uint64_t);
int varint_s64_to_sink(Sink*,  int64_t);

size_t varint_u32_length(uint32_t);
size_t varint_s32_length( int32_t);
size_t varint_u64_length(uint64_t);
size_t varint_s64_length( int64_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_VARIABLE_LENGTH_INTEGER_H */
