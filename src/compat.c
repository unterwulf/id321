#include "config.h"

#ifndef HAVE_STRNLEN

#include <string.h>

size_t strnlen(const char *s, size_t maxlen)
{
    char *pos = memchr(s, '\0', maxlen);
    return pos ? (pos - s) : maxlen;
}

#endif
