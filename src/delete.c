#include <unistd.h>
#include <fcntl.h>
#include "params.h"
#include "output.h"
#include "common.h" /* NOT_SET */
#include "file.h"
#include "crop.h"

int delete_tags(const char *filename)
{
    struct file *file;
    int          ret;

    file = open_file(filename, O_RDWR);

    if (!file)
        return -1;
    
    /* the order makes sense */
    if (g_config.ver.major == 1 || g_config.ver.major == NOT_SET)
        ret = crop_id3v1_tag(file, g_config.ver.minor);

    if (g_config.ver.major == 2 || g_config.ver.major == NOT_SET)
        ret = crop_id3v2_tag(file, g_config.ver.minor);

    if (file->crop.start == 0 && file->crop.end == file->size)
        print(OS_DEBUG, "there are no specified tags found in file `%s'",
                        filename);
    else
    {
        print(OS_DEBUG, "start: %d, end: %d, size: %d",
                        file->crop.start, file->crop.end, file->size);

        shift_file_payload(file, -file->crop.start);

        if (file->crop.end < file->size)
            ftruncate(file->fd, file->crop.end);
    }

    close_file(file);

    return 0;
}
