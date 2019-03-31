#include <math.h>
#include <string.h>

#include <tap.h>
#include <common/compiler.h>
#include <c/register-table.h>

static void
t_invalid_tables(void)
{
    RegisterTable *r_null = NULL;
    RegisterTable r_areas_null = {
        .area = (RegisterArea*)NULL,
        .entry = (RegisterEntry[]) {
            REGISTER_ENTRY_END
        }
    };
    RegisterTable r_entries_null = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0000ul, 0x10ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry*)NULL
    };

    RegisterInit success = register_init(r_null);
    cmp_ok(success.code, "==", REG_INIT_TABLE_INVALID,
           "NULL register table fails");
    success = register_init(&r_areas_null);
    cmp_ok(success.code, "==", REG_INIT_TABLE_INVALID, "NULL r.area fails");
    success = register_init(&r_entries_null);
    cmp_ok(success.code, "==", REG_INIT_TABLE_INVALID, "NULL r.entry fails");
}

static void
t_trivial_success(void)
{
    /* A register table needs at least one area. It does not require any items
     * to be defined within that. */
    RegisterTable registers = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0000ul, 0x10ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REGISTER_ENTRY_END
        }
    };

    RegisterInit success = register_init(&registers);
    cmp_ok(success.code, "==", REG_INIT_SUCCESS, "One area table succeeds");
}

static void
t_trivial_fail(void)
{
    /* A register table needs at least one area. */
    RegisterTable registers = {
        .area = (RegisterArea[]) {
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REGISTER_ENTRY_END
        }
    };

    RegisterInit success = register_init(&registers);
    cmp_ok(success.code, "==", REG_INIT_NO_AREAS, "Trivial table fails");
}

static void
t_area_init_checks(void)
{
    /* A register table needs at least one area. */
    RegisterTable r_overlap = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0000ul, 0x10ul),
            MEMORY_AREA(0x000ful, 0x10ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REGISTER_ENTRY_END
        }
    };
    RegisterTable r_just_no_overlap = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0000ul, 0x10ul),
            MEMORY_AREA(0x0010ul, 0x10ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REGISTER_ENTRY_END
        }
    };
    RegisterTable r_area_order = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0010ul, 0x10ul),
            MEMORY_AREA(0x0000ul, 0x10ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REGISTER_ENTRY_END
        }
    };

    RegisterInit success = register_init(&r_overlap);
    cmp_ok(success.code, "==", REG_INIT_AREA_ADDRESS_OVERLAP,
           "Area overlap detected");
    success = register_init(&r_just_no_overlap);
    cmp_ok(success.code, "==", REG_INIT_SUCCESS,
           "Adjacent areas without gap in between succeeds");
    success = register_init(&r_area_order);
    cmp_ok(success.code, "==", REG_INIT_AREA_INVALID_ORDER,
           "Area order is not proper");
    cmp_ok(success.pos.area, "==", 1,
           "  ...area at index 1 yielded the error");
}

