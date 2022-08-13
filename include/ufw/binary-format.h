/*
 * Copyright (c) 2019-2021 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
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
 */

#ifndef INC_UFW_BINARY_FORMAT_H
#define INC_UFW_BINARY_FORMAT_H

#include <stdint.h>

#ifdef __cplusplus
#ifndef CXX_ALLOW_TYPE_PUNNING
#warning "binary-format uses type punning, which is undefined behaviour in C++!"
#warning "Your toolchain may allow it as an extension, but be advised!"
#warning "To disable this warning, define the CXX_ALLOW_TYPE_PUNNING macro."
#endif /* CXX_ALLOW_TYPE_PUNNING */
#endif /* __cplusplus */

/* Use toolchain information to decide whether or not __builtin_bswapXX() are
 * available. If not, use the usual mask-shift-or routine. */
#include <ufw/toolchain.h>

#if !(defined(WITH_UINT8_T))
#if !(defined(SYSTEM_ENDIANNESS_BIG)) && !(defined(SYSTEM_ENDIANNESS_LITTLE))

#error "System is not octet-addressable and octet-order is not indicated!"
#error "Cannot use binary-format.h for that reason!"

#endif /* !(defined(SYSTEM_ENDIANNESS_*)) */

#define bf_type uint_least8_t

#else

#define bf_type uint8_t

#endif /* !(defined(WITH_UINT8_T)) */

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
bf_swap16(uint16_t value)
{
#ifdef HAVE_COMPILER_BUILTIN_BSWAP16
    return __builtin_bswap16(value);
#else
    return (((value >> 8u) & 0xffu) | ((value & 0xffu) << 8u));
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
bf_swap32(uint32_t value)
{
#ifdef HAVE_COMPILER_BUILTIN_BSWAP32
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
 * Turn `0x1122334455667788` into `0x8877665544332211`.
 *
 * @param  value   The value to transform
 *
 * @return The byte-swapped value.
 * @sideeffects None.
 */
static inline uint64_t
bf_swap64(uint64_t value)
{
#ifdef HAVE_COMPILER_BUILTIN_BSWAP64
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

static inline uint16_t
bf_ref_u16n(const bf_type *buf)
{
    return *(uint16_t *)buf;
}

static inline uint32_t
bf_ref_u32n(const bf_type *buf)
{
    return *(uint32_t *)buf;
}

static inline uint64_t
bf_ref_u64n(const bf_type *buf)
{
    return *(uint64_t *)buf;
}

static inline int16_t
bf_ref_s16n(const bf_type *buf)
{
    return *(int16_t *)buf;
}

static inline int32_t
bf_ref_s32n(const bf_type *buf)
{
    return *(int32_t *)buf;
}

static inline int64_t
bf_ref_s64n(const bf_type *buf)
{
    return *(int64_t *)buf;
}

static inline uint16_t
bf_ref_u16b(const bf_type *buf)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_ref_u16n(buf);
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_swap16(bf_ref_u16n(buf));
#else
    return ( (((uint_least16_t)buf[0]) << 8u)
           |  ((uint_least16_t)buf[1]));
#endif /* SYSTEM_ENDIANNESS_* */
}

static inline uint32_t
bf_ref_u32b(const bf_type *buf)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_ref_u32n(buf);
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_swap32(bf_ref_u32n(buf));
#else
    return ( (((uint_least32_t)buf[0]) << 24u)
           | (((uint_least32_t)buf[1]) << 16u)
           | (((uint_least32_t)buf[2]) <<  8u)
           |  ((uint_least32_t)buf[3]));
#endif /* SYSTEM_ENDIANNESS_* */
}

static inline uint64_t
bf_ref_u64b(const bf_type *buf)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_ref_u64n(buf);
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_swap64(bf_ref_u64n(buf));
#else
    return ( (((uint_least64_t)buf[0]) << 56u)
           | (((uint_least64_t)buf[1]) << 48u)
           | (((uint_least64_t)buf[2]) << 40u)
           | (((uint_least64_t)buf[3]) << 32u)
           | (((uint_least64_t)buf[4]) << 24u)
           | (((uint_least64_t)buf[5]) << 16u)
           | (((uint_least64_t)buf[6]) <<  8u)
           |  ((uint_least64_t)buf[7]));
#endif /* SYSTEM_ENDIANNESS_* */
}

