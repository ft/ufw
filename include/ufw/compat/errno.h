/*
 * Copyright (c) 2022-2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file errno.h
 * @brief Compatibility layer for POSIX error numbers
 *
 * While a common error number base is not the worst idea in the world, the
 * number and kind of available errors varies a lot from implementation to
 * implementation. The C standard only requires EDOM, EILSEQ and ERANGE.
 *
 * This header tries to smooth over some differences. To use error handling
 * with ufw's API, include “ufw/compat/errno.h” instead of “errno.h”.
 */

#ifndef INC_UFW_UFW_MATH_CONSTANTS_H
#define INC_UFW_UFW_MATH_CONSTANTS_H

#include <errno.h>

#include <ufw/toolchain.h>

#ifndef EINVAL
/* While the C Standard does not mandate EINVAL, it is a *very* common error
 * number, so lets be loud about this one. */
#warning compat: Implementing EINVAL - Are you sure your C library is okay?
#define EINVAL      (UFW_PRIVATE_ERRNO_OFFSET + 0)
#endif /* EINVAL */

#ifndef ENOMEM
#define ENOMEM      (UFW_PRIVATE_ERRNO_OFFSET + 1)
#endif /* ENOMEM */

#ifndef ENODATA
#define ENODATA     (UFW_PRIVATE_ERRNO_OFFSET + 2)
#endif /* ENODATA */

#ifndef EBADMSG
#define EBADMSG     (UFW_PRIVATE_ERRNO_OFFSET + 3)
#endif /* EBADMSG */

#ifndef EOVERFLOW
#define EOVERFLOW   (UFW_PRIVATE_ERRNO_OFFSET + 4)
#endif /* EOVERFLOW */

#ifndef EILSEQ
#define EILSEQ      (UFW_PRIVATE_ERRNO_OFFSET + 5)
#endif /* EILSEQ */

#ifndef EPROTO
#define EPROTO      (UFW_PRIVATE_ERRNO_OFFSET + 6)
#endif /* EPROTO */

#ifndef ENOBUFS
#define ENOBUFS     (UFW_PRIVATE_ERRNO_OFFSET + 7)
#endif /* ENOBUFS */

#ifndef ENOMSG
#define ENOMSG      (UFW_PRIVATE_ERRNO_OFFSET + 8)
#endif /* ENOMSG */

#ifndef EFAULT
#define EFAULT      (UFW_PRIVATE_ERRNO_OFFSET + 9)
#endif /* EFAULT */

#ifndef EMSGSIZE
#define EMSGSIZE    (UFW_PRIVATE_ERRNO_OFFSET + 10)
#endif /* EMSGSIZE */

#ifndef EDOM
#define EDOM        (UFW_PRIVATE_ERRNO_OFFSET + 11)
#endif /* EDOM */

#ifndef ERANGE
#define ERANGE      (UFW_PRIVATE_ERRNO_OFFSET + 12)
#endif /* ERANGE */

#endif /* INC_UFW_UFW_MATH_CONSTANTS_H */
