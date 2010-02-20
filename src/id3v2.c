#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "id3v2.h"
#include "frames.h"
#include "synchsafe.h"
#include "dump.h"
#include "output.h"
#include "common.h"

#define IS_WHOLE_TAG_UNSYNC(hdr) \
    ((hdr.version == 2 || hdr.version == 3) && hdr.flags & ID3V2_FLAG_UNSYNC)

int id3_tag_add_frame(struct id3v2_tag *tag, struct id3v2_frame *frame)
{
    struct id3v2_frame_list *cur_node = NULL;
    struct id3v2_frame_list *new_node = NULL;

    if ((new_node = (struct id3v2_frame_list *)
                malloc(sizeof(struct id3v2_frame_list))) == NULL)
    {
        return 0;
    }
    new_node->next = NULL;

    memcpy(&new_node->frame, frame, sizeof(struct id3v2_frame));

    if (tag->first_frame == NULL)
    {
        tag->first_frame = new_node;
    }
    else
    {
        cur_node = tag->first_frame;
        while (cur_node->next != NULL)
            cur_node = cur_node->next;

        cur_node->next = new_node;
    }

    return 1;
}

static const char *map_v22_frame_to_v24(const char *v22frame)
{
    /* TODO: */

    return NULL;
}

static int unpack_id3v2_frame_header(
    struct id3v2_frame *frame,
    const unsigned char *buf,
    unsigned char version)
{
    const char *frame_id;

    if (version == 2)
    {
        uint8_t size[4] = {0};

        frame_id = map_v22_frame_to_v24(buf);

        if (frame_id == NULL)
        {
            print(OS_ERROR, "unknown id3v2.2 frame %.3s", buf);
            return -1;
        }

        memcpy(frame->id, frame_id, 4);
        /* size array will contain 4-byte word in network byte order */
        memcpy(size, buf + 3, 3);
        frame->size = ntohl(*((uint32_t *)size));
        frame->status_flags = 0;
        frame->format_flags = 0;
    }
    else
    {
        frame_id = map_v23_to_v24(buf);

        if (version == 3 && frame_id != NULL)
        {
            memcpy(frame->id, frame_id, 4);
        }
        else
        {
            memcpy(frame->id, buf, 4);
        }

        frame->size = ntohl(*((uint32_t *)&(buf[4])));
        frame->status_flags = buf[8];
        frame->format_flags = buf[9];

        if (version == 4)
            frame->size = deunsync_uint32(frame->size);
    }

    return 0;
}

int read_id3v2_frames(int fd, struct id3v2_tag *tag)
{
    uint32_t            size;
    uint8_t             buf[ID3V2_FRAME_HEADER_SIZE];
    int32_t             bytes_left = tag->header.size;
    struct id3v2_frame  frame;
    int                 retval;
    int                 frame_header_size = ID3V2_FRAME_HEADER_SIZE;
    int                 pre = 0;
    ssize_t             bytes_read = 0;

    if (tag->header.version == 2)
    {
        frame_header_size = ID3V22_FRAME_HEADER_SIZE;
    }

    while (bytes_left > frame_header_size)
    {
        if (IS_WHOLE_TAG_UNSYNC(tag->header))
        {
            bytes_read = read_unsync(fd, buf, frame_header_size, pre);
            if (bytes_read == -1)
                return -1;
            pre = (buf[frame_header_size - 1] == 0xFF);
        }
        else
        {
            READORDIE(fd, buf, frame_header_size);
            bytes_read = frame_header_size;
        }

        /* check if there is a padding */
        if (buf[0] == '\0')
        {
            lseek(fd, -bytes_read, SEEK_CUR);
            break;
        }

        memset(&frame, 0, sizeof(frame));
        bytes_left -= bytes_read;

        unpack_id3v2_frame_header(&frame, buf, tag->header.version);

        if (frame.size > bytes_left)
        {
            print(OS_ERROR, "size was %d, space left %d",
                    frame.size, bytes_left);
            return -1;
        }

        if ((frame.data = malloc(frame.size)) == NULL)
        {
            return;
        }

        if (IS_WHOLE_TAG_UNSYNC(tag->header))
        {
            bytes_read = read_unsync(fd, frame.data, frame.size, pre);
            if (bytes_read == -1)
                return -1;
            pre = (buf[frame.size - 1] == 0xFF);
        }
        else
        {
            READORDIE(fd, frame.data, frame.size);
            bytes_read = frame.size;
        }

        bytes_left -= bytes_read;
        dump_frame(&frame);

        if (tag->header.version == 4
                && frame.format_flags & ID3V24_FRM_FMT_FLAG_UNSYNC)
        {
            frame.size = deunsync_buf(frame.data, frame.size, 0);
        }

        unpack_frame_data(&frame);

        if (!id3_tag_add_frame(tag, &frame))
            return;
    }

    /* check that padding contains zero bytes only */
    if (bytes_left > 0)
    {
        uint8_t block[BLOCK_SIZE];
        size_t bytes_to_read;
        size_t pos;

        print(OS_DEBUG, "padding length is %u bytes", bytes_left);

        while (bytes_left > 0)
        {
            bytes_to_read = (bytes_left > sizeof(block))
                ? sizeof(block) : bytes_left;

            READORDIE(fd, block, bytes_to_read);
            bytes_left -= bytes_to_read;

            for (pos = 0; pos < bytes_to_read; pos++)
            {
                if (block[pos] != '\0')
                {
                    print(OS_DEBUG, "padding contains non-zero byte");
                    lseek(fd, bytes_left, SEEK_CUR);
                    break;
                }
            }
        }
    }

    return 0;
}

