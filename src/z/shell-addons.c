/*
 * Copyright (c) 2022-2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file shell-addons.c
 * @brief Zephyr System Shell extensions
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include <ufw/register-table.h>
#include <ufw/register-utilities.h>

#include <ufwz/shell-addons.h>

/**
 * fprintf like printer for zephyr shell instances
 *
 * @param  dst      Pointer to ‘struct shell’ referencing the context to run in.
 * @param  fmt      printf-like format string to specifying output.
 * @param  ...      stdarg va_list of arguments.
 *
 * @return void
 * @sideeffects Print data to referenced shell instance.
 */
void
ufw_shell_fprintf(void *dst, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    shell_vfprintf(dst, SHELL_NORMAL, fmt, args);
    va_end(args);
}

static int
ufw_shell_addons_init(void)
{
    register_set_printer(ufw_shell_fprintf);
    return 0;
}

SYS_INIT(ufw_shell_addons_init, PRE_KERNEL_1, 0);

static RegisterTable *table = NULL;

/**
 * Specify register table to use for ‘regshow’ command
 *
 * @param  t        Pointer to the register table to use.
 *
 * @return void
 * @sideeffects Sets ‘table’ pointer to specified table.
 */
void
ufw_shell_reg_init(RegisterTable *t)
{
    table = t;
}

static void
outofrange(struct shell *shell, long int n)
{
    ufw_shell_fprintf(shell, "Argument out of range: %ld\n", n);
}

static int
print_by_handle(struct shell *shell, RegisterHandle h)
{
    RegisterEntry* e = register_get_entry(table, h);
    if (e == NULL) {
        outofrange(shell, h);
        return -1;
    }
    ufw_shell_fprintf(shell, "  RegisterEntryID: %" PRIu32 "\n", h);
    register_entry_print(shell, "", e);
    return 0;
}

static void
print_current(struct shell *shell, RegisterHandle h)
{
    RegisterValue value;
    RegisterAccess rv = register_get(table, h, &value);
    if (rv.code != REG_ACCESS_SUCCESS) {
        ufw_shell_fprintf(shell, "register_get(): %d, %s\n",
                          (int)rv.code,
                          register_accesscode_to_string(rv.code));
    } else {
        ufw_shell_fprintf(shell, "    Current Value   : ");
        register_value_print(shell, &value);
        ufw_shell_fprintf(shell, "\n");
    }
}

static int
print_with_current(struct shell *shell, long int reg)
{
    if (print_by_handle(shell, reg) == 0) {
        print_current(shell, reg);
        return 0;
    }
    return -1;
}

static int
print_range(struct shell *shell, const char *s, const char *e)
{
    long int start = strtol(s, NULL, 10u);
    long int end   = strtol(e, NULL, 10u);

    if (start < 0) {
        outofrange(shell, start);
        return -1;
    }

    if (start > end) {
        ufw_shell_fprintf(shell,
                          "Start parameter must be <= End parameter.\n");
        return -1;
    }
    for (long int i = start; i <= end; ++i) {
        if (print_with_current(shell, i) < 0) {
            return -1;
        }
    }

    return 0;
}

static bool
want_help(size_t argc, const char *arg)
{
    return ((argc == 2u)
            && (   (strcmp(arg, "--usage") == 0)
                || (strcmp(arg, "-u")      == 0)));
}

static void
regshow_usage(struct shell *shell)
{
    ufw_shell_fprintf(shell, "usage: regshow "
                             "[HANDLE : Integer] "
                             "[HANDLE : Integer]\n\n");
    ufw_shell_fprintf(
        shell,
        "When called with a single integer argument, it will print the\n"
        "referenced register's specification and also reach into storage\n"
        "to reveal its current value.\n\n"
        "When called with two integer arguments, the command prints the\n"
        "same information as it would with one When called with two integer\n"
        "arguments, the command prints the same information as with one ar-\n"
        "gument, but for all the register within the range if register IDs,\n"
        "including the range boundaries.\n");
}

/**
 * Shell command callback for the ‘regshow’ command
 *
 * When called with zero arguments, it prints the register table specification,
 * without accessing current data from storage backends.
 *
 * When called with a single integer argument, it will print the referenced
 * register's specification and also reach into storage to reveal its current
 * value. If the first argument indicates a cry for help, print usage message.
 *
 * When called with two integer arguments, the command prints the same
 * information as it would with one When called with two integer arguments, the
 * command prints the same information as with one argument, but for all the
 * register within the range if register IDs, including the range boundaries.
 *
 * Note that currently, read-access to storage is not serialised in this call
 * back. So if the application in question uses zephyr's preemptive scheduler,
 * there is no guarantee the data read is intact if a second thread writes to
 * it.
 *
 * @param  s          Pointer to the shell instance the command was issued in.
 * @param  argc       Number of entries in the argv parameter.
 * @param  argv       List of command line arguments. argv[0] is "regshow".
 *
 * @return void
 * @sideeffects Reads and prints register table data as specified.
 */
void
ufw_shell_regshow(const struct shell *s, size_t argc, char **argv)
{
    /*
     * Casting off the ‘const’. UFW's register utilities use an fprintf like
     * API for its printing backend, with the first argument being a void poin-
     * ter to allow handing in arbitrary destinations like FILE* or indeed
     * struct shell*. Keeping this const-correct is tough, because some prin-
     * ters like that either won't be able to have that argument const, or the
     * codebase just didn't bother with stiffer const-correctness.
     *
     * In any case, our printer callback ‘ufw_shell_fprintf()’ hands its desti-
     * nation parameter directly to ‘shell_vfprintf()’, which uses a const spe-
     * cification on its ‘struct shell*’ argument. So no harm done.
     */
    void *shell = (void*)s;

    if (want_help(argc, argv[1])) {
        regshow_usage(shell);
        return;
    }

    if (table == NULL) {
        ufw_shell_fprintf(shell, "No register table found.\n");
        return;
    }

    if (argc == 1u) {
        register_table_print(shell, "", table);
    } else if (argc == 2u) {
        print_range(shell, argv[1], argv[1]);
    } else if (argc == 3u) {
        print_range(shell, argv[1], argv[2]);
    } else {
        regshow_usage(shell);
    }
}

SHELL_CMD_REGISTER(regshow, NULL,
                   "Show register table item",
                   ufw_shell_regshow);
