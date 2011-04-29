#include <config.h>  /* WORDS_BIGENDIAN, ICONV_CONST */
#include <errno.h>
#include <iconv.h>
#include <stdlib.h>
#include <strings.h> /* strcasecmp() */

/***
 * It seems that iconv implementations tend not to consider BOM as part of
 * UCS-2 encoded string. This module is intended to work around this issue
 * by wrapping legacy iconv functions.
 *
 * Considering the fact that UCS-2 is a subset of UTF-16, the latter is used
 * here for decoding of UCS-2 strings.
 *
 * For encoding the legacy iconv UCS-2 encoder with handmade BOM is used.
 * iconv() encodes UCS-2 in the machine native byte order, so to generate
 * BOM the macro WORDS_BIGENDIAN which is determined by the configure script
 * is used.
 *
 * By now this workaround is unconditionally enabled, as no BOM-enabled
 * UCS-2 iconv implementations were reported so far.
 */

struct iconv_ucs2
{
    iconv_t cd;
    size_t pos;
    unsigned add_bom;
};

typedef struct iconv_ucs2 *iconv_ucs2_t;

iconv_ucs2_t iconv_open_ucs2(const char *tocode, const char *fromcode)
{
    iconv_t cd;
    iconv_ucs2_t cd_ucs2;

    if (!strcasecmp(fromcode, "UCS2") || !strcasecmp(fromcode, "UCS-2"))
        fromcode = "UTF-16";

    cd = iconv_open(tocode, fromcode);

    if (cd == (iconv_t)(-1))
        return (iconv_ucs2_t)(-1);

    cd_ucs2 = calloc(1, sizeof(struct iconv_ucs2));

    if (!cd_ucs2)
    {
        iconv_close(cd);
        errno = ENOMEM;
        return (iconv_ucs2_t)(-1);
    }

    if (!strcasecmp(tocode, "UCS2") || !strcasecmp(tocode, "UCS-2"))
        cd_ucs2->add_bom = 1;

    cd_ucs2->cd = cd;

    return cd_ucs2;
}

size_t iconv_ucs2(iconv_ucs2_t cd,
                  ICONV_CONST char **inbuf, size_t *inbytesleft,
                  char **outbuf, size_t *outbytesleft)
{
#ifdef WORDS_BIGENDIAN
    const char bom[] = { 0xFE, 0xFF };
#else
    const char bom[] = { 0xFF, 0xFE };
#endif

    if (cd->pos < sizeof(bom) && cd->add_bom)
    {
        for (; *outbytesleft > 0 && cd->pos < sizeof(bom); cd->pos++)
        {
            **outbuf = bom[cd->pos];
            (*outbytesleft)--;
            (*outbuf)++;
        }

        if (cd->pos < sizeof(bom) && *outbytesleft == 0)
        {
            errno = E2BIG;
            return (size_t)(-1);
        }
    }

    return iconv(cd->cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

int iconv_close_ucs2(iconv_ucs2_t cd)
{
    int ret = iconv_close(cd->cd);

    if (ret != 0)
        return ret;

    free(cd);
    return 0;
}
