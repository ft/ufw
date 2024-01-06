/*
 * Copyright (c) 2019-2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 *
 * This file is generated by ‘tools/make-binary-format’.
 */

/**
 * @file binary-format.h
 * @brief Binary format conversion API
 *
 * This module implements a number of functions that revolve around reading and
 * writing value to/from raw memory. That is three octet-orders (endianness):
 * Native, big and little; where native endianness is one of the other two.
 *
 * The API follows a naming scheme:
 *
 *     bf_[OPERATION]_[TYPEMNEMONIC][WIDTH][ORDER](...)
 *
 * Where OPERATION is either `ref` or `set`; and
 *
 * Where TYPEMNEMONIC is either `u` or `s` or `f` for unsigned integers, signed
 * integers and floating point values respectively; and
 *
 * Where OPERATION is either `ref` or `set`; and
 *
 * Where WIDTH is a width designation in bits. Integer operations support 16,
 * 32 and 64 bits; floating point values support 32 and 64 bits; and
 *
 * Where ORDER is either `n` or `b` or `l` for native, big or little endianness
 * respectively.
 *
 * To put a 32-bit floating point value into memory, in big-endian octet order:
 *
 * @code
 *     uint8_t memory[128];
 *     // ...
 *     bf_set_f32b(memory, 123.f);
 * @endcode
 *
 * Integer behaviour is pretty portable these days. Two's complement is used
 * universally in all modern architectures. There are things like IBM main
 * frames that use BCD for example, and there will be other examples, that
 * could be brought up here.
 *
 * Floating point numbers are less portable, but IEEE754 is pretty common.
 * This module implements all accesses on the architecture's representation.
 *
 * The module works with big- and little-endian archtectures and using bytes
 * of either eight or sixteen bit size.
 *
 * If your system does not use two's complement, signed-integer semantics will
 * be off. And similarly if your system does not use IEEE754 floating point
 * encoding, those semantics will be off.
 */

#ifndef INC_UFW_BINARY_FORMAT_H
#define INC_UFW_BINARY_FORMAT_H

#include <stdint.h>

#include <ufw/bit-operations.h>
#include <ufw/toolchain.h>

#ifdef __cplusplus
#ifndef CXX_ALLOW_TYPE_PUNNING
#warning "binary-format uses type punning, which is undefined behaviour in C++!"
#warning "Your toolchain may allow it as an extension, but be advised!"
#warning "To disable this warning, define the CXX_ALLOW_TYPE_PUNNING macro."
#endif /* CXX_ALLOW_TYPE_PUNNING */
#endif /* __cplusplus */

#if !(defined(SYSTEM_ENDIANNESS_BIG)) && !(defined(SYSTEM_ENDIANNESS_LITTLE))
#error "System octet-order is not indicated! Cannot use binary-format.h for that reason!"
#endif /* !(defined(SYSTEM_ENDIANNESS_*)) */

#if !(UFW_BITS_PER_BYTE == 8 || UFW_BITS_PER_BYTE == 16)
#error "System byte-size is unsupported! Cannot use binary-format.h for that reason!"
#endif /* Unsupported Byte Size */

union bf_convert16 {
    uint16_t u16;
    int16_t s16;
};

union bf_convert32 {
    uint32_t u32;
    int32_t s32;
    float f32;
};

union bf_convert64 {
    uint64_t u64;
    int64_t s64;
    double f64;
};

/**
 * Byte-swap 16 bit value
 *
 * Turn `0x1234` into `0x3412`.
 *
 * @param  value   The value to transform
 *
 * @return The byte-swapped value.
 * @sideeffects None.
 */
static inline uint16_t
bf_swap16(const uint16_t value)
{
#if defined(HAVE_COMPILER_BUILTIN_BSWAP16) && defined(UFW_USE_BUILTIN_SWAP)
    return __builtin_bswap16(value);
#else
    return ( ((value & 0xff00u) >> 8u)
           | ((value & 0x00ffu) << 8u));
#endif /* HAVE_COMPILER_BUILTIN_BSWAP16 */
}

