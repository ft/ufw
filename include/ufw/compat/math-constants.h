/**
 * @file math-constants.h
 * @brief Compatibility layer for math constants
 *
 * POSIX defines a couple of maths constants in math.h; however the C
 * standard's version of math.h does not define these. This is why on some
 * systems, these constants are missing. On top of that, many toolchains
 * disable these constants when put into strict-standards mode.
 *
 * With this header, you get all constants mentioned in POSIX's math.h(7)
 * manual. It works with POSIX math.h implementations by first checking if a
 * constant is present already.
 *
 * If that is desired, first include math.h then c/compat/math-constants.h.
 */

#ifndef INC_UFW_MATH_CONSTANTS_H
#define INC_UFW_MATH_CONSTANTS_H

#ifndef M_E
/** Euler's number (e) */
#define M_E 2.7182818284590452354
#endif

#ifndef M_LOG2E
/** log_2(e) */
#define M_LOG2E 1.4426950408889634074
#endif

#ifndef M_LOG10E
/** log_10(e) */
#define M_LOG10E 0.43429448190325182765
#endif

#ifndef M_LN2
/** log_e(2) */
#define M_LN2 0.69314718055994530942
#endif

#ifndef M_LN10
/** log_e(10) */
#define M_LN10 2.30258509299404568402
#endif

#ifndef M_PI
/** Circumference of a circle with radius 0.5 (pi) */
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
/** pi / 2 */
#define M_PI_2 1.57079632679489661923
#endif

#ifndef M_PI_4
/** pi / 4 */
#define M_PI_4 0.78539816339744830962
#endif

#ifndef M_1_PI
/** 1 / pi */
#define M_1_PI 0.31830988618379067154
#endif

#ifndef M_2_PI
/** 2 / pi */
#define M_2_PI 0.63661977236758134308
#endif

#ifndef M_2_SQRTPI
/** 2 / sqrt(pi) */
#define M_2_SQRTPI 1.12837916709551257390
#endif

#ifndef M_SQRT2
/** sqrt(2) */
#define M_SQRT2 1.41421356237309504880
#endif

#ifndef M_SQRT1_2
/** 1 / sqrt(2) */
#define M_SQRT1_2 0.70710678118654752440
#endif

#endif /* INC_UFW_MATH_CONSTANTS_H */
