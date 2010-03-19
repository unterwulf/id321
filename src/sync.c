#include <stdlib.h>
#include <stdio.h>        /* sscanf() */
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include "common.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v2.h"
#include "params.h"
#include "output.h"
#include "alias.h"
#include "frames.h"       /* set_id3v2_tag_genre_by_id() */
#include "framelist.h"

static int sync_v1_with_v2(struct id3v1_tag *tag1, const struct id3v2_tag *tag2)
{
    print(OS_ERROR, "sorry, this has not been implemented yet");
    exit(EXIT_FAILURE);
}

/*
 * Function:     get_v1_timestamp
 *
 * Description:  parses the string with the following timestamp format: mmm:ss 
 *
 * Return value: time in seconds or -1 if passed string is not a valid
 *               timestamp string
 */
static int32_t get_v1_timestamp(const char *timestamp)
{
    unsigned minutes;
    unsigned seconds;
    int      ret = sscanf(timestamp, "%u:%u", &minutes, &seconds);

    /* some sanity checks */
    if (ret != 2 || seconds > 59 || minutes > 999)
        return -1;

    return minutes*60 + seconds;
}

static int sync_v2_with_v1(struct id3v2_tag *tag2, const struct id3v1_tag *tag1)
{
    const char *id;
    char       *buf;
    unsigned    i;
    int         ret;
    size_t      bufsize;
    char        trackno[4];
    int32_t     time;
    char        frame_enc_byte;
    const char *frame_enc_name;
    struct 
    {
        char        alias;
        const char *data;
    }
    map[] =
    {
        { 't', tag1->title },
        { 'a', tag1->artist },
        { 'l', tag1->album },
        { 'y', tag1->year },
        { 'n', tag1->track ? trackno : NULL },
        { 'c', tag1->comment }
    };

    if (tag1->track)
        snprintf(trackno, sizeof(trackno), "%u", tag1->track);

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

            ret = iconv_alloc(frame_enc_name, g_config.enc_v1,
                              map[i].data, strlen(map[i].data),
                              &buf, &bufsize);

            if (ret != 0)
                return -1;

            frame.size = bufsize + 1;
            frame.data = malloc(frame.size);

            if (!frame.data)
            {
                free(buf);
                print(OS_ERROR, "out of memory");
                return -1;
            }

            frame.data[0] = frame_enc_byte;
            memcpy(frame.data + 1, buf, frame.size - 1);

            ret = update_id3v2_tag_frame(tag2, &frame);
            if (ret != 0)
                free(frame.data);

            free(buf);
        }
    }

    set_id3v2_tag_genre_by_id(tag2, tag1->genre_id);

    time = get_v1_timestamp(tag1->starttime);
//    if (time != -1)
//        update_id3v2_tag_event(tag2, ETCO_END_OF_INITIAL_SILENCE,
//                               time*1000);

    time = get_v1_timestamp(tag1->endtime);
//    if (time != -1)
//        update_id3v2_tag_event(tag2, ETCO_AUDIO_END, time*1000);

    return 0;
}

int sync_tags(const char *filename)
{
    struct id3v1_tag *tag1 = NULL;
    struct id3v2_tag *tag2 = NULL;
    struct version    ver = { NOT_SET, NOT_SET };
    int               ret;

    ret = get_tags(filename, ver, &tag1, &tag2);

    if (ret != 0)
        return -1;

    if (g_config.ver.major == 1)
    {
        if (!tag2)
        {
            print(OS_ERROR, "no ID3v2 tag in `%s' to synchronise with",
                            filename);
            ret = -1;
        }
        else
        {
            if (!tag1)
                tag1 = malloc(sizeof(struct id3v1_tag));

            if (!tag1)
                goto oom;

            tag1->version = g_config.ver.minor != NOT_SET
                ? g_config.ver.minor : 3;

            ret = sync_v1_with_v2(tag1, tag2);

            if (ret == 0)
                write_tags(filename, tag1, NULL);
        }
    }
    else if (g_config.ver.major == 2)
    {
        if (!tag1)
        {
            print(OS_ERROR, "no ID3v1 tag in `%s' to synchronise with",
                            filename);
            ret = -1;
        }
        else
        {
            if (!tag2)
                tag2 = new_id3v2_tag();

            if (!tag2)
                goto oom;

            tag2->header.version = g_config.ver.minor != NOT_SET
                ? g_config.ver.minor : 4;

            /* TODO: convert between ID3v2 minor versions */

            ret = sync_v2_with_v1(tag2, tag1);

            if (ret == 0)
                write_tags(filename, NULL, tag2);
        }
    }

    free(tag1);
    free_id3v2_tag(tag2);
    return ret;

oom:

    free(tag1);
    free_id3v2_tag(tag2);
    print(OS_ERROR, "out of memory");
    return -1;
}
