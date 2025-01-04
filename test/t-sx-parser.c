/*
 * Copyright (c) 2020-2023 chip-remote workers, All rights reserved.
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file t-sx-parser.c
 * @brief Unit tests for s-expression parser module
 */

/* See the src/registers/utilities.c for reasons to this. */
#include <ufw/toolchain.h>

#ifdef WITH_SYS_TYPES_H
#include <sys/types.h>
#endif /* WITH_SYS_TYPES_H */

#include <inttypes.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/sx.h>

#include <ufw/test/tap.h>

/* sx_parse_token() test cases */

static void
t_sx_parse_token_empty(void)
{
    struct sx_parse_result p = sx_parse_token("", 0, 0);
    ok(p.node == NULL, "Empty string parses to NULL");
    ok(p.status == SXS_SUCCESS, "Empty string does not indicate error");
}

static void
t_sx_parse_token_only_whitespace(void)
{
    struct sx_parse_result p = sx_parse_token(" \t   ", 5, 0);
    ok(p.node == NULL, "Just whitespace parses to NULL");
    ok(p.status == SXS_SUCCESS, "Just whitespace does not indicate error");
}

static void
t_sx_parse_token_symbol(void)
{
    struct sx_parse_result p = sx_parse_token("foobar", 6, 0);
    unless (ok(p.node != NULL, "foobar parses to non-NULL")) {
        goto cleanup;
    }
    ok(sx_is_the_symbol(p.node, "foobar"), "foobar parses to SXT_SYMBOL");
    ok(p.status == SXS_SUCCESS, "foobar does not indicate error");

cleanup:
    sx_destroy(&p.node);
}

static void
t_sx_parse_token_int_dec(void)
{
    struct sx_parse_result p = sx_parse_token("12345", 5, 0);
    unless (ok(p.node != NULL, "12345 parses to non-NULL")) {
        goto cleanup;
    }
    ok(sx_is_the_integer(p.node, 12345u), "12345 parses to SXT_INTEGER");
    ok(p.status == SXS_SUCCESS, "12345 does not indicate error");

cleanup:
    sx_destroy(&p.node);
}

static void
t_sx_parse_token_int_hex(void)
{
    struct sx_parse_result p = sx_parse_token("#x400", 5, 0);
    unless (ok(p.node != NULL, "#x400 parses to non-NULL")) {
        goto cleanup;
    }
    ok(sx_is_the_integer(p.node, 0x400u), "#x400 parses to SXT_INTEGER");
    ok(p.status == SXS_SUCCESS, "#x400 does not indicate error");

cleanup:
    sx_destroy(&p.node);
}

static void
t_sx_parse_token_error_dec(void)
{
    struct sx_parse_result p = sx_parse_token("1234a", 5, 0);
    ok(p.node == NULL, "1234a parses to NULL");
    ok(p.status == SXS_BROKEN_INTEGER, "1234a indicates integer-error");
    ok(p.position == 4, "...error at position 4 == %" PRIu64, p.position);
}

static void
t_sx_parse_token_error_hex(void)
{
    struct sx_parse_result p = sx_parse_token("#x12g", 5, 0);
    ok(p.node == NULL, "#x12g parses to NULL");
    ok(p.status == SXS_BROKEN_INTEGER, "#x12g indicates integer-error");
    ok(p.position == 4, "...error at position 4 == %" PRIu64, p.position);
}

static void
t_sx_parse_token_error_symbol(void)
{
    struct sx_parse_result p = sx_parse_token("foo{}bar", 5, 0);
    ok(p.node == NULL, "foo{}bar parses to NULL");
    ok(p.status == SXS_BROKEN_SYMBOL, "foo{}bar indicates symbol-error");
    ok(p.position == 3, "...error at position 3 == %" PRIu64, p.position);
}

/* sx_parse_string() test cases */

static void
t_sx_parse_empty_list(void)
{
    struct sx_parse_result p = sx_parse_string("()");
    unless (ok(p.node != NULL, "() parses to non-NULL")) {
        goto cleanup;
    }
    ok(sx_is_null(p.node), "() parses to SXT_EMPTY_LIST");
    ok(p.status == SXS_SUCCESS, "() does not indicate an error");

cleanup:
    sx_destroy(&p.node);
}

static void
t_sx_parse_empty_one_elem_list(void)
{
    struct sx_parse_result p = sx_parse_string("(1)");
    unless (ok(p.node != NULL, "(1) parses to non-NULL")) {
        goto cleanup;
    }
    unless (ok(sx_is_pair(p.node), "(1) parses to pair")) {
        pru16(p.node->type, SXT_PAIR);
        goto cleanup;
    }
    ok(sx_is_the_integer(sx_car(p.node), 1), "car of (1) is the integer 1");
    unless (ok(sx_is_null(sx_cdr(p.node)), "cdr of (1) is the empty list")) {
        pru16(sx_cdr(p.node)->type, SXT_EMPTY_LIST);
    }
    ok(p.status == SXS_SUCCESS, "(1) indicates an success");

cleanup:
    sx_destroy(&p.node);
}

