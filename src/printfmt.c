#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "printfmt.h"
#include "u32_char.h"

static inline void print_padding(char ch, size_t len)
{
    for (; len > 0; len--)
        putchar(ch);
}

/***
 * printfmt_gen - prints string @str using format from @pf
 *
 * @pf - pointer to format struct
 * @str - char or u32_char string
 * @is_u32 - kind of string @str is pointed to
 *
 * The function can be called with @pf pointing to NULL. In this case default
 * format is used.
 */

static void printfmt_gen(const struct print_fmt *pf, void *str, int is_u32)
{
    size_t len = is_u32 ? u32_strlen(str) : strlen(str);
    int precision;
    int actlen = len;
    static const struct print_fmt default_pf = {}; /* to use when @pf is NULL */

    if (!pf)
        pf = &default_pf;

    precision = pf->precision;

    if (pf->flags & FL_INT)
    {
        if (!(pf->flags & FL_PREC) && (pf->flags & FL_ZERO)
            && !(pf->flags & FL_LEFT))
            precision = pf->width;

        actlen = len > precision ? len : precision;
    }
    else if ((pf->flags & FL_PREC) && precision < len)
        actlen = len = precision;

    if (pf->width > actlen && !(pf->flags & FL_LEFT))
        print_padding(' ', pf->width - actlen);

    if ((pf->flags & FL_INT) && precision > len)
        print_padding('0', precision - len);

    if (!is_u32)
    {
        printf("%.*s", len, (char *)str);
    }
    else
    {
        char *buf;
        size_t size;

        iconv_alloc(locale_encoding(), U32_CHAR_CODESET,
                    (const char *)str, len * sizeof(u32_char),
                    &buf, &size);

        printf("%.*s", size, buf);
        free(buf);
    }

    if (pf->width > actlen && (pf->flags & FL_LEFT))
        print_padding(' ', pf->width - actlen);
}

void printfmt(const struct print_fmt *pf, char *str)
{
    printfmt_gen(pf, str, 0);
}

void u32_printfmt(const struct print_fmt *pf, u32_char *u32_str)
{
    printfmt_gen(pf, u32_str, 1);
}
