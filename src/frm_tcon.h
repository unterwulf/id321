#ifndef FRM_TCON_H
#define FRM_TCON_H

#include <inttypes.h>
#include "id3v2.h"
#include "u32_char.h"

int get_id3v2_tag_genre(const struct id3v2_tag *tag, u32_char **genre_ustr);

void set_id3v2_tag_genre(struct id3v2_tag *tag,
                         uint8_t genre_id,
                         u32_char *genre_ustr);

#endif /* FRM_TCON_H */
