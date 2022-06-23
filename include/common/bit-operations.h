/*
 * Copyright (c) 2017-2021 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file bit-operations.h
 * @brief Macros implementing bit operations and related constants
 */

#ifndef INC_BITOPS_H
#define INC_BITOPS_H

#include <limits.h>

/**
 * @addtogroup ufw
 * @{
 * @addtogroup bit-operations
 * @{
 */

/*
 * Constants
 */

#ifndef BITS_PER_BYTE
#ifdef CHAR_BIT
/** Number of bits in a byte. Most of the time, a byte is an octet (eight
 *  bits), but on some plattforms (notably DSPs) this is not the case. */
#define BITS_PER_BYTE ((unsigned long int)CHAR_BIT)
#else
#warning "Assuming BITS_PER_BYTE = 8"
#define BITS_PER_BYTE 8ul
#endif /* CHAR_BIT */
#endif /* BITS_PER_BYTE */

/** Number of bits in an unsigned integer */
#define BITS_PER_UNSIGNED (sizeof(unsigned int) * BITS_PER_BYTE)

#ifdef BITS_PER_LONG
/* Zephyr defines this too, so in order to be able to use this without being
 * warned, undefine beforehand. */
#undef BITS_PER_LONG
#endif /* BITS_PER_LONG */
/** Number of bits in an unsigned long integer */
#define BITS_PER_LONG (sizeof(unsigned long int) * BITS_PER_BYTE)

/** Number of bits in an unsigned long long integer */
#define BITS_PER_LONG_LONG (sizeof(unsigned long long int) * BITS_PER_BYTE)

/*
 * Unsigned int generators
 */

#ifdef BIT
/* Zephyr compatibility. */
#undef BIT
#endif /* BIT */
/**
 * Unsigned integer where the nth bit is set
 *
 * @param n     The index of the bit to set in the returned value (counting
 *              starts at zero).
 *
 * @return Unsigned integer as described.
 * @sideeffects None
 */
#define BIT(n) (1u << (n))

/**
 * Unsigned integer where n consecutive bits are set, starting at offset o
 *
 * @verbatim
 *     BIT_ONES(4,4) => 0x00f0u
 * @endverbatim
 *
 * @param n     The number of consecutive bits set in the returned value.
 * @param o     The offset at which the series of set bits starts.
 *
 * @return Unsigned integer as described.
 * @sideeffects None
 */
#define BIT_ONES(n, o) ((~0u) >> (BITS_PER_UNSIGNED - (n)) << (o))

/**
 * Extract a string of bits from an unsigned integer container
 *
 * @param  container   The container to fetch the string from
 * @param  n           The width of the string to fetch
 * @param  o           The offset at which the string starts
 *
 * @return The string of bits moved to offset zero
 * @sideeffects None
 */
#define BIT_GET(container, n, o) (((container) & BIT_ONES((n),(o))) >> (o))

#ifdef BIT_MASK
/* Zephyr compatibility. */
#undef BIT_MASK
#endif /* BIT_MASK */
/**
 * Set a bit in a block of unsigned int words
 *
 * This marco works similar to BIT(), but works with exceedingly large values
 * of n. The purpose is to, in conjunction with BIT_WORD(), set a bit in a
 * block of unsigned integer values.
 *
 * @code
 *   unsigned int foo[8] = { 0 };
 *   foo[BIT_WORD(72)] |= BIT_MASK(72);
 * @endcode
 *
 * @param n     The index of the bit to set in the returned value
 *
 * @return Unsigned integer as described.
 * @sideeffects None
 */
#define BIT_MASK(n) (BIT((n) % BITS_PER_UNSIGNED))

/**
 * Return the word index of a bit within a block of unsigned ints
 *
 * See BIT_MASK() for a detailed description.
 *
 * @param n     The index of the bit to find the word index for.
 *
 * @return Unsigned integer as described.
 * @sideeffects None
 */
#define BIT_WORD(n) ((n) / BITS_PER_LONG)

/*
 * Unsigned long int generators
 */

/**
 * Unsigned long integer where the nth bit is set
 *
 * @param n     The index of the bit to set in the returned value (counting
 *              starts at zero).
 *
 * @return Unsigned long integer as described.
 * @sideeffects None
 */
#define BITL(n) (1ul << (n))

/**
 * Unsigned long integer where n consecutive bits are set, starting at offset o
 *
 * @verbatim
 *     BITL_ONES(8,8) => 0x0000ff00ul
 * @endverbatim
 *
 * @param n     The number of consecutive bits set in the returned value.
 * @param o     The offset at which the series of set bits starts.
 *
 * @return Unsigned long integer as described.
 * @sideeffects None
 */
#define BITL_ONES(n, o) ((~0ul) >> (BITS_PER_LONG - (n)) << (o))

/**
 * Extract a string of bits from an long unsigned integer container
 *
 * @param  container   The container to fetch the string from
 * @param  n           The width of the string to fetch
 * @param  o           The offset at which the string starts
 *
 * @return The string of bits moved to offset zero
 * @sideeffects None
 */
#define BITL_GET(container, n, o) (((container) & BITL_ONES((n),(o))) >> (o))