/**
 * Byte-swap 32 bit value
 *
 * Turn `0x12345678` into `0x78563412`.
 *
 * @param  value   The value to transform
 *
 * @return The byte-swapped value.
 * @sideeffects None.
 */
static inline uint32_t
bf_swap32(const uint32_t value)
{
#if defined(HAVE_COMPILER_BUILTIN_BSWAP32) && defined(UFW_USE_BUILTIN_SWAP)
    return __builtin_bswap32(value);
#else
    return ( ((value & 0xff000000ul) >> 24u)
           | ((value & 0x00ff0000ul) >>  8u)
           | ((value & 0x0000ff00ul) <<  8u)
           | ((value & 0x000000fful) << 24u));
#endif /* HAVE_COMPILER_BUILTIN_BSWAP32 */
}

/**
 * Byte-swap 64 bit value
 *
 * Turn `0x1234567890abcdef` into `0xefcdab9078563412`.
 *
 * @param  value   The value to transform
 *
 * @return The byte-swapped value.
 * @sideeffects None.
 */
static inline uint64_t
bf_swap64(const uint64_t value)
{
#if defined(HAVE_COMPILER_BUILTIN_BSWAP64) && defined(UFW_USE_BUILTIN_SWAP)
    return __builtin_bswap64(value);
#else
    return ( ((value & 0xff00000000000000ull) >> 56u)
           | ((value & 0x00ff000000000000ull) >> 40u)
           | ((value & 0x0000ff0000000000ull) >> 24u)
           | ((value & 0x000000ff00000000ull) >>  8u)
           | ((value & 0x00000000ff000000ull) <<  8u)
           | ((value & 0x0000000000ff0000ull) << 24u)
           | ((value & 0x000000000000ff00ull) << 40u)
           | ((value & 0x00000000000000ffull) << 56u));
#endif /* HAVE_COMPILER_BUILTIN_BSWAP64 */
}

/**
 * Read uint16_t datum from buffer in native octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A uint16_t datum read from memory pointed to by ptr in
 *         native octet order.
 * @sideeffects None.
 */
static inline uint16_t
bf_ref_u16n(const void *ptr)
{
    uint16_t buffer = 0u;
    const unsigned char *src = ptr;
    unsigned char *dst = (unsigned char*)&buffer;
#if UFW_BITS_PER_BYTE == 8
    dst[0u] = src[0u];
    dst[1u] = src[1u];
#endif /* UFW_BITS_PER_BYTE == 8 */
#if UFW_BITS_PER_BYTE == 16
    dst[0u] = src[0u];
#endif /* UFW_BITS_PER_BYTE == 16 */
    return buffer;
}

/**
 * Read uint32_t datum from buffer in native octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A uint32_t datum read from memory pointed to by ptr in
 *         native octet order.
 * @sideeffects None.
 */
static inline uint32_t
bf_ref_u32n(const void *ptr)
{
    uint32_t buffer = 0ul;
    const unsigned char *src = ptr;
    unsigned char *dst = (unsigned char*)&buffer;
#if UFW_BITS_PER_BYTE == 8
    dst[0u] = src[0u];
    dst[1u] = src[1u];
    dst[2u] = src[2u];
    dst[3u] = src[3u];
#endif /* UFW_BITS_PER_BYTE == 8 */
#if UFW_BITS_PER_BYTE == 16
    dst[0u] = src[0u];
    dst[1u] = src[1u];
#endif /* UFW_BITS_PER_BYTE == 16 */
    return buffer;
}

/**
 * Read uint64_t datum from buffer in native octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A uint64_t datum read from memory pointed to by ptr in
 *         native octet order.
 * @sideeffects None.
 */
static inline uint64_t
bf_ref_u64n(const void *ptr)
{
    uint64_t buffer = 0ull;
    const unsigned char *src = ptr;
    unsigned char *dst = (unsigned char*)&buffer;
#if UFW_BITS_PER_BYTE == 8
    dst[0u] = src[0u];
    dst[1u] = src[1u];
    dst[2u] = src[2u];
    dst[3u] = src[3u];
    dst[4u] = src[4u];
    dst[5u] = src[5u];
    dst[6u] = src[6u];
    dst[7u] = src[7u];
#endif /* UFW_BITS_PER_BYTE == 8 */
#if UFW_BITS_PER_BYTE == 16
    dst[0u] = src[0u];
    dst[1u] = src[1u];
    dst[2u] = src[2u];
    dst[3u] = src[3u];
#endif /* UFW_BITS_PER_BYTE == 16 */
    return buffer;
}

