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

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(25);
    t_invalid_tables();    /*  3 */
    t_trivial_success();   /*  1 */
    t_trivial_fail();      /*  1 */
    t_area_init_checks();  /*  4 */
    t_entry_init_checks(); /* 16 */
}
