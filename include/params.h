#ifndef CONFIG_H
#define CONFIG_H

#include "id3v2.h"

#define ID3T_FORCE_ENCODING  0x8

#define NOT_SET -1

typedef enum {
    ID3_GET, ID3_MODIFY, ID3_DELETE, ID3_SYNC, ID3_NORMALIZE
} id3_action_t;

struct {
    id3_action_t action;
    uint32_t     options;
    int          major_ver;
    int          minor_ver;
    const char  *filename;
    const char  *fmtstr;
    const char  *encoding;
} g_config;

int init_config(int argc, char **argv);

#endif /* CONFIG_H */
