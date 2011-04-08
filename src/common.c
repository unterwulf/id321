#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "iconv_wrap.h"
#include "output.h"
#include "u32_char.h"

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
    char *ptr = buf;

    while (left != 0 && (ret = read(fd, ptr, left)) != 0)
    {
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            perror("read");
            return -errno;
        }
        left -= ret;
        ptr += ret;
    }

    return len - left;
}

ssize_t writeordie(int fd, const void *buf, size_t count)
{
    size_t written = 0;
    ssize_t len;
    const char *ptr = buf;

    while (written < count)
    {
        len = write(fd, ptr, count - written);

        if (len < 0)
        {
            if (errno == EINTR)
                continue;
            else
                break;
        }
        written += len;
        ptr += len;
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
            enc = ISO_8859_1_CODESET;
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
    size_t reqsize; /* required size of @dst */
    size_t ret;
    char dummy[BUFSIZ];
    int is_from_u32 = (!strcmp(fromcode, U32_CHAR_CODESET)) ? 1 : 0;

    cd = xiconv_open(tocode, fromcode);

    if (!dst && dstsize == 0)
    {
        /* we need to estimate required buffer size if no buffer passed,
         * so use dummy buffer to let iconv do its work */
        dst = dummy;
        dstsize = sizeof(dummy);
    }

    reqsize = dstsize;

    while (srcsize > 0)
    {
        errno = 0;
        ret = id321_iconv(cd, (char **)&src, &srcsize, &dst, &dstsize);

        if (ret == (size_t)(-1))
        {
            switch (errno)
            {
                case EILSEQ:
                case EINVAL:
                {
                    ssize_t chsize = iconvordie(tocode, ISO_8859_1_CODESET,
                                                "?", 1, dst, dstsize);

                    if (chsize <= dstsize)
                    {
                        /* skip invalid character */
                        if (is_from_u32 && srcsize >= sizeof(u32_char))
                        {
                            src += sizeof(u32_char);
                            srcsize -= sizeof(u32_char);
                        }
                        else
                        {
                            src++;
                            srcsize--;
                        }
                        dst += chsize;
                        dstsize -= chsize;

                        if (dstsize > 0)
                            continue;
                    }
                    /* else go to E2BIG as buffer too small */
                }

                case E2BIG:
                    /* we need to estimate required buffer size if the passed
                     * buffer is not large enough, so use dummy buffer to let
                     * iconv finish its work */
                    reqsize -= dstsize;
                    dst = dummy;
                    dstsize = sizeof(dummy);
                    reqsize += dstsize;
                    break;
            }
        }
    }

    id321_iconv_close(cd);

    return reqsize - dstsize;
}

/***
 * iconv_alloc
 *
 * Converts the buffer @src from @fromcode to @tocode and places result
 * into internally allocated memory area. *@dst will be pointing to the
 * result, and *@dstsize will contain its size.
 *
 * If @tocode is U32_CHAR_CODESET, resulting u32 string will be null-terminated
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
    char *buf;
    char *tmp;
    char *out;
    size_t tmppos;
    size_t outsize = srcsize;
    size_t outbytesleft;
    size_t ret;
    int is_from_u32 = (!strcmp(fromcode, U32_CHAR_CODESET)) ? 1 : 0;

    cd = xiconv_open(tocode, fromcode);

    buf = malloc(outsize);
    if (!buf)
        goto oom;

    out = buf;
    outbytesleft = outsize;

    while (srcsize > 0)
    {
        errno = 0;
        ret = id321_iconv(cd, (char **)&src, &srcsize, &out, &outbytesleft);

        if (ret == (size_t)(-1))
        {
            switch (errno)
            {
                case EILSEQ:
                case EINVAL:
                {
                    ssize_t chsize = iconvordie(tocode, ISO_8859_1_CODESET,
                                                "?", 1, out, outbytesleft);

                    if (chsize <= outbytesleft)
                    {
                        /* skip invalid character */
                        if (is_from_u32 && srcsize >= sizeof(u32_char))
                        {
                            src += sizeof(u32_char);
                            srcsize -= sizeof(u32_char);
                        }
                        else
                        {
                            src++;
                            srcsize--;
                        }
                        out += chsize;
                        outbytesleft -= chsize;

                        if (outbytesleft > 0)
                            continue;
                    }
                    /* else go to E2BIG as buffer too small */
                }

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
                    continue;
            }
        }
    }

    id321_iconv_close(cd);

    if (!strcmp(tocode, U32_CHAR_CODESET) &&
        !(out - buf > (ptrdiff_t)sizeof(u32_char) &&
          ((u32_char *)out)[-1] == U32_CHAR('\0')))
    {
        /* make sure that resulting U32 string will be null-terminated */

        if (outbytesleft < sizeof(u32_char))
        {
            tmppos = out - buf;
            tmp = realloc(buf, outsize + sizeof(u32_char) - outbytesleft);
            if (!tmp)
            {
                free(buf);
                goto oom;
            }
            buf = tmp;
            out = buf + tmppos;
        }

        *(u32_char *)out = U32_CHAR('\0');
        out += sizeof(u32_char);
    }

    *dst = buf;
    if (dstsize)
        *dstsize = out - buf;
    return 0;

oom:

    id321_iconv_close(cd);
    return -ENOMEM;
}

int u32_snprintf_alloc(u32_char **u32_str, const char *fmt, ...)
{
    u32_char *u32_data;
    size_t    u32_size = 4096;
    int       ret;
    va_list   args;

    u32_data = malloc(u32_size*sizeof(u32_char));
    if (!u32_data)
        return -ENOMEM;

    do {
        va_start(args, fmt);
        ret = u32_vsnprintf(u32_data, u32_size, fmt, args);
        va_end(args);

        if (ret == -1)
        {
            free(u32_data);
            return -EFAULT;
        }
        else if ((size_t)ret == u32_size)
        {
            u32_char *tmp;

            u32_size *= 2;
            tmp = realloc(u32_data, u32_size);

            if (!tmp)
            {
                free(u32_data);
                return -ENOMEM;
            }

            u32_data = tmp;
        }
    } while ((size_t)ret == u32_size);

    *u32_str = u32_data;

    return ret;
}
