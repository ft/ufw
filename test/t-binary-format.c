/*
 * Copyright (c) 2022-2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/* Compatibility note: This test bundle is only compiled if CHAR_BITS == 8. */

#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <ufw/compiler.h>
#include <ufw/test/tap.h>

#include <ufw/binary-format.h>

/*
 * Octet swapping
 */

static void
t_swap16(void)
{
    const uint16_t value = 0x1234;
    const uint16_t swapped = bf_swap16(value);
    const uint16_t reswapped = bf_swap16(swapped);

    ok(swapped == 0x3412u,
       "bf_swap16() swaps correctly");
    ok(value == reswapped,
       "bf_swap16() swapping twice results in starting value");
}

static void
t_swap32(void)
{
    const uint32_t value = 0x12345678;
    const uint32_t swapped = bf_swap32(value);
    const uint32_t reswapped = bf_swap32(swapped);

    ok(swapped == 0x78563412ul,
       "bf_swap32() swaps correctly");
    ok(value == reswapped,
       "bf_swap32() swapping twice results in starting value");
}

static void
t_swap64(void)
{
    const uint64_t value = 0x1122334455667788;
    const uint64_t swapped = bf_swap64(value);
    const uint64_t reswapped = bf_swap64(swapped);

    ok(swapped == 0x8877665544332211ull,
       "bf_swap64() swaps correctly");
    ok(value == reswapped,
       "bf_swap64() swapping twice results in starting value");
}

static void
t_swap24(void)
{
    const uint32_t value = 0x00345678;
    const uint32_t swapped = bf_swap24(value);
    const uint32_t reswapped = bf_swap24(swapped);

    ok(swapped == 0x00785634ul,
       "bf_swap24() swaps correctly");
    ok(value == reswapped,
       "bf_swap24() swapping twice results in starting value");
}

static void
t_swap40(void)
{
    const uint64_t value = 0x0000004455667788;
    const uint64_t swapped = bf_swap40(value);
    const uint64_t reswapped = bf_swap40(swapped);

    ok(swapped == 0x0000008877665544ull,
       "bf_swap40() swaps correctly");
    ok(value == reswapped,
       "bf_swap40() swapping twice results in starting value");
}

static void
t_swap48(void)
{
    const uint64_t value = 0x0000334455667788;
    const uint64_t swapped = bf_swap48(value);
    const uint64_t reswapped = bf_swap48(swapped);

    ok(swapped == 0x0000887766554433ull,
       "bf_swap48() swaps correctly");
    ok(value == reswapped,
       "bf_swap48() swapping twice results in starting value");
}

static void
t_swap56(void)
{
    const uint64_t value = 0x0022334455667788;
    const uint64_t swapped = bf_swap56(value);
    const uint64_t reswapped = bf_swap56(swapped);

    ok(swapped == 0x0088776655443322ull,
       "bf_swap56() swaps correctly");
    ok(value == reswapped,
       "bf_swap56() swapping twice results in starting value");
}

static void
t_swap(void)
{
    t_swap16(); /* 2 */
    t_swap32(); /* 2 */
    t_swap64(); /* 2 */
    t_swap24(); /* 2 */
    t_swap40(); /* 2 */
    t_swap48(); /* 2 */
    t_swap56(); /* 2 */
}

/*
 * Native octet-order tests
 */

/* Referencing */

static void
t_native_ref_unsigned(void)
{
    const uint16_t u16 = 0x1234u;
    const uint32_t u32 = 0x12345678ul;
    const uint64_t u64 = 0x1122334455667788ull;

    ok(u16 == bf_ref_u16n(&u16),
       "Referencing u16 in native order works");
    ok(u32 == bf_ref_u32n(&u32),
       "Referencing u32 in native order works");
    ok(u64 == bf_ref_u64n(&u64),
       "Referencing u64 in native order works");
#if defined(SYSTEM_ENDIANNESS_BIG)
    ok(0x123456ul == bf_ref_u24n(&u32),
       "Referencing u24 in native order works");
    ok(0x1122334455ull == bf_ref_u40n(&u64),
       "Referencing u40 in native order works");
    ok(0x112233445566ull == bf_ref_u48n(&u64),
       "Referencing u48 in native order works");
    ok(0x11223344556677ull == bf_ref_u56n(&u64),
       "Referencing u56 in native order works");
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    /* On little-endian machines, the byte-wise memory of the u32 will look
     * like this: 78 56 34 12; interpreting the first 24 bits of this on a
     * little endian machine is 0x345678. */
    ok(0x345678ul == bf_ref_u24n(&u32),
       "Referencing u24 in native order works");
    ok(0x4455667788ull == bf_ref_u40n(&u64),
       "Referencing u40 in native order works");
    ok(0x334455667788ull == bf_ref_u48n(&u64),
       "Referencing u48 in native order works");
    ok(0x22334455667788ull == bf_ref_u56n(&u64),
       "Referencing u56 in native order works");
#endif /* ENDIANNESS */
}