/**
 * Read uint16_t datum from buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A uint16_t datum read from memory pointed to by ptr in
 *         big endian octet order.
 * @sideeffects None.
 */
static inline uint16_t
bf_ref_u16b(const void *ptr)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_ref_u16n(ptr);
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_swap16(bf_ref_u16n(ptr));
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Read uint32_t datum from buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A uint32_t datum read from memory pointed to by ptr in
 *         big endian octet order.
 * @sideeffects None.
 */
static inline uint32_t
bf_ref_u32b(const void *ptr)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_ref_u32n(ptr);
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_swap32(bf_ref_u32n(ptr));
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Read uint64_t datum from buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A uint64_t datum read from memory pointed to by ptr in
 *         big endian octet order.
 * @sideeffects None.
 */
static inline uint64_t
bf_ref_u64b(const void *ptr)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_ref_u64n(ptr);
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_swap64(bf_ref_u64n(ptr));
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Read uint16_t datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A uint16_t datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline uint16_t
bf_ref_u16l(const void *ptr)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_swap16(bf_ref_u16n(ptr));
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_ref_u16n(ptr);
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Read uint32_t datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A uint32_t datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline uint32_t
bf_ref_u32l(const void *ptr)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_swap32(bf_ref_u32n(ptr));
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_ref_u32n(ptr);
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Read uint64_t datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A uint64_t datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline uint64_t
bf_ref_u64l(const void *ptr)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_swap64(bf_ref_u64n(ptr));
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_ref_u64n(ptr);
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Read int16_t datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A int16_t datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline int16_t
bf_ref_s16n(const void *ptr)
{
    const union bf_convert16 data = { .u16 = bf_ref_u16n(ptr) };
    return data.s16;
}

/**
 * Read int32_t datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A int32_t datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline int32_t
bf_ref_s32n(const void *ptr)
{
    const union bf_convert32 data = { .u32 = bf_ref_u32n(ptr) };
    return data.s32;
}

/**
 * Read int64_t datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A int64_t datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline int64_t
bf_ref_s64n(const void *ptr)
{
    const union bf_convert64 data = { .u64 = bf_ref_u64n(ptr) };
    return data.s64;
}

/**
 * Read int16_t datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A int16_t datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline int16_t
bf_ref_s16l(const void *ptr)
{
    const union bf_convert16 data = { .u16 = bf_ref_u16l(ptr) };
    return data.s16;
}

/**
 * Read int32_t datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A int32_t datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline int32_t
bf_ref_s32l(const void *ptr)
{
    const union bf_convert32 data = { .u32 = bf_ref_u32l(ptr) };
    return data.s32;
}

/**
 * Read int64_t datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A int64_t datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline int64_t
bf_ref_s64l(const void *ptr)
{
    const union bf_convert64 data = { .u64 = bf_ref_u64l(ptr) };
    return data.s64;
}

/**
 * Read int16_t datum from buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A int16_t datum read from memory pointed to by ptr in
 *         big endian octet order.
 * @sideeffects None.
 */
static inline int16_t
bf_ref_s16b(const void *ptr)
{
    const union bf_convert16 data = { .u16 = bf_ref_u16b(ptr) };
    return data.s16;
}

/**
 * Read int32_t datum from buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A int32_t datum read from memory pointed to by ptr in
 *         big endian octet order.
 * @sideeffects None.
 */
static inline int32_t
bf_ref_s32b(const void *ptr)
{
    const union bf_convert32 data = { .u32 = bf_ref_u32b(ptr) };
    return data.s32;
}

/**
 * Read int64_t datum from buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A int64_t datum read from memory pointed to by ptr in
 *         big endian octet order.
 * @sideeffects None.
 */
