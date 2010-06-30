#define _GNU_SOURCE       /* wcsdup() */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>        /* snprintf() */
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "alias.h"
#include "id3v2.h"
#include "output.h"
#include "params.h"       /* g_config */
#include "common.h"
#include "frames.h"
#include "frm_comm.h"
#include "framelist.h"

struct id3v2_frm_comm *new_id3v2_frm_comm()
{
    struct id3v2_frm_comm *comm = calloc(1, sizeof(struct id3v2_frm_comm));

    if (!comm)
        return NULL;

    /* ID3v2.4 specification says: If the language is not known the string
     * "XXX" should be used.
     *
     * Since the ID3v2.2 and ID3v2.3 specs do not specify this case, we will
     * use "XXX" for v2.2 and v2.3 tags as well. */

    memcpy(comm->lang, "XXX", 3);
    return comm;
}

void free_id3v2_frm_comm(struct id3v2_frm_comm *comm)
{
    free(comm->desc);
    free(comm->text);
    free(comm);
}

int unpack_id3v2_frm_comm(unsigned minor, const struct id3v2_frame *frame,
                          struct id3v2_frm_comm **comm)
{
    const char *from_enc;
    int ret;
    char *text;
    struct id3v2_frm_comm *tmp_comm = NULL;

    if (frame->size < ID3V2_FRM_COMM_HDR_SIZE + 1)
        return -EILSEQ;

    from_enc = get_id3v2_tag_encoding_name(minor, frame->data[0]);

    if (!from_enc)
    {
        print(OS_WARN, "invalid string encoding 0x%.2X in frame '%.4s'",
                       (unsigned char)frame->data[0], frame->id);
        return -EILSEQ;
    }

    /* find the data offset */

    text = frame->data + ID3V2_FRM_COMM_HDR_SIZE;
    while ((text = memchr(text, '\0', frame->size - (text - frame->data))))
    {
        switch (frame->data[0])
        {
            case ID3V23_STR_UCS2: /* same as ID3V24_STR_UTF16 */
            case ID3V24_STR_UTF16BE:
                if (text + 1 >= frame->data + frame->size)
                    goto endloop;
                else
                    text++;

                if (((text - frame->data) % 2) && *text == '\0')
                    goto endloop;
                break;
            default:
                goto endloop;
        }
    }

endloop:

    if (!text)
        return -EILSEQ;

    text++; /* skip the null character */

    tmp_comm = new_id3v2_frm_comm();

    if (!tmp_comm)
        return -ENOMEM;

    memcpy(tmp_comm->lang, frame->data + 1, ID3V2_LANG_HDR_SIZE);

    ret = iconv_alloc(WCHAR_CODESET, from_enc,
                      frame->data + ID3V2_FRM_COMM_HDR_SIZE,
                      text - frame->data - ID3V2_FRM_COMM_HDR_SIZE,
                      (void *)&tmp_comm->desc, NULL);

    if (ret != 0)
        goto err;

    if (text + 1 < frame->data + frame->size)
    {
        ret = iconv_alloc(WCHAR_CODESET, from_enc,
                          text, frame->size - (text - frame->data),
                          (void *)&tmp_comm->text, NULL);

        if (ret != 0)
            goto err;
    }

    *comm = tmp_comm;
    return 0;

err:

    free_id3v2_frm_comm(tmp_comm);
    return ret;
}

int peek_next_id3v2_frm_comm(const struct id3v2_tag *tag,
                             struct id3v2_frame **frame,
                             struct id3v2_frm_comm *comm, uint8_t flags)
{
    const struct alias *al = get_alias('c');
    const char *frame_id;
    struct id3v2_frm_comm *tmp_comm;
    int ret;

    assert(al);
    frame_id = alias_to_frame_id(al, tag->header.version);

    while ((*frame = peek_next_frame(&tag->frame_head, frame_id, *frame)))
    {
        ret = unpack_id3v2_frm_comm(tag->header.version, *frame, &tmp_comm);

        if (ret == -EILSEQ)
            continue; /* just skip malformed frame */
        else if (ret != 0)
            return ret; /* something bad happend */

        if (flags & ID321_OPT_ANY_COMM_LANG)
            memcpy(comm->lang, tmp_comm->lang, ID3V2_LANG_HDR_SIZE);

        if (flags & ID321_OPT_ANY_COMM_DESC)
        {
            free(comm->desc);
            if (tmp_comm->desc)
            {
                comm->desc = wcsdup(tmp_comm->desc);
                if (!comm->desc)
                    return -ENOMEM;
            }
            else
                comm->desc = NULL;
        }

        if (!memcmp(tmp_comm->lang, comm->lang, ID3V2_LANG_HDR_SIZE) 
            && ((IS_EMPTY_WCS(tmp_comm->desc)
                 && IS_EMPTY_WCS(comm->desc))
                || (tmp_comm->desc && comm->desc
                        && !wcscmp(tmp_comm->desc, comm->desc))))
        {
            free_id3v2_frm_comm(tmp_comm);
            /* a matching frame found */
            return 0;
        }

        free_id3v2_frm_comm(tmp_comm);
    }

