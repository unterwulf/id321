#include <errno.h>     /* EILSEQ */
#include <stdlib.h>    /* free() */
#include "id3v2.h"     /* struct id3v2_tag */
#include "textframe.h" /* get_text_frame_data_by_alias() */
#include "u32_char.h"  /* u32_char, u32_strtol() */

int get_id3v2_tag_trackno(const struct id3v2_tag *tag)
{
    u32_char *u32_data;
    long trackno;
    int ret;

    ret = get_text_frame_data_by_alias(tag, 'n', &u32_data, NULL);

    if (ret != 0)
        return ret;

    errno = 0;
    trackno = u32_strtol(u32_data, NULL, 10);
    free(u32_data);

    if (errno == 0 && trackno >= 0 && trackno <= 255)
        return (int)trackno;

    return -EILSEQ;
}
