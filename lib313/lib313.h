/*
 * lib313.h -- lib313 API
 *
 * lib313: a C library for packing/unpacking id3v1.3 tags
 * Copyright (c) 2009 Vitaly Sinilin <vs@kp4.ru>
 *
 */

#ifndef _LIB313_H_
#define _LIB313_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ID3V1_TAG_SIZE       128

#define ID3V1_HEADER         "TAG"
#define ID3V1_HEADER_SIZE    3

#define ID3V1_TIT_SIZE       30
#define ID3V1_TIT_OFF        3
#define ID3V1_ART_SIZE       30
#define ID3V1_ART_OFF        33
#define ID3V1_ALB_SIZE       30
#define ID3V1_ALB_OFF        63
#define ID3V1_YER_SIZE       4
#define ID3V1_YER_OFF        93
#define ID3V1_COM_SIZE       30
#define ID3V1_COM_OFF        97
#define ID3V11_COM_SIZE      28
#define ID3V11_TRK_IND_OFF   125
#define ID3V11_TRK_OFF       126
#define ID3V1_GEN_OFF        127

#define ID3V13_MAX_EXT_SIZE  119
#define ID3V13_MAX_ART_SIZE  120
#define ID3V13_MAX_ALB_SIZE  120
#define ID3V13_MAX_TIT_SIZE  120
#define ID3V13_MAX_COM_SIZE  120

#define LIB313_SUCCESS       0
#define LIB313_ERR_BAD_TAG  (-2)
#define LIB313_ERR_INVALID  (-3)

#define LIB313_TAG_ART_SIZE(tag)  ID3V1_ART_SIZE
#define LIB313_TAG_ALB_SIZE(tag)  ID3V1_ALB_SIZE
#define LIB313_TAG_TIT_SIZE(tag)  ID3V1_TIT_SIZE
#define LIB313_TAG_COM_SIZE(tag)  ((tag).track == '\0' ? \
                                      ID3V1_COM_SIZE : ID3V11_COM_SIZE)
#define LIB313_TAG_YER_SIZE(tag)  ID3V1_YER_SIZE

#define LIB313_TAG_ART(tag) ((tag).artist)
#define LIB313_TAG_ALB(tag) ((tag).album)
#define LIB313_TAG_TIT(tag) ((tag).title)
#define LIB313_TAG_COM(tag) ((tag).comment)
#define LIB313_TAG_YER(tag) ((tag).year)


struct lib313_tag
{
    int     	  version;
    char    	  title[ID3V13_MAX_TIT_SIZE + 1];
    char    	  artist[ID3V13_MAX_ART_SIZE + 1];
    char    	  album[ID3V13_MAX_ALB_SIZE + 1];
    char    	  year[ID3V1_YER_SIZE + 1];
    char    	  comment[ID3V13_MAX_COM_SIZE + 1];
    unsigned char track;        /* absent in id3v1.0 */
    unsigned char genre;
};

int lib313_estimate(const struct lib313_tag *tag);
int lib313_pack(char *buf, const struct lib313_tag *tag);
int lib313_unpack(struct lib313_tag *tag, const char *buf);

#ifdef __cplusplus
}
#endif

#endif /* _LIB313_H_ */
