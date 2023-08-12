/*
 * Copyright (c) 2020-2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file memdiff.c
 * @brief Hexdump-like memory diffing utility
 *
 * Calculate position of two differences A and B.
 *
 * - If A is invalid, we are at the start of memory. Print up to context lines
 *   of context, depending on whether or not lineoffset of B is large enough.
 *
 * - If B is invalid, we are at the end of memory. Print up to context lines of
 *   context, depending on whether or not lineoffset of B is small enough.
 *
 * - Otherwise:
 *
 *     - If line offsets between the two is >= context*2, print context after
 *       A and before B.
 *     - otherwise, print everything between the two line offsets.
 *
 * Each iteration only produces *one* new difference position. That way,
 * producing the context, that is surrounding the differences becomes rather
 * straight forward.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ufw/bit-operations.h>
#include <ufw/compiler.h>
#include <ufw/toolchain.h>
#include <ufw/test/tap.h>

struct diffstate {
    const void *a;
    const void *b;
    const size_t size;
    const size_t columns;
    const size_t context;
    size_t position;
    size_t count;
};

struct difference {
    bool valid;
    size_t byte;
    size_t lineoffset;
};

typedef size_t(*hexdigitprinter)(const void*, const void*, size_t,
                                 unsigned char, size_t);
typedef size_t(*printableprinter)(const void*, const void*, size_t,
                                  unsigned char);

static const unsigned char digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f' };

static size_t bytes_to_print(struct diffstate*, size_t);
static void difflines(struct diffstate*, struct difference*);
static struct difference finddiff(struct diffstate*);
static size_t lineoffset(size_t, size_t);
static size_t nextline(size_t, size_t);

static void diffpostcontext(struct diffstate*, struct difference*);
static void diffprecontext(struct diffstate*, struct difference*);
static void printline(struct diffstate*, size_t);
static void printlines(struct diffstate*, size_t, size_t);
static void printskip(void);
static bool rundiff(struct diffstate*, struct difference*, struct difference*);

static size_t diffunderline(const void*, const void*, size_t, size_t, size_t);
static void diffoverline(const void*, const void*, size_t, size_t, size_t);

static size_t wprint_word_hex(const void*, const void*, size_t, size_t, size_t,
                              hexdigitprinter, printableprinter);

static size_t pp(const void*, const void*, size_t, unsigned char);
static size_t px(const void*, const void*, size_t, unsigned char, size_t);

static size_t pp_above(const void*, const void*, size_t, unsigned char);
static size_t px_above(const void*, const void*, size_t, unsigned char, size_t);

static size_t pp_below(const void*, const void*, size_t, unsigned char);
static size_t px_below(const void*, const void*, size_t, unsigned char, size_t);

static size_t
lineoffset(size_t byte, size_t columns)
{
    return byte - (byte % columns);
}

static size_t
nextline(size_t position, size_t columns)
{
    return lineoffset(position + columns, columns);
}

static struct difference
finddiff(struct diffstate *diff)
{
    const unsigned char *aptr = diff->a;
    const unsigned char *bptr = diff->b;
    struct difference rv;

    rv.valid = false;
    for (; diff->position < diff->size; ++diff->position) {
        if (aptr[diff->position] != bptr[diff->position]) {
            rv.valid = true;
            rv.byte = diff->position;
            rv.lineoffset = lineoffset(rv.byte, diff->columns);
            break;
        }
    }
    return rv;
}

static size_t
bytes_to_print(struct diffstate *diff, size_t offset)
{
    const size_t rest = diff->size - offset;
    return rest < diff->columns ? rest : diff->columns;
}

static void
difflines(struct diffstate *diff, struct difference *d)
{
    const size_t bytes = bytes_to_print(diff, d->lineoffset);

    printf("#  d:  %8s  ", "");
    diffoverline(diff->a, diff->b, d->lineoffset, bytes, diff->columns);
    printf("#  a:  %08lx  ", (long unsigned int)d->lineoffset);
    print_word_hex(diff->a, d->lineoffset, bytes, diff->columns);
    printf("#  b:  %08lx  ", (long unsigned int)d->lineoffset);
    print_word_hex(diff->b, d->lineoffset, bytes, diff->columns);
    printf("#  d:  %8s  ", "");
    diff->count +=
        diffunderline(diff->a, diff->b, d->lineoffset, bytes, diff->columns);
}

static void
printskip(void)
{
    printf("#      [...]\n");
}

static void
printline(struct diffstate *diff, size_t offset)
{
    const size_t bytes = bytes_to_print(diff, offset);
    printf("#      %08lx  ", (long unsigned int)offset);
    print_word_hex(diff->a, offset, bytes, diff->columns);
}

static void
printlines(struct diffstate *diff, size_t offset, size_t n)
{
    for (size_t i = 0u; i < n; ++i) {
        printline(diff, offset);
        offset = nextline(offset, diff->columns);
    }
}

static void
diffprecontext(struct diffstate *diff, struct difference *A)
{
    if (A->lineoffset <= (diff->context+1)*diff->columns) {
        printlines(diff, 0u, A->lineoffset/diff->columns);
    } else {
        printskip();
        printlines(diff, A->lineoffset - diff->context*diff->columns,
                   diff->context);
    }
}

static void
diffpostcontext(struct diffstate *diff, struct difference *A)
{
    if (diff->size < diff->columns || diff->size < diff->position)
        return;

    const size_t distance =
        nextline(diff->size - diff->columns, diff->columns)
        - nextline(A->lineoffset, diff->columns);
    const size_t limit = (diff->context+1)*diff->columns;
    if (distance <= limit) {
        printlines(diff, A->lineoffset+diff->columns,
                   (diff->size - A->lineoffset - 1)/diff->columns);
    } else {
        printlines(diff, A->lineoffset+diff->columns, diff->context);
        printskip();
    }
}

static bool
rundiff(struct diffstate *diff, struct difference *A, struct difference *B)
{
    if (A->valid == false) {
        /* First memory difference is in B */
        diffprecontext(diff, B);
        difflines(diff, B);
    } else if (B->valid == false) {
        /* Last memory difference is in A */
        diffpostcontext(diff, A);
        return false;
    } else {
        /* Both A and B mark different lines of differing memory */
        const size_t distance = B->lineoffset - A->lineoffset - diff->columns;
        /* Why the +1? Well, skipping lines is indicated by a line containing
         * "[...]". If we'd *only* be skipping one single like, though, we
         * might as well print it, too. */
        const size_t limit = diff->columns*(1 + 2*diff->context);
        if (distance <= limit) {
            const size_t offset = A->lineoffset+diff->columns;
            const size_t lines = distance/diff->columns;
            printlines(diff, offset, lines);
        } else {
            const size_t postoffset = A->lineoffset+diff->columns;
            const size_t preoffset = B->lineoffset-diff->columns*diff->context;
            printlines(diff, postoffset, diff->context);
            printskip();
            printlines(diff, preoffset, diff->context);
        }
        difflines(diff, B);
    }
    return true;
}

