/*
 * Copyright (c) 2019-2021 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

/* With fairly recent newlib (3.3.0), this header has a couple of dependencies
 * that breaks the availability of macros like PRIu64 (the 64-bit ones). Lea-
 * ding to compilation errors in ‘register_value_print()’. Including this after
 * stdio.h fixes the issue. */
#include <inttypes.h>

#include <ufw/register-table.h>
#include <ufw/register-utilities.h>

#include "register-internal.h"

#define IDX2STR(x) [x] = #x

static void
rimpl_fprintf(void *dst, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(dst, fmt, args);
    va_end(args);
}

static fprintf_like r_fprintf = rimpl_fprintf;

void
register_set_printer(fprintf_like p)
{
    r_fprintf = p;
}

void
register_table_print(void *fh, const char *prefix, const RegisterTable *t)
{
    const bool is_initialised = BIT_ISSET(t->flags, REG_TF_INITIALISED);
    r_fprintf(fh, "%sRegisterTable ", prefix);
    if (is_initialised == false) {
        r_fprintf(fh, "is NOT initialised!\n");
    } else {
        r_fprintf(fh, "is initialised.\n");
        r_fprintf(fh, "%s     areas: %" PRIu16 "\n", prefix, t->areas);
        r_fprintf(fh, "%s   entries: %" PRIu32 "\n", prefix, t->entries);
    }

    r_fprintf(fh, "%s\n%sList of Areas:\n", prefix, prefix);
    AreaHandle area = 0;
    while ((area < AREA_HANDLE_MAX) && !(is_end_of_areas(t->area + area))) {
        RegisterArea *a = t->area + area;
        r_fprintf(fh, "%s  AreaID: %" PRIu16 "\n", prefix, area);
        register_area_print(fh, prefix, a);
        area += 1;
    }

    r_fprintf(fh, "%s\n%sList of Entries:\n", prefix, prefix);
    RegisterHandle reg = 0;
    while ((reg < AREA_HANDLE_MAX) && !(is_end_of_entries(t->entry + reg))) {
        RegisterEntry *e = t->entry + reg;
        r_fprintf(fh, "%s  RegisterEntryID: %" PRIu32 "\n", prefix, reg);
        register_entry_print(fh, prefix, e);
        reg += 1;
    }
}

void
register_area_print(void *fh, const char *prefix, const RegisterArea *a)
{
    r_fprintf(fh, "%s    Area Start: 0x%08" PRIx32 "\n", prefix, a->base);
    r_fprintf(fh, "%s    Area Size : 0x%08" PRIx32 "\n", prefix, a->size);
    r_fprintf(fh, "%s    Area Flags: 0x%08" PRIx32 "\n", prefix,
              (uint32_t)a->flags);
    r_fprintf(fh, "%s    Area has %s read method.\n", prefix,
              a->read == NULL ? "no" : "a");
    r_fprintf(fh, "%s    Area has %s write method.\n", prefix,
              a->write == NULL ? "no" : "a");
}

void
register_entry_print(void *fh, const char *prefix, const RegisterEntry *e)
{
    r_fprintf(fh, "%s    Register Name   : %s\n", prefix,
              e->name != NULL ? e->name : "<UNNAMED-REGISTER>");
    r_fprintf(fh, "%s    Register Type   : %s\n", prefix,
              register_registertype_to_string(e->type));
    r_fprintf(fh, "%s    Register Flags  : 0x%08" PRIx32 "\n", prefix,
              (uint32_t)e->flags);
    r_fprintf(fh, "%s    Register Address: 0x%08" PRIx32 "\n", prefix, e->address);
    r_fprintf(fh, "%s    Default Value   : ", prefix);
    register_entry_print_value(fh, e);
    r_fprintf(fh, "\n%s    Validation Type : ", prefix);
    register_validator_print(fh, e->type, &e->check);
    r_fprintf(fh, "\n");
}

void
register_entry_print_value(void *fh, const RegisterEntry *e)
{
    RegisterValue def = { .type = e->type, .value = e->default_value };
    register_value_print(fh, &def);
}

