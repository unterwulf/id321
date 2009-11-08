#ifndef FRAMELIST_H
#define FRAMELIST_H

#include "id3v2.h"

struct id3v2_frame *
//int
find_frame(
//        struct id3v2_frame **frame,
        struct id3v2_frame_list *list,
        const char *name);

#endif /* FRAMELIST_H */