static size_t
px(UNUSED const void *a, UNUSED const void *b, UNUSED size_t offset,
   const unsigned char ch, const size_t idx)
{
    const size_t bits_per_digit = 4u;
    const size_t bitoffset = (idx-1) * bits_per_digit;
    const unsigned int digit = BIT_GET(ch, bits_per_digit, bitoffset);
    putchar(digits[digit]);
    return 0u;
}

static size_t
pr_if_diff(const void *a, const void *b, const size_t offset,
           const unsigned char ch)
{
    const unsigned char *aptr = a;
    const unsigned char *bptr = b;
    if (aptr[offset] == bptr[offset]) {
        putchar(' ');
        return 0u;
    } else {
        putchar(ch);
        return 1u;
    }
}

static size_t
px_above(const void *a, const void *b, const size_t offset,
         UNUSED const unsigned char ch, UNUSED const size_t idx)
{
    (void)pr_if_diff(a, b, offset, 'v');
    return 0u;
}

static size_t
px_below(const void *a, const void *b, size_t offset,
         UNUSED const unsigned char ch, UNUSED const size_t idx)
{
    (void)pr_if_diff(a, b, offset, '^');
    return 0u;
}

static size_t
pp(UNUSED const void *a, UNUSED const void *b, UNUSED size_t offset,
   unsigned char ch)
{
    if (isprint(ch) && (isspace(ch) == false))
        putchar(ch);
    else
        putchar('.');
    return 0u;
}

