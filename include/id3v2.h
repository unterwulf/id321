#ifndef ID3V2_H
#define ID3V2_H

#include <inttypes.h>
#include <sys/types.h>
#include "u32_char.h"

#ifdef _FRAME_LIST
#define PRIVATE(type, name) type name
#else
#define PRIVATE(type, name) type const name
#endif

#define ID3V2_HEADER_LEN            10
#define ID3V2_FOOTER_LEN            10
#define ID3V2_FRAME_HEADER_SIZE     10
#define ID3V22_FRAME_HEADER_SIZE    6
#define ID3V2_FRAME_ID_MAX_SIZE     4

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

#define ID3V22_STR_ISO88591         '\x00'
#define ID3V22_STR_UCS2             '\x01'

#define ID3V23_STR_ISO88591         '\x00'
#define ID3V23_STR_UCS2             '\x01'

#define ID3V24_STR_ISO88591         '\x00'
#define ID3V24_STR_UTF16            '\x01'
#define ID3V24_STR_UTF16BE          '\x02'
#define ID3V24_STR_UTF8             '\x03'

/* non standard value, for internal use only */
#define ID3V2_UNSUPPORTED_ENCODING  '\xFF'

#define ID3V2_ENC_HDR_SIZE          1
#define ID3V2_LANG_HDR_SIZE         3
#define ID3V2_FRM_COMM_HDR_SIZE    (ID3V2_ENC_HDR_SIZE + ID3V2_LANG_HDR_SIZE)

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
    PRIVATE(struct id3v2_frame *, prev);
    PRIVATE(struct id3v2_frame *, next);

    char      id[ID3V2_FRAME_ID_MAX_SIZE];
    uint32_t  size;
    uint8_t   status_flags;
    uint8_t   format_flags;
    char     *data;
};

struct id3v2_tag
{
    struct id3v2_header     header;
    struct id3v2_ext_header ext_header;
    struct id3v2_frame      frame_head;
};

typedef int (* id3_frame_handler_t)(const struct id3v2_frame *,
                                    u32_char *buf,
                                    size_t size);

typedef struct id3_frame_handler_table_t
{
    const char          *id;
    id3_frame_handler_t  get_data;
    const char          *desc;
} id3_frame_handler_table_t;

struct id3v2_tag *new_id3v2_tag();
void free_id3v2_tag(struct id3v2_tag *tag);

int is_valid_frame_id_str(const char *str, size_t len);
int is_valid_frame_id(const char *str);

int read_id3v2_header(int fd, struct id3v2_header *hdr);
int read_id3v2_footer(int fd, struct id3v2_header *hdr);
int read_id3v2_ext_header(int fd, struct id3v2_tag *tag);
int read_id3v2_frames(int fd, struct id3v2_tag *tag);

ssize_t pack_id3v2_tag(const struct id3v2_tag *tag, char **buf);

const char *map_v22_to_v24(const char *v23frame);
const char *map_v23_to_v24(const char *v23frame);

#undef PRIVATE
#endif /* ID3V2_H */
