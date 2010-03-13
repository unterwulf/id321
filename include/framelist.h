#ifndef FRAMELIST_H
#define FRAMELIST_H

#include "id3v2.h"

struct id3v2_frame *find_frame(const struct id3v2_frame_list *head,
                               const char *name);
void free_frame_list(struct id3v2_frame_list *head);

#endif /* FRAMELIST_H */