static void
t_native_ref_signed(void)
{
    /* In comments, we note the encoding in two's complement. We will need
     * these values to determine the correct value in the partial getters. */
    const int16_t s16 = -0x1234;               /* 0xedcc */
    const int32_t s32 = -0x12345678l;          /* 0xedcba988 */
    const int64_t s64 = -0x1122334455667788ll; /* 0xeeddccbbaa998878 */

    ok(s16 == bf_ref_s16n(&s16),
       "Referencing s16 in native order works");
    ok(s32 == bf_ref_s32n(&s32),
       "Referencing s32 in native order works");
    ok(s64 == bf_ref_s64n(&s64),
       "Referencing s64 in native order works");
#if defined(SYSTEM_ENDIANNESS_BIG)
    /* On big-endian systems, the memory looks like this: ed cb a9 88. The
     * first 24 bits are 0xedcba9; this encodes -1193047. */
    ok(-1193047l == bf_ref_s24n(&s32),
       "Referencing s24 in native order works");
    /* 0xeeddccbbaa => -73588229206 */
    ok(-73588229206ll == bf_ref_s40n(&s64),
       "Referencing s40 in native order works");
    /* 0xeeddccbbaa99 => -18838586676583 */
    ok(-18838586676583ll == bf_ref_s48n(&s64),
       "Referencing s48 in native order works");
    /* 0xeeddccbbaa9988 => -4822678189205112 */
    ok(-4822678189205112ll == bf_ref_s56n(&s64),
       "Referencing s56 in native order works");
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    /* On little-endian machines, the memory looks like this: 88 a9 cb ed. The
     * first 24 bits 0xcba988; this encodes: -3430008. */
    ok(-3430008l == bf_ref_s24n(&s32),
       "Referencing s24 in native order works");
    /* 0xbbaa998878 => -293490554760 */
    ok(-293490554760ll == bf_ref_s40n(&s64),
       "Referencing s40 in native order works");
    /* 0xccbbaa998878 => -56368583571336 */
    ok(-56368583571336ll == bf_ref_s48n(&s64),
       "Referencing s48 in native order works");
    /* 0xddccbbaa998878 => -9626517791733640 */
    ok(-9626517791733640ll == bf_ref_s56n(&s64),
       "Referencing s56 in native order works");
#endif /* ENDIANNESS */
}

static void
t_native_ref_float(void)
{
    const float f32 = 1.f / 123.3e12f;
    const double f64 = 1. / 123.3e12;

    ok(f32 == bf_ref_f32n(&f32),
       "Referencing f32 in native order works");
    ok(f64 == bf_ref_f64n(&f64),
       "Referencing f64 in native order works");
}

static void
t_native_ref(void)
{
    t_native_ref_unsigned(); /* 7 */
    t_native_ref_signed();   /* 7 */
    t_native_ref_float();    /* 2 */
}

/* Mutation */

static void
t_native_set_u16(void)
{
    const uint16_t u16 = 0x1234u;
    uint16_t m16[] = { 0 };
    void *p16;

    p16 = bf_set_u16n(m16, u16);
    ok(u16 == *m16, "Setting uint16_t memory in native order works");
    ok(p16 == m16 + 1, "Setting uint16_t memory returns correct memory");
}

static void
t_native_set_u24(void)
{
    const uint32_t u24 = 0x345678ul;
    uint8_t m24[3] = { 0u, 0u, 0u };
#if defined(SYSTEM_ENDIANNESS_BIG)
    uint8_t x24[3] = { 0x34u, 0x56u, 0x78u };
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    uint8_t x24[3] = { 0x78u, 0x56u, 0x34u };
#endif /* ENDIANNESS */
    void *p24;

    p24 = bf_set_u24n(m24, u24);
    cmp_mem(m24, x24, 3, "Setting u24 memory in native order works");
    ok(p24 == m24 + 3, "Setting u24 memory returns correct memory");
}

static void
t_native_set_u32(void)
{
    const uint32_t u32 = 0x12345678ul;
    uint32_t m32[] = { 0 };
    void *p32;

    p32 = bf_set_u32n(m32, u32);
    ok(u32 == *m32, "Setting uint32_t memory in native order works");
    ok(p32 == m32 + 1, "Setting uint32_t memory returns correct memory");
}

static void
t_native_set_u40(void)
{
    const uint64_t u40 = 0xffeeddccbbull;
    uint8_t m40[5] = { 0u, 0u, 0u, 0u, 0u };
#if defined(SYSTEM_ENDIANNESS_BIG)
    uint8_t x40[5] = { 0xffu, 0xeeu, 0xddu, 0xccu, 0xbbu };
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    uint8_t x40[5] = { 0xbbu, 0xccu, 0xddu, 0xeeu, 0xffu };
#endif /* ENDIANNESS */
    void *p40;

    p40 = bf_set_u40n(m40, u40);
    cmp_mem(m40, x40, 5, "Setting u40 memory in native order works");
    ok(p40 == m40 + 5, "Setting u40 memory returns correct memory");
}

static void
t_native_set_u48(void)
{
    const uint64_t u48 = 0xffeeddccbbaaull;
    uint8_t m48[6] = { 0u, 0u, 0u, 0u, 0u, 0u };
#if defined(SYSTEM_ENDIANNESS_BIG)
    uint8_t x48[6] = { 0xffu, 0xeeu, 0xddu, 0xccu, 0xbbu, 0xaau };
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    uint8_t x48[6] = { 0xaau, 0xbbu, 0xccu, 0xddu, 0xeeu, 0xffu };
#endif /* ENDIANNESS */
    void *p48;

    p48 = bf_set_u48n(m48, u48);
    cmp_mem(m48, x48, 6, "Setting u48 memory in native order works");
    ok(p48 == m48 + 6, "Setting u48 memory returns correct memory");
}

static void
t_native_set_u56(void)
{
    const uint64_t u56 = 0xffeeddccbbaa99ull;
    uint8_t m56[7] = { 0u, 0u, 0u, 0u, 0u, 0u };
#if defined(SYSTEM_ENDIANNESS_BIG)
    uint8_t x56[7] = { 0xffu, 0xeeu, 0xddu, 0xccu, 0xbbu, 0xaau, 0x99u };
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    uint8_t x56[7] = { 0x99u, 0xaau, 0xbbu, 0xccu, 0xddu, 0xeeu, 0xffu };
#endif /* ENDIANNESS */
    void *p56;

    p56 = bf_set_u56n(m56, u56);
    cmp_mem(m56, x56, 7, "Setting u56 memory in native order works");
    ok(p56 == m56 + 7, "Setting u56 memory returns correct memory");
}

