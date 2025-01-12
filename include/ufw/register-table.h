/*
 * Copyright (c) 2019-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_REGISTER_TABLE_H
#define INC_UFW_REGISTER_TABLE_H

/**
 * @addtogroup registers Register Table
 *
 * Featureful register table implementation
 *
 * @{
 *
 * @file ufw/register-table.h
 * @brief Register Table API
 *
 * @}
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ufw/bit-operations.h>

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

typedef bool(*registerSer)(const RegisterValue, RegisterAtom*, bool);
typedef bool(*registerDes)(const RegisterAtom*, RegisterValue*, bool);
typedef bool(*validatorFunction)(const RegisterEntry*, RegisterValue);

typedef RegisterAccess(*registerRead)(
    const RegisterArea*, RegisterAtom*, RegisterOffset, RegisterOffset);
typedef RegisterAccess(*registerWrite)(
    RegisterArea*, const RegisterAtom*, RegisterOffset, RegisterOffset);

typedef enum RegisterType {
    REG_TYPE_UINT16,
    REG_TYPE_UINT32,
    REG_TYPE_UINT64,
    REG_TYPE_SINT16,
    REG_TYPE_SINT32,
    REG_TYPE_SINT64,
    REG_TYPE_FLOAT32,
    REG_TYPE_FLOAT64,
    REG_TYPE_INVALID
} RegisterType;

typedef union RegisterValueU {
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int16_t s16;
    int32_t s32;
    int64_t s64;
    float f32;
    double f64;
} RegisterValueU;

struct RegisterValue {
    RegisterType type;
    RegisterValueU value;
};

typedef struct RegisterSerDes {
    registerSer ser;
    registerDes des;
} RegisterSerDes;

typedef enum RegisterValidatorType {
    REGV_TYPE_TRIVIAL = 0,
    REGV_TYPE_FAIL,
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
    REG_EF_TOUCHED = (1U << 0U)
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
    void *user;
};

#define REGISTER_ENTRY_END                              \
    { .type = REG_TYPE_INVALID, .default_value.u16 = 0, \
      .address = 0, .area = NULL, .offset = 0,          \
      .check.type = REGV_TYPE_TRIVIAL, .name = NULL,    \
      .flags = 0, .user = NULL }

typedef enum RegisterAreaFlags {
    REG_AF_READABLE      = (1U << 0U),
    REG_AF_WRITEABLE     = (1U << 1U),
    REG_AF_SKIP_DEFAULTS = (1U << 2U)
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
#ifdef REGISTER_TABLE_WITH_AREA_USER_DATA
    void *user;
#endif /* REGISTER_TABLE_WITH_AREA_USER_DATA */
};

#define REGISTER_AREA_END                                   \
    { .read = NULL, .write = NULL,                          \
      .flags = 0,                                           \
      .base = 0, .size = 0,                                 \
      .entry.first = 0, .entry.last = 0, .entry.count = 0,  \
      .mem = NULL }

typedef enum RegisterTableFlags {
    REG_TF_INITIALISED = (1U << 0U),
    REG_TF_DURING_INIT = (1U << 1U),
    REG_TF_BIG_ENDIAN  = (1U << 2U)
} RegisterTableFlags;

typedef struct RegisterTable {
    uint16_t flags;
    AreaHandle areas;
    RegisterArea *area;
    RegisterHandle entries;
    RegisterEntry *entry;
} RegisterTable;

typedef int(*registerCallback)(RegisterTable*, RegisterHandle, void*);

/* Public API Macros and Functions */

/* Area Macros */

#define MAKE_CUSTOM_AREA(READ,WRITE,ADDR,SIZE,FLAGS)  \
    { .read  = (READ),                                \
      .write = (WRITE),                               \
      .flags = (FLAGS),                               \
      .base  = (ADDR),                                \
      .size  = (SIZE),                                \
      .mem   = NULL }

#define CUSTOM_AREA(R,W,A,S) MAKE_CUSTOM_AREA(R,W,A,S,REG_AF_RW)
#define CUSTOM_AREA_RO(R,A,S) MAKE_CUSTOM_AREA(R,NULL,A,S,REG_AF_READABLE)
#define CUSTOM_AREA_WO(W,A,S) MAKE_CUSTOM_AREA(NULL,W,S,REG_AF_WRITEABLE)

