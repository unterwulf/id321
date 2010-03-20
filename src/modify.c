#include <stdlib.h>
#include <stdio.h>  /* sscanf() */
#include <string.h>
#include <assert.h>
#include "common.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v2.h"
#include "params.h"
#include "output.h"
#include "alias.h"
#include "framelist.h"
#include "frames.h"

#define copy_if_not_null(field, dst, src) \
    if ((src).field) \
    { \
        memset((dst).field, '\0', sizeof((dst).field)); \
        istrncpy((dst).field, (src).field, sizeof((dst).field) - 1); \
    }

static char *istrncpy(char *dst, const char *src, size_t size)
{
    char *res = NULL;
    size_t bufsize;
    char *buf;
    int ret;
    
    ret = iconv_alloc(g_config.enc_v1, locale_encoding(),
                      src, strlen(src), &buf, &bufsize);

    if (ret == 0)
    {
        /* size is a field length - 1, so the field will be null terminated */
        if (bufsize > size)
            bufsize = size;
        res = strncpy(dst, buf, bufsize);
        free(buf);
    }

    return res;
}

int modify_tags(const char *filename)
{
    struct id3v1_tag *tag1 = NULL;
    struct id3v2_tag *tag2 = NULL;
    struct version    ver = { g_config.ver.major, NOT_SET };
    int               ret;

    ret = get_tags(filename, ver, &tag1, &tag2);

    if (ret != 0)
        return -1;

    if (g_config.ver.major == 1 && !tag1)
    {
        tag1 = malloc(sizeof(struct id3v1_tag));

        if (tag1)
            tag1->version = g_config.ver.minor != NOT_SET
                ? g_config.ver.minor : 3;
        else
            goto oom;
    }

    if ((g_config.ver.major == 2 || (g_config.ver.major == NOT_SET && !tag1))
        && !tag2)
    {
        tag2 = new_id3v2_tag();

        if (tag2)
            tag2->header.version = g_config.ver.minor != NOT_SET
                                   ? g_config.ver.minor : 4;
        else
            goto oom;
    }

    if (tag1)
    {
        unsigned track;

        if (g_config.ver.minor != NOT_SET)
            tag1->version = g_config.ver.minor;
        else if (tag1->version == 0 || tag1->version == 1)
            tag1->version = 3;

        copy_if_not_null(title,     *tag1, g_config);
        copy_if_not_null(artist,    *tag1, g_config);
        copy_if_not_null(album,     *tag1, g_config);
        copy_if_not_null(year,      *tag1, g_config);
        copy_if_not_null(comment,   *tag1, g_config);
        copy_if_not_null(genre_str, *tag1, g_config);

        if (g_config.options & ID321_OPT_SET_GENRE_ID)
            tag1->genre_id = g_config.genre_id;

        if (g_config.track && sscanf(g_config.track, "%u", &track))
            tag1->track = (track <= 0xFF) ? track : 0;

        if (g_config.options & ID321_OPT_SET_SPEED)
            tag1->speed = g_config.speed;
    }

    if (tag2)
    {
        const char *id;
        char       *buf;
        unsigned    i;
        size_t      bufsize;
        char        frame_enc_byte;
        const char *frame_enc_name;
        struct 
        {
            char        alias;
            const char *data;
        }
        map[] =
        {
            { 't', g_config.title },
            { 'a', g_config.artist },
            { 'l', g_config.album },
            { 'y', g_config.year },
            { 'c', g_config.comment },
            { 'n', g_config.track },
            { 'g', g_config.genre_str },
        };

        assert(tag2->header.version == 2 ||
               tag2->header.version == 3 ||
               tag2->header.version == 4);

        frame_enc_byte = g_config.v2_def_encs[tag2->header.version];
        frame_enc_name = get_id3v2_tag_encoding_name(tag2->header.version,
                                                     frame_enc_byte);

        assert(frame_enc_name);

        for_each (i, map)
        {
            if (map[i].data)
            {
                struct id3v2_frame frame = { };

                id = alias_to_frame_id(map[i].alias, tag2->header.version);
                strncpy(frame.id, id, 4);

                ret = iconv_alloc(frame_enc_name, locale_encoding(),
                                  map[i].data, strlen(map[i].data),
                                  &buf, &bufsize);

                if (ret != 0)
                    goto err; /* iconv_alloc itself reports about an error */

                frame.size = bufsize + 1;
                frame.data = malloc(frame.size);

                if (!frame.data)
                {
                    free(buf);
                    goto oom;
                }

                frame.data[0] = frame_enc_byte;
                memcpy(frame.data + 1, buf, frame.size - 1);

                ret = update_id3v2_tag_frame(tag2, &frame);
                if (ret != 0)
                {
                    free(frame.data);
                    goto oom;
                }

                free(buf);
            }
        }
    }

    write_tags(filename, tag1, tag2);

    free(tag1);
    free_id3v2_tag(tag2);
    return 0;

oom:

    print(OS_ERROR, "out of memory");

err:

    free(tag1);
    free_id3v2_tag(tag2);
    return -1;
}
