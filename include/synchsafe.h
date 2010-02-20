#ifndef SYNCHSAFE_H
#define SYNCHSAFE_H

#include <inttypes.h>
#include <unistd.h>

uint32_t deunsync_uint32(uint32_t src);
uint32_t unsync_uint32(uint32_t src);
uint16_t deunsync_buf(char *buf, uint16_t size, int pre);
uint16_t unsync_buf(char *buf, uint16_t size);
ssize_t read_unsync(int fd, void *buf, size_t size, int pre);

#endif /* SYNCHSAFE_H */
