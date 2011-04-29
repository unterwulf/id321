#ifndef U32_CHAR_H
#define U32_CHAR_H

#include <config.h>
#include <inttypes.h>
#include <stdarg.h>

typedef uint32_t u32_char;

/* both GNU and Solaris iconv() understand these codeset names */
#ifdef WORDS_BIGENDIAN
#define U32_CHAR_CODESET "UCS-4BE"
#else
#define U32_CHAR_CODESET "UCS-4LE"
#endif

#define U32_CHAR(c) ((u32_char) (c))

int32_t u32_strcmp(const u32_char *s1, const u32_char *s2);
u32_char *u32_strcpy(u32_char *dst, const u32_char *src);
u32_char *u32_strncpy(u32_char *dst, const u32_char *src, size_t size);
u32_char *u32_strdup(const u32_char *str);
size_t u32_strlen(const u32_char *str);
size_t u32_strnlen(const u32_char *str, size_t maxlen);
long u32_strtol(const u32_char *str, u32_char **endptr, int base);

int u32_vsnprintf(u32_char *str, size_t size, const char *fmt, va_list ap);
int u32_snprintf(u32_char *str, size_t size, const char *fmt, ...);

#endif /* U32_CHAR_H */
