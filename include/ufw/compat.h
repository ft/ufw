/*
 * Copyright (c) 2019-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_UFW_COMPAT_H
#define INC_UFW_UFW_COMPAT_H

/**
 * @addtogroup compat Compatibility Layer
 *
 * Ironing out incompatibilities in C standard libraries
 *
 * The header files in `ufw/compat/` provide workarounds for some compatibility
 * issues between toolchains. These include:
 *
 * - `ufw/compat/errno.h` — Defines some `errno` values that are not found in
 *   the active toolchain's C library.
 *
 * - `ufw/compat/math-constants.h` — This defines all POSIX math constants,
 *    that are not defined by the toolchain's C library.
 *
 * - `ufw/compat/ssize-t.h` — The `ssize_t` data type is useful as an addition
 *   to `size_t`, it is used as a return value type in POSIX `read()` and
 *   `write()`, which is a blueprint for many other APIs of this kind. However,
 *   some toolchains do not implement the type. This header tries to detect the
 *   situation and implements a suitable version if it can't find it.
 *
 * - `ufw/compat/strings.h` — This header defines prototype for `strlcat()`,
 *    `strlcyp()`, and `strnlen()`, if the toolchain's libc does not support
 *    them.
 *
 * See the individual header files documentation for all the details.
 *
 * @{
 *
 * @file ufw/compat.h
 * @brief Top level compatibility header
 *
 * This header includes all compatbility layers at once. It is mainly added for
 * backward compatibility. Consider using the compatibility portion you
 * actually need.
 *
 * @}
 */

#include <ufw/toolchain.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/math-constants.h>
#include <ufw/compat/ssize-t.h>
#include <ufw/compat/strings.h>

#endif /* INC_UFW_UFW_COMPAT_H */
