/*
 * lib313int.h -- lib313 internal definitions
 *
 * lib313: a C library for packing/unpacking id3v1.3 tags
 * Copyright (c) 2009 Vitaly Sinilin <vs@kp4.ru>
 *
 */

#ifndef _LIB313INT_H_
#define _LIB313INT_H_

#include "lib313.h"

#define ID3V13_EXT_FIELDS_COUNT   4

/* extension space segments order */
#define ID3V13_ART_ORD            0
#define ID3V13_ALB_ORD            1
#define ID3V13_TIT_ORD            2
#define ID3V13_COM_ORD            3

struct id3v13 {
    int           totalcount;
    int           termcount;
    int           segcount;
    unsigned char mask;
    int           seglen[ID3V13_EXT_FIELDS_COUNT];
    unsigned char ext[ID3V13_MAX_EXT_SIZE+3]; /* must be null-terminated */
    int           extlen;
};

#endif /* _LIB313INT_H_ */
