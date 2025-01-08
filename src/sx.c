/*
 * Copyright (c) 2020-2023 chip-remote workers, All rights reserved.
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup protosexp Simple S-Expression Parser
 * @{
 *
 * @file sx.c
 * @brief Simple S-Expression Parser
 *
 * This module implements an S-Expression parser, for use in instrumentation
 * of the chip-remote firmware in native builds. As such it is allowed more
 * liberal use of dynamically allocated memory.
 *
 * It is a very small subset of scheme's s-expressions:
 *
 *   - Lists              (exp exp exp ...)
 *   - Symbols            foobar
 *   - Unsigned Integers  decimal: `1234`, hex: `#x1234`
 *
 * That's it. Lists can be nested, so this allows for arbitrarily complex
 * structures.
 *
 * @}
 */

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <ufw/compat/strings.h>
#include <ufw/compiler.h>
#include <ufw/sx.h>

enum sx_what {
    LOOKING_AT_UNKNOWN,
    LOOKING_AT_SYMBOL,
    LOOKING_AT_INT_DEC,
    LOOKING_AT_INT_HEX,
    LOOKING_AT_PAREN_OPEN,
    LOOKING_AT_PAREN_CLOSE
};

static char *digits = "0123456789abcdef";

static inline uint64_t
minu64(const uint64_t a, const uint64_t b)
{
    return a < b ? a : b;
}

static inline uint64_t
digit2int(const char c)
{
    uint64_t rv = 0u;
    while (rv < 16) {
        if (digits[rv] == c)
            return rv;
        rv++;
    }
    return 0;
}

static const char *syminitchtab =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "+%|/_:;.!?$&=*<>~";

static bool
issyminitch(const char c)
{
    return strchr(syminitchtab, c) != NULL;
}

static bool
issymch(const char c)
{
    return issyminitch(c) || isdigit(c) || c == '-';
}

static void NORETURN
sxoom(const char *f, const int n)
{
    fprintf(stderr, "%s:%d: Could not allocate memory!\n", f, n);
    _Exit(1);
}

static struct sx_node *
make_node(void)
{
    struct sx_node *node = malloc(sizeof *node);
    if (node == NULL) {
        sxoom(__FILE__, __LINE__);
    }
    return node;
}

struct sx_node *
sx_make_integer(uint64_t n)
{
    struct sx_node *node = make_node();
    node->type = SXT_INTEGER;
    node->data.u64 = n;
    return node;
}

struct sx_node *
sx_make_empty_list(void)
{
    struct sx_node *node = make_node();
    node->type = SXT_EMPTY_LIST;
    return node;
}

static struct sx_node *
sx_make_symboln(const char *s, size_t len)
{
    const size_t n = len + 1;
    struct sx_node *node = make_node();
    node->type = SXT_SYMBOL;
    node->data.symbol = calloc(n, sizeof(char));
    if (node->data.symbol == NULL) {
        sxoom(__FILE__, __LINE__);
    }
    strlcpy(node->data.symbol, s, n);
    return node;
}

struct sx_node *
sx_make_symbol(const char *s)
{
    return sx_make_symboln(s, strlen(s));
}

static bool
nextisdelimiter(const char c)
{
    return c == '(' || c == ')' || isspace(c);
}

static struct sx_node *
parse_symbol(const char *s, const size_t n, size_t *i)
{
    size_t j = *i;

    while (j < n) {
        if (issymch(s[j]) == false)
            break;
        j++;
    }

    if ((j < n) && (nextisdelimiter(s[j]) == false)) {
        *i = j;
        return NULL;
    }

    struct sx_node *rv = sx_make_symboln(s + *i, j - *i);
    *i = minu64(j, n);

    return rv;
}

static struct sx_node *
parse_integer_(const char *s, const size_t n, size_t *i, size_t offset,
               int(*digitpredicate)(int), uint64_t base)
{
    size_t j = *i + offset;

    while (j < n) {
        if (digitpredicate(s[j]) == false)
            break;
        j++;
    }

    if ((j < n) && (nextisdelimiter(s[j]) == false)) {
        *i = j;
        return NULL;
    }

    uint64_t newi = 0u;
    size_t mult = 1u;
    size_t save = j;
    j--;
    while (j >= *i) {
        newi += mult * digit2int(s[j]);
        mult *= base;
        if (j == 0u)
            break;
        j--;
    }
    *i = minu64(save, n);

    return sx_make_integer(newi);
}

