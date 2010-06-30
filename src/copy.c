#include <errno.h>
#include <stdlib.h>
#include "id3v1.h"
#include "id3v2.h"
#include "params.h"
#include "output.h"
#include "common.h" /* get_tags(), write_tags() */

int copy_tags(int argc, char **argv)
{
    struct id3v1_tag *tag1 = NULL;
    struct id3v2_tag *tag2 = NULL;
    int ret;

    if (argc != 2)
    {
        print(OS_ERROR, "exactly two files should be specified");
        return -EFAULT;
    }

    ret = get_tags(argv[0], g_config.ver, &tag1, &tag2);

    if (ret < 0)
        return NOMEM_OR_FAULT(ret);

    if (!tag1 && !tag2)
    {
        print(OS_ERROR, "%s: no specified tags in the source file", argv[0]);
        return -EFAULT;
    }

    ret = write_tags(argv[1], tag1, tag2);

    free(tag1);
    free_id3v2_tag(tag2);
    return SUCC_NOMEM_OR_FAULT(ret);
}
