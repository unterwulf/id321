#include <inttypes.h>
#include <unistd.h>
#include "synchsafe.h"

ss_uint32_t unsync_uint32(uint32_t src)
{
    ss_uint32_t res;

    /* The unsyncronisation scheme is the following:
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

    /* The deunsyncronisation scheme is the following:
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

/*
 * Function:     unsync_buf
 *
 * Description:  Unsynchronises buffer src of size srcsize and write the result
 *               to the buffer dst of size dstsize
 *
 * Return value: size of unsynchronised buffer, if it is greater than
 *               dstsize the buffer was truncated
 *
 * Notes:        May be called with dstsize == 0 to estimate required
 *               destination buffer size
 */
size_t unsync_buf(char *dst, size_t dstsize, const char *src, size_t srcsize)
{
    size_t pos;
    size_t dstpos;
    unsigned char *usrc = (unsigned char *)src; /* do not spell it in Russian */

    for (pos = 0, dstpos = 0; pos < srcsize && dstpos < dstsize; pos++)
    {
        dst[dstpos++] = src[pos];

        if (usrc[pos] == 0xFF &&
            ((pos + 1 < srcsize
              && ((usrc[pos+1] & 0xE0) == 0xE0 || usrc[pos+1] == '\0'))
             || pos + 1 == srcsize))
        {
            if (dstpos < dstsize)
                dst[dstpos++] = '\0';
            else
                dstpos++;
        }
    }

    /* just estimate required buffer size */
    for (dstpos += srcsize - pos; pos < srcsize; pos++)
        if (usrc[pos] == 0xFF &&
            ((pos + 1 < srcsize
              && ((usrc[pos+1] & 0xE0) == 0xE0 || usrc[pos+1] == '\0'))
             || pos + 1 == srcsize))
            dstpos++;

    return dstpos;
}

size_t deunsync_buf(char *buf, size_t size, int pre)
{
    size_t  pos = 0;
    char   *wr_pos = buf;

    /* checking precondition */
    if (pre == 1 && buf[0] == '\0')
        pos++;

    for (; pos < size; pos++)
    {
        *(wr_pos++) = buf[pos];

        if ((unsigned char)buf[pos] == 0xFF
            && pos + 1 < size && buf[pos+1] == '\0')
            pos++;
    }

    return wr_pos - buf;
}
