#include <float.h>
#include <stdint.h>
#include <stdlib.h>

#include <tap.h>

#include <common/compiler.h>

#include <c/binary-format.h>

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
t_swap(void)
{
    t_swap16(); /* 2 */
    t_swap32(); /* 2 */
    t_swap64(); /* 2 */
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

    ok(u16 == bf_ref_u16n((uint8_t*)&u16),
       "Referencing u16 in native order works");
    ok(u32 == bf_ref_u32n((uint8_t*)&u32),
       "Referencing u32 in native order works");
    ok(u64 == bf_ref_u64n((uint8_t*)&u64),
       "Referencing u64 in native order works");
}

static void
t_native_ref_signed(void)
{
    const int16_t s16 = 0x1234;
    const int32_t s32 = 0x12345678l;
    const int64_t s64 = 0x1122334455667788ll;

    ok(s16 == bf_ref_s16n((uint8_t*)&s16),
       "Referencing s16 in native order works");
    ok(s32 == bf_ref_s32n((uint8_t*)&s32),
       "Referencing s32 in native order works");
    ok(s64 == bf_ref_s64n((uint8_t*)&s64),
       "Referencing s64 in native order works");
}

static void
t_native_ref_float(void)
{
    const float f32 = (float)(1. / 123.3e12);
    const double f64 = (double)(1. / 123.3e12);

    ok(f32 == bf_ref_f32n((uint8_t*)&f32),
       "Referencing f32 in native order works");
    ok(f64 == bf_ref_f64n((uint8_t*)&f64),
       "Referencing f64 in native order works");
}

static void
t_native_ref(void)
{
    t_native_ref_unsigned(); /* 3 */
    t_native_ref_signed();   /* 3 */
    t_native_ref_float();    /* 2 */
}

/* Mutation */

static void
t_native_set_u16(void)
{
    const uint16_t u16 = 0x1234u;
    uint16_t m16[] = { 0 };
    uint16_t *p16;

    p16 = bf_set_u16n((uint8_t*)m16, u16);
    ok(u16 == *m16, "Setting uint16_t memory in native order works");
    ok(p16 == m16, "Setting uint16_t memory returns correct memory");
}

static void
t_native_set_u32(void)
{
    const uint32_t u32 = 0x12345678ul;
    uint32_t m32[] = { 0 };
    uint32_t *p32;

    p32 = bf_set_u32n((uint8_t*)m32, u32);
    ok(u32 == *m32, "Setting uint32_t memory in native order works");
    ok(p32 == m32, "Setting uint32_t memory returns correct memory");
}

static void
t_native_set_u64(void)
{
    const uint64_t u64 = 0x1122334455667788ull;
    uint64_t m64[] = { 0 };
    uint64_t *p64;

    p64 = bf_set_u64n((uint8_t*)m64, u64);
    ok(u64 == *m64, "Setting uint64_t memory in native order works");
    ok(p64 == m64, "Setting uint64_t memory returns correct memory");
}

static void
t_native_set_unsigned(void)
{
    t_native_set_u16();  /* 2 */
    t_native_set_u32();  /* 2 */
    t_native_set_u64();  /* 2 */
}

static void
t_native_set_s16(void)
{
    const int16_t s16 = INT16_MIN;
    int16_t m16[] = { 0 };
    int16_t *p16;

    p16 = bf_set_s16n((uint8_t*)m16, s16);
    ok(s16 == *m16, "Setting int16_t memory in native order works");
    ok(p16 == m16, "Setting int16_t memory returns correct memory");
}

static void
t_native_set_s32(void)
{
    const int32_t s32 = INT32_MIN;
    int32_t m32[] = { 0 };
    int32_t *p32;

    p32 = bf_set_s32n((uint8_t*)m32, s32);
    ok(s32 == *m32, "Setting int32_t memory in native order works");
    ok(p32 == m32, "Setting int32_t memory returns correct memory");
}

static void
t_native_set_s64(void)
{
    const int64_t s64 = INT64_MIN;
    int64_t m64[] = { 0 };
    int64_t *p64;

    p64 = bf_set_s64n((uint8_t*)m64, s64);
    ok(s64 == *m64, "Setting int64_t memory in native order works");
    ok(p64 == m64, "Setting int64_t memory returns correct memory");
}

static void
t_native_set_signed(void)
{
    t_native_set_s16();  /* 2 */
    t_native_set_s32();  /* 2 */
    t_native_set_s64();  /* 2 */
}

static void
t_native_set_f32(void)
{
    const float f32 = FLT_MAX;
    float m32[] = { 0.f };
    float *p32;

    p32 = bf_set_f32n((uint8_t*)m32, f32);
    ok(f32 == *m32, "Setting float memory in native order works");
    ok(p32 == m32, "Setting float memory returns correct memory");
}

