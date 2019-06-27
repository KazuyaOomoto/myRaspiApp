#include <stdio.h>
#include <stdarg.h>

void debug(const char* restrict format, ...)
{
    va_list va;

    fflush(stdout);
    va_start(va, format);
    // int vprintf(const char *format, va_list ap);
    vprintf(format, va);
    va_end(va);
}
