/*
 * Copyright (c) 2019-2021 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file logger.h
 * @brief This files contains datatype definitions and function prototypes.
 */

#ifndef INC_LOGGER_H
#define INC_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup ufw
 * @{
 * @addtogroup Logger
 * @{
 * @addtogroup Datatypes
 * @{
 */

/** Defines an enum datatype for colours */
typedef enum {
    /** Default colour */
    LOG_DEFAULT_COLOUR = 0,
    /** Red, foreground */
    LOG_RED_FG = 31,
    /** Red, background */
    LOG_RED_BG = 41,
    /** Green, foreground */
    LOG_GREEN_FG = 32,
    /** Yellow, foreground */
    LOG_YELLOW_FG = 33,
    /** Blue, foreground */
    LOG_BLUE_FG = 34,
    /** White, foreground */
    LOG_WHITE_FG = 37,
    /** Bright blue, foreground */
    LOG_BLUE_BRIGHT_FG = 94
} LogColour;

/**
 * Defines an enum datatype for the verbosity levels.
 *
 * The verbosity level is interpreted as a bit mask.
 */
typedef enum {
    /** "DEBUG" will be printed in blue */
    LOG_DEBUG = 1u << 0,
    /** "INFO" will be printed in green */
    LOG_INFO = 1u << 1,
    /** "WARNING" will be printed in yellow */
    LOG_WARNING = 1u << 2,
    /** "ERROR" will be printed in red */
    LOG_ERROR = 1u << 3,
    /** "FATAL" will be printed in white on red */
    LOG_FATAL = 1u << 4
} LogLevel;

/**
 * @}
 *
 * @addtogroup Functions
 * @{
 */

void log_set_level(LogLevel level);
void log_print(LogLevel level, const char *msg);
void log_printf(LogLevel level, const char *frmt_msg, ...);

/**
 * @}
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_LOGGER_H */