/**
 * Set a bit in a block of unsigned long int words
 *
 * This marco works similar to BITL(), but works with exceedingly large values
 * of n. The purpose is to, in conjunction with BITL_WORD(), set a bit in a
 * block of unsigned integer values.
 *
 * @code
 *   unsigned long int foo[8] = { 0 };
 *   foo[BITL_WORD(72)] |= BITL_MASK(72);
 * @endcode
 *
 * @param n     The index of the bit to set in the returned value
 *
 * @return Unsigned long integer as described.
 * @sideeffects None
 */
#define BITL_MASK(n) (BITL((n) % BITS_PER_LONG))

/**
 * Return the word index of a bit within a block of unsigned long ints
 *
 * See BITL_MASK() for a detailed description.
 *
 * @param n     The index of the bit to find the word index for.
 *
 * @return Unsigned long integer as described.
 * @sideeffects None
 */
#define BITL_WORD(n) ((n) / BITS_PER_LONG)

/*
 * Unsigned long long int generators
 */

/**
 * Unsigned long long integer where the nth bit is set
 *
 * @param n     The index of the bit to set in the returned value (counting
 *              starts at zero).
 *
 * @return Unsigned long long integer as described.
 * @sideeffects None
 */
#define BITLL(n) (1ull << (n))

/**
 * Unsigned long long integer where n consecutive bits are set, starting at
 * offset o
 *
 * @verbatim
 *     BITLL_ONES(16,32) => 0x0000ffff00000000ull
 * @endverbatim
 *
 * @param n     The number of consecutive bits set in the returned value.
 * @param o     The offset at which the series of set bits starts.
 *
 * @return Unsigned long integer as described.
 * @sideeffects None
 */
#define BITLL_ONES(n, o) ((~0ull) >> (BITS_PER_LONG_LONG - (n)) << (o))

/**
 * Extract a string of bits from an long long unsigned integer container
 *
 * @param  container   The container to fetch the string from
 * @param  n           The width of the string to fetch
 * @param  o           The offset at which the string starts
 *
 * @return The string of bits moved to offset zero
 * @sideeffects None
 */
#define BITLL_GET(container, n, o) (((container) & BITLL_ONES((n),(o))) >> (o))

/**
 * Set a bit in a block of unsigned long long int words
 *
 * This marco works similar to BITLL(), but works with exceedingly large values
 * of n. The purpose is to, in conjunction with BITLL_WORD(), set a bit in a
 * block of unsigned integer values.
 *
 * @code
 *   unsigned long long int foo[8] = { 0 };
 *   foo[BITL_WORD(72)] |= BITL_MASK(72);
 * @endcode
 *
 * @param n     The index of the bit to set in the returned value
 *
 * @return Unsigned long long integer as described.
 * @sideeffects None
 */
#define BITLL_MASK(n) (BITLL((n) % BITS_PER_LONG_LONG))

/**
 * Return the word index of a bit within a block of unsigned long long ints
 *
 * See BITLL_MASK() for a detailed description.
 *
 * @param n     The index of the bit to find the word index for.
 *
 * @return Unsigned long long integer as described.
 * @sideeffects None
 */
#define BITLL_WORD(n) ((n) / BITS_PER_LONG_LONG)

/*
 * Generic predicates and mutators
 */

/**
 * Return true if a set of bits is set in a given container
 *
 * All bits given in mask must of set in container for this to return true.
 *
 * @param container    Unsigned integer to test against
 * @param mask         Unsigned integer mask to test with
 *
 * @return True if all bits from mask are set in container; false otherwise.
 * @sideeffects None
 */
#define BIT_ISSET(container, mask) (((container) & (mask)) == (mask))

/**
 * Return true if a any bit from a set of bits is set in a given container
 *
 * @param container    Unsigned integer to test against
 * @param mask         Unsigned integer mask to test with
 *
 * @return True if any of the bits from mask are set in container; false
 *         otherwise.
 * @sideeffects None
 */
#define BIT_ISSET_ANY(container, mask) (!!((container) & (mask)))

/**
 * Set a number of bits in a container
 *
 * This sets all the bits from mask in container.
 *
 * @param container    Unsigned integer storage to modify
 * @param mask         Unsigned integer mask of target bits
 *
 * @return The modified value of container
 * @sideeffects Modifies container as described.
 */
#define BIT_SET(container, mask) ((container) |= (mask))

/**
 * Clear a number of bits in a container
 *
 * This clears all the bits from mask in container.
 *
 * @param container    Unsigned integer storage to modify
 * @param mask         Unsigned integer mask of target bits
 *
 * @return The modified value of container
 * @sideeffects Modifies container as described.
 */
#define BIT_CLEAR(container, mask) ((container) &= (~(mask)))

/**
 * Toggle a number of bits in a container
 *
 * This toggles all the bits from mask in container.
 *
 * @param container    Unsigned integer storage to modify
 * @param mask         Unsigned integer mask of target bits
 *
 * @return The modified value of container
 * @sideeffects Modifies container as described.
 */
#define BIT_TOGGLE(container, mask) ((container) ^= (mask))

/**
 * @}
 * @}
 */

#endif /* INC_BITOPS_H */
