#ifndef TEXTFRAME_H
#define TEXTFRAME_H

#include <inttypes.h>
#include <stddef.h>
#include "id3v2.h"
#include "u32_char.h"

const char *get_id3v2_tag_encoding_name(unsigned minor, char enc);

char get_id3v2_tag_encoding_byte(unsigned minor, const char *enc_name);

char get_id3v2_frame_encoding(uint8_t minor, const char *standard_encoding,
                              const char **actual_encoding);

void update_id3v2_tag_text_frame_payload(struct id3v2_frame *frame,
                                         char frame_enc_byte,
                                         const char *data, size_t size);

int update_id3v2_tag_text_frame(struct id3v2_tag *tag,
                                const char *frame_id,
                                const char *encoding,
                                const char *data, size_t size);

int get_text_frame_data_by_alias(const struct id3v2_tag *tag, char alias,
                                 u32_char **udata, size_t *udatasize);

#endif /* TEXTFRAME_H */
