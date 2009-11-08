#ifndef DUMP_H
#define DUMP_H

#include "id3v2.h"

void dump_id3_header(struct id3v2_header *hdr);
void dump_frame(struct id3v2_frame *frame);

#endif /* DUMP_H */
