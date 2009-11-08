#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "params.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v2.h"
#include "output.h"
#include "framelist.h"

void print_id3v1_tag(struct id3v1_tag *tag)
{
    const char *genre_str = get_id3v1_genre_str(tag->genre);

    print(OS_OUT,
            "Title: %s\n"
            "Artist: %s\n"
            "Album: %s\n"
            "Year: %s\n"
            "Comment: %s\n"
            "Genre: (%u)%s",
            tag->title, tag->artist, tag->album, tag->year,
            tag->comment, tag->genre, genre_str ? genre_str : "");

    if (tag->version != 0)
        print(OS_OUT, "Track: %u", tag->track);

    if (tag->version == ID3V1E_MINOR)
    {
        print(OS_OUT,
                "Genre2: %.30s\n"
                "Start time: %.6s\n"
                "End time: %.6s\n"
                "Speed: %u",
                tag->genre2, tag->starttime, tag->endtime, tag->speed);
    }
}

void print_id3v2_tag(struct id3v2_tag *tag)
{
    struct id3v2_frame_list *frame = tag->first_frame;

    for (; frame != NULL; frame = frame->next)
    {
        printf("%.4s: ", frame->frame.id);

        /* TODO: print function for each frame type should be
         *       implemented
         */

        if (frame->frame.id[0] == 'T')
            print_utf8(frame->frame.data);
        else
            printf("[some data] ;)");
        printf("\n");
    }
}

int print_tag(struct id3v2_tag *tag)
{
    char *curpos;
    char *newpos;
    char *fmtstr;
    size_t len;
    struct id3v2_frame *frame;
    const char *frame_id;

    if (g_config.fmtstr != NULL)
    {
        /* let's parse format string */
        fmtstr = strdup(g_config.fmtstr);

        if (fmtstr == NULL)
        {
            print(OS_ERROR, "i need some more memory");
            return -1;
        }

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
                frame_id = alias_to_frame_id(newpos[1], tag->header.version);

                if (frame_id != NULL)
                    newpos += 2;
                else if (newpos + 5 <= fmtstr + len)
                {
                    frame_id = newpos + 1;
                    newpos += 5;
                }

                frame = find_frame(tag->first_frame, frame_id);

                if (frame != NULL)
                    print_utf8(frame->data);
            }
        }

        if (curpos - fmtstr < len)
        {
            printf(curpos);
        }
        printf("\n");

        free(fmtstr);
    }
}

int get_tags()
{
    int               fd;
    int               retval;
    struct id3v2_tag  tag2 = {0};
    struct id3v1_tag  tag1 = {0};
    int               is_tag2_read = 0;
    int               is_tag1_read = 0;

    fd = open(g_config.filename, O_RDONLY);

    if (fd == -1)
    {
        print(OS_ERROR, "unable to open source file %s", g_config.filename);
        return;
    }

    if (g_config.major_ver == 2 || g_config.major_ver == NOT_SET)
    {
        /* read id3v2 tag if available */
        if (read_id3v2_header(fd, &tag2.header) != -1)
        {
            if (g_config.minor_ver == tag2.header.version
                || g_config.minor_ver == NOT_SET)
            {
                dump_id3_header(&tag2.header);

                if (tag2.header.flags | ID3V2_FLAG_EXT_HEADER)
                    retval = read_id3v2_ext_header(fd, &tag2);

                retval = read_id3v2_frames(fd, &tag2);

                is_tag2_read = 1;

                if (g_config.fmtstr == NULL)
                {
                    print_id3v2_tag(&tag2);
                }
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

    if (g_config.major_ver == 1 || g_config.major_ver == NOT_SET)
    {
        retval = -1;

        if (g_config.minor_ver == ID3V1E_MINOR || g_config.minor_ver == NOT_SET)
        {
            /* read id3v1 extended tag if available */
            lseek(fd, -(ID3V1E_TAG_SIZE), SEEK_END);
            retval = read_id3v1_ext_tag(fd, &tag1);
        }

        if (retval == -1 && (g_config.minor_ver == 2
                             || g_config.minor_ver == NOT_SET))
        {
            /* read id3v1.2 tag if available */
            lseek(fd, -ID3V12_TAG_SIZE, SEEK_END);
            retval = read_id3v1_tag(fd, &tag1);
        }

        if (retval == -1 && (g_config.minor_ver == 1
                             || g_config.minor_ver == 3
                             || g_config.minor_ver == 0
                             || g_config.minor_ver == NOT_SET))
        {
            /* read id3v1 tag if available */
            lseek(fd, -ID3V1_TAG_SIZE, SEEK_END);
            retval = read_id3v1_tag(fd, &tag1);
        }

        if (retval != -1)
        {
            is_tag1_read = 1;
            if (g_config.fmtstr == NULL)
            {
                print_id3v1_tag(&tag1);
            }
        }
    }

    if (g_config.fmtstr != NULL && is_tag2_read)
    {
        print_tag(&tag2);
    }

    close(fd);
}
