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

struct id3v2_frm_comm *new_id3v2_frm_comm();

void free_id3v2_frm_comm(struct id3v2_frm_comm *comm);

int unpack_id3v2_frm_comm(const struct id3v2_frame *frame,
                          unsigned minor,
                          struct id3v2_frm_comm **comm);

int peek_next_id3v2_frm_comm(const struct id3v2_tag *tag,
                             struct id3v2_frame **frame,
                             struct id3v2_frm_comm *comm,
                             uint8_t flags);

int update_id3v2_frm_comm(struct id3v2_tag *tag,
                          struct id3v2_frm_comm *comm,
                          uint8_t flags);

#endif /* FRM_COMM_H */
