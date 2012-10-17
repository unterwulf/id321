#include <assert.h>
#include <errno.h>
#include <stdio.h>  /* sscanf() */
#include <stdlib.h>
#include <string.h>
#include "alias.h"
#include "common.h"
#include "framelist.h"
#include "frm_comm.h"
#include "frm_tcon.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v2.h"
#include "output.h"
#include "params.h"
#include "textframe.h"
#include "u32_char.h"
#include "xalloc.h"

static int modify_v1_tag(struct id3v1_tag *tag)
{
    unsigned track;
    const char fields[] = "talycG";
    const char *field_alias;

    if (g_config.ver.minor != NOT_SET)
        tag->version = g_config.ver.minor;
    else if (tag->version == 0 || tag->version == 1)
        tag->version = 3;

    for (field_alias = fields; *field_alias != '\0'; field_alias++)
    {
        const char *new_value = get_config_data_by_alias(*field_alias);

        if (new_value)
        {
            size_t field_size;
            char *field = get_v1_data_by_alias(*field_alias, tag, &field_size);

            memset(field, '\0', field_size);
            iconvordie(g_config.enc_v1, locale_encoding(),
                       new_value, strlen(new_value), field, field_size - 1);
        }
    }

    if (g_config.options & ID321_OPT_SET_GENRE_ID)
        tag->genre_id = g_config.genre_id;

    if (g_config.track && sscanf(g_config.track, "%u", &track))
        tag->track = (track <= 0xFF) ? track : 0;

    if (g_config.options & ID321_OPT_SET_SPEED)
        tag->speed = g_config.speed;

    return 0;
}

static void modify_arbitrary_frame(struct id3v2_tag *tag,
                                   struct id3v2_frame **frame)
{
    if (g_config.options & ID321_OPT_RM_FRAME)
    {
        struct id3v2_frame *prev = (*frame)->prev;

        /* delete frame */
        unlink_frame(*frame);
        free_frame(*frame);

        *frame = prev;
        /* we need to rewind the frame pointer to the previous frame
         * in order to make the caller able to continue iterating */
    }
    else if (g_config.options & ID321_OPT_BIN_FRAME)
    {
        free((*frame)->data);
        (*frame)->data = xmalloc(g_config.frame_size);
        memcpy((*frame)->data, g_config.frame_data, g_config.frame_size);
        (*frame)->size = g_config.frame_size;
    }
    else
    {
        const char *frame_enc_name;
        char frame_enc_byte;
        char *buf;
        size_t bufsize;

        if (g_config.frame_enc)
        {
            frame_enc_byte = get_id3v2_tag_encoding_byte(
                    tag->header.version, g_config.frame_enc);

            if (frame_enc_byte == ID3V2_UNSUPPORTED_ENCODING)
            {
                print(OS_WARN,
                        "ID3v2.%d tag has no support of encoding '%s'",
                        tag->header.version, g_config.frame_enc);
                return;
            }

            frame_enc_name = g_config.frame_enc;
        }
        else
        {
            frame_enc_byte = g_config.v2_def_encs[tag->header.version];
            frame_enc_name = get_id3v2_tag_encoding_name(
                    tag->header.version, frame_enc_byte);
        }

        iconv_alloc(frame_enc_name, locale_encoding(),
                    g_config.frame_data, g_config.frame_size,
                    &buf, &bufsize);

        update_id3v2_tag_text_frame_payload(
                *frame, frame_enc_byte, buf, bufsize);

        free(buf);
    }

    return;
}

