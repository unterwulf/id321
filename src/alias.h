#ifndef ALIAS_H
#define ALIAS_H

#include <stddef.h>
#include "id3v1.h"

int is_valid_alias(char alias);
const char *get_frame_id_by_alias(char alias, unsigned version);
const char *get_config_data_by_alias(char alias);
void *get_v1_data_by_alias(char alias,
                           const struct id3v1_tag *tag, size_t *size);

#endif /* ALIAS_H */
