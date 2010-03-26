#include <stdlib.h>
#include "id3v1.h"
#include "id3v2.h"
#include "params.h"
#include "output.h"
#include "common.h" /* NOT_SET, get_tags(), write_tags() */

int copy_tags(int argc, char **argv)
{
    struct id3v1_tag *tag1 = NULL;
    struct id3v2_tag *tag2 = NULL;
    int               ret;

    if (argc != 2)
    {
        print(OS_ERROR, "exactly two files should be specified");
        return EXIT_FAILURE;
    }

    ret = get_tags(argv[0], g_config.ver, &tag1, &tag2);

    if (ret != 0)
        return EXIT_FAILURE;

    ret = write_tags(argv[1], tag1, tag2);

    free(tag1);
    free_id3v2_tag(tag2);
    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
