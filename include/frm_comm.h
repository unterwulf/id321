#ifndef FRM_COMM_H
#define FRM_COMM_H

#include <inttypes.h>
#include <stdlib.h>
#include <wchar.h>
#include "id3v2.h"

struct id3v2_frm_comm
{
    char lang[ID3V2_LANG_HDR_SIZE];
    wchar_t *desc;
    wchar_t *text;
};

struct id3v2_frm_comm *new_id3v2_frm_comm();
void free_id3v2_frm_comm(struct id3v2_frm_comm *comm);

int unpack_id3v2_frm_comm(unsigned minor, const struct id3v2_frame *frame,
                          struct id3v2_frm_comm **comm);

int peek_next_id3v2_frm_comm(const struct id3v2_tag *tag,
                             struct id3v2_frame **frame,
                             struct id3v2_frm_comm *comm, uint8_t flags);

int update_id3v2_frm_comm(struct id3v2_tag *tag,
                          struct id3v2_frm_comm *comm,
                          uint8_t flags);

#endif /* FRM_COMM_H */
