#include <errno.h>
#include <iconv.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void lprint(const char *fromcode, const char *str)
{
    iconv_t      cd;
    char         buf[BUFSIZ];
    char        *out = buf;
    const char  *in = str;
    size_t       inbytesleft = strlen(str);
    size_t       outbytesleft = sizeof(buf);
    size_t       retval;

    cd = iconv_open(locale_encoding(), fromcode);

    if (cd == (iconv_t)-1)
    {
        perror("iconv_open");
        exit(-1);
    }

    do {
        errno = 0;
        retval = iconv(cd, (char **)&in, &inbytesleft, &out, &outbytesleft);

        if (retval == (size_t)(-1))
        {
            switch (errno)
            {
                case EILSEQ:
                case EINVAL:
                    /* skip invalid character */
                    in++;
                    inbytesleft--;
                    /* dump output buffer */
                    fwrite(buf, sizeof(buf) - outbytesleft, 1, stdout);
                    out = buf;
                    outbytesleft = sizeof(buf);
                    /* print invalid character replacement */
                    lprint(fromcode, "?");
                    continue;

                case E2BIG:
                    /* dump output buffer */
                    fwrite(buf, sizeof(buf) - outbytesleft, 1, stdout);
                    out = buf;
                    outbytesleft = sizeof(buf);
                    continue;
            }
        }
    } while (inbytesleft > 0);

    iconv_close(cd);
    fwrite(buf, sizeof(buf) - outbytesleft, 1, stdout);
}
