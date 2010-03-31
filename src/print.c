#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "alias.h"
#include "common.h"
#include "params.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v1e_speed.h"
#include "id3v2.h"
#include "output.h"
#include "frames.h"       /* print_frame_data() */
#include "framelist.h"

static void print_id3v1_data(char alias, const struct id3v1_tag *tag)
{
    size_t size;
    const void *buf = alias_to_v1_data(alias, tag, &size);

    if (!buf)
        return;

    if (size == 1)
        printf("%u", *(const uint8_t *)buf);
    else
        lprint(g_config.enc_v1, (const char *)buf);
}

static void print_id3v1_tag_field(const char *name, const char *value)
{
    printf("%s: ", name);
    lprint(g_config.enc_v1, value);
    printf("\n");
}

static void print_id3v1_tag(const struct id3v1_tag *tag)
{
    const char *genre_str = get_id3v1_genre_str(tag->genre_id);
    const char *speed_str = get_id3v1e_speed_str(tag->speed);

    print_id3v1_tag_field("Title", tag->title);
    print_id3v1_tag_field("Artist", tag->artist);
    print_id3v1_tag_field("Album", tag->album);
    print_id3v1_tag_field("Year", tag->year);
    print_id3v1_tag_field("Comment", tag->comment);
    printf("Genre: (%u) %s\n", tag->genre_id, genre_str ? genre_str : "");

    if (tag->version != 0 && tag->track != '\0')
        printf("Track no.: %u\n", tag->track);

    if (tag->version == 2 || tag->version == ID3V1E_MINOR)
        print_id3v1_tag_field("Genre2", tag->genre_str);

    if (tag->version == ID3V1E_MINOR)
    {
        print_id3v1_tag_field("Start time", tag->starttime);
        print_id3v1_tag_field("End time", tag->endtime);
        printf("Speed: (%u) %s\n", tag->speed, speed_str ? speed_str : "");
    }
}

static void print_id3v2_tag(const struct id3v2_tag *tag)
{
    const struct id3v2_frame *frame;

    for (frame = tag->frame_head.next;
         frame != &tag->frame_head;
         frame = frame->next)
    {
        printf("%.*s: ", ID3V2_FRAME_ID_MAX_SIZE, frame->id);
        print_frame_data(tag, frame);
        printf("\n");
    }
}

static int is_valid_frame_id_str(const char *str, size_t len)
{
    unsigned i;
    int res = 1;

    for (i = 0; i < len; i++)
        res &= isupper(str[i]) || isdigit(str[i]);

    return res;
}

static void print_tag(const struct id3v1_tag *tag1,
                      const struct id3v2_tag *tag2)
{
    const char *curpos;
    const char *newpos;
    size_t len;
    struct id3v2_frame *frame;

    if (!g_config.fmtstr)
        return;

    /* let's parse format string */
    len = strlen(g_config.fmtstr);

    for (curpos = g_config.fmtstr, frame = NULL;
         (newpos = strchr(curpos, '%')) && (newpos + 1 < g_config.fmtstr + len);
         curpos = newpos)
    {
        if (newpos > curpos)
            fwrite(curpos, newpos - curpos, 1, stdout);

        if (is_valid_alias(newpos[1]))
        {
            if (tag2)
            {
                const char *frame_id; 
                frame_id = alias_to_frame_id(newpos[1], tag2->header.version);
                frame = peek_frame(&tag2->frame_head, frame_id);
            }

            if (!frame && tag1)
                print_id3v1_data(newpos[1], tag1);

            newpos += 2;
        }
        else if (tag2 && (newpos + 3 < g_config.fmtstr + len)
                 && is_valid_frame_id_str(newpos + 1, 3))
        {
            char   local_frame_id[ID3V2_FRAME_ID_MAX_SIZE] = "\0";
            size_t frame_id_len = ((newpos + 4 < g_config.fmtstr + len)
                                   && is_valid_frame_id_str(newpos + 4, 1))
                                  ? 4 : 3;

            strncpy(local_frame_id, newpos + 1, frame_id_len);
            newpos += frame_id_len + 1;
            frame = peek_frame(&tag2->frame_head, local_frame_id);
        }
        else /* invalid format sequence, so just print it as is */
        {
            fwrite(newpos, 2, 1, stdout);
            newpos += 2;
        }

        if (frame != NULL)
        {
            print_frame_data(tag2, frame);
            frame = NULL;
        }
    }

    printf("%s\n", curpos);
}

int print_tags(const char *filename)
{
    struct id3v2_tag *tag2 = NULL;
    struct id3v1_tag *tag1 = NULL;
    int               ret;

    ret = get_tags(filename, g_config.ver, &tag1, &tag2);

    if (ret != 0)
        return -1;

    if (!tag1 && !tag2)
    {
        print(OS_WARN, "no ID3 tags in `%s'", filename);
        return 0;
    }

    if (g_config.fmtstr)
        print_tag(tag1, tag2);
    else if (g_config.frame)
    {
        struct id3v2_frame *frame = NULL;

        if (tag2)
            frame = peek_frame(&tag2->frame_head, g_config.frame);

        if (frame)
            fwrite(frame->data, frame->size, 1, stdout);
        else
            print(OS_ERROR, "there is no ID3v2 `%s' frame", g_config.frame);
    }
    else
    {
        if (tag2)
            print_id3v2_tag(tag2);

        if (tag1)
            print_id3v1_tag(tag1);
    }

    free(tag1);
    free_id3v2_tag(tag2);

    return 0;
}
