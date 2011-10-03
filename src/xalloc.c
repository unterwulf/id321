#include <stdlib.h>
#include "common.h"

void *xmalloc(size_t sz)
{
    void *buf = malloc(sz);

    if (!buf)
        fatal("can't allocate %lu bytes of memory", (unsigned long)sz);

    return buf;
}

void *xcalloc(size_t nmemb, size_t sz)
{
    void *buf = calloc(nmemb, sz);

    if (!buf)
        fatal("can't allocate %lu bytes of memory",
              (unsigned long)(nmemb * sz));

    return buf;
}

void *xrealloc(void *ptr, size_t sz)
{
    void *buf = realloc(ptr, sz);

    if (!buf)
        fatal("can't allocate %lu bytes of memory", (unsigned long)sz);

    return buf;
}
