#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include "output.h"

/*
 * Function:     readordie
 *
 * Description:  reads until len bytes has been read or EOF has been reached
 *
 * Return value: number of bytes has been read, or -1 on error
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

ssize_t writeordie(int fd, const void *buf, size_t count)
{
    size_t written = 0;
    ssize_t len;

    while (written < count)
    {
        len = write(fd, buf + written, count - written);

        if (len < 0)
        {
            if (errno == EINTR)
                continue;
            else
                break;
        }
        else
            written += len;
    }

    return written;
}

const char *locale_encoding()
{
    static const char *enc = NULL;

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

int str_to_long(const char *nptr, long *ret)
{
    char *endptr;

    if (!nptr)
        return -1;

    *ret = strtol(nptr, &endptr, 0);

    return (*nptr != '\0' && *endptr == '\0') ? 0 : -1;
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

/*
 * Function:     iconv_alloc
 *
 * Description:  Converts src buffer from fromcode to tocode and places
 *               result into internally allocated memory area. *dst will
 *               be pointing to the result, and *dstsize will contain its
 *               size.
 *
 *               *dst must be freed with free() after use.
 *
 * Return value: 0 on success, -1 on any error
 *
 */

int iconv_alloc(const char *tocode, const char *fromcode,
                const char *src, size_t srcsize,
                char **dst, size_t *dstsize)
{
    iconv_t      cd;
    char        *buf;
    char        *tmp;
    char        *out;
    size_t       tmppos;
    const char  *in = src;
    size_t       outsize = srcsize;
    size_t       inbytesleft = srcsize;
    size_t       outbytesleft;
    size_t       ret;

    print(OS_DEBUG, "to: %s, from: %s!", tocode, fromcode);

    cd = xiconv_open(tocode, fromcode);

    buf = malloc(outsize);
    if (!buf)
        goto oom;

    buf[0] = '\0';
    out = buf;
    outbytesleft = outsize;

    do {
        errno = 0;
        ret = iconv(cd, (char **)&in, &inbytesleft, &out, &outbytesleft);

        if (ret == (size_t)(-1))
        {
            switch (errno)
            {
                case EILSEQ:
                case EINVAL:
                    /* skip invalid character */
                    in++;
                    inbytesleft--;
                    continue;

                case E2BIG:
                    tmppos = out - buf;
                    tmp = realloc(buf, outsize*2);
                    if (!tmp)
                    {
                        free(buf);
                        goto oom;
                    }
                    buf = tmp;
                    out = buf + tmppos;
                    outbytesleft += outsize;
                    outsize *= 2;
                    out[0] = '\0';
                    continue;
            }
        }
    } while (inbytesleft > 0);

    iconv_close(cd);
    *dst = buf;
    *dstsize = out - buf;
    return 0;

oom:

    iconv_close(cd);
    print(OS_ERROR, "out of memory");
    return -1;
}
