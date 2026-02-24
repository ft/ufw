/*
 * Copyright (c) 2020-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup testing Unit Testing Framework
 * @{
 *
 * @file memdiff.c
 * @brief Hexdump-like memory diffing utility
 *
 * @}
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/toolchain.h>

#include <ufw/bit-operations.h>

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

static size_t bytes_to_print(struct diffstate *diff, size_t offset);
static void difflines(struct diffstate *diff, struct difference *d);
static struct difference finddiff(struct diffstate *diff);
static size_t lineoffset(size_t byte, size_t columns);
static size_t nextline(size_t position, size_t columns);

static void diffpostcontext(struct diffstate *diff, struct difference *A);
static void diffprecontext(struct diffstate *diff, struct difference *A);
static void printline(struct diffstate *diff, size_t offset);
static void printlines(struct diffstate *diff, size_t offset, size_t n);
static void printskip(void);
static bool rundiff(struct diffstate *diff, struct difference *A,
                    struct difference *B);

static size_t diffunderline(const void *a, const void *b, size_t offset,
                            size_t n, size_t columns);
static void diffoverline(const void *a, const void *b, size_t offset,
                         size_t n, size_t columns);

static size_t wprint_chunk_hex(const void *memory, const void *aux,
                               size_t offset, size_t bytes, size_t columns,
                               hexdigitprinter hexdigit,
                               printableprinter printable);

static size_t pp(const void *a, const void *b, size_t offset,
                 unsigned char ch);
static size_t px(const void *a, const void *b, size_t offset,
                 unsigned char ch, size_t idx);

static size_t pp_above(const void *a, const void *b, size_t offset,
                       unsigned char ch);
static size_t px_above(const void *a, const void *b, size_t offset,
                       unsigned char ch, size_t idx);

static size_t pp_below(const void *a, const void *b, size_t offset,
                       unsigned char ch);
static size_t px_below(const void *a, const void *b, size_t offset,
                       unsigned char ch, size_t idx);

/**
 * Return the byte position of the beginning of a line of output
 *
 * Given a byte position and the number of bytes in a line, return the position
 * of the first byte in the line that the given byte will be printed in.
 *
 * @param  byte     Byte position
 * @param  columns  Number of bytes per line
 *
 * @return Byte position of the beginning of the line of output.
 * @sideeffects None
 */
static size_t
lineoffset(size_t byte, size_t columns)
{
    return byte - (byte % columns);
}

/**
 * Like lineoffset() but returns the position of the beginning of the next line
 *
 * @param  position  Byte position
 * @param  columns   Number of bytes per line
 *
 * @return Byte position of the beginning of the next line of output.
 * @sideeffects None
 */
static size_t
nextline(size_t position, size_t columns)
{
    return lineoffset(position + columns, columns);
}

/**
 * Find the next difference starting at given diffstate
 *
 * Walk through diff->a and diff->b in lockstep and return a difference
 * instance, when a difference between two associated bytes is detected.
 *
 * If no difference can be detected, the valid member shall be set to false.
 * True otherwise.
 *
 * @param  diff  The diff engine state to start running with
 *
 * @return struct difference instance; see above.
 * @sideeffects None
 */
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

/**
 * Calculate number of bytes to print given a position
 *
 * If the there is less data left than bytes per line being printed, return
 * that rest. Otherwise return the configured bytes per line (columns).
 *
 * @param  diff    The diffstate to use for reference
 * @param  offset  The current position into diff's data
 *
 * @return Number of bytes to print
 * @sideeffects None
 */
static size_t
bytes_to_print(struct diffstate *diff, size_t offset)
{
    const size_t rest = diff->size - offset;
    return rest < diff->columns ? rest : diff->columns;
}

/**
 * Produce output if differences in a line of data is detected
 *
 * See pr_if_diff() for an example of what this looks like.
 *
 * @param  diff  The diff engine state to start running with
 * @param  d     Description of difference that was encountered
 *
 * @sideeffects Prints via stdio, and modifies "diff".
 */
