/* Copyright (c) 2026 Riverside Research */
#ifndef HAMMER_PLATFORM__H
#define HAMMER_PLATFORM__H

/**
 * @file interface between hammer and the operating system /
 * underlying platform.
 */

#include "compiler_specifics.h"

/*
 * Library builds find the generated configuration through their build include
 * directory.  Extension users sometimes include a source-tree internal header
 * while linking against an installed Hammer; in that case use the installed
 * sibling configuration header instead.
 */
#if defined(__has_include)
#if __has_include("platform_config.h")
#include "platform_config.h"
#elif __has_include(<hammer/platform_config.h>)
#include <hammer/platform_config.h>
#else
#error "Hammer platform_config.h was not generated or installed"
#endif
#else
#include "platform_config.h"
#endif

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/* String Formatting */

/** see GNU C asprintf */
int h_platform_asprintf(char **strp, const char *fmt, ...);

/** see GNU C vasprintf */
int h_platform_vasprintf(char **strp, const char *fmt, va_list arg);

/* Return nonzero when stream is attached to an interactive terminal. */
int h_platform_is_terminal(FILE *stream);

/* Error Reporting */

/* Error reporting function that prints a message and exits */
void h_platform_errx(int err, const char *format, ...)
    H_GCC_ATTRIBUTE((noreturn, format(printf, 2, 3)));

/* Time Measurement */

struct HStopWatch; /* forward definition */

/* initialize a stopwatch */
void h_platform_stopwatch_reset(struct HStopWatch *stopwatch);

/* return difference between last reset point and now */
int64_t h_platform_stopwatch_ns(struct HStopWatch *stopwatch);

/* Platform dependent definitions for HStopWatch */

#if H_PLATFORM_POSIX
#include <time.h>
struct HStopWatch {
    struct timespec start;
};
#else
struct HStopWatch {
    /* Opaque provider state. Use the stopwatch functions above. */
    int64_t words[2];
};
#endif

#endif
