#include <errno.h>
#include <locale.h>
#include <stdlib.h> /* EXIT_*, size_t */
#include "common.h" /* for_each() */
#include "output.h"
#include "params.h"

extern int init_config(int *argc, char ***argv);
extern int print_tags(const char *filename);
extern int delete_tags(const char *filename);
extern int modify_tags(const char *filename);
extern int sync_tags(const char *filename);
extern int copy_tags(int argc, char **argv);

char *program_name;

int main(int argc, char **argv)
{
    int ret = 0;
    static const struct {
        enum id3_action action;
        int (*func)(const char *);
    }
    actions[] =
    {
        { ID3_PRINT,  print_tags  },
        { ID3_MODIFY, modify_tags },
        { ID3_DELETE, delete_tags },
        { ID3_SYNC,   sync_tags   },
    };

    /* take care of locale */
    setlocale(LC_ALL, "");

    program_name = argv[0];

    if (init_config(&argc, &argv) != 0)
        return EXIT_FAILURE;

    if (argc == 0)
    {
        print(OS_ERROR, "no input files");
        return EXIT_FAILURE;
    }

    if (g_config.action == ID3_COPY)
    {
        ret = copy_tags(argc, argv);

        if (ret == -ENOMEM)
            print(OS_ERROR, "out of memory");
    }
    else
    {
        size_t i;

        for_each (i, actions)
            if (actions[i].action == g_config.action)
                break;

        for (; argc > 0; argc--, argv++)
        {
            ret = actions[i].func(*argv);

            if (ret == -ENOMEM)
                print(OS_ERROR, "%s: out of memory", *argv);
        }
    }

    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