static void
difflines(struct diffstate *diff, struct difference *d)
{
    const size_t bytes = bytes_to_print(diff, d->lineoffset);

    printf("#  d:  %8s  ", "");
    diffoverline(diff->a, diff->b, d->lineoffset, bytes, diff->columns);
    printf("#  a:  %08lx  ", (long unsigned int)d->lineoffset);
    print_chunk_hex(diff->a, d->lineoffset, bytes, diff->columns);
    printf("#  b:  %08lx  ", (long unsigned int)d->lineoffset);
    print_chunk_hex(diff->b, d->lineoffset, bytes, diff->columns);
    printf("#  d:  %8s  ", "");
    diff->count +=
        diffunderline(diff->a, diff->b, d->lineoffset, bytes, diff->columns);
}

/**
 * Print a continuation marker
 *
 * This indicates to the user, that a chunk of memory that contained no
 * differences was skipped in output.
 *
 * @sideeffects Prints via stdio.
 */
static void
printskip(void)
{
    printf("#      [...]\n");
}

/**
 * Print a line of diff output
 *
 * This prints the address of the chunk that is being presented, followed by
 * the chunk itself, as well as a hexdump-like ASCII-printable preview. This
 * also adds markers, for positions of differences.
 *
 * Almost all of this is powered by print_chunk_hex() which itself is a wrapper
 * for wprint_chunk_hex().
 *
 * @sideeffects Prints via stdio.
 */
static void
printline(struct diffstate *diff, size_t offset)
{
    const size_t bytes = bytes_to_print(diff, offset);
    printf("#      %08lx  ", (long unsigned int)offset);
    print_chunk_hex(diff->a, offset, bytes, diff->columns);
}

/**
 * Produce a number of lines of outputs from a given offset
 *
 * This function assumes, that "offset" points to a byte that would be printed
 * at the beginning of a line of diff output. With that, it calls printline() n
 * times to produce another line of output.
 *
 * @param  diff    State of the diffing engine
 * @param  offset  Offset into memory referenced by "diff".
 * @param  n       Number of lines of output to produce.
 *
 * @sideeffects Prints via stdio.
 */
static void
printlines(struct diffstate *diff, size_t offset, size_t n)
{
    for (size_t i = 0U; i < n; ++i) {
        printline(diff, offset);
        offset = nextline(offset, diff->columns);
    }
}

/**
 * Print unified-diff-like context for before a difference
 *
 * @param  diff  The diff engine state to start running with
 * @param  A     Description of difference that was encountered
 *
 * @sideeffects Prints via stdio
 */
static void
diffprecontext(struct diffstate *diff, struct difference *A)
{
    if (A->lineoffset <= (diff->context+1)*diff->columns) {
        printlines(diff, 0U, A->lineoffset / diff->columns);
    } else {
        printskip();
        printlines(diff, A->lineoffset - (diff->context*diff->columns),
                   diff->context);
    }
}

/**
 * Print unified-diff-like context for after a difference
 *
 * @param  diff  The diff engine state to start running with
 * @param  A     Description of difference that was encountered
 *
 * @sideeffects Prints via stdio
 */
static void
diffpostcontext(struct diffstate *diff, struct difference *A)
{
    if (diff->size < diff->columns || diff->size < diff->position) {
        return;
    }

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

/**
 * Process a difference marker
 *
 * This is the main processor of the diffing engine. It is being called with
 * the state of the diffing process, a description of the previous encountered
 * difference and the new difference that is being processed. The system needs
 * information about the previous difference to make decisions such as printing
 * continuation lines, context etc.
 *
 * The procedure returns a boolean that indicates whether or not it has been
 * detected that processing can be stopped because it is done.
 *
 * @param  diff  State of the diffing engine
 * @param  A     Description of previously encountered difference in data
 * @param  B     Description of newly encountered difference in data
 *
 * @return True if processing is done; false otherwise.
 * @sideeffects The procedure prints via stdio and modifies "diff".
 */
static bool
rundiff(struct diffstate *diff, struct difference *A, struct difference *B)
{
    if ((A->valid || B->valid) == false) {
        /* The diffing engine was called with two pieces of memory that have no
         * differences. We're done. */
        return false;
    } else if (A->valid == false) {
        /*
         * First memory difference is in B. A is marked as invalid. This is
         * only the case for the first iteration of the process in main().
         */
        diffprecontext(diff, B);
        difflines(diff, B);
    } else if (B->valid == false) {
        /*
         * Last memory difference is in A. The content of A and B are produced
         * by finddiff() in main(). When B is invalid, we're at the end of
         * processing (so this branch returns false as well), and only produce
         * final missing diff context.
         */
        diffpostcontext(diff, A);
        return false;
    } else {
        /*
         * Both A and B mark different lines of differing memory. Calculate the
         * distance (in output) between the two differences, as well as a limit
         * that decides if output happens in one or two chunks.
         */
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
            const size_t preoffset =
                B->lineoffset - (diff->columns*diff->context);
            printlines(diff, postoffset, diff->context);
            printskip();
            printlines(diff, preoffset, diff->context);
        }
        difflines(diff, B);
    }
    return true;
}

