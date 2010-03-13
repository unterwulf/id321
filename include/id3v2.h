#ifndef ID3V2_H
#define ID3V2_H

#include <inttypes.h>
#include <unistd.h>

struct id3v2_header
{
    uint8_t   version;
    uint8_t   revision;
    uint8_t   flags;
    uint32_t  size;
};

struct id3v2_ext_header
{
    uint32_t  size;   /* can never has a size of fewer than six bytes */
    uint8_t   number; /* 01 */
    uint8_t   flags;
};

struct id3v2_frame
{
    char      id[4];
    uint32_t  size;
    uint8_t   status_flags;
    uint8_t   format_flags;
    char     *data;
};

struct id3v2_frame_list
{
    struct id3v2_frame       frame;
    struct id3v2_frame_list *prev;
    struct id3v2_frame_list *next;
};

struct id3v2_tag
{
    struct id3v2_header     header;
    struct id3v2_ext_header ext_header;
    struct id3v2_frame_list frame_head;
};

typedef void (* id3_frame_handler_t)(const struct id3v2_frame *);

typedef struct id3_frame_handler_table_t
{
    const char          *id;
    id3_frame_handler_t  print;
    const char          *desc;
} id3_frame_handler_table_t;

#define ID3V2_HEADER_LEN            10
#define ID3V2_FOOTER_LEN            10
#define ID3V2_FRAME_HEADER_SIZE     10
#define ID3V22_FRAME_HEADER_SIZE    6

#define ID3V22_FLAG_MASK            0xC0
#define ID3V23_FLAG_MASK            0xE0
#define ID3V24_FLAG_MASK            0xF0

#define ID3V22_FLAG_COMPRESS        0x40

#define ID3V2_FLAG_UNSYNC           0x80
#define ID3V2_FLAG_EXT_HEADER       0x40
#define ID3V2_FLAG_EXPERIMANTAL_IND 0x20
#define ID3V2_FLAG_FOOTER_PRESENT   0x10

#define ID3V2_EXT_FLAG_UPDATE       0x40
#define ID3V2_EXT_FLAG_CRC          0x20
#define ID3V2_EXT_FLAG_RESTRICT     0x10

#define ID3V23_FRM_STATUS_FLAG_TAG_ALT  0x80
#define ID3V23_FRM_STATUS_FLAG_FILE_ALT 0x40
#define ID3V23_FRM_STATUS_FLAG_RO       0x20

#define ID3V23_FRM_FMT_FLAG_CMP     0x80
#define ID3V23_FRM_FMT_FLAG_ENC     0x40
#define ID3V23_FRM_FMT_FLAG_GRP     0x20

#define ID3V24_FRM_STATUS_FLAG_TAG_ALT  0x40
#define ID3V24_FRM_STATUS_FLAG_FILE_ALT 0x20
#define ID3V24_FRM_STATUS_FLAG_RO       0x10

#define ID3V24_FRM_FMT_FLAG_GRP     0x40
#define ID3V24_FRM_FMT_FLAG_CMP     0x08
#define ID3V24_FRM_FMT_FLAG_ENC     0x04
#define ID3V24_FRM_FMT_FLAG_UNSYNC  0x02
#define ID3V24_FRM_FMT_FLAG_LEN     0x01

#define ID3V22_STR_ISO88591         0
#define ID3V22_STR_UCS2             1

#define ID3V23_STR_ISO88591         0
#define ID3V23_STR_UCS2             1

#define ID3V24_STR_ISO88591         0
#define ID3V24_STR_UTF16            1
#define ID3V24_STR_UTF16BE          2
#define ID3V24_STR_UTF8             3

struct id3v2_tag *new_id3v2_tag();
void free_id3v2_tag(struct id3v2_tag *tag);

int read_id3v2_header(int fd, struct id3v2_header *hdr);
int read_id3v2_footer(int fd, struct id3v2_header *hdr);
int unpack_id3v2_header(struct id3v2_header *hdr, const char *buf);
int read_id3v2_ext_header(int fd, struct id3v2_tag *tag);
int read_id3v2_frames(int fd, struct id3v2_tag *tag);

int append_id3v2_tag_frame(struct id3v2_tag *tag,
                           const struct id3v2_frame *frame);
int update_id3v2_tag_frame(struct id3v2_tag *tag,
                           const struct id3v2_frame *frame);

ssize_t pack_id3v2_tag(const struct id3v2_tag *tag, char **buf);

const char *map_v22_to_v24(const char *v23frame);
const char *map_v23_to_v24(const char *v23frame);

#endif /* ID3V2_H */