static void
t_entry_init_checks(void)
{
    /* A register table needs at least one area. */
    RegisterTable r_overlap = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0000ul, 0x100ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REG_U32(0, 0x0000ul, 0x2342u),
            REG_U32(1, 0x0001ul, 0x2342u),
            REGISTER_ENTRY_END
        }
    };
    RegisterTable r_just_no_overlap = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0000ul, 0x100ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REG_U32(0, 0x0000ul, 0x2342u),
            REG_U32(1, 0x0002ul, 0x2342u),
            REGISTER_ENTRY_END
        }
    };
    RegisterTable r_entry_order = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0000ul, 0x100ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REG_U32(0, 0x0002ul, 0x2342u),
            REG_U32(1, 0x0000ul, 0x2342u),
            REGISTER_ENTRY_END
        }
    };
    RegisterTable r_with_hole = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0000ul, 0x100ul),
            /* Hole from 0x100 to 0x1ff */
            MEMORY_AREA(0x0200ul, 0x100ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REG_U32(0, 0x0100ul, 0x2342u),
            REG_U32(1, 0x02f0ul, 0x2342u),
            REGISTER_ENTRY_END
        }
    };

    RegisterInit success = register_init(&r_overlap);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_ADDRESS_OVERLAP,
           "Entry overlap detected");
    success = register_init(&r_just_no_overlap);
    cmp_ok(success.code, "==", REG_INIT_SUCCESS,
           "Adjacent entries without gap in between succeeds");
    success = register_init(&r_entry_order);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_INVALID_ORDER,
           "Entry order is not proper");
    cmp_ok(success.pos.entry, "==", 1,
           "  ...entry at index 1 yielded the error");
    success = register_init(&r_with_hole);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_IN_MEMORY_HOLE,
           "Entry in memory hole");
    cmp_ok(success.pos.entry, "==", 0,
           "  ...entry at index 0 yielded the error");
    r_with_hole.entry[0].address = 0xfful;
    success = register_init(&r_with_hole);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_IN_MEMORY_HOLE,
           "Entry still in memory hole (part of it)");
    cmp_ok(success.pos.entry, "==", 0,
           "  ...entry at index 0 yielded the error");
    r_with_hole.entry[0].address = 0xfeul;
    success = register_init(&r_with_hole);
    cmp_ok(success.code, "==", REG_INIT_SUCCESS,
           "Entry completely in memory succeeds");
    r_with_hole.entry[0].address = 0x1feul;
    success = register_init(&r_with_hole);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_IN_MEMORY_HOLE,
           "Entry in memory hole again");
    cmp_ok(success.pos.entry, "==", 0,
           "  ...entry at index 0 yielded the error");
    r_with_hole.entry[0].address = 0x1fful;
    success = register_init(&r_with_hole);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_IN_MEMORY_HOLE,
           "Entry still in memory hole (part of it)");
    cmp_ok(success.pos.entry, "==", 0,
           "  ...entry at index 0 yielded the error");
    r_with_hole.entry[0].address = 0x200ul;
    success = register_init(&r_with_hole);
    cmp_ok(success.code, "==", REG_INIT_SUCCESS,
           "Entry completely in memory succeeds");
    r_with_hole.entry[1].check.type = REGV_TYPE_MIN;
    r_with_hole.entry[1].check.arg.range.min.u32 = 0x3000ul;
    success = register_init(&r_with_hole);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_INVALID_DEFAULT,
           "Entry default value does not pass inspection");
    cmp_ok(success.pos.entry, "==", 1,
           "  ...entry at index 1 yielded the error");
}

typedef enum SensorRegister {
    SENSOR_DEVICE_ID = 0,
    SENSOR_RAMP_DURATION,
    SENSOR_AGE_OF_UNIVERSE,
    SENSOR_PHASE_DELAY_A,
    SENSOR_PHASE_DELAY_B,
    SENSOR_PHASE_DELAY_C,
    SENSOR_TRIGGER_PERIOD
} SensorRegister;

#define STRMAP(name) [name] = #name

static char*
type2string(RegisterType type)
{
    if (type > REG_TYPE_FLOAT32)
        return "<INVALID-TYPE>";

    char *map[] = {
        STRMAP(REG_TYPE_INVALID),
        STRMAP(REG_TYPE_UINT16),
        STRMAP(REG_TYPE_UINT32),
        STRMAP(REG_TYPE_UINT64),
        STRMAP(REG_TYPE_SINT16),
        STRMAP(REG_TYPE_SINT32),
        STRMAP(REG_TYPE_SINT64),
        STRMAP(REG_TYPE_FLOAT32)
    };

    return map[type];
}

