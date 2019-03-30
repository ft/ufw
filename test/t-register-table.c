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

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(72);
    t_invalid_tables();    /*  3 */
    t_trivial_success();   /*  1 */
    t_trivial_fail();      /*  1 */
    t_area_init_checks();  /*  4 */
    t_entry_init_checks(); /* 16 */
    t_bfg2000();           /* 47 */
}
