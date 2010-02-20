#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include "params.h"
#include "id3v2.h"
#include "id3v1.h"
#include "output.h"
#include "common.h"

struct crop_area
{
    off_t start;
    off_t end;
};

static int crop_id3v1_header(int fd, struct crop_area *crop)
{
    char buf[4];

    if (crop->end - ID3V1_TAG_SIZE >= crop->start)
    {
        lseek(fd, crop->end - ID3V1_TAG_SIZE, SEEK_SET);

        read(fd, buf, ID3V1_HEADER_SIZE);

        if (memcmp(buf, ID3V1_HEADER, ID3V1_HEADER_SIZE) == 0)
        {
            print(OS_DEBUG, "id3v1 tag found");
            crop->end -= ID3V1_TAG_SIZE;
        }

        /* check presence of id3v1 enhanced tag */
        if (crop->end - (ID3V1E_TAG_SIZE - ID3V1_TAG_SIZE) >= crop->start)
        {
            lseek(fd, crop->end - (ID3V1E_TAG_SIZE - ID3V1_TAG_SIZE), SEEK_SET);

            read(fd, buf, ID3V1E_HEADER_SIZE);

            if (memcmp(buf, ID3V1E_HEADER, ID3V1E_HEADER_SIZE) == 0)
            {
                print(OS_DEBUG, "id3v1 enhanced tag found");
                crop->end -= ID3V1E_TAG_SIZE - ID3V1_TAG_SIZE;
            }
        }
    }

    return 0;
}

static int crop_id3v2_header(int fd, struct crop_area *crop)
{
    struct id3v2_header hdr;

    if (crop->start + ID3V2_HEADER_LEN < crop->end)
    {
        /* check presence of an id3v2 header at the very beginning
         * of the crop area */
        lseek(fd, crop->start, SEEK_SET);
        if (read_id3v2_header(fd, &hdr) != -1)
        {
            print(OS_DEBUG, "id3v2 tag found");
            crop->start += ID3V2_HEADER_LEN + hdr.size;
            if (hdr.version == 4 && hdr.flags & ID3V2_FLAG_FOOTER_PRESENT)
                crop->start += ID3V2_FOOTER_LEN;
        }
    }

    if (crop->end - ID3V2_FOOTER_LEN >= crop->start)
    {
        /* check presence of an id3v2 footer at the very end
         * of the crop area */
        lseek(fd, crop->end - ID3V2_FOOTER_LEN, SEEK_SET);
        if (read_id3v2_footer(fd, &hdr) != -1)
        {
            print(OS_DEBUG, "id3v2 appended tag found");
            crop->end -= ID3V2_HEADER_LEN + ID3V2_FOOTER_LEN + hdr.size;
        }
    }

    return 0;
}

static int write_ent(int fd, const void *buf, size_t count)
{
    ssize_t written = 0;
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
        {
            written += len;
        }
    }

    return written == count;
}

static int crop_file(int fd, const struct crop_area *crop)
{
    long    blksize = BLOCK_SIZE;
    char    buf[BLOCK_SIZE];
    off_t   rd_pos = crop->start;
    off_t   wr_pos = 0;
    ssize_t len;
    int     retval;

    while (rd_pos < crop->end)
    {
        lseek(fd, rd_pos, SEEK_SET);

        if (rd_pos + blksize > crop->end)
            blksize = crop->end - rd_pos;

        len = read(fd, buf, blksize);

        if (len > 0)
        {
            rd_pos += len;
            lseek(fd, wr_pos, SEEK_SET);
            retval = write_ent(fd, buf, len);
            if (retval)
            {
                wr_pos += len;
            }
            else
            {
                perror(__FUNCTION__);
                return -1;
            }
        }
        else if (len == 0)
        {
            print(OS_DEBUG, "EOF before end of crop area! OMG!");
            break;
        }
        else if (len < 0)
        {
            if (errno == EINTR)
                continue;
            else
            {
                perror(__FUNCTION__);
                return -1;
            }
        }
    }

    ftruncate(fd, crop->end - crop->start);

    if (retval < 0)
        perror(__FUNCTION__);

    return 0;
}

int delete_tags(const char *filename)
{
    int              fd;
    off_t            size = 0;
    struct crop_area crop;
    int              retval;

    fd = open(filename, O_RDWR);
    
    if (fd == -1)
    {
        print(OS_ERROR, "unable to open source file `%s'", filename);
        return;
    }

    size = lseek(fd, 0, SEEK_END);

    /* initialize crop params */
    crop.start = 0;
    crop.end = size;

    retval = crop_id3v2_header(fd, &crop);

    if (crop.end == size)
    {
        retval = crop_id3v1_header(fd, &crop);
    }

    if (crop.start == 0 && crop.end == size)
    {
        print(OS_DEBUG, "there are no tags in this file");
    }
    else
    {
        print(OS_DEBUG, "start: %d, end: %d, size: %d",
                crop.start, crop.end, size);

        crop_file(fd, &crop);
    }

    close(fd);
}
