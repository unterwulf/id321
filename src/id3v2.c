#include <arpa/inet.h> /* ntohl(), htonl() */
#include <ctype.h>     /* isdigit(), isupper() */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "dump.h"
#include "framelist.h"
#include "id3v2.h"
#include "output.h"
#include "synchsafe.h"

#define READORDIE(fd, buf, size, ret) \
    if (readordie(fd, buf, size) != 0) \
        return ret;

#define IS_WHOLE_TAG_UNSYNC(hdr) \
    ((hdr.version == 2 || hdr.version == 3) && hdr.flags & ID3V2_FLAG_UNSYNC)

struct id3v2_tag *new_id3v2_tag()
{
    struct id3v2_tag *tag = calloc(1, sizeof(struct id3v2_tag));

    if (tag)
        init_frame_list(&tag->frame_head);

    return tag;
}

void free_id3v2_tag(struct id3v2_tag *tag)
{
    if (!tag)
        return;

    free_frame_list(&tag->frame_head);
    free(tag);
}

int is_valid_frame_id_str(const char *str, size_t len)
{
    for (; len > 0; str++, len--)
        if (!isupper(*str) && !isdigit(*str))
            return 0;

    return 1;
}

int is_valid_frame_id(const char *str)
{
    size_t len = strlen(str);

    if (len > ID3V2_FRAME_ID_MAX_SIZE)
        return 0;

    return is_valid_frame_id_str(str, len);
}

static void unpack_id3v2_frame_header(const unsigned char *buf,
                                      unsigned version,
                                      struct id3v2_frame *frame)
{
    if (version == 2)
    {
        memcpy(frame->id, buf, 3);
        frame->size = ((uint32_t)buf[3] << 16) |
                      ((uint32_t)buf[4] << 8) |
                      (uint32_t)buf[5];
        /* no need to do ntohl(frame->size), as we use shift operator here */
        frame->status_flags = 0;
        frame->format_flags = 0;
    }
    else
    {
        memcpy(frame->id, buf, 4);
        memcpy(&frame->size, buf + 4, sizeof(uint32_t));
        frame->size = ntohl(frame->size);
        frame->status_flags = buf[8];
        frame->format_flags = buf[9];

        if (version == 4)
            frame->size = deunsync_uint32(frame->size);
    }
}

static ssize_t read_unsync(int fd, void *buf, size_t size, char *pre)
{
    size_t realsize;
    ssize_t bytes_read = 0;
    char *ptr = buf;

    while (readordie(fd, ptr, size) == 0)
    {
        bytes_read += size;
        realsize = deunsync_buf(ptr, size, *pre);
        *pre = *(ptr + size - 1);

        if (realsize < size)
        {
            size -= realsize;
            ptr += realsize;
        }
        else
            return bytes_read;
    }

    return -1;
}

