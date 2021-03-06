#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h> /* lseek, SEEK_* */
#include <fcntl.h>  /* O_RDWR */
#include "common.h"
#include "params.h" /* NOT_SET */
#include "id3v1.h"
#include "id3v2.h"
#include "output.h"
#include "trim.h"
#include "file.h"
#include "dump.h"
#include "xalloc.h"

/***
 * get_id3v1_tag - get ID3v1 tag from file
 *
 * @file - file to process
 * @minor - minor version of ID3v1 tag to be found, may be NOT_SET
 * @tag - pointer to a pointer which will be set to the allocated
 *        struct id3v2_tag if the routine succeed
 *
 * The routine finds ID3v1 tag specified by @minor in @file. If the tag has
 * been found, struct id3v1_tag is allocated and filled with parsed tag values.
 *
 * Returns 0 on success, -ENOENT if no matching ID3v1 tag was found and
 * -EFAULT on other errors.
 */

static int get_id3v1_tag(struct file *file, unsigned minor,
                         struct id3v1_tag **tag)
{
    char   buf[ID3V1E_TAG_SIZE];
    size_t size;
    int    ret;

    ret = trim_id3v1_tag(file, minor);

    if (ret == -ENOENT)
        return ret;
    else if (ret < 0)
        return -EFAULT;

    size = ret;
    assert(size <= sizeof(buf));

    lseek(file->fd, file->crop.end, SEEK_SET);
    if (readordie(file->fd, buf, size) != 0)
        return -EFAULT;

    *tag = xmalloc(sizeof(struct id3v1_tag));

    ret = -ENOENT;

    if (minor == ID3V1E_MINOR || minor == NOT_SET)
        /* read v1 enhanced tag if available */
        ret = unpack_id3v1_enh_tag(buf, size, *tag);

    if (ret != 0 && (minor == 2 || minor == NOT_SET))
        /* read v1.2 tag if available */
        ret = unpack_id3v12_tag(buf, size, *tag);

    if (ret != 0 && (minor == 0 || minor == 1))
        /* read v1.[01] tag if available */
        ret = unpack_id3v1_tag(buf, size, *tag);

    if (ret != 0 && (minor == 3 || minor == NOT_SET))
        /* read v1.3 tag if available */
        ret = unpack_id3v13_tag(buf, size, *tag);

    if (ret != 0)
    {
        assert(ret != -E2BIG);
        free(*tag);
        *tag = NULL;
    }

    return (ret == -ENOENT || ret == 0) ? ret : -EFAULT;
}

static int get_id3v2_tag_prealloc(struct file *file, unsigned minor,
                                  struct id3v2_tag *tag)
{
    int ret;

    assert(tag);

    lseek(file->fd, 0, SEEK_SET);

    /* read ID3v2 tag if available */
    ret = read_id3v2_header(file->fd, &tag->header);

    if (ret != 0)
        return ret;

    if (minor == tag->header.version || minor == NOT_SET)
    {
        dump_id3_header(&tag->header);

        if (tag->header.flags & ID3V2_FLAG_EXT_HEADER)
            ret = read_id3v2_ext_header(file->fd, tag);

        ret = read_id3v2_frames(file->fd, tag);

        if (ret != 0)
            return ret;
    }
    else
    {
        print(OS_INFO, "file has ID3v2.%d tag, ignore it",
                       tag->header.version);
        return -ENOENT;
    }

    return 0;
}

static int get_id3v2_tag(struct file *file, unsigned minor,
                         struct id3v2_tag **tag)
{
    int ret;

    *tag = new_id3v2_tag();
    ret = get_id3v2_tag_prealloc(file, minor, *tag);

    if (ret != 0)
    {
        free_id3v2_tag(*tag);
        *tag = NULL;
        return ret;
    }

    /* TODO: check presence of an appended ID3v2 tag */

    return 0;
}

int get_tags(const char *filename, struct version ver,
             struct id3v1_tag **tag1, struct id3v2_tag **tag2)
{
    int ret = 0;
    struct file *file;

    file = open_file(filename, O_RDWR);

    if (!file)
        return -EFAULT;

    /* the order makes sense */
    if (ver.major == 1 || ver.major == NOT_SET)
    {
        ret = get_id3v1_tag(file, ver.minor, tag1);

        if (ret == -ENOENT)
        {
            print(OS_INFO, "no matching ID3v1 tag found");
            ret = 0;
        }
        else if (ret == -EFAULT)
            print(OS_ERROR, "%s: unable to read ID3v1 tag", filename);
    }
    else
        *tag1 = NULL;

    if (ret == 0 && (ver.major == 2 || ver.major == NOT_SET))
    {
        ret = get_id3v2_tag(file, ver.minor, tag2);

        if (ret == -ENOENT)
        {
            print(OS_INFO, "no matching ID3v2 tag found");
            ret = 0;
        }
        else if (ret == -EFAULT)
            print(OS_ERROR, "%s: unable to read ID3v2 tag", filename);
    }
    else
        *tag2 = NULL;

    if (ret != 0 && *tag1)
    {
        free(*tag1);
        *tag1 = NULL;
    }

    close_file(file);

    return SUCC_OR_FAULT(ret);
}
