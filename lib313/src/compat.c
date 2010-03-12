/*
 * compat.c -- lib313 compatibility stuff implementation
 *
 * lib313: a C library for packing/unpacking id3v1.3 tags
 * Copyright (c) 2009 Vitaly Sinilin <vs@kp4.ru>
 *
 */

#include "config.h"

#ifndef HAVE_STRNLEN
#include <string.h>

size_t strnlen(const char *s, size_t maxlen)
{
    char *pos = memchr(s, '\0', maxlen);
    return pos ? (pos - s) : maxlen;
}

#endif
