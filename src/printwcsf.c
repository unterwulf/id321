#include <stdio.h>
#include <string.h>
#include <limits.h>   /* MB_LEN_MAX */
#include <wchar.h>
#include "printfmt.h"

static void print_padding(char ch, int len)
{
    int n;

    for (n = 0; n < len; n++)
        putchar(ch);
}

/***
 * printwcsf - prints wide-character string @wcs using format from @pf
 *
 * @pf - pointer to format struct
 * @wcs - wide character string
 *
 * This function differs from printf("%ls"). It does not stop processing @wcs
 * on the first non-representable wide character. Instead, it prints a dummy
 * characted and continues processing.
 *
 * The function can be called with @pf pointing to NULL. In this case default
 * format is used.
 */

void printwcsf(const struct print_fmt *pf, wchar_t *wcs)
{
    int len = wcslen(wcs);
    int precision;
    int actlen = len;
    int i;
    mbstate_t ps;
    char mbchar[MB_LEN_MAX];
    size_t mbsize;
    static const struct print_fmt default_pf = {}; /* to use when @pf is NULL */

    memset(&ps, '\0', sizeof(ps));

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

    /* we cannot use printf here because %ls stops format string processing if
     * wc cannot be represented as a multibyte sequence, and we need to just
     * print ? in such case and continue processing */

    for (i = 0; i < len; i++)
    {
        mbsize = wcrtomb(mbchar, wcs[i], &ps);

        if (mbsize == -1)
            mbsize = wcrtomb(mbchar, L'?', &ps);

        fwrite(mbchar, mbsize, 1, stdout);
    }

    if (pf->width > actlen && (pf->flags & FL_LEFT))
        print_padding(' ', pf->width - actlen);
}
