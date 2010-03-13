#ifndef FILE_H
#define FILE_H

#include <fcntl.h>

struct crop_area
{
    off_t start;
    off_t end;
};

struct file
{
    int fd;
    struct crop_area crop;
    off_t size;
};

struct file *open_file(const char *filename, mode_t mode);
int close_file(struct file *file);
int shift_file_payload(struct file *file, off_t delta);

#endif /* FILE_H */
