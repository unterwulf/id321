#ifndef OUTPUT_H
#define OUTPUT_H

#include <inttypes.h>
#include <sys/types.h>

typedef enum {
    OS_DEBUG = 0x8,
    OS_INFO  = 0x4,
    OS_WARN  = 0x2,
    OS_ERROR = 0x1,
} output_severity;

void init_output(uint16_t mask);
void print(output_severity sev, const char *format, ...);
void lprint(const char *fromcode, const char *str);
void lnprint(const char *fromcode, size_t size, const char *str);

#endif /* OUTPUT_H */