static void
t_native_set_f64(void)
{
    const double f64 = DBL_MAX;
    double m64[] = { 0. };
    double *p64;

    p64 = bf_set_f64n((uint8_t*)m64, f64);
    ok(f64 == *m64, "Setting double memory in native order works");
    ok(p64 == m64, "Setting double memory returns correct memory");
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
    t_native_set_unsigned(); /* 6 */
    t_native_set_signed();   /* 6 */
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
    const uint8_t m32[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    const uint64_t u64 = 0x1122334455667788ull;
    const uint8_t m64[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                            0x55u, 0x66u, 0x77u, 0x88u };

    ok(u16 == bf_ref_u16b(m16), "Referencing u16 in big endian order works");
    ok(u32 == bf_ref_u32b(m32), "Referencing u32 in big endian order works");
    ok(u64 == bf_ref_u64b(m64), "Referencing u64 in big endian order works");
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
    t_big_ref_unsigned(); /* 3 */
    t_big_ref_signed();   /* 3 */
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

    p16 = bf_set_u16b((uint8_t*)m16, u16);
    cmp_mem(m16, expect, sizeof(expect),
            "Setting uint16_t memory in big endian order works");
    ok(p16 == m16, "Setting uint16_t memory returns correct memory");
}

static void
t_big_set_u32(void)
{
    const uint32_t u32 = 0x12345678ul;
    const uint8_t expect[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    uint32_t m32[] = { 0 };
    uint32_t *p32;

    p32 = bf_set_u32b((uint8_t*)m32, u32);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting uint32_t memory in big endian order works");
    ok(p32 == m32, "Setting uint32_t memory returns correct memory");
}

static void
t_big_set_u64(void)
{
    const uint64_t u64 = 0x1122334455667788ull;
    const uint8_t expect[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                               0x55u, 0x66u, 0x77u, 0x88u };
    uint64_t m64[] = { 0 };
    uint64_t *p64;

    p64 = bf_set_u64b((uint8_t*)m64, u64);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting uint64_t memory in big endian order works");
    ok(p64 == m64, "Setting uint64_t memory returns correct memory");
}

static void
t_big_set_unsigned(void)
{
    t_big_set_u16();  /* 2 */
    t_big_set_u32();  /* 2 */
    t_big_set_u64();  /* 2 */
}

static void
t_big_set_s16(void)
{
    const int16_t s16 = 0x1234;
    const uint8_t expect[] = { 0x12u, 0x34u };
    int16_t m16[] = { 0 };
    int16_t *p16;

    p16 = bf_set_s16b((uint8_t*)m16, s16);
    cmp_mem(m16, expect, sizeof(expect),
            "Setting int16_t memory in big endian order works");
    ok(p16 == m16, "Setting int16_t memory returns correct memory");
}

static void
t_big_set_s32(void)
{
    const int32_t s32 = 0x12345678l;
    const uint8_t expect[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    int32_t m32[] = { 0 };
    int32_t *p32;

    p32 = bf_set_s32b((uint8_t*)m32, s32);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting int32_t memory in big endian order works");
    ok(p32 == m32, "Setting int32_t memory returns correct memory");
}

static void
t_big_set_s64(void)
{
    const int64_t s64 = 0x1122334455667788ll;
    const uint8_t expect[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                               0x55u, 0x66u, 0x77u, 0x88u };
    int64_t m64[] = { 0 };
    int64_t *p64;

    p64 = bf_set_s64b((uint8_t*)m64, s64);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting int64_t memory in big endian order works");
    ok(p64 == m64, "Setting int64_t memory returns correct memory");
}

static void
t_big_set_signed(void)
{
    t_big_set_s16();  /* 2 */
    t_big_set_s32();  /* 2 */
    t_big_set_s64();  /* 2 */
}

static void
t_big_set_f32(void)
{
    const union { float f; uint32_t u; } f32 = { .u = 0x12345678ul };
    const uint8_t expect[] = { 0x12u, 0x34u, 0x56u, 0x78u };
    float m32[] = { 0.f };
    float *p32;

    p32 = bf_set_f32b((uint8_t*)m32, f32.f);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting float memory in big endian order works");
    ok(p32 == m32, "Setting float memory returns correct memory");
}

static void
t_big_set_f64(void)
{
    const union { double f; uint64_t u; } f64 = { .u = 0x1122334455667788ull };
    const uint8_t expect[] = { 0x11u, 0x22u, 0x33u, 0x44u,
                               0x55u, 0x66u, 0x77u, 0x88u };
    double m64[] = { 0. };
    double *p64;

    p64 = bf_set_f64b((uint8_t*)m64, f64.f);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting double memory in big endian order works");
    ok(p64 == m64, "Setting double memory returns correct memory");
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
    t_big_set_unsigned(); /* 6 */
    t_big_set_signed();   /* 6 */
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
    const uint8_t m32[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    const uint64_t u64 = 0x1122334455667788ull;
    const uint8_t m64[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                            0x44u, 0x33u, 0x22u, 0x11u };

    ok(u16 == bf_ref_u16l(m16), "Referencing u16 in little endian order works");
    ok(u32 == bf_ref_u32l(m32), "Referencing u32 in little endian order works");
    ok(u64 == bf_ref_u64l(m64), "Referencing u64 in little endian order works");
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
    t_little_ref_unsigned(); /* 3 */
    t_little_ref_signed();   /* 3 */
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

    p16 = bf_set_u16l((uint8_t*)m16, u16);
    cmp_mem(m16, expect, sizeof(expect),
            "Setting uint16_t memory in big endian order works");
    ok(p16 == m16, "Setting uint16_t memory returns correct memory");
}

static void
t_little_set_u32(void)
{
    const uint32_t u32 = 0x12345678u;
    const uint8_t expect[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    uint32_t m32[] = { 0 };
    uint32_t *p32;

    p32 = bf_set_u32l((uint8_t*)m32, u32);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting uint32_t memory in little endian order works");
    ok(p32 == m32, "Setting uint32_t memory returns correct memory");
}

static void
t_little_set_u64(void)
{
    const uint64_t u64 = 0x1122334455667788ull;
    const uint8_t expect[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                               0x44u, 0x33u, 0x22u, 0x11u };
    uint64_t m64[] = { 0 };
    uint64_t *p64;

    p64 = bf_set_u64l((uint8_t*)m64, u64);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting uint64_t memory in little endian order works");
    ok(p64 == m64, "Setting uint64_t memory returns correct memory");
}

static void
t_little_set_unsigned(void)
{
    t_little_set_u16();  /* 2 */
    t_little_set_u32();  /* 2 */
    t_little_set_u64();  /* 2 */
}

static void
t_little_set_s16(void)
{
    const int16_t s16 = 0x1234;
    const uint8_t expect[] = { 0x34u, 0x12u };
    int16_t m16[] = { 0 };
    int16_t *p16;

    p16 = bf_set_s16l((uint8_t*)m16, s16);
    cmp_mem(m16, expect, sizeof(expect),
            "Setting int16_t memory in little endian order works");
    ok(p16 == m16, "Setting int16_t memory returns correct memory");
}

static void
t_little_set_s32(void)
{
    const int32_t s32 = 0x12345678l;
    const uint8_t expect[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    int32_t m32[] = { 0 };
    int32_t *p32;

    p32 = bf_set_s32l((uint8_t*)m32, s32);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting int32_t memory in little endian order works");
    ok(p32 == m32, "Setting int32_t memory returns correct memory");
}

static void
t_little_set_s64(void)
{
    const int64_t s64 = 0x1122334455667788ll;
    const uint8_t expect[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                               0x44u, 0x33u, 0x22u, 0x11u };
    int64_t m64[] = { 0 };
    int64_t *p64;

    p64 = bf_set_s64l((uint8_t*)m64, s64);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting int64_t memory in little endian order works");
    ok(p64 == m64, "Setting int64_t memory returns correct memory");
}

static void
t_little_set_signed(void)
{
    t_little_set_s16();  /* 2 */
    t_little_set_s32();  /* 2 */
    t_little_set_s64();  /* 2 */
}

static void
t_little_set_f32(void)
{
    const union { float f; uint32_t u; } f32 = { .u = 0x12345678ul };
    const uint8_t expect[] = { 0x78u, 0x56u, 0x34u, 0x12u };
    float m32[] = { 0.f };
    float *p32;

    p32 = bf_set_f32l((uint8_t*)m32, f32.f);
    cmp_mem(m32, expect, sizeof(expect),
            "Setting float memory in little endian order works");
    ok(p32 == m32, "Setting float memory returns correct memory");
}

static void
t_little_set_f64(void)
{
    const union { double f; uint64_t u; } f64 = { .u = 0x1122334455667788ull };
    const uint8_t expect[] = { 0x88u, 0x77u, 0x66u, 0x55u,
                               0x44u, 0x33u, 0x22u, 0x11u };
    double m64[] = { 0. };
    double *p64;

    p64 = bf_set_f64l((uint8_t*)m64, f64.f);
    cmp_mem(m64, expect, sizeof(expect),
            "Setting double memory in little endian order works");
    ok(p64 == m64, "Setting double memory returns correct memory");
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

/* ***** */

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(6 + 3 * (8 + 16));

    t_swap();       /*  6 */
    t_native_ref(); /*  8 */
    t_native_set(); /* 16 */
    t_big_ref();    /*  8 */
    t_big_set();    /* 16 */
    t_little_ref(); /*  8 */
    t_little_set(); /* 16 */

    return EXIT_SUCCESS;
}
