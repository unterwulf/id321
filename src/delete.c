#include <config.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h> /* ftruncate() */
#include "trim.h"
#include "file.h"
#include "output.h"
#include "params.h" /* NOT_SET */

int delete_tags(const char *filename)
{
    struct file *file;
    int ret;

    file = open_file(filename, O_RDWR);

    if (!file)
        return -EFAULT;

    /* the order makes sense */
    if (g_config.ver.major == 1 || g_config.ver.major == NOT_SET)
    {
        ret = trim_id3v1_tag(file, g_config.ver.minor);

        if (ret < 0 && ret != -ENOENT)
        {
            close_file(file);
            return -EFAULT;
        }
    }

    if (g_config.ver.major == 2 || g_config.ver.major == NOT_SET)
    {
        ret = trim_id3v2_tag(file, g_config.ver.minor);

        if (ret < 0 && ret != -ENOENT)
        {
            close_file(file);
            return -EFAULT;
        }
    }

    if (file->crop.start == 0 && file->crop.end == file->size)
        print(OS_WARN, "%s: no specified tags found", filename);
    else
    {
        print(OS_DEBUG, "%s: start: %d, end: %d, size: %d",
                        filename, file->crop.start, file->crop.end, file->size);

        shift_file_payload(file, -file->crop.start);

        if (file->crop.end < file->size)
            ftruncate(file->fd, file->crop.end);
    }

    close_file(file);
    return 0;
}
