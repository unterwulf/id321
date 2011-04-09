#include <ctype.h>        /* isdigit() */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alias.h"
#include "common.h"
#include "params.h"       /* g_config, NOT_SET */
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v1e_speed.h"
#include "id3v2.h"
#include "output.h"
#include "printfmt.h"
#include "frames.h"       /* get_frame_data() */
#include "frm_trck.h"     /* get_id3v2_tag_trackno() */
#include "framelist.h"
#include "u32_char.h"

extern void printfmt(const struct print_fmt *pf, char *str);
extern void u32_printfmt(const struct print_fmt *pf, u32_char *str);

static void print_id3v1_data(char alias, const struct id3v1_tag *tag,
                             struct print_fmt *pfmt)
{
    size_t size;
    const struct alias *al = get_alias(alias);
    const void *buf;

    if (!al)
        return;

    buf = alias_to_v1_data(al, tag, &size);

    if (size == 1)
    {
        char int_str[4];

        snprintf(int_str, sizeof(int_str), "%u", *(const uint8_t *)buf);
        pfmt->flags |= FL_INT;
        printfmt(pfmt, int_str);
    }
    else
    {
        u32_char *u32_str;
        int ret = iconv_alloc(U32_CHAR_CODESET, g_config.enc_v1,
                              buf, strlen(buf),
                              (void *)&u32_str, NULL);

        if (ret != 0)
            return;

        u32_printfmt(pfmt, u32_str);
        free(u32_str);
    }
}

static void print_id3v1_tag_field(const char *name, const char *value)
{
    u32_char *u32_str = NULL;
    int ret = iconv_alloc(U32_CHAR_CODESET, g_config.enc_v1,
                          value, strlen(value),
                          (void *)&u32_str, NULL);

    printf("%s: ", name);

    if (ret == 0)
    {
        u32_printfmt(NULL, u32_str);
        free(u32_str);
        putchar('\n');
    }
    else
        puts("[ENOMEM]");
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
        int len = get_frame_data(tag, frame, NULL, 0);

        printf("%.*s: ", ID3V2_FRAME_ID_MAX_SIZE, frame->id);

        if (len > 0)
        {
            u32_char *u32_str = malloc(sizeof(u32_str)*(len+1));
            if (u32_str)
            {
                get_frame_data(tag, frame, u32_str, len);
                u32_str[len] = U32_CHAR('\0');
                u32_printfmt(NULL, u32_str);
                putchar('\n');
                free(u32_str);
            }
            else
                puts("[ENOMEM]");
        }
        else if (len == 0)
            putchar('\n');
        else if (len == -ENOSYS)
            puts("[parser for this frame is not implemented yet]");
        else if (len == -EINVAL)
            puts("[unknown frame]");
        else if (len == -EILSEQ)
            puts("[malformed frame]");
    }
}

