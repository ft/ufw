#include <tap.h>
#include <common/compiler.h>
#include <c/register-table.h>

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
    cmp_ok(success.code, "==", REG_INIT_SUCCESS, "Trivial table succeeds");
}

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(1);
    t_trivial_success();
}