static void
t_sx_parse_empty_two_elem_list(void)
{
    struct sx_parse_result p = sx_parse_string("(1 2)");
    ok(p.status == SXS_SUCCESS, "(1 2) does not indicate an error");
    unless (ok(p.node != NULL, "(1 2) parses to non-NULL")) {
        goto cleanup;
    }
    unless (ok(p.node->type == SXT_PAIR, "(1 2) parses to pair")) {
        pru16(p.node->type, SXT_PAIR);
        goto cleanup;
    }
    ok(sx_is_the_integer(sx_car(p.node), 1), "car of (1 2) is the integer 1");
    unless (ok(sx_is_pair(sx_cdr(p.node)), "cdr of (1 2) parses to pair")) {
        pru16(p.node->data.pair->cdr->type, SXT_PAIR);
        goto cleanup;
    }
    ok(sx_is_the_integer(sx_car(sx_cdr(p.node)), 2),
       "cadr of (1 2) is the integer 2");
    ok(sx_is_null(sx_cdr(sx_cdr(p.node))), "cddr of (1 2) is the empty list");

cleanup:
    sx_destroy(&p.node);
}

static void
t_sx_parse_incomplete_list(void)
{
    struct sx_parse_result p = sx_parse_string("(");
    ok(p.status == SXS_UNEXPECTED_END, "( signals unexpected end");
    ok(p.node == NULL, "( returns NULL");

    p = sx_parse_string("(1 2");
    ok(p.status == SXS_UNEXPECTED_END, "(1 2 signals unexpected end");
    ok(p.node == NULL, "(1 2 returns NULL");

    p = sx_parse_string("(foobar (stuff) (1 2)");
    /*                   0123456789012345678901
     *                   0000000000111111111122 */
    ok(p.status == SXS_UNEXPECTED_END, "(foobar (stuff) (1 2) signals unexpected end");
    ok(p.node == NULL, "(foobar (stuff) (1 2) returns NULL");
}

static void
t_cxr(void)
{
    const char *expr = "((1 (a b c) 3) (q w e) r t (5) 6)";
    /*                  0123456789012345678901234567890123
     *                  0000000000111111111122222222223333 */
    struct sx_parse_result p = sx_parse_string(expr);
    struct sx_node *n;
    unless(ok(p.status == SXS_SUCCESS, "%s signals success", expr)) {
        pru16(p.status, SXS_SUCCESS);
    }

    n = sx_cxr(p.node, "aa");
    unless (ok(n != NULL, "caar of %s does not return NULL", expr)) {
        goto cleanup;
    }
    ok(sx_is_the_integer(n, 1), "caar of %s is the integer 1", expr);

    n = sx_cxr(p.node, "adada");
    unless (ok(n != NULL, "cadadar of %s does not return NULL", expr)) {
        goto cleanup;
    }
    ok(sx_is_the_symbol(n, "b"), "cadadar of %s is the symbol b", expr);

    n = sx_cxr(p.node, "aad");
    unless (ok(n != NULL, "caadr of %s does not return NULL", expr)) {
        goto cleanup;
    }
    ok(sx_is_the_symbol(n, "q"), "caadr of %s is the symbol q", expr);

    n = sx_cxr(p.node, "addd");
    unless (ok(n != NULL, "cadddr of %s does not return NULL", expr)) {
        goto cleanup;
    }
    ok(sx_is_the_symbol(n, "t"), "cadddr of %s is the symbol t", expr);

    n = sx_cxr(p.node, "aadddd");
    unless (ok(n != NULL, "caaddddr of %s does not return NULL", expr)) {
        goto cleanup;
    }
    ok(sx_is_the_integer(n, 5), "caaddddr of %s is the integer 5", expr);

    n = sx_cxr(p.node, "addddd");
    unless (ok(n != NULL, "cadddddr of %s does not return NULL", expr)) {
        goto cleanup;
    }
    ok(sx_is_the_integer(n, 6), "cadddddr of %s is the integer 6", expr);

    n = sx_cxr(p.node, "dddddd");
    unless (ok(n != NULL, "cddddddr of %s does not return NULL", expr)) {
        goto cleanup;
    }
    ok(sx_is_null(n), "cddddddr of %s is the empty list", expr);

cleanup:
    sx_destroy(&p.node);
}

