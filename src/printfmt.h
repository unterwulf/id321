#ifndef PRINTFMT_H
#define PRINTFMT_H

#include <inttypes.h>

#define FL_ZERO 0x1
#define FL_LEFT 0x2
#define FL_PREC 0x4
#define FL_INT  0x8

struct print_fmt
{
    int     width;
    int     precision;
    uint8_t flags;
};

#endif /* PRINTFMT_H */