static void
test_register_value(RegisterTable *t, RegisterHandle reg,
                    RegisterType type, RegisterValueU def)
{
    RegisterAccess success;
    RegisterValue v;
    success = register_default(t, reg, &v);
    cmp_ok(success.code, "==", REG_ACCESS_SUCCESS,
           "%s: Accessing default value works",
           register_name(t, reg));
    cmp_ok(v.type, "==", type,
           "%s: Default value has correct type [%s]",
           register_name(t, reg),
           type2string(type));
    switch (type) {
    case REG_TYPE_UINT16:
        cmp_ok(v.value.u16, "==", def.u16,
               "%s: Default value checks out",
               register_name(t, reg));
        break;
    case REG_TYPE_UINT32:
        cmp_ok(v.value.u32, "==", def.u32,
               "%s: Default value checks out",
               register_name(t, reg));
        break;
    case REG_TYPE_UINT64:
        cmp_ok(v.value.u64, "==", def.u64,
               "%s: Default value checks out",
               register_name(t, reg));
        break;
    case REG_TYPE_SINT16:
        cmp_ok(v.value.s16, "==", def.s16,
               "%s: Default value checks out",
               register_name(t, reg));
        break;
    case REG_TYPE_SINT32:
        cmp_ok(v.value.s32, "==", def.s32,
               "%s: Default value checks out",
               register_name(t, reg));
        break;
    case REG_TYPE_SINT64:
        cmp_ok(v.value.s64, "==", def.s64,
               "%s: Default value checks out",
               register_name(t, reg));
        break;
    default:
        cmp_ok(v.value.f32, "==", def.f32,
               "%s: Default value checks out",
               register_name(t, reg));
        break;
    }
}

static void
test_register(RegisterTable *t, RegisterHandle reg, RegisterAddress area,
              RegisterOffset offset,
              RegisterType type, RegisterValueU def)
{
    test_register_value(t, reg, type, def);          /* 3 */
    cmp_ok(register_area(t, reg)->base, "==", area,  /* 1 */
           "%s: Is in the correct area",
           register_name(t, reg));
    cmp_ok(register_offset(t, reg), "==", offset,    /* 1 */
           "%s: Offset in area is correct",
           register_name(t, reg));
}

static RegisterTable bfg2000 = {
    .area = (RegisterArea[]) {
        MEMORY_AREA(0x0000ul, 0x40ul),
        MEMORY_AREA(0x1000ul, 0x40ul),
        MEMORY_AREA(0x1040ul, 0x40ul),
        REGISTER_AREA_END
    },
    .entry = (RegisterEntry[]) {
        REG_U16(SENSOR_DEVICE_ID,       0x0000ul, 0x2342u),
        REG_U32(SENSOR_RAMP_DURATION,   0x0010ul, 0x12345678ul),
        REG_U64(SENSOR_AGE_OF_UNIVERSE, 0x0020ul, 0x8765432112345678ull),
        REG_S16(SENSOR_PHASE_DELAY_A,   0x1000ul, -23),
        REG_S32(SENSOR_PHASE_DELAY_B,   0x1010ul, -123456l),
        REG_S64(SENSOR_PHASE_DELAY_C,   0x1020ul, -112233445566778899ll),
        REG_F32(SENSOR_TRIGGER_PERIOD,  0x1040ul, 42e-6),
        REGISTER_ENTRY_END
    }
};

