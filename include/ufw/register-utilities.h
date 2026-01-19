/*
 * Copyright (c) 2019-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_REGISTER_UTILITIES_H
#define INC_UFW_REGISTER_UTILITIES_H

/**
 * @addtogroup registers Register Table
 * @{
 *
 * @file ufw/register-utilities.h
 * @brief Register Table Utilities API
 *
 * @}
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <ufw/register-table.h>

void register_table_print(
    void *fh, const char *prefix, const RegisterTable *t);
void register_area_print(
    void *fh, const char *prefix, const RegisterArea *a);
void register_entry_print(
    void *fh, const char *prefix, const RegisterEntry *e);
void register_entry_print_value(void *fh, const RegisterEntry *e);
void register_value_print(void *fh, RegisterValue *v);
void register_validator_print(
    void *h, RegisterType type, const RegisterValidator *v);
void register_init_print(void *fh, const char *prefix, RegisterInit result);
char *register_accesscode_to_string(RegisterAccessCode code);
char *register_initcode_to_string(RegisterInitCode code);
char *register_registertype_to_string(RegisterType type);
char *register_validatortype_to_string(RegisterValidatorType type);

typedef void (*fprintf_like)(void*, const char*, ...);
void register_set_printer(fprintf_like p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_REGISTER_UTILITIES_H */
