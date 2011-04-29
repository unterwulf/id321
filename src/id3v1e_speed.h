#ifndef ID3V1E_SPEED_H
#define ID3V1E_SPEED_H

#include <inttypes.h>

const char *get_id3v1e_speed_str(uint8_t speed_id);
uint8_t get_id3v1e_speed_id(const char *speed_str);

#define is_valid_id3v1e_speed_id(speed_id) \
    (get_id3v1e_speed_str(speed_id) != NULL)

#endif /* ID3V1E_SPEED_H */
