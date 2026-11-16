/* C99-only platform services shared by every provider. */
#include "platform.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int h_platform_asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    int result;

    va_start(ap, fmt);
    result = h_platform_vasprintf(strp, fmt, ap);
    va_end(ap);
    return result;
}

int h_platform_vasprintf(char **strp, const char *fmt, va_list arg) {
    va_list copy;
    int length;
    char *buffer;

    if (strp == NULL)
        return -1;
    *strp = NULL;
    if (fmt == NULL)
        return -1;

    va_copy(copy, arg);
    length = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (length < 0 || (size_t)length >= SIZE_MAX)
        return -1;

    buffer = malloc((size_t)length + 1);
    if (buffer == NULL)
        return -1;

    va_copy(copy, arg);
    if (vsnprintf(buffer, (size_t)length + 1, fmt, copy) != length) {
        va_end(copy);
        free(buffer);
        return -1;
    }
    va_end(copy);

    *strp = buffer;
    return length;
}

void h_platform_errx(int err, const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    if (format != NULL)
        vfprintf(stderr, format, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(err);
}
