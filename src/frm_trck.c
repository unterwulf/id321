#include <errno.h>     /* EILSEQ */
#include <stdlib.h>    /* free() */
#include "id3v2.h"     /* struct id3v2_tag */
#include "textframe.h" /* get_text_frame_data_by_alias() */
#include "u32_char.h"  /* u32_char, u32_strtol() */

int get_id3v2_tag_trackno(const struct id3v2_tag *tag)
{
    u32_char *udata;
    long trackno;
    int ret;

    ret = get_text_frame_data_by_alias(tag, 'n', &udata, NULL);

    if (ret != 0)
        return ret;

    errno = 0;
    trackno = u32_strtol(udata, NULL, 10);
    free(udata);

    if (errno != 0 || trackno < 0)
        return -EILSEQ;

    return (int)trackno;
}
