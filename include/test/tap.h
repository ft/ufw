/*
 * Copyright (c) 2020 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file tap.h
 * @brief API for a minimal TAP emitting testing module
 */

#ifndef INC_UFW_TEST_TAP_H
#define INC_UFW_TEST_TAP_H

#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void plan(long unsigned int);
bool ufw_test_ok(const char*, long unsigned int, bool, const char*, ...);

#define ok(...) ufw_test_ok(__FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* INC_UFW_TEST_TAP_H */
