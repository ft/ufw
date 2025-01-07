/*
 * Copyright (c) 2020-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_UFW_TEST_TAP_H
#define INC_UFW_UFW_TEST_TAP_H

/**
 * @addtogroup testing Unit Testing Framework
 * @{
 */

/**
 * @file tap.h
 * @brief API for a minimal TAP emitting testing module
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* TAP worker API */

bool ufw_test_ok(const char*, long unsigned int,
                 bool, const char*,
                 const char*, ...);
bool ufw_test_cmp_mem(const char*, long unsigned int,
                      const void*, const char*,
                      const void*, const char*,
                      size_t,
                      const char*, ...);

/* TAP user API */

void tap_init(void);
void plan(long unsigned int);
void noplan(void);

#define ok(expr, ...)                           \
    ufw_test_ok(__FILE__, __LINE__,             \
                expr, #expr,                    \
                __VA_ARGS__)

#define okx(expr) ufw_test_ok(__FILE__, __LINE__, expr, #expr, NULL)

#define cmp_mem(a, b, n, ...)                   \
    ufw_test_cmp_mem(__FILE__, __LINE__,        \
                     a, #a,                     \
                     b, #b,                     \
                     n,                         \
                     __VA_ARGS__)

/* Generic utilities for tests */

size_t memdiff(const void*, const void*, size_t);
void print_word_hex(const void*, size_t, size_t, size_t);

#define unless(x) if (!(x))

/* Output utilities */

int ufw_tap_hexdump(const char*, unsigned long,
                    const char*, const char*,
                    const void*, size_t);
#define thexdump(data, size) ufw_tap_hexdump(__FILE__, __LINE__,        \
                                             #data, #size, data, size)

#define declare_printer(N,T)                                    \
    void ufw_test_pr ## N(const char*, T, const char*, T)

#ifdef WITH_UINT8_T
declare_printer(u8, uint8_t);
declare_printer(s8,  int8_t);
#define pru8(ls, rs) ufw_test_pru8(#ls, ls, #rs, rs)
#define prs8(ls, rs) ufw_test_prs8(#ls, ls, #rs, rs)
#endif /* WITH_UINT8_T */

declare_printer(u16, uint16_t);
declare_printer(s16,  int16_t);
#define pru16(ls, rs) ufw_test_pru16(#ls, ls, #rs, rs)
#define prs16(ls, rs) ufw_test_prs16(#ls, ls, #rs, rs)

declare_printer(u32, uint32_t);
declare_printer(s32,  int32_t);
declare_printer(f32,  float);
#define pru32(ls, rs) ufw_test_pru32(#ls, ls, #rs, rs)
#define prs32(ls, rs) ufw_test_prs32(#ls, ls, #rs, rs)
#define prf32(ls, rs) ufw_test_prf32(#ls, ls, #rs, rs)

declare_printer(u64, uint64_t);
declare_printer(s64,  int64_t);
#define pru64(ls, rs) ufw_test_pru64(#ls, ls, #rs, rs)
#define prs64(ls, rs) ufw_test_prs64(#ls, ls, #rs, rs)

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* INC_UFW_UFW_TEST_TAP_H */
