#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include "output.h"
#include "common.h"

static uint16_t g_output_mask = OS_WARN | OS_OUT;

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
        if (sev == OS_OUT)
            fd = stdout;

        va_start(ap, format);
        vfprintf(fd, format, ap);
        fprintf(fd, "\n");
        va_end(ap);
    }
}

void print_utf8(const uint8_t *str)
{
    iconv_t      cd;
    char         buf[1024];
    char        *out = buf;
    char        *in = str;
    size_t       inbytesleft = strlen(str);
    size_t       outbytesleft = sizeof(buf) - 1;
    size_t       retval;

    memset(buf, 0, sizeof(buf));
    cd = iconv_open(locale_encoding(), "UTF-8");

    do {
        errno = 0;
        retval = iconv(cd, &in, &inbytesleft, &out, &outbytesleft);

        if (retval == (size_t)(-1))
        {
            switch (errno)
            {
                case EILSEQ:
                case EINVAL:
                    in++;
                    inbytesleft--;
                    continue;

                case E2BIG:
                    continue;
            }
        }
    } while (inbytesleft > 0);

    iconv_close(cd);
    printf("%s", buf);
}
