/*
 * Copyright (c) 2020 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file tap.h
 * @brief API for a minimal TAP emitting testing module
 */

#ifndef INC_UFW_TEST_TAP_H
#define INC_UFW_TEST_TAP_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void plan(long unsigned int);
bool ufw_test_ok(const char*, long unsigned int,
                 bool, const char*,
                 const char*, ...);


void print_word_hex(void*, size_t, size_t);

#define ok(expr, ...)                           \
    ufw_test_ok(__FILE__, __LINE__,             \
                expr, #expr,                    \
                __VA_ARGS__)

#define unless(x) if (!(x))

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
#define pru32(ls, rs) ufw_test_pru32(#ls, ls, #rs, rs)
#define prs32(ls, rs) ufw_test_prs32(#ls, ls, #rs, rs)

declare_printer(u64, uint64_t);
declare_printer(s64,  int64_t);
#define pru64(ls, rs) ufw_test_pru64(#ls, ls, #rs, rs)
#define prs64(ls, rs) ufw_test_prs64(#ls, ls, #rs, rs)

#ifdef __cplusplus
}
#endif

#endif /* INC_UFW_TEST_TAP_H */
