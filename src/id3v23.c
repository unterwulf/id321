#include <string.h>
#include "common.h"

const char *map_v23_to_v24(const char *v23frame)
{
    unsigned i;
    static const struct
    {
        const char *v23;
        const char *v24;
    }
    framemap[] =
    {
        { "EQUA", "EQU2" },
        { "IPLS", "TIPL" },
        { "RVAD", "RVA2" },
        { "TDAT", "TDRC" },
        { "TIME", "TDRC" },
        { "TORY", "TDOR" },
        { "TRDA", "TSIZ" },
        { "TYER", "TDRC" }
    };

    for_each (i, framemap)
    {
        if (memcmp(framemap[i].v23, v23frame, 4) == 0)
            return framemap[i].v24;
    }

    return NULL;
}
