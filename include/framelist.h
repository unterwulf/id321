#ifndef FRAMELIST_H
#define FRAMELIST_H

#include "id3v2.h"

#define init_frame_list(list) { (list)->next = (list); (list)->prev = (list); }

int append_frame(struct id3v2_frame_list *head,
                 const struct id3v2_frame *frame);

struct id3v2_frame *find_frame(const struct id3v2_frame_list *head,
                               const char *name);

void free_frame_list(struct id3v2_frame_list *head);

#endif /* FRAMELIST_H */
