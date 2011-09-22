#include <stddef.h>
#include "u16_char.h"

void *u16_memchr(const void *s, u16_char c, size_t sz)
{
    const u16_char *u16_s = (const u16_char *)s;

    for (; sz > 0; u16_s++, sz--)
        if (*u16_s == c)
            return (void *)u16_s;

    return NULL;
}
