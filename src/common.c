#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include "output.h"

/*
 * FUNCTION: readordie
 * This function reads until len bytes has been read or EOF has been reached
 * Returns size read, or -1 on error
*/
ssize_t readordie(int fd, void *buf, size_t len)
{
    ssize_t ret;
    ssize_t left = len;

    while (left != 0 && (ret = read(fd, buf, left)) != 0)
    {
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            perror("read");
            return -1;
        }
        left -= ret;
        buf += ret;
    }

    return len - left;
}

const char *locale_encoding()
{
    static char *enc = NULL;

    if (!enc)
    {
        enc = getenv("LANG");

        if (enc && (enc = strchr(enc, '.')) != NULL)
            enc++;
        else
            enc = "ISO-8859-1";
    }

    return enc;
}

char *xstrdup(const char *s)
{
    char *dup = strdup(s);

    if (dup == NULL)
    {
        print(OS_ERROR, "i need some more memory");
        exit(EXIT_FAILURE);
    }

    return dup;
}

iconv_t xiconv_open(const char *tocode, const char *fromcode)
{
    iconv_t cd = iconv_open(tocode, fromcode);

    if (cd == (iconv_t)-1)
    {
        print(OS_ERROR, "unable to convert string from `%s' to `%s'",
                fromcode, tocode);
        exit(EXIT_FAILURE);
    }

    return cd;
}
