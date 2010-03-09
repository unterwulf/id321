#include <inttypes.h>
#include <unistd.h>
#include "id3v2.h"

ss_uint32_t unsync_uint32(uint32_t src)
{
    uint32_t res;

    /* The unsyncronisation scheme is the following:
     *
     * pos: 76543210 76543210 76543210 76543210
     * src: FEDCBAzy xwvutsrq ponmlkji hgfedcba
     * res: 0BAzyxwv 0utsrqpo 0nmlkjih 0gfedcba
     */

    res = src & 0x7F |
         (src & 0x3F80) << 1 |
         (src & 0x1FC000) << 2 |
         (src & 0xFE00000) << 3;

    return (ss_uint32_t)res;
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

    res = src & 0x7F |
         (src & 0x7F00) >> 1 |
         (src & 0x7F0000) >> 2 |
         (src & 0x7F000000) >> 3;

    return res;
}

/*
int unsyncBuffer(char *)
{
}
*/

/*
 *
 */

uint16_t deunsync_buf(uint8_t *buf, uint16_t size, int pre)
{
    uint16_t  pos = 0;
    uint8_t  *wr_pos = buf;

    /* checking precondition */
    if (pre == 1 && buf[0] == '\0')
    {
        pos++;
    }

    for (; pos < size; pos++)
    {
        *(wr_pos++) = buf[pos];

        if (buf[pos] == 0xFF && pos + 1 < size && buf[pos+1] == 0x00)
        {
            pos++;
        }
    }

    return wr_pos - buf;
}

ssize_t read_unsync(int fd, void *buf, size_t size, int pre)
{
    size_t realsize;
    ssize_t bytes_read = 0;

    while (readordie(fd, buf, size) == size)
    {
        bytes_read += size;
        realsize = deunsync_buf(buf, size, pre);
        pre = (((uint8_t *)buf)[realsize - 1] == 0xFF);

        if (realsize < size)
        {
            size -= realsize;
            buf += realsize;
        }
        else
            return bytes_read;
    }

    return -1;
}
