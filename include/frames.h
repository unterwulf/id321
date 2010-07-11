#ifndef FRAMES_H
#define FRAMES_H

#include <inttypes.h>
#include "id3v2.h"
#include "u32_char.h"

const char *get_id3v2_tag_encoding_name(unsigned minor, char enc);

char get_id3v2_tag_encoding_byte(unsigned minor, const char *enc_name);

int get_frame_data(const struct id3v2_tag *tag,
                   const struct id3v2_frame *frame,
                   u32_char *buf, size_t size);

void unpack_frame_data(struct id3v2_frame *frame);

void pack_frame_data(struct id3v2_frame *frame);

int update_id3v2_tag_text_frame_payload(struct id3v2_frame *frame,
                                        char frame_enc_byte,
                                        char *data, size_t size);

int update_id3v2_tag_text_frame(struct id3v2_tag *tag,
                                const char *frame_id,
                                char frame_enc_byte,
                                char *data, size_t size);

int set_id3v2_tag_genre(struct id3v2_tag *tag,
                        uint8_t genre_id,
                        u32_char *genre_u32_str);

int get_id3v2_tag_trackno(const struct id3v2_tag *tag);

int get_id3v2_tag_genre(const struct id3v2_tag *tag, u32_char **genre_u32_str);

#endif /* FRAMES_H */
