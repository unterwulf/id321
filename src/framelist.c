#include <stdlib.h>
#include <string.h>
#include "id3v2.h"

int append_frame(struct id3v2_frame_list *head,
                 const struct id3v2_frame *frame)
{
    struct id3v2_frame_list *new_node =
                                 malloc(sizeof(struct id3v2_frame_list));

    if (!new_node)
        return -1;

    memcpy(&new_node->frame, frame, sizeof(struct id3v2_frame));

    new_node->prev = head->prev;
    head->prev->next = new_node;
    new_node->next = head;
    head->prev = new_node;

    return 0;
}
 
struct id3v2_frame *find_frame(const struct id3v2_frame_list *head,
                               const char *name)
{
    struct id3v2_frame_list *frame;

    for (frame = head->next; frame != head; frame = frame->next)
        if (!memcmp(frame->frame.id, name, 4))
            return &frame->frame;

    return NULL;
}

void free_frame_list(struct id3v2_frame_list *head)
{
    struct id3v2_frame_list *next = head->next;
    struct id3v2_frame_list *tmp;

    while (next != head)
    {
        tmp = next;
        next = next->next;
        free(tmp->frame.data);
        free(tmp);
    }
}
