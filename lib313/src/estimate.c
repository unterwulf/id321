/*
 * estimate.c -- lib313_estimate() implementation
 *
 * lib313: a C library for packing/unpacking id3v1.3 tags
 * Copyright (c) 2009 Vitaly Sinilin <vs@kp4.ru>
 *
 */

#include <string.h>
#include "lib313.h"
#include "lib313int.h"

#define INIT_FIELD_SIZE(fld, fs, tag) \
    fs[ID3V13_##fld##_ORD].actual = strlen(LIB313_TAG_##fld(*tag)); \
    fs[ID3V13_##fld##_ORD].max = LIB313_TAG_##fld##_SIZE(*tag);

struct field_size
{
    int actual;
    int max;
};

int lib313_estimate(const struct lib313_tag *tag)
{
    struct field_size fs[ID3V13_EXT_FIELDS_COUNT];
    int seglen = 0;
    int termcount = 0;
    int segcount = 0;
    int extlen = 0;
    int i;

    INIT_FIELD_SIZE(ART, fs, tag);
    INIT_FIELD_SIZE(ALB, fs, tag);
    INIT_FIELD_SIZE(TIT, fs, tag);
    INIT_FIELD_SIZE(COM, fs, tag);

    if (strlen(tag->year) < ID3V1_YER_SIZE)
        extlen += ID3V1_YER_SIZE - strlen(tag->year) - 1;

    for (i = 0; i < ID3V13_EXT_FIELDS_COUNT; i++)
    {
        if (fs[i].actual < fs[i].max)
        {
            extlen += fs[i].max - fs[i].actual - 1;
            termcount++;
        }
        else if (fs[i].actual > fs[i].max)
        {
            seglen += fs[i].actual - fs[i].max;
            segcount++;
        }
    }

    return extlen - (seglen +
            (termcount < 3 && segcount != 0) + /* +1 byte if header needed */
            (segcount == 3));                  /* +1 byte for 2-byte header */
}
