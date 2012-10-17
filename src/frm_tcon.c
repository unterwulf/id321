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

int get_id3v2_tag_genre(const struct id3v2_tag *tag, u32_char **genre_u32_str)
{
    u32_char *ptr;
    u32_char *u32_data;
    size_t u32_size;
    int genre_id = ID3V1_UNKNOWN_GENRE;
    long long_genre_id = -1;
    int ret;

    ret = get_text_frame_data_by_alias(tag, 'g', &u32_data, &u32_size);

    if (ret != 0)
        return ret;

    ptr = u32_data;

    switch (tag->header.version)
    {
        case 4:
            errno = 0;
            long_genre_id = u32_strtol(u32_data, &ptr, 10);
            if (errno != 0)
                long_genre_id = -1;
            break;

        case 3:
        case 2:
            if (u32_data[0] == U32_CHAR('('))
            {
                errno = 0;
                long_genre_id = u32_strtol(u32_data + 1, &ptr, 10);
                if (errno != 0)
                    long_genre_id = -1;

                if (*ptr != U32_CHAR(')'))
                {
                    ptr = u32_data;
                    long_genre_id = -1;
                }
                else
                    ptr++;
            }
            break;
    }

    if (long_genre_id >= 0 && long_genre_id <= 255)
        genre_id = (int)long_genre_id;

    if (ptr < u32_data + u32_size && ptr[1] != U32_CHAR('\0') && genre_u32_str)
        *genre_u32_str = u32_xstrdup(ptr);
    else
        *genre_u32_str = NULL;

    free(u32_data);

    return genre_id;
}

void set_id3v2_tag_genre(struct id3v2_tag *tag, uint8_t genre_id,
                         u32_char *genre_u32_str)
{
    const char *frame_id = get_frame_id_by_alias('g', tag->header.version);
    u32_char   *u32_data;
    int         u32_size;

    if (genre_id != ID3V1_UNKNOWN_GENRE)
    {
        if (!genre_u32_str)
            genre_u32_str = U32_EMPTY_STR;

        switch (tag->header.version)
        {
            case 4:
                u32_size = u32_snprintf_alloc(&u32_data, "%u%lc%ls",
                               genre_id, U32_CHAR('\0'), genre_u32_str);

                if (u32_size > 0 && genre_u32_str[0] == U32_CHAR('\0'))
                    u32_size--; /* no need to have the separator */
                break;

            default:
            case 2:
            case 3:
                u32_size = u32_snprintf_alloc(&u32_data, "(%u)%ls",
                                              genre_id, genre_u32_str);
        }
    }
    else if (!IS_EMPTY_STR(genre_u32_str))
    {
        u32_data = genre_u32_str;
        u32_size = u32_strlen(genre_u32_str);
    }
    else
        return; /* senseless input parameters => do nothing */

    update_id3v2_tag_text_frame(
            tag, frame_id, U32_CHAR_CODESET,
            (char *)u32_data, u32_size * sizeof(u32_char));

    if (u32_data != genre_u32_str)
        free(u32_data);

    return;
}
