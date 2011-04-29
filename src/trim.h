#ifndef TRIM_H
#define TRIM_H

#include "file.h"

int trim_id3v1_tag(struct file *file, unsigned minor);
int trim_id3v2_tag(struct file *file, unsigned minor);

#endif /* TRIM_H */