/**
 * Print a hexdigit of a byte at a given nibble index
 *
 * This procedure ignores its first three arguments, and only works with "ch"
 * and "idx". Given a value of 0x12, and idx being 1, "1" is printed. With idx
 * being 0 a "2" characters is printed.
 *
 * @param  a       Unused memory pointer
 * @param  b       A second unused memory pointer
 * @param  offset  An unused memory offset parameter
 * @param  ch      Value of byte to work with
 * @param  idx     Nibble index into the byte
 *
 * @return size_t or brief description
 * @sideeffects None or brief description
 */
static size_t
px(UNUSED const void *a, UNUSED const void *b, UNUSED size_t offset,
   const unsigned char ch, const size_t idx)
{
    const size_t bits_per_digit = 4U;
    const size_t bitoffset = (idx-1) * bits_per_digit;
    const unsigned int digit = BIT_GET(ch, bits_per_digit, bitoffset);
    (void)putchar(digits[digit]);
    return 0U;
}

/**
 * Print a marker is a difference in two bytes is detected
 *
 * This is the driver for px_above(), pp_above(), px_below() as well as
 * pp_below(); the output of the system can looks like this:
 *
 * #      00000000  c0 db dc 61 62 63 db dc  64 65 66 db dd 67 68 69  | ...abc.. def..ghi |
 * #  d:                              vv                              |       v           |
 * #  a:  00000010  dd 6a 6b 6c dc 6d 7a 6f  dc 65 6e 64 c0           | .jkl.mzo .end.    |
 * #  b:  00000010  dd 6a 6b 6c dc 6d 6e 6f  dc 65 6e 64 c0           | .jkl.mno .end.    |
 * #  d:                              ^^                              |       ^           |
 * #
 * # Found differences in 1 of 29 byte(s).
 *
 * The lines "a" and "b" correspond to the actual data from the two pieces of
 * memory being diffed. The "d" lines above and below these mark positions
 * where the two pieces of memory differ.
 *
 * This procedure implement this in the end. It either prints a space or the
 * given character in "ch" (which is going to be either "v" or "^" depending on
 * which function calls this).
 *
 * @param  a       Pointer to first piece of memory
 * @param  b       Pointer to second piece of memory
 * @param  offset  Offset into both pieces of memory, addressing active byte
 * @param  ch      The character to print if there is a difference
 *
 * @return 1 if a difference between a and b at offset is found; 0 otherwise.
 * @sideeffects Prints via stdio.
 */
static size_t
pr_if_diff(const void *a, const void *b, const size_t offset,
           const unsigned char ch)
{
    const unsigned char *aptr = a;
    const unsigned char *bptr = b;
    if (aptr[offset] == bptr[offset]) {
        (void)putchar(' ');
        return 0U;
    } else {
        (void)putchar(ch);
        return 1U;
    }
}

/**
 * Print difference markers above data lines in hex section of output
 *
 * See pr_if_diff() for details.
 *
 * @param  a       Pointer to first piece of memory
 * @param  b       Pointer to second piece of memory
 * @param  offset  Offset into both pieces of memory, addressing active byte
 * @param  ch      Unused character data
 * @param  idx     Unused nibble index
 *
 * @return See pr_if_diff().
 * @sideeffects Prints via stdio.
 */
