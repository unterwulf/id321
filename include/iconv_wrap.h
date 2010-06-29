#ifndef ICONV_WRAP_H
#define ICONV_WRAP_H
#include "config.h"

#ifdef UCS2_WORKAROUND

#include "iconv_ucs2.h"

typedef iconv_ucs2_t       id321_iconv_t;
#define id321_iconv_open   iconv_open_ucs2
#define id321_iconv        iconv_ucs2
#define id321_iconv_close  iconv_close_ucs2

#else

#include <iconv.h>

typedef iconv_t            id321_iconv_t;
#define id321_iconv_open   iconv_open
#define id321_iconv        iconv
#define id321_iconv_close  iconv_close

#endif /* !UCS2_WORKAROUND */

#endif /* ICONV_WRAP_H */