static void
t_bfg2000(void)
{
    RegisterInit init_success;

    init_success = register_init(&bfg2000);
    /* Check initialisation results: 12 */
    cmp_ok(init_success.code, "==", REG_INIT_SUCCESS,
           "BFG2000 register table initialises");
    cmp_ok(bfg2000.areas, "==", 3u,
           "BFG2000 has the correct number of areas");
    cmp_ok(bfg2000.entries, "==", 7u,
           "BFG2000 has the correct number of entries");
    cmp_ok(bfg2000.area[0].entry.count, "==", 3u,
           "BFG2000 area[0] has three entries");
    cmp_ok(bfg2000.area[0].entry.first, "==", 0u,
           "  ...first is 0");
    cmp_ok(bfg2000.area[0].entry.last, "==", 2u,
           "  ...last is 2");
    cmp_ok(bfg2000.area[1].entry.count, "==", 3u,
           "BFG2000 area[1] has three entries");
    cmp_ok(bfg2000.area[1].entry.first, "==", 3u,
           "  ...first is 3");
    cmp_ok(bfg2000.area[1].entry.last, "==", 5u,
           "  ...last is 5");
    cmp_ok(bfg2000.area[2].entry.count, "==", 1u,
           "BFG2000 area[2] has one entry");
    cmp_ok(bfg2000.area[2].entry.first, "==", 6u,
           "  ...first is 6");
    cmp_ok(bfg2000.area[2].entry.last, "==", 6u,
           "  ...as is last.");

    /* Check default values and register meta-data: 7 * 5 = 35 */
    test_register(&bfg2000, SENSOR_DEVICE_ID, 0, 0,
                  REG_TYPE_UINT16, (RegisterValueU){ .u16 = 0x2342 });
    test_register(&bfg2000, SENSOR_RAMP_DURATION, 0, 0x10,
                  REG_TYPE_UINT32, (RegisterValueU){ .u32 = 0x12345678ul });
    test_register(&bfg2000, SENSOR_AGE_OF_UNIVERSE, 0, 0x20,
                  REG_TYPE_UINT64, (RegisterValueU){ .u64 = 0x8765432112345678ull });
    test_register(&bfg2000, SENSOR_PHASE_DELAY_A, 0x1000, 0,
                  REG_TYPE_SINT16, (RegisterValueU){ .s16 = -23 });
    test_register(&bfg2000, SENSOR_PHASE_DELAY_B, 0x1000, 0x10,
                  REG_TYPE_SINT32, (RegisterValueU){ .s32 = -123456l });
    test_register(&bfg2000, SENSOR_PHASE_DELAY_C, 0x1000, 0x20,
                  REG_TYPE_SINT64, (RegisterValueU){ .s64 = -112233445566778899ll });
    test_register(&bfg2000, SENSOR_TRIGGER_PERIOD, 0x1040, 0x00,
                  REG_TYPE_FLOAT32, (RegisterValueU){ .f32 = 42e-6 });
}

#define RV(T,M,V) ((RegisterValue){ .type = REG_TYPE_##T, .value.M = V })
#define F32EQ(A,B,EPS) ((A > (B-EPS)) && (A < (B+EPS)))

/* Here's a large macro that is used to generate a bunch of similar tests
 * against the seven supported data-types that the register table suports. As
 * with most macros, that's not an awesome solution. But it's better than
 * typing it all out manually. */

