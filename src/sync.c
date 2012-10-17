#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>        /* sscanf() */
#include <string.h>
#include "alias.h"
#include "common.h"
#include "framelist.h"
#include "frm_comm.h"
#include "frm_tcon.h"     /* get/set_id3v2_tag_genre() */
#include "frm_trck.h"     /* get_id3v2_tag_trackno() */
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v2.h"
#include "output.h"
#include "params.h"
#include "textframe.h"
#include "u32_char.h"
#include "xalloc.h"

static int sync_v1_with_v2(struct id3v1_tag *tag1, const struct id3v2_tag *tag2)
{
    const char fields[] = "taly";
    const char *field_alias;
    const char *frame_id;
    struct id3v2_frame *frame;

    /* sync pure text fields */
    for (field_alias = fields; *field_alias != '\0'; field_alias++)
    {
        frame_id = get_frame_id_by_alias(*field_alias, tag2->header.version);
        frame = peek_frame(&tag2->frame_head, frame_id);

        if (frame && frame->size > 1)
        {
            const char *frame_enc_name =
                get_id3v2_tag_encoding_name(tag2->header.version,
                                            frame->data[0]);
            if (frame_enc_name)
            {
                size_t field_size;
                char *field = get_v1_data_by_alias(*field_alias, tag1,
                                                   &field_size);

                memset(field, '\0', field_size);
                iconvordie(g_config.enc_v1, frame_enc_name,
                        frame->data + 1, frame->size - 1,
                        field, field_size - 1);
            }
        }
    }

    /* sync comment - use a first comment frame */
    {
        frame_id = get_frame_id_by_alias('c', tag2->header.version);
        frame = peek_frame(&tag2->frame_head, frame_id);

        if (frame)
        {
            int ret;
            struct id3v2_frm_comm *comm;

            ret = unpack_id3v2_frm_comm(frame, tag2->header.version, &comm);

            if (ret == 0)
            {
                if (comm->utext)
                {
                    memset(tag1->comment, '\0', sizeof(tag1->comment));
                    iconvordie(g_config.enc_v1, U32_CHAR_CODESET,
                               (char *)comm->utext,
                               u32_strlen(comm->utext) * sizeof(u32_char),
                               tag1->comment, sizeof(tag1->comment) - 1);
                }

                free_id3v2_frm_comm(comm);
            }
        }
    }

    /* sync track number */
    {
        int trackno = get_id3v2_tag_trackno(tag2);

        if (trackno >= 0 && trackno <= 255)
            tag1->track = trackno;
        else if (trackno > 255)
            print(OS_WARN, "track number '%i' is too big to be stored "
                           "in ID3v1, has not been synced", trackno);
        else if (trackno == -EILSEQ)
            print(OS_WARN, "track number frame has invalid format, "
                           "has not been synced");
    }

    /* sync genre_id and genre_str */
    {
        u32_char *genre_ustr = NULL;
        int genre_id = get_id3v2_tag_genre(tag2, &genre_ustr);

        if (genre_id >= 0)
        {
            tag1->genre_id = genre_id;

            if (genre_ustr)
            {
                memset(tag1->genre_str, '\0', sizeof(tag1->genre_str));
                iconvordie(g_config.enc_v1, U32_CHAR_CODESET,
                           (char *)genre_ustr,
                           u32_strlen(genre_ustr)*sizeof(u32_char),
                           tag1->genre_str, sizeof(tag1->genre_str) - 1);
                free(genre_ustr);
            }
        }
        else if (genre_id == -EILSEQ)
            print(OS_WARN, "genre frame has invalid format, "
                           "has not been synced");
    }

    return 0;
}

/***
 * get_v1e_timestamp
 *
 * The routine parses the string with the following timestamp format as per
 * the enhanced tag specification:
 *
 *    MMM:SS
 *
 * Returns time in seconds or -1 if passed string is not a valid timestamp
 * string.
 */

