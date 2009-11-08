#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "id3v2.h"

struct id3v2_frame *find_frame(
        struct id3v2_frame_list *list,
        const char *name)
{
    for (; list != NULL; list = list->next)
    {
        if (memcmp(list->frame.id, name, 4) == 0)
            return &list->frame;
    }

    return NULL;
}

void free_frame_list(struct id3v2_frame_list *list)
{
    struct id3v2_frame_list *cur;

    while (list != NULL)
    {
        cur = list;
        list = cur->next;
        free(cur);
    }
}
