#include <stdlib.h>
#include <unistd.h> /* lseek, SEEK_* */
#include <string.h>
#include <fcntl.h>  /* O_RDWR */
#include "common.h"
#include "params.h"
#include "id3v1.h"
#include "id3v2.h"
#include "output.h"
#include "crop.h"
#include "file.h"
#include "dump.h"

static int get_id3v1_tag(struct file *file, unsigned minor,
                         struct id3v1_tag **tag)
{
    char    buf[ID3V1E_TAG_SIZE];
    ssize_t size;
    int     ret;

    ret = crop_id3v1_tag(file, NOT_SET);

    if (ret != 0)
        return 0;

    lseek(file->fd, file->crop.end, SEEK_SET);
    size = readordie(file->fd, buf, ID3V1E_TAG_SIZE);

    if (size <= 0)
        return -1;

    *tag = malloc(sizeof(struct id3v1_tag));

    if (!*tag)
    {
        print(OS_ERROR, "out of memory");
        return -1;
    }

    ret = -1;

    if (minor == ID3V1E_MINOR || minor == NOT_SET)
        /* read v1 enhanced tag if available */
        ret = unpack_id3v1_enh_tag(*tag, buf, size);

    if (ret == -1 && (minor == 2 || minor == NOT_SET))
        /* read v1.2 tag if available */
        ret = unpack_id3v12_tag(*tag, buf, size);

    if (ret == -1 && (minor == 0 || minor == 1))
        /* read v1.[01] tag if available */
        ret = unpack_id3v1_tag(*tag, buf, size);

    if (ret == -1 && (minor == 3 || minor == NOT_SET))
        /* read v1.3 tag if available */
        ret = unpack_id3v13_tag(*tag, buf, size);

    if (ret == -1)
    {
        free(*tag);
        *tag = NULL;
    }

    return 0;
}

static int get_id3v2_tag(struct file *file, unsigned minor,
                         struct id3v2_tag **tag)
{
    int ret;

    *tag = new_id3v2_tag();

    if (!*tag)
    {
        print(OS_ERROR, "out of memory");
        return -1;
    }

    lseek(file->fd, 0, SEEK_SET);

    /* read id3v2 tag if available */
    if (read_id3v2_header(file->fd, &(*tag)->header) != -1)
    {
        if (minor == (*tag)->header.version || minor == NOT_SET)
        {
            dump_id3_header(&(*tag)->header);

            if ((*tag)->header.flags & ID3V2_FLAG_EXT_HEADER)
                ret = read_id3v2_ext_header(file->fd, *tag);

            ret = read_id3v2_frames(file->fd, *tag);
        }
        else
        {
            print(OS_DEBUG, "file has id3v2.%d tag, ignore it",
                            (*tag)->header.version);
            free_id3v2_tag(*tag);
            *tag = NULL;
        }
    }
    else
    {
        /* check presence of an appended id3v2 tag */
        free_id3v2_tag(*tag);
        *tag = NULL;
    }

    return 0;
}

int get_tags(const char *filename, struct version ver,
             struct id3v1_tag **tag1, struct id3v2_tag **tag2)
{
    int ret = 0;
    struct file *file;

    file = open_file(filename, O_RDWR);

    if (!file)
        return -1;

    /* the order makes sense */
    if (ver.major == 1 || ver.major == NOT_SET)
        ret = get_id3v1_tag(file, ver.minor, tag1);
    else
        *tag1 = NULL;

    if (ret == 0 && (ver.major == 2 || ver.major == NOT_SET))
        ret = get_id3v2_tag(file, ver.minor, tag2);
    else
        *tag2 = NULL;

    if (ret != 0 && *tag1)
    {
        free(*tag1); 
        *tag1 = NULL;
    }

    close_file(file);

    return ret;
}