static inline struct sx_node *
parse_integer(const char *s, const size_t n, size_t *i)
{
    return parse_integer_(s, n, i, 0u, isdigit, 10u);
}

static inline struct sx_node *
parse_hinteger(const char *s, const size_t n, size_t *i)
{
    return parse_integer_(s, n, i, 2u, isxdigit, 16u);
}

static struct sx_node *
make_pair(void)
{
    struct sx_node *rv = make_node();

    rv->data.pair = calloc(1u, sizeof *rv->data.pair);
    if (rv->data.pair == NULL) {
        sxoom(__FILE__, __LINE__);
    }

    rv->type = SXT_PAIR;
    return rv;
}

static enum sx_what
looking_at(const char *s, const size_t n, const size_t i)
{
    if ((n > i + 1) && s[i] == '#' && s[i+1] == 'x' && isxdigit((int)s[i+2])) {
        return LOOKING_AT_INT_HEX;
    }
    if (s[i] == '(') {
        return LOOKING_AT_PAREN_OPEN;
    }
    if (s[i] == ')') {
        return LOOKING_AT_PAREN_CLOSE;
    }
    if (isdigit((int)s[i])) {
        return LOOKING_AT_INT_DEC;
    }
    if (issyminitch(s[i])) {
        return LOOKING_AT_SYMBOL;
    }

    return LOOKING_AT_UNKNOWN;
}

void
sx_destroy(struct sx_node **n)
{
    struct sx_node *node = *n;
    if (node == NULL)
        return;

    if (node->type == SXT_PAIR) {
        sx_destroy(&node->data.pair->car);
        sx_destroy(&node->data.pair->cdr);
        free(node->data.pair);
        node->data.pair = NULL;
    } else if (node->type == SXT_SYMBOL) {
        free(node->data.symbol);
        node->data.symbol = NULL;
    }

    free(node);
    *n = NULL;
}

static size_t
skip_ws(const char *s, const size_t n, size_t i)
{
    while (i < n && isspace((int)s[i])) {
        i += 1;
    }

    return i;
}

struct sx_parse_result
sx_parse_token(const char *s, const size_t n, const size_t i)
{
    struct sx_parse_result rv = SX_PARSE_RESULT_INIT;
    size_t j = skip_ws(s, n, i);

    if (j == n)
        return rv;


    switch (looking_at(s, n, j)) {
    case LOOKING_AT_INT_DEC:
        rv.node = parse_integer(s, n, &j);
        if (rv.node == NULL)
            rv.status = SXS_BROKEN_INTEGER;
        break;
    case LOOKING_AT_INT_HEX:
        rv.node = parse_hinteger(s, n, &j);
        if (rv.node == NULL)
            rv.status = SXS_BROKEN_INTEGER;
        break;
    case LOOKING_AT_SYMBOL:
        rv.node = parse_symbol(s, n, &j);
        if (rv.node == NULL)
            rv.status = SXS_BROKEN_SYMBOL;
        break;
    case LOOKING_AT_PAREN_OPEN:
        rv.status = SXS_FOUND_LIST;
        j++;
        break;
    case LOOKING_AT_PAREN_CLOSE:
        rv.node = sx_make_empty_list();
        j++;
        break;
    case LOOKING_AT_UNKNOWN:
        /* FALLTHROUGH */
    default:
        rv.status = SXS_UNKNOWN_INPUT;
        break;
    }

    rv.position = j;
    return rv;
}

static inline bool
result_is_empty_listp(const struct sx_parse_result *res)
{
    return (res->status == SXS_SUCCESS && res->node->type == SXT_EMPTY_LIST);
}

static inline bool
result_is_error(const struct sx_parse_result *res)
{
    return (res->status != SXS_SUCCESS && res->status != SXS_FOUND_LIST);
}

static struct sx_parse_result sx_parse_(const char*, size_t, size_t);
static struct sx_parse_result sx_parse_list(const char*, size_t, size_t);

