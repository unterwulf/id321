#include <errno.h>
#include <iconv.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "output.h"
#include "common.h"

static uint16_t g_output_mask = OS_ERROR | OS_WARN;
extern char *program_name;

void init_output(uint16_t mask)
{
    g_output_mask = mask;
}

void print(output_severity sev, const char* format, ...)
{
    va_list ap;
    FILE *fd = stderr;

    if (g_output_mask & sev)
    {
        if (sev & (OS_INFO | OS_DEBUG))
            fd = stdout;
        else
            fprintf(fd, "%s: ", program_name);

        va_start(ap, format);
        vfprintf(fd, format, ap);
        fprintf(fd, "\n");
        va_end(ap);
    }
}
