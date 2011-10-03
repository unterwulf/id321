#ifndef XALLOC_H
#define XALLOC_H

#include <stddef.h>

void *xmalloc(size_t sz);
void *xcalloc(size_t nmemb, size_t sz);
void *xrealloc(void *ptr, size_t sz);

#endif /* XALLOC_H */
