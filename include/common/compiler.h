/*
 * Copyright (c) 2017-2020 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file compiler.h
 * @brief Short-hand macros for compiler features
 */

#ifndef INC_COMPILER_H
#define INC_COMPILER_H

#include <common/toolchain.h>

/**
 * @addtogroup ufw
 * @{
 * @addtogroup compiler
 * @{
 */

/*
 * Annotation macros
 */

/**
 * Tell the compiler, that it is known that a variable is currently unused.
 *
 * @code
 * void
 * some_callback(int index, UNUSED char *stuff)
 * {
 *     // ...code...
 * }
 * @endcode
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_UNUSED
/* ST defined UNUSED like this: #define UNUSED(x) (void)x which is meant to be
 * used like this:
 *
 * void
 * foo(int a, int b)
 * {
 *     UNUSED(b);
 *     return a * 2;
 * }
 *
 * With attributes, this becomes:
 *
 * void foo(int a, UNUSED int b) ...
 *
 * Which is arguably preferable. In order to use this with ST headers in play
 * as well, include this header *after* the ST system headers.
 */
#ifdef UNUSED
#undef UNUSED
#endif /* UNUSED */
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif /* HAVE_COMPILER_ATTRIBUTE_UNUSED */

/**
 * Define a weak alias for a symbol
 *
 * This allows user code to override the definition of a symbol.

 * @code
 * void _ufw_isr_system_tick(void) WEAK_ALIAS(_ufw_isr_fallback);
 * @endcode
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_WEAK_ALIAS
#define WEAK_ALIAS(x) __attribute__ ((weak, alias(#x)))
#else
#define WEAK_ALIAS
#endif /* HAVE_COMPILER_ATTRIBUTE_UNUSED */

/**
 * Tell the compiler that function will never ever return.
 *
 * @code
 * NORETURN void die(const char *fmt, ...);
 * @endcode
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_NORETURN
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif /* HAVE_COMPILER_ATTRIBUTE_NORETURN */

/*
 * Optimisation macros
 */

/**
 * Request hole-free packing of composite data structures
 *
 * Even if that impacts performance.
 *
 * @code
 * struct foo {
 *    uint8_t thing;
 *    uint32_t fish;
 *    uint8_t hey;
 *    uint16_t ho[7];
 * } PACKED;
 * @endcode
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_PACKED
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif /* HAVE_COMPILER_ATTRIBUTE_PACKED */

/**
 * Tell the compiler if it is very unlikely, that a function is called.
 *
 * @code
 * COLD void rare_problem_solver(struct problem *p);
 * @endcode
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_COLD
#define COLD __attribute__((cold))
#else
#define COLD
#endif /* HAVE_COMPILER_ATTRIBUTE_COLD */

/**
 * Tell the compiler, that a function is a "hot-spot".
 *
 * This may result in more aggressive optimisations by the compiler.
 *
 * @code
 * HOT int important_computation(int start);
 * @endcode
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_HOT
#define HOT __attribute__((hot))
#else
#define HOT
#endif /* HAVE_COMPILER_ATTRIBUTE_HOT */

/**
 * Tell the compiler, the expression 'EXP' is likely to be true.
 *
 * @code
 *     if (LIKELY(a > b)) {
 *         // ...code...
 *     }
 * @endcode
 */
#ifdef HAVE_COMPILER_BUILTIN_EXPECT
#define LIKELY(EXP) __builtin_expect(!!(EXP), 1)
#else
#define LIKELY
#endif /* HAVE_COMPILER_BUILTIN_EXPECT */

/**
 * Tell the compiler, the expression 'EXP' is unlikely to be true.
 *
 * @code
 *     if (UNLIKELY(a < b)) {
 *         // ...code...
 *     }
 * @endcode
 */
#ifdef HAVE_COMPILER_BUILTIN_EXPECT
#define UNLIKELY(EXP) __builtin_expect(!!(EXP), 0)
#else
#define UNLIKELY
#endif /* HAVE_COMPILER_BUILTIN_EXPECT */

/*
 * Semantic helper macros
 */

/**
 * Tell the compiler that a function takes printf-like arguments.
 *
 * This allows the compiler to apply the same set of tests and warnings as with
 * printf.
 *
 * @code
 * PRINTF_FMT(1, 2) void log(const char *fmt, ...);
 * @endcode
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_PRINTF_FMT
#define PRINTF_FMT(FORMAT_INDEX, ARG_INDEX)                                   \
    __attribute__((format(__printf__, FORMAT_INDEX, ARG_INDEX)))
#else
#define PRINTF_FMT
#endif /* HAVE_COMPILER_ATTRIBUTE_PRINTF_FMT */

/**
 * Tell the compiler to warn if the return value of a function is ignored.
 *
 * That would make sense for `malloc(3)' etc.
 *
 * @code
 * WARN_UNUSED_RESULT struct foo *new_foo(char *bar);
 * @endcode
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_WARN_UNUSED_RESULT
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN
#endif /* HAVE_COMPILER_ATTRIBUTE_WARN_UNUSED_RESULT */

/*
 * Storage macros
 */

/**
 * Define the data section a symbol should be placed in
 *
 * @code
 * int foo SECTION(".some-section") = 23;
 * @endcode
 *
 * @param  SECTION   A string naming the target section.
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_SECTION
#define SECTION(SEC) __attribute__((section (SEC)))
#else
#define SECTION
#endif /* HAVE_COMPILER_ATTRIBUTE_SECTION */

/*
 * Other macros
 */

/**
 * Emit a warning if a function marked like this is used in the code.
 *
 * @code
 * void old(int horrible, char *api) DEPRECATED;
 * @endcode
 */
#ifdef HAVE_COMPILER_ATTRIBUTE_DEPRECATED
#define DEPRECATED __attribute__((deprecated))
#else
#define DEPRECATED
#endif /* HAVE_COMPILER_ATTRIBUTE_DEPRECATED */

/**
 * @}
 * @}
 */

#endif /* INC_COMPILER_H */