static inline int16_t
bf_ref_s16b(const bf_type *buf)
{
    return (int16_t)bf_ref_u16b(buf);
}

static inline int32_t
bf_ref_s32b(const bf_type *buf)
{
    return (int32_t)bf_ref_u32b(buf);
}

static inline int64_t
bf_ref_s64b(const bf_type *buf)
{
    return (int64_t)bf_ref_u64b(buf);
}

static inline uint16_t
bf_ref_u16l(const bf_type *buf)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_swap16(bf_ref_u16n(buf));
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_ref_u16n(buf);
#else
    return ( (((uint_least16_t)buf[1]) << 8u)
           |  ((uint_least16_t)buf[0]));
#endif /* SYSTEM_ENDIANNESS_* */
}

static inline uint32_t
bf_ref_u32l(const bf_type *buf)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_swap32(bf_ref_u32n(buf));
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_ref_u32n(buf);
#else
    return ( (((uint_least32_t)buf[3]) << 24u)
           | (((uint_least32_t)buf[2]) << 16u)
           | (((uint_least32_t)buf[1]) <<  8u)
           |  ((uint_least32_t)buf[0]));
#endif /* SYSTEM_ENDIANNESS_* */
}

static inline uint64_t
bf_ref_u64l(const bf_type *buf)
{
#if defined(SYSTEM_ENDIANNESS_BIG)
    return bf_swap64(bf_ref_u64n(buf));
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    return bf_ref_u64n(buf);
#else
    return ( (((uint_least64_t)buf[7]) << 56u)
           | (((uint_least64_t)buf[6]) << 48u)
           | (((uint_least64_t)buf[5]) << 40u)
           | (((uint_least64_t)buf[4]) << 32u)
           | (((uint_least64_t)buf[3]) << 24u)
           | (((uint_least64_t)buf[2]) << 16u)
           | (((uint_least64_t)buf[1]) <<  8u)
           |  ((uint_least64_t)buf[0]));
#endif /* SYSTEM_ENDIANNESS_* */
}

static inline int16_t
bf_ref_s16l(const bf_type *buf)
{
    return (int16_t)bf_ref_u16l(buf);
}

static inline int32_t
bf_ref_s32l(const bf_type *buf)
{
    return (int32_t)bf_ref_u32l(buf);
}

static inline int64_t
bf_ref_s64l(const bf_type *buf)
{
    return (int64_t)bf_ref_u64l(buf);
}

union bf_ieee754_f32 {
    float f32;
    uint32_t data;
};

union bf_ieee754_f64 {
    double f64;
    uint64_t data;
};

static inline float
bf_ref_f32n(const bf_type *buf)
{
    const union bf_ieee754_f32 map = { .data = bf_ref_u32n(buf) };
    return map.f32;
}

static inline double
bf_ref_f64n(const bf_type *buf)
{
    const union bf_ieee754_f64 map = { .data = bf_ref_u64n(buf) };
    return map.f64;
}

static inline float
bf_ref_f32b(const bf_type *buf)
{
    const union bf_ieee754_f32 map = { .data = bf_ref_u32b(buf) };
    return map.f32;
}

static inline double
bf_ref_f64b(const bf_type *buf)
{
    const union bf_ieee754_f64 map = { .data = bf_ref_u64b(buf) };
    return map.f64;
}

static inline float
bf_ref_f32l(const bf_type *buf)
{
    const union bf_ieee754_f32 map = { .data = bf_ref_u32l(buf) };
    return map.f32;
}

static inline double
bf_ref_f64l(const bf_type *buf)
{
    const union bf_ieee754_f64 map = { .data = bf_ref_u64l(buf) };
    return map.f64;
}

static inline uint16_t*
bf_set_u16n(bf_type *buf, uint16_t value)
{
    *(uint16_t *)buf = value;
    return (uint16_t *)buf;
}

static inline uint32_t*
bf_set_u32n(bf_type *buf, uint32_t value)
{
    *(uint32_t *)buf = value;
    return (uint32_t *)buf;
}

static inline uint64_t*
bf_set_u64n(bf_type *buf, uint64_t value)
{
    *(uint64_t *)buf = value;
    return (uint64_t *)buf;
}

