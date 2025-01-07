/*
 * Copyright (c) 2020-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup testing Unit Testing Framework
 * @{
 */

/**
 * @file output-utils.c
 * @brief Output utilities for testing framework
 */

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include <ufw/compiler.h>
#include <ufw/hexdump.h>
#include <ufw/toolchain.h>

#include <ufw/test/tap.h>

static int
tap_hd_printf(UNUSED void *driver, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int rc = vprintf(fmt, args);
    va_end(args);
    return rc;
}

int
ufw_tap_hexdump(const char *file, const unsigned long line,
                const char *sdata, const char *ssize,
                const void *data, const size_t size)
{
    struct hexdump_cfg hd = {
        .printf = tap_hd_printf,
        .driver = NULL,
        .per_line_prefix = "# ",
        .octets_per_line  = HEXDUMP_DEFAULT_OCTETS_PER_LINE,
        .octets_per_chunk = HEXDUMP_DEFAULT_OCTETS_PER_CHUNK
    };
    printf("# %s:%lu:\n", file, line);
    printf("#     thexdump(%s, %s): <%zu>\n#\n", sdata, ssize, size);
    const int rc = hexdump(&hd, data, size, 0u);
    printf("#\n");
    return rc;
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
    print_word_hex(&ls, 0u, sizeof(ls), sizeof(ls));            \
    printf("#         b: ");                                    \
    print_word_hex(&rs, 0u, sizeof(rs), sizeof(rs));            \
    printf("#\n");

#define printer_body_simple(P,T)                                \
    printf("# Expressions: a: (%s) b: (%s)\n#\n", nls, nrs);    \
    printf("#   dec:  a: %"        P  "\n", (T)ls);             \
    printf("#         b: %"        P  "\n", (T)rs);             \
    printf("#   mem:  a: ");                                    \
    print_word_hex(&ls, 0u, sizeof(ls), sizeof(ls));            \
    printf("#         b: ");                                    \
    print_word_hex(&rs, 0u, sizeof(rs), sizeof(rs));            \
    printf("#\n");

#define unsupported(N,T)                                                \
    void                                                                \
    ufw_test_pr ## N(const char *nls, UNUSED T ls,                      \
                     const char *nrs, UNUSED T rs)

#define unsupported_body(T)                                             \
    printf("# Expressions: a: (%s) b: (%s)\n#\n", nls, nrs);            \
    printf("#   test/tap: Unsupported data-type: " #T  "\n#\n");        \
    printf("#   mem:  a: ");                                            \
    print_word_hex(&ls, 0u, sizeof(ls), sizeof(ls));                    \
    printf("#         b: ");                                            \
    print_word_hex(&rs, 0u, sizeof(rs), sizeof(rs));                    \
    printf("#\n");


#ifdef WITH_UINT8_T
define_printer(u8,   uint8_t) { printer_body(PRIu8,   PRIx8,  2,  PRIo8,  3); }
define_printer(s8,    int8_t) { printer_body(PRId8,   PRIx8,  2,  PRIo8,  3); }
#endif /* WITH_UINT8_T */
define_printer(u16, uint16_t) { printer_body(PRIu16, PRIx16,  4, PRIo16,  6); }
define_printer(s16,  int16_t) { printer_body(PRId16, PRIx16,  4, PRIo16,  6); }
define_printer(u32, uint32_t) { printer_body(PRIu32, PRIx32,  8, PRIo32, 11); }
define_printer(s32,  int32_t) { printer_body(PRId32, PRIx32,  8, PRIo32, 11); }
define_printer(f32,  float)   { printer_body_simple("f", double); }
#if defined(PRId64) && defined(PRIx64) && defined (PRIo64)
define_printer(u64, uint64_t) { printer_body(PRIu64, PRIx64, 16, PRIo64, 22); }
define_printer(s64,  int64_t) { printer_body(PRId64, PRIx64, 16, PRIo64, 22); }
#else
unsupported(u64, uint64_t) { unsupported_body(uint64_t); }
unsupported(s64,  int64_t) { unsupported_body( int64_t); }
#endif /* defined(PRId64) && defined(PRIx64) && defined (PRIo64) */

/**
 * @}
 */
