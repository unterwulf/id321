#include "id3v2.h"
#include "frames.h"
#include "output.h"

void dump_id3_header(const struct id3v2_header *hdr)
{
    print(OS_DEBUG, "id3v2 tag version: %u.%u, flags: %u, size: %u",
        hdr->version, hdr->revision, hdr->flags, hdr->size);
}

void dump_frame(const struct id3v2_frame *frame)
{
    print(OS_DEBUG, "frame %.4s, len=%u", frame->id, frame->size);
}
