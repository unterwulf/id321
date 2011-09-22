#ifndef U16_CHAR_H
#define U16_CHAR_H

#include <inttypes.h>

typedef uint16_t u16_char;

#define U16_CHAR(c) ((u16_char) (c))

void *u16_memchr(const void *s, u16_char c, size_t sz);

#endif /* U16_CHAR_H */