#define GENERATE_CONSTRAIN_TESTS(                                       \
    TST,                                                                \
    T,M,MM,                                                             \
    VT_EPS,                                                             \
    VLD,VLD_VAL,VLD_A,VLD_B,VLD_C,VLD_D,VLD_FAIL,                       \
    VT_UNC_VAL,VT_UNC_MIN,VT_UNC_MAX,                                   \
    VT_MIN_VAL,VT_MIN_MIN,VT_MIN_MAX,                                   \
    VT_MAX_VAL,VT_MAX_MIN,VT_MAX_MAX,                                   \
    VT_RAN_VAL,VT_RAN_MIN,VT_RAN_MAX)                                   \
                                                                        \
    static bool                                                         \
    VLD(const RegisterEntry *e, const RegisterValue v)                  \
    {                                                                   \
        if (e->type != REG_TYPE_##T)                                    \
            return false;                                               \
                                                                        \
        if (v.type != REG_TYPE_##T)                                     \
            return false;                                               \
                                                                        \
        if (e->type == REG_TYPE_FLOAT32) {                              \
            float eps = VT_EPS / 10.;                                   \
            return (F32EQ(v.value.M, VLD_A, eps)                        \
                    || F32EQ(v.value.M, VLD_B, eps)                     \
                    || F32EQ(v.value.M, VLD_C, eps)                     \
                    || F32EQ(v.value.M, VLD_D, eps));                   \
        } else {                                                        \
            return ((v.value.M == VLD_A)                                \
                    || (v.value.M == VLD_B)                             \
                    || (v.value.M == VLD_C)                             \
                    || (v.value.M == VLD_D));                           \
        }                                                               \
    }                                                                   \
                                                                        \
    static void                                                         \
    TST(void)                                                           \
    {                                                                   \
        RegisterTable rok = {                                           \
            .area = (RegisterArea[]) {                                  \
                MEMORY_AREA(0x0000ul, 0x100ul),                         \
                REGISTER_AREA_END                                       \
            },                                                          \
            .entry = (RegisterEntry[]) {                                \
                REG_##MM(       0, 0x0000ul,                         VT_UNC_VAL), \
                REG_##MM##MIN(  1, 0x0010ul, VT_MIN_MIN,             VT_MIN_VAL), \
                REG_##MM##MAX(  2, 0x0020ul,             VT_MAX_MAX, VT_MAX_VAL), \
                REG_##MM##RANGE(3, 0x0030ul, VT_RAN_MIN, VT_RAN_MAX, VT_RAN_VAL), \
                REG_##MM##FNC(  4, 0x0040ul, VLD,                    VLD_VAL), \
                REGISTER_ENTRY_END                                      \
            }                                                           \
        };                                                              \
        RegisterInit success = register_init(&rok);                     \
        cmp_ok(success.code, "==", REG_INIT_SUCCESS, #M " table initialises"); \
                                                                        \
        RegisterAccess a = register_set(&rok, 0, RV(T, M, VT_UNC_MIN)); \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " unconstrained min"); \
        a = register_set(&rok, 0,  RV(T, M, VT_UNC_MAX));               \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " unconstrained max"); \
                                                                        \
        a = register_set(&rok, 1, RV(T, M, VT_MIN_MIN));                \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " min-constrained min"); \
        a = register_set(&rok, 1, RV(T, M, VT_MIN_MIN - VT_EPS));       \
        cmp_ok(a.code, "==", REG_ACCESS_RANGE, #M " min-constrained min fail"); \
        a = register_set(&rok, 1,  RV(T, M, VT_MIN_MAX));               \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " min-constrained max"); \
                                                                        \
        a = register_set(&rok, 2, RV(T, M, VT_MAX_MIN));                \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " max-constrained min"); \
        a = register_set(&rok, 2, RV(T, M, VT_MAX_MAX));                \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " max-constrained max"); \
        a = register_set(&rok, 2,  RV(T, M, VT_MAX_MAX + VT_EPS));      \
        cmp_ok(a.code, "==", REG_ACCESS_RANGE, #M " max-constrained max fail"); \
                                                                        \
        a = register_set(&rok, 3, RV(T, M, VT_RAN_MIN));                \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " range-constrained min"); \
        a = register_set(&rok, 3,  RV(T, M, VT_RAN_MIN - VT_EPS));      \
        cmp_ok(a.code, "==", REG_ACCESS_RANGE, #M " range-constrained min fail"); \
        a = register_set(&rok, 3, RV(T, M, VT_RAN_MAX));                \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " range-constrained max"); \
        a = register_set(&rok, 3,  RV(T, M, VT_RAN_MAX + VT_EPS));      \
        cmp_ok(a.code, "==", REG_ACCESS_RANGE, #M " range-constrained max fail"); \
                                                                        \
        a = register_set(&rok, 4, RV(T, M, VLD_A));                     \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " fnc-constrained a"); \
        a = register_set(&rok, 4, RV(T, M, VLD_B));                     \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " fnc-constrained b"); \
        a = register_set(&rok, 4, RV(T, M, VLD_C));                     \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " fnc-constrained c"); \
        a = register_set(&rok, 4, RV(T, M, VLD_D));                     \
        cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, #M " fnc-constrained d"); \
        a = register_set(&rok, 4, RV(T, M, VLD_FAIL));                  \
        cmp_ok(a.code, "==", REG_ACCESS_RANGE, #M " fnc-constrained e fail"); \
    }

