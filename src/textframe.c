#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "alias.h"        /* get_alias(), alias_to_frame_id() */
#include "common.h"
#include "framelist.h"
#include "id3v2.h"
#include "params.h"
#include "u32_char.h"

/***
 * get_id3v2_tag_encoding - gets v2 tag frame encoding name by encoding byte
 *
 * @minor - v2 tag minor version
 * @enc - encoding byte
 *
 * Returns string with encoding name on success, or NULL on error.
 */

const char *get_id3v2_tag_encoding_name(unsigned minor, char enc)
{
    switch (minor)
    {
        case 2:
            switch (enc)
            {
                case ID3V22_STR_ISO88591: return g_config.enc_iso8859_1;
                case ID3V22_STR_UCS2:     return g_config.enc_ucs2;
            }
            break;

        case 3:
            switch (enc)
            {
                case ID3V23_STR_ISO88591: return g_config.enc_iso8859_1;
                case ID3V23_STR_UCS2:     return g_config.enc_ucs2;
            }
            break;

        case 4:
            switch (enc)
            {
                case ID3V24_STR_ISO88591: return g_config.enc_iso8859_1;
                case ID3V24_STR_UTF16:    return g_config.enc_utf16;
                case ID3V24_STR_UTF16BE:  return g_config.enc_utf16be;
                case ID3V24_STR_UTF8:     return g_config.enc_utf8;
            }
            break;

        default:
            assert(0);
    }

    return NULL;
}

char get_id3v2_tag_encoding_byte(unsigned minor, const char *enc_name)
{
    if (!strcasecmp(enc_name, g_config.enc_iso8859_1))
    {
        switch (minor)
        {
            case 2: return ID3V22_STR_ISO88591;
            case 3: return ID3V23_STR_ISO88591;
            case 4: return ID3V24_STR_ISO88591;
        }
    }
    else if (!strcasecmp(enc_name, g_config.enc_ucs2))
    {
        switch (minor)
        {
            case 2: return ID3V22_STR_UCS2;
            case 3: return ID3V23_STR_UCS2;
        }
    }
    else if (!strcasecmp(enc_name, g_config.enc_utf16))
    {
        switch (minor)
        {
            case 4: return ID3V24_STR_UTF16;
        }
    }
    else if (!strcasecmp(enc_name, g_config.enc_utf16be))
    {
        switch (minor)
        {
            case 4: return ID3V24_STR_UTF16BE;
        }
    }
    else if (!strcasecmp(enc_name, g_config.enc_utf8))
    {
        switch (minor)
        {
            case 4: return ID3V24_STR_UTF8;
        }
    }

    return ID3V2_UNSUPPORTED_ENCODING;
}

int update_id3v2_tag_text_frame_payload(struct id3v2_frame *frame,
                                        char frame_enc_byte,
                                        char *data, size_t size)
{
    size_t frame_size = size + 1;
    char *frame_data = malloc(frame_size);

    if (!frame_data)
        return -ENOMEM;

    frame_data[0] = frame_enc_byte;
    memcpy(frame_data + 1, data, frame_size - 1);
    free(frame->data);
    frame->size = frame_size;
    frame->data = frame_data;

    return 0;
}

int update_id3v2_tag_text_frame(struct id3v2_tag *tag, const char *frame_id,
                                char frame_enc_byte, char *data, size_t size)
{
    struct id3v2_frame *frame = peek_frame(&tag->frame_head, frame_id);
    int ret = 0;

    if (frame)
    {
        ret = update_id3v2_tag_text_frame_payload(
                  frame, frame_enc_byte, data, size);
    }
    else
    {
        frame = calloc(1, sizeof(struct id3v2_frame));

        if (!frame)
            return -ENOMEM;

        ret = update_id3v2_tag_text_frame_payload(
                  frame, frame_enc_byte, data, size);

        if (ret == 0)
        {
            strncpy(frame->id, frame_id, ID3V2_FRAME_ID_MAX_SIZE);
            append_frame(&tag->frame_head, frame);
        }
        else
            free_frame(frame);
    }

    return ret;
}

int get_text_frame_data_by_alias(const struct id3v2_tag *tag, char alias,
                                 u32_char **data, size_t *datasize)
{
    const char *frame_id;
    struct id3v2_frame *frame;
    const char *frame_enc_name;
    const struct alias *al = get_alias(alias);
    int ret;

    assert(al);
    frame_id = alias_to_frame_id(al, tag->header.version);
    frame = peek_frame(&tag->frame_head, frame_id);

    if (!frame)
        return -ENOENT;

    if (frame->size <= 1)
        return -EILSEQ;

    frame_enc_name =
        get_id3v2_tag_encoding_name(tag->header.version, frame->data[0]);

    if (!frame_enc_name)
        return -EILSEQ;

    ret = iconv_alloc(U32_CHAR_CODESET, frame_enc_name,
                      frame->data + 1, frame->size - 1,
                      (void *)data, datasize);

    if (datasize)
        *datasize /= sizeof(u32_char);

    return ret;
}
