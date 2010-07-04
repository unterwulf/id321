#define _ISOC99_SOURCE /* vswprintf() */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include "common.h"
#include "iconv_wrap.h"
#include "output.h"

/***
 * readordie
 *
 * The routine reads from file descriptior @fd until @len bytes has been
 * read or EOF has been reached.
 *
 * Returns the number of bytes read, or -errno on error.
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
            return -errno;
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

id321_iconv_t xiconv_open(const char *tocode, const char *fromcode)
{
    id321_iconv_t cd = id321_iconv_open(tocode, fromcode);

    if (cd == (id321_iconv_t)-1)
    {
        print(OS_ERROR, "unable to convert string from '%s' to '%s'",
                        fromcode, tocode);
        exit(EXIT_FAILURE);
    }

    return cd;
}

/***
 * iconvordie
 *
 * @tocode - destination character encoding
 * @fromcode - source character encoding
 * @src - source buffer
 * @srcsize - source buffer size
 * @dst - destination buffer
 * @dstsize - destination buffer size
 *
 * Converts the buffer @src of size @srcsize from @fromcode to @tocode and
 * places result into the buffer @dst of size @dstsize.
 *
 * Returns the number of bytes which would have been written if @dst had
 * been large enough.
 */

ssize_t iconvordie(const char *tocode, const char *fromcode,
                   const char *src, size_t srcsize,
                   char *dst, size_t dstsize)
{
    id321_iconv_t cd;
    const char  *in = src;
    char        *out = dst;
    size_t       reqsize = dstsize; /* required size of @dst */
    size_t       inbytesleft = srcsize;
    size_t       outbytesleft = dstsize;
    size_t       ret;
    char         dummy[BUFSIZ];

    cd = xiconv_open(tocode, fromcode);

    if (!out && dstsize == 0)
    {
        /* we need to estimate required buffer size if no buffer passed,
         * so use dummy buffer to let iconv do its work */
        out = dummy;
        outbytesleft = reqsize = dstsize = sizeof(dummy);
    }

    do {
        errno = 0;
        ret = id321_iconv(cd, (char **)&in, &inbytesleft, &out, &outbytesleft);

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
                    /* we need to estimate required buffer size if the passed
                     * buffer is not large enough, so use dummy buffer to let
                     * iconv finish its work */
                    out = dummy;
                    outbytesleft = sizeof(dummy);
                    reqsize += outbytesleft;
                    break;
            }
        }
    } while (inbytesleft > 0);

    reqsize -= outbytesleft;
    id321_iconv_close(cd);

    return reqsize;
}

/***
 * iconv_alloc
 *
 * Converts the buffer @src from @fromcode to @tocode and places result
 * into internally allocated memory area. *@dst will be pointing to the
 * result, and *@dstsize will contain its size.
 *
 * If @tocode is WCHAR_CODESET, resulting wcstring will be null-terminated
 * even if @src is not.
 *
 * *@dst must be freed with free() after use.
 *
 * Returns 0 on success, or -ENOMEM on out of memory error.
 */

int iconv_alloc(const char *tocode, const char *fromcode,
                const char *src, size_t srcsize,
                char **dst, size_t *dstsize)
{
    id321_iconv_t cd;
    char        *buf;
    char        *tmp;
    char        *out;
    size_t       tmppos;
    const char  *in = src;
    size_t       outsize = srcsize;
    size_t       inbytesleft = srcsize;
    size_t       outbytesleft;
    size_t       ret;

    cd = xiconv_open(tocode, fromcode);

    buf = malloc(outsize);
    if (!buf)
        goto oom;

    buf[0] = '\0';
    out = buf;
    outbytesleft = outsize;

    do {
        errno = 0;
        ret = id321_iconv(cd, (char **)&in, &inbytesleft, &out, &outbytesleft);

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

    if (!strcmp(tocode, WCHAR_CODESET) &&
        !(out - buf > (ptrdiff_t)sizeof(wchar_t) &&
          ((wchar_t *)out)[-1] == L'\0'))
    {
        /* make sure that resulting wcstring will be null-terminated */

        if (outbytesleft < sizeof(wchar_t))
        {
            tmppos = out - buf;
            tmp = realloc(buf, outsize + sizeof(wchar_t) - outbytesleft);
            if (!tmp)
            {
                free(buf);
                goto oom;
            }
            buf = tmp;
            out = buf + tmppos;
        }

        *(wchar_t *)out = L'\0';
        out += sizeof(wchar_t);
    }

    id321_iconv_close(cd);
    *dst = buf;
    if (dstsize)
        *dstsize = out - buf;
    return 0;

oom:

    id321_iconv_close(cd);
    return -ENOMEM;
}

int swprintf_alloc(wchar_t **wcs, const wchar_t *fmt, ...)
{
    wchar_t *wdata;
    size_t   wsize = 4096;
    int      ret;
    va_list  args;

    wdata = malloc(wsize*sizeof(wchar_t));
    if (!wdata)
        return -ENOMEM;

    do {
        va_start(args, fmt);
        ret = vswprintf(wdata, wsize, fmt, args);
        va_end(args);

        if (ret == -1)
        {
            free(wdata);
            return -EFAULT;
        }
        else if ((size_t)ret == wsize)
        {
            wchar_t *tmp;

            wsize *= 2;
            tmp = realloc(wdata, wsize);

            if (!tmp)
            {
                free(wdata);
                return -ENOMEM;
            }

            wdata = tmp;
        }
    } while ((size_t)ret == wsize);

    *wcs = wdata;

    return ret;
}
