#include <assert.h>
#include <stddef.h>  /* NULL, size_t, offsetof() */
#include "alias.h"
#include "id3v1.h"   /* struct id3v1_tag {...} */
#include "common.h"  /* for_each() */
#include "params.h"  /* g_config */

#define OFF1(field) offsetof(struct id3v1_tag, field)
#define SIZ1(field) sizeof(((struct id3v1_tag *)NULL)->field)
#define OFF1SIZ1(field) OFF1(field), SIZ1(field)
#define CONF(field) &g_config.field
#define OFF1SIZ1CONF(field) OFF1SIZ1(field), CONF(field)

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

static const struct alias *find_alias(char alias)
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

int is_valid_alias(char alias)
{
    return (find_alias(alias) != NULL);
}

static const struct alias *get_alias(char alias)
{
    const struct alias *al = find_alias(alias);
    assert(al != NULL);
    return al;
}

/**
 * The following functions do not report errors. It is a caller
 * responsibility to call these functions with only right parameters.
 */

const char *get_frame_id_by_alias(char alias, unsigned version)
{
    const struct alias *al = get_alias(alias);
    switch (version)
    {
        case 2: return al->v22;
        case 3: return al->v23;
        case 4: return al->v24;
        default: assert(0);
    }
}

void *get_v1_data_by_alias(char alias, const struct id3v1_tag *tag,
                           size_t *size)
{
    const struct alias *al = get_alias(alias);
    if (size)
        *size = al->v1size;
    return (char *)tag + al->v1offset;
}

const char *get_config_data_by_alias(char alias)
{
    const struct alias *al = get_alias(alias);
    assert(al->conf != NULL);
    return *al->conf;
}