static size_t
px_above(const void *a, const void *b, const size_t offset,
         UNUSED const unsigned char ch, UNUSED const size_t idx)
{
    (void)pr_if_diff(a, b, offset, 'v');
    return 0U;
}

/**
 * Print difference markers below data lines in hex section of output
 *
 * See pr_if_diff() for details.
 *
 * @param  a       Pointer to first piece of memory
 * @param  b       Pointer to second piece of memory
 * @param  offset  Offset into both pieces of memory, addressing active byte
 * @param  ch      Unused character data
 * @param  idx     Unused nibble index
 *
 * @return See pr_if_diff().
 * @sideeffects Prints via stdio.
 */
static size_t
px_below(const void *a, const void *b, size_t offset,
         UNUSED const unsigned char ch, UNUSED const size_t idx)
{
    (void)pr_if_diff(a, b, offset, '^');
    return 0U;
}

/**
 * Print a printable version of a byte if possible
 *
 * This is for the printable section of the hexdiff-like output. If a byte is a
 * space character (all of them) or the byte is not printable, print a ".";
 * otherwise print the byte itself.
 *
 * @param  a       Unused memory pointer
 * @param  b       A second unused memory pointer
 * @param  offset  An unused memory offset parameter
 * @param  ch      Value of byte to work with
 *
 * @return size_t or brief description
 * @sideeffects None or brief description
 */
static size_t
pp(UNUSED const void *a, UNUSED const void *b, UNUSED size_t offset,
   unsigned char ch)
{
    if (isprint(ch) && (isspace(ch) == false)) {
        (void)putchar(ch);
    } else {
        (void)putchar('.');
    }
    return 0U;
}

/**
 * Print difference markers above data lines in printable section of output
 *
 * See pr_if_diff() for details.
 *
 * @param  a       Pointer to first piece of memory
 * @param  b       Pointer to second piece of memory
 * @param  offset  Offset into both pieces of memory, addressing active byte
 * @param  ch      Unused character data
 *
 * @return See pr_if_diff().
 * @sideeffects Prints via stdio.
 */
static size_t
pp_above(const void *a, const void *b, size_t offset,
         UNUSED const unsigned char ch)
{
    (void)pr_if_diff(a, b, offset, 'v');
    return 0U;
}

/**
 * Print difference markers below data lines in printable section of output
 *
 * See pr_if_diff() for details.
 *
 * @param  a       Pointer to first piece of memory
 * @param  b       Pointer to second piece of memory
 * @param  offset  Offset into both pieces of memory, addressing active byte
 * @param  ch      Unused character data
 *
 * @return See pr_if_diff().
 * @sideeffects Prints via stdio.
 */
static size_t
pp_below(const void *a, const void *b, size_t offset,
         UNUSED const unsigned char ch)
{
    return pr_if_diff(a, b, offset, '^');
}

/**
 * Print a difference marker like under data chunks
 *
 * See the example in pr_if_diff() for what this does. This is implemented
 * using wprint_chunk_hex() with the appropriate printer arguments.
 *
 * @param  a        Pointer to first piece of memory
 * @param  b        Pointer to second piece of memory
 * @param  offset   Offset to the byte into both pieces of memory
 * @param  n        Number of bytes to print
 * @param  columns  Maximum number of bytes to print
 *
 * @return Number of differences encountered between "a" and "b".
 * @sideeffects Prints via stdio.
 */
static size_t
diffunderline(const void *a, const void *b, size_t offset, size_t n,
              size_t columns)
{
    return wprint_chunk_hex(a, b, offset, n, columns, px_below, pp_below);
}

/**
 * Print a difference marker like above data chunks
 *
 * This is like diffunderline() but produces output for marker lines above data
 * chunk lines. This procedure also does not return a counter of differences,
 * as it is not needed in this procedure's call site.
 *
 * @param  a        Pointer to first piece of memory
 * @param  b        Pointer to second piece of memory
 * @param  offset   Offset to the byte into both pieces of memory
 * @param  n        Number of bytes to print
 * @param  columns  Maximum number of bytes to print
 *
 * @sideeffects Prints via stdio.
 */