static int32_t get_v1e_timestamp(const char *timestamp)
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
    char        trackno[4] = "\0"; /* as maximum byte value is 255 */
    int32_t     time;
    const char  fields[] = "talyn";
    const char *field_alias;

    if (tag1->track)
        snprintf(trackno, sizeof(trackno), "%u", tag1->track);

    assert(tag2->header.version == 2 ||
           tag2->header.version == 3 ||
           tag2->header.version == 4);

    for (field_alias = fields; *field_alias != '\0'; field_alias++)
    {
        const char *frame_id;
        const char *field_data;
        size_t      field_data_sz;

        field_data = (*field_alias == 'n')
                     ? trackno
                     : get_v1_data_by_alias(*field_alias, tag1, NULL);

        field_data_sz = strlen(field_data);

        if (field_data_sz == 0)
            continue;

        frame_id = get_frame_id_by_alias(*field_alias, tag2->header.version);

        update_id3v2_tag_text_frame(
                tag2, frame_id, g_config.enc_v1,
                field_data, field_data_sz);
    }

    /* sync comment */

    if (tag1->comment[0] != '\0')
    {
        int ret;
        u32_char *utext;

        iconv_alloc(U32_CHAR_CODESET, g_config.enc_v1,
                    tag1->comment, strlen(tag1->comment),
                    (void *)&utext, NULL);

        ret = update_id3v2_frm_comm(tag2, "XXX", U32_EMPTY_STR, utext);
        free(utext);

        if (ret != 0)
            return ret;
    }

    /* sync genre */
    {
        u32_char *genre_ustr = NULL;

        if (!IS_EMPTY_STR(tag1->genre_str))
        {
            iconv_alloc(U32_CHAR_CODESET, g_config.enc_v1,
                        tag1->genre_str, strlen(tag1->genre_str),
                        (void *)&genre_ustr, NULL);
        }

        set_id3v2_tag_genre(tag2, tag1->genre_id, genre_ustr);
        free(genre_ustr);
    }

    /* TODO: sync starttime and endtime */

    time = get_v1e_timestamp(tag1->starttime);
//    if (time != -1)
//        update_id3v2_tag_event(tag2, ETCO_END_OF_INITIAL_SILENCE,
//                               time*1000);

    time = get_v1e_timestamp(tag1->endtime);
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
        return -EFAULT;

    if (g_config.ver.major == 1)
    {
        if (!tag2)
        {
            print(OS_ERROR, "%s: file has no ID3v2 tag to synchronise with",
                            filename);
            ret = -ENOENT;
        }
        else
        {
            if (!tag1)
            {
                tag1 = xcalloc(1, sizeof(struct id3v1_tag));
                tag1->version = 3;
                tag1->genre_id = ID3V1_UNKNOWN_GENRE;
            }

            if (g_config.ver.minor != NOT_SET)
                tag1->version = g_config.ver.minor;

            ret = sync_v1_with_v2(tag1, tag2);

            if (ret == 0)
                ret = write_tags(filename, tag1, NULL);
        }
    }
    else if (g_config.ver.major == 2)
    {
        if (!tag1)
        {
            print(OS_ERROR, "%s: file has no ID3v1 tag to synchronise with",
                            filename);
            ret = -ENOENT;
        }
        else
        {
            if (!tag2)
            {
                tag2 = new_id3v2_tag();

                if (g_config.ver.minor != NOT_SET)
                    tag2->header.version = g_config.ver.minor;
            }
            else if (g_config.ver.minor != tag2->header.version &&
                     g_config.ver.minor != NOT_SET)
            {
                print(OS_ERROR, "%s: present ID3v2 tag has a different "
                                "minor version, conversion is not implemented "
                                "yet, skipping this file", filename);

                /* TODO: convert between ID3v2 minor versions */

                ret = -ENOSYS;
            }

            if (ret == 0)
            {
                ret = sync_v2_with_v1(tag2, tag1);

                if (ret == 0)
                    ret = write_tags(filename, NULL, tag2);
            }
        }
    }

    free(tag1);
    free_id3v2_tag(tag2);
    return SUCC_OR_FAULT(ret);
}
