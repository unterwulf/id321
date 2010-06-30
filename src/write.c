#include <errno.h>
#include <fcntl.h>
#include "output.h"
#include "params.h" /* NOT_SET */
#include "common.h" /* BLOCK_SIZE */
#include "crop.h"
#include "id3v1.h"
#include "id3v2.h"
#include "file.h"

int write_tags(const char *filename, const struct id3v1_tag *tag1,
               const struct id3v2_tag *tag2)
{
    struct file *file;
    char *tag2_buf;
    ssize_t tag2_size = 0;
    int ret;

    file = open_file(filename, O_RDWR);

    if (!file)
        return -EFAULT;

    if (tag1)
    {
        ret = crop_id3v1_tag(file, NOT_SET);

        if (ret < 0 && ret != -ENOENT)
        {
            close_file(file);
            return -EFAULT;
        }
    }

    if (tag2)
    {
        ret = crop_id3v2_tag(file, NOT_SET);

        if (ret < 0 && ret != -ENOENT)
        {
            close_file(file);
            return -EFAULT;
        }

        tag2_size = pack_id3v2_tag(tag2, &tag2_buf);

        if (tag2_size < 0)
        {
            close_file(file);
            return NOMEM_OR_FAULT(tag2_size);
        }
    }

    /* check if existing padding space is not enough or should be changed */
    if (tag2 && file->crop.start != tag2_size)
        ret = shift_file_payload(file, tag2_size - file->crop.start);

    if (tag2)
    {
        lseek(file->fd, 0, SEEK_SET);
        write(file->fd, tag2_buf, tag2_size);
        print(OS_INFO, "ID3v2.%u tag written", tag2->header.version);
    }

    if (tag1)
    {
        char   buf[ID3V1E_TAG_SIZE];
        size_t size = pack_id3v1_tag(buf, tag1); 

        lseek(file->fd, file->crop.end, SEEK_SET);
        write(file->fd, buf, size);
        ftruncate(file->fd, file->crop.end + size);
        print(OS_INFO, "ID3v1.%u tag written", tag1->version);
    }
    else if (file->crop.end < file->size)
    {
        /* v2 tag was shrunk, so need to truncate file */
        ftruncate(file->fd, file->crop.end);
    }

    close_file(file);

    return 0;
}