static void
diffoverline(const void *a, const void *b, size_t offset, size_t n,
             size_t columns)
{
    (void)wprint_chunk_hex(a, b, offset, n, columns, px_above, pp_above);
}

/**
 * Print chunk of data
 *
 * This function is the driver to produce output similar to this:
 *
 *  72 79 20 66 6f 72 20 65  6d 62 65 64 64 65 64 20  |ry for embedded |
 *  70 79 0a                                          |py.             |
 *  ^^^^^^^^^^^^ Hex-Columns of Output ^^^^^^^^^^^^^  ^ Printable Data ^
 *
 * This procedure steps through "memory" for at most "bytes" bytes, to produce
 * the two sections indicated in the example above. The procedure itself only
 * prints seperators, deliminators, as well as padding for this output. The
 * actual bytes are handled by the hexdigit and printable arguments, which are
 * procedures to actually produce the desired output.
 *
 * This is done, because the general pattern in traversing a chunk of memory is
 * almost the same in many places of the system: For instance when printing a
 * chunk of memory, that contains a byte that is different from another piece
 * of memory, the sytem prints a "^" in the line below the byte in exactly the
 * same place. Everything, all the white space and delimiters will be exacly
 * the same in this second line, only the handling of the actual data differs.
 *
 * This procedure abstracts that part of the system so it can be reused.
 *
 * The procedure parameters are presented with pointers to "memory" and "aux",
 * as well as an offset into them to address the byte that is currently being
 * processed. Additionally, the byte in question is passed in as well. The hex
 * section printer also gets an index into the byte, that specifies the nibble
 * within a the byte that has to be printed. So the hexdigit procedure is
 * called multiple times for each nibble of a byte (two times usually for eight
 * bit bytes, but this also supports architectures like TI C2000 with 16 bit
 * bytes, which has four nibbles per byte). The procedures should return
 * whether or not a difference was deteced in the byte at the offset given in
 * the two pieces of memory presented to it, if applicable. The general pattern
 * implemented by this procedure does not depend on this, but the greater
 * system in diffunderline() used in difflines(), which is used by rundiff()
 * use this mechanism to detect the number of differences found in the two
 * pieces of memory that the system is diffing.
 *
 * @param  memory     Pointer to memory to use
 * @param  aux        Optional buffer to compare "memory" to
 * @param  offset     Offset into memory and aux
 * @param  bytes      Number of bytes to print
 * @param  columns    Maximum number of bytes to print
 * @param  hexdigit   Function to print a byte in hex columns of output
 * @param  printable  Function to print a byte in printable output
 *
 * @return Accumulated return values of the hexdigit and printer callbacks.
 * @sideeffects Prints via stdio.
 */
static size_t
wprint_chunk_hex(const void *memory, const void *aux, const size_t offset, /* NOLINT */
                 const size_t bytes, size_t columns,
                 hexdigitprinter hexdigit, printableprinter printable)
{
    const size_t bits_per_digit = 4U;
    const size_t bits_per_byte = (size_t)UFW_BITS_PER_BYTE;
    const size_t bits_per_word = bits_per_byte * bytes;
    const size_t digitsteps = bits_per_byte / bits_per_digit;
    const size_t steps = bits_per_word / bits_per_byte;
    size_t count = 0U;
    size_t step, pad;

    if (bytes > columns) {
        columns = bytes;
    }

    const unsigned char *ptr = memory;
    ptr += offset;
    /* Print hexadecimal data dump */
    for (step = 0U; step < steps; ++step) {
        if (step > 0 && (step % 8) == 0) {
            (void)putchar(' ');
        }
        for (size_t j = digitsteps; j > 0; --j) {
            count += hexdigit(memory, aux, offset + step, *ptr, j);
        }
        ptr++;
        (void)putchar(' ');
    }
    /* Pad to a given column width */
    if (bytes < columns) {
        size_t rest = columns - bytes;
        size_t spaces = step + rest;
        for (pad = step; pad < spaces; ++pad) {
            if (pad > 0 && (pad % 8) == 0) {
                (void)putchar(' ');
            }
            for (size_t j = digitsteps; j > 0; --j) {
                (void)putchar(' ');
            }
            ptr++;
            (void)putchar(' ');
        }
    }
    /* Print printable-characters after data dump, similar to hexdump */
    (void)fputs(" | ", stdout);
    ptr = memory;
    ptr += offset;
    for (step = 0U; step < bytes; ++step) {
        if (step > 0 && (step % 8) == 0) {
            (void)putchar(' ');
        }
        count += printable(memory, aux, offset + step, *ptr);
        ptr++;
    }
    /* Pad printable-characters to a given column width as well */
    if (bytes < columns) {
        size_t rest = step + columns - bytes;
        for (pad = step; pad < rest; ++pad) {
            if (pad > 0 && (pad % 8) == 0) {
                (void)putchar(' ');
            }
            (void)putchar(' ');
        }
    }
    (void)fputs(" |\n", stdout);
    return count;
}

