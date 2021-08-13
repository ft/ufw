/*
 * Copyright (c) 2019-2021 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_REGISTER_INTERNAL_H
#define INC_REGISTER_INTERNAL_H

#include <c/register-table.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define REG_INIT_CODE_MAXIDX REG_INIT_ENTRY_INVALID_DEFAULT
#define REG_ACCESS_CODE_MAXIDX REG_ACCESS_READONLY
#define REG_TYPE_MAXIDX REG_TYPE_FLOAT32
#define REGV_TYPE_MAXIDX REGV_TYPE_CALLBACK

#define REG_LARGEST_DATUM uint64_t
#define REG_SIZEOF_LARGEST_DATUM (sizeof(REG_LARGEST_DATUM) / sizeof(RegisterAtom))

static inline bool
is_end_of_areas(RegisterArea *a)
{
    if (a->read != NULL || a->write != NULL)
        return false;
    if (a->size != 0 || a->base != 0)
        return false;
    if (a->mem != NULL)
        return false;
    return true;
}

static inline bool
is_end_of_entries(RegisterEntry *e)
{
    return (e->type == REG_TYPE_INVALID);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_REGISTER_INTERNAL_H */
