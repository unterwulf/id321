#ifndef COMPAT_H
#define COMPAT_H

#include "config.h"

#ifndef HAVE_STRNLEN
#include <string.h>
size_t strnlen(const char *s, size_t maxlen);
#endif

#ifndef HAVE_WCSDUP
#include <wchar.h>
wchar_t *wcsdup(const wchar_t *wcs);
#endif

#endif /* COMPAT_H */
