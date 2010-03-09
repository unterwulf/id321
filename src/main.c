#include <stdlib.h>
#include <locale.h>
#include "params.h"
#include "output.h"
#include "common.h" /* for_each() */

extern int get_tags(const char *filename);
extern int delete_tags(const char *filename);

char *program_name;

static int dummy(const char *filename)
{
    print(OS_ERROR, "sorry, this has not been implemented yet");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    static const struct
    {
        id3_action_t id;
        int (* handler)(const char *);
    }
    modes[] =
    {
        { ID3_PRINT,  get_tags    },
        { ID3_MODIFY, dummy       },
        { ID3_DELETE, delete_tags },
        { ID3_SYNC,   dummy       },
        { ID3_COPY,   dummy       }
    };
    unsigned mode;

    /* take care of locale */
    setlocale(LC_ALL, "");

    program_name = argv[0];

    if (init_config(&argc, &argv) == -1)
        return EXIT_FAILURE;

    for_each (mode, modes)
    {
        if (g_config.action == modes[mode].id)
        {
            for (; argc > 0; argc--, argv++)
                if (modes[mode].handler(*argv) != 0)
                    return EXIT_FAILURE;

            return EXIT_SUCCESS;
        }
    }

    /* never reaches this line, because ID3_PRINT is default mode */
    return EXIT_FAILURE;
}
