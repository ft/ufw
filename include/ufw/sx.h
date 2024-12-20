/*
 * Copyright (c) 2020-2023 chip-remote workers, All rights reserved.
 * Copyright (c) 2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_SX_H_8528c541
#define INC_UFW_SX_H_8528c541

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum sx_status {
    SXS_SUCCESS,
    SXS_FOUND_LIST,
    SXS_BROKEN_INTEGER,
    SXS_BROKEN_SYMBOL,
    SXS_UNKNOWN_INPUT,
    SXS_UNEXPECTED_END
};

enum sx_node_type {
    SXT_SYMBOL,
    SXT_INTEGER,
    SXT_PAIR,
    SXT_EMPTY_LIST
};

struct sx_pair;

struct sx_node {
    enum sx_node_type type;
    union {
        uint64_t u64;
        char *symbol;
        struct sx_pair *pair;
    } data;
};

struct sx_pair {
    struct sx_node *car;
    struct sx_node *cdr;
};

struct sx_parse_result {
    size_t position;
    enum sx_status status;
    struct sx_node *node;
};

#define SX_PARSE_RESULT_INIT {          \
        .position = 0u,                 \
        .status = SXS_SUCCESS,          \
        .node = NULL }

typedef struct sx_node *(*sx_nodefnc)(struct sx_node*, void*);

#define SX_PARSER_INIT { .state = SXS_INIT, .error = SXE_NONE, .position = 0u }

struct sx_parse_result sx_parse_string(const char*);
struct sx_parse_result sx_parse_stringn(const char*, size_t);
struct sx_parse_result sx_parse(const char*, size_t, size_t);
struct sx_parse_result sx_parse_token(const char*, size_t, size_t);
void sx_destroy(struct sx_node**);

struct sx_node *sx_make_integer(uint64_t);
struct sx_node *sx_make_symbol(const char*);
struct sx_node *sx_make_empty_list(void);
struct sx_node *sx_cons(struct sx_node*, struct sx_node*);

struct sx_node *sx_cxr(struct sx_node*, const char*);
struct sx_node *sx_pop(struct sx_node**);
struct sx_node *sx_append(struct sx_node*, struct sx_node*);
void sx_foreach(struct sx_node*, sx_nodefnc, void*);

bool sx_is_list(struct sx_node*);

static inline bool
sx_is_integer(const struct sx_node *node)
{
    return node->type == SXT_INTEGER;
}

static inline bool
sx_is_the_integer(const struct sx_node *node, const uint64_t n)
{
    return sx_is_integer(node) && (node->data.u64 == n);
}

static inline bool
sx_is_null(const struct sx_node *node)
{
    return node->type == SXT_EMPTY_LIST;
}

static inline bool
sx_is_pair(const struct sx_node *node)
{
    return node->type == SXT_PAIR;
}

static inline bool
sx_is_symbol(const struct sx_node *node)
{
    return node->type == SXT_SYMBOL;
}

static inline bool
sx_is_the_symbol(const struct sx_node *node, const char *s)
{
    return sx_is_symbol(node) && (strcmp(node->data.symbol, s) == 0);
}

static inline struct sx_node *
sx_car_unsafe(struct sx_node *n)
{
    return n->data.pair->car;
}

static inline struct sx_node *
sx_cdr_unsafe(struct sx_node *n)
{
    return n->data.pair->cdr;
}

static inline struct sx_node *
sx_car(struct sx_node *n)
{
    return sx_is_pair(n) ? sx_car_unsafe(n) : NULL;
}

static inline struct sx_node *
sx_cdr(struct sx_node *n)
{
    return sx_is_pair(n) ? sx_cdr_unsafe(n) : NULL;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_SX_H_8528c541 */
