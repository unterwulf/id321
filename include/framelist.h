#ifndef FRAMELIST_H
#define FRAMELIST_H

#include "id3v2.h"

void init_frame_list(struct id3v2_frame *head);

void append_frame(struct id3v2_frame *head, struct id3v2_frame *frame);

void unlink_frame(struct id3v2_frame *frame);
 
struct id3v2_frame *peek_frame(const struct id3v2_frame *head,
                               const char *name);

void free_frame(struct id3v2_frame *frame);

void free_frame_list(struct id3v2_frame *head);

#endif /* FRAMELIST_H */
