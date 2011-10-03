#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>   /* strerror() */
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"   /* BLOCK_SIZE */
#include "output.h"
#include "file.h"
#include "xalloc.h"

struct file *open_file(const char *filename, mode_t mode)
{
    struct stat st;
    struct file *file = xmalloc(sizeof(struct file));
    int ret;

    ret = stat(filename, &st);

    if (ret != 0)
    {
        print(OS_ERROR, "%s: %s", filename, strerror(errno));
        free(file);
        return NULL;
    }
    else if (!S_ISREG(st.st_mode))
    {
        print(OS_ERROR, "%s: not a regular file", filename);
        free(file);
        return NULL;
    }

    file->fd = open(filename, mode);

    if (file->fd == -1)
    {
        print(OS_ERROR, "%s: %s", filename, strerror(errno));
        free(file);
        return NULL;
    }

    file->size = st.st_size;

    /* initialize crop params */
    file->crop.start = 0;
    file->crop.end = file->size;

    return file;
}

int close_file(struct file *file)
{
    int ret;

    ret = close(file->fd);
    free(file);

    return ret;
}

int shift_file_payload(struct file *file, off_t delta)
{
    size_t  blksize = BLOCK_SIZE;
    size_t  zero = 0;
    char    buf[BLOCK_SIZE];
    size_t  size;
    off_t   pos;
    int     direction;
    size_t *corr;

    if (!delta)
        /* nothing to do */
        return 0;

    if (delta < 0)
    {
        pos = file->crop.start;
        direction = 1;
        corr = &zero;
    }
    else
    {
        pos = file->crop.end;
        direction = -1;
        corr = &blksize;
    }

    for (size = file->crop.end - file->crop.start;
         size != 0;
         size -= blksize, pos += direction*blksize)
    {
        if (size < blksize)
            blksize = size;

        lseek(file->fd, pos - *corr, SEEK_SET);
        read(file->fd, buf, blksize);
        lseek(file->fd, pos - *corr + delta, SEEK_SET);
        write(file->fd, buf, blksize);
    }

    file->crop.start += delta;
    file->crop.end += delta;

    if (file->crop.end > file->size)
        file->size = file->crop.end;

    return 0;
}