#define MAKE_MEMORY_AREA(ADDR,SIZE,FLAGS)   \
    { .read  = reg_mem_read,                \
      .write = reg_mem_write,               \
      .flags = (FLAGS),                     \
      .base  = (ADDR),                      \
      .size  = (SIZE),                      \
      .mem   = (RegisterAtom[SIZE]) { 0 } }

#define MEMORY_AREA(A,S) MAKE_MEMORY_AREA(A,S,REG_AF_RW)
#define MEMORY_AREA_RO(A,S) MAKE_MEMORY_AREA(A,S,REG_AF_READABLE)
#define MEMORY_AREA_WO(A,S) MAKE_MEMORY_AREA(A,S,REG_AF_WRITEABLE)

/* Entry Macros */

#ifdef REGISTER_TABLE_WITH_NAMES
#define REGCOMMON(IDX,ADDR,TYPE,MEMBER,DEFAULT) \
    .type = TYPE,                               \
    .default_value.MEMBER = DEFAULT,            \
    .address = ADDR,                            \
    .name = #IDX
#else
#define REGCOMMON(IDX,ADDR,TYPE,MEMBER,DEFAULT) \
    .type = (TYPE),                             \
    .default_value.MEMBER = (DEFAULT),          \
    .address = (ADDR),                          \
    .name = NULL
#endif

#define MAKE_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,USR)        \
    [IDX] = { REGCOMMON(IDX,ADDR,TYPE,MEMBER,DEFAULT),          \
              .check.type = REGV_TYPE_TRIVIAL,                  \
              .user = (USR) }

#define MAKE_FAIL_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,USR)   \
    [IDX] = { REGCOMMON(IDX,ADDR,TYPE,MEMBER,DEFAULT),          \
              .check.type = REGV_TYPE_FAIL,                     \
              .user = (USR) }

#define MAKE_MIN_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,MIN,USR)        \
    [IDX] = { REGCOMMON(IDX,ADDR,TYPE,MEMBER,DEFAULT),                  \
              .check.type = REGV_TYPE_MIN,                              \
              .check.arg.min.MEMBER = (MIN),                            \
              .user = (USR) }

#define MAKE_MAX_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,MAX, USR)   \
    [IDX] = { REGCOMMON(IDX,ADDR,TYPE,MEMBER,DEFAULT),              \
              .check.type = REGV_TYPE_MAX,                          \
              .check.arg.max.MEMBER = (MAX),                        \
              .user = (USR) }

#define MAKE_RANGE_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,MIN,MAX,USR)  \
    [IDX] = { REGCOMMON(IDX,ADDR,TYPE,MEMBER,DEFAULT),                  \
              .check.type = REGV_TYPE_RANGE,                            \
              .check.arg.range.min.MEMBER = (MIN),                      \
              .check.arg.range.max.MEMBER = (MAX),                      \
              .user = (USR) }

#define MAKE_VALIDATOR_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,FNC,USR)  \
    [IDX] = { REGCOMMON(IDX,ADDR,TYPE,MEMBER,DEFAULT),                  \
              .check.type = REGV_TYPE_CALLBACK,                         \
              .check.arg.cb = (FNC),                                    \
              .user = (USR) }

#define MAKE_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT)     \
    MAKE_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,NULL)

#define MAKE_FAIL_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT)        \
    MAKE_FAIL_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,NULL)

#define MAKE_MIN_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT,MIN)     \
    MAKE_MIN_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,MIN,NULL)

#define MAKE_MAX_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT,MAX)     \
    MAKE_MAX_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,MAX,NULL)

#define MAKE_RANGE_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT,MIN,MAX)       \
    MAKE_RANGE_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,MIN,MAX,NULL)

#define MAKE_VALIDATOR_REGISTER(IDX,ADDR,TYPE,MEMBER,DEFAULT,FNC)       \
    MAKE_VALIDATOR_REGISTERx(IDX,ADDR,TYPE,MEMBER,DEFAULT,FNC,NULL)

/*
 * Register Specification Front-Ends
 *
 * This code is generated by ‘tools/make-register-macros.scm’, since manual
 * definition is too tedious and error prone. For this portion, the 80
 * characters style rule is suspended.
 */