/**
 * Print a chunk of memory in hexdump-like fashion
 *
 * This calls the system's main worker wprint_chunk_hex() with arguments to
 * achieve hexdump-like output.
 *
 * @param  memory   Pointer to memory to use
 * @param  offset   Offset into memory
 * @param  bytes    Number of bytes to output
 * @param  columns  Maximum number of bytes per line
 *
 * @sideeffects Prints via stdio.
 */
void
print_chunk_hex(const void *memory, const size_t offset, const size_t bytes,
                const size_t columns)
{
    (void)wprint_chunk_hex(memory, NULL, offset, bytes, columns, px, pp);
}

/**
 * Show differences between two pieces of memory
 *
 * Given two pointers to memory (a and b), show unified-diff-like output of
 * hexdump(1) like output of the first n bytes of the two memory areas.
 *
 * @param  a  Pointer to the first chunk of memory
 * @param  b  Pointer to the second chunk of memory
 * @param  n  Number of bytes to scan for differences
 *
 * @return Number of differences that were detected between a and b.
 * @sideeffects Prints data via stdio.
 */
size_t
memdiff(const void *a, const void *b, size_t n)
{
    /* A difference notes a position of a difference within memory, with its
     * position inside the memory as well as the position of a byte in the line
     * in the hexdump like output produced, that it is a part of. */
    struct difference A, B;

    /* The diffstate is the internal state of the diff-ing engine. It contains
     * things like the pointers to the memory sections, configuration like the
     * number of desired bytes per line, and numner of context lines to print,
     * as well as mutable data like the byte-position of the diff-ing process,
     * as well as the number of detected differences at any point in time. */
    struct diffstate diff = {
        .a = a, .b = b, .size = n,
        .columns = 16U, .context = 2U,
        .position = 0U, .count = 0U };

    /*
     * This uses two difference instances (A and B), because the engine needs
     * to know about the position of the previous and the current difference
     * positions. At the start we set the previous state (A) to something
     * invalid at offset zero.
     *
     * Calculate position of two differences A and B.
     *
     * - If A is invalid, we are at the start of memory. Print up to context
     *   lines of context, depending on whether or not lineoffset of B is large
     *   enough.
     *
     * - If B is invalid, we are at the end of memory. Print up to context
     *   lines of context, depending on whether or not lineoffset of B is small
     *   enough.
     *
     * - Otherwise:
     *
     *   - If line offsets between the two is >= context*2, print context
     *     after A and before B.
     *   - otherwise, print everything between the two line offsets.
     *
     * Each iteration only produces *one* new difference position. That way,
     * producing the context, that is surrounding the differences becomes
     * rather straight forward.
     */
    A.valid = false;
    A.lineoffset = 0U;
    for (;;) {
        /* Find the next difference within input. And run the diff engine. */
        B = finddiff(&diff);
        if (rundiff(&diff, &A, &B) == false) {
            break;
        }

        /* If there were differences printed with the above, it always took
         * care of printing until a line of output. So the next diff searach
         * has to start at the position the next line of output would start. */
        diff.position = nextline(diff.position, diff.columns);

        /* Make the current position the previous position for the next
         * iteration. */
        A = B;
    }

    return diff.count;
}
