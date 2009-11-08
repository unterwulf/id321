#include <string.h>

const char *map_v23_to_v24(const char *v23frame)
{
    int i;
    static const struct {
        const char *v23;
        const char *v24;
    }
    framemap[] = {
        { "EQUA", "EQU2" },
        { "IPLS", "TIPL" },
        { "RVAD", "RVA2" },
        { "TDAT", "TDRC" },
        { "TIME", "TDRC" },
        { "TORY", "TDOR" },
        { "TRDA", "TSIZ" },
        { "TYER", "TDRC" }
    };

    for (i = 0; i < sizeof(framemap)/sizeof(framemap[0]); i++)
    {
        if (memcmp(framemap[i].v23, v23frame, 4) == 0)
            return framemap[i].v24;
    }

    return NULL;
}
