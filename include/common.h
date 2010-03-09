#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <iconv.h>

#define BLOCK_SIZE 4096

#define READORDIE(fd, buf, size) \
    if (readordie(fd, buf, size) != size) \
        return -1;

#define for_each(i, array) \
    for (i = 0; i < sizeof(array)/sizeof(array[0]); i++)

ssize_t readordie(int fd, void *buf, size_t len);
const char *locale_encoding();
char *xstrdup(const char *s);
iconv_t xiconv_open(const char *tocode, const char *fromcode);

#endif /* COMMON_H */
