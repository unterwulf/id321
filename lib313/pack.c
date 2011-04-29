/*
 * pack.c -- lib313_pack() implementation
 *
 * lib313: a C library for packing/unpacking id3v1.3 tags
 * Copyright (c) 2009 Vitaly Sinilin <vs@kp4.ru>
 *
 */

#include <string.h>
#include "lib313.h"
#include "lib313int.h"

#define STORE(fld, buf, tag, intag) \
    store(&buf[ID3V1_##fld##_OFF], tag, \
          LIB313_TAG_##fld##_SIZE(*intag), \
          LIB313_TAG_##fld(*intag))

#define FILL(fld, buf, str, tag, intag) \
    fill(&buf[ID3V1_##fld##_OFF], str, LIB313_TAG_##fld##_SIZE(*intag));

static int fill(char *dst, const char *src, int size)
{
    char *ptr = dst + size - 1;
    int   len;

    while (ptr > dst && *(ptr-1) == '\0')
        ptr--;

    ptr++;              /* leave one null-terminator */
    size -= ptr - dst;  /* size equals to available space now */
    len = strlen(src);  /* number of bytes to be written */

    if (len > size)
        len = size;

    memcpy(ptr, src, len);

    return len;
}

static inline void pack(
        char                    *buf,
        const struct id3v13     *tag,
        const struct lib313_tag *intag)
{
    const char *str = tag->ext;

    /* there can be one or two null-bytes at the very beginning of the
     * extension space buffer depending on header length */
    if (*str == '\0') str++;
    if (*str == '\0') str++;

    /* order makes sense */
    str += FILL(COM, buf, str, tag, intag);
    str += FILL(TIT, buf, str, tag, intag);
    str += FILL(ALB, buf, str, tag, intag);
    str += FILL(ART, buf, str, tag, intag);
    /*  */ FILL(YER, buf, str, tag, intag);
}

static void store(char *field, struct id3v13 *tag, int size, const char *value)
{
    int len = strlen(value);

    if (len <= size)
    {
        memcpy(field, value, size);
        if (len < size)
        {
            tag->extlen += size - len - 1;
            tag->termcount++;
        }
    }
    else
    {
        char *ext = &tag->ext[2];

        memcpy(field, value, size);
        tag->seglen[tag->segcount++] = len - size;
        tag->mask |= 0x80 >> (tag->totalcount - tag->termcount);
        strncat(ext, value + size, sizeof(tag->ext) - 2 - strlen(ext) - 1);
    }

    tag->totalcount++;
}

static void normalize(struct id3v13 *tag)
{
    static const int hdr_size[] = { 1, 1, 2 };
    int segnum;
    int avail = tag->extlen;

    for (segnum = 0; segnum < tag->segcount; segnum++)
    {
        if (tag->seglen[segnum] > avail - hdr_size[segnum])
        {
            tag->seglen[segnum] = avail - hdr_size[segnum];
            tag->mask &= ~(0xFF >> (segnum + 1));
            tag->segcount = segnum + 1;
            break;
        }
        avail -= tag->seglen[segnum];
    }

    /* according to id3v1.3 specification we must not store the length
     * of the last segment in the header; it is responsibility of the
     * caller to check that tag->segcount > 0, because otherwise this
     * function makes no sense */
    tag->seglen[tag->segcount - 1] = 0;
}

int lib313_pack(char *buf, const struct lib313_tag *intag)
{
    struct id3v13 tag;

    if (!intag || !buf)
        return LIB313_ERR_INVALID;

    memset((void *)&tag, '\0', sizeof(tag));
    memset((void *)buf, '\0', ID3V1_TAG_SIZE);
    strcpy(buf, ID3V1_HEADER);

    if (intag->track != 0)
    {
        buf[ID3V11_TRK_IND_OFF] = '\0';
        buf[ID3V11_TRK_OFF] = intag->track;
    }

    buf[ID3V1_GEN_OFF] = intag->genre;
    strncpy(&buf[ID3V1_YER_OFF], intag->year, ID3V1_YER_SIZE);
    if (strlen(intag->year) < ID3V1_YER_SIZE)
        tag.extlen += ID3V1_YER_SIZE - strlen(intag->year) - 1;

    /* order makes sense */
    STORE(ART, buf, &tag, intag);
    STORE(ALB, buf, &tag, intag);
    STORE(TIT, buf, &tag, intag);
    STORE(COM, buf, &tag, intag);

    /* check if we need a header */
    if (tag.termcount < 3 && tag.segcount > 0)
    {
        char *hdr_ptr = &tag.ext[1];
        normalize(&tag);

        if (tag.segcount == 3)
            *hdr_ptr-- = tag.seglen[1];

        *hdr_ptr = tag.mask | tag.seglen[0];
    }

    /* embed the extension space */
    pack(buf, &tag, intag);

    return LIB313_SUCCESS;
}