GENERATE_CONSTRAIN_TESTS(t_u16_regs, UINT16, u16, U16,
                         1u,
                         validate_u16,
                         0x1000u, 0x1000u, 0x2000u, 0x3000u, 0x4000u, 0x5000u,
                         0x1234u, 0, UINT16_MAX,
                         0x1000u, 0x1000u, UINT16_MAX,
                         0x3000u, 0, 0x3000u,
                         0x3fffu, 0x1000u, 0x4000u)

GENERATE_CONSTRAIN_TESTS(t_u32_regs, UINT32, u32, U32,
                         1u,
                         validate_u32,
                         0x10000000ul, 0x10000000ul,
                         0x20000000ul, 0x30000000ul, 0x40000000ul, 0x50000000ul,
                         0x12340000ul, 0, UINT32_MAX,
                         0x10000000ul, 0x10000000ul, UINT32_MAX,
                         0x30000000ul, 0, 0x30000000ul,
                         0x3ffffffful, 0x10000000ul, 0x40000000ul)

GENERATE_CONSTRAIN_TESTS(t_u64_regs, UINT64, u64, U64,
                         1ull,
                         validate_u64,
                         0x1000000000000000ull, 0x1000000000000000ull,
                         0x2000000000000000ull, 0x3000000000000000ull,
                         0x4000000000000000ull, 0x5000000000000000ull,
                         0x1234000000000000ull, 0, UINT64_MAX,
                         0x1000000000000000ull, 0x1000000000000000ull, UINT64_MAX,
                         0x3000000000000000ull, 0, 0x3000000000000000ull,
                         0x3fffffffffffffffull, 0x1000000000000000ull, 0x4000000000000000ull)

GENERATE_CONSTRAIN_TESTS(t_s16_regs, SINT16, s16, S16,
                         1,
                         validate_s16,
                         0x1000, 0x1000, 0x2000, -0x3000, -0x4000, 0x5000,
                         -0x1234, INT16_MIN, INT16_MAX,
                         0x1000, -0x1000, INT16_MAX,
                         -0x3000, INT16_MIN, 0x3000,
                         0x3fff, -0x1000, 0x4000)

GENERATE_CONSTRAIN_TESTS(t_s32_regs, SINT32, s32, S32,
                         1l,
                         validate_s32,
                         0x10000000l, 0x10000000l,
                         0x20000000l, -0x30000000l, -0x40000000l, 0x50000000l,
                         -0x12340000l, INT32_MIN, INT32_MAX,
                         0x10000000l, -0x10000000l, INT32_MAX,
                         -0x30000000l, INT32_MIN, 0x30000000l,
                         0x3fffffffl, -0x10000000l, 0x40000000l)

GENERATE_CONSTRAIN_TESTS(t_s64_regs, SINT64, s64, S64,
                         1ll,
                         validate_s64,
                         0x1000000000000000ll, 0x1000000000000000ll,
                         0x2000000000000000ll, -0x3000000000000000ll,
                         -0x4000000000000000ll, 0x5000000000000000ll,
                         -0x1234000000000000ll, INT64_MIN, INT64_MAX,
                         0x1000000000000000ll, 0x1000000000000000ll, INT64_MAX,
                         -0x3000000000000000ll, INT64_MIN, 0x3000000000000000ll,
                         0x3fffffffffffffffll, -0x1000000000000000ll, 0x4000000000000000ll)

#define F32_MIN -1e6
#define F32_MAX  1e6

