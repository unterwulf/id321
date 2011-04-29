/*
 * unpack.c -- lib313_unpack() implementation
 *
 * lib313: a C library for packing/unpacking id3v1.3 tags
 * Copyright (c) 2009 Vitaly Sinilin <vs@kp4.ru>
 *
 */

#include <string.h>
#include "lib313.h"
#include "lib313int.h"
#include "compat.h"

#define PARSE(fld, outtag, tag, src) \
    parse(LIB313_TAG_##fld(outtag), \
          &tag, &src[ID3V1_##fld##_OFF], LIB313_TAG_##fld##_SIZE(outtag))

#define COMPLETE(fld, outtag, tag, ptr, segcount) \
    if (LIB313_TAG_##fld(outtag)[LIB313_TAG_##fld##_SIZE(outtag) - 1] != '\0') \
    { \
        if (tag.mask & (0x80 >> segcount)) \
            ptr += append(LIB313_TAG_##fld(outtag), ptr, \
                          tag.seglen[segcount]); \
        segcount++; \
    }

static inline int readpair(char *dst1, char *dst2, const char *src, int size)
{
    int len1, len2;

    len1 = strnlen(src, size);
    memcpy(dst1, src, len1);
    len2 = size - len1 - 1;
    if (len2 > 0)
    {
        memcpy(dst2, src + len1 + 1, len2);
    }

    return len1;
}

static int parse(char *field, struct id3v13 *tag, const char *src, int size)
{
    int extlen = size - readpair(field, tag->ext + tag->extlen, src, size) - 1;

    if (extlen >= 0)
    {
        tag->extlen += extlen;

        /* we don't want to count 'year' field as terminated as it is not an
         * expandable field, so we exclude it in this silly way using size */
        if (size != ID3V1_YER_SIZE)
            tag->termcount++;
    }
}

static int append(char *dst, const char *src, int len)
{
    if (len == 0)
        /* last segment or a broken header */
        len = strlen(src);

    strncat(dst, src, len);

    return len;
}

static int unpack_header(struct id3v13 *tag)
{
    static const unsigned masks[] = { 0x0F, 0x1F, 0x3F };
    static const unsigned nibble_bits[] =
        { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
    int hdr_size = 1;

    if (tag->termcount >= ID3V13_EXT_FIELDS_COUNT - 1)
    {
        /* no explicit header in this case */
        tag->mask = 0x80;
        return 0;
    }

    tag->mask = tag->ext[0] & ~masks[tag->termcount];
    tag->segcount = nibble_bits[tag->mask >> 4];

    if (tag->segcount > 1)
    {
        tag->seglen[0] = tag->ext[0] & masks[tag->termcount];

        if (tag->segcount > 2)
        {
            tag->seglen[1] = tag->ext[1] & masks[tag->termcount];
            hdr_size++;
        }
    }

    return hdr_size;
}

int lib313_unpack(struct lib313_tag *outtag, const char *buf)
{
    struct id3v13  tag;
    char          *ptr;
    unsigned       segcount = 0;

    /* check input params */
    if (!outtag || !buf)
        return LIB313_ERR_INVALID;

    /* check tag signature */
    if (memcmp(buf, ID3V1_HEADER, ID3V1_HEADER_SIZE) != 0)
        return LIB313_ERR_BAD_TAG;

    /* initialize empty tags */
    memset((void *)outtag, '\0', sizeof(*outtag));
    memset((void *)&tag, '\0', sizeof(tag));

    outtag->genre = buf[ID3V1_GEN_OFF];

    /* check if tag contains track number (id3v1.1) */
    if (buf[ID3V11_TRK_IND_OFF] == '\0' && buf[ID3V11_TRK_OFF] != '\0')
    {
        outtag->version = 1;
        outtag->track = buf[ID3V11_TRK_OFF];
    }

    /* read id3v1 values and form the extension space, order makes sense */
    PARSE(COM, *outtag, tag, buf);
    PARSE(TIT, *outtag, tag, buf);
    PARSE(ALB, *outtag, tag, buf);
    PARSE(ART, *outtag, tag, buf);
    PARSE(YER, *outtag, tag, buf);

    ptr = &tag.ext[unpack_header(&tag)];

    /* don't care about segcount value because seglen has enough elements
     * and again order makes sense */
    COMPLETE(ART, *outtag, tag, ptr, segcount);
    COMPLETE(ALB, *outtag, tag, ptr, segcount);
    COMPLETE(TIT, *outtag, tag, ptr, segcount);
    COMPLETE(COM, *outtag, tag, ptr, segcount);

    if (segcount > 0)
        outtag->version = 3;

    return LIB313_SUCCESS;
}
