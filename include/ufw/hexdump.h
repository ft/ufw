/*
 * Copyright (c) 2023-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup output Output Utilities
 *
 * Common useful output utilities
 *
 * @{
 *
 * @file ufw/hexdump.h
 * @brief Memory dumper in the style of the hexdump(1) utility
 *
 * @}
 */

#ifndef INC_UFW_HEXDUMP_H_998d0241
#define INC_UFW_HEXDUMP_H_998d0241

#include <stddef.h>

#define HEXDUMP_DEFAULT_OCTETS_PER_LINE  16u
#define HEXDUMP_DEFAULT_OCTETS_PER_CHUNK  8u

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct hexdump_cfg {
    int (*printf)(void*, const char*, ...);
    const char *per_line_prefix;
    void *driver;
    size_t octets_per_line;
    size_t octets_per_chunk;
};

int hexdump(const struct hexdump_cfg*, const void*, size_t, size_t);
int hexdump_stdout(const void*, size_t, size_t);
int hexdump_stderr(const void*, size_t, size_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_HEXDUMP_H_998d0241 */