static int modify_v2_tag(const char *filename, struct id3v2_tag *tag)
{
    const char  frames[] = "talyn";
    const char *frame_alias;

    assert(tag->header.version == 2 ||
           tag->header.version == 3 ||
           tag->header.version == 4);

    for (frame_alias = frames; *frame_alias != '\0'; frame_alias++)
    {
        const char *data = get_config_data_by_alias(*frame_alias);

        if (data)
        {
            size_t data_sz = strlen(data);
            const char *frame_id = get_frame_id_by_alias(*frame_alias,
                                                         tag->header.version);

            if (data_sz == 0)
            {
                /* delete frame if it exists */
                struct id3v2_frame *frame =
                    peek_frame(&tag->frame_head, frame_id);

                if (frame)
                {
                    unlink_frame(frame);
                    free_frame(frame);
                }
            }
            else
            {
                update_id3v2_tag_text_frame(
                        tag, frame_id, locale_encoding(),
                        data, data_sz);
            }
        }
    }

    /* modify comment */
    if (g_config.comment)
    {
        int ret;
        u32_char *udesc = locale_to_u32_alloc(g_config.comment_desc);
        u32_char *utext = locale_to_u32_alloc(g_config.comment);

        ret = update_id3v2_frm_comm(tag, g_config.comment_lang, udesc, utext);

        if (ret == -ENOENT)
        {
            print(OS_WARN, "%s: no matching comment frame", filename);
            ret = 0;
        }

        free(utext);
        free(udesc); /* may be null pointer, but it is ok to free(NULL) */

        if (ret != 0)
            return ret;
    }

    /* modify genre */
    if (g_config.options & ID321_OPT_RM_GENRE_FRAME)
    {
        const char *frame_id = get_frame_id_by_alias('g', tag->header.version);
        struct id3v2_frame *frame = peek_frame(&tag->frame_head, frame_id);

        /* delete frame if it exists */
        if (frame)
        {
            unlink_frame(frame);
            free_frame(frame);
        }
    }
    else
    {
        u32_char *genre_ustr = NULL;
        uint8_t genre_id = (g_config.options & ID321_OPT_SET_GENRE_ID)
                           ? g_config.genre_id : ID3V1_UNKNOWN_GENRE;

        if (!IS_EMPTY_STR(g_config.genre_str))
        {
            iconv_alloc(U32_CHAR_CODESET, locale_encoding(),
                        g_config.genre_str, strlen(g_config.genre_str),
                        (void *)&genre_ustr, NULL);
        }

        set_id3v2_tag_genre(tag, genre_id, genre_ustr);
        free(genre_ustr);
    }

    /* modify arbitrary frame */
    if (g_config.frame_id)
    {
        struct id3v2_frame *frame = NULL;

        if ((g_config.options & ID321_OPT_CREATE_FRAME_IF_NOT_EXISTS
             && !(g_config.options & ID321_OPT_RM_FRAME))
            || g_config.options & ID321_OPT_CREATE_FRAME)
        {
            if (g_config.options & ID321_OPT_CREATE_FRAME_IF_NOT_EXISTS)
                frame = peek_frame(&tag->frame_head, g_config.frame_id);

            if (!frame)
            {
                frame = xcalloc(1, sizeof(struct id3v2_frame));
                strncpy(frame->id, g_config.frame_id, ID3V2_FRAME_ID_MAX_SIZE);
                append_frame(&tag->frame_head, frame);
            }

            modify_arbitrary_frame(tag, &frame);
        }
        else
        {
            int frame_no;

            for (frame_no = 0, frame = &tag->frame_head;
                 (frame_no <= g_config.frame_no
                  || (g_config.options & ID321_OPT_ALL_FRAMES)) &&
                 (frame = peek_next_frame(&tag->frame_head, g_config.frame_id,
                                          frame)); frame_no++)
            {
                if ((g_config.options & ID321_OPT_ALL_FRAMES)
                    || frame_no == g_config.frame_no)
                    modify_arbitrary_frame(tag, &frame);
                else
                    ; /* iterate through the matching frames until
                       * frame_no is reached */
            }

            if (!(g_config.options & ID321_OPT_ALL_FRAMES)
                && frame_no != g_config.frame_no + 1)
            {
                print(OS_WARN, "no matching frame '%s' found",
                               g_config.frame_id);
            }
        }
    }

    return 0;
}

int modify_tags(const char *filename)
{
    struct id3v1_tag *tag1 = NULL;
    struct id3v2_tag *tag2 = NULL;
    struct version ver = { g_config.ver.major, NOT_SET };
    int ret;

    ret = get_tags(filename, ver, &tag1, &tag2);

    if (ret != 0)
        return -EFAULT;

    if (g_config.ver.major == 1 && !tag1)
    {
        tag1 = xcalloc(1, sizeof(struct id3v1_tag));
        tag1->version =
            (g_config.ver.minor != NOT_SET) ? g_config.ver.minor : 3;
        tag1->genre_id = ID3V1_UNKNOWN_GENRE;
    }

    if (tag2 && g_config.ver.major == 2 && g_config.ver.minor != NOT_SET
        && g_config.ver.minor != tag2->header.version)
    {
        print(OS_ERROR, "%s: present ID3v2 tag has a different "
                        "minor version, conversion is not implemented "
                        "yet, skipping this file", filename);

        /* TODO: convert between ID3v2 minor versions */

        ret = -ENOSYS;
    }
    else if ((g_config.ver.major == 2
              || (g_config.ver.major == NOT_SET && !tag1)) && !tag2)
    {
        tag2 = new_id3v2_tag();

        if (g_config.ver.minor != NOT_SET)
            tag2->header.version = g_config.ver.minor;
    }

    if (tag1)
        ret = modify_v1_tag(tag1);

    if (ret == 0 && tag2)
        ret = modify_v2_tag(filename, tag2);

    if (ret == 0)
        ret = write_tags(filename, tag1, tag2);

    free(tag1);
    free_id3v2_tag(tag2);
    return SUCC_OR_FAULT(ret);
}
