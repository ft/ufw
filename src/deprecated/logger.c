/*
 * Copyright (c) 2019-2021 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file logger.c
 * @brief This files contains the implementation of the logger functions
 */

#include <stdarg.h>
#include <stdio.h>

#include <ufw/deprecated/logger.h>

/**
 * @addtogroup ufw
 * @{
 * @addtogroup Logger
 * @{
 * @addtogroup Statics
 * @{
 */

/**
 * Defines the current set log level. This will be used as bit mask when
 * deciding weather to print the message or not.
 */
static LogLevel logLevel = LOG_INFO;
static inline void log_print_colour_escape_seq(LogColour);
static inline void log_print_msg_type(LogLevel);

/**
 * @}
 *
 * @addtogroup Internal-Functions
 * @{
 */

/**
 * Sets the output colour.
 *
 * This internal function is called by #log_printMsgType to print/send escape
 * sequences to the output device to print the following message in colour.
 *
 * @param colour The following message will be printed in this colour
 */
static inline void
log_print_colour_escape_seq(LogColour colour) {
    printf("\033[%dm", (int)colour);
}

/**
 * Prints the log message type
 *
 * This internal function is called by #log_print and #log_printf to print the
 * log message type in colour at the beginning of the line.
 *
 * @param level The message type and the colour depend on the value of level
 */
static inline void
log_print_msg_type(LogLevel level) {
    switch (level) {
    case LOG_DEBUG:
        log_print_colour_escape_seq(LOG_BLUE_FG);
        printf("[DEBUG]   ");
        log_print_colour_escape_seq(LOG_DEFAULT_COLOUR);
        break;

    case LOG_INFO:
        log_print_colour_escape_seq(LOG_GREEN_FG);
        printf("[INFO]    ");
        log_print_colour_escape_seq(LOG_DEFAULT_COLOUR);
        break;

    case LOG_WARNING:
        log_print_colour_escape_seq(LOG_YELLOW_FG);
        printf("[WARNING] ");
        log_print_colour_escape_seq(LOG_DEFAULT_COLOUR);
        break;

    case LOG_ERROR:
        log_print_colour_escape_seq(LOG_RED_FG);
        printf("[ERROR]   ");
        log_print_colour_escape_seq(LOG_DEFAULT_COLOUR);
        break;

    case LOG_FATAL:
        log_print_colour_escape_seq(LOG_RED_BG);
        log_print_colour_escape_seq(LOG_WHITE_FG);
        printf("[FATAL]");
        log_print_colour_escape_seq(LOG_DEFAULT_COLOUR);
        printf("   ");
        log_print_colour_escape_seq(LOG_DEFAULT_COLOUR);
        break;
    default:
        break;
    }
}

/**
 * @}
 *
 * @addtogroup Functions
 * @{
 */

/**
 * Sets the logging level.
 *
 * @param level The new log level
 *
 * The logging levels are defined in #LogLevel enumeration datatype.
 * The log message will be printed depending on the bit mask #LogLevel.
 */
void
log_set_level(LogLevel level) {
    logLevel = level;
}

/**
 * Print a message depending on the current log level
 *
 * @note Don't append '\\n' and/or '\\r' at the end of message, for this will
 * be
 *       done by this function.
 *
 * @param level will be bitwise &-ed with the current log level
 * @param msg character string to be printed.
 *
 * @return void
 * @sideeffects Prints to implemented output.
 */
void
log_print(LogLevel level, const char *msg) {
    if (level & logLevel) {
        log_print_msg_type(level);
        printf(" %s\r\n", msg);
    }
}

/**
 * Print a message depending on the current log level
 *
 * This function behaves like the libc/stdio printf function
 *
 * @note Don't append '\\n' and/or '\\r' at the end of message, for this will
 * be
 *       done by this function.
 *
 * @param level will be bitwise &-ed with the current log level
 * @param format character string to be printed.
 * @param ... variable length argument list
 *
 */
void
log_printf(LogLevel level, const char *format, ...) {
    va_list args;

    if (level & logLevel) {
        log_print_msg_type(level);

        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\r\n");
    }
}

/**
 * @}
 * @}
 * @}
 */
