# ID321_ICONV_UCS2_WITH_BOM
# -------------------------
# Determine whether iconv adds BOM when converting to UCS-2.
AC_DEFUN([ID321_ICONV_UCS2_WITH_BOM],
[
  AC_CACHE_CHECK([whether iconv adds BOM when converting to UCS-2],
    id321_cv_iconv_ucs2_with_bom,
    [AC_RUN_IFELSE(
      [AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT
#include <iconv.h>], [[
#define BOMLE "\xFF\xFE"
#define BOMBE "\xFE\xFF"

const char src[] = "foobar";
char dst[sizeof(src)*2 + 2] = "";
const char *srcp = src;
char *dstp = dst;

size_t src_sz = sizeof(src);
size_t dst_sz = sizeof(dst);
size_t ret;

iconv_t cd = iconv_open("UCS-2", "ISO8859-1");
if (cd != (iconv_t)(-1))
    ret = iconv(cd, &srcp, &src_sz, &dstp, &dst_sz);

return (memcmp(dst, BOMBE, 2) && memcmp(dst, BOMLE, 2));
]])],
      [id321_cv_iconv_ucs2_with_bom=yes],
      [id321_cv_iconv_ucs2_with_bom=no],
      [id321_cv_iconv_ucs2_with_bom=no])])

  if test "x$id321_cv_iconv_ucs2_with_bom" = xyes
  then
    AC_DEFINE(ICONV_UCS2_WITH_BOM, [1],
      [Define to 1 if iconv() adds BOM when converting to UCS-2.])
  else
    AC_LIBOBJ([iconv_ucs2])
  fi
])# ID321_ICONV_UCS2_WITH_BOM
