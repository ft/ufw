#ifndef INC_REGISTER_UTILITIES_H
#define INC_REGISTER_UTILITIES_H

#include <stdio.h>

#include <c/register-table.h>

void register_table_print(FILE*, const char*, const RegisterTable*);
void register_area_print(FILE*, const char*, const RegisterArea*);
void register_entry_print(FILE*, const char*, const RegisterEntry*);
void register_init_print(FILE*, const char*, RegisterInit);
char *register_accesscode_to_string(RegisterAccessCode);
char *register_initcode_to_string(RegisterInitCode);
char *register_registertype_to_string(RegisterType);
char *register_validatortype_to_string(RegisterValidatorType);

#endif /* INC_REGISTER_UTILITIES_H */
