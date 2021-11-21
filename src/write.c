#include <config.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "output.h"
#include "params.h" /* NOT_SET */
#include "common.h" /* BLOCK_SIZE */
#include "trim.h"
#include "id3v1.h"
#include "id3v2.h"
#include "file.h"

int write_tags(const char *filename, const struct id3v1_tag *tag1,
               const struct id3v2_tag *tag2)
{
    struct file *file;
    char tag1_buf[ID3V1E_TAG_SIZE];
    size_t tag1_size = 0;
    char *tag2_buf;
    ssize_t tag2_size = 0;
    int ret;

    file = open_file(filename, O_RDWR);

    if (!file)
        return -EFAULT;

    if (tag1)
    {
        ret = trim_id3v1_tag(file, NOT_SET);

        if (ret < 0 && ret != -ENOENT)
        {
            close_file(file);
            return -EFAULT;
        }

        tag1_size = pack_id3v1_tag(tag1, tag1_buf);
    }

    if (tag2)
    {
        ret = trim_id3v2_tag(file, NOT_SET);

        if (ret < 0 && ret != -ENOENT)
        {
            close_file(file);
            return -EFAULT;
        }

        if (tag2->frame_head.next == &tag2->frame_head)
        {
            /* ID3v2.x standards: "A tag MUST contain at least one frame." */
            print(OS_WARN, "%s: no frames, ID3v2 tag omitted", filename);
        }
        else
        {
            off_t no_tag2_size = file->crop.end - file->crop.start + tag1_size;
            tag2_size = pack_id3v2_tag(tag2, &tag2_buf, no_tag2_size);

            if (tag2_size < 0)
            {
                close_file(file);
                switch (tag2_size)
                {
                    case -E2BIG:
                        print(OS_ERROR, "%s: tag too big", filename);
                        break;
                    case -EINVAL:
                        print(OS_ERROR, "%s: frame too big", filename);
                        break;
                }
                return -EFAULT;
            }
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
        lseek(file->fd, file->crop.end, SEEK_SET);
        write(file->fd, tag1_buf, tag1_size);
        ftruncate(file->fd, file->crop.end + tag1_size);
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
