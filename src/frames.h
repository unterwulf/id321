#ifndef FRAMES_H
#define FRAMES_H

#include <stddef.h>
#include "id3v2.h"
#include "u32_char.h"

int get_frame_data(const struct id3v2_tag *tag,
                   const struct id3v2_frame *frame,
                   u32_char *ubuf, size_t usize);

#endif /* FRAMES_H */
