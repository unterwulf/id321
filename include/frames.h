#ifndef FRAMES_H
#define FRAMES_H

#include <inttypes.h>
#include "id3v2.h"

void print_frame_data(const struct id3v2_tag *tag,
                      const struct id3v2_frame *frame);
void unpack_frame_data(struct id3v2_frame *frame);
void pack_frame_data(struct id3v2_frame *frame);

int set_id3v2_tag_genre_by_id(struct id3v2_tag *tag, uint8_t genre_id);

#endif /* FRAMES_H */
