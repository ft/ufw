/*
 * Copyright (c) 2019 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include <c/register-internal.h>
#include <c/register-table.h>
#include <c/register-utilities.h>

#define IDX2STR(x) [x] = #x

void
register_table_print(FILE *fh, const char *prefix, const RegisterTable *t)
{
    const bool is_initialised = BIT_ISSET(t->flags, REG_TF_INITIALISED);
    fprintf(fh, "%sRegisterTable ", prefix);
    if (is_initialised == false) {
        fprintf(fh, "is NOT initialised!\n");
    } else {
        fprintf(fh, "is initialised.\n");
        fprintf(fh, "%s     areas: %" PRIu16 "\n", prefix, t->areas);
        fprintf(fh, "%s   entries: %" PRIu32 "\n", prefix, t->entries);
    }

    fprintf(fh, "%s\n%sList of Areas:\n", prefix, prefix);
    AreaHandle area = 0;
    while ((area < AREA_HANDLE_MAX) && !(is_end_of_areas(t->area + area))) {
        RegisterArea *a = t->area + area;
        fprintf(fh, "%s  AreaID: %" PRIu16 "\n", prefix, area);
        register_area_print(fh, prefix, a);
        area += 1;
    }

    fprintf(fh, "%s\n%sList of Entries:\n", prefix, prefix);
    RegisterHandle reg = 0;
    while ((reg < AREA_HANDLE_MAX) && !(is_end_of_entries(t->entry + reg))) {
        RegisterEntry *e = t->entry + reg;
        fprintf(fh, "%s  RegisterEntryID: %" PRIu32 "\n", prefix, reg);
        register_entry_print(fh, prefix, e);
        reg += 1;
    }
}

void
register_area_print(FILE *fh, const char *prefix, const RegisterArea *a)
{
    fprintf(fh, "%s    Area Start: 0x%08" PRIx32 "\n", prefix, a->base);
    fprintf(fh, "%s    Area Size : 0x%08" PRIx32 "\n", prefix, a->size);
    fprintf(fh, "%s    Area Flags: 0x%08" PRIx32 "\n", prefix,
            (uint32_t)a->flags);
    fprintf(fh, "%s    Area has %s read method.\n", prefix,
            a->read == NULL ? "no" : "a");
    fprintf(fh, "%s    Area has %s write method.\n", prefix,
            a->write == NULL ? "no" : "a");
}

void
register_entry_print(FILE *fh, const char *prefix, const RegisterEntry *e)
{
    fprintf(fh, "%s    Register Name   : %s\n", prefix,
            e->name != NULL ? e->name : "<UNNAMED-REGISTER>");
    fprintf(fh, "%s    Register Type   : %s\n", prefix,
            register_registertype_to_string(e->type));
    fprintf(fh, "%s    Register Flags  : 0x%08" PRIx32 "\n", prefix,
            (uint32_t)e->flags);
    fprintf(fh, "%s    Register Address: 0x%08" PRIx32 "\n", prefix, e->address);
}

void
register_init_print(FILE *fh, const char *prefix, const RegisterInit result)
{
    fprintf(fh, "%sRegister Init Code: %s\n", prefix,
            register_initcode_to_string(result.code));
    switch (result.code) {
    case REG_INIT_TABLE_INVALID:
        fprintf(fh, "%sBasic Initialisation Error!\n", prefix);
        break;
    case REG_INIT_NO_AREAS:
        fprintf(fh, "%sSupplied area table is empty!\n", prefix);
        break;
    case REG_INIT_TOO_MANY_AREAS:
        fprintf(fh, "%sSupplied area table has more"
                " than AREA_HANDLE_MAX entries!\n", prefix);
        break;
    case REG_INIT_AREA_INVALID_ORDER:
        fprintf(fh, "%sAddresses in area table is not linear!\n", prefix);
        fprintf(fh, "%sFirst offending area: %" PRIu16 "!\n", prefix,
                result.pos.area);
        break;
    case REG_INIT_AREA_ADDRESS_OVERLAP:
        fprintf(fh, "%sAddresses of areas overlap!\n", prefix);
        fprintf(fh, "%sFirst offending area: %" PRIu16 "!\n", prefix,
                result.pos.area);
        break;
    case REG_INIT_TOO_MANY_ENTRIES:
        fprintf(fh, "%sSupplied entry table has more"
                " than REGISTER_HANDLE_MAX entries!\n", prefix);
        break;
    case REG_INIT_ENTRY_INVALID_ORDER:
        fprintf(fh, "%sAddresses in entry table is not linear!\n", prefix);
        fprintf(fh, "%sFirst offending entry: %" PRIu32 "!\n", prefix,
                result.pos.entry);
        break;
    case REG_INIT_ENTRY_ADDRESS_OVERLAP:
        fprintf(fh, "%sAddresses of entries overlap!\n", prefix);
        fprintf(fh, "%sFirst offending entry: %" PRIu32 "!\n", prefix,
                result.pos.entry);
        break;
    case REG_INIT_ENTRY_IN_MEMORY_HOLE:
        fprintf(fh, "%sMemory of entry in unmapped address-space!\n", prefix);
        fprintf(fh, "%sFirst offending entry: %" PRIu32 "!\n", prefix,
                result.pos.entry);
        break;
    case REG_INIT_ENTRY_INVALID_DEFAULT:
        fprintf(fh, "%sEntry defines default value that does not validate!\n",
                prefix);
        fprintf(fh, "%sFirst offending entry: %" PRIu32 "!\n", prefix,
                result.pos.entry);
        break;
    case REG_INIT_SUCCESS:
        fprintf(fh, "%sRegister Table Initialisation Successful!\n", prefix);
        break;
    default:
        fprintf(fh, "%sUnexpected result code: 0x%04x\n", prefix, result.code);
        fprintf(fh, "%sHow did this happen?\n", prefix);
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
        IDX2STR(REG_ACCESS_UNINITIALISED),
        IDX2STR(REG_ACCESS_NOENTRY),
        IDX2STR(REG_ACCESS_RANGE),
        IDX2STR(REG_ACCESS_INVALID),
        IDX2STR(REG_ACCESS_READONLY)
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