int read_id3v2_ext_header(int fd, struct id3v2_tag *tag)
{
    if (tag->header.flags & ID3V2_FLAG_EXT_HEADER)
    {

    }
}

int validate_id3v2_header(struct id3v2_header *hdr)
{
    int i;
    static const struct flagmask {
        uint8_t version;
        uint8_t mask;
    }
    fm[] = {
        { 2, ID3V22_FLAG_MASK },
        { 3, ID3V23_FLAG_MASK },
        { 4, ID3V24_FLAG_MASK }
    };

    for (i = 0; i < sizeof(fm)/sizeof(fm[0]); i++)
    {
        if (fm[i].version == hdr->version)
        {
            if (hdr->flags & ~fm[i].mask != 0)
            {
                print(OS_WARN, "header has extra flags that are absented "
                               "in spec");
                break;
            }
        }
    }
}

int unpack_id3v2_header(struct id3v2_header *hdr, const unsigned char *buf)
{
    hdr->version = buf[3];
    hdr->revision = buf[4];
    hdr->flags = buf[5];
    hdr->size = deunsync_uint32(ntohl(*((uint32_t*)&(buf[6]))));
}

#if 0
int find_id3v2_tag(struct id3v2_header *header, FILE *fp_src, FILE *fp_dst)
{
    char headerBuf[10];
    unsigned int ch;
    int pos = 0;

    while (pos != ID3V2_HEADER_LEN && !feof(fp_src))
    {
        ch = fgetc(fp_src);

        if (pos == 0 && ch == 'I'
                || (pos == 1 && ch == 'D')
                || (pos == 2 && ch == '3')
                || (ch != 0xFF && (pos == 3 || pos == 4))
                || (pos == 5)
                || (ch < 0x80 && pos >= 6 && pos <= 9))
            headerBuf[pos++] = ch;
        else
        {
            if (fp_src != NULL)
                fwrite(headerBuf, pos, 1, fp_src);
            //pos = 0;
            return 0;
        }
    }

    if (pos == ID3V2_HEADER_LEN)
    {
        unpack_id3v2_header(header, headerBuf);
        return 1;
    }
    else
        return 0;
}
#endif

static int read_id3v2_headfoot(int fd, struct id3v2_header *hdr, int footer)
{
    unsigned char buf[ID3V2_HEADER_LEN];
    int           pos;
    const char   *id = footer ? "3DI" : "ID3";

    read(fd, buf, ID3V2_HEADER_LEN);

    if (memcmp(buf, id, 3) == 0)
    {
        for (pos = 3; pos < ID3V2_HEADER_LEN; pos++)
        {
            if ((buf[pos] == 0xFF && (pos == 3 || pos == 4))
                    || (buf[pos] >= 0x80 && pos >= 6 && pos <= 9))
                return -1;
        }

        unpack_id3v2_header(hdr, buf);

        if (hdr->version != 3 && hdr->version != 4)
        {
            print(OS_ERROR, "i don't know id3v2.%d", hdr->version);
            return -1;
        }

        return 0;
    }

    return -1;
}

int read_id3v2_header(int fd, struct id3v2_header *hdr)
{
    return read_id3v2_headfoot(fd, hdr, 0);
}

int read_id3v2_footer(int fd, struct id3v2_header *hdr)
{
    int retval = read_id3v2_headfoot(fd, hdr, 1);

    if (retval == 0 && hdr->version != 4)
    {
        print(OS_WARN, "id3v2 appended tag is not allowed for id3v2.%d",
                hdr->version);
        return -1;
    }

    return retval;
}
