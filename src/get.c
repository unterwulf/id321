#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "alias.h"
#include "common.h"
#include "params.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v2.h"
#include "output.h"
#include "framelist.h"

static void print_id3v1_data(const char alias, const struct id3v1_tag *tag)
{
    size_t size;
    const void *buf = alias_to_v1_data(alias, tag, &size);

    if (!buf)
        return;

    if (size == 1)
        printf("%u", *(const uint8_t *)buf);
    else
    {
        lprint(g_config.options & ID3T_FORCE_ENCODING
                ? g_config.encoding
                : "ISO8859-1",
                (const char *)buf);
    }
}

static void print_id3v1_tag_field(const char *name, const char *value)
{
    printf("%s: ", name);
    lprint(g_config.options & ID3T_FORCE_ENCODING
            ? g_config.encoding
            : "ISO8859-1",
            value);
    printf("\n");
}

static void print_id3v1_tag(const struct id3v1_tag *tag)
{
    const char *genre_str = get_id3v1_genre_str(tag->genre);

    print_id3v1_tag_field("Title", tag->title);
    print_id3v1_tag_field("Artist", tag->artist);
    print_id3v1_tag_field("Album", tag->album);
    print_id3v1_tag_field("Year", tag->year);
    print_id3v1_tag_field("Comment", tag->comment);
    printf("Genre: (%u) %s\n", tag->genre, genre_str ? genre_str : "");

    if (tag->version != 0)
        printf("Track no.: %u\n", tag->track);

    if (tag->version == ID3V1E_MINOR)
    {
        print_id3v1_tag_field("Genre2", tag->genre2);
        print_id3v1_tag_field("Start time", tag->starttime);
        print_id3v1_tag_field("End time", tag->endtime);
        printf("Speed: %u\n", tag->speed);
    }
}

static void print_id3v2_tag(const struct id3v2_tag *tag)
{
    struct id3v2_frame_list *frame = tag->first_frame;

    for (; frame != NULL; frame = frame->next)
    {
        printf("%.4s: ", frame->frame.id);

        /* TODO: print function for each frame type should be
         *       implemented
         */

        if (frame->frame.id[0] == 'T')
            lprint("UTF-8", frame->frame.data);
        else
            printf("[some data] ;)");

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
    fmtstr = xstrdup(g_config.fmtstr);
    len = strlen(fmtstr);

    for (curpos = fmtstr;
            (newpos = strchr(curpos, '%')) != NULL;
            curpos = newpos)
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
                    frame = find_frame(tag2->first_frame, frame_id);
                }

                if (!frame && tag1)
                    print_id3v1_data(newpos[1], tag1);

                newpos += 2;
            }
            else if (tag2 && (newpos + 5 <= fmtstr + len))
            {
                frame_id = newpos + 1;
                newpos += 5;
                frame = find_frame(tag2->first_frame, frame_id);
            }

            if (frame != NULL)
                lprint("UTF-8", frame->data);
        }
    }

    if (curpos - fmtstr < len)
        printf(curpos);

    printf("\n");

    free(fmtstr);
}

int get_tags(const char *filename)
{
    int               fd;
    int               retval;
    struct id3v2_tag  tag2 = {0};
    struct id3v1_tag  tag1 = {0};
    int               is_tag2_read = 0;
    int               is_tag1_read = 0;

    fd = open(filename, O_RDONLY);

    if (fd == -1)
    {
        print(OS_ERROR, "unable to open source file `%s'", filename);
        return;
    }

    if (g_config.ver.major == 2 || g_config.ver.major == NOT_SET)
    {
        /* read id3v2 tag if available */
        if (read_id3v2_header(fd, &tag2.header) != -1)
        {
            if (g_config.ver.minor == tag2.header.version
                || g_config.ver.minor == NOT_SET)
            {
                dump_id3_header(&tag2.header);

                if (tag2.header.flags | ID3V2_FLAG_EXT_HEADER)
                    retval = read_id3v2_ext_header(fd, &tag2);

                retval = read_id3v2_frames(fd, &tag2);
                is_tag2_read = 1;

                if (!g_config.fmtstr)
                    print_id3v2_tag(&tag2);
            }
            else
            {
                print(OS_DEBUG, "file has id3v2.%d tag, ignore it",
                       tag2.header.version);
            }
        }
        else
        {
            /* check presence of an appended id3v2 tag */
        }
    }

    if (g_config.ver.major == 1 || g_config.ver.major == NOT_SET)
    {
        retval = -1;

        if (g_config.ver.minor == ID3V1E_MINOR || g_config.ver.minor == NOT_SET)
        {
            /* read id3v1 extended tag if available */
            lseek(fd, -(ID3V1E_TAG_SIZE), SEEK_END);
            retval = read_id3v1_ext_tag(fd, &tag1);
        }

        if (retval == -1 && (g_config.ver.minor == 2
                             || g_config.ver.minor == NOT_SET))
        {
            /* read id3v1.2 tag if available */
            lseek(fd, -ID3V12_TAG_SIZE, SEEK_END);
            retval = read_id3v1_tag(fd, &tag1);
        }

        if (retval == -1 && (g_config.ver.minor == 1
                             || g_config.ver.minor == 3
                             || g_config.ver.minor == 0
                             || g_config.ver.minor == NOT_SET))
        {
            /* read id3v1 tag if available */
            lseek(fd, -ID3V1_TAG_SIZE, SEEK_END);
            retval = read_id3v1_tag(fd, &tag1);
        }

        if (retval != -1)
        {
            is_tag1_read = 1;
            if (!g_config.fmtstr)
                print_id3v1_tag(&tag1);
        }
    }

    if (g_config.fmtstr)
        print_tag(is_tag1_read ? &tag1 : NULL, is_tag2_read ? &tag2 : NULL);

    close(fd);
}
