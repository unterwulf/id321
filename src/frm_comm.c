#include <errno.h>
#include <inttypes.h>
#include <stdio.h>        /* snprintf() */
#include <stdlib.h>
#include <string.h>
#include "alias.h"
#include "id3v2.h"
#include "output.h"
#include "params.h"       /* g_config */
#include "common.h"
#include "frm_comm.h"
#include "framelist.h"
#include "textframe.h"    /* get_id3v2_tag_encoding_name() */
#include "u16_char.h"
#include "u32_char.h"
#include "xalloc.h"

struct id3v2_frm_comm *new_id3v2_frm_comm(void)
{
    struct id3v2_frm_comm *comm = xcalloc(1, sizeof(struct id3v2_frm_comm));

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
    if (comm)
    {
        free(comm->udesc);
        free(comm->utext);
        free(comm);
    }
}

int unpack_id3v2_frm_comm(const struct id3v2_frame *frame, unsigned minor,
                          struct id3v2_frm_comm **comm)
{
    const char *from_enc;
    struct id3v2_frm_comm *tmp_comm = NULL;
    char *desc_ptr;
    char *text_ptr;
    size_t desc_sz;
    size_t text_sz = 0;

    if (frame->size < ID3V2_FRM_COMM_HDR_SIZE + 1)
        return -EILSEQ;

    from_enc = get_id3v2_tag_encoding_name(minor, frame->data[0]);

    if (!from_enc)
    {
        print(OS_WARN, "invalid string encoding 0x%.2X in frame '%.4s'",
                       (unsigned char)frame->data[0], frame->id);
        return -EILSEQ;
    }

    desc_ptr = frame->data + ID3V2_FRM_COMM_HDR_SIZE;
    desc_sz = frame->size - ID3V2_FRM_COMM_HDR_SIZE;

    /* find description/text NULL-separator */
    switch (frame->data[0])
    {
        case ID3V24_STR_UTF16: /* ID3V22_STR_UCS2, ID3V23_STR_UCS2 as well */
        case ID3V24_STR_UTF16BE:
            text_ptr = u16_memchr(desc_ptr, U16_CHAR('\0'),
                                  desc_sz / sizeof(u16_char));
            if (text_ptr)
            {
                desc_sz = text_ptr - desc_ptr;
                text_ptr += sizeof(u16_char);
            }
            break;
        default:
            text_ptr = memchr(desc_ptr, '\0', desc_sz);
            if (text_ptr)
            {
                desc_sz = text_ptr - desc_ptr;
                text_ptr++;
            }
            break;
    }

    if (text_ptr)
        text_sz = frame->size - ID3V2_FRM_COMM_HDR_SIZE - (text_ptr - desc_ptr);

    tmp_comm = new_id3v2_frm_comm();

    if (desc_sz > 0)
    {
        iconv_alloc(U32_CHAR_CODESET, from_enc,
                    desc_ptr, desc_sz,
                    (void *)&tmp_comm->udesc, NULL);
    }

    if (text_ptr && text_sz > 0)
    {
        iconv_alloc(U32_CHAR_CODESET, from_enc,
                    text_ptr, text_sz,
                    (void *)&tmp_comm->utext, NULL);
    }

    memcpy(tmp_comm->lang, frame->data + ID3V2_ENC_HDR_SIZE,
           ID3V2_LANG_HDR_SIZE);

    *comm = tmp_comm;

    return 0;
}

static struct id3v2_frm_comm *query_next_frm_comm(
                                    const struct id3v2_tag *tag,
                                    struct id3v2_frame **frame,
                                    const char *lang,
                                    const u32_char *udesc)
{
    const char *frame_id = get_frame_id_by_alias('c', tag->header.version);

    while ((*frame = peek_next_frame(&tag->frame_head, frame_id, *frame)))
    {
        struct id3v2_frm_comm *comm;
        int ret = unpack_id3v2_frm_comm(*frame, tag->header.version, &comm);

        if (ret == -EILSEQ)
            continue; /* just skip malformed frame */
        else if (ret != 0)
            return NULL; /* something bad happend */

        if ((lang == NULL || !memcmp(comm->lang, lang, ID3V2_LANG_HDR_SIZE)) &&
            (udesc == NULL ||
             (IS_EMPTY_STR(comm->udesc) && IS_EMPTY_STR(udesc)) ||
             !u32_strcmp(comm->udesc, udesc)))
        {
            /* a matching frame found */
            return comm;
        }

        free_id3v2_frm_comm(comm);
    }

    return NULL;
}

