#ifndef FRAMES_H
#define FRAMES_H

#include <inttypes.h>
#include "id3v2.h"

const char *get_id3v2_tag_encoding_name(unsigned minor, char enc);

void print_frame_data(const struct id3v2_tag *tag,
                      const struct id3v2_frame *frame);
void unpack_frame_data(struct id3v2_frame *frame);
void pack_frame_data(struct id3v2_frame *frame);

int set_id3v2_tag_genre_by_id(struct id3v2_tag *tag, uint8_t genre_id);

int update_id3v2_tag_text_frame(struct id3v2_tag *tag, const char *frame_id,
                                char frame_enc_byte, char *data, size_t size);

#endif /* FRAMES_H */