static void
t_native_set_u64(void)
{
    const uint64_t u64 = 0x1122334455667788ull;
    uint64_t m64[] = { 0 };
    void *p64;

    p64 = bf_set_u64n(m64, u64);
    ok(u64 == *m64, "Setting uint64_t memory in native order works");
    ok(p64 == m64 + 1, "Setting uint64_t memory returns correct memory");
}

static void
t_native_set_unsigned(void)
{
    t_native_set_u16();  /* 2 */
    t_native_set_u24();  /* 2 */
    t_native_set_u32();  /* 2 */
    t_native_set_u40();  /* 2 */
    t_native_set_u48();  /* 2 */
    t_native_set_u56();  /* 2 */
    t_native_set_u64();  /* 2 */
}

static void
t_native_set_s16(void)
{
    const int16_t s16 = INT16_MIN;
    int16_t m16[] = { 0 };
    void *p16;

    p16 = bf_set_s16n(m16, s16);
    ok(s16 == *m16, "Setting int16_t memory in native order works");
    ok(p16 == m16 + 1, "Setting int16_t memory returns correct memory");
}

static void
t_native_set_s24(void)
{
    const union bf_convert32 data = { .u32 = 0xfedcbaul };
    const int32_t s24 = data.s32;
    uint8_t m24[3] = { 0u, 0u, 0u };
#if defined(SYSTEM_ENDIANNESS_BIG)
    uint8_t x24[3] = { 0xfeu, 0xdcu, 0xbau };
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    uint8_t x24[3] = { 0xbau, 0xdcu, 0xfeu };
#endif /* ENDIANNESS */
    void *p24;

    p24 = bf_set_s24n(m24, s24);
    cmp_mem(m24, x24, 3, "Setting s24 memory in native order works");
    ok(p24 == m24 + 3, "Setting s24 memory returns correct memory");
}

static void
t_native_set_s32(void)
{
    const int32_t s32 = INT32_MIN;
    int32_t m32[] = { 0 };
    void *p32;

    p32 = bf_set_s32n(m32, s32);
    ok(s32 == *m32, "Setting int32_t memory in native order works");
    ok(p32 == m32 + 1, "Setting int32_t memory returns correct memory");
}

static void
t_native_set_s40(void)
{
    const union bf_convert64 data = { .u64 = 0xfedcba9876ull };
    const int64_t s40 = data.s64;
    uint8_t m40[5] = { 0u, 0u, 0u, 0u, 0u };
#if defined(SYSTEM_ENDIANNESS_BIG)
    uint8_t x40[5] = { 0xfeu, 0xdcu, 0xbau, 0x98u, 0x76u };
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    uint8_t x40[5] = { 0x76u, 0x98u, 0xbau, 0xdcu, 0xfeu };
#endif /* ENDIANNESS */
    void *p40;

    p40 = bf_set_s40n(m40, s40);
    cmp_mem(m40, x40, 5, "Setting s40 memory in native order works");
    ok(p40 == m40 + 5, "Setting s40 memory returns correct memory");
}

static void
t_native_set_s48(void)
{
    const union bf_convert64 data = { .u64 = 0xfedcba987654ull };
    const int64_t s48 = data.s64;
    uint8_t m48[6] = { 0u, 0u, 0u, 0u, 0u, 0u };
#if defined(SYSTEM_ENDIANNESS_BIG)
    uint8_t x48[6] = { 0xfeu, 0xdcu, 0xbau, 0x98u, 0x76u, 0x54u };
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    uint8_t x48[6] = { 0x54u, 0x76u, 0x98u, 0xbau, 0xdcu, 0xfeu };
#endif /* ENDIANNESS */
    void *p48;

    p48 = bf_set_s48n(m48, s48);
    cmp_mem(m48, x48, 6, "Setting s48 memory in native order works");
    ok(p48 == m48 + 6, "Setting s48 memory returns correct memory");
}

static void
t_native_set_s56(void)
{
    const union bf_convert64 data = { .u64 = 0xfedcba98765432ull };
    const int64_t s56 = data.s64;
    uint8_t m56[7] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
#if defined(SYSTEM_ENDIANNESS_BIG)
    uint8_t x56[7] = { 0xfeu, 0xdcu, 0xbau, 0x98u, 0x76u, 0x54u, 0x32u };
#elif defined(SYSTEM_ENDIANNESS_LITTLE)
    uint8_t x56[7] = { 0x32u, 0x54u, 0x76u, 0x98u, 0xbau, 0xdcu, 0xfeu };
#endif /* ENDIANNESS */
    void *p56;

    p56 = bf_set_s56n(m56, s56);
    cmp_mem(m56, x56, 7, "Setting s56 memory in native order works");
    ok(p56 == m56 + 7, "Setting s56 memory returns correct memory");
}

static void
t_native_set_s64(void)
{
    const int64_t s64 = INT64_MIN;
    int64_t m64[] = { 0 };
    void *p64;

    p64 = bf_set_s64n(m64, s64);
    ok(s64 == *m64, "Setting int64_t memory in native order works");
    ok(p64 == m64 + 1, "Setting int64_t memory returns correct memory");
}

static void
t_native_set_signed(void)
{
    t_native_set_s16();  /* 2 */
    t_native_set_s24();  /* 2 */
    t_native_set_s32();  /* 2 */
    t_native_set_s40();  /* 2 */
    t_native_set_s48();  /* 2 */
    t_native_set_s56();  /* 2 */
    t_native_set_s64();  /* 2 */
}

