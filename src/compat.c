#include "config.h"

#ifndef HAVE_STRNLEN

#include <string.h>

size_t strnlen(const char *s, size_t maxlen)
{
    char *pos = memchr(s, '\0', maxlen);
    return pos ? (pos - s) : maxlen;
}

#endif

#ifndef HAVE_WCSDUP

#include <errno.h>
#include <stdlib.h>
#include <wchar.h>

wchar_t *wcsdup(const wchar_t *wcs)
{
    size_t len = wcslen(wcs);
    wchar_t dup = malloc((len + 1)*sizeof(wchar_t));

    if (!dup)
    {
        errno = ENOMEM;
        return NULL;
    }

    return wcscpy(dup, wcs);
}

#endif