static inline int64_t
bf_ref_s64b(const void *ptr)
{
    const union bf_convert64 data = { .u64 = bf_ref_u64b(ptr) };
    return data.s64;
}

/**
 * Read float datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A float datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline float
bf_ref_f32n(const void *ptr)
{
    const union bf_convert32 data = { .u32 = bf_ref_u32n(ptr) };
    return data.f32;
}

/**
 * Read float datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A float datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline float
bf_ref_f32l(const void *ptr)
{
    const union bf_convert32 data = { .u32 = bf_ref_u32l(ptr) };
    return data.f32;
}

/**
 * Read float datum from buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A float datum read from memory pointed to by ptr in
 *         big endian octet order.
 * @sideeffects None.
 */
static inline float
bf_ref_f32b(const void *ptr)
{
    const union bf_convert32 data = { .u32 = bf_ref_u32b(ptr) };
    return data.f32;
}

/**
 * Read double datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A double datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline double
bf_ref_f64n(const void *ptr)
{
    const union bf_convert64 data = { .u64 = bf_ref_u64n(ptr) };
    return data.f64;
}

/**
 * Read double datum from buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A double datum read from memory pointed to by ptr in
 *         little endian octet order.
 * @sideeffects None.
 */
static inline double
bf_ref_f64l(const void *ptr)
{
    const union bf_convert64 data = { .u64 = bf_ref_u64l(ptr) };
    return data.f64;
}

/**
 * Read double datum from buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A double datum read from memory pointed to by ptr in
 *         big endian octet order.
 * @sideeffects None.
 */
static inline double
bf_ref_f64b(const void *ptr)
{
    const union bf_convert64 data = { .u64 = bf_ref_u64b(ptr) };
    return data.f64;
}

/**
 * Store uint16_t datum into a buffer in native octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_u16n(void *ptr, const uint16_t value)
{
    const unsigned char *src = (const unsigned char*)&value;
    unsigned char *dst = ptr;
#if UFW_BITS_PER_BYTE == 8
    dst[0u] = src[0u];
    dst[1u] = src[1u];
#endif /* UFW_BITS_PER_BYTE == 8 */
#if UFW_BITS_PER_BYTE == 16
    dst[0u] = src[0u];
#endif /* UFW_BITS_PER_BYTE == 16 */
    return dst + sizeof(value);
}

/**
 * Store uint32_t datum into a buffer in native octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_u32n(void *ptr, const uint32_t value)
{
    const unsigned char *src = (const unsigned char*)&value;
    unsigned char *dst = ptr;
#if UFW_BITS_PER_BYTE == 8
    dst[0u] = src[0u];
    dst[1u] = src[1u];
    dst[2u] = src[2u];
    dst[3u] = src[3u];
#endif /* UFW_BITS_PER_BYTE == 8 */
#if UFW_BITS_PER_BYTE == 16
    dst[0u] = src[0u];
    dst[1u] = src[1u];
#endif /* UFW_BITS_PER_BYTE == 16 */
    return dst + sizeof(value);
}

/**
 * Store uint64_t datum into a buffer in native octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_u64n(void *ptr, const uint64_t value)
{
    const unsigned char *src = (const unsigned char*)&value;
    unsigned char *dst = ptr;
#if UFW_BITS_PER_BYTE == 8
    dst[0u] = src[0u];
    dst[1u] = src[1u];
    dst[2u] = src[2u];
    dst[3u] = src[3u];
    dst[4u] = src[4u];
    dst[5u] = src[5u];
    dst[6u] = src[6u];
    dst[7u] = src[7u];
#endif /* UFW_BITS_PER_BYTE == 8 */
#if UFW_BITS_PER_BYTE == 16
    dst[0u] = src[0u];
    dst[1u] = src[1u];
    dst[2u] = src[2u];
    dst[3u] = src[3u];
#endif /* UFW_BITS_PER_BYTE == 16 */
    return dst + sizeof(value);
}