static void
t_native_set_f32(void)
{
    const float f32 = FLT_MAX;
    float m32[] = { 0.f };
    float *p32;

    p32 = bf_set_f32n(m32, f32);
    ok(f32 == *m32, "Setting float memory in native order works");
    ok(p32 == m32 + 1, "Setting float memory returns correct memory");
}

static void
t_native_set_f64(void)
{
    const double f64 = DBL_MAX;
    double m64[] = { 0. };
    double *p64;

    p64 = bf_set_f64n(m64, f64);
    ok(f64 == *m64, "Setting double memory in native order works");
    ok(p64 == m64 + 1, "Setting double memory returns correct memory");
}

static void
t_native_set_float(void)
{
    t_native_set_f32();  /* 2 */
    t_native_set_f64();  /* 2 */
}

static void
t_native_set(void)
{
    t_native_set_unsigned(); /* 14 */
    t_native_set_signed();   /* 14 */
    t_native_set_float();    /* 4 */
}

/*
 * Big endian octet-order tests
 */

/* Referencing */

static void
t_big_ref_unsigned(void)
{
    const uint16_t u16 = 0x1234u;
    const uint8_t m16[] = { 0x12u, 0x34u };
    const uint32_t u32 = 0x12345678ul;
    const uint32_t u24 = 0x123456ul;
    const uint8_t m32[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    const uint64_t u64 = 0x1122334455667788ull;
    const uint64_t u40 = 0x1122334455ull;
    const uint64_t u48 = 0x112233445566ull;
    const uint64_t u56 = 0x11223344556677ull;
    const uint8_t m64[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                            0x55u, 0x66u, 0x77u, 0x88u };

    ok(u16 == bf_ref_u16b(m16), "Referencing u16 in big endian order works");
    ok(u32 == bf_ref_u32b(m32), "Referencing u32 in big endian order works");
    ok(u64 == bf_ref_u64b(m64), "Referencing u64 in big endian order works");
    ok(u24 == bf_ref_u24b(m32), "Referencing u24 in big endian order works");
    ok(u40 == bf_ref_u40b(m64), "Referencing u40 in big endian order works");
    ok(u48 == bf_ref_u48b(m64), "Referencing u48 in big endian order works");
    ok(u56 == bf_ref_u56b(m64), "Referencing u56 in big endian order works");
}

static void
t_big_ref_signed(void)
{
    const int16_t s16 = 0x1234;
    const uint8_t m16[] = { 0x12u, 0x34u };
    const int32_t s32 = 0x12345678l;
    const uint8_t m32[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    const int64_t s64 = 0x1122334455667788ll;
    const uint8_t m64[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                            0x55u, 0x66u, 0x77u, 0x88u };

    ok(s16 == bf_ref_s16b(m16), "Referencing s16 in big endian order works");
    ok(s32 == bf_ref_s32b(m32), "Referencing s32 in big endian order works");
    ok(s64 == bf_ref_s64b(m64), "Referencing s64 in big endian order works");
    const uint8_t x64[] = { 0xffu, 0xeeu, 0xddu, 0xccu,
                            0xbbu, 0xaau, 0x99u, 0x88u };
    ok(-4387           == bf_ref_s24b(x64),
       "Referencing s24 in big endian order works");
    ok(-287454021      == bf_ref_s40b(x64),
       "Referencing s40 in big endian order works");
    ok(-73588229206    == bf_ref_s48b(x64),
       "Referencing s48 in big endian order works");
    ok(-18838586676583 == bf_ref_s56b(x64),
        "Referencing s56 in big endian order works");
}

static void
t_big_ref_float(void)
{
    const union { float f; uint32_t u; } f32 = { .u = 0x12345678ul };
    const uint8_t m32[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    const union { double f; uint64_t u; } f64 = { .u = 0x1122334455667788ull };
    const uint8_t m64[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                            0x55u, 0x66u, 0x77u, 0x88u };

    ok(f32.f == bf_ref_f32b(m32), "Referencing f32 in big endian order works");
    ok(f64.f == bf_ref_f64b(m64), "Referencing f64 in big endian order works");
}

static void
t_big_ref(void)
{
    t_big_ref_unsigned(); /* 7 */
    t_big_ref_signed();   /* 7 */
    t_big_ref_float();    /* 2 */
}

/* Mutation */

static void
t_big_set_u16(void)
{
    const uint16_t u16 = 0x1234u;
    const uint8_t expect[] = { 0x12u, 0x34u };
    uint16_t m16[] = { 0 };
    uint16_t *p16;

    p16 = bf_set_u16b(m16, u16);
    cmp_mem(m16, expect, sizeof(expect),
            "Setting uint16_t memory in big endian order works");
    ok(p16 == m16 + 1, "Setting uint16_t memory returns correct memory");
}

static void
t_big_set_u24(void)
{
    const uint32_t u24 = 0xffeeddul;
    const uint8_t expect[] = { 0xffu, 0xeeu, 0xddu };
    uint8_t m24[3] = { 0u, 0u, 0u };
    void *p24;

    p24 = bf_set_u24b(m24, u24);
    cmp_mem(m24, expect, sizeof(expect),
            "Setting u24 memory in big endian order works");
    ok(p24 == m24 + 3, "Setting u24 memory returns correct memory");
}

static void
t_big_set_u32(void)
{
    const uint32_t u32 = 0x12345678ul;
    const uint8_t expect[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    uint32_t m32[] = { 0 };
    uint32_t *p32;

    p32 = bf_set_u32b(m32, u32);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting uint32_t memory in big endian order works");
    ok(p32 == m32 + 1, "Setting uint32_t memory returns correct memory");
}

static void
t_big_set_u40(void)
{
    const uint64_t u40 = 0xffeeddccbbull;
    const uint8_t expect[] = { 0xffu, 0xeeu, 0xddu, 0xccu, 0xbbu };
    uint8_t m40[5] = { 0u, 0u, 0u, 0u, 0u };
    void *p40;

    p40 = bf_set_u40b(m40, u40);
    cmp_mem(m40, expect, sizeof(expect),
            "Setting u40 memory in big endian order works");
    ok(p40 == m40 + 5, "Setting u40 memory returns correct memory");
}

static void
t_big_set_u48(void)
{
    const uint64_t u48 = 0xffeeddccbbaaull;
    const uint8_t expect[] = { 0xffu, 0xeeu, 0xddu, 0xccu, 0xbbu, 0xaau };
    uint8_t m48[6] = { 0u, 0u, 0u, 0u, 0u, 0u };
    void *p48;

    p48 = bf_set_u48b(m48, u48);
    cmp_mem(m48, expect, sizeof(expect),
            "Setting u48 memory in big endian order works");
    ok(p48 == m48 + 6, "Setting u48 memory returns correct memory");
}

static void
t_big_set_u56(void)
{
    const uint64_t u56 = 0xffeeddccbbaa99ull;
    const uint8_t expect[] = { 0xffu, 0xeeu, 0xddu, 0xccu,
                               0xbbu, 0xaau, 0x99u };
    uint8_t m56[7] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    void *p56;

    p56 = bf_set_u56b(m56, u56);
    cmp_mem(m56, expect, sizeof(expect),
            "Setting u56 memory in big endian order works");
    ok(p56 == m56 + 7, "Setting u56 memory returns correct memory");
}

static void
t_big_set_u64(void)
{
    const uint64_t u64 = 0x1122334455667788ull;
    const uint8_t expect[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                               0x55u, 0x66u, 0x77u, 0x88u };
    uint64_t m64[] = { 0 };
    uint64_t *p64;

    p64 = bf_set_u64b(m64, u64);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting uint64_t memory in big endian order works");
    ok(p64 == m64 + 1, "Setting uint64_t memory returns correct memory");
}

static void
t_big_set_unsigned(void)
{
    t_big_set_u16();  /* 2 */
    t_big_set_u24();  /* 2 */
    t_big_set_u32();  /* 2 */
    t_big_set_u40();  /* 2 */
    t_big_set_u48();  /* 2 */
    t_big_set_u56();  /* 2 */
    t_big_set_u64();  /* 2 */
}

static void
t_big_set_s16(void)
{
    const int16_t s16 = 0x1234;
    const uint8_t expect[] = { 0x12u, 0x34u };
    int16_t m16[] = { 0 };
    void *p16;

    p16 = bf_set_s16b(m16, s16);
    cmp_mem(m16, expect, sizeof(expect),
            "Setting int16_t memory in big endian order works");
    ok(p16 == m16 + 1, "Setting int16_t memory returns correct memory");
}

static void
t_big_set_s24(void)
{
    const int32_t s24 = -4387l; /* 0xffeedd */
    const uint8_t expect[] = { 0xffu, 0xeeu, 0xddu };
    uint8_t m24[3] = { 0u, 0u, 0u };
    void *p24;

    p24 = bf_set_s24b(m24, s24);
    cmp_mem(m24, expect, sizeof(expect),
            "Setting s24 memory in big endian order works");
    ok(p24 == m24 + 3, "Setting s24 memory returns correct memory");
}

static void
t_big_set_s32(void)
{
    const int32_t s32 = 0x12345678l;
    const uint8_t expect[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    int32_t m32[] = { 0 };
    void *p32;

    p32 = bf_set_s32b(m32, s32);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting int32_t memory in big endian order works");
    ok(p32 == m32 + 1, "Setting int32_t memory returns correct memory");
}

static void
t_big_set_s40(void)
{
    const int64_t s40 = -287454021ll; /* 0xffeeddccbb */
    const uint8_t expect[] = { 0xffu, 0xeeu, 0xddu, 0xccu, 0xbbu };
    uint8_t m40[5] = { 0u, 0u, 0u, 0u, 0u };
    void *p40;

    p40 = bf_set_s40b(m40, s40);
    cmp_mem(m40, expect, sizeof(expect),
            "Setting s40 memory in big endian order works");
    ok(p40 == m40 + 5, "Setting s40 memory returns correct memory");
}

static void
t_big_set_s48(void)
{
    const int64_t s48 = -73588229206ll; /* 0xffeeddccbbaa */
    const uint8_t expect[] = { 0xffu, 0xeeu, 0xddu, 0xccu, 0xbbu, 0xaau };
    uint8_t m48[6] = { 0u, 0u, 0u, 0u, 0u, 0u };
    void *p48;

    p48 = bf_set_s48b(m48, s48);
    cmp_mem(m48, expect, sizeof(expect),
            "Setting s48 memory in big endian order works");
    ok(p48 == m48 + 6, "Setting s48 memory returns correct memory");
}

static void
t_big_set_s56(void)
{
    const int64_t s56 = -18838586676583ll; /* 0xffeeddccbbaa99 */
    const uint8_t expect[] = { 0xffu, 0xeeu, 0xddu, 0xccu,
                               0xbbu, 0xaau, 0x99u };
    uint8_t m56[7] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    void *p56;

    p56 = bf_set_s56b(m56, s56);
    cmp_mem(m56, expect, sizeof(expect),
            "Setting s56 memory in big endian order works");
    ok(p56 == m56 + 7, "Setting s56 memory returns correct memory");
}

static void
t_big_set_s64(void)
{
    const int64_t s64 = 0x1122334455667788ll;
    const uint8_t expect[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                               0x55u, 0x66u, 0x77u, 0x88u };
    int64_t m64[] = { 0 };
    void *p64;

    p64 = bf_set_s64b(m64, s64);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting int64_t memory in big endian order works");
    ok(p64 == m64 + 1, "Setting int64_t memory returns correct memory");
}

static void
t_big_set_signed(void)
{
    t_big_set_s16();  /* 2 */
    t_big_set_s24();  /* 2 */
    t_big_set_s32();  /* 2 */
    t_big_set_s40();  /* 2 */
    t_big_set_s48();  /* 2 */
    t_big_set_s56();  /* 2 */
    t_big_set_s64();  /* 2 */
}

static void
t_big_set_f32(void)
{
    const union { float f; uint32_t u; } f32 = { .u = 0x12345678ul };
    const uint8_t expect[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    float m32[] = { 0.f };
    float *p32;

    p32 = bf_set_f32b(m32, f32.f);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting float memory in big endian order works");
    ok(p32 == m32 + 1, "Setting float memory returns correct memory");
}

static void
t_big_set_f64(void)
{
    const union { double f; uint64_t u; } f64 = { .u = 0x1122334455667788ull };
    const uint8_t expect[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                               0x55u, 0x66u, 0x77u, 0x88u };
    double m64[] = { 0. };
    double *p64;

    p64 = bf_set_f64b(m64, f64.f);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting double memory in big endian order works");
    ok(p64 == m64 + 1, "Setting double memory returns correct memory");
}

static void
t_big_set_float(void)
{
    t_big_set_f32();  /* 2 */
    t_big_set_f64();  /* 2 */
}

static void
t_big_set(void)
{
    t_big_set_unsigned(); /* 14 */
    t_big_set_signed();   /* 14 */
    t_big_set_float();    /* 4 */
}

/*
 * Little endian octet-order tests
 */

/* Referencing */

static void
t_little_ref_unsigned(void)
{
    const uint16_t u16 = 0x1234u;
    const uint8_t m16[] = { 0x34u, 0x12u };
    const uint32_t u32 = 0x12345678ul;
    const uint32_t u24 = 0x345678ul;
    const uint8_t m32[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    const uint64_t u64 = 0x1122334455667788ull;
    const uint64_t u40 = 0x4455667788ull;
    const uint64_t u48 = 0x334455667788ull;
    const uint64_t u56 = 0x22334455667788ull;
    const uint8_t m64[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                            0x44u, 0x33u, 0x22u, 0x11u };

    ok(u16 == bf_ref_u16l(m16), "Referencing u16 in little endian order works");
    ok(u32 == bf_ref_u32l(m32), "Referencing u32 in little endian order works");
    ok(u64 == bf_ref_u64l(m64), "Referencing u64 in little endian order works");
    ok(u24 == bf_ref_u24l(m32), "Referencing u24 in little endian order works");
    ok(u40 == bf_ref_u40l(m64), "Referencing u40 in little endian order works");
    ok(u48 == bf_ref_u48l(m64), "Referencing u48 in little endian order works");
    ok(u56 == bf_ref_u56l(m64), "Referencing u56 in little endian order works");
}

static void
t_little_ref_signed(void)
{
    const int16_t s16 = 0x1234;
    const uint8_t m16[] = { 0x34u, 0x12u };
    const int32_t s32 = 0x12345678l;
    const uint8_t m32[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    const int64_t s64 = 0x1122334455667788ll;
    const uint8_t m64[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                            0x44u, 0x33u, 0x22u, 0x11u };

    ok(s16 == bf_ref_s16l(m16), "Referencing s16 in little endian order works");
    ok(s32 == bf_ref_s32l(m32), "Referencing s32 in little endian order works");
    ok(s64 == bf_ref_s64l(m64), "Referencing s64 in little endian order works");
    const uint8_t x64[] = { 0x88u, 0x99u, 0xaau, 0xbbu,
                            0xccu, 0xddu, 0xeeu, 0xffu };
    ok(-4387           == bf_ref_s24l(x64 + 5),
       "Referencing s24 in little endian order works");
    ok(-287454021      == bf_ref_s40l(x64 + 3),
       "Referencing s40 in little endian order works");
    ok(-73588229206    == bf_ref_s48l(x64 + 2),
       "Referencing s48 in little endian order works");
    ok(-18838586676583 == bf_ref_s56l(x64 + 1),
        "Referencing s56 in little endian order works");
}

static void
t_little_ref_float(void)
{
    const union { float f; uint32_t u; } f32 = { .u = 0x12345678ul };
    const uint8_t m32[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    const union { double f; uint64_t u; } f64 = { .u = 0x1122334455667788ull };
    const uint8_t m64[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                            0x44u, 0x33u, 0x22u, 0x11u };

    ok(f32.f == bf_ref_f32l(m32), "Referencing f32 in little endian order works");
    ok(f64.f == bf_ref_f64l(m64), "Referencing f64 in little endian order works");
}

static void
t_little_ref(void)
{
    t_little_ref_unsigned(); /* 7 */
    t_little_ref_signed();   /* 7 */
    t_little_ref_float();    /* 2 */
}

/* Mutation */

static void
t_little_set_u16(void)
{
    const uint16_t u16 = 0x1234u;
    const uint8_t expect[] = { 0x34u, 0x12u };
    uint16_t m16[] = { 0 };
    uint16_t *p16;

    p16 = bf_set_u16l(m16, u16);
    cmp_mem(m16, expect, sizeof(expect),
            "Setting uint16_t memory in big endian order works");
    ok(p16 == m16 + 1, "Setting uint16_t memory returns correct memory");
}

static void
t_little_set_u24(void)
{
    const uint32_t u24 = 0xffeeddul;
    const uint8_t expect[] = { 0xddu, 0xeeu, 0xffu };
    uint8_t m24[3] = { 0u, 0u, 0u };
    void *p24;

    p24 = bf_set_u24l(m24, u24);
    cmp_mem(m24, expect, sizeof(expect),
            "Setting u24 memory in little endian order works");
    ok(p24 == m24 + 3, "Setting u24 memory returns correct memory");
}

static void
t_little_set_u32(void)
{
    const uint32_t u32 = 0x12345678u;
    const uint8_t expect[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    uint32_t m32[] = { 0 };
    uint32_t *p32;

    p32 = bf_set_u32l(m32, u32);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting uint32_t memory in little endian order works");
    ok(p32 == m32 + 1, "Setting uint32_t memory returns correct memory");
}

static void
t_little_set_u40(void)
{
    const uint64_t u40 = 0xffeeddccbbul;
    const uint8_t expect[] = { 0xbbu, 0xccu, 0xddu, 0xeeu, 0xffu };
    uint8_t m40[5] = { 0u, 0u, 0u, 0u, 0u };
    void *p40;

    p40 = bf_set_u40l(m40, u40);
    cmp_mem(m40, expect, sizeof(expect),
            "Setting u40 memory in little endian order works");
    ok(p40 == m40 + 5, "Setting u40 memory returns correct memory");
}

static void
t_little_set_u48(void)
{
    const uint64_t u48 = 0xffeeddccbbaaul;
    const uint8_t expect[] = { 0xaau, 0xbbu, 0xccu, 0xddu, 0xeeu, 0xffu };
    uint8_t m48[6] = { 0u, 0u, 0u, 0u, 0u, 0u };
    void *p48;

    p48 = bf_set_u48l(m48, u48);
    cmp_mem(m48, expect, sizeof(expect),
            "Setting u48 memory in little endian order works");
    ok(p48 == m48 + 6, "Setting u48 memory returns correct memory");
}

static void
t_little_set_u56(void)
{
    const uint64_t u56 = 0xffeeddccbbaa99ul;
    const uint8_t expect[] = { 0x99u, 0xaau, 0xbbu, 0xccu,
                               0xddu, 0xeeu, 0xffu };
    uint8_t m56[7] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    void *p56;

    p56 = bf_set_u56l(m56, u56);
    cmp_mem(m56, expect, sizeof(expect),
            "Setting u56 memory in little endian order works");
    ok(p56 == m56 + 7, "Setting u56 memory returns correct memory");
}

static void
t_little_set_u64(void)
{
    const uint64_t u64 = 0x1122334455667788ull;
    const uint8_t expect[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                               0x44u, 0x33u, 0x22u, 0x11u };
    uint64_t m64[] = { 0 };
    uint64_t *p64;

    p64 = bf_set_u64l(m64, u64);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting uint64_t memory in little endian order works");
    ok(p64 == m64 + 1, "Setting uint64_t memory returns correct memory");
}

static void
t_little_set_unsigned(void)
{
    t_little_set_u16();  /* 2 */
    t_little_set_u24();  /* 2 */
    t_little_set_u32();  /* 2 */
    t_little_set_u40();  /* 2 */
    t_little_set_u48();  /* 2 */
    t_little_set_u56();  /* 2 */
    t_little_set_u64();  /* 2 */
}

static void
t_little_set_s16(void)
{
    const int16_t s16 = 0x1234;
    const uint8_t expect[] = { 0x34u, 0x12u };
    int16_t m16[] = { 0 };
    int16_t *p16;

    p16 = bf_set_s16l(m16, s16);
    cmp_mem(m16, expect, sizeof(expect),
            "Setting int16_t memory in little endian order works");
    ok(p16 == m16 + 1, "Setting int16_t memory returns correct memory");
}

static void
t_little_set_s24(void)
{
    const int32_t s24 = -4387l; /* 0xffeedd */
    const uint8_t expect[] = { 0xddu, 0xeeu, 0xffu };
    uint8_t m24[3] = { 0u, 0u, 0u };
    void *p24;

    p24 = bf_set_s24l(m24, s24);
    cmp_mem(m24, expect, sizeof(expect),
            "Setting s24 memory in little endian order works");
    ok(p24 == m24 + 3, "Setting s24 memory returns correct memory");
}

static void
t_little_set_s32(void)
{
    const int32_t s32 = 0x12345678l;
    const uint8_t expect[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    int32_t m32[] = { 0 };
    int32_t *p32;

    p32 = bf_set_s32l(m32, s32);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting int32_t memory in little endian order works");
    ok(p32 == m32 + 1, "Setting int32_t memory returns correct memory");
}

static void
t_little_set_s40(void)
{
    const int64_t s40 = -287454021ll; /* 0xffeeddccbb */
    const uint8_t expect[] = { 0xbbu, 0xccu, 0xddu, 0xeeu, 0xffu };
    uint8_t m40[5] = { 0u, 0u, 0u, 0u, 0u };
    void *p40;

    p40 = bf_set_s40l(m40, s40);
    cmp_mem(m40, expect, sizeof(expect),
            "Setting s40 memory in little endian order works");
    ok(p40 == m40 + 5, "Setting s40 memory returns correct memory");
}

static void
t_little_set_s48(void)
{
    const int64_t s48 = -73588229206ll; /* 0xffeeddccbbaa */
    const uint8_t expect[] = { 0xaau, 0xbbu, 0xccu, 0xddu, 0xeeu, 0xffu };
    uint8_t m48[6] = { 0u, 0u, 0u, 0u, 0u, 0u };
    void *p48;

    p48 = bf_set_s48l(m48, s48);
    cmp_mem(m48, expect, sizeof(expect),
            "Setting s48 memory in little endian order works");
    ok(p48 == m48 + 6, "Setting s48 memory returns correct memory");
}

static void
t_little_set_s56(void)
{
    const int64_t s56 = -18838586676583ll; /* 0xffeeddccbbaa99 */
    const uint8_t expect[] = { 0x99u, 0xaau, 0xbbu, 0xccu,
                               0xddu, 0xeeu, 0xffu };
    uint8_t m56[7] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    void *p56;

    p56 = bf_set_s56l(m56, s56);
    cmp_mem(m56, expect, sizeof(expect),
            "Setting s56 memory in little endian order works");
    ok(p56 == m56 + 7, "Setting s56 memory returns correct memory");
}

static void
t_little_set_s64(void)
{
    const int64_t s64 = 0x1122334455667788ll;
    const uint8_t expect[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                               0x44u, 0x33u, 0x22u, 0x11u };
    int64_t m64[] = { 0 };
    int64_t *p64;

    p64 = bf_set_s64l(m64, s64);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting int64_t memory in little endian order works");
    ok(p64 == m64 + 1, "Setting int64_t memory returns correct memory");
}

static void
t_little_set_signed(void)
{
    t_little_set_s16();  /* 2 */
    t_little_set_s24();  /* 2 */
    t_little_set_s32();  /* 2 */
    t_little_set_s40();  /* 2 */
    t_little_set_s48();  /* 2 */
    t_little_set_s56();  /* 2 */
    t_little_set_s64();  /* 2 */
}

static void
t_little_set_f32(void)
{
    const union { float f; uint32_t u; } f32 = { .u = 0x12345678ul };
    const uint8_t expect[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    float m32[] = { 0.f };
    float *p32;

    p32 = bf_set_f32l(m32, f32.f);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting float memory in little endian order works");
    ok(p32 == m32 + 1, "Setting float memory returns correct memory");
}

static void
t_little_set_f64(void)
{
    const union { double f; uint64_t u; } f64 = { .u = 0x1122334455667788ull };
    const uint8_t expect[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                               0x44u, 0x33u, 0x22u, 0x11u };
    double m64[] = { 0. };
    double *p64;

    p64 = bf_set_f64l(m64, f64.f);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting double memory in little endian order works");
    ok(p64 == m64 + 1, "Setting double memory returns correct memory");
}

static void
t_little_set_float(void)
{
    t_little_set_f32();  /* 2 */
    t_little_set_f64();  /* 2 */
}

static void
t_little_set(void)
{
    t_little_set_unsigned(); /* 6 */
    t_little_set_signed();   /* 6 */
    t_little_set_float();    /* 4 */
}

static void
t_range_test(void)
{
    /* 4*4 + 4*3 = 16 + 12 = 28 */
    ok(bf_inrange_s24(-8388609l) == false,
       "min(s24)-1 is not in valid range");
    ok(bf_inrange_s24(-8388608l) == true,
       "min(s24) is in valid range");
    ok(bf_inrange_s24( 8388607l) == true,
       "max(s24) is in valid range");
    ok(bf_inrange_s24( 8388608l) == false,
       "max(s24)+1 is not in valid range");

    ok(bf_inrange_s40(-549755813889ll) == false,
       "min(s40)-1 is not in valid range");
    ok(bf_inrange_s40(-549755813888) == true,
       "min(s40) is in valid range");
    ok(bf_inrange_s40( 549755813887ll) == true,
       "max(s40) is in valid range");
    ok(bf_inrange_s40( 549755813888ll) == false,
       "max(s40)+1 is not in valid range");

    ok(bf_inrange_s48(-140737488355329ll) == false,
       "min(s48)-1 is not in valid range");
    ok(bf_inrange_s48(-140737488355328ll) == true,
       "min(s48) is in valid range");
    ok(bf_inrange_s48( 140737488355327ll) == true,
       "max(s48) is in valid range");
    ok(bf_inrange_s48( 140737488355328ll) == false,
       "max(s48)+1 is not in valid range");

    ok(bf_inrange_s56(-36028797018963969) == false,
       "min(s56)-1 is not in valid range");
    ok(bf_inrange_s56(-36028797018963968) == true,
       "min(s56) is in valid range");
    ok(bf_inrange_s56( 36028797018963967) == true,
       "max(s56) is in valid range");
    ok(bf_inrange_s56( 36028797018963968) == false,
       "max(s56)+1 is not in valid range");

    ok(bf_inrange_u24(0) == true,
       "min(u24) is in valid range");
    ok(bf_inrange_u24(16777215ul) == true,
       "max(u24) is in valid range");
    ok(bf_inrange_u24(16777216ul) == false,
       "max(u24)+1 is not in valid range");

    ok(bf_inrange_u40(0) == true,
       "min(u40) is in valid range");
    ok(bf_inrange_u40(1099511627775ull) == true,
       "max(u40) is in valid range");
    ok(bf_inrange_u40(1099511627776ull) == false,
       "max(u40)+1 is not in valid range");

    ok(bf_inrange_u48(0) == true,
       "min(u48) is in valid range");
    ok(bf_inrange_u48(281474976710655ull) == true,
       "max(u48) is in valid range");
    ok(bf_inrange_u48(281474976710656ull) == false,
       "max(u48)+1 is not in valid range");

    ok(bf_inrange_u56(0) == true,
       "min(u56) is in valid range");
    ok(bf_inrange_u56(72057594037927935ull) == true,
       "max(u56) is in valid range");
    ok(bf_inrange_u56(72057594037927936ull) == false,
       "max(u56)+1 is not in valid range");
}

/* ***** */

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(14 + 3 * (16 + 32) + 28);

    t_swap();       /* 14 */
    t_native_ref(); /* 16 */
    t_native_set(); /* 32 */
    t_big_ref();    /* 16 */
    t_big_set();    /* 32 */
    t_little_ref(); /* 16 */
    t_little_set(); /* 32 */
    t_range_test(); /* 28 */

    return EXIT_SUCCESS;
}
