#ifndef ALIAS_H
#define ALIAS_H

#include <stddef.h>
#include "id3v1.h"

struct alias
{
    char         alias;
    const char  *v22;
    const char  *v23;
    const char  *v24;
    size_t       v1offset;
    size_t       v1size;
    const char **conf;
};

const struct alias *get_alias(char alias);
const char *alias_to_frame_id(const struct alias *alias, unsigned version);
void *alias_to_v1_data(const struct alias *alias,
                       const struct id3v1_tag *tag, size_t *size);
const char *alias_to_config_data(const struct alias *alias);

#define is_valid_alias(x) (get_alias(x) != NULL)

#endif /* ALIAS_H */
