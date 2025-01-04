/*
 * Copyright (c) 2019-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file ssize-t.h
 * @brief Compatibility layer for ssize_t
 *
 * Provide ssize_t for toolchains/targets that do not support it by
 * implementing <sys/types.h>.
 */

#ifndef INC_UFW_UFW_COMPAT_SSIZE_T_H
#define INC_UFW_UFW_COMPAT_SSIZE_T_H

#include <ufw/toolchain.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/* Outer #if */
#ifdef WITH_SYS_TYPES_H
#include <sys/types.h>

#ifndef SSIZE_MAX
#ifdef SIZE_MAX
#define SSIZE_MAX ((SIZE_MAX) >> 1u)
#else
#error "How big is ssize_t?"
#endif /* SIZE_MAX */
#endif /* SSIZE_MAX */

/* Belongs to outer #if */
#else

#ifndef _SSIZE_T_DECLARED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef SIZE_MAX
#warning "No value for SIZE_MAX found!"
#endif

#if SIZE_MAX == UINT_MAX
typedef int ssize_t;
#define SSIZE_MAX INT_MAX
#define _SSIZE_T_DECLARED

#elif SIZE_MAX == ULONG_MAX
typedef long int ssize_t;
#define SSIZE_MAX LONG_MAX
#define _SSIZE_T_DECLARED

#else
#error "How big is ssize_t?"

#endif /* SIZE_MAX */
#endif /* !_SSIZE_T_DECLARED */
#endif /* WITH_SYS_TYPES_H */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_UFW_COMPAT_SSIZE_T_H */
