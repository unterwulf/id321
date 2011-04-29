#ifndef COMPAT_H
#define COMPAT_H

#include <config.h>

#ifndef HAVE_STRNLEN
#include <string.h>
#define strnlen id321_strnlen
size_t id321_strnlen(const char *s, size_t maxlen);
#endif

#endif /* COMPAT_H */