#define REG_U16(I,A,D)                 MAKE_REGISTER(I,A,REG_TYPE_UINT16,u16,D)
#define REGx_U16(I,A,D,U)              MAKE_REGISTERx(I,A,REG_TYPE_UINT16,u16,D,U)
#define REG_U16FAIL(I,A,D)             MAKE_FAIL_REGISTER(I,A,REG_TYPE_UINT16,u16,D)
#define REGx_U16FAIL(I,A,D,U)          MAKE_FAIL_REGISTERx(I,A,REG_TYPE_UINT16,u16,D,U)
#define REG_U16MIN(I,A,MIN,D)          MAKE_MIN_REGISTER(I,A,REG_TYPE_UINT16,u16,D,MIN)
#define REGx_U16MIN(I,A,MIN,D,U)       MAKE_MIN_REGISTERx(I,A,REG_TYPE_UINT16,u16,D,MIN,U)
#define REG_U16MAX(I,A,MAX,D)          MAKE_MAX_REGISTER(I,A,REG_TYPE_UINT16,u16,D,MAX)
#define REGx_U16MAX(I,A,MAX,D,U)       MAKE_MAX_REGISTERx(I,A,REG_TYPE_UINT16,u16,D,MAX,U)
#define REG_U16RANGE(I,A,MIN,MAX,D)    MAKE_RANGE_REGISTER(I,A,REG_TYPE_UINT16,u16,D,MIN,MAX)
#define REGx_U16RANGE(I,A,MIN,MAX,D,U) MAKE_RANGE_REGISTERx(I,A,REG_TYPE_UINT16,u16,D,MIN,MAX,U)
#define REG_U16FNC(I,A,FNC,D)          MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_UINT16,u16,D,FNC)
#define REGx_U16FNC(I,A,FNC,D,U)       MAKE_VALIDATOR_REGISTERx(I,A,REG_TYPE_UINT16,u16,D,FNC,U)
#define REG_U32(I,A,D)                 MAKE_REGISTER(I,A,REG_TYPE_UINT32,u32,D)
#define REGx_U32(I,A,D,U)              MAKE_REGISTERx(I,A,REG_TYPE_UINT32,u32,D,U)
#define REG_U32FAIL(I,A,D)             MAKE_FAIL_REGISTER(I,A,REG_TYPE_UINT32,u32,D)
#define REGx_U32FAIL(I,A,D,U)          MAKE_FAIL_REGISTERx(I,A,REG_TYPE_UINT32,u32,D,U)
#define REG_U32MIN(I,A,MIN,D)          MAKE_MIN_REGISTER(I,A,REG_TYPE_UINT32,u32,D,MIN)
#define REGx_U32MIN(I,A,MIN,D,U)       MAKE_MIN_REGISTERx(I,A,REG_TYPE_UINT32,u32,D,MIN,U)
#define REG_U32MAX(I,A,MAX,D)          MAKE_MAX_REGISTER(I,A,REG_TYPE_UINT32,u32,D,MAX)
#define REGx_U32MAX(I,A,MAX,D,U)       MAKE_MAX_REGISTERx(I,A,REG_TYPE_UINT32,u32,D,MAX,U)
#define REG_U32RANGE(I,A,MIN,MAX,D)    MAKE_RANGE_REGISTER(I,A,REG_TYPE_UINT32,u32,D,MIN,MAX)
#define REGx_U32RANGE(I,A,MIN,MAX,D,U) MAKE_RANGE_REGISTERx(I,A,REG_TYPE_UINT32,u32,D,MIN,MAX,U)
#define REG_U32FNC(I,A,FNC,D)          MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_UINT32,u32,D,FNC)
#define REGx_U32FNC(I,A,FNC,D,U)       MAKE_VALIDATOR_REGISTERx(I,A,REG_TYPE_UINT32,u32,D,FNC,U)
#define REG_U64(I,A,D)                 MAKE_REGISTER(I,A,REG_TYPE_UINT64,u64,D)
#define REGx_U64(I,A,D,U)              MAKE_REGISTERx(I,A,REG_TYPE_UINT64,u64,D,U)
#define REG_U64FAIL(I,A,D)             MAKE_FAIL_REGISTER(I,A,REG_TYPE_UINT64,u64,D)
#define REGx_U64FAIL(I,A,D,U)          MAKE_FAIL_REGISTERx(I,A,REG_TYPE_UINT64,u64,D,U)
#define REG_U64MIN(I,A,MIN,D)          MAKE_MIN_REGISTER(I,A,REG_TYPE_UINT64,u64,D,MIN)
#define REGx_U64MIN(I,A,MIN,D,U)       MAKE_MIN_REGISTERx(I,A,REG_TYPE_UINT64,u64,D,MIN,U)
#define REG_U64MAX(I,A,MAX,D)          MAKE_MAX_REGISTER(I,A,REG_TYPE_UINT64,u64,D,MAX)
#define REGx_U64MAX(I,A,MAX,D,U)       MAKE_MAX_REGISTERx(I,A,REG_TYPE_UINT64,u64,D,MAX,U)
#define REG_U64RANGE(I,A,MIN,MAX,D)    MAKE_RANGE_REGISTER(I,A,REG_TYPE_UINT64,u64,D,MIN,MAX)
#define REGx_U64RANGE(I,A,MIN,MAX,D,U) MAKE_RANGE_REGISTERx(I,A,REG_TYPE_UINT64,u64,D,MIN,MAX,U)
#define REG_U64FNC(I,A,FNC,D)          MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_UINT64,u64,D,FNC)
#define REGx_U64FNC(I,A,FNC,D,U)       MAKE_VALIDATOR_REGISTERx(I,A,REG_TYPE_UINT64,u64,D,FNC,U)
#define REG_S16(I,A,D)                 MAKE_REGISTER(I,A,REG_TYPE_SINT16,s16,D)
#define REGx_S16(I,A,D,U)              MAKE_REGISTERx(I,A,REG_TYPE_SINT16,s16,D,U)
#define REG_S16FAIL(I,A,D)             MAKE_FAIL_REGISTER(I,A,REG_TYPE_SINT16,s16,D)
#define REGx_S16FAIL(I,A,D,U)          MAKE_FAIL_REGISTERx(I,A,REG_TYPE_SINT16,s16,D,U)
#define REG_S16MIN(I,A,MIN,D)          MAKE_MIN_REGISTER(I,A,REG_TYPE_SINT16,s16,D,MIN)
#define REGx_S16MIN(I,A,MIN,D,U)       MAKE_MIN_REGISTERx(I,A,REG_TYPE_SINT16,s16,D,MIN,U)
#define REG_S16MAX(I,A,MAX,D)          MAKE_MAX_REGISTER(I,A,REG_TYPE_SINT16,s16,D,MAX)
#define REGx_S16MAX(I,A,MAX,D,U)       MAKE_MAX_REGISTERx(I,A,REG_TYPE_SINT16,s16,D,MAX,U)
#define REG_S16RANGE(I,A,MIN,MAX,D)    MAKE_RANGE_REGISTER(I,A,REG_TYPE_SINT16,s16,D,MIN,MAX)
#define REGx_S16RANGE(I,A,MIN,MAX,D,U) MAKE_RANGE_REGISTERx(I,A,REG_TYPE_SINT16,s16,D,MIN,MAX,U)
#define REG_S16FNC(I,A,FNC,D)          MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_SINT16,s16,D,FNC)
#define REGx_S16FNC(I,A,FNC,D,U)       MAKE_VALIDATOR_REGISTERx(I,A,REG_TYPE_SINT16,s16,D,FNC,U)
#define REG_S32(I,A,D)                 MAKE_REGISTER(I,A,REG_TYPE_SINT32,s32,D)
#define REGx_S32(I,A,D,U)              MAKE_REGISTERx(I,A,REG_TYPE_SINT32,s32,D,U)
#define REG_S32FAIL(I,A,D)             MAKE_FAIL_REGISTER(I,A,REG_TYPE_SINT32,s32,D)
#define REGx_S32FAIL(I,A,D,U)          MAKE_FAIL_REGISTERx(I,A,REG_TYPE_SINT32,s32,D,U)
#define REG_S32MIN(I,A,MIN,D)          MAKE_MIN_REGISTER(I,A,REG_TYPE_SINT32,s32,D,MIN)
#define REGx_S32MIN(I,A,MIN,D,U)       MAKE_MIN_REGISTERx(I,A,REG_TYPE_SINT32,s32,D,MIN,U)
#define REG_S32MAX(I,A,MAX,D)          MAKE_MAX_REGISTER(I,A,REG_TYPE_SINT32,s32,D,MAX)
#define REGx_S32MAX(I,A,MAX,D,U)       MAKE_MAX_REGISTERx(I,A,REG_TYPE_SINT32,s32,D,MAX,U)
#define REG_S32RANGE(I,A,MIN,MAX,D)    MAKE_RANGE_REGISTER(I,A,REG_TYPE_SINT32,s32,D,MIN,MAX)
#define REGx_S32RANGE(I,A,MIN,MAX,D,U) MAKE_RANGE_REGISTERx(I,A,REG_TYPE_SINT32,s32,D,MIN,MAX,U)
#define REG_S32FNC(I,A,FNC,D)          MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_SINT32,s32,D,FNC)
#define REGx_S32FNC(I,A,FNC,D,U)       MAKE_VALIDATOR_REGISTERx(I,A,REG_TYPE_SINT32,s32,D,FNC,U)
#define REG_S64(I,A,D)                 MAKE_REGISTER(I,A,REG_TYPE_SINT64,s64,D)
#define REGx_S64(I,A,D,U)              MAKE_REGISTERx(I,A,REG_TYPE_SINT64,s64,D,U)
#define REG_S64FAIL(I,A,D)             MAKE_FAIL_REGISTER(I,A,REG_TYPE_SINT64,s64,D)
#define REGx_S64FAIL(I,A,D,U)          MAKE_FAIL_REGISTERx(I,A,REG_TYPE_SINT64,s64,D,U)
#define REG_S64MIN(I,A,MIN,D)          MAKE_MIN_REGISTER(I,A,REG_TYPE_SINT64,s64,D,MIN)
#define REGx_S64MIN(I,A,MIN,D,U)       MAKE_MIN_REGISTERx(I,A,REG_TYPE_SINT64,s64,D,MIN,U)
#define REG_S64MAX(I,A,MAX,D)          MAKE_MAX_REGISTER(I,A,REG_TYPE_SINT64,s64,D,MAX)
#define REGx_S64MAX(I,A,MAX,D,U)       MAKE_MAX_REGISTERx(I,A,REG_TYPE_SINT64,s64,D,MAX,U)
#define REG_S64RANGE(I,A,MIN,MAX,D)    MAKE_RANGE_REGISTER(I,A,REG_TYPE_SINT64,s64,D,MIN,MAX)
#define REGx_S64RANGE(I,A,MIN,MAX,D,U) MAKE_RANGE_REGISTERx(I,A,REG_TYPE_SINT64,s64,D,MIN,MAX,U)
#define REG_S64FNC(I,A,FNC,D)          MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_SINT64,s64,D,FNC)
#define REGx_S64FNC(I,A,FNC,D,U)       MAKE_VALIDATOR_REGISTERx(I,A,REG_TYPE_SINT64,s64,D,FNC,U)
#define REG_F32(I,A,D)                 MAKE_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D)
#define REGx_F32(I,A,D,U)              MAKE_REGISTERx(I,A,REG_TYPE_FLOAT32,f32,D,U)
#define REG_F32FAIL(I,A,D)             MAKE_FAIL_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D)
#define REGx_F32FAIL(I,A,D,U)          MAKE_FAIL_REGISTERx(I,A,REG_TYPE_FLOAT32,f32,D,U)
#define REG_F32MIN(I,A,MIN,D)          MAKE_MIN_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D,MIN)
#define REGx_F32MIN(I,A,MIN,D,U)       MAKE_MIN_REGISTERx(I,A,REG_TYPE_FLOAT32,f32,D,MIN,U)
#define REG_F32MAX(I,A,MAX,D)          MAKE_MAX_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D,MAX)
#define REGx_F32MAX(I,A,MAX,D,U)       MAKE_MAX_REGISTERx(I,A,REG_TYPE_FLOAT32,f32,D,MAX,U)
#define REG_F32RANGE(I,A,MIN,MAX,D)    MAKE_RANGE_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D,MIN,MAX)
#define REGx_F32RANGE(I,A,MIN,MAX,D,U) MAKE_RANGE_REGISTERx(I,A,REG_TYPE_FLOAT32,f32,D,MIN,MAX,U)
#define REG_F32FNC(I,A,FNC,D)          MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_FLOAT32,f32,D,FNC)
#define REGx_F32FNC(I,A,FNC,D,U)       MAKE_VALIDATOR_REGISTERx(I,A,REG_TYPE_FLOAT32,f32,D,FNC,U)
#define REG_F64(I,A,D)                 MAKE_REGISTER(I,A,REG_TYPE_FLOAT64,f64,D)
#define REGx_F64(I,A,D,U)              MAKE_REGISTERx(I,A,REG_TYPE_FLOAT64,f64,D,U)
#define REG_F64FAIL(I,A,D)             MAKE_FAIL_REGISTER(I,A,REG_TYPE_FLOAT64,f64,D)
#define REGx_F64FAIL(I,A,D,U)          MAKE_FAIL_REGISTERx(I,A,REG_TYPE_FLOAT64,f64,D,U)
#define REG_F64MIN(I,A,MIN,D)          MAKE_MIN_REGISTER(I,A,REG_TYPE_FLOAT64,f64,D,MIN)
#define REGx_F64MIN(I,A,MIN,D,U)       MAKE_MIN_REGISTERx(I,A,REG_TYPE_FLOAT64,f64,D,MIN,U)
#define REG_F64MAX(I,A,MAX,D)          MAKE_MAX_REGISTER(I,A,REG_TYPE_FLOAT64,f64,D,MAX)
#define REGx_F64MAX(I,A,MAX,D,U)       MAKE_MAX_REGISTERx(I,A,REG_TYPE_FLOAT64,f64,D,MAX,U)
#define REG_F64RANGE(I,A,MIN,MAX,D)    MAKE_RANGE_REGISTER(I,A,REG_TYPE_FLOAT64,f64,D,MIN,MAX)
#define REGx_F64RANGE(I,A,MIN,MAX,D,U) MAKE_RANGE_REGISTERx(I,A,REG_TYPE_FLOAT64,f64,D,MIN,MAX,U)
#define REG_F64FNC(I,A,FNC,D)          MAKE_VALIDATOR_REGISTER(I,A,REG_TYPE_FLOAT64,f64,D,FNC)
#define REGx_F64FNC(I,A,FNC,D,U)       MAKE_VALIDATOR_REGISTERx(I,A,REG_TYPE_FLOAT64,f64,D,FNC,U)