GENERATE_CONSTRAIN_TESTS(
    /* TST, T, M, MM */
    t_f32_regs, FLOAT32, f32, F32,
    /* VT_EPS */
    0.1,
    /* VLD */
    validate_f32,
    /* VLD_VAL, VLD_A, VLD_B, VLD_C, VLD_D, VLD_FAIL */
    1e3, 1e3, 2e3, -3e3, -4e3, 5e3,
    /* VT_UNC_VAL, VT_UNC_MIN, VT_UNC_MAX */
    200e3, F32_MIN, F32_MAX,
    /* VT_MIN_VAL, VT_MIN_MIN, VT_MIN_MAX */
    300e3, -1e3, F32_MAX,
    /* VT_MAX_VAL, VT_MAX_MIN, VT_MAX_MAX */
    400e3, F32_MIN, 1e6,
    /* VT_RAN_VAL, VT_RAN_MIN, VT_RAN_MAX */
    500e3, -1e3, 800e3)

void
t_f32_abnormal(void)
{
    RegisterTable regs = {
        .area = (RegisterArea[]) {
            MEMORY_AREA(0x0000ul, 0x40ul),
            REGISTER_AREA_END
        },
        .entry = (RegisterEntry[]) {
            REG_F32(0, 0x0000ul, NAN),
            REGISTER_ENTRY_END
        }
    };

    RegisterInit success = register_init(&regs);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_INVALID_DEFAULT,
           "f32 can't default to NAN");
    regs.entry[0].default_value.f32 = -NAN;
    success = register_init(&regs);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_INVALID_DEFAULT,
           "f32 can't default to -NAN");
    regs.entry[0].default_value.f32 = INFINITY;
    success = register_init(&regs);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_INVALID_DEFAULT,
           "f32 can't default to INFINITY");
    regs.entry[0].default_value.f32 = -INFINITY;
    success = register_init(&regs);
    cmp_ok(success.code, "==", REG_INIT_ENTRY_INVALID_DEFAULT,
           "f32 can't default to -INFINITY");
    regs.entry[0].default_value.f32 = 0.;
    success = register_init(&regs);
    cmp_ok(success.code, "==", REG_INIT_SUCCESS, "f32 with zero works");

    RegisterAccess a = register_set(&regs, 0, RV(FLOAT32, f32, NAN));
    cmp_ok(a.code, "==", REG_ACCESS_INVALID, "f32 can't set to NAN");
    a = register_set(&regs, 0, RV(FLOAT32, f32, -NAN));
    cmp_ok(a.code, "==", REG_ACCESS_INVALID, "f32 can't set to -NAN");
    a = register_set(&regs, 0, RV(FLOAT32, f32, INFINITY));
    cmp_ok(a.code, "==", REG_ACCESS_INVALID, "f32 can't set to INFINITY");
    a = register_set(&regs, 0, RV(FLOAT32, f32, -INFINITY));
    cmp_ok(a.code, "==", REG_ACCESS_INVALID, "f32 can't set to -INFINITY");

    RegisterValue v;
    a = register_get(&regs, 0, &v);
    cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, "f32 can get value");
    cmp_ok(v.type, "==", REG_TYPE_FLOAT32, "Gotten value is type f32");
    cmp_ok(v.value.f32, "==", 0., "Gotten value is correct value");
}

typedef enum SensorRegisterV2 {
    V2_DEVICE_ID = 0,
    V2_MAJOR_VERSION,
    V2_MINOR_VERSION,
    V2_PLASMA_KIND,
    V2_AGE_OF_UNIVERSE,
    V2_PHASE_FACTOR,
    V2_TRIGGER_TICK
} SensorRegisterV2;

static RegisterTable bfg2000v2 = {
    .area = (RegisterArea[]) {
        MEMORY_AREA_RO(0x0000ul, 0x40ul),
        MEMORY_AREA(0x1000ul, 0x40ul),
        MEMORY_AREA(0x1040ul, 0x40ul),
        REGISTER_AREA_END
    },
    .entry = (RegisterEntry[]) {
        REG_U16(V2_DEVICE_ID,       0x0000ul, 0x234fu),
        REG_U16(V2_MAJOR_VERSION,   0x0001ul, 2),
        REG_U16(V2_MINOR_VERSION,   0x0002ul, 427),
        REG_U32(V2_PLASMA_KIND,     0x1000ul, 0x12345678ul),
        REG_U64(V2_AGE_OF_UNIVERSE, 0x1002ul, 0x8765432112345678ull),
        REG_F32(V2_PHASE_FACTOR,    0x1006ul, -23.54),
        REG_U32(V2_TRIGGER_TICK,    0x1009ul, 8002ul),
        REGISTER_ENTRY_END
    }
};