static inline float*
bf_set_f32n(bf_type *buf, float value)
{
    *(float *)buf = value;
    return (float *)buf;
}

static inline double*
bf_set_f64n(bf_type *buf, double value)
{
    *(double *)buf = value;
    return (double *)buf;
}

static inline uint16_t*
bf_set_u16b(bf_type *buf, uint16_t value)
{
    *(uint16_t *)buf = bf_ref_u16b((const bf_type*)&value);
    return (uint16_t *)buf;
}

static inline uint32_t*
bf_set_u32b(bf_type *buf, uint32_t value)
{
    *(uint32_t *)buf = bf_ref_u32b((const bf_type*)&value);
    return (uint32_t *)buf;
}

static inline uint64_t*
bf_set_u64b(bf_type *buf, uint64_t value)
{
    *(uint64_t *)buf = bf_ref_u64b((const bf_type*)&value);
    return (uint64_t *)buf;
}

static inline uint16_t*
bf_set_u16l(bf_type *buf, uint16_t value)
{
    *(uint16_t *)buf = bf_ref_u16l((const bf_type*)&value);
    return (uint16_t *)buf;
}

static inline uint32_t*
bf_set_u32l(bf_type *buf, uint32_t value)
{
    *(uint32_t *)buf = bf_ref_u32l((const bf_type*)&value);
    return (uint32_t *)buf;
}

static inline uint64_t*
bf_set_u64l(bf_type *buf, uint64_t value)
{
    *(uint64_t *)buf = bf_ref_u64l((const bf_type*)&value);
    return (uint64_t *)buf;
}

static inline int16_t*
bf_set_s16n(bf_type *buf, int16_t value)
{
    *(int16_t *)buf = value;
    return (int16_t *)buf;
}

static inline int32_t*
bf_set_s32n(bf_type *buf, int32_t value)
{
    *(int32_t *)buf = value;
    return (int32_t *)buf;
}

static inline int64_t*
bf_set_s64n(bf_type *buf, int64_t value)
{
    *(int64_t *)buf = value;
    return (int64_t *)buf;
}

static inline int16_t*
bf_set_s16b(bf_type *buf, int16_t value)
{
    *(int16_t *)buf = bf_ref_s16b((const bf_type*)&value);
    return (int16_t *)buf;
}

static inline int32_t*
bf_set_s32b(bf_type *buf, int32_t value)
{
    *(int32_t *)buf = bf_ref_s32b((const bf_type*)&value);
    return (int32_t *)buf;
}

static inline int64_t*
bf_set_s64b(bf_type *buf, int64_t value)
{
    *(int64_t *)buf = bf_ref_s64b((const bf_type*)&value);
    return (int64_t *)buf;
}

static inline float*
bf_set_f32b(bf_type *buf, float value)
{
    *(float *)buf = bf_ref_f32b((const bf_type*)&value);
    return (float *)buf;
}

static inline double*
bf_set_f64b(bf_type *buf, double value)
{
    *(double *)buf = bf_ref_f64b((const bf_type*)&value);
    return (double *)buf;
}

static inline int16_t*
bf_set_s16l(bf_type *buf, int16_t value)
{
    *(int16_t *)buf = bf_ref_s16l((const bf_type*)&value);
    return (int16_t *)buf;
}

static inline int32_t*
bf_set_s32l(bf_type *buf, int32_t value)
{
    *(int32_t *)buf = bf_ref_s32l((const bf_type*)&value);
    return (int32_t *)buf;
}

static inline int64_t*
bf_set_s64l(bf_type *buf, int64_t value)
{
    *(int64_t *)buf = bf_ref_s64l((const bf_type*)&value);
    return (int64_t *)buf;
}

static inline float*
bf_set_f32l(bf_type *buf, float value)
{
    *(float *)buf = bf_ref_f32l((const bf_type*)&value);
    return (float *)buf;
}

static inline double*
bf_set_f64l(bf_type *buf, double value)
{
    *(double *)buf = bf_ref_f64l((const bf_type*)&value);
    return (double *)buf;
}

/* Remove temporary macro to not pollute namespace */
#undef bf_type

#endif /* INC_UFW_BINARY_FORMAT_H */
