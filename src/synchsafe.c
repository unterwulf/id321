#include <inttypes.h>   /* uint32_t */
#include <stddef.h>     /* size_t */
#include "synchsafe.h"  /* ss_uint32_t */

ss_uint32_t unsync_uint32(uint32_t src)
{
    ss_uint32_t res;

    /* The unsynchronisation scheme is the following:
     *
     * pos: 76543210 76543210 76543210 76543210
     * src: FEDCBAzy xwvutsrq ponmlkji hgfedcba
     * res: 0BAzyxwv 0utsrqpo 0nmlkjih 0gfedcba
     */

    res = (src & 0x7F) |
         ((src & 0x3F80) << 1) |
         ((src & 0x1FC000) << 2) |
         ((src & 0xFE00000) << 3);

    return res;
}

uint32_t deunsync_uint32(ss_uint32_t src)
{
    uint32_t res;

    /* The deunsynchronisation scheme is the following:
     *
     * pos: 76543210 76543210 76543210 76543210
     * src: 0BAzyxwv 0utsrqpo 0nmlkjih 0gfedcba
     * res: 0000BAzy xwvutsrq ponmlkji hgfedcba
     */

    res = (src & 0x7F) |
         ((src & 0x7F00) >> 1) |
         ((src & 0x7F0000) >> 2) |
         ((src & 0x7F000000) >> 3);

    return res;
}

/***
 * unsync_buf - unsynchronise buffer
 *
 * The function unsynchronises the buffer @src of size @srcsize and writes
 * a result to the buffer @dst of size @dstsize. It may be called with
 * @dstsize == 0 to estimate the necessary destination buffer size.
 *
 * Returns the size of the unsynchronised buffer, if it is greater than
 * @dstsize the value in @dst is truncated.
 */

size_t unsync_buf(char *dst, size_t dstsize,
                  const char *src, size_t srcsize, char post)
{
    size_t ressize = dstsize;

    for (; srcsize > 0 && dstsize > 0; src++, dst++, srcsize--, dstsize--)
    {
        *dst = *src;

        if (*src == '\xFF' &&
            (srcsize > 1 && ((*(src+1) & '\xE0') == '\xE0' || *(src+1) == '\0')
             || srcsize == 1 && (post & '\xE0') == '\xE0' || post == '\0'))
        {
            if (dstsize > 1)
            {
                *++dst = '\0';
                dstsize--;
            }
            else
                ressize++;
        }
    }

    /* just estimate required buffer size */
    for (ressize += srcsize; srcsize > 0; src++, srcsize--)
        if (*src == '\xFF' &&
            (srcsize > 1 && ((*(src+1) & '\xE0') == '\xE0' || *(src+1) == '\0')
             || srcsize == 1 && (post & '\xE0') == '\xE0' || post == '\0'))
            ressize++;

    return ressize - dstsize;
}

size_t deunsync_buf(char *buf, size_t size, char pre)
{
    char *wrptr = buf;
    size_t origsize = size;

    /* checking precondition */
    if (pre == '\xFF' && size > 0 && *buf == '\0')
    {
        buf++;
        size--;
    }

    if (size > 0)
    {
        *wrptr++ = *buf++;
        size--;
    }

    for (; size > 0; buf++, size--)
        if (*(buf-1) != '\xFF' || *buf != '\0')
            *wrptr++ = *buf;

    return origsize - (buf - wrptr);
}
