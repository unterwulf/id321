#ifndef FRAMELIST_H
#define FRAMELIST_H

#include "id3v2.h"

void init_frame_list(struct id3v2_frame *head);
void append_frame(struct id3v2_frame *head, struct id3v2_frame *frame);
#define insert_frame_before(before, frame) append_frame(before, frame)
void unlink_frame(struct id3v2_frame *frame);
 
struct id3v2_frame *peek_next_frame(const struct id3v2_frame *head,
                                    const char *name,
                                    const struct id3v2_frame *pos);

#define peek_frame(head, name) peek_next_frame(head, name, head)

void free_frame(struct id3v2_frame *frame);
void free_frame_list(struct id3v2_frame *head);

#endif /* FRAMELIST_H */
