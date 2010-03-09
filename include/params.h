#ifndef PARAMS_H
#define PARAMS_H

#include <inttypes.h>
#include "id3v2.h"

#define NOT_SET 255

typedef enum {
    ID3_PRINT, ID3_MODIFY, ID3_DELETE, ID3_SYNC, ID3_COPY
} id3_action_t;

struct version
{
    unsigned major;
    unsigned minor;
};

struct {
    id3_action_t    action;
    uint32_t        options;
    struct version  ver;
    const char     *fmtstr;
    const char     *enc_iso8859_1;
    const char     *enc_utf8;
    const char     *enc_utf16;
    const char     *enc_utf16be;

    const char     *artist;
    const char     *album;
    const char     *title;
    const char     *comment;
    const char     *year;
    const char     *track;
    const char     *genre;
} g_config;

int init_config(int *argc, char ***argv);

#endif /* PARAMS_H */
