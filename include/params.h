#ifndef PARAMS_H
#define PARAMS_H

#include <inttypes.h>

#define ID321_OPT_SET_GENRE_ID               0x1
#define ID321_OPT_RM_GENRE_FRAME             0x2
#define ID321_OPT_SET_SPEED                  0x4
#define ID321_OPT_CHANGE_SIZE                0x8
#define ID321_OPT_EXPERT                     0x10
#define ID321_OPT_NO_UNSYNC                  0x20
#define ID321_OPT_ANY_COMM_LANG              0x40
#define ID321_OPT_ANY_COMM_DESC              0x80
#define ID321_OPT_RM_FRAME                   0x100
#define ID321_OPT_BIN_FRAME                  0x200
#define ID321_OPT_CREATE_FRAME               0x400
#define ID321_OPT_CREATE_FRAME_IF_NOT_EXISTS 0x800
#define ID321_OPT_ALL_FRAMES                 0x1000

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

struct id321_config
{
    enum id3_action action;
    uint32_t        options;
    uint32_t        size;
    struct version  ver;
    const char     *v2_def_encs;
    const char     *fmtstr;
    const char     *enc_v1;
    const char     *enc_iso8859_1;
    const char     *enc_ucs2;
    const char     *enc_utf16;
    const char     *enc_utf16be;
    const char     *enc_utf8;

    const char     *frame_id;
    int             frame_no;
    const char     *frame_enc;
    const char     *frame_data;
    uint32_t        frame_size;

    const char     *artist;
    const char     *album;
    const char     *title;
    const char     *comment_lang;
    const char     *comment_desc;
    const char     *comment;
    const char     *year;
    const char     *track;
    uint8_t         genre_id;
    const char     *genre_str;

    uint8_t         speed;
} g_config;

#endif /* PARAMS_H */
