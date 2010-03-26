#ifndef PARAMS_H
#define PARAMS_H

#include <inttypes.h>

#define ID321_OPT_SET_GENRE_ID 0x1
#define ID321_OPT_SET_SPEED    0x2
#define ID321_OPT_CHANGE_SIZE  0x4
#define ID321_OPT_EXPERT       0x8
#define ID321_OPT_NO_UNSYNC    0x10

#define NOT_SET 255

enum id3_action
{
    ID3_PRINT,
    ID3_MODIFY,
    ID3_DELETE,
    ID3_SYNC,
    ID3_COPY,
};

struct version
{
    unsigned major;
    unsigned minor;
};

struct
{
    enum id3_action action;
    uint32_t        options;
    uint32_t        size;
    struct version  ver;
    const char     *v2_def_encs;
    const char     *fmtstr;
    const char     *frame;
    const char     *enc_v1;
    const char     *enc_iso8859_1;
    const char     *enc_ucs2;
    const char     *enc_utf16;
    const char     *enc_utf16be;
    const char     *enc_utf8;

    const char     *artist;
    const char     *album;
    const char     *title;
    const char     *comment;
    const char     *year;
    const char     *track;
    uint8_t         genre_id;
    const char     *genre_str;

    uint8_t         speed;
} g_config;

#endif /* PARAMS_H */