static void print_tag(const struct id3v1_tag *tag1,
                      const struct id3v2_tag *tag2)
{
    const char *pos;
    const char *lastspec = NULL;
    size_t len;
    struct id3v2_frame *frame;
    enum {
        st_escape, st_normal, st_flags, st_width, st_prec, st_spec
    } state = st_normal;
    struct print_fmt pfmt = { };

    if (!g_config.fmtstr)
        return;

    /* let's parse format string */
    len = strlen(g_config.fmtstr);

    for (pos = g_config.fmtstr, frame = NULL;
         pos < g_config.fmtstr + len;
         pos++)
    {
        switch (state)
        {
            case st_normal:
                switch (*pos)
                {
                    case '\\': state = st_escape; break;
                    case '%':  memset(&pfmt, '\0', sizeof(pfmt));
                               lastspec = pos;
                               state = st_flags; break;
                    default:   putchar(*pos);
                }
                break;

            case st_escape:
                switch (*pos)
                {
                    case 'n': putchar('\n'); break;
                    case 'r': putchar('\r'); break;
                    case 't': putchar('\t'); break;
                    case '\\': putchar('\\'); break;
                    default:  putchar('\\'); pos--; /* process it again */
                }
                state = st_normal;
                break;

            case st_flags:
                switch (*pos)
                {
                    case '0': pfmt.flags |= FL_ZERO; break;
                    case '-': pfmt.flags |= FL_LEFT; break;
                    default:  pos--; state = st_width;
                }
                break;

            case st_width:
                if (isdigit(*pos))
                    pfmt.width = 10*pfmt.width + (*pos - '0');
                else if (*pos == '.')
                {
                    state = st_prec;
                    pfmt.flags |= FL_PREC;
                }
                else
                {
                    state = st_spec;
                    pos--; /* process it again */
                }
                break;

            case st_prec:
                if (isdigit(*pos))
                    pfmt.precision = 10*pfmt.precision + (*pos - '0');
                else
                {
                    state = st_spec;
                    pos--; /* process it again */
                }
                break;

            case st_spec:
            {
                const struct alias *al;

                if (*pos == 'n')
                {
                    char trackno_str[] = "###";
                    int trackno = -1;

                    if (tag2)
                        trackno = get_id3v2_tag_trackno(tag2);

                    if (trackno < 0 && tag1 && tag1->track != 0)
                        trackno = tag1->track;

                    if (trackno >= 0)
                    {
                        snprintf(trackno_str, sizeof(trackno_str),
                                 "%u", trackno);
                        pfmt.flags |= FL_INT;
                    }

                    printfmt(&pfmt, trackno_str);
                }
                else if ((al = get_alias(*pos)))
                {
                    if (tag2)
                    {
                        const char *frame_id =
                            alias_to_frame_id(al, tag2->header.version);
                        frame = peek_frame(&tag2->frame_head, frame_id);
                    }

                    if (!frame && tag1)
                        print_id3v1_data(*pos, tag1, &pfmt);
                }
                else if (tag2 && (pos + 2 < g_config.fmtstr + len)
                         && is_valid_frame_id_str(pos, 3))
                {
                    char   local_frame_id[ID3V2_FRAME_ID_MAX_SIZE] = "\0";
                    size_t frame_id_len = ((pos + 3 < g_config.fmtstr + len)
                                           && is_valid_frame_id_str(pos + 3, 1))
                                          ? 4 : 3;

                    strncpy(local_frame_id, pos, frame_id_len);
                    pos += frame_id_len - 1;
                    frame = peek_frame(&tag2->frame_head, local_frame_id);
                }
                else if (*pos == '%' && pos - lastspec == 1)
                    putchar('%');
                else /* invalid format spec, so just print it as is */
                    printf("%.*s", pos - lastspec + 1, lastspec);

                if (frame)
                {
                    int len = get_frame_data(tag2, frame, NULL, 0);

                    if (len > 0)
                    {
                        u32_char *u32_str = malloc(sizeof(u32_char)*(len+1));

                        if (u32_str)
                        {
                            get_frame_data(tag2, frame, u32_str, len);
                            u32_str[len] = U32_CHAR('\0');
                            u32_printfmt(&pfmt, u32_str);
                            free(u32_str);
                        }
                        else
                            printfmt(&pfmt, "ENOMEM");
                    }

                    frame = NULL;
                }
                state = st_normal;
                break;
            }
        }
    }

    /* flush incomplete state */

    switch (state)
    {
        case st_escape: puts("\\"); break;
        case st_normal: putchar('\n'); break;
        default:        puts(lastspec); break;
    }
}

int print_tags(const char *filename)
{
    struct id3v2_tag *tag2 = NULL;
    struct id3v1_tag *tag1 = NULL;
    int               ret;

    ret = get_tags(filename, g_config.ver, &tag1, &tag2);

    if (ret != 0)
        return NOMEM_OR_FAULT(ret);

    if (!tag1 && !tag2)
    {
        print(OS_WARN, "%s: file has no ID3 tags%s", filename,
              g_config.ver.major != NOT_SET ? " of specified version" : "");
        return 0;
    }

    if (g_config.fmtstr)
        print_tag(tag1, tag2);
    else if (g_config.frame_id)
    {
        struct id3v2_frame *frame = NULL;

        if (tag2)
        {
            frame = peek_frame(&tag2->frame_head, g_config.frame_id);

            if (!(g_config.options & ID321_OPT_ALL_FRAMES))
            {
                int i;
                for (i = g_config.frame_no; frame && i > 0; i--)
                {
                    frame = peek_next_frame(&tag2->frame_head,
                            g_config.frame_id, frame);
                }
            }
        }

        if (frame)
        {
            if (g_config.options & ID321_OPT_ALL_FRAMES)
            {
                while (frame)
                {
                    fwrite(frame->data, frame->size, 1, stdout);
                    frame = peek_next_frame(&tag2->frame_head,
                                            g_config.frame_id, frame);
                }
            }
            else
                fwrite(frame->data, frame->size, 1, stdout);
        }
        else
            print(OS_ERROR, "%s: file has no matching ID3v2 frame '%s[%d]'",
                  filename, g_config.frame_id, g_config.frame_no);
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
