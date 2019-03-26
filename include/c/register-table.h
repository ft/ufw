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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Data types */

typedef uint16_t AreaHandle;
#define AREA_HANDLE_MAX UINT16_MAX

typedef uint32_t RegisterHandle;
#define REGISTER_HANDLE_MAX UINT32_MAX

typedef uint16_t RegisterAtom;
#define REGISTER_ATOM_MAX UINT16_MAX

typedef uint32_t RegisterAddress;
#define REGISTER_ADDRESS_MAX UINT32_MAX

typedef uint32_t RegisterOffset;
#define REGISTER_OFFSET_MAX UINT32_MAX

/* A couple of forward declarations */
typedef struct RegisterArea RegisterArea;
typedef struct RegisterEntry RegisterEntry;
typedef struct RegisterValue RegisterValue;

typedef enum RegisterAccessCode {
    REG_ACCESS_SUCCESS,
    REG_ACCESS_UNINITIALISED,
    REG_ACCESS_NOENTRY,
    REG_ACCESS_INVALID,
    REG_ACCESS_READONLY
} RegisterAccessCode;

typedef struct RegisterAccess {
    RegisterAccessCode code;
    RegisterAddress address;
} RegisterAccess;

#define REG_ACCESS_RESULT_INIT { .code = REG_ACCESS_SUCCESS, .address = 0u }

typedef enum RegisterInitCode {
    REG_INIT_SUCCESS,
    REG_INIT_NO_AREAS,
    REG_INIT_TOO_MANY_AREAS,
    REG_INIT_AREA_INVALID_ORDER,
    REG_INIT_AREA_ADDRESS_OVERLAP,
    REG_INIT_TOO_MANY_ENTRIES,
    REG_INIT_ENTRY_INVALID_ORDER,
    REG_INIT_ENTRY_ADDRESS_OVERLAP,
    REG_INIT_ENTRY_IN_MEMORY_HOLE,
    REG_INIT_ENTRY_INVALID_DEFAULT
} RegisterInitCode;

typedef struct RegisterInit {
    RegisterInitCode code;
    union {
        AreaHandle area;
        RegisterHandle entry;
        RegisterAddress address;
    } pos;
} RegisterInit;

#define REG_INIT_RESULT_INIT { .code = REG_INIT_SUCCESS, .pos.address = 0u }

typedef bool(*registerSer)(const RegisterValue, RegisterAtom*);
typedef bool(*registerDes)(const RegisterAtom*, RegisterValue*);
typedef bool(*validatorFunction)(const RegisterEntry*, RegisterValue);

typedef RegisterAccess(*registerRead)(
    const RegisterArea*, RegisterAtom*, RegisterOffset, RegisterOffset);
typedef RegisterAccess(*registerWrite)(
    RegisterArea*, const RegisterAtom*, RegisterOffset, RegisterOffset);

typedef enum RegisterType {
    REG_TYPE_INVALID = 0u,
    REG_TYPE_UINT16,
    REG_TYPE_UINT32,
    REG_TYPE_UINT64,
    REG_TYPE_SINT16,
    REG_TYPE_SINT32,
    REG_TYPE_SINT64,
    REG_TYPE_FLOAT32
} RegisterType;

#define REG_LARGEST_DATUM uint64_t
#define REG_SIZEOF_LARGEST_DATUM (sizeof(REG_LARGEST_DATUM) / sizeof(RegisterAtom))

typedef union RegisterValueU {
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int16_t s16;
    int32_t s32;
    int64_t s64;
    float f32;
} RegisterValueU;

struct RegisterValue {
    RegisterType type;
    RegisterValueU value;
};

