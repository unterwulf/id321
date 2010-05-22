#include <fcntl.h>
#include "output.h"
#include "params.h"
#include "common.h" /* BLOCK_SIZE */
#include "crop.h"
#include "id3v1.h"
#include "id3v2.h"
#include "file.h"

int write_tags(const char *filename, const struct id3v1_tag *tag1,
               const struct id3v2_tag *tag2)
{
    struct file *file;
    int          ret;
    char        *tag2buf;
    ssize_t      tag2size = 0;

    file = open_file(filename, O_RDWR);

    if (!file)
        return -1;

    if (tag1)
        (void)crop_id3v1_tag(file, NOT_SET);

    if (tag2)
    {
        (void)crop_id3v2_tag(file, NOT_SET);
        tag2size = pack_id3v2_tag(tag2, &tag2buf);
    }

    /* check if existing padding space is not enough or should be changed */
    if (tag2 && file->crop.start != tag2size)
        ret = shift_file_payload(file, tag2size - file->crop.start);

    if (tag2)
    {
        lseek(file->fd, 0, SEEK_SET);
        write(file->fd, tag2buf, tag2size);
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
