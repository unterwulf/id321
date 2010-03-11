#include <stdlib.h>
#include <stddef.h> /* offsetof() */
#include "id3v1.h"  /* struct id3v1_tag {...} */
#include "common.h" /* for_each() */

#define OFFV1(field) offsetof(struct id3v1_tag, field)
#define SIZV1(field) sizeof(((struct id3v1_tag *)NULL)->field)

static const struct
{
    const char alias;
    const char *v22;
    const char *v23;
    const char *v24;
    const size_t v1offset;     
    const size_t v1size;     
}
map[] =
{
    { 'a', "TP1", "TPE1", "TPE1", OFFV1(artist),  SIZV1(artist)  },
    { 'c', "COM", "COMM", "COMM", OFFV1(comment), SIZV1(comment) },
    { 'g', "TCO", "TCON", "TCON", OFFV1(genre),   SIZV1(genre)   },
    { 'G', "TCO", "TCON", "TCON", OFFV1(genre),   SIZV1(genre)   },
    { 'l', "TAL", "TALB", "TALB", OFFV1(album),   SIZV1(album)   },
    { 'n', "TRK", "TRCK", "TRCK", OFFV1(track),   SIZV1(track)   },
    { 't', "TT2", "TIT2", "TIT2", OFFV1(title),   SIZV1(title)   },
    { 'y', "TYE", "TYER", "TDRC", OFFV1(year),    SIZV1(year)    }
};

const char *alias_to_frame_id(char alias, unsigned version)
{
    unsigned i;

    for_each (i, map)
    {
        if (map[i].alias == alias)
        {
            switch (version)
            {
                case 0: return &map[i].alias;
                case 2: return map[i].v22;
                case 3: return map[i].v23;
                case 4: return map[i].v24;
                default: return NULL;
            }
        }
    }

    return NULL;
}

const void *alias_to_v1_data(char alias, const struct id3v1_tag *tag,
                             size_t *size)
{
    unsigned i;

    for_each (i, map)
    {
        if (alias == map[i].alias)
        {
            *size = map[i].v1size;
            return (void *)tag + map[i].v1offset;
        }
    }

    return NULL;
}
