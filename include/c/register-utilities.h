#ifndef INC_REGISTER_UTILITIES_H
#define INC_REGISTER_UTILITIES_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <c/register-table.h>

void register_table_print(void*, const char*, const RegisterTable*);
void register_area_print(void*, const char*, const RegisterArea*);
void register_entry_print(void*, const char*, const RegisterEntry*);
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

#endif /* INC_REGISTER_UTILITIES_H */