/* End of Registers Specification Front-Ends */

/*
 * Public Functions
 */

void register_make_bigendian(RegisterTable *t, bool bigendian);
RegisterInit register_init(RegisterTable *t);
RegisterAccess register_user_init(RegisterTable *t, registerCallback f);

RegisterAccess reg_mem_read(const RegisterArea *a, RegisterAtom *dest,
                            RegisterOffset offset, RegisterOffset n);
RegisterAccess reg_mem_write(RegisterArea *a, const RegisterAtom *src,
                             RegisterOffset offset, RegisterOffset n);

RegisterAccess register_set(RegisterTable *t, RegisterHandle idx,
                            RegisterValue v);
RegisterAccess register_set_unsafe(RegisterTable *t, RegisterHandle idx,
                                   RegisterValue v);
RegisterAccess register_get(RegisterTable *t, RegisterHandle idx,
                            RegisterValue *v);

RegisterAccess register_bit_set(RegisterTable *t, RegisterHandle idx,
                                RegisterValue v);
RegisterAccess register_bit_clear(RegisterTable *t, RegisterHandle idx,
                                  RegisterValue v);

RegisterAccess register_default(RegisterTable *t, RegisterHandle idx,
                                RegisterValue *v);
RegisterAccess register_block_read(RegisterTable *t, RegisterAddress addr,
                                   RegisterOffset n, RegisterAtom *buf);
