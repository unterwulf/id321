#include <inttypes.h>
#include <string.h>
#include "common.h"

static const struct
{
    uint8_t      id;
    const char  *str;
}
id3v1e_speed[] =
{
    { 1, "slow" },
    { 2, "medium" },
    { 3, "fast" },
    { 4, "hardcore" }
};

const char *get_id3v1e_speed_str(uint8_t speed_id)
{
    uint8_t i;

    for_each (i, id3v1e_speed)
        if (id3v1e_speed[i].id == speed_id)
            return id3v1e_speed[i].str;

    return NULL;
}

uint8_t get_id3v1e_speed_id(const char *speed_str)
{
    uint8_t i;

    for_each (i, id3v1e_speed)
        if (!strcmp(id3v1e_speed[i].str, speed_str))
            return id3v1e_speed[i].id;

    return 0;
}