static void
t_block_access(void)
{
    RegisterAtom buf[1024];
    RegisterAccess a;

    RegisterInit success = register_init(&bfg2000v2);
    cmp_ok(success.code, "==", REG_INIT_SUCCESS, "BFG2000v2 initialises");

    /* Reading the memory from one defined table entry has to work, if the
     * register table initialised.*/
    a = register_block_read(&bfg2000v2, 0x2, 1, buf);
    cmp_ok(a.code, "==", REG_ACCESS_SUCCESS, "V2_MINOR_VERSION reads");
    cmp_ok(buf[0], "==", 427, "V2_MINOR_VERSION has correct value");

    /* Now here's a spot in the address space, that's not mapped to memory.
     * That's what we call a hole. Accessing those fails. This hole is located
     * directly above a mapped area. */
    a = register_block_read(&bfg2000v2, 0x40, 1, buf);
    cmp_ok(a.code, "==", REG_ACCESS_NOENTRY, "0x40 is the start of a hole");
    cmp_ok(a.address, "==", 0x40, "Address 0x40 is correctly signaled");

    /* Starting out in mapped addresses, but reading into a hole also fails.
     * And the system has to return the address where the error occured. */
    a = register_block_read(&bfg2000v2, 0x3f, 2, buf);
    cmp_ok(a.code, "==", REG_ACCESS_NOENTRY, "0x3f is okay, but 0x40 still a hole");
    cmp_ok(a.address, "==", 0x40, "Address 0x40 is correctly signaled (partial)");

    /* This is a direct hole again. It's directly below a mapped area. */
    a = register_block_read(&bfg2000v2, 0xfff, 1, buf);
    cmp_ok(a.code, "==", REG_ACCESS_NOENTRY, "0xfff is a hole");
    cmp_ok(a.address, "==", 0xfff, "Address 0xfff is correctly signaled");

    /* Starting in a hole fails. It doesn't matter if the block read would span
     * into mapped memory eventually. */
    a = register_block_read(&bfg2000v2, 0xfff, 2, buf);
    cmp_ok(a.code, "==", REG_ACCESS_NOENTRY, "Still 0xfff is a hole");
    cmp_ok(a.address, "==", 0xfff, "Address 0xfff is correctly signaled (partial)");

    /* Now reading mapped memory that does not contain entries, will *work*.
     * This register implementation will initialise all unmapped memory to all
     * zero bits. Let's read 0x20 atoms starting at address 0x20. */
    a = register_block_read(&bfg2000v2, 0x20, 0x20, buf);
    cmp_ok(a.code, "==", REG_ACCESS_SUCCESS,
           "Reading a mapped but unoccupied chunk of memory works");
    {
        RegisterAtom expect[0x20];
        memset(expect, 0u, 0x20 * sizeof(RegisterAtom));
        cmp_mem(buf, expect, 0x20, "Unoccupied memory is initialised to zero");
    }
}

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(3+1+1+4+16+47+(7*18)+12+13);
    t_invalid_tables();    /*  3 */
    t_trivial_success();   /*  1 */
    t_trivial_fail();      /*  1 */
    t_area_init_checks();  /*  4 */
    t_entry_init_checks(); /* 16 */
    t_bfg2000();           /* 47 */
    t_u16_regs();          /* 18 */
    t_u32_regs();          /* 18 */
    t_u64_regs();          /* 18 */
    t_s16_regs();          /* 18 */
    t_s32_regs();          /* 18 */
    t_s64_regs();          /* 18 */
    t_f32_regs();          /* 18 */
    t_f32_abnormal();      /* 12 */
    t_block_access();      /* 13 */
}
