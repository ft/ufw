/*
 * Copyright (c) 2022-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup protolenp Length Prefix Framing
 *
 * Implementation of length prefix framing protocols
 *
 * This module implements two length prefix encoding schemes: One based on
 * variable-length integers as specified in Google's protobuf serialisation
 * format. The other is a family of encodings based on different fixed word
 * width integer encodings.
 *
 * Rationale: For unreliable data channels, RFC1055 (SLIP) implements a robust,
 * automatically re-synchronising kind of framing. SLIP however has two large
 * downsides: A large worst case overhead, and the inability to make use of
 * techniques such as DMA. For a SLIP implementation see the rfc1055.h module.
 *
 * For reliable channels, such as TCP, length prefixing immediately fixes the
 * latter, because it does not transform the its payload. For naïve integer
 * encodings, length prefixing either poses a significant overhead, if mainly
 * used with tiny frames, or it significantly limits the maximum representable
 * frame size.
 *
 * Using variable-length integers solves this issue: Small frames only take an
 * overhead of a single octet. Using 64 bit integers allow for, for all intends
 * and purposes, infinitely large frames.
 *
 * Of course, there is a limit, but with unsigned 64 bit values, 16 exbi bytes
 * (~18.4e18 bytes) is large by any standard. Since the endpoint API uses
 * negative numbers to encode errors, ufw's implementation allows a maximum of
 * half that.
 *
 * The actual limits depends on the size of `size_t` if that is smaller than
 * `uint64_t`. None of that should make any sort of a difference in embedded
 * systems.
 *
 * This implements two kinds of encoders: One that directly sends into a sink
 * and another, that keeps the encoded variant inside of a data type, in case
 * the result is required multiple times.
 *
 * The previously described API uses variable length integers, combining low
 * overhead with small frames while allowing for arbitrarily large frames as
 * well. While this is a rather elegant solution, there are systems that use a
 * fixed number of octets to perform a similar job. These systems use different
 * kinds of octet orders and word sizes in their encoding. We can use the API
 * from binary-format.h to deal with that, but it leads to a whole family of
 * different possible implementations.
 *
 * This module also implements this family of encodings for 8, 16, and 32 bits,
 * with little and big endian octet orders. Other word widths are possible of
 * course, but these two seem most applicable to embedded systems. So we will
 * stick with these for now. The code is written in a style that makes is easy
 * to extend these encoding kinds later.
 *
 * @{
 *
 * @file ufw/length-prefix.h
 * @brief Length prefix framing implementation
 *
 * @}
 */

#ifndef INC_UFW_LENGTH_PREFIX_H
#define INC_UFW_LENGTH_PREFIX_H

#include <stddef.h>

#include <ufw/byte-buffer.h>
#include <ufw/endpoints.h>
#include <ufw/variable-length-integer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum ufw_lenp_kind {
    LENP_VARIABLE,
    LENP_OCTET,
    LENP_LE_16BIT,
    LENP_LE_32BIT,
    LENP_BE_16BIT,
    LENP_BE_32BIT
} LengthPrefixKind;

typedef struct ufw_length_prefix_buffer {
    unsigned char prefix_[VARINT_64BIT_MAX_OCTETS];
    ByteBuffer prefix;
    ByteBuffer payload;
} LengthPrefixBuffer;

typedef struct ufw_length_prefix_chunks {
    unsigned char prefix_[VARINT_64BIT_MAX_OCTETS];
    ByteBuffer prefix;
    ByteChunks payload;
} LengthPrefixChunks;

int flenp_memory_encode(LengthPrefixKind, LengthPrefixBuffer*, void*, size_t);
int flenp_buffer_encode(LengthPrefixKind, LengthPrefixBuffer*, ByteBuffer*);
int flenp_buffer_encode_n(LengthPrefixKind, LengthPrefixBuffer*,
                          ByteBuffer*, size_t);
int flenp_chunks_use(LengthPrefixKind, LengthPrefixChunks*);

ssize_t flenp_memory_to_sink(LengthPrefixKind, Sink*, void*, size_t);
ssize_t flenp_buffer_to_sink(LengthPrefixKind, Sink*, ByteBuffer*);
ssize_t flenp_buffer_to_sink_n(LengthPrefixKind, Sink*, ByteBuffer*, size_t);
ssize_t flenp_chunks_to_sink(LengthPrefixKind, Sink*, ByteChunks*);

ssize_t flenp_memory_from_source(LengthPrefixKind, Source*, void*, size_t);
ssize_t flenp_buffer_from_source(LengthPrefixKind, Source*, ByteBuffer*);

ssize_t flenp_decode_source_to_sink(LengthPrefixKind, Source*, Sink*);

/*
 * For backward compatibility, we implement the lenp_* functions in terms of
 * flenp_* with  kind set to LENP_VARIABLE.
 */

static inline int
lenp_memory_encode(LengthPrefixBuffer *lpb, void *buf, size_t n)
{
    return flenp_memory_encode(LENP_VARIABLE, lpb, buf, n);
}

static inline int
lenp_buffer_encode(LengthPrefixBuffer *lpb, ByteBuffer *bb)
{
    return flenp_buffer_encode(LENP_VARIABLE, lpb, bb);
}

static inline int
lenp_buffer_encode_n(LengthPrefixBuffer *lpb, ByteBuffer *bb, size_t n)
{
    return flenp_buffer_encode_n(LENP_VARIABLE, lpb, bb, n);
}

static inline int
lenp_chunks_use(LengthPrefixChunks *lpc)
{
    return flenp_chunks_use(LENP_VARIABLE, lpc);
}

static inline ssize_t
lenp_memory_to_sink(Sink *sink, void *buf, size_t n)
{
    return flenp_memory_to_sink(LENP_VARIABLE, sink, buf, n);
}

static inline ssize_t
lenp_buffer_to_sink(Sink *sink, ByteBuffer *bb)
{
    return flenp_buffer_to_sink(LENP_VARIABLE, sink, bb);
}

static inline ssize_t
lenp_buffer_to_sink_n(Sink *sink, ByteBuffer *bb, size_t n)
{
    return flenp_buffer_to_sink_n(LENP_VARIABLE, sink, bb, n);
}

static inline ssize_t
lenp_chunks_to_sink(Sink *sink, ByteChunks *bc)
{
    return flenp_chunks_to_sink(LENP_VARIABLE, sink, bc);
}

static inline ssize_t
lenp_memory_from_source(Source *source, void *buf, size_t n)
{
    return flenp_memory_from_source(LENP_VARIABLE, source, buf, n);
}

static inline ssize_t
lenp_buffer_from_source(Source *source, ByteBuffer *bb)
{
    return flenp_buffer_from_source(LENP_VARIABLE, source, bb);
}

static inline ssize_t
lenp_decode_source_to_sink(Source *source, Sink *sink)
{
    return flenp_decode_source_to_sink(LENP_VARIABLE, source, sink);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_LENGTH_PREFIX_H */
