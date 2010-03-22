#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    const struct id3v2_frame_list *frame;

    for (frame = tag->frame_head.next;
         frame != &tag->frame_head;
         frame = frame->next)
    {
        printf("%.4s: ", frame->frame.id);
        print_frame_data(tag, &frame->frame);
        printf("\n");
    }
}

static void print_tag(const struct id3v1_tag *tag1,
                      const struct id3v2_tag *tag2)
{
    char *curpos;
    char *newpos;
    char *fmtstr;
    size_t len;
    struct id3v2_frame *frame = NULL;
    const char *frame_id;

    if (!g_config.fmtstr)
        return;

    /* let's parse format string */
    fmtstr = strdup(g_config.fmtstr);

    if (!fmtstr)
    {
        print(OS_ERROR, "out of memory");
        return;
    }

    len = strlen(fmtstr);

    for (curpos = fmtstr; (newpos = strchr(curpos, '%')); curpos = newpos)
    {
        if (newpos > curpos)
        {
            *newpos = '\0';
            printf("%s", curpos);
        }

        if (newpos + 1 <= fmtstr + len)
        {
            frame = NULL;

            if (is_valid_alias(newpos[1]))
            {
                if (tag2)
                {
                    frame_id = alias_to_frame_id(newpos[1], tag2->header.version);
                    frame = find_frame(&tag2->frame_head, frame_id);
                }

                if (!frame && tag1)
                    print_id3v1_data(newpos[1], tag1);

                newpos += 2;
            }
            else if (tag2 && (newpos + 5 <= fmtstr + len))
            {
                frame_id = newpos + 1;
                newpos += 5;
                frame = find_frame(&tag2->frame_head, frame_id);
            }

            if (frame != NULL)
                print_frame_data(tag2, frame);
        }
    }

    if (curpos < fmtstr + len)
        printf(curpos);

    printf("\n");

    free(fmtstr);
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
            frame = find_frame(&tag2->frame_head, g_config.frame);

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