    return -ENOENT;
}

static int pack_id3v2_frm_comm(unsigned minor,
                               const struct id3v2_frm_comm *comm,
                               struct id3v2_frame **frame)
{
    const struct alias *al = get_alias('c');
    const char *frame_id;
    char frame_enc_byte;
    const char *frame_enc_name;
    char *desc;
    char *text = NULL;
    size_t desc_size;
    size_t text_size = 0;
    struct id3v2_frame *new_frame;
    int ret;

    assert(al);
    frame_id = alias_to_frame_id(al, minor);

    frame_enc_byte = g_config.v2_def_encs[minor];
    frame_enc_name = get_id3v2_tag_encoding_name(minor, frame_enc_byte);

    if (!frame_enc_name)
        return -EINVAL;

    new_frame = calloc(1, sizeof(struct id3v2_frame));

    if (!new_frame)
        return -ENOMEM;

    /* desc shall include null-terminator */
    ret = iconv_alloc(frame_enc_name, WCHAR_CODESET,
                      comm->desc ? (char *)comm->desc : (char *)L"",
                      (comm->desc ? wcslen(comm->desc) + 1 : 1)*sizeof(wchar_t),
                      &desc, &desc_size);

    if (ret != 0)
    {
        free(new_frame);
        return -ENOMEM;
    }

    if (comm->text)
    {
        /* text shall not include null-terminator */
        ret = iconv_alloc(frame_enc_name, WCHAR_CODESET,
                      (char *)comm->text, wcslen(comm->text)*sizeof(wchar_t),
                      &text, &text_size);
        if (ret != 0)
        {
            free(new_frame);
            free(desc);
            return -ENOMEM;
        }
    }

    new_frame->size =
        ID3V2_ENC_HDR_SIZE + ID3V2_LANG_HDR_SIZE + desc_size + text_size;
    new_frame->data = malloc(new_frame->size);

    if (!new_frame->data)
    {
        free(new_frame);
        free(desc);
        free(text);
        return -ENOMEM;
    }

    strncpy(new_frame->id, frame_id, ID3V2_FRAME_ID_MAX_SIZE);
    new_frame->data[0] = frame_enc_byte;
    memcpy(new_frame->data + ID3V2_ENC_HDR_SIZE,
           comm->lang, ID3V2_LANG_HDR_SIZE);
    memcpy(new_frame->data + ID3V2_ENC_HDR_SIZE + ID3V2_LANG_HDR_SIZE,
           desc, desc_size);
    memcpy(new_frame->data + ID3V2_ENC_HDR_SIZE +
           ID3V2_LANG_HDR_SIZE + desc_size,
           text, text_size);

    free(desc);
    free(text);

    *frame = new_frame;

    return 0;
}

int update_id3v2_frm_comm(struct id3v2_tag *tag, struct id3v2_frm_comm *comm,
                          uint8_t flags)
{
    struct id3v2_frame *old_frame = &tag->frame_head;
    struct id3v2_frame *new_frame;
    int ret;

    ret = peek_next_id3v2_frm_comm(tag, &old_frame, comm, flags);

    if (ret == -ENOENT && !flags && !IS_EMPTY_WCS(comm->text))
    {
        ret = pack_id3v2_frm_comm(tag->header.version, comm, &new_frame);

        if (ret != 0)
            return ret;

        append_frame(&tag->frame_head, new_frame);
    }
    else if (ret == 0)
    {
        do
        {
            if (!IS_EMPTY_WCS(comm->text))
            {
                ret = pack_id3v2_frm_comm(tag->header.version,
                                          comm, &new_frame);

                if (ret != 0)
                    return ret;

                insert_frame_before(old_frame, new_frame);
            }
            else
                new_frame = old_frame->prev;

            unlink_frame(old_frame);
            free_frame(old_frame);
            old_frame = new_frame;
        } while ((ret = peek_next_id3v2_frm_comm(tag, &old_frame, comm, flags))
                    == 0);

        if (ret == -ENOENT)
            return 0; /* it is okay if there is no more matching frames */
    }

    return ret;
}
