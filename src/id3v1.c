#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v2.h"
#include "dump.h"
#include "common.h"
#include "output.h"
#include "params.h"

static int add_id3v1_frame(
        struct id3v2_tag *tag,
        const char *id,
        const char *buf,
        int size)
{
    struct id3v2_frame frame;

    if (size != 0 && buf[0] != '\0')
    {
        memset(&frame, 0, sizeof(frame));

        memcpy(frame.id, id, 4);
        frame.size = strnlen(buf, size) + 1;

        /* trim spaces at the end of buf */
        while (buf[frame.size - 2] == ' ' && frame.size > 0)
            frame.size--;

        if (frame.size == 0)
            return;

        frame.data = calloc(1, frame.size);
        memcpy(frame.data + 1, buf, frame.size - 1);

        dump_frame(&frame);

        unpack_frame_data(&frame);

        id3_tag_add_frame(tag, &frame);
    }
}

#if 0
int find_id3v1_tag(id3_tag_t *tag, FILE *fp)
{
    char title[92] = {0};
    char artist[92] = {0};
    char album[92] = {0};
    char genre[92] = {0};
    char buf[ID3V1_TAG_SIZE];
    char tag_buf[ID3V1_TAG_SIZE];
    char ext_tag_buf[ID3V1_EXT_TAG_SIZE];
    unsigned char track;
    unsigned char genre_index;
    unsigned char speed;

    tag->header.version = 1;
    tag->header.revision = 0;
    tag->header.size = ID3V1_TAG_SIZE;

    if (fseek(fp, -ID3V1_TAG_SIZE, SEEK_END) != 0)
        return 1;

    fread(tag_buf, ID3V1_TAG_SIZE, 1, fp);
    if (memcmp(tag_buf, "TAG", 3) != 0)
        return 1;

    // check if this is a id3v1.1 tag
    if (tag_buf[125] == '\0' && tag_buf[126] != '\0')
    {
        tag->header.revision = 1;
    }

    dump_id3_header(&tag->header);

    strncpy(title + 1, tag_buf + 3, 30);
    strncpy(artist + 1, tag_buf + 33, 30);
    strncpy(album + 1, tag_buf + 63, 30);
    sprintf(genre + 1, "(%u)", tag_buf[127]);

    // check if there is an extended tag
    if (fseek(fp, -(ID3V1_TAG_SIZE + ID3V1_EXT_TAG_SIZE), SEEK_END) == 0
        && fread(ext_tag_buf, ID3V1_EXT_TAG_SIZE, 1, fp) == ID3V1_EXT_TAG_SIZE)
    {
        if (memcmp(ext_tag_buf, "TAG+", 4) == 0)
        {
            strncat(title + 1, ext_tag_buf + 4, 60);
            strncat(artist + 1, ext_tag_buf + 64, 60);
            strncat(album + 1, ext_tag_buf + 124, 60);
/*
            speed = fgetc(fp);
            fread(genre + strlen(genre + 1), 30, 1, fp);
            fread(buf + 1, 6, 1, fp);
            read_id3v1_frame(tag, "TSTA", buf, strlen(buf + 1) + 1);
            fread(buf + 1, 6, 1, fp);
            read_id3v1_frame(tag, "TSTO", buf, strlen(buf + 1) + 1);
*/
        }
    }

    read_id3v1_frame(tag, "TIT2", title, strlen(title + 1) + 1);
    read_id3v1_frame(tag, "TPE1", artist, strlen(artist + 1) + 1);
    read_id3v1_frame(tag, "TALB", album, strlen(album + 1) + 1);
    read_id3v1_frame(tag, "TGEN", genre, strlen(genre + 1) + 1);

/*
    fread(buf + 1, 4, 1, fp);
    read_id3v1_frame(tag, "TYER", buf, 6);

    fread(buf + 1, 30, 1, fp);
    read_id3v1_frame(tag, "TCOM", buf, strlen(buf + 1) + 1);
*/
    // check if this is a id3v1.1 tag
    if (tag->header.revision == 1)
    {
        sprintf(buf + 1, "%u", tag_buf[126]);
        read_id3v1_frame(tag, "TNUM", buf, strlen(buf + 1) + 1);
    }

    return 0;
}
#endif

static read_id3v1_field(uint8_t **buf, char *field, size_t size)
{
    strncpy(field, *buf, size);
    *buf += size;
    size--;
    for (; (field[size] == ' ' || field[size] == '\0') && size > 0; size--)
    {
        field[size] == '\0';
    }
}

ssize_t read_id3v1_tag(int fd, struct id3v1_tag *tag)
{
    uint8_t           buf[ID3V1_TAG_SIZE];
    struct lib313_tag tag313;

    READORDIE(fd, buf, ID3V1_TAG_SIZE);

    if (lib313_unpack(&tag313, buf) != LIB313_SUCCESS)
        return -1;

    print(OS_DEBUG, "id3v1.%d tag found", tag313.version);
    memset(tag, '\0', sizeof(*tag));

    tag->version = tag313.version;
    strncpy(tag->title,   tag313.title,   ID3V13_MAX_TIT_SIZE);
    strncpy(tag->artist,  tag313.artist,  ID3V13_MAX_ART_SIZE);
    strncpy(tag->album,   tag313.album,   ID3V13_MAX_ALB_SIZE);
    strncpy(tag->year,    tag313.year,    ID3V1_YER_SIZE);
    strncpy(tag->comment, tag313.comment, ID3V13_MAX_COM_SIZE);
    tag->track = tag313.track;
    tag->genre = tag313.genre;

    return 0;
}

ssize_t read_id3v1_ext_tag(int fd, struct id3v1_tag *tag)
{
    return -1;
}

ssize_t read_id3v12_tag(int fd, struct id3v1_tag *tag)
{
    uint8_t buf[ID3V1_TAG_SIZE + ID3V12_TAG_SIZE];
    uint8_t framebuf[60];

    READORDIE(fd, buf, ID3V1_TAG_SIZE + ID3V12_TAG_SIZE);

    if (memcmp(buf, ID3V12_HEADER, ID3V12_HEADER_SIZE) == 0
        && memcmp(buf + ID3V12_TAG_SIZE, ID3V1_HEADER, ID3V1_HEADER_SIZE) == 0)
    {
        memcpy(framebuf + 30, buf + ID3V12_TAG_SIZE + ID3V1_HEADER_SIZE, 30);
        memcpy(framebuf + 30, buf + ID3V12_HEADER_SIZE, 30);
    }

    return -1;
}
