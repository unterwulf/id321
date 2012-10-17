#ifndef FRM_COMM_H
#define FRM_COMM_H

#include <inttypes.h>
#include "id3v2.h"
#include "u32_char.h"

struct id3v2_frm_comm
{
    char lang[ID3V2_LANG_HDR_SIZE];
    u32_char *desc;
    u32_char *text;
};

struct id3v2_frm_comm *new_id3v2_frm_comm(void);

void free_id3v2_frm_comm(struct id3v2_frm_comm *comm);

int unpack_id3v2_frm_comm(const struct id3v2_frame *frame,
                          unsigned minor,
                          struct id3v2_frm_comm **comm);

int update_id3v2_frm_comm(struct id3v2_tag *tag, const char *lang,
                          const u32_char *udesc, const u32_char *utext);

#endif /* FRM_COMM_H */
