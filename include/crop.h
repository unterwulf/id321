#ifndef CROP_H
#define CROP_H

#include "file.h"

int crop_id3v1_tag(struct file *file, unsigned minor);
int crop_id3v2_tag(struct file *file, unsigned minor);

#endif /* CROP_H */
