/*
 * Copyright (c) 2022-2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file strings.h
 * @brief Compatibility header for bsd-ish extensions to string library
 *
 * Declare OpenBSD style string operation if configuration couldn't find them
 * in the system's std-c lib.
 */

#ifndef INC_UFW_UFW_COMPAT_STRINGS_H
#define INC_UFW_UFW_COMPAT_STRINGS_H

#include <stddef.h>
#include <ufw/toolchain.h>

#ifndef UFW_COMPAT_HAVE_STRLCAT
size_t strlcat(char *, const char *, size_t);
#endif /* !UFW_COMPAT_HAVE_STRLCAT */

#ifndef UFW_COMPAT_HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif /* !UFW_COMPAT_HAVE_STRLCPY */

#ifndef UFW_COMPAT_HAVE_STRNLEN
size_t strnlen(const char *, size_t);
#endif /* !UFW_COMPAT_HAVE_STRNLEN */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_UFW_COMPAT_STRINGS_H */
