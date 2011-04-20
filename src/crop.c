#include <errno.h>
#include <unistd.h> /* SEEK_* */
#include <string.h>
#include "id3v2.h"
#include "id3v1.h"
#include "output.h"
#include "params.h" /* NOT_SET */
#include "crop.h"
#include "file.h"

/***
 * crop_id3v1_tag - crop ID3v1 tag from @file
 *
 * Returns size of the tag cropped or negative errno on error.
 */

int crop_id3v1_tag(struct file *file, unsigned minor)
{
    char buf[4];

    if (minor == 0 || minor == 1 || minor == 3 || minor == NOT_SET)
    {
        if (file->crop.start + ID3V1_TAG_SIZE > file->crop.end)
            return -ENOENT;

        lseek(file->fd, file->crop.end - ID3V1_TAG_SIZE, SEEK_SET);
        read(file->fd, buf, ID3V1_HEADER_SIZE);

        if (!memcmp(buf, ID3V1_HEADER, ID3V1_HEADER_SIZE))
        {
            print(OS_INFO, "ID3v1 tag found");
            file->crop.end -= ID3V1_TAG_SIZE;
        }
        else
            return -ENOENT;
    }

    /* check presence of v1 enhanced tag */
    if ((minor == ID3V1E_MINOR || minor == NOT_SET) &&
        file->crop.end - (ID3V1E_TAG_SIZE - ID3V1_TAG_SIZE) >= file->crop.start)
    {
        lseek(file->fd, file->crop.end - (ID3V1E_TAG_SIZE - ID3V1_TAG_SIZE),
              SEEK_SET);
        read(file->fd, buf, ID3V1E_HEADER_SIZE);

        if (!memcmp(buf, ID3V1E_HEADER, ID3V1E_HEADER_SIZE))
        {
            print(OS_INFO, "ID3v1 enhanced tag found");
            file->crop.end -= ID3V1E_TAG_SIZE - ID3V1_TAG_SIZE;
            return ID3V1E_TAG_SIZE;
        }
    }

    /* check presence of tag v1.2 */
    if ((minor == 2 || minor == NOT_SET) &&
        file->crop.end - (ID3V12_TAG_SIZE - ID3V1_TAG_SIZE) >= file->crop.start)
    {
        lseek(file->fd, file->crop.end - (ID3V12_TAG_SIZE - ID3V1_TAG_SIZE),
              SEEK_SET);
        read(file->fd, buf, ID3V12_HEADER_SIZE);

        if (!memcmp(buf, ID3V12_HEADER, ID3V12_HEADER_SIZE))
        {
            print(OS_INFO, "ID3v1.2 tag found");
            file->crop.end -= ID3V12_TAG_SIZE - ID3V1_TAG_SIZE;
            return ID3V12_TAG_SIZE;
        }
    }

    return ID3V1_TAG_SIZE;
}

int crop_id3v2_tag(struct file *file, unsigned minor)
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
