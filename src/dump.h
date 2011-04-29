#ifndef DUMP_H
#define DUMP_H

#include "id3v2.h"

void dump_id3_header(const struct id3v2_header *hdr);
void dump_frame(const struct id3v2_frame *frame);

#endif /* DUMP_H */
