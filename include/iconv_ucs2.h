#ifndef ICONV_UCS2_H
#define ICONV_UCS2_H

#include "config.h"

typedef void *iconv_ucs2_t;

iconv_ucs2_t iconv_open_ucs2(const char *tocode, const char *fromcode);

size_t iconv_ucs2(iconv_ucs2_t cd,
                  ICONV_CONST char **inbuf, size_t *inbytesleft,
                  char **outbuf, size_t *outbytesleft);

int iconv_close_ucs2(iconv_ucs2_t cd);

#endif /* ICONV_UCS2_H */
