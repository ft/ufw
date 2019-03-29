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

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(9);
    t_invalid_tables();    /* 3 */
    t_trivial_success();   /* 1 */
    t_trivial_fail();      /* 1 */
    t_area_init_checks();  /* 4 */
}
