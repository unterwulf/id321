#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include "alias.h"
#include "common.h"
#include "id3v1_genres.h" /* ID3V1_UNKNOWN_GENRE */
#include "id3v2.h"
#include "params.h"
#include "textframe.h"
#include "u32_char.h"

int get_id3v2_tag_genre(const struct id3v2_tag *tag, u32_char **genre_ustr)
{
    u32_char *uptr;
    u32_char *udata;
    size_t usize;
    int genre_id = ID3V1_UNKNOWN_GENRE;
    long long_genre_id = -1;
    int ret;

    ret = get_text_frame_data_by_alias(tag, 'g', &udata, &usize);

    if (ret != 0)
        return ret;

    uptr = udata;

    switch (tag->header.version)
    {
        case 4:
            errno = 0;
            long_genre_id = u32_strtol(udata, &uptr, 10);
            if (errno != 0)
                long_genre_id = -1;
            break;

        case 3:
        case 2:
            if (udata[0] == U32_CHAR('('))
            {
                errno = 0;
                long_genre_id = u32_strtol(udata + 1, &uptr, 10);
                if (errno != 0)
                    long_genre_id = -1;

                if (*uptr != U32_CHAR(')'))
                {
                    uptr = udata;
                    long_genre_id = -1;
                }
                else
                    uptr++;
            }
            break;
    }

    if (long_genre_id >= 0 && long_genre_id <= 255)
        genre_id = (int)long_genre_id;

    if ((uptr < (udata + usize)) && uptr[1] != U32_CHAR('\0') && genre_ustr)
        *genre_ustr = u32_xstrdup(uptr);
    else
        *genre_ustr = NULL;

    free(udata);

    return genre_id;
}

static int set_id3v2_tag_genre_raw(struct id3v2_tag *tag, const u32_char *ustr,
                                   size_t usize)
{
    const char *frame_id = get_frame_id_by_alias('g', tag->header.version);
    return update_id3v2_tag_text_frame(tag, frame_id, U32_CHAR_CODESET,
                                       (const char *)ustr,
                                       usize * sizeof(u32_char));
}

int set_id3v2_tag_genre(struct id3v2_tag *tag, uint8_t genre_id,
                        const u32_char *genre_ustr)
{
    int ret = 0;

    if (genre_id != ID3V1_UNKNOWN_GENRE)
    {
        u32_char *udata;
        int       usize;

        if (!genre_ustr)
            genre_ustr = U32_EMPTY_STR;

        switch (tag->header.version)
        {
            case 4:
                usize = u32_snprintf_alloc(&udata, "%u%lc%ls",
                                           genre_id, U32_CHAR('\0'),
                                           genre_ustr);

                if (usize > 0 && genre_ustr[0] == U32_CHAR('\0'))
                    usize--; /* no need to have the separator */
                break;

            default:
            case 2:
            case 3:
                usize = u32_snprintf_alloc(&udata, "(%u)%ls",
                                           genre_id, genre_ustr);
        }

        ret = set_id3v2_tag_genre_raw(tag, udata, usize);
        free(udata);
    }
    else if (!IS_EMPTY_STR(genre_ustr))
    {
        ret = set_id3v2_tag_genre_raw(tag, genre_ustr, u32_strlen(genre_ustr));
    }

    return ret;
}
