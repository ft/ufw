/*
 * Copyright (c) 2019-2020 micro framework workers, All rights reserved.
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
    REG_ACCESS_FAILURE,
    REG_ACCESS_UNINITIALISED,
    REG_ACCESS_NOENTRY,
    REG_ACCESS_RANGE,
    REG_ACCESS_INVALID,
    REG_ACCESS_READONLY,
    REG_ACCESS_IO_ERROR,
} RegisterAccessCode;

typedef struct RegisterAccess {
    RegisterAccessCode code;
    RegisterAddress address;
} RegisterAccess;

#define REG_ACCESS_RESULT_INIT { .code = REG_ACCESS_SUCCESS, .address = 0u }

typedef enum RegisterInitCode {
    REG_INIT_SUCCESS,
    REG_INIT_TABLE_INVALID,
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
    REG_AF_WRITEABLE = (1u << 1u),
    REG_AF_SKIP_DEFAULTS = (1u << 2u)
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

#define MAKE_CUSTOM_AREA(READ,WRITE,ADDR,SIZE,FLAGS)  \
    { .read = READ,                                   \
      .write = WRITE,                                 \
      .flags = FLAGS,                                 \
      .base = ADDR,                                   \
      .size = SIZE,                                   \
      .mem = NULL }

#define CUSTOM_AREA(R,W,A,S) MAKE_CUSTOM_AREA(R,W,A,S,REG_AF_RW)
#define CUSTOM_AREA_RO(R,A,S) MAKE_CUSTOM_AREA(R,NULL,A,S,REG_AF_READABLE)
#define CUSTOM_AREA_WO(W,A,S) MAKE_CUSTOM_AREA(NULL,W,S,REG_AF_WRITEABLE)

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
              .address = ADDR, .name = #IDX,                \
              .check.type = REGV_TYPE_TRIVIAL }

#define MAKE_MIN_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT,MIN) \
    [IDX] = { .type = TYPE,                                 \
              .default_value.MEMBER = DEFAULT,              \
              .address = ADDR, .name = #IDX,                \
              .check.type = REGV_TYPE_MIN,                  \
              .check.arg.min.MEMBER = MIN }

#define MAKE_MAX_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT,MAX) \
    [IDX] = { .type = TYPE,                                 \
              .default_value.MEMBER = DEFAULT,              \
              .address = ADDR, .name = #IDX,                \
              .check.type = REGV_TYPE_MAX,                  \
              .check.arg.max.MEMBER = MAX }

#define MAKE_RANGE_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT,MIN,MAX)       \
    [IDX] = { .type = TYPE,                                             \
              .default_value.MEMBER = DEFAULT,                          \
              .address = ADDR, .name = #IDX,                            \
              .check.type = REGV_TYPE_RANGE,                            \
              .check.arg.range.min.MEMBER = MIN,                        \
              .check.arg.range.max.MEMBER = MAX }

#define MAKE_VALIDATOR_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT,FNC)       \
    [IDX] = { .type = TYPE,                                             \
              .default_value.MEMBER = DEFAULT,                          \
              .address = ADDR, .name = #IDX,                            \
              .check.type = REGV_TYPE_CALLBACK,                         \
              .check.arg.cb = FNC }

#define REG_U16(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_UINT16,u16,D)
#define REG_U16MIN(I,A,MIN,D) MAKE_MIN_REGISTER(I,A,REG_TYPE_UINT16,u16,D,MIN)
#define REG_U16MAX(I,A,MAX,D) MAKE_MAX_REGISTER(I,A,REG_TYPE_UINT16,u16,D,MAX)
#define REG_U16RANGE(I,A,MIN,MAX,D) \
    MAKE_RANGE_REGISTER(I,A,REG_TYPE_UINT16,u16,D,MIN,MAX)
#define REG_U16FNC(I,A,FNC,D) \
    MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_UINT16,u16,D,FNC)

#define REG_U32(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_UINT32,u32,D)
#define REG_U32MIN(I,A,MIN,D) MAKE_MIN_REGISTER(I,A,REG_TYPE_UINT32,u32,D,MIN)
#define REG_U32MAX(I,A,MAX,D) MAKE_MAX_REGISTER(I,A,REG_TYPE_UINT32,u32,D,MAX)
#define REG_U32RANGE(I,A,MIN,MAX,D) \
    MAKE_RANGE_REGISTER(I,A,REG_TYPE_UINT32,u32,D,MIN,MAX)
#define REG_U32FNC(I,A,FNC,D) \
    MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_UINT32,u32,D,FNC)

#define REG_U64(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_UINT64,u64,D)
#define REG_U64MIN(I,A,MIN,D) MAKE_MIN_REGISTER(I,A,REG_TYPE_UINT64,u64,D,MIN)
#define REG_U64MAX(I,A,MAX,D) MAKE_MAX_REGISTER(I,A,REG_TYPE_UINT64,u64,D,MAX)
#define REG_U64RANGE(I,A,MIN,MAX,D) \
    MAKE_RANGE_REGISTER(I,A,REG_TYPE_UINT64,u64,D,MIN,MAX)
#define REG_U64FNC(I,A,FNC,D) \
    MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_UINT64,u64,D,FNC)

#define REG_S16(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_SINT16,s16,D)
#define REG_S16MIN(I,A,MIN,D) MAKE_MIN_REGISTER(I,A,REG_TYPE_SINT16,s16,D,MIN)
#define REG_S16MAX(I,A,MAX,D) MAKE_MAX_REGISTER(I,A,REG_TYPE_SINT16,s16,D,MAX)
#define REG_S16RANGE(I,A,MIN,MAX,D) \
    MAKE_RANGE_REGISTER(I,A,REG_TYPE_SINT16,s16,D,MIN,MAX)
#define REG_S16FNC(I,A,FNC,D) \
    MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_SINT16,s16,D,FNC)

#define REG_S32(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_SINT32,s32,D)
#define REG_S32MIN(I,A,MIN,D) MAKE_MIN_REGISTER(I,A,REG_TYPE_SINT32,s32,D,MIN)
#define REG_S32MAX(I,A,MAX,D) MAKE_MAX_REGISTER(I,A,REG_TYPE_SINT32,s32,D,MAX)
#define REG_S32RANGE(I,A,MIN,MAX,D) \
    MAKE_RANGE_REGISTER(I,A,REG_TYPE_SINT32,s32,D,MIN,MAX)
#define REG_S32FNC(I,A,FNC,D) \
    MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_SINT32,s32,D,FNC)

#define REG_S64(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_SINT64,s64,D)
#define REG_S64MIN(I,A,MIN,D) MAKE_MIN_REGISTER(I,A,REG_TYPE_SINT64,s64,D,MIN)
#define REG_S64MAX(I,A,MAX,D) MAKE_MAX_REGISTER(I,A,REG_TYPE_SINT64,s64,D,MAX)
#define REG_S64RANGE(I,A,MIN,MAX,D) \
    MAKE_RANGE_REGISTER(I,A,REG_TYPE_SINT64,s64,D,MIN,MAX)
#define REG_S64FNC(I,A,FNC,D) \
    MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_SINT64,s64,D,FNC)

#define REG_F32(I,A,D) MAKE_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D)
#define REG_F32MIN(I,A,MIN,D) MAKE_MIN_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D,MIN)
#define REG_F32MAX(I,A,MAX,D) MAKE_MAX_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D,MAX)
#define REG_F32RANGE(I,A,MIN,MAX,D) \
    MAKE_RANGE_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D,MIN,MAX)
#define REG_F32FNC(I,A,FNC,D) \
    MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D,FNC)

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

RegisterAccess register_block_read_unsafe(RegisterTable*, RegisterAddress,
                                          RegisterOffset, RegisterAtom*);
RegisterAccess register_block_write_unsafe(RegisterTable*, RegisterAddress,
                                           RegisterOffset, RegisterAtom*);

RegisterAccess register_block_touches_hole(RegisterTable*,
                                           RegisterAddress,
                                           RegisterOffset);
RegisterAccess register_set_from_hexstr(RegisterTable*,
                                        RegisterAddress,
                                        const char*, size_t);

RegisterAccess register_mcopy(RegisterTable*, AreaHandle, AreaHandle);

static inline RegisterArea*
register_area(RegisterTable *t, RegisterHandle reg)
{
    return t->entry[reg].area;
}

static inline RegisterOffset
register_offset(RegisterTable *t, RegisterHandle reg)
{
    return t->entry[reg].offset;
}

static inline char*
register_name(RegisterTable *t, RegisterHandle reg)
{
    return t->entry[reg].name;
}

static inline void
register_touch(RegisterTable *t, RegisterHandle reg)
{
    BIT_SET(t->entry[reg].flags, REG_EF_TOUCHED);
}

static inline void
register_untouch(RegisterTable *t, RegisterHandle reg)
{
    BIT_CLEAR(t->entry[reg].flags, REG_EF_TOUCHED);
}

static inline bool
register_was_touched(RegisterTable *t, RegisterHandle reg)
{
    return BIT_ISSET(t->entry[reg].flags, REG_EF_TOUCHED);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_REGISTER_TABLE_H */