int read_id3v2_frames(int fd, struct id3v2_tag *tag)
{
    uint8_t buf[ID3V2_FRAME_HEADER_SIZE];
    size_t  bytes_left = tag->header.size;
    size_t  frame_header_size = (tag->header.version == 2)
                                ? ID3V22_FRAME_HEADER_SIZE
                                : ID3V2_FRAME_HEADER_SIZE;
    char    pre = '\0';
    ssize_t bytes_read = 0;


    while (bytes_left > frame_header_size)
    {
        struct id3v2_frame *frame = NULL;

        if (IS_WHOLE_TAG_UNSYNC(tag->header))
        {
            bytes_read = read_unsync(fd, buf, frame_header_size, &pre);
            if (bytes_read == -1)
                return -EFAULT;
        }
        else
        {
            READORDIE(fd, buf, frame_header_size, -EFAULT);
            bytes_read = frame_header_size;
        }

        /* check if there is a padding */
        if (buf[0] == '\0')
        {
            lseek(fd, -bytes_read, SEEK_CUR);
            break;
        }

        bytes_left -= bytes_read;

        frame = calloc(1, sizeof(struct id3v2_frame));

        if (!frame)
            return -ENOMEM;

        unpack_id3v2_frame_header(buf, tag->header.version, frame);

        if (frame->size > bytes_left)
        {
            print(OS_ERROR, "frame '%.4s' size is %d, but space left is %d",
                  frame->id, frame->size, bytes_left);
            free(frame);
            return -EFAULT;
        }

        frame->data = malloc(frame->size);

        if (!frame->data)
        {
            free(frame);
            return -ENOMEM;
        }

        if (IS_WHOLE_TAG_UNSYNC(tag->header))
        {
            bytes_read = read_unsync(fd, frame->data, frame->size, &pre);
            if (bytes_read == -1)
            {
                free_frame(frame);
                return -EFAULT;
            }
        }
        else
        {
            if (readordie(fd, frame->data, frame->size) != 0)
            {
                free_frame(frame);
                return -EFAULT;
            }
            bytes_read = frame->size;
        }

        bytes_left -= bytes_read;
        dump_frame(frame);

        if (tag->header.version == 4
                && frame->format_flags & ID3V24_FRM_FMT_FLAG_UNSYNC)
        {
            frame->size = deunsync_buf(frame->data, frame->size, '\0');
        }

        append_frame(&tag->frame_head, frame);
    }

    /* check that padding contains zero bytes only */
    if (bytes_left > 0)
    {
        char   block[BLOCK_SIZE];
        size_t bytes_to_read;
        size_t pos;

        print(OS_DEBUG, "padding length is %u bytes", bytes_left);

        while (bytes_left > 0)
        {
            bytes_to_read = (bytes_left > sizeof(block))
                ? sizeof(block) : bytes_left;

            READORDIE(fd, block, bytes_to_read, -EFAULT);
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

    return -EFAULT;
}

static int validate_id3v2_header(const struct id3v2_header *hdr)
{
    size_t i;
    static const struct flagmask
    {
        uint8_t version;
        uint8_t mask;
    }
    fm[] =
    {
        { 2, ID3V22_FLAG_MASK },
        { 3, ID3V23_FLAG_MASK },
        { 4, ID3V24_FLAG_MASK }
    };

    for_each (i, fm)
    {
        if (fm[i].version == hdr->version)
        {
            if ((hdr->flags & ~fm[i].mask) != 0)
            {
                print(OS_WARN, "header has extra flags that are absent "
                               "in spec");
                break;
            }
        }
    }
}

static void unpack_id3v2_header(const char *buf, struct id3v2_header *hdr)
{
    hdr->version = (uint8_t)buf[3];
    hdr->revision = (uint8_t)buf[4];
    hdr->flags = (uint8_t)buf[5];
    memcpy(&hdr->size, buf + 6, sizeof(uint32_t));
    hdr->size = deunsync_uint32(ntohl(hdr->size));
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
        unpack_id3v2_header(headerBuf, header);
        return 1;
    }
    else
        return 0;
}
#endif

static int read_id3v2_headfoot(int fd, struct id3v2_header *hdr, int footer)
{
    char         buf[ID3V2_HEADER_LEN];
    size_t       pos;
    const char  *id = footer ? "3DI" : "ID3";
    int          ret;

    ret = readordie(fd, buf, ID3V2_HEADER_LEN);

    if (ret != 0)
        return ret == -ENOENT ? ret : -EFAULT;

    if (!memcmp(buf, id, 3))
    {
        for (pos = 3; pos < ID3V2_HEADER_LEN; pos++)
        {
            if (((uint8_t)buf[pos] == 0xFF && (pos == 3 || pos == 4))
                    || (((uint8_t)buf[pos] & 0x80) && pos >= 6 && pos <= 9))
                return -ENOENT;
        }

        unpack_id3v2_header(buf, hdr);

        if (hdr->version != 2 && hdr->version != 3 && hdr->version != 4)
        {
            print(OS_ERROR, "i don't know ID3v2.%d", hdr->version);
            return -ENOENT;
        }

        return 0;
    }

    return -ENOENT;
}

int read_id3v2_header(int fd, struct id3v2_header *hdr)
{
    return read_id3v2_headfoot(fd, hdr, 0);
}

int read_id3v2_footer(int fd, struct id3v2_header *hdr)
{
    int ret = read_id3v2_headfoot(fd, hdr, 1);

    if (ret == 0 && hdr->version != 4)
    {
        print(OS_WARN, "appended tag is not allowed for ID3v2.%d",
                       hdr->version);
        return -ENOENT;
    }

    return ret;
}

/*
 * Pack functions
 */

/***
 * pack_id3v2_frame
 *
 * Returns packed frame size or (size_t)-1 if the frame is too big to be
 * packed correctly.
 */

static size_t pack_id3v2_frame(const struct id3v2_frame *frame,
                               struct id3v2_header *hdr,
                               char post,
                               char *buf, size_t size)
{
    size_t  hdr_size = (hdr->version == 2)
                       ? ID3V22_FRAME_HEADER_SIZE
                       : ID3V2_FRAME_HEADER_SIZE;
    uint8_t format_flags = frame->format_flags;
    size_t  payload_size;
    char   *hdr_buf = buf;
    static const uint32_t frame_max_size[] =
    {
        /* unused */ 0,
        /* unused */ 0,
        /*  v2.2  */ ID3V22_FRAME_MAX_SIZE,
        /*  v2.3  */ ID3V23_FRAME_MAX_SIZE,
        /*  v2.4  */ ID3V24_FRAME_MAX_SIZE,
    };

    if (size < hdr_size + frame->size)
        return hdr_size + frame->size;

    buf += hdr_size;

    if (hdr->version == 4 && !(g_config.options & ID321_OPT_NO_UNSYNC))
    {
        payload_size = unsync_buf(buf, size - hdr_size,
                                  frame->data, frame->size, post);

        /* check if unsynchronisation has changed the frame payload */
        if (payload_size != frame->size)
            format_flags |= ID3V24_FRM_FMT_FLAG_UNSYNC;
        else
        {
            format_flags &= ~ID3V24_FRM_FMT_FLAG_UNSYNC;
            /* reset the unsync bit in the tag header, as at least one
             * frame has not been affected by unsynchronisation */
            hdr->flags &= ~ID3V2_FLAG_UNSYNC;
        }
    }
    else
    {
        memcpy(buf, frame->data, frame->size);
        payload_size = frame->size;
    }

    /* check frame payload size */
    if (payload_size > frame_max_size[hdr->version])
        return (size_t)-1;

    if (hdr->version == 2)
    {
        memcpy(hdr_buf, frame->id, 3);
        /* no need to do htonl(payload_size), as we use shift operator here */
        hdr_buf[3] = (uint8_t)((payload_size >> 16) & 0xFF);
        hdr_buf[4] = (uint8_t)((payload_size >> 8) & 0xFF);
        hdr_buf[5] = (uint8_t)(payload_size & 0xFF);
    }
    else
    {
        uint32_t net_payload_size = (hdr->version == 4)
                                    ? htonl(unsync_uint32(payload_size))
                                    : htonl(payload_size);

        memcpy(hdr_buf, frame->id, 4);
        memcpy(hdr_buf + 4, &net_payload_size, sizeof(uint32_t));
        hdr_buf[8] = frame->status_flags;
        hdr_buf[9] = format_flags;
    }

    return hdr_size + payload_size;
}

static void pack_id3v2_header(const struct id3v2_header *hdr, char *buf)
{
    uint32_t net_tag_size = htonl(unsync_uint32(hdr->size));

    strcpy(buf, "ID3");
    buf[3] = hdr->version;
    buf[4] = hdr->revision;
    buf[5] = hdr->flags;
    memcpy(buf + 6, &net_tag_size, sizeof(uint32_t));
}

ssize_t pack_id3v2_tag(const struct id3v2_tag *tag, char **buf)
{
    struct id3v2_header header = tag->header;
    const struct id3v2_frame *frame;
    size_t bufsize = header.size > BLOCK_SIZE ? header.size : BLOCK_SIZE;
    size_t pos = ID3V2_HEADER_LEN;
    size_t frame_size;
    size_t newsize;

    *buf = malloc(bufsize);

    if (!*buf)
        return -ENOMEM;

    if (header.version == 4 && !(g_config.options & ID321_OPT_NO_UNSYNC))
        header.flags |= ID3V2_FLAG_UNSYNC;
    else
        header.flags &= ~ID3V2_FLAG_UNSYNC;

    for (frame = tag->frame_head.next; frame != &tag->frame_head;)
    {
        char post = (frame->next == &tag->frame_head)
                    ? '\xFF' : frame->next->id[0];

        /* According to the ID3v2.3 and ID3v2.4 specifications the last byte
         * of the last frame in the tag should be unsynchronised in case
         * it is 0xFF. unsync_buf() implements this functionality in
         * a general way, it takes a byte after buffer to decide whether
         * the last byte should be unsynchronised or not. So we pass
         * '\xFF' as a byte after buffer for the last frame to show that
         * the last byte needs to be unsynchronised.
         *
         * Note that for the rest of frames we pass the first byte of the
         * next frame ID as a byte after buffer because it will actually be
         * such a byte. According to the standards frame ID should be
         * composed of [A-Z0-9], so it will guarantee that the last byte of
         * each frame but last will not be unsynchronised. */

        frame_size = pack_id3v2_frame(frame, &header, post,
                                      *buf+pos, bufsize-pos);

        if (frame_size == (size_t)-1)
        {
            /* frame too big */
            free(*buf);
            *buf = NULL;
            return -EINVAL;
        }
        else if (frame_size > bufsize - pos)
        {
            char *newbuf;
            size_t newbufsize = 2*bufsize;

            while (newbufsize < pos + frame_size)
                newbufsize *= 2;

            newbuf = realloc(*buf, newbufsize);

            if (!newbuf)
                goto oom; /* *buf will be freed there */

            bufsize = newbufsize;
            *buf = newbuf;
            continue;
        }

        pos += frame_size;
        frame = frame->next;
    }

    if ((header.version == 2 || header.version == 3)
        && !(g_config.options & ID321_OPT_NO_UNSYNC))
    {
        size_t reqbufsize = ID3V2_HEADER_LEN +
                            unsync_buf(NULL, 0, *buf + ID3V2_HEADER_LEN,
                                       pos - ID3V2_HEADER_LEN, '\xFF');

        /* check if unsynchronisation has changed the tag payload */
        if (reqbufsize != pos)
        {
            size_t newbufsize = reqbufsize;
            char *newbuf = malloc(newbufsize);

            if (!newbuf)
                goto oom;

            (void)unsync_buf(newbuf + ID3V2_HEADER_LEN,
                             reqbufsize - ID3V2_HEADER_LEN,
                             *buf + ID3V2_HEADER_LEN,
                             pos - ID3V2_HEADER_LEN, '\xFF');

            memcpy(newbuf, *buf, ID3V2_HEADER_LEN);
            free(*buf);
            bufsize = newbufsize;
            *buf = newbuf;
            pos = reqbufsize;

            if (reqbufsize > header.size + ID3V2_HEADER_LEN)
                header.size = reqbufsize - ID3V2_HEADER_LEN;

            header.flags |= ID3V2_FLAG_UNSYNC;
        }
    }

    /* if new tag size has not been specified, we will try to use old tag
     * space not changed */

    newsize = g_config.options & ID321_OPT_CHANGE_SIZE
        ? g_config.size : header.size + ID3V2_HEADER_LEN;

    if (pos <= newsize)
    {
        /* it seems there should be a padding, so fill it with null bytes */

        if (bufsize < newsize)
        {
            /* damn, the current buffer is not large enough; need to
             * enlarge it */
            char *tmp = realloc(*buf, newsize);

            if (!tmp)
               goto oom; /* *buf will be freed there */

            *buf = tmp;
        }

        memset(*buf + pos, '\0', newsize - pos);
        pos = newsize;
    }

    header.size = pos - ID3V2_HEADER_LEN;

    if (header.size > ID3V2_TAG_MAX_SIZE)
    {
        /* tag too big */
        free(*buf);
        *buf = NULL;
        return -E2BIG;
    }

    pack_id3v2_header(&header, *buf);

    return pos;

oom:

    free(*buf);
    *buf = NULL;
    return -ENOMEM;
}
