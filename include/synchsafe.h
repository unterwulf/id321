#ifndef SYNCHSAFE_H
#define SYNCHSAFE_H

#include <inttypes.h>
#include <unistd.h>

typedef uint32_t ss_uint32_t;

uint32_t deunsync_uint32(ss_uint32_t src);
ss_uint32_t unsync_uint32(uint32_t src);

size_t deunsync_buf(char *buf, size_t size, int pre);
size_t unsync_buf(char *dst, size_t dstsize, const char *src, size_t srcsize);

#endif /* SYNCHSAFE_H */
