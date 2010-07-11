#include <assert.h>
#include <ctype.h>    /* isdigit() */
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "printfmt.h"
#include "u32_char.h"

int32_t u32_strcmp(const u32_char *s1, const u32_char *s2)
{
    for (; *s1 != U32_CHAR('\0') && *s1 == *s2; s1++, s2++)
        ;

    return *s1 - *s2;
}

u32_char *u32_strcpy(u32_char *dst, const u32_char *src)
{
    u32_char *dstptr = dst;

    for (; (*dstptr = *src) != U32_CHAR('\0'); src++, dstptr++)
        ;

    return dst;
}

u32_char *u32_strncpy(u32_char *dst, const u32_char *src, size_t size)
{
    u32_char *dstptr = dst;
    
    for (; size > 0 && (*dstptr = *src) != U32_CHAR('\0');
         src++, dstptr++, size--)
        ;

    return dst;
}

u32_char *u32_strdup(const u32_char *str)
{
    size_t len = u32_strlen(str);
    u32_char *dup = malloc((len + 1)*sizeof(u32_char));

    if (!dup)
    {
        errno = ENOMEM;
        return NULL;
    }

    return u32_strcpy(dup, str);
}

size_t u32_strlen(const u32_char *str)
{
    const u32_char *ptr;
    
    for (ptr = str; *ptr != U32_CHAR('\0'); ptr++)
        ;

    return ptr - str;
}

size_t u32_strnlen(const u32_char *str, size_t maxlen)
{
    const u32_char *ptr;
    
    for (ptr = str; *ptr != U32_CHAR('\0') && maxlen > 0; ptr++, maxlen--)
        ;

    return ptr - str;
}

long u32_strtol(const u32_char *str, u32_char **endptr, int base)
{
    long ret = 0;
    long val;

    for (; (*str >= U32_CHAR('0') && *str <= U32_CHAR('9'))
           || (*str >= U32_CHAR('A') && *str <= U32_CHAR('Z'));
         str++)
    {
        ret *= base;
        val = *str <= U32_CHAR('9') ? *str - U32_CHAR('0')
                                    : *str - U32_CHAR('A') + 10;

        if (val >= base)
            break;

        ret += val;
    }

    if (endptr)
        *endptr = (u32_char *)str;

    return ret;
}

int u32_vsnprintf(u32_char *str, size_t size, const char *fmt, va_list ap)
{
    const char *pos;
    const char *lastspec = NULL;
    size_t len;
    int is_empty_str = (size == 0);
    size_t ret = size;
    enum {
        ln_normal, ln_long
    } length = ln_normal;
    enum {
        st_escape, st_normal, st_flags, st_width, st_prec, st_length, st_spec
    } state = st_normal;
    struct print_fmt pfmt = { };

#define append_char(c) { if (size > 0) { *str++ = c; size--; } else ret++; }

    len = strlen(fmt);

    for (pos = fmt; pos < fmt + len; pos++)
    {
        switch (state)
        {
            case st_normal:
                switch (*pos)
                {
                    case '\\': state = st_escape; break;
                    case '%':  memset(&pfmt, '\0', sizeof(pfmt));
                               lastspec = pos;
                               length = ln_normal;
                               state = st_flags;
                               break;
                    default:   append_char(U32_CHAR(*pos));
                }
                break;

            case st_escape:
                switch (*pos)
                {
                    case 'n': append_char(U32_CHAR('\n')); break;
                    case 'r': append_char(U32_CHAR('\r')); break;
                    case 't': append_char(U32_CHAR('\t')); break;
                    case '\\': append_char(U32_CHAR('\\')); break;
                    /* process it again */
                    default:  append_char(U32_CHAR('\\')); pos--;
                }
                state = st_normal;
                break;

            case st_flags:
                switch (*pos)
                {
                    case '0': pfmt.flags |= FL_ZERO; break;
                    case '-': pfmt.flags |= FL_LEFT; break;
                    default:  pos--; state = st_width;
                }
                break;

            case st_width:
                if (isdigit(*pos))
                    pfmt.width = 10*pfmt.width + (*pos - '0');
                else if (*pos == '.')
                {
                    state = st_prec;
                    pfmt.flags |= FL_PREC;
                }
                else
                {
                    state = st_length;
                    pos--; /* process it again */
                }
                break;

            case st_prec:
                if (*pos == '*')
                    pfmt.precision = va_arg(ap, int);
                else if (isdigit(*pos))
                    pfmt.precision = 10*pfmt.precision + (*pos - '0');
                else
                {
                    state = st_length;
                    pos--; /* process it again */
                }
                break;

            case st_length:
                if (*pos == 'l')
                    length = ln_long;
                else
                    pos--; /* process it again */
                state = st_spec;
                break;

            case st_spec:
                switch (*pos)
                {
                    case 'u':
                    {
                        u32_char buf[33] = { };
                        u32_char *ptr = buf + sizeof(buf) - 1;
                        size_t arg_len = 0;
                        unsigned u = va_arg(ap, unsigned);
                        do {
                            *--ptr = U32_CHAR('0') + u % 10;
                            arg_len++;
                        } while ((u /= 10) != 0);

                        if (size > 0)
                        {
                            size_t towrite = arg_len > size
                                             ? size : arg_len;

                            u32_strncpy(str, ptr, towrite);
                            str += towrite;
                            size -= towrite;
                            ret += arg_len - towrite;
                        }
                        else
                            ret += arg_len;
                        break;
                    }
                    case 's':
                        if (length == ln_long)
                        {
                            u32_char *arg = va_arg(ap, u32_char *);
                            size_t arg_len;

                            arg_len = pfmt.flags & FL_PREC
                                ? u32_strnlen(arg, pfmt.precision)
                                : u32_strlen(arg);

                            if (size > 0)
                            {
                                size_t towrite = arg_len > size
                                                 ? size : arg_len;

                                u32_strncpy(str, arg, towrite);
                                str += towrite;
                                size -= towrite;
                                ret += arg_len - towrite;
                            }
                            else
                                ret += arg_len;
                        }
                        else
                        {
                            
                        }
                        break;
                    case 'c':
                        if (length == ln_long)
                        {
                            u32_char ch = va_arg(ap, u32_char);
                            append_char(ch);
                        }
                        break;
                    default:
                        assert(1);
                        break;
                }

                state = st_normal;
                break;
        }
    }

    if (size > 0)
        *str = U32_CHAR('\0');
    else if (!is_empty_str)
        *(str-1) = U32_CHAR('\0');

    return ret - size;
}

int u32_snprintf(u32_char *str, size_t size, const char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = u32_vsnprintf(str, size, fmt, ap);
    va_end(ap);
    return ret;
}
