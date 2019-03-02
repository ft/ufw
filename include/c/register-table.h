/*
 * Copyright (c) 2019 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_REGISTER_TABLE_H
#define INC_REGISTER_TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <common/bit-operations.h>

/* Data types */

typedef uint16_t RegisterAtom;
typedef uint16_t RegisterState;
typedef size_t RegisterAddress;
typedef size_t RegisterOffset;
typedef struct RegisterArea RegisterArea;
typedef struct RegisterEntry RegisterEntry;
typedef struct RegisterValue RegisterValue;

typedef enum RegisterAccessBits {
    REG_ACCESS_BIT_READ  = (1u << 0u),
    REG_ACCESS_BIT_WRITE = (1u << 1u)
} RegisterAccessBits;

typedef enum RegisterAccessCode {
    REG_ACCESS_SUCCESS,
    REG_ACCESS_NOENTRY,
    REG_ACCESS_INVALID,
    REG_ACCESS_READONLY
} RegisterAccessCode;

typedef struct RegisterAccessResult {
    RegisterAccessCode code;
    RegisterAddress address;
} RegisterAccessResult;

#define REG_ACCESS_RESULT_INIT { REG_ACCESS_SUCCESS, 0u }

typedef bool(*registerSer)(const RegisterValue*, RegisterAtom*);
typedef bool(*registerDes)(const RegisterAtom*, RegisterValue*);
typedef bool(*validatorFunction)(const RegisterEntry*, RegisterValue);

typedef RegisterAccessResult(*registerRead)(
    const RegisterArea*, RegisterAtom*, RegisterOffset, size_t);
typedef RegisterAccessResult(*registerWrite)(
    RegisterArea*, const RegisterAtom*, RegisterOffset, size_t);

typedef struct RegisterString {
    char *data;
    size_t size;
} RegisterString;

typedef enum RegisterType {
    REG_TYPE_INVALID = 0u,
    REG_TYPE_UINT16,
    REG_TYPE_UINT32,
    REG_TYPE_UINT64,
    REG_TYPE_SINT16,
    REG_TYPE_SINT32,
    REG_TYPE_SINT64,
    REG_TYPE_FLOAT32,
    REG_TYPE_STRING
} RegisterType;

#define REG_ATOM_BIGGEST_DATUM (sizeof(uint64_t) / sizeof(RegisterAtom))

typedef union RegisterValueU {
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int16_t s16;
    int32_t s32;
    int64_t s64;
    float f32;
    RegisterString string;
} RegisterValueU;

struct RegisterValue {
    RegisterType type;
    RegisterValueU value;
};

typedef struct RegisterSerDes {
    registerSer ser;
    registerDes des;
    size_t size;
} RegisterSerDes;

typedef enum RegisterValidatorType {
    REGV_TYPE_TRIVIAL = 0,
    REGV_TYPE_MIN,
    REGV_TYPE_MAX,
    REGV_TYPE_RANGE,
    REGV_TYPE_CALLBACK
} RegisterValidatorType;

typedef struct RegisterValidator {
    RegisterValidatorType type;
    union {
        RegisterValueU min;
        RegisterValueU max;
        struct {
            RegisterValueU min;
            RegisterValueU max;
        } range;
        validatorFunction cb;
    } arg;
} RegisterValidator;

#define REGV_INIT { .type = REGV_TYPE_TRIVIAL }

typedef enum RegisterStateBits {
    REG_STATE_TOUCHED  = (1u << 0u)
} RegisterStateBits;

struct RegisterEntry {
    RegisterType type;
    RegisterValueU default_value;
    RegisterAddress address;
    RegisterArea *area;
    RegisterOffset offset;
    RegisterValidator check;
    RegisterState state;
};

#define REGISTER_ENTRY_END                              \
    { .type = REG_TYPE_INVALID, .default_value.u16 = 0, \
      .address = 0, .area = NULL, .offset = 0,          \
      .check.type = REGV_TYPE_TRIVIAL }

typedef enum RegisterAreaFlags {
    REG_AF_READABLE = (1u << 0u),
    REG_AF_WRITEABLE = (1u << 1u)
} RegisterAreaFlags;

#define REG_AF_RW (REG_AF_READABLE | REG_AF_WRITEABLE)

struct RegisterArea {
    registerRead read;
    registerWrite write;
    uint16_t flags;
    size_t base;
    size_t size;
    RegisterAtom *mem;
};

#define REGISTER_AREA_END \
    { .read = NULL, .write = NULL, .base = 0, .size = 0, .mem = NULL }

typedef struct RegisterTable {
    size_t areas;
    RegisterArea *area;
    size_t entries;
    RegisterEntry *entry;
} RegisterTable;

/* Public API prototypes */

RegisterAccessResult reg_mem_read(const RegisterArea*, RegisterAtom*,
                                  RegisterOffset, size_t);
RegisterAccessResult reg_mem_write(RegisterArea*, const RegisterAtom*,
                                   RegisterOffset, size_t);

RegisterAccessResult register_set(RegisterTable*, size_t, RegisterValue*);
RegisterAccessResult register_get(RegisterTable*, size_t, RegisterValue*);
RegisterAccessResult register_default(RegisterTable*, size_t, RegisterValue*);
RegisterAccessResult register_block_read(RegisterTable*, RegisterAddress,
                                         size_t, RegisterAtom*);
RegisterAccessResult register_block_write(RegisterTable*, RegisterAddress,
                                          size_t, RegisterAtom*);

void register_block_read_unsafe(RegisterTable*, RegisterAddress, size_t,
                                RegisterAtom*);
void register_block_write_unsafe(RegisterTable*, RegisterAddress, size_t,
                                 RegisterAtom*);

RegisterAccessResult register_block_touches_hole(RegisterTable*,
                                                 RegisterAddress,
                                                 size_t);

static inline bool
register_area_is_writable(RegisterArea *a)
{
    return ((a->write != NULL) && (BIT_ISSET(a->flags, REG_AF_WRITEABLE)));
}

#endif /* INC_REGISTER_TABLE_H */