static void
t_pop(void)
{
    /* Make a tree by parsing this expression, then traverse it using sx_pop,
     * inspecting all that is revealed. sx_pop makes sure you only have to
     * destroy the elements that have been returned to you. Thus, running this
     * test-suite in a memory tester like valgrind should still show that no
     * memory was leaked. */
    const char *expr = "((1 (a b c) 3) (q w e) r t (5) 6)";
    /*                  0123456789012345678901234567890123
     *                  0000000000111111111122222222223333 */
    struct sx_parse_result p = sx_parse_string(expr);
    struct sx_node *n;

    unless(ok(p.status == SXS_SUCCESS, "%s signals success", expr)) {
        pru16(p.status, SXS_SUCCESS);
    }

    /* lst1 is (1 (a b c) 3) */
    struct sx_node *lst1 = sx_pop(&p.node);
    ok(sx_is_list(lst1), "pop reveals a list");
    n = sx_pop(&lst1);
    ok(sx_is_the_integer(n, 1), "pop reveals the integer 1");
    sx_destroy(&n);

    /* lst2 is (a b c) */
    struct sx_node *lst2 = sx_pop(&lst1);
    ok(sx_is_list(lst2), "pop reveals a list");
    n = sx_pop(&lst2);
    ok(sx_is_the_symbol(n, "a"), "pop reveals the symbol a");
    sx_destroy(&n);
    n = sx_pop(&lst2);
    ok(sx_is_the_symbol(n, "b"), "pop reveals the symbol b");
    sx_destroy(&n);
    n = sx_pop(&lst2);
    ok(sx_is_the_symbol(n, "c"), "pop reveals the symbol c");
    sx_destroy(&n);
    ok(sx_is_null(lst2), "pop made lst2 into the empty list");
    n = sx_pop(&lst2);
    ok(sx_is_null(n), "pop reveals the empty list");
    sx_destroy(&n);

    n = sx_pop(&lst1);
    ok(sx_is_the_integer(n, 3), "pop reveals the integer 1");
    sx_destroy(&n);
    ok(sx_is_null(lst1), "pop made lst1 into the empty list");
    n = sx_pop(&lst1);
    ok(sx_is_null(n), "pop reveals the empty list");
    sx_destroy(&n);

    /* lst3 is (q w e) */
    struct sx_node *lst3 = sx_pop(&p.node);
    ok(sx_is_list(lst3), "pop reveals a list");
    n = sx_pop(&lst3);
    ok(sx_is_the_symbol(n, "q"), "pop reveals the symbol q");
    sx_destroy(&n);
    n = sx_pop(&lst3);
    ok(sx_is_the_symbol(n, "w"), "pop reveals the symbol w");
    sx_destroy(&n);
    n = sx_pop(&lst3);
    ok(sx_is_the_symbol(n, "e"), "pop reveals the symbol e");
    sx_destroy(&n);
    ok(sx_is_null(lst3), "pop made lst3 into the empty list");
    n = sx_pop(&lst3);
    ok(sx_is_null(n), "pop reveals the empty list");
    sx_destroy(&n);

    /* Now on to r and t from p.node */
    n = sx_pop(&p.node);
    ok(sx_is_the_symbol(n, "r"), "pop reveals the symbol r");
    sx_destroy(&n);
    n = sx_pop(&p.node);
    ok(sx_is_the_symbol(n, "t"), "pop reveals the symbol t");
    sx_destroy(&n);

    /* lst4 is (5) */
    struct sx_node *lst4 = sx_pop(&p.node);
    ok(sx_is_list(lst4), "pop reveals a list");
    n = sx_pop(&lst4);
    ok(sx_is_the_integer(n, 5), "pop reveals the integer 5");
    sx_destroy(&n);
    ok(sx_is_null(lst4), "pop made lst4 into the empty list");
    n = sx_pop(&lst4);
    ok(sx_is_null(n), "pop reveals the empty list");
    sx_destroy(&n);

    /* p.node is now (6) */
    n = sx_pop(&p.node);
    ok(sx_is_the_integer(n, 6), "pop reveals the integer 6");
    sx_destroy(&n);
    ok(sx_is_null(p.node), "pop made lst4 into the empty list");
    n = sx_pop(&p.node);
    ok(sx_is_null(n), "pop reveals the empty list");
    sx_destroy(&n);
}

struct fnc_t_data {
    uint64_t cnt;
    uint64_t errors;
    uint64_t expect;
};

