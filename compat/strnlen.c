#include <string.h>

size_t id321_strnlen(const char *s, size_t maxlen)
{
    char *pos = memchr(s, '\0', maxlen);
    return pos ? (pos - s) : maxlen;
}
