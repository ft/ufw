/*
 * Copyright (c) 2023-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup output Output Utilities
 * @{
 */

/**
 * @file hexdump.c
 * @brief Memory dumper in the style of the hexdump(1) utility
 */

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <ufw/compat/errno.h>
#include <ufw/hexdump.h>
#include <ufw/toolchain.h>

static void
hexdump_ascii(const struct hexdump_cfg *cfg, const unsigned char *data,
              size_t bol, size_t n)
{
#ifdef UFW_HAVE_CTYPE_ISPRINT
    cfg->printf(cfg->driver, "  |");
    for (size_t i = bol; i < bol + n; ++i) {
        cfg->printf(cfg->driver, "%c", isprint(data[i]) ? data[i] : '.');
    }
    cfg->printf(cfg->driver, "|\n");
#else
    (void)data;
    (void)bol;
    (void)n;
    cfg->printf(cfg->driver, "\n");
#endif /* UFW_HAVE_CTYPE_ISPRINT */
}

/**
 * Memory dumper in the style of the hexdump(1) utility
 *
 * @verbatim
 * 00010000  20 43 6f 70 79 72 69 67  68 74 20 c2 a9 20 32 30  | Copyright .. 20|
 * 00010010  31 37 2d 32 30 32 32 20  6d 69 63 72 6f 20 66 72  |17-2022 micro fr|
 * 00010020  61 6d 65 77 6f 72 6b 20  77 6f 72 6b 65 72 73 2c  |amework workers,|
 * 00010030  20 41 6c 6c 20 72 69 67  68 74 73 20 72 65 73 65  | All rights rese|
 * 00010040  72 76 65 64 2e 0a 0a 20  52 65 64 69 73 74 72 69  |rved... Redistri|
 * @endverbatim
 *
 * @param  cfg      Hexdump configuration data
 * @param  mem      Pointer to memory to dump
 * @param  n        Number of octets of dump from ‘mem’.
 * @param  doffset  Offset to use for display purposes only.
 *
 * @return Zero on success; negative errno on error.
 * @sideeffects Prints to a output backend handed in by ‘cfg’.
 */
int
hexdump(const struct hexdump_cfg *cfg, const void *mem,
        const size_t n, const size_t doffset)
{
    if (cfg->printf == NULL) {
        return -EINVAL;
    }
    if (cfg->octets_per_line < cfg->octets_per_chunk) {
        return -EINVAL;
    }
    if ((cfg->octets_per_line == 0u) || (cfg->octets_per_chunk == 0u)) {
        return -EINVAL;
    }

    const unsigned char *data = mem;
    const size_t lastn = n % cfg->octets_per_line;
    const size_t pad = lastn > 0 ? cfg->octets_per_line - lastn : 0u;

    if (cfg->per_line_prefix != NULL) {
        printf("%s", cfg->per_line_prefix);
    }

    size_t bol = 0u;
    for (size_t i = 0u; i < (n + pad); ++i) {
        if ((i % cfg->octets_per_line) == 0) {
            if (i > 0u) {
                hexdump_ascii(cfg, mem, bol, i - bol);
                if (cfg->per_line_prefix != NULL) {
                    printf("%s", cfg->per_line_prefix);
                }
            }
            /* Some embedded toolchains don't implement %zx in printf, which
             * will lead to unexpected output. PRIx32 from inttypes.h seems to
             * be more widespread, so that's what we're using here. Should be
             * more than enough for embedded memory buffers. */
            cfg->printf(cfg->driver, "%08"PRIx32" ", (uint32_t)i + doffset);
            bol = i;
        }
        if ((i > bol) && ((i - bol) % cfg->octets_per_chunk) == 0) {
            cfg->printf(cfg->driver, " ");
        }
        if (i >= n) {
            cfg->printf(cfg->driver, "   ");
        } else {
            cfg->printf(cfg->driver, " %02x", data[i]);
        }
    }

    hexdump_ascii(cfg, mem, bol, n - bol);
    return 0;
}

static int
hd_printf(void *driver, const char *fmt, ...)
{
    FILE *stream = driver;
    va_list args;
    va_start(args, fmt);
    const int rc = vfprintf(stream, fmt, args);
    va_end(args);

    return rc;
}

/**
 * Variant of hexdump() that uses stdio's stdout
 *
 * @param  mem      Pointer to memory to dump
 * @param  n        Number of octets of dump from ‘mem’.
 * @param  doffset  Offset to use for display purposes only.
 *
 * @return Zero on success; negative errno on error.
 * @sideeffects Prints to a output backend handed in by ‘cfg’.
 */
int
hexdump_stdout(const void *mem, const size_t n, const size_t doffset)
{
    struct hexdump_cfg hd = {
        .printf = hd_printf,
        .driver = stdout,
        .per_line_prefix = NULL,
        .octets_per_line  = HEXDUMP_DEFAULT_OCTETS_PER_LINE,
        .octets_per_chunk = HEXDUMP_DEFAULT_OCTETS_PER_CHUNK
    };
    return hexdump(&hd, mem, n, doffset);
}

/**
 * Variant of hexdump() that uses stdio's stderr
 *
 * @param  mem      Pointer to memory to dump
 * @param  n        Number of octets of dump from ‘mem’.
 * @param  doffset  Offset to use for display purposes only.
 *
 * @return Zero on success; negative errno on error.
 * @sideeffects Prints to a output backend handed in by ‘cfg’.
 */
int
hexdump_stderr(const void *mem, const size_t n, const size_t doffset)
{
    struct hexdump_cfg hd = {
        .printf = hd_printf,
        .driver = stderr,
        .per_line_prefix = NULL,
        .octets_per_line  = HEXDUMP_DEFAULT_OCTETS_PER_LINE,
        .octets_per_chunk = HEXDUMP_DEFAULT_OCTETS_PER_CHUNK
    };
    return hexdump(&hd, mem, n, doffset);
}

/**
 * @}
 */