static struct sx_node *
fnc_t_append(struct sx_node *node, void *arg)
{
    struct fnc_t_data *data = arg;
    data->cnt++;
    unless (ok(sx_is_integer(node), "...node is an integer")) {
        data->errors++;
        return node;
    }
    unless (ok(node->data.u64 == data->expect,
               "...node has expected value (%"PRIu64" == %"PRIu64")",
               node->data.u64, data->expect))
    {
        data->errors++;
    }
    data->expect++;
    return node;
}

static void
t_append(void)
{
    struct fnc_t_data data = { .cnt = 0, .errors = 0, .expect = 1 };
    struct sx_parse_result pa = sx_parse_string("(1 2 3)");
    struct sx_parse_result pb = sx_parse_string("(4 5 6)");
    ok(pa.status == SXS_SUCCESS, "parsing (1 2 3) returned successfully");
    ok(sx_is_pair(pa.node), "parsing (1 2 3) returns a pair");
    ok(sx_is_list(pa.node), "parsing (1 2 3) returns a list too");
    ok(pb.status == SXS_SUCCESS, "parsing (4 5 6) returned successfully");
    ok(sx_is_pair(pb.node), "parsing (4 5 6) returns a pair");
    ok(sx_is_list(pb.node), "parsing (4 5 6) returns a list too");
    struct sx_node *lst = sx_append(pa.node, pb.node);
    unless (ok(lst != NULL, "appending lists returned a non-NULL value")) {
        goto cleanup;
    }
    sx_foreach(lst, fnc_t_append, &data);
    ok(data.cnt == 6, "appended list has six elements");
    ok(data.errors == 0, "elements of appended list look the way they should");
    sx_destroy(&lst);

    lst = sx_append(sx_make_empty_list(),
                    sx_cons(sx_make_integer(1),
                            sx_cons(sx_make_integer(2), sx_make_empty_list())));
    ok(sx_is_list(lst), "(append '() '(....)) is a list");
    ok(sx_is_the_integer(sx_cxr(lst, "a"), 1), "(car lst) → 1");
    ok(sx_is_the_integer(sx_cxr(lst, "ad"), 2), "(cadr lst) → 2");
    ok(sx_is_null(sx_cxr(lst, "dd")), "(cddr lst) → '()");
    sx_destroy(&lst);

    lst = sx_append(sx_cons(sx_make_integer(1),
                            sx_cons(sx_make_integer(2), sx_make_empty_list())),
                    sx_make_empty_list());
    ok(sx_is_list(lst), "(append '(....) '()) is a list");
    ok(sx_is_the_integer(sx_cxr(lst, "a"), 1), "(car lst) → 1");
    ok(sx_is_the_integer(sx_cxr(lst, "ad"), 2), "(cadr lst) → 2");
    ok(sx_is_null(sx_cxr(lst, "dd")), "(cddr lst) → '()");

cleanup:
    sx_destroy(&lst);
}

static void
t_make_things(void)
{
    struct sx_node *t_int = sx_make_integer(1234567890);
    struct sx_node *t_sym = sx_make_symbol("foobarbaz");
    struct sx_node *t_eol = sx_make_empty_list();
    struct sx_node *t_lst = sx_cons(t_int, sx_cons(t_sym, t_eol));
    ok(sx_is_the_integer(t_int, 1234567890), "t_int is 1234567890");
    ok(sx_is_the_symbol(t_sym, "foobarbaz"), "t_sym is 'foobarbaz'");
    ok(sx_is_null(t_eol), "t_eol is the empty list");
    ok(sx_is_list(t_lst), "t_lst is a list");
    ok(sx_is_the_integer(sx_car(t_lst), 1234567890),
       "(car t_lst) is 1234567890");
    ok(sx_is_the_symbol(sx_car(sx_cdr(t_lst)), "foobarbaz"),
       "(car (cdr t_lst)) is 'foobarbaz'");
    ok(sx_is_null(sx_cxr(t_lst, "dd")), "(cdr (cdr t_lst)) is the empty list");
    sx_destroy(&t_lst);
}

static void
t_sx_parse_token(void)
{
    t_sx_parse_token_empty();
    t_sx_parse_token_only_whitespace();
    t_sx_parse_token_symbol();
    t_sx_parse_token_int_dec();
    t_sx_parse_token_int_hex();
    t_sx_parse_token_error_dec();
    t_sx_parse_token_error_hex();
    t_sx_parse_token_error_symbol();
}

static void
t_sx_parse(void)
{
    t_sx_parse_empty_list();
    t_sx_parse_empty_one_elem_list();
    t_sx_parse_empty_two_elem_list();
    t_sx_parse_incomplete_list();
}

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(121);

    t_sx_parse_token();
    t_sx_parse();

    t_cxr();
    t_pop();
    t_append();

    t_make_things();

    return EXIT_SUCCESS;
}