void
register_validator_print(void *h, RegisterType type, const RegisterValidator *v)
{
    RegisterValue value;
    value.type = type;

    switch (v->type) {
    case REGV_TYPE_TRIVIAL:
        r_fprintf(h, "<trivial>");
        break;
    case REGV_TYPE_MIN:
        r_fprintf(h, "<min: ");
        value.value = v->arg.min;
        register_value_print(h, &value);
        r_fprintf(h, ">");
        break;
    case REGV_TYPE_MAX:
        r_fprintf(h, "<max: ");
        value.value = v->arg.max;
        register_value_print(h, &value);
        r_fprintf(h, ">");
        break;
    case REGV_TYPE_RANGE:
        r_fprintf(h, "<range: ");
        value.value = v->arg.range.min;
        register_value_print(h, &value);
        r_fprintf(h, ", ");
        value.value = v->arg.range.max;
        register_value_print(h, &value);
        r_fprintf(h, ">");
        break;
    case REGV_TYPE_CALLBACK:
        r_fprintf(h, "<callback>");
        break;
    default:
        r_fprintf(h, "<unknown-type>");
        break;
    }
}

void
register_value_print(void *fh, RegisterValue *v)
{
    switch (v->type) {
    case REG_TYPE_UINT16:
        r_fprintf(fh, "[%" PRIu16 "; 0x%04" PRIx16 "]",
                  v->value.u16, v->value.u16);
        break;
    case REG_TYPE_SINT16:
        r_fprintf(fh, "%" PRId16, v->value.s16);
        break;
    case REG_TYPE_UINT32:
        r_fprintf(fh, "[%" PRIu32 "; 0x%08" PRIx32 "]",
                  v->value.u32, v->value.u32);
        break;
    case REG_TYPE_SINT32:
        r_fprintf(fh, "%" PRId32, v->value.s32);
        break;
    case REG_TYPE_UINT64:
        r_fprintf(fh, "[%" PRIu64 "; 0x%016" PRIx64 "]",
                  v->value.u64, v->value.u64);
        break;
    case REG_TYPE_SINT64:
        r_fprintf(fh, "%" PRId64, v->value.s64);
        break;
    case REG_TYPE_FLOAT32:
        r_fprintf(fh, "%e", v->value.f32);
        break;
    case REG_TYPE_INVALID:
        r_fprintf(fh, "<value-invalid>");
        break;
    }
}

void
register_init_print(void *fh, const char *prefix, const RegisterInit result)
{
    r_fprintf(fh, "%sRegister Init Code: %s\n", prefix,
              register_initcode_to_string(result.code));
    switch (result.code) {
    case REG_INIT_TABLE_INVALID:
        r_fprintf(fh, "%sBasic Initialisation Error!\n", prefix);
        break;
    case REG_INIT_NO_AREAS:
        r_fprintf(fh, "%sSupplied area table is empty!\n", prefix);
        break;
    case REG_INIT_TOO_MANY_AREAS:
        r_fprintf(fh, "%sSupplied area table has more"
                  " than AREA_HANDLE_MAX entries!\n", prefix);
        break;
    case REG_INIT_AREA_INVALID_ORDER:
        r_fprintf(fh, "%sAddresses in area table is not linear!\n", prefix);
        r_fprintf(fh, "%sFirst offending area: %" PRIu16 "!\n", prefix,
                  result.pos.area);
        break;
    case REG_INIT_AREA_ADDRESS_OVERLAP:
        r_fprintf(fh, "%sAddresses of areas overlap!\n", prefix);
        r_fprintf(fh, "%sFirst offending area: %" PRIu16 "!\n", prefix,
                  result.pos.area);
        break;
    case REG_INIT_TOO_MANY_ENTRIES:
        r_fprintf(fh, "%sSupplied entry table has more"
                  " than REGISTER_HANDLE_MAX entries!\n", prefix);
        break;
    case REG_INIT_ENTRY_INVALID_ORDER:
        r_fprintf(fh, "%sAddresses in entry table is not linear!\n", prefix);
        r_fprintf(fh, "%sFirst offending entry: %" PRIu32 "!\n", prefix,
                  result.pos.entry);
        break;
    case REG_INIT_ENTRY_ADDRESS_OVERLAP:
        r_fprintf(fh, "%sAddresses of entries overlap!\n", prefix);
        r_fprintf(fh, "%sFirst offending entry: %" PRIu32 "!\n", prefix,
                  result.pos.entry);
        break;
    case REG_INIT_ENTRY_IN_MEMORY_HOLE:
        r_fprintf(fh, "%sMemory of entry in unmapped address-space!\n", prefix);
        r_fprintf(fh, "%sFirst offending entry: %" PRIu32 "!\n", prefix,
                  result.pos.entry);
        break;
    case REG_INIT_ENTRY_INVALID_DEFAULT:
        r_fprintf(fh, "%sEntry defines default value that does not validate!\n",
                  prefix);
        r_fprintf(fh, "%sFirst offending entry: %" PRIu32 "!\n", prefix,
                  result.pos.entry);
        break;
    case REG_INIT_SUCCESS:
        r_fprintf(fh, "%sRegister Table Initialisation Successful!\n", prefix);
        break;
    default:
        r_fprintf(fh, "%sUnexpected result code: 0x%04x\n", prefix, result.code);
        r_fprintf(fh, "%sHow did this happen?\n", prefix);
        break;
    }
}

