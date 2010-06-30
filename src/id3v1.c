#include <errno.h>
#include <stddef.h>   /* size_t */
#include <string.h>
#include <inttypes.h>
#include <lib313.h>
#include "id3v1.h"

#define ID3V1X_TIT_SIZE(tag) ID3V1_TIT_SIZE
#define ID3V1X_ART_SIZE(tag) ID3V1_ART_SIZE
#define ID3V1X_ALB_SIZE(tag) ID3V1_ALB_SIZE
#define ID3V1X_YER_SIZE(tag) ID3V1_YER_SIZE
#define ID3V1X_COM_SIZE(tag) \
    (tag->version != 0 && tag->track != '\0' ? ID3V11_COM_SIZE : ID3V1_COM_SIZE)


#define unpack_id3v1_field(tag, buf, field, FLD) \
    strncpy(tag->field, buf + ID3V1_##FLD##_OFF, ID3V1_##FLD##_SIZE);

#define pack_id3v1_field(buf, tag, field, FLD) \
    strncpy(buf + ID3V1_##FLD##_OFF, tag->field, ID3V1X_##FLD##_SIZE(tag));

#define unpack_id3v12_field(tag, buf, field, FLD) \
    strncpy(tag->field + ID3V1X_##FLD##_SIZE(tag), \
            buf + ID3V12_##FLD##_OFF, ID3V12_##FLD##_SIZE);

#define pack_id3v12_field(buf, tag, field, FLD) \
    strncpy(buf + ID3V12_##FLD##_OFF, \
            tag->field + ID3V1X_##FLD##_SIZE(tag), \
            ID3V12_##FLD##_SIZE);

#define unpack_id3v1e_field(tag, buf, field, FLD) \
    strncpy(tag->field + ID3V1X_##FLD##_SIZE(tag), \
            buf + ID3V1E_##FLD##_OFF, ID3V1E_##FLD##_SIZE);

#define pack_id3v1e_field(buf, tag, field, FLD) \
    strncpy(buf + ID3V1E_##FLD##_OFF, \
            tag->field + ID3V1X_##FLD##_SIZE(tag), \
            ID3V1E_##FLD##_SIZE);


int unpack_id3v1_tag(struct id3v1_tag *tag, const char *buf, size_t size)
{
    if (size < ID3V1_TAG_SIZE)
        return -E2BIG;

    /* we will find a tag at the end of the buffer */
    buf += size - ID3V1_TAG_SIZE;

    if (!memcmp(buf, ID3V1_HEADER, ID3V1_HEADER_SIZE))
    {
        unpack_id3v1_field(tag, buf, title,   TIT);
        unpack_id3v1_field(tag, buf, artist,  ART);
        unpack_id3v1_field(tag, buf, album,   ALB);
        unpack_id3v1_field(tag, buf, year,    YER);
        unpack_id3v1_field(tag, buf, comment, COM);
        tag->genre_id = buf[ID3V1_GEN_OFF];

        if (buf[ID3V11_TRK_IND_OFF] == '\0' && buf[ID3V11_TRK_OFF] != '\0')
        {
            tag->version = 1;
            tag->track = buf[ID3V11_TRK_OFF];
        }
        else
            tag->version = 0;
    }
    else
        return -ENOENT;

    return 0;
}

int unpack_id3v13_tag(struct id3v1_tag *tag, const char *buf, size_t size)
{
    struct lib313_tag tag313;

    if (size < ID3V1_TAG_SIZE)
        return -E2BIG;

    /* we will find a tag at the end of the buffer */
    buf += size - ID3V1_TAG_SIZE;

    if (lib313_unpack(&tag313, buf) != LIB313_SUCCESS)
        return -ENOENT;

    memset(tag, '\0', sizeof(*tag));

    tag->version = tag313.version;
    strncpy(tag->title,   tag313.title,   ID3V13_MAX_TIT_SIZE);
    strncpy(tag->artist,  tag313.artist,  ID3V13_MAX_ART_SIZE);
    strncpy(tag->album,   tag313.album,   ID3V13_MAX_ALB_SIZE);
    strncpy(tag->year,    tag313.year,    ID3V1_YER_SIZE);
    strncpy(tag->comment, tag313.comment, ID3V13_MAX_COM_SIZE);
    tag->track = tag313.track;
    tag->genre_id = tag313.genre;

    return 0;
}

int unpack_id3v1_enh_tag(struct id3v1_tag *tag, const char *buf, size_t size)
{
    const char *legacy;

    if (size < ID3V1E_TAG_SIZE)
        return -E2BIG;
    
    /* we will find a tag at the end of the buffer */
    buf += size - ID3V1E_TAG_SIZE;
    legacy = buf + ID3V1E_TAG_SIZE - ID3V1_TAG_SIZE;

    if (!memcmp(buf, ID3V1E_HEADER, ID3V1E_HEADER_SIZE)
        && !memcmp(legacy, ID3V1_HEADER, ID3V1_HEADER_SIZE))
    {
        (void)unpack_id3v1_tag(tag, legacy, ID3V1_TAG_SIZE);

        unpack_id3v1e_field(tag, buf, title,  TIT);
        unpack_id3v1e_field(tag, buf, artist, ART);
        unpack_id3v1e_field(tag, buf, album,  ALB);

        tag->speed = buf[ID3V1E_SPD_OFF];
        strncpy(tag->genre_str, buf + ID3V1E_GN2_OFF, ID3V1E_GN2_SIZE);
        strncpy(tag->starttime, buf + ID3V1E_STM_OFF, ID3V1E_STM_SIZE);
        strncpy(tag->endtime, buf + ID3V1E_ETM_OFF, ID3V1E_ETM_SIZE);

        tag->version = ID3V1E_MINOR;
    }
    else
        return -ENOENT;

    return 0;
}

int unpack_id3v12_tag(struct id3v1_tag *tag, const char *buf, size_t size)
{
    const char *legacy;

    if (size < ID3V12_TAG_SIZE)
        return -E2BIG;
    
    /* we will find a tag at the end of the buffer */
    buf += size - ID3V12_TAG_SIZE;
    legacy = buf + ID3V12_TAG_SIZE - ID3V1_TAG_SIZE;

    if (!memcmp(buf, ID3V12_HEADER, ID3V12_HEADER_SIZE)
        && !memcmp(legacy, ID3V1_HEADER, ID3V1_HEADER_SIZE))
    {
        (void)unpack_id3v1_tag(tag, legacy, ID3V1_TAG_SIZE);

        unpack_id3v12_field(tag, buf, title,   TIT);
        unpack_id3v12_field(tag, buf, artist,  ART);
        unpack_id3v12_field(tag, buf, album,   ALB);
        unpack_id3v12_field(tag, buf, comment, COM);
        strncpy(tag->genre_str, buf + ID3V12_GN2_OFF, ID3V12_GN2_SIZE);

        tag->version = 2;
    }
    else
        return -ENOENT;

    return 0;
}

size_t pack_id3v1_tag(char *buf, const struct id3v1_tag *tag)
{
    size_t  size = ID3V1_TAG_SIZE;
    char   *pos = buf;

    memset((void *)buf, '\0', ID3V1E_TAG_SIZE);

    if (tag->version == 3)
    {
        struct lib313_tag tag313;

        strncpy(tag313.title,   tag->title,   ID3V13_MAX_TIT_SIZE);
        strncpy(tag313.artist,  tag->artist,  ID3V13_MAX_ART_SIZE);
        strncpy(tag313.album,   tag->album,   ID3V13_MAX_ALB_SIZE);
        strncpy(tag313.year,    tag->year,    ID3V1_YER_SIZE);
        strncpy(tag313.comment, tag->comment, ID3V13_MAX_COM_SIZE);
        tag313.track = tag->track;
        tag313.genre = tag->genre_id;

        lib313_pack(buf, &tag313);
    }
    else if (tag->version == 2)
    {
        size = ID3V12_TAG_SIZE;
        strcpy(pos, ID3V12_HEADER);
        pack_id3v12_field(pos, tag, title,   TIT);
        pack_id3v12_field(pos, tag, artist,  ART);
        pack_id3v12_field(pos, tag, album,   ALB);
        pack_id3v12_field(pos, tag, comment, COM);
        strncpy(pos + ID3V12_GN2_OFF, tag->genre_str, ID3V12_GN2_SIZE);
        pos += ID3V12_TAG_SIZE - ID3V1_TAG_SIZE; 
    }
    else if (tag->version == ID3V1E_MINOR)
    {
        size = ID3V1E_TAG_SIZE;
        strcpy(pos, ID3V1E_HEADER);
        pack_id3v1e_field(pos, tag, title,   TIT);
        pack_id3v1e_field(pos, tag, artist,  ART);
        pack_id3v1e_field(pos, tag, album,   ALB);
        pos[ID3V1E_SPD_OFF] = tag->speed;
        strncpy(pos + ID3V1E_GN2_OFF, tag->genre_str, ID3V1E_GN2_SIZE);
        strncpy(pos + ID3V1E_STM_OFF, tag->starttime, ID3V1E_STM_SIZE);
        strncpy(pos + ID3V1E_ETM_OFF, tag->endtime, ID3V1E_ETM_SIZE);
        pos += ID3V1E_TAG_SIZE - ID3V1_TAG_SIZE; 
    }

    /* all versions but 3rd have the same format for the last 128 bytes */
    if (tag->version != 3)
    {
        strcpy(pos, ID3V1_HEADER);
        pack_id3v1_field(pos, tag, title,   TIT);
        pack_id3v1_field(pos, tag, artist,  ART);
        pack_id3v1_field(pos, tag, album,   ALB);
        pack_id3v1_field(pos, tag, year,    YER);
        pack_id3v1_field(pos, tag, comment, COM);
        pos[ID3V1_GEN_OFF] = tag->genre_id;

        if (tag->version != 0 && tag->track != '\0')
        {
            pos[ID3V11_TRK_IND_OFF] = '\0';
            pos[ID3V11_TRK_OFF] = tag->track;
        }
    }

    return size;
}
