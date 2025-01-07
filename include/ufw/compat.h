/*
 * Copyright (c) 2019-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_UFW_COMPAT_H
#define INC_UFW_UFW_COMPAT_H

/**
 * @addtogroup compat Compatibility Layer
 * @{
 */

/**
 * @file compat.h
 * @brief Top level compatibility header
 *
 * This header includes all compatbility layers at once. It is mainly added for
 * backward compatibility. Consider using the compatibility portion you
 * actually need.
 */

#include <ufw/toolchain.h>
#include <ufw/compat/errno.h>
#include <ufw/compat/math-constants.h>
#include <ufw/compat/ssize-t.h>
#include <ufw/compat/strings.h>

/**
 * @}
 */

#endif /* INC_UFW_UFW_COMPAT_H */
