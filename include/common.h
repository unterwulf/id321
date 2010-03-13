#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <iconv.h>
#include "id3v1.h"
#include "id3v2.h"
#include "params.h"

#define BLOCK_SIZE 4096

#define READORDIE(fd, buf, size) \
    if (readordie(fd, buf, size) != (ssize_t)(size)) \
        return -1;

#define for_each(i, array) \
    for (i = 0; i < sizeof(array)/sizeof(array[0]); i++)

int get_tags(const char *filename, struct version ver,
             struct id3v1_tag **tag1, struct id3v2_tag **tag2);

int write_tags(const char *filename, const struct id3v1_tag *tag1,
               const struct id3v2_tag *tag2);

ssize_t readordie(int fd, void *buf, size_t len);
ssize_t writeordie(int fd, const void *buf, size_t count);
const char *locale_encoding();
int str_to_long(const char *nptr, long *ret);

iconv_t xiconv_open(const char *tocode, const char *fromcode);
char *iconv_buf(const char *tocode, const char *fromcode,
                size_t size, const char *str, size_t *retsize);

#endif /* COMMON_H */
