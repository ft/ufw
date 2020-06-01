#ifndef INC_UFW_COMPAT_H
#define INC_UFW_COMPAT_H

#include <common/toolchain.h>

#ifdef WITH_SYS_TYPES_H
#include <sys/types.h>

#else

#ifndef _SSIZE_T_DECLARED

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef SIZE_MAX
#warning "No value for SIZE_MAX found!"
#endif

#if SIZE_MAX == UINT_MAX
typedef int ssize_t;
#define _SSIZE_T_DECLARED

#elif SIZE_MAX == ULONG_MAX
typedef long int ssize_t;
#define _SSIZE_T_DECLARED

#else
#error "How big is ssize_t?"

#endif /* SIZE_MAX */

#endif /* !_SSIZE_T_DECLARED */

#endif /* WITH_SYS_TYPES_H */

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

#endif /* INC_UFW_COMPAT_H */
