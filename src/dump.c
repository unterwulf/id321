#include "id3v2.h"
#include "output.h"

void dump_id3_header(const struct id3v2_header *hdr)
{
    print(OS_DEBUG, "ID3v2 tag version: %u.%u, flags: 0x%.2X, size: %u",
                    hdr->version, hdr->revision, hdr->flags, hdr->size);
}

void dump_frame(const struct id3v2_frame *frame)
{
    print(OS_DEBUG, "   frame %.*s: len=%u, first_byte=0x%.2X, status=0x%.2X,"
                    " format=0x%.2X",
                    ID3V2_FRAME_ID_MAX_SIZE, frame->id,
                    frame->size, frame->data[0],
                    frame->status_flags, frame->format_flags);
}