char *
register_accesscode_to_string(RegisterAccessCode code)
{
    if (code > REG_ACCESS_CODE_MAXIDX)
        return "<INVALID-REGISTER-ACCESS-CODE>";

    static char *map[] = {
        IDX2STR(REG_ACCESS_SUCCESS),
        IDX2STR(REG_ACCESS_FAILURE),
        IDX2STR(REG_ACCESS_UNINITIALISED),
        IDX2STR(REG_ACCESS_NOENTRY),
        IDX2STR(REG_ACCESS_RANGE),
        IDX2STR(REG_ACCESS_INVALID),
        IDX2STR(REG_ACCESS_READONLY),
        IDX2STR(REG_ACCESS_IO_ERROR)
    };

    return map[code];
}

char *
register_initcode_to_string(RegisterInitCode code)
{
    if (code > REG_INIT_CODE_MAXIDX)
        return "<INVALID-REGISTER-INIT-CODE>";

    static char *map[] = {
        IDX2STR(REG_INIT_SUCCESS),
        IDX2STR(REG_INIT_TABLE_INVALID),
        IDX2STR(REG_INIT_NO_AREAS),
        IDX2STR(REG_INIT_TOO_MANY_AREAS),
        IDX2STR(REG_INIT_AREA_INVALID_ORDER),
        IDX2STR(REG_INIT_AREA_ADDRESS_OVERLAP),
        IDX2STR(REG_INIT_TOO_MANY_ENTRIES),
        IDX2STR(REG_INIT_ENTRY_INVALID_ORDER),
        IDX2STR(REG_INIT_ENTRY_ADDRESS_OVERLAP),
        IDX2STR(REG_INIT_ENTRY_IN_MEMORY_HOLE),
        IDX2STR(REG_INIT_ENTRY_INVALID_DEFAULT)
    };

    return map[code];
}

char *
register_registertype_to_string(RegisterType type)
{
    if (type > REG_TYPE_MAXIDX)
        return "<INVALID-REGISTER-TYPE>";

    static char *map[] = {
        IDX2STR(REG_TYPE_INVALID),
        IDX2STR(REG_TYPE_UINT16),
        IDX2STR(REG_TYPE_UINT32),
        IDX2STR(REG_TYPE_UINT64),
        IDX2STR(REG_TYPE_SINT16),
        IDX2STR(REG_TYPE_SINT32),
        IDX2STR(REG_TYPE_SINT64),
        IDX2STR(REG_TYPE_FLOAT32)
    };

    return map[type];
}

char *
register_validatortype_to_string(RegisterValidatorType type)
{
    if (type > REGV_TYPE_MAXIDX)
        return "<INVALID-VALIDATOR-TYPE>";

    static char *map[] = {
        IDX2STR(REGV_TYPE_TRIVIAL),
        IDX2STR(REGV_TYPE_MIN),
        IDX2STR(REGV_TYPE_MAX),
        IDX2STR(REGV_TYPE_RANGE),
        IDX2STR(REGV_TYPE_CALLBACK)
    };

    return map[type];
}
