#include <assert.h>
#include <errno.h>
#include <unistd.h> /* SEEK_* */
#include <string.h>
#include "id3v2.h"
#include "id3v1.h"
#include "output.h"
#include "params.h" /* NOT_SET */
#include "file.h"

#define ID3V1E_TAG_EXTRA_SIZE (ID3V1E_TAG_SIZE - ID3V1_TAG_SIZE)
#define ID3V12_TAG_EXTRA_SIZE (ID3V12_TAG_SIZE - ID3V1_TAG_SIZE)

/***
 * trim_trailing_chunk
 *
 * Trims a chunk of size @size from the end of crop area of @file
 * if the chunk's first @hdr_sz bytes are equal to the ones @hdr is
 * pointing to.
 *
 * Returns 0 if the chunk has been trimmed, or -ENOENT otherwise.
 */

static int trim_trailing_chunk(struct file *file, const char *hdr,
                               size_t hdr_sz, size_t size)
{
    char buf[4];

    assert(hdr_sz <= sizeof(buf));

    if (file->crop.start + size <= file->crop.end)
    {
        lseek(file->fd, file->crop.end - size, SEEK_SET);
        read(file->fd, buf, hdr_sz);

        if (!memcmp(buf, hdr, hdr_sz))
        {
            file->crop.end -= size;
            return 0;
        }
    }

    return -ENOENT;
}

/***
 * trim_id3v1_tag - trim ID3v1 tag from @file
 *
 * Trims an ID3v1 tag of the specified version @minor from the end of crop
 * area of @file.
 *
 * Returns size of the tag trimmed, or -ENOENT if the tag has not been found.
 */

int trim_id3v1_tag(struct file *file, unsigned minor)
{
    if (minor == 0 || minor == 1 || minor == 2 || minor == 3 ||
        minor == ID3V1E_MINOR || minor == NOT_SET)
    {
        off_t orig_crop_end = file->crop.end;

        if (trim_trailing_chunk(file, ID3V1_HEADER, ID3V1_HEADER_SIZE,
                                ID3V1_TAG_SIZE) == 0)
        {
            print(OS_INFO, "ID3v1 tag found");

            /* check presence of v1 enhanced tag */
            if ((minor == ID3V1E_MINOR || minor == NOT_SET) &&
                trim_trailing_chunk(file, ID3V1E_HEADER, ID3V1E_HEADER_SIZE,
                                    ID3V1E_TAG_EXTRA_SIZE) == 0)
            {
                print(OS_INFO, "ID3v1 enhanced tag found");
                return ID3V1E_TAG_SIZE;
            }

            /* check presence of tag v1.2 */
            if ((minor == 2 || minor == NOT_SET) &&
                trim_trailing_chunk(file, ID3V12_HEADER, ID3V12_HEADER_SIZE,
                                    ID3V12_TAG_EXTRA_SIZE) == 0)
            {
                print(OS_INFO, "ID3v1.2 tag found");
                return ID3V12_TAG_SIZE;
            }

            if (minor == 0 || minor == 1 || minor == 3 || minor == NOT_SET)
                return ID3V1_TAG_SIZE;

            /* tag of the specified version has not been found */
            file->crop.end = orig_crop_end;
        }
    }

    return -ENOENT;
}

int trim_id3v2_tag(struct file *file, unsigned minor)
{
    struct id3v2_header hdr;
    int ret;
    int bigret = -ENOENT;

    if (file->crop.start + ID3V2_HEADER_LEN < file->crop.end)
    {
        /* check presence of an id3v2 header at the very beginning
         * of the crop area */
        lseek(file->fd, file->crop.start, SEEK_SET);
        ret = read_id3v2_header(file->fd, &hdr);

        if (ret == 0)
        {
            if (minor == hdr.version || minor == NOT_SET)
            {
                print(OS_INFO, "ID3v2.%u tag found", hdr.version);
                file->crop.start += ID3V2_HEADER_LEN + hdr.size;
                if (hdr.version == 4 && hdr.flags & ID3V2_FLAG_FOOTER_PRESENT)
                    file->crop.start += ID3V2_FOOTER_LEN;
                bigret = 0;
            }
        }
        else if (ret == -EFAULT)
            return ret;
    }

    if (file->crop.end - ID3V2_FOOTER_LEN >= file->crop.start)
    {
        /* check presence of an id3v2 footer at the very end
         * of the crop area */
        lseek(file->fd, file->crop.end - ID3V2_FOOTER_LEN, SEEK_SET);
        ret = read_id3v2_footer(file->fd, &hdr);

        if (ret == 0)
        {
            if (minor == hdr.version || minor == NOT_SET)
            {
                print(OS_INFO, "ID3v2.%u appended tag found", hdr.version);
                file->crop.end -=
                    ID3V2_HEADER_LEN + ID3V2_FOOTER_LEN + hdr.size;
                bigret = 0;
            }
        }
        else if (ret == -EFAULT)
            return ret;
    }

    return bigret;
}
