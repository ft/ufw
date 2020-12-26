/*
 * Copyright (c) 2020 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file tap.c
 * @brief Minimal TAP emitting testing module
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <common/compiler.h>
#include <common/toolchain.h>
#include <test/tap.h>

static void tap_result(
    const bool, const char*, unsigned long, const char*, va_list);

static void
tap_result(const bool result,
           const char *file, unsigned long line,
           const char *fmt, va_list ap)
{
    if (result == false) {
        fputs("not ", stdout);
    }

    fputs("ok - ", stdout);
    vprintf(fmt, ap);
    putchar('\n');

    if (result == false) {
        printf("#\n# failed test at:\n");
        printf("#   file: %s\n", file);
        printf("#   line: %lu\n", line);
    }
}

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
    tap_result(result, file, line, format, ap);
    va_end(ap);

    if (result == false) {
        printf("#   expr: (%s) => false\n#\n", expr);
    }

    va_end(ap);
    return result;
}

bool
ufw_test_cmp_mem(const char *file, long unsigned int line,
                 const void *a, const char *an,
                 const void *b, const char *bn,
                 size_t n,
                 const char *format, ...)
{
    const bool result = (memcmp(a, b, n) == 0);
    va_list ap;

    va_start(ap, format);
    tap_result(result, file, line, format, ap);
    va_end(ap);

    if (result == false) {
        printf("#   expr: (memcmp(%s, %s, %lu) == 0) => false\n#\n",
               an, bn, (unsigned long int)n);
        printf("# Expressions: a: (%s) b: (%s)\n#\n", an, bn);
        fputs("# memdiff:\n#\n", stdout);
        size_t differences = memdiff(a, b, n);
        printf("#\n# Found differences in %lu of %lu byte(s).\n#\n",
               (unsigned long int)differences, (unsigned long int)n);
    }

    return result;
}
