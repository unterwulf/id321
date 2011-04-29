#ifndef ID3V1_GENRES_H
#define ID3V1_GENRES_H

#include <inttypes.h>

#define ID3V1_UNKNOWN_GENRE 255
#define ID3V1_GENRE_ID_MAX  125

const char *get_id3v1_genre_str(uint8_t genre);
uint8_t get_id3v1_genre_id(const char *name);

#endif /* ID3V1_GENRES_H */
