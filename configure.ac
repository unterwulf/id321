AC_INIT([id321], [0.0.1], [vs@kp4.ru])
AC_CONFIG_AUX_DIR([build])
AM_INIT_AUTOMAKE([-Wall])
AM_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_INSTALL

AC_HEADER_STDC

AC_CHECK_LIB(313, lib313_unpack)
AC_CHECK_LIB(313, lib313_pack)

AC_C_BIGENDIAN

AM_CONDITIONAL([STATIC_LIB313], [test "x$ac_cv_lib_313_lib313_unpack" = xno])
AC_DEFINE([UCS2_WORKAROUND], [1],
          [Define it 0 if your iconv() uses BOM in UCS-2 encoding.])
AC_CONFIG_SUBDIRS([lib313])

AC_OUTPUT([Makefile src/Makefile])
