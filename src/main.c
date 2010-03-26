#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include "params.h"
#include "output.h"
#include "common.h" /* for_each() */

extern int init_config(int *argc, char ***argv);
extern int print_tags(const char *filename);
extern int delete_tags(const char *filename);
extern int modify_tags(const char *filename);
extern int sync_tags(const char *filename);
extern int copy_tags(int argc, char **argv);

char *program_name;

static void usage()
{
    printf("usage: %s {[pr]|mo|rm|sy} [-1|-2] [ARGS] FILE...\n", program_name);
}

int main(int argc, char **argv)
{
    static int (* const actions[])(const char *) =
    {
        [ ID3_PRINT  ] = print_tags,
        [ ID3_MODIFY ] = modify_tags,
        [ ID3_DELETE ] = delete_tags,
        [ ID3_SYNC   ] = sync_tags,
    };

    /* take care of locale */
    setlocale(LC_ALL, "");

    program_name = argv[0];

    if (init_config(&argc, &argv) == -1)
        return EXIT_FAILURE;

    if (argc == 0)
    {
        usage();
        return EXIT_SUCCESS;
    }

    if (g_config.action == ID3_COPY)
        return copy_tags(argc, argv);

    for (; argc > 0; argc--, argv++)
        if (actions[g_config.action](*argv) != 0)
            return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
