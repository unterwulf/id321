/*
 * compat.h -- lib313 compatibility stuff definition
 *
 * lib313: a C library for packing/unpacking id3v1.3 tags
 * Copyright (c) 2009 Vitaly Sinilin <vs@kp4.ru>
 *
 */

#ifndef _COMPAT_H_
#define _COMPAT_H_
#include "config.h"

#ifndef HAVE_STRNLEN
#include <string.h>
size_t strnlen(const char *s, size_t maxlen);
#endif

#endif /* _COMPAT_H_ */
