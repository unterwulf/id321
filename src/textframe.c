#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "alias.h"
#include "common.h"
#include "framelist.h"
#include "id3v2.h"
#include "output.h"
#include "params.h"
#include "u32_char.h"
#include "xalloc.h"

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
    if (!strcasecmp(enc_name, "ISO-8859-1"))
    {
        switch (minor)
        {
            case 2: return ID3V22_STR_ISO88591;
            case 3: return ID3V23_STR_ISO88591;
            case 4: return ID3V24_STR_ISO88591;
        }
    }
    else if (!strcasecmp(enc_name, "UCS-2"))
    {
        switch (minor)
        {
            case 2: return ID3V22_STR_UCS2;
            case 3: return ID3V23_STR_UCS2;
        }
    }
    else if (!strcasecmp(enc_name, "UTF-16"))
    {
        switch (minor)
        {
            case 4: return ID3V24_STR_UTF16;
        }
    }
    else if (!strcasecmp(enc_name, "UTF-16BE"))
    {
        switch (minor)
        {
            case 4: return ID3V24_STR_UTF16BE;
        }
    }
    else if (!strcasecmp(enc_name, "UTF-8"))
    {
        switch (minor)
        {
            case 4: return ID3V24_STR_UTF8;
        }
    }

    return ID3V2_UNSUPPORTED_ENCODING;
}

char get_id3v2_frame_encoding(uint8_t minor, const char *standard_encoding,
                              const char **actual_encoding)
{
    if (!standard_encoding)
        standard_encoding = (minor == 4) ? "UTF-8" : "UCS-2";

    char frame_enc_byte = get_id3v2_tag_encoding_byte(minor, standard_encoding);

    if (frame_enc_byte == ID3V2_UNSUPPORTED_ENCODING)
    {
        print(OS_WARN, "ID3v2.%d tag has no support of encoding '%s'",
              minor, standard_encoding);
    }
    else
    {
        *actual_encoding = get_id3v2_tag_encoding_name(minor, frame_enc_byte);
    }

    return frame_enc_byte;
}

void update_id3v2_tag_text_frame_payload(struct id3v2_frame *frame,
                                         char frame_enc_byte,
                                         const char *data, size_t size)
{
    free(frame->data);
    frame->size = size + 1;
    frame->data = xmalloc(frame->size);
    frame->data[0] = frame_enc_byte;
    memcpy(frame->data + 1, data, size);
}

static void update_id3v2_tag_text_frame_raw(struct id3v2_tag *tag,
                                            const char *frame_id,
                                            char frame_enc_byte,
                                            const char *data, size_t size)
{
    struct id3v2_frame *frame = peek_frame(&tag->frame_head, frame_id);

    if (!frame)
    {
        frame = xcalloc(1, sizeof(struct id3v2_frame));
        strncpy(frame->id, frame_id, ID3V2_FRAME_ID_MAX_SIZE);
        append_frame(&tag->frame_head, frame);
    }

    update_id3v2_tag_text_frame_payload(frame, frame_enc_byte, data, size);
}

int update_id3v2_tag_text_frame(struct id3v2_tag *tag,
                                const char *frame_id,
                                const char *encoding,
                                const char *data, size_t size)
{
    char *frame_data;
    size_t frame_data_sz;
    char frame_enc_byte = 0; /* all minor versions use 0 for ISO-8859-1 */

    int nr_errors = iconv_alloc(ASCII_CODESET, encoding,
                                data, size,
                                &frame_data, &frame_data_sz);

    if (nr_errors != 0)
    {
        /* Data contains characters outside ASCII range. */
        free(frame_data);
        const char *tgt_encoding;
        frame_enc_byte = get_id3v2_frame_encoding(tag->header.version,
                                                  g_config.default_v2_enc,
                                                  &tgt_encoding);

        if (frame_enc_byte == ID3V2_UNSUPPORTED_ENCODING)
            return -EINVAL;

        iconv_alloc(tgt_encoding, encoding,
                    data, size,
                    &frame_data, &frame_data_sz);
    }

    update_id3v2_tag_text_frame_raw(tag, frame_id, frame_enc_byte,
                                    frame_data, frame_data_sz);

    free(frame_data);
    return 0;
}

int get_text_frame_data_by_alias(const struct id3v2_tag *tag, char alias,
                                 u32_char **udata, size_t *udatasize)
{
    const char *frame_id = get_frame_id_by_alias(alias, tag->header.version);
    struct id3v2_frame *frame = peek_frame(&tag->frame_head, frame_id);
    const char *frame_enc_name;

    if (!frame)
        return -ENOENT;

    if (frame->size <= 1)
        return -EILSEQ;

    frame_enc_name =
        get_id3v2_tag_encoding_name(tag->header.version, frame->data[0]);

    if (!frame_enc_name)
        return -EILSEQ;

    iconv_alloc(U32_CHAR_CODESET, frame_enc_name,
                frame->data + 1, frame->size - 1,
                (void *)udata, udatasize);

    if (udatasize)
        *udatasize /= sizeof(u32_char);

    return 0;
}
