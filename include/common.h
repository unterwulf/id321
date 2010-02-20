#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <iconv.h>

#define BLOCK_SIZE 4096

#define READORDIE(fd, buf, size) \
    if (readordie(fd, buf, size) != size) \
        return -1;

ssize_t readordie(int fd, void *buf, size_t len);
const char *locale_encoding();
iconv_t xiconv_open(const char *tocode, const char *fromcode);

#endif /* COMMON_H */
