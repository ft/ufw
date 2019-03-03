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
typedef uint16_t RegisterHandle;
typedef size_t RegisterAddress;
typedef size_t RegisterOffset;
typedef struct RegisterArea RegisterArea;
typedef struct RegisterEntry RegisterEntry;
typedef struct RegisterValue RegisterValue;

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

typedef bool(*registerSer)(const RegisterValue, RegisterAtom*);
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

/**
 * Register Entry flags
 *
 * The access bits are able to override the access control of the area the
 * entry is situated in. If both _ENABLE and _DISABLE are unset, the entry uses
 * the area's access control.
 *
 * Setting both bits is will cause undefined behaviour.
 *
 * The _LOCK bits can be used to temporatily allow/disallow a form of access.
 * If set, the corresponding form of access is disabled. If it is unset, the
 * normal access control applies.
 */
typedef enum RegisterEntryFlags {
    /** If set, read-access to the corresponding entry is enabled. */
    REG_EF_READ_ENABLE  = (1u << 0u),
    /** If set, read-access to the corresponding entry is disabled. */
    REG_EF_READ_DISABLE  = (1u << 1u),
    /**
     * If set, read-access to the corresponding entry is locked, meaning it
     * will behave as though it cannot be read, regardless of any other access
     * control.
     */
    REG_EF_READ_LOCK  = (1u << 2u),
    /** If set, write-access to the corresponding entry is enabled. */
    REG_EF_WRITE_ENABLE = (1u << 3u),
    /** If set, write-access to the corresponding entry is disabled. */
    REG_EF_WRITE_DISABLE = (1u << 4u),
    /**
     * If set, write-access to the corresponding entry is locked, meaning it
     * will behave as though it cannot be written to, regardless of any other
     * access control.
     */
    REG_EF_WRITE_LOCK = (1u << 5u),
    /** If set, a block access has changed this entry. */
    REG_EF_TOUCHED = (1u << 6u)
} RegisterEntryFlags;

struct RegisterEntry {
    RegisterType type;
    RegisterValueU default_value;
    RegisterAddress address;
    RegisterArea *area;
    RegisterOffset offset;
    RegisterValidator check;
    uint16_t flags;
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
    RegisterHandle entries;
    RegisterEntry *entry;
} RegisterTable;

/* Public API prototypes */

RegisterAccessResult reg_mem_read(const RegisterArea*, RegisterAtom*,
                                  RegisterOffset, size_t);
RegisterAccessResult reg_mem_write(RegisterArea*, const RegisterAtom*,
                                   RegisterOffset, size_t);

RegisterAccessResult register_set(RegisterTable*, RegisterHandle,
                                  const RegisterValue);
RegisterAccessResult register_get(RegisterTable*, RegisterHandle,
                                  RegisterValue*);
RegisterAccessResult register_default(RegisterTable*, RegisterHandle,
                                      RegisterValue*);
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
register_was_touched(RegisterTable *t, RegisterHandle reg)
{
    const bool rc = BIT_ISSET(t->entry[reg].flags, REG_EF_TOUCHED);
    BIT_CLEAR(t->entry[reg].flags, REG_EF_TOUCHED);
    return rc;
}

static inline void
register_read_lock(RegisterTable *t, RegisterHandle reg)
{
    BIT_SET(t->entry[reg].flags, REG_EF_READ_LOCK);
}

static inline void
register_read_unlock(RegisterTable *t, RegisterHandle reg)
{
    BIT_CLEAR(t->entry[reg].flags, REG_EF_READ_LOCK);
}

static inline void
register_write_lock(RegisterTable *t, RegisterHandle reg)
{
    BIT_SET(t->entry[reg].flags, REG_EF_WRITE_LOCK);
}

static inline void
register_write_unlock(RegisterTable *t, RegisterHandle reg)
{
    BIT_CLEAR(t->entry[reg].flags, REG_EF_WRITE_LOCK);
}

#endif /* INC_REGISTER_TABLE_H */
