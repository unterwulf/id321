#ifndef ALIAS_H
#define ALIAS_H

#include <stddef.h>
#include "id3v1.h"

const char *alias_to_frame_id(char alias, unsigned version);
const void *alias_to_v1_data(char alias, const struct id3v1_tag *tag,
                             size_t *size);

#define is_valid_alias(x) ((x) == *(char *)alias_to_frame_id((x), 0))

#endif /* ALIAS_H */
