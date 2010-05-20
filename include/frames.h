#ifndef FRAMES_H
#define FRAMES_H

#include <inttypes.h>
#include <wchar.h>
#include "id3v2.h"

const char *get_id3v2_tag_encoding_name(unsigned minor, char enc);

int get_frame_data(const struct id3v2_tag *tag, const struct id3v2_frame *frame,
                   wchar_t *buf, size_t size);

void unpack_frame_data(struct id3v2_frame *frame);
void pack_frame_data(struct id3v2_frame *frame);

int update_id3v2_tag_text_frame(struct id3v2_tag *tag, const char *frame_id,
                                char frame_enc_byte, char *data, size_t size);

int set_id3v2_tag_genre(struct id3v2_tag *tag, uint8_t genre_id,
                        wchar_t *genre_wcs);

int get_id3v2_tag_trackno(const struct id3v2_tag *tag);

int get_id3v2_tag_genre(const struct id3v2_tag *tag, wchar_t **genre_wcs);

#endif /* FRAMES_H */
