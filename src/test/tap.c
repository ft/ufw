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
#include <stdio.h>

#include <test/tap.h>

void
plan(long unsigned int n)
{
    printf("1..%lu\n", n);
}

bool
ufw_test_ok(const char *file, long unsigned int line, bool result,
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
        printf("# %s:%lu: result == false\n", file, line);
    }

    va_end(ap);
    return result;
}
