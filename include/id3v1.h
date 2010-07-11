#ifndef ID3V1_H
#define ID3V1_H

#include <inttypes.h>
#include <stddef.h>   /* size_t */
#include <lib313.h>   /* ID3V1_*, ID3V13_* */

/* v1.1 defines */

#define ID3V11_TIT_SIZE         ID3V1_TIT_SIZE
#define ID3V11_ART_SIZE         ID3V1_ART_SIZE
#define ID3V11_ALB_SIZE         ID3V1_ALB_SIZE

/* v1.2 defines */

#define ID3V12_TAG_SIZE         (128+128)

#define ID3V12_HEADER           "EXT"
#define ID3V12_HEADER_SIZE      3

#define ID3V12_TIT_SIZE         30
#define ID3V12_ART_SIZE         30
#define ID3V12_ALB_SIZE         30
#define ID3V12_COM_SIZE         15
#define ID3V12_GN2_SIZE         20

#define ID3V12_TIT_OFF          3
#define ID3V12_ART_OFF          33
#define ID3V12_ALB_OFF          63
#define ID3V12_COM_OFF          93
#define ID3V12_GN2_OFF          108

/* enhanced tag defines */

/* though enhanced tag is not known as v1.4 we will consider it as
 * having 4 as minor version because we need an integer minor version
 * for internal use */

#define ID3V1E_MINOR            4
#define ID3V1E_TAG_SIZE         (128+227)

#define ID3V1E_HEADER           "TAG+"
#define ID3V1E_HEADER_SIZE      4

#define ID3V1E_TIT_SIZE         60
#define ID3V1E_ART_SIZE         60
#define ID3V1E_ALB_SIZE         60
#define ID3V1E_GN2_SIZE         30
#define ID3V1E_STM_SIZE         6
#define ID3V1E_ETM_SIZE         6

#define ID3V1E_TIT_OFF          4
#define ID3V1E_ART_OFF          64
#define ID3V1E_ALB_OFF          124
#define ID3V1E_SPD_OFF          184
#define ID3V1E_GN2_OFF          185
#define ID3V1E_STM_OFF          215
#define ID3V1E_ETM_OFF          221

struct id3v1_tag 
{
    unsigned version;
    char     title[ID3V13_MAX_TIT_SIZE+1];
    char     artist[ID3V13_MAX_ART_SIZE+1];
    char     album[ID3V13_MAX_ALB_SIZE+1];
    char     year[ID3V1_YER_SIZE+1];
    char     comment[ID3V13_MAX_COM_SIZE+1];
    uint8_t  track;        /* absent in v1.0 */
    uint8_t  genre_id;

    /* v1.2 and enhanced tag common fields */

    char     genre_str[ID3V1E_GN2_SIZE+1];

    /* enhanced tag only fields */

    char     starttime[ID3V1E_STM_SIZE+1];
    char     endtime[ID3V1E_ETM_SIZE+1];
    uint8_t  speed;
};

int unpack_id3v1_tag(const char *buf, size_t size, struct id3v1_tag *tag);
int unpack_id3v12_tag(const char *buf, size_t size, struct id3v1_tag *tag);
int unpack_id3v13_tag(const char *buf, size_t size, struct id3v1_tag *tag);
int unpack_id3v1_enh_tag(const char *buf, size_t size, struct id3v1_tag *tag);

size_t pack_id3v1_tag(const struct id3v1_tag *tag, char *buf);

#endif /* ID3V1_H */