static struct sx_parse_result
sx_parse_list(const char *s, const size_t n, const size_t i)
{
    if (i >= n) {
        struct sx_parse_result rv = SX_PARSE_RESULT_INIT;
        rv.status = SXS_UNEXPECTED_END;
        return rv;
    }
    struct sx_parse_result carres = sx_parse_(s, n, i);
    if (result_is_empty_listp(&carres) || result_is_error(&carres)) {
        return carres;
    }

    struct sx_parse_result cdrres = sx_parse_list(s, n, carres.position);
    struct sx_node *cons = sx_cons(carres.node, cdrres.node);

    carres.node = cons;
    carres.position = cdrres.position;
    carres.status = cdrres.status;

    return carres;
}

static struct sx_parse_result
sx_parse_(const char *s, const size_t n, const size_t i)
{
    struct sx_parse_result rv = sx_parse_token(s, n, i);
    if (rv.status == SXS_FOUND_LIST) {
        return sx_parse_list(s, n, rv.position);
    }
    if (i >= n && rv.node == NULL) {
        rv.status = SXS_UNEXPECTED_END;
        return rv;
    }
    return rv;
}

struct sx_parse_result
sx_parse(const char *s, const size_t n, const size_t i)
{
    struct sx_parse_result rv = sx_parse_(s, n, i);
    if (result_is_error(&rv)) {
        sx_destroy(&rv.node);
    }
    return rv;
}

struct sx_parse_result
sx_parse_stringn(const char *s, const size_t n)
{
    return sx_parse(s, n, 0);
}

struct sx_parse_result
sx_parse_string(const char *s)
{
    return sx_parse_stringn(s, strlen(s));
}

struct sx_node *
sx_cxr(struct sx_node *root, const char *addr)
{
    struct sx_node *ptr = root;
    size_t i = strlen(addr);
    if (i == 0)
        return root;

    --i;
    for (;;) {
        if (sx_is_pair(ptr) == false) {
            return NULL;
        }
        switch (addr[i]) {
        case 'a':
            ptr = sx_car_unsafe(ptr);
            break;
        case 'd':
            ptr = sx_cdr_unsafe(ptr);
            break;
        default:
            return NULL;
        }
        if (i == 0u)
            break;
        --i;
    }

    return ptr;
}

struct sx_node *
sx_pop(struct sx_node **root)
{
    struct sx_node *node = *root;

    if (node == NULL)
        return NULL;

    if (sx_is_pair(node) == false) {
        *root = NULL;
        return node;
    }

    *root = sx_cdr_unsafe(node);
    struct sx_node *car = sx_car_unsafe(node);
    free(node->data.pair);
    free(node);

    return car;
}

bool
sx_is_list(struct sx_node *n)
{
    struct sx_node *ptr = n;
    for (;;) {
        if (sx_is_null(ptr))
            return true;
        if (sx_is_pair(ptr) == false)
            break;
        ptr = sx_cdr_unsafe(ptr);
    }
    return false;
}

struct sx_node *
sx_append(struct sx_node *a, struct sx_node *b)
{
    if (a == NULL || b == NULL)
        return NULL;

    if (sx_is_null(a)) {
        sx_destroy(&a);
        return b;
    }

    if (sx_is_null(b)) {
        sx_destroy(&b);
        return a;
    }

    if ((sx_is_pair(a) && sx_is_pair(b)) == false)
        return NULL;

    struct sx_pair *ptr = a->data.pair;
    while (sx_is_pair(ptr->cdr)) {
        ptr = ptr->cdr->data.pair;
    }

    sx_destroy(&ptr->cdr);
    ptr->cdr = b;

    return a;
}

void
sx_foreach(struct sx_node *node, sx_nodefnc f, void *arg)
{
    struct sx_node *ptr = node;

    if (sx_is_pair(ptr) == false)
        return;

    while (sx_is_pair(ptr)) {
        f(ptr->data.pair->car, arg);
        ptr = sx_cdr_unsafe(ptr);
    }
}

struct sx_node *
sx_cons(struct sx_node *car, struct sx_node *cdr)
{
    struct sx_node *cons = make_pair();
    cons->data.pair->car = car;
    cons->data.pair->cdr = cdr;
    return cons;
}
