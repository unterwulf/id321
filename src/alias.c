#include <stdlib.h>

const char *alias_to_frame_id(char alias, int version)
{
    int i;
    static const struct
    {
        const char alias;
        const char *v22;
        const char *v23;
        const char *v24;
    }
    map[] =
    {
        { 'a', "TP1", "TPE1", "TPE1" },
        { 'c', "COM", "COMM", "COMM" },
        { 'g', "TCO", "TCON", "TCON" },
        { 'G', "TCO", "TCON", "TCON" },
        { 'l', "TAL", "TALB", "TALB" },
        { 'n', "TRK", "TRCK", "TRCK" },
        { 't', "TT2", "TIT2", "TIT2" },
        { 'y', "TYE", "TYER", "TDRC" }
    };

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        if (alias == map[i].alias)
        {
            switch (version)
            {
                case 2: return map[i].v22;
                case 3: return map[i].v23;
                case 4: return map[i].v24;
                default: return NULL;
            }
        }
    }

    return NULL;
}
