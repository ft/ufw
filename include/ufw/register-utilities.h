/*
 * Copyright (c) 2019-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_REGISTER_UTILITIES_H
#define INC_UFW_REGISTER_UTILITIES_H

/**
 * @addtogroup registers Register Table
 * @{
 *
 * @file register-utilities.h
 * @brief Register Table Utilities API
 *
 * @}
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <ufw/register-table.h>

void register_table_print(void*, const char*, const RegisterTable*);
void register_area_print(void*, const char*, const RegisterArea*);
void register_entry_print(void*, const char*, const RegisterEntry*);
void register_entry_print_value(void *fh, const RegisterEntry *e);
void register_value_print(void*, RegisterValue*);
void register_validator_print(void*, RegisterType, const RegisterValidator*);
void register_init_print(void*, const char*, RegisterInit);
char *register_accesscode_to_string(RegisterAccessCode);
char *register_initcode_to_string(RegisterInitCode);
char *register_registertype_to_string(RegisterType);
char *register_validatortype_to_string(RegisterValidatorType);

typedef void (*fprintf_like)(void*, const char*, ...);
void register_set_printer(fprintf_like);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_REGISTER_UTILITIES_H */