RegisterAccess register_block_write(RegisterTable *t, RegisterAddress addr,
                                    RegisterOffset n, RegisterAtom *buf);

RegisterAccess register_block_read_unsafe(RegisterTable *t,
                                          RegisterAddress addr,
                                          RegisterOffset n,
                                          RegisterAtom *buf);
RegisterAccess register_block_write_unsafe(RegisterTable *t,
                                           RegisterAddress addr,
                                           RegisterOffset n,
                                           RegisterAtom *buf);

RegisterAccess register_block_touches_hole(RegisterTable *t,
                                           RegisterAddress addr,
                                           RegisterOffset n);
RegisterAccess register_set_from_hexstr(RegisterTable *t,
                                        RegisterAddress start,
                                        const char *str, size_t n);
RegisterAccess register_sanitise(RegisterTable *t);

RegisterEntry *register_get_entry(const RegisterTable *t, RegisterHandle r);
size_t register_entry_size(const RegisterEntry *e);

RegisterAccess register_mcopy(RegisterTable *t, AreaHandle dst, AreaHandle src);
bool register_value_compare(const RegisterValue *a, const RegisterValue *b);
RegisterAccess register_compare(
    RegisterTable *t, RegisterHandle a, RegisterHandle b);

RegisterAccess register_foreach_in(RegisterTable *t, RegisterAddress addr,
                                   RegisterOffset off, registerCallback f,
                                   void *arg);

static inline RegisterAddress
register_address(RegisterTable *t, RegisterHandle reg)
{
    return t->entry[reg].address;
}

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

#endif /* INC_UFW_REGISTER_TABLE_H */