static int pack_id3v2_frm_comm(const struct id3v2_frm_comm *comm,
                               struct id3v2_frame **frame,
                               unsigned minor)
{
    const char *frame_id = get_frame_id_by_alias('c', minor);
    char frame_enc_byte;
    const char *frame_enc_name;
    char *desc;
    char *text = NULL;
    size_t desc_size;
    size_t text_size = 0;
    struct id3v2_frame *new_frame;

    frame_enc_byte = g_config.v2_def_encs[minor];
    frame_enc_name = get_id3v2_tag_encoding_name(minor, frame_enc_byte);

    if (!frame_enc_name)
        return -EINVAL;

    new_frame = xcalloc(1, sizeof(struct id3v2_frame));

    /* convert comment description */
    {
        const u32_char *udesc = (comm->udesc) ? comm->udesc : U32_EMPTY_STR;

        /* desc shall include null-terminator */
        iconv_alloc(frame_enc_name, U32_CHAR_CODESET,
                    (const char *)udesc,
                    (u32_strlen(udesc) + 1) * sizeof(u32_char),
                    &desc, &desc_size);
    }

    /* convert comment text */
    if (comm->utext)
    {
        /* text shall not include null-terminator */
        iconv_alloc(frame_enc_name, U32_CHAR_CODESET,
                    (char *)comm->utext,
                    u32_strlen(comm->utext) * sizeof(u32_char),
                    &text, &text_size);
    }

    new_frame->size =
        ID3V2_ENC_HDR_SIZE + ID3V2_LANG_HDR_SIZE + desc_size + text_size;
    new_frame->data = xmalloc(new_frame->size);

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

int update_id3v2_frm_comm(struct id3v2_tag *tag, const char *lang,
                          const u32_char *udesc, const u32_char *utext)
{
    struct id3v2_frame *next_frame = &tag->frame_head;
    struct id3v2_frame *tmp_frame;
    struct id3v2_frm_comm *comm;
    int ret = 0;

    /* If utext is empty string delete all matching frames. */
    if (IS_EMPTY_STR(utext))
    {
        while (comm = query_next_frm_comm(tag, &next_frame, lang, udesc))
        {
            tmp_frame = next_frame->prev;
            unlink_frame(next_frame);
            free_frame(next_frame);
            free_id3v2_frm_comm(comm);
            next_frame = tmp_frame;
        }
    }
    /* If there are matching frames update contents of all of them. */
    else if (comm = query_next_frm_comm(tag, &next_frame, lang, udesc))
    {
        do
        {
            u32_xstrupd(&comm->utext, utext);
            ret = pack_id3v2_frm_comm(comm, &tmp_frame,
                                      tag->header.version);

            if (ret != 0)
            {
                free_id3v2_frm_comm(comm);
                return ret;
            }

            insert_frame_before(next_frame, tmp_frame);
            unlink_frame(next_frame);
            free_frame(next_frame);
            free_id3v2_frm_comm(comm);
            next_frame = tmp_frame;
        } while (comm = query_next_frm_comm(tag, &next_frame, lang, udesc));
    }
    /* If no matching frame found and all parameters have been specified
     * create new frame and append it to the tag. */
    else if (lang != NULL && udesc != NULL)
    {
        struct id3v2_frm_comm new_comm;

        memcpy(new_comm.lang, lang, ID3V2_LANG_HDR_SIZE);
        new_comm.udesc = (u32_char *)udesc;
        new_comm.utext = (u32_char *)utext;

        ret = pack_id3v2_frm_comm(&new_comm, &tmp_frame, tag->header.version);

        if (ret != 0)
            return ret;

        append_frame(&tag->frame_head, tmp_frame);
    }
    else
    {
        ret = -ENOENT;
    }

    return ret;
}
