#include <stdlib.h>
#include <string.h>
#define _FRAME_LIST
#include "id3v2.h"

void init_frame_list(struct id3v2_frame *head)
{
    head->next = head;
    head->prev = head;
}

void append_frame(struct id3v2_frame *head, struct id3v2_frame *frame)
{
    frame->prev = head->prev;
    head->prev->next = frame;
    frame->next = head;
    head->prev = frame;
}

void unlink_frame(struct id3v2_frame *frame)
{
    frame->prev->next = frame->next;
    frame->next->prev = frame->prev;
    frame->next = frame->prev = NULL;
}
 
struct id3v2_frame *peek_frame(const struct id3v2_frame *head,
                               const char *name)
{
    struct id3v2_frame *frame;

    for (frame = head->next; frame != head; frame = frame->next)
        if (!memcmp(frame->id, name, ID3V2_FRAME_ID_MAX_SIZE))
            return frame;

    return NULL;
}

void free_frame(struct id3v2_frame *frame)
{
    free(frame->data);
    free(frame);
}

void free_frame_list(struct id3v2_frame *head)
{
    struct id3v2_frame *next = head->next;
    struct id3v2_frame *tmp;

    while (next != head)
    {
        tmp = next;
        next = next->next;
        free_frame(tmp);
    }
}
