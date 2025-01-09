/*
 * Copyright (c) 2017-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_COMPILER_H
#define INC_UFW_COMPILER_H

#include <ufw/toolchain.h>

/**
 * @addtogroup compiler Compiler Features
 *
 * Access and utilities for compiler features and extensions
 *
 * The `ufw` library has some support for finding information about the
 * toolchain, that is currently active, and implements a number of helpers to
 * more succinctly use some of the detected extensions.
 *
 * The `ufw/toolchain.h` header (which is generated from `toolchain.h.in` at
 * configuration time) defines a number of macros, that reflect the test
 * results for compiler command line options, all of which at the moment are
 * tests for for warning-options. These tests are run for the C and C++
 * compilers of a toolchain. The C macros have the form `UFW_CC_HAS_*`, while
 * the C++ macros look like `UFW_CXX_HAS_*`. For instance:
 *
 * - `UFW_CC_HAS_Wused_but_marked_unused`
 * - `UFW_CXX_HAS_Wused_but_marked_unused`
 *
 * Similarly, there are tests for extension attributes and builtins supported
 * by the C and C++ compilers. Examples:
 *
 * - `UFW_CC_HAS_ATTRIBUTE_COLD`
 * - `UFW_CXX_HAS_ATTRIBUTE_COLD`
 * - `UFW_CC_HAS_BUILTIN_EXPECT`
 * - `UFW_CXX_HAS_BUILTIN_EXPECT`
 *
 * There are more macros for various kinds of environment tests, for instance:
 *
 * - `WITH_UNISTD_H`
 * - `WITH_SYS_TYPES_H`
 * - `WITH_UINT8_T`
 * - `UFW_HAVE_POSIX_READ`
 *
 * See the documentation for `ufw/toolchain.h` for all details.
 *
 * Additionally, `ufw/compiler.h` implements short-hands for
 * `__attribute__((...))` extensions of a compiler, like `packed` and others.
 * Code (that is unportable by definition) that depends on these attributes,
 * can test for the availability of the attribute in a toolchain by checking
 * the according macro from `ufw/toolchain.h`. Example:
 *
 * @code
 * #if UFW_CC_HAS_ATTRIBUTE_PACKED == 0
 * #error Code depends on "packed" support in compiler. Cannot continue!
 * #endif
 *
 * struct foobar {
 *     // ...
 * } PACKED;
 * @endcode
 *
 * See the documentation of `ufw/compiler.h` for all attributes, that are
 * supported in this way.
 *
 * @{
 *
 * @file ufw/compiler.h
 * @brief Short-hand macros for compiler features
 *
 * `__cplusplus` note: This file is macro-only, so we don't need the extern C
 * block in this header.
 *
 * @}
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

#endif /* INC_UFW_COMPILER_H */
