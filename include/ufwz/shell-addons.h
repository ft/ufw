/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFWZ_SHELL_ADDONS_H
#define INC_UFWZ_SHELL_ADDONS_H

#include <stdarg.h>

#include <ufw/register-table.h>

void ufw_shell_fprintf(void*, const char*, ...);
void ufw_shell_reg_init(RegisterTable*);

#endif /* INC_UFWZ_SHELL_ADDONS_H */