/**
 * Store uint16_t datum into a buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_u16b(void *ptr, const uint16_t value)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_set_u16n(ptr, value);
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_set_u16n(ptr, bf_swap16(value));
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Store uint32_t datum into a buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_u32b(void *ptr, const uint32_t value)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_set_u32n(ptr, value);
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_set_u32n(ptr, bf_swap32(value));
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Store uint64_t datum into a buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_u64b(void *ptr, const uint64_t value)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_set_u64n(ptr, value);
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_set_u64n(ptr, bf_swap64(value));
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Store uint16_t datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_u16l(void *ptr, const uint16_t value)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_set_u16n(ptr, bf_swap16(value));
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_set_u16n(ptr, value);
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Store uint32_t datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_u32l(void *ptr, const uint32_t value)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_set_u32n(ptr, bf_swap32(value));
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_set_u32n(ptr, value);
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Store uint64_t datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_u64l(void *ptr, const uint64_t value)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_set_u64n(ptr, bf_swap64(value));
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_set_u64n(ptr, value);
#else
    /* Top of file names sure this can't happen. */
#endif /* SYSTEM_ENDIANNESS_* */
}

/**
 * Store int16_t datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_s16n(void *ptr, const int16_t value)
{
    const union bf_convert16 data = { .s16 = value };
    return bf_set_u16n(ptr, data.u16);
}

/**
 * Store int32_t datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_s32n(void *ptr, const int32_t value)
{
    const union bf_convert32 data = { .s32 = value };
    return bf_set_u32n(ptr, data.u32);
}

/**
 * Store int64_t datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_s64n(void *ptr, const int64_t value)
{
    const union bf_convert64 data = { .s64 = value };
    return bf_set_u64n(ptr, data.u64);
}

/**
 * Store int16_t datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_s16l(void *ptr, const int16_t value)
{
    const union bf_convert16 data = { .s16 = value };
    return bf_set_u16l(ptr, data.u16);
}

/**
 * Store int32_t datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_s32l(void *ptr, const int32_t value)
{
    const union bf_convert32 data = { .s32 = value };
    return bf_set_u32l(ptr, data.u32);
}

/**
 * Store int64_t datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_s64l(void *ptr, const int64_t value)
{
    const union bf_convert64 data = { .s64 = value };
    return bf_set_u64l(ptr, data.u64);
}

/**
 * Store int16_t datum into a buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_s16b(void *ptr, const int16_t value)
{
    const union bf_convert16 data = { .s16 = value };
    return bf_set_u16b(ptr, data.u16);
}

/**
 * Store int32_t datum into a buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_s32b(void *ptr, const int32_t value)
{
    const union bf_convert32 data = { .s32 = value };
    return bf_set_u32b(ptr, data.u32);
}

/**
 * Store int64_t datum into a buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_s64b(void *ptr, const int64_t value)
{
    const union bf_convert64 data = { .s64 = value };
    return bf_set_u64b(ptr, data.u64);
}

/**
 * Store float datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_f32n(void *ptr, const float value)
{
    const union bf_convert32 data = { .f32 = value };
    return bf_set_u32n(ptr, data.u32);
}

/**
 * Store float datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_f32l(void *ptr, const float value)
{
    const union bf_convert32 data = { .f32 = value };
    return bf_set_u32l(ptr, data.u32);
}

/**
 * Store float datum into a buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_f32b(void *ptr, const float value)
{
    const union bf_convert32 data = { .f32 = value };
    return bf_set_u32b(ptr, data.u32);
}

/**
 * Store double datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_f64n(void *ptr, const double value)
{
    const union bf_convert64 data = { .f64 = value };
    return bf_set_u64n(ptr, data.u64);
}

/**
 * Store double datum into a buffer in little endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_f64l(void *ptr, const double value)
{
    const union bf_convert64 data = { .f64 = value };
    return bf_set_u64l(ptr, data.u64);
}

/**
 * Store double datum into a buffer in big endian octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
static inline void*
bf_set_f64b(void *ptr, const double value)
{
    const union bf_convert64 data = { .f64 = value };
    return bf_set_u64b(ptr, data.u64);
}

#endif /* INC_UFW_BINARY_FORMAT_H */