static size_t
pp_above(const void *a, const void *b, size_t offset,
         UNUSED const unsigned char ch)
{
    (void)pr_if_diff(a, b, offset, 'v');
    return 0u;
}

static size_t
pp_below(const void *a, const void *b, size_t offset,
         UNUSED const unsigned char ch)
{
    return pr_if_diff(a, b, offset, '^');
}

static size_t
diffunderline(const void *a, const void *b, size_t offset, size_t n,
              size_t columns)
{
    return wprint_word_hex(a, b, offset, n, columns, px_below, pp_below);
}

static void
diffoverline(const void *a, const void *b, size_t offset, size_t n,
             size_t columns)
{
    (void)wprint_word_hex(a, b, offset, n, columns, px_above, pp_above);
}

static size_t
wprint_word_hex(const void *memory, const void *aux, const size_t offset,
                const size_t bytes, size_t columns,
                hexdigitprinter hexdigit, printableprinter printable)
{
    const size_t bits_per_digit = 4u;
    const size_t bits_per_byte = (size_t)UFW_BITS_PER_BYTE;
    const size_t bits_per_word = bits_per_byte * bytes;
    const size_t digitsteps = bits_per_byte / bits_per_digit;
    const size_t steps = bits_per_word / bits_per_byte;
    size_t count = 0u;
    size_t step, pad;

    if (bytes > columns)
        columns = bytes;

    const unsigned char *ptr = memory;
    ptr += offset;
    /* Print hexadecimal data dump */
    for (step = 0u; step < steps; ++step) {
        if (step > 0 && (step % 8) == 0)
            putchar(' ');
        for (size_t j = digitsteps; j > 0; --j) {
            count += hexdigit(memory, aux, offset + step, *ptr, j);
        }
        ptr++;
        putchar(' ');
    }
    /* Pad to a given column width */
    if (bytes < columns) {
        size_t rest = columns - bytes;
        size_t spaces = step + rest;
        for (pad = step; pad < spaces; ++pad) {
            if (pad > 0 && (pad % 8) == 0)
                putchar(' ');
            for (size_t j = digitsteps; j > 0; --j) {
                putchar(' ');
            }
            ptr++;
            putchar(' ');
        }
    }
    /* Print printable-characters after data dump, similar to hexdump */
    fputs(" | ", stdout);
    ptr = memory;
    ptr += offset;
    for (step = 0u; step < bytes; ++step) {
        if (step > 0 && (step % 8) == 0)
            putchar(' ');
        count += printable(memory, aux, offset + step, *ptr);
        ptr++;
    }
    /* Pad printable-characters to a given column width as well */
    if (bytes < columns) {
        size_t rest = step + columns - bytes;
        for (pad = step; pad < rest; ++pad) {
            if (pad > 0 && (pad % 8) == 0)
                putchar(' ');
            putchar(' ');
        }
    }
    fputs(" |\n", stdout);
    return count;
}

void
print_word_hex(const void *memory, const size_t offset, const size_t bytes,
               const size_t columns)
{
    (void)wprint_word_hex(memory, NULL, offset, bytes, columns, px, pp);
}

size_t
memdiff(const void *a, const void *b, size_t n)
{
    struct difference A, B;
    struct diffstate diff = {
        .a = a, .b = b, .size = n,
        .columns = 16u, .context = 2u,
        .position = 0u, .count = 0u };

    A.valid = false;
    A.lineoffset = 0u;
    for (;;) {
        B = finddiff(&diff);
        if (rundiff(&diff, &A, &B) == false) {
            break;
        }
        diff.position = nextline(diff.position, diff.columns);
        A = B;
    }

    return diff.count;
}
