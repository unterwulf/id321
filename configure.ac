AC_PREREQ(2.64)
AC_INIT([id321],[0.0.3],[vs@kp4.ru])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_LIBOBJ_DIR([compat])
AC_CONFIG_MACRO_DIR([acinclude])

AC_USE_SYSTEM_EXTENSIONS
AM_PROG_AR
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB

AC_HEADER_STDC

AC_C_BIGENDIAN

AC_FUNC_STRNLEN

if test "x$ac_cv_func_strnlen_working" = xyes
then
  AC_DEFINE([HAVE_STRNLEN], [1],
            [Define to 1 if you have the strnlen() function and it works.])
fi

AM_ICONV
ID321_ICONV_UCS2_WITH_BOM

AC_ARG_WITH([lib313],
            [AS_HELP_STRING([--without-lib313],
                            [do not use external lib313])],
            [lib313=${withval}], [lib313=yes])

if test "x$lib313" = xyes
then
  AC_CHECK_LIB(313, lib313_unpack)
  AC_CHECK_LIB(313, lib313_pack)
  AC_MSG_CHECKING([for lib313])
  if test "x$ac_cv_lib_313_lib313_unpack" = xyes
  then
    AC_MSG_RESULT([found])
  else
    lib313=no
    AC_MSG_RESULT([not found, building with internal lib313])
  fi
else
  AC_MSG_RESULT([building with internal lib313])
fi

AM_CONDITIONAL([STATIC_LIB313], [test "x$lib313" = xno])

AC_CONFIG_FILES([Makefile src/Makefile lib313/Makefile compat/Makefile])
AC_OUTPUT