typedef struct RegisterSerDes {
    registerSer ser;
    registerDes des;
    RegisterOffset size;
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

typedef enum RegisterEntryFlags {
    REG_EF_TOUCHED = (1u << 0u)
} RegisterEntryFlags;

struct RegisterEntry {
    RegisterType type;
    RegisterValueU default_value;
    RegisterAddress address;
    RegisterArea *area;
    RegisterOffset offset;
    RegisterValidator check;
    char *name;
    uint16_t flags;
};

#define REGISTER_ENTRY_END                              \
    { .type = REG_TYPE_INVALID, .default_value.u16 = 0, \
      .address = 0, .area = NULL, .offset = 0,          \
      .check.type = REGV_TYPE_TRIVIAL, .name = NULL, .flags = 0 }

typedef enum RegisterAreaFlags {
    REG_AF_READABLE = (1u << 0u),
    REG_AF_WRITEABLE = (1u << 1u)
} RegisterAreaFlags;

#define REG_AF_RW (REG_AF_READABLE | REG_AF_WRITEABLE)

struct RegisterArea {
    registerRead read;
    registerWrite write;
    uint16_t flags;
    RegisterAddress base;
    RegisterOffset size;
    struct {
        RegisterHandle first;
        RegisterHandle last;
        RegisterOffset count;
    } entry;
    RegisterAtom *mem;
};

#define REGISTER_AREA_END                                   \
    { .read = NULL, .write = NULL,                          \
      .flags = 0,                                           \
      .base = 0, .size = 0,                                 \
      .entry.first = 0, .entry.last = 0, .entry.count = 0,  \
      .mem = NULL }

typedef enum RegisterTableFlags {
    REG_TF_INITIALISED = (1u << 0u)
} RegisterTableFlags;

typedef struct RegisterTable {
    uint16_t flags;
    AreaHandle areas;
    RegisterArea *area;
    RegisterHandle entries;
    RegisterEntry *entry;
} RegisterTable;

/* Public API prototypes */

#define MAKE_MEMORY_AREA(ADDR,SIZE,FLAGS)   \
    { .read = reg_mem_read,                 \
      .write = reg_mem_write,               \
      .flags = FLAGS,                       \
      .base = ADDR,                         \
      .size = SIZE,                         \
      .mem = (RegisterAtom[SIZE]) { 0 } }

#define MEMORY_AREA(A,S) MAKE_MEMORY_AREA(A,S,REG_AF_RW)
#define MEMORY_AREA_RO(A,S) MAKE_MEMORY_AREA(A,S,REG_AF_READABLE)
#define MEMORY_AREA_WO(A,S) MAKE_MEMORY_AREA(A,S,REG_AF_WRITEABLE)

#define MAKE_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT)         \
    [IDX] = { .type = TYPE,                                 \
              .default_value.MEMBER = DEFAULT,              \
              .address = ADDR, .name = #IDX }

#define REG_U16(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_UINT16,u16,D)
#define REG_U32(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_UINT32,u32,D)
#define REG_U64(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_UINT64,u64,D)
#define REG_S16(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_SINT16,s16,D)
#define REG_S32(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_SINT32,s32,D)
#define REG_S64(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_SINT64,s64,D)
#define REG_F32(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D)

RegisterInit register_init(RegisterTable*);

RegisterAccess reg_mem_read(const RegisterArea*, RegisterAtom*,
                            RegisterOffset, RegisterOffset);
RegisterAccess reg_mem_write(RegisterArea*, const RegisterAtom*,
                             RegisterOffset, RegisterOffset);

RegisterAccess register_set(RegisterTable*, RegisterHandle,
                            const RegisterValue);
RegisterAccess register_get(RegisterTable*, RegisterHandle,
                            RegisterValue*);
RegisterAccess register_default(RegisterTable*, RegisterHandle,
                                RegisterValue*);
RegisterAccess register_block_read(RegisterTable*, RegisterAddress,
                                   RegisterOffset, RegisterAtom*);
RegisterAccess register_block_write(RegisterTable*, RegisterAddress,
                                    RegisterOffset, RegisterAtom*);

void register_block_read_unsafe(RegisterTable*, RegisterAddress,
                                RegisterOffset, RegisterAtom*);
void register_block_write_unsafe(RegisterTable*, RegisterAddress,
                                 RegisterOffset, RegisterAtom*);

RegisterAccess register_block_touches_hole(RegisterTable*,
                                           RegisterAddress,
                                           RegisterOffset);

static inline bool
register_was_touched(RegisterTable *t, RegisterHandle reg)
{
    const bool rc = BIT_ISSET(t->entry[reg].flags, REG_EF_TOUCHED);
    BIT_CLEAR(t->entry[reg].flags, REG_EF_TOUCHED);
    return rc;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_REGISTER_TABLE_H */