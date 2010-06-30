#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>        /* sscanf() */
#include <string.h>
#include <inttypes.h>
#include <wchar.h>
#include "common.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v2.h"
#include "params.h"
#include "output.h"
#include "alias.h"
#include "frames.h"       /* set_id3v2_tag_genre_by_id() */
#include "frm_comm.h"
#include "framelist.h"

static int sync_v1_with_v2(struct id3v1_tag *tag1, const struct id3v2_tag *tag2)
{
    const char fields[] = "taly";
    const char *field_alias;
    const char *frame_id;
    const struct alias *al;
    struct id3v2_frame *frame;
    int ret;

    /* sync pure text fields */
    for (field_alias = fields; *field_alias != '\0'; field_alias++)
    {
        al = get_alias(*field_alias);
        assert(al);
        frame_id = alias_to_frame_id(al, tag2->header.version);

        if (!frame_id)
            return -EINVAL;
        
        frame = peek_frame(&tag2->frame_head, frame_id);

        if (frame && frame->size > 1)
        {
            const char *frame_enc_name =
                get_id3v2_tag_encoding_name(tag2->header.version,
                                            frame->data[0]);
            if (frame_enc_name)
            {
                size_t field_size;
                char *field = alias_to_v1_data(al, tag1, &field_size);

                memset(field, '\0', field_size);
                iconvordie(g_config.enc_v1, frame_enc_name,
                        frame->data + 1, frame->size - 1,
                        field, field_size - 1);
            }
        }
    }

    /* sync comment - use a first comment frame */
    {
        al = get_alias('c');
        assert(al);
        frame_id = alias_to_frame_id(al, tag2->header.version);

        if (!frame_id)
            return -EINVAL;

        frame = peek_frame(&tag2->frame_head, frame_id);

        if (frame)
        {
            struct id3v2_frm_comm *comm;

            ret = unpack_id3v2_frm_comm(tag2->header.version, frame, &comm);

            if (ret == 0)
            {
                if (comm->text)
                {
                    memset(tag1->comment, '\0', sizeof(tag1->comment));
                    iconvordie(g_config.enc_v1, WCHAR_CODESET,
                        (char *)comm->text, wcslen(comm->text)*sizeof(wchar_t),
                        tag1->comment, sizeof(tag1->comment) - 1);
                }

                free_id3v2_frm_comm(comm);
            }
        }
    }

    /* sync track number */
    {
        int trackno = get_id3v2_tag_trackno(tag2);

        if (trackno >= 0)
            tag1->track = trackno;
        else if (trackno == -ENOENT)
            ; /* nothing to sync with */
        else if (trackno == -EILSEQ)
            print(OS_WARN, "track number frame has invalid format, "
                           "has not been synced");
        else
            return trackno; /* something bad happened */
    }

    /* sync genre_id and genre_str */
    {
        wchar_t *genre_wcs = NULL;
        int genre_id = get_id3v2_tag_genre(tag2, &genre_wcs);

        if (genre_id >= 0)
        {
            tag1->genre_id = genre_id;

            if (genre_wcs)
            {
                memset(tag1->genre_str, '\0', sizeof(tag1->genre_str));
                iconvordie(g_config.enc_v1, WCHAR_CODESET,
                           (char *)genre_wcs, wcslen(genre_wcs)*sizeof(wchar_t),
                           tag1->genre_str, sizeof(tag1->genre_str) - 1);
                free(genre_wcs);
            }
        }
        else if (genre_id == -ENOENT)
            ; /* nothing to sync with */
        else if (genre_id == -EILSEQ)
            print(OS_WARN, "genre frame has invalid format, "
                           "has not been synced");
        else
            return genre_id; /* something bad happened */
    }

    return 0;
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
    char        trackno[4] = "\0"; /* as maximum byte value is 255 */
    int32_t     time;
    char        frame_enc_byte;
    const char *frame_enc_name;
    const char  fields[] = "talyn";
    const char *field_alias;
    int         ret = 0;

    if (tag1->track)
        snprintf(trackno, sizeof(trackno), "%u", tag1->track);

    assert(tag2->header.version == 2 ||
           tag2->header.version == 3 ||
           tag2->header.version == 4);

    frame_enc_byte = g_config.v2_def_encs[tag2->header.version];
    frame_enc_name = get_id3v2_tag_encoding_name(tag2->header.version,
                                                 frame_enc_byte);

    if (!frame_enc_name)
        return -EINVAL;

    for (field_alias = fields; *field_alias != '\0'; field_alias++)
    {
        char       *buf;
        size_t      bufsize;
        size_t      data_size;
        const char *frame_id;
        const char *field_data;
        const struct alias *al = get_alias(*field_alias);

        field_data = (*field_alias == 'n')
                     ? trackno
                     : alias_to_v1_data(al, tag1, NULL);

        data_size = strlen(field_data);

        if (data_size == 0)
            continue;

        assert(al);
        frame_id = alias_to_frame_id(al, tag2->header.version);

        if (!frame_id)
            return -EINVAL;

        ret = iconv_alloc(frame_enc_name, g_config.enc_v1,
                          field_data, data_size,
                          &buf, &bufsize);
        if (ret != 0)
            return ret;

        ret = update_id3v2_tag_text_frame(tag2, frame_id, frame_enc_byte,
                                          buf, bufsize);
        
        free(buf);

        if (ret != 0)
            return ret;
    }

    /* sync comment */

    if (tag1->comment[0] != '\0')
    {
        struct id3v2_frm_comm *comm = new_id3v2_frm_comm();

        if (!comm)
            return -ENOMEM;

        ret = iconv_alloc(WCHAR_CODESET, g_config.enc_v1,
                          tag1->comment, strlen(tag1->comment),
                          (void *)&comm->text, NULL);
        if (ret != 0)
            return ret;

        ret = update_id3v2_frm_comm(tag2, comm, 0);
        free_id3v2_frm_comm(comm);

        if (ret != 0)
            return ret;
    }

    /* sync genre */

    if (tag1->genre_id != ID3V1_UNKNOWN_GENRE || tag1->genre_str[0] != '\0')
    {
        wchar_t *genre_wcs = NULL;
        
        if (tag1->genre_str[0] != '\0')
        {
            ret = iconv_alloc(WCHAR_CODESET, g_config.enc_v1,
                              tag1->genre_str, strlen(tag1->genre_str),
                              (void *)&genre_wcs, NULL);
            if (ret != 0)
                return ret;
        }
                                 
        ret = set_id3v2_tag_genre(tag2, tag1->genre_id, genre_wcs);
        free(genre_wcs);

        if (ret != 0)
            return ret;
    }

    /* TODO: sync starttime and endtime */

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
        return NOMEM_OR_FAULT(ret);

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
                tag1 = calloc(1, sizeof(struct id3v1_tag));

                if (tag1)
                {
                    tag1->version = (g_config.ver.minor != NOT_SET)
                                    ? g_config.ver.minor : 3;
                    tag1->genre_id = ID3V1_UNKNOWN_GENRE;
                }
                else
                    ret = -ENOMEM;
            }

            if (ret == 0)
            {
                ret = sync_v1_with_v2(tag1, tag2);

                if (ret == 0)
                    ret = write_tags(filename, tag1, NULL);
            }
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

                if (tag2)
                    tag2->header.version = (g_config.ver.minor != NOT_SET)
                                           ? g_config.ver.minor : 4;
                else
                    ret = -ENOMEM;
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
    return SUCC_NOMEM_OR_FAULT(ret);
}
