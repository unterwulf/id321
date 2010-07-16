#include <stddef.h>  /* NULL, size_t, offsetof() */
#include "alias.h"   /* struct alias {...} */
#include "id3v1.h"   /* struct id3v1_tag {...} */
#include "common.h"  /* for_each() */
#include "params.h"  /* g_config */

#define OFF1(field) offsetof(struct id3v1_tag, field)
#define SIZ1(field) sizeof(((struct id3v1_tag *)NULL)->field)
#define OFF1SIZ1(field) OFF1(field), SIZ1(field)
#define CONF(field) &g_config.field
#define OFF1SIZ1CONF(field) OFF1SIZ1(field), CONF(field)

const struct alias *get_alias(char alias)
{
    size_t i;
    static const struct alias map[] =
    {
        { 'a', "TP1", "TPE1", "TPE1", OFF1SIZ1CONF(artist)     },
        { 'c', "COM", "COMM", "COMM", OFF1SIZ1CONF(comment)    },
        { 'g', "TCO", "TCON", "TCON", OFF1SIZ1(genre_id), NULL },
        { 'G', "TCO", "TCON", "TCON", OFF1SIZ1CONF(genre_str)  },
        { 'l', "TAL", "TALB", "TALB", OFF1SIZ1CONF(album)      },
        { 'n', "TRK", "TRCK", "TRCK", OFF1SIZ1CONF(track)      },
        { 't', "TT2", "TIT2", "TIT2", OFF1SIZ1CONF(title)      },
        { 'y', "TYE", "TYER", "TDRC", OFF1SIZ1CONF(year)       },
    };

    for_each (i, map)
        if (map[i].alias == alias)
            return &map[i];

    return NULL;
}

const char *alias_to_frame_id(const struct alias *alias, unsigned version)
{
    switch (version)
    {
        case 0: return &alias->alias;
        case 2: return alias->v22;
        case 3: return alias->v23;
        case 4: return alias->v24;
        default: return NULL;
    }
}

void *alias_to_v1_data(const struct alias *alias,
                       const struct id3v1_tag *tag, size_t *size)
{
    if (size)
        *size = alias->v1size;

    return (void *)tag + alias->v1offset;
}

const char *alias_to_config_data(const struct alias *alias)
{
    if (alias->conf)
        return *alias->conf;
    else
        return NULL;
}
