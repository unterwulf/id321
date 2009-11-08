#include <locale.h>
#include "params.h"
#include "output.h"

extern int get_tags();
extern int delete_tags();

static int dummy()
{
    print(OS_ERROR, "sorry, this has not been implemented yet");

    return 0;
}

int main(int argc, char **argv)
{
    struct {
        id3_action_t id;
        int (* handler)();
    }
    modes[] =
    {
        { ID3_GET,       get_tags    },
        { ID3_MODIFY,    dummy       },
        { ID3_DELETE,    delete_tags },
        { ID3_SYNC,      dummy       },
        { ID3_NORMALIZE, dummy       }
    };
    int mode;

    /* take care of locale */
    setlocale(LC_ALL, "");

    if (init_config(argc, argv) == -1)
        return -1;

    for (mode = 0; mode < sizeof(modes)/sizeof(modes[0]); mode++)
    {
        if (g_config.action == modes[mode].id)
        {
            return modes[mode].handler();
        }
    }

    /* never reaches this line, because ID3_GET is default mode */
    return -1;
}
