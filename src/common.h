#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>  /* size_t, ssize_t */
#include "iconv_wrap.h"
#include "id3v1.h"
#include "id3v2.h"
#include "params.h"
#include "u32_char.h"

/* both GNU and Solaris iconv() understand this codeset name */
#define ISO_8859_1_CODESET "ISO8859-1"

#define BLOCK_SIZE 4096

#define for_each(i, array) \
    for (i = 0; i < sizeof(array)/sizeof(array[0]); i++)

#define IS_EMPTY_STR(str) (!(str) || !str[0])

#define NOMEM_OR_FAULT(ret) (ret == -ENOMEM ? ret : -EFAULT)
#define SUCC_NOMEM_OR_FAULT(ret) (ret == 0 || ret == -ENOMEM ? ret : -EFAULT)

int get_tags(const char *filename, struct version ver,
             struct id3v1_tag **tag1, struct id3v2_tag **tag2);

int write_tags(const char *filename, const struct id3v1_tag *tag1,
               const struct id3v2_tag *tag2);

int readordie(int fd, void *buf, size_t len);
int writeordie(int fd, const void *buf, size_t len);
const char *locale_encoding();
int str_to_long(const char *nptr, long *ret);

id321_iconv_t xiconv_open(const char *tocode, const char *fromcode);

ssize_t iconvordie(const char *tocode, const char *fromcode,
                   const char *src, size_t srcsize,
                   char *dst, size_t dstsize);

int iconv_alloc(const char *tocode, const char *fromcode,
                const char *src, size_t srcsize,
                char **dst, size_t *dstsize);

int u32_snprintf_alloc(u32_char **u32_str, const char *fmt, ...);

#endif /* COMMON_H */
