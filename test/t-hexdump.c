/*
 * Copyright (c) 2023 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compat/errno.h>

#include <ufw/hexdump.h>
#include <ufw/toolchain.h>

#ifdef WITH_UNISTD_H
#include <unistd.h>
#endif /* WITH_UNISTD_H */

#include <ufw/test/tap.h>

#define MEMORY_SIZE 1024u
unsigned char memory[MEMORY_SIZE];

#define LINES    32u
#define COLUMNS 256u
#define DISPLAY_SIZE (LINES * COLUMNS + 1u)
char display[DISPLAY_SIZE];

struct t_print_cfg {
    char *display;
    size_t size;
    bool verbose;
    size_t offset;
};

struct t_print_cfg outconf = {
    .display = display,
    .size = DISPLAY_SIZE,
#ifdef UFW_NATIVE_BUILD
    .verbose = false,
#else
    .verbose = true,
#endif /* UFW_NATIVE_BUILD */
    .offset = 0u
};

static int
t_printf(void *cfg, const char *fmt, ...)
{
    struct t_print_cfg *driver = cfg;
    if (driver->offset == driver->size) {
        return -EINVAL;
    }

    va_list args;
    va_start(args, fmt);
    const int rc = vsnprintf(driver->display + driver->offset,
                             driver->size - driver->offset - 1u,
                             fmt, args);
    va_end(args);

    if (rc > 0) {
        driver->offset += rc;
        driver->display[driver->offset] = '\0';
    }

    return rc;
}

static void
t_print_reset(struct t_print_cfg *driver)
{
    driver->offset = 0u;
}

static void
t_check_display(struct t_print_cfg *driver, size_t n, char *expect[])
{
    size_t m;
    char *start = driver->display;
    for (m = 0u; /* nop */; ++m) {
        char *end = strchr(start, '\n');
        if (end == NULL) {
            unless (ok(start >= (driver->display + driver->offset),
                       "Final line has a line-feed at the end"))
            {
                printf("# %s (no EOL)\n", start);
            }
            break;
        }
        *end = '\0';
        if (m < n) {
            ok(strcmp(start, expect[m]) == 0, "Line %u matches expectation",
               (unsigned)m);
        }
        if (driver->verbose) {
            printf("# %s\n", start);
        }
        start = end + 1;
    }
    ok(n == m, "Found as many lines as expected: %u", (unsigned)n);
}

int
main(int argc, char **argv)
{
#ifdef UFW_NATIVE_BUILD
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
        case 'v':
            outconf.verbose = true;
            break;
        default:
            fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                    argv[0]);
            printf("Unknown option: %c\n", opt);
            exit(EXIT_FAILURE);
        }

    }
    char *env_verbose = getenv("UFW_TEST_VERBOSE");
    if (env_verbose && (strcmp(env_verbose, "1") == 0)) {
        outconf.verbose = true;
    }
#else
    (void)argc;
    (void)argv;
#endif /* UFW_NATIVE_BUILD */

    memset(memory,  0, MEMORY_SIZE);
    memset(display, 0, LINES*COLUMNS);
    memory[23] = 0x42u;

    struct hexdump_cfg hd = {
        .printf = t_printf,
        .driver = &outconf,
        .octets_per_line  = HEXDUMP_DEFAULT_OCTETS_PER_LINE,
        .octets_per_chunk = HEXDUMP_DEFAULT_OCTETS_PER_CHUNK
    };

    plan(9u);

    t_print_reset(&outconf);
    okx(0 == hexdump(&hd, memory, 32u, 0x1000u));
    t_check_display(
        &outconf, 2u,
        (char*[]){
            "00001000  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|",
            "00001010  00 00 00 00 00 00 00 42  00 00 00 00 00 00 00 00  |.......B........|"
        });

    t_print_reset(&outconf);
    okx(0 == hexdump(&hd, memory + 20u, 5u, 0x2000u));
    t_check_display(
        &outconf, 1u,
        (char*[]){
            "00002000  00 00 00 42 00                                    |...B.|"
        });

    return EXIT_SUCCESS;
}
