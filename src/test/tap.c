/*
 * Copyright (c) 2020 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file tap.c
 * @brief Minimal TAP emitting testing module
 */

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <common/bit-operations.h>
#include <common/compiler.h>
#include <common/toolchain.h>
#include <test/tap.h>

void
plan(long unsigned int n)
{
    printf("1..%lu\n", n);
}

bool
ufw_test_ok(const char *file, long unsigned int line,
            bool result, const char *expr,
            const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    if (result == true) {
        printf("ok - ");
        vprintf(format, ap);
        putchar('\n');
    } else {
        printf("not ok - ");
        vprintf(format, ap);
        putchar('\n');
        printf("#\n# failed test at:\n");
        printf("#   file: %s\n", file);
        printf("#   line: %lu\n", line);
        printf("#   expr: (%s) => false\n#\n", expr);
    }

    va_end(ap);
    return result;
}

static const char digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f' };

void
print_word_hex(void *memory, const size_t bytes, size_t columns)
{
    const size_t bits_per_digit = 4u;
    const size_t bits_per_byte = (size_t)BITS_PER_BYTE;
    const size_t bits_per_word = bits_per_byte * bytes;
    const size_t digitsteps = bits_per_byte / bits_per_digit;
    const size_t steps = bits_per_word / bits_per_byte;
    size_t step, pad;

    if (bytes > columns)
        columns = bytes;

    unsigned char *ptr = memory;
    /* Print hexadecimal data dump */
    for (step = 0u; step < steps; ++step) {
        if (step > 0 && (step % 8) == 0)
            putchar(' ');
        for (size_t j = digitsteps; j > 0; --j) {
            const unsigned int digit
                = BIT_GET(*ptr, bits_per_digit, (j-1) * bits_per_digit);
            putchar(digits[digit]);
        }
        ptr++;
        putchar(' ');
    }
    /* Pad to a given column width */
    if (bytes < columns) {
        size_t rest = columns - bytes;
        size_t spaces = step + rest;
        for (pad = step; pad < spaces; ++pad) {
            if (pad > 0 && (pad % 8) == 0)
                putchar(' ');
            for (size_t j = digitsteps; j > 0; --j) {
                putchar(' ');
            }
            ptr++;
            putchar(' ');
        }
    }
    /* Print printable-characters after data dump, similar to hexdump */
    fputs(" | ", stdout);
    ptr = memory;
    for (step = 0u; step < bytes; ++step) {
        const char ch = *ptr;
        if (step > 0 && (step % 8) == 0)
            putchar(' ');
        if (isprint(ch) && (isspace(ch) == false))
            putchar(ch);
        else
            putchar('.');
        ptr++;
    }
    /* Pad printable-characters to a given column width as well */
    if (bytes < columns) {
        size_t rest = step + columns - bytes;
        for (pad = step; pad < rest; ++pad) {
            if (pad > 0 && (pad % 8) == 0)
                putchar(' ');
            putchar(' ');
        }
    }
    fputs(" |\n", stdout);
}

#define define_printer(N,T)                                             \
    void                                                                \
    ufw_test_pr ## N(const char *nls, T ls, const char *nrs, T rs)

#define printer_body(P,Px,Nx,Po,No)                             \
    printf("# Expressions: a: (%s) b: (%s)\n#\n", nls, nrs);    \
    printf("#   dec:  a: %"        P  "\n", ls);                \
    printf("#         b: %"        P  "\n", rs);                \
    printf("#   hex:  a: 0x%0" #Nx Px "\n", ls);                \
    printf("#         b: 0x%0" #Nx Px "\n", rs);                \
    printf("#   oct:  a: o%0"  #No Po "\n", ls);                \
    printf("#         b: o%0"  #No Po "\n", rs);                \
    printf("#   mem:  a: ");                                    \
    print_word_hex(&ls, sizeof(ls), sizeof(ls));                \
    printf("#         b: ");                                    \
    print_word_hex(&rs, sizeof(rs), sizeof(rs));                \
    printf("#\n");

#define unsupported(N,T)                                                \
    void                                                                \
    ufw_test_pr ## N(const char *nls, UNUSED T ls,                      \
                     const char *nrs, UNUSED T rs)

#define unsupported_body(T)                                             \
    printf("# Expressions: a: (%s) b: (%s)\n#\n", nls, nrs);            \
    printf("#   test/tap: Unsupported data-type: " #T  "\n#\n");        \
    printf("#   mem:  a: ");                                            \
    print_word_hex(&ls, sizeof(ls), sizeof(ls));                        \
    printf("#         b: ");                                            \
    print_word_hex(&rs, sizeof(rs), sizeof(rs));                        \
    printf("#\n");


#ifdef WITH_UINT8_T
define_printer(u8,   uint8_t) { printer_body(PRIu8,   PRIx8,  2,  PRIo8,  3); }
define_printer(s8,    int8_t) { printer_body(PRId8,   PRIx8,  2,  PRIo8,  3); }
#endif /* WITH_UINT8_T */
define_printer(u16, uint16_t) { printer_body(PRIu16, PRIx16,  4, PRIo16,  6); }
define_printer(s16,  int16_t) { printer_body(PRId16, PRIx16,  4, PRIo16,  6); }
define_printer(u32, uint32_t) { printer_body(PRIu32, PRIx32,  8, PRIo32, 11); }
define_printer(s32,  int32_t) { printer_body(PRId32, PRIx32,  8, PRIo32, 11); }
#if defined(PRId64) && defined(PRIx64) && defined (PRIo64)
define_printer(u64, uint64_t) { printer_body(PRIu64, PRIx64, 16, PRIo64, 22); }
define_printer(s64,  int64_t) { printer_body(PRId64, PRIx64, 16, PRIo64, 22); }
#else
unsupported(u64, uint64_t) { unsupported_body(uint64_t); }
unsupported(s64,  int64_t) { unsupported_body( int64_t); }
#endif /* defined(PRId64) && defined(PRIx64) && defined (PRIo64) */
