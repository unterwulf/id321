#ifndef COMPAT_H
#define COMPAT_H

#include <config.h>

#ifndef HAVE_STRNLEN
#include <string.h>
size_t strnlen(const char *s, size_t maxlen);
#endif

#endif /* COMPAT_H */
