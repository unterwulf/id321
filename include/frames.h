#ifndef FRAMES_H
#define FRAMES_H

#include "id3v2.h"

void unpack_frame_data(struct id3v2_frame *frame);
void pack_frame_data(struct id3v2_frame *frame);

#endif /* FRAMES_H */
