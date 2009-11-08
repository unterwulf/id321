#include "id3v2.h"
#include "frames.h"
#include "output.h"

void dump_id3_header(struct id3v2_header *hdr)
{
    print(OS_DEBUG,
        "Id3 tag info:\n"
        "  version: %u.%u\n"
        "  flags: %u\n"
        "  size: %u",
        hdr->version, hdr->revision, hdr->flags, hdr->size);
}

void dump_frame(struct id3v2_frame *frame)
{
    print(PARSE_DEBUG,
        "frame %.4s, len=%u",
        frame->id, frame->size);
}
