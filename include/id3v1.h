#ifndef ID3V1_H
#define ID3V1_H

#include <unistd.h>
#include <lib313.h>

/* though v1 enhanced tag is not known as v1.4 we will consider it as
 * having 4 as minor version because we need an integer minor version
 * for internal use */

#define ID3V1E_MINOR            4

#define ID3V12_TAG_SIZE         128+128
#define ID3V1E_TAG_SIZE         128+227

#define ID3V12_HEADER           "EXT"
#define ID3V1E_HEADER           "TAG+"
#define ID3V12_HEADER_SIZE      3
#define ID3V1E_HEADER_SIZE      4

#define ID3V1E_GEN_SIZE         30

struct id3v1_tag 
{
    int     version;
    char    title[ID3V13_MAX_TIT_SIZE+1];
    char    artist[ID3V13_MAX_ART_SIZE+1];
    char    album[ID3V13_MAX_ALB_SIZE+1];
    char    year[ID3V1_YER_SIZE+1];
    char    comment[ID3V13_MAX_COM_SIZE+1];
    uint8_t track;        /* absent in id3v1.0 */
    uint8_t genre;

    /* enhanced tag only fields */

    char    genre2[ID3V1E_GEN_SIZE+1];
    char    starttime[6];
    char    endtime[6];
    uint8_t speed;
};

ssize_t read_id3v1_tag(int fd, struct id3v1_tag *tag);
ssize_t read_id3v1_enh_tag(int fd, struct id3v1_tag *tag);
ssize_t read_id3v11_tag(int fd, struct id3v1_tag *tag);

int pack_id3v1_tag(struct id3v1_tag *tag, void *buf, size_t size);

#endif /* ID3V1_H */
