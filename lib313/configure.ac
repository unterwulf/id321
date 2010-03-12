# require autoconf 2.61
AC_PREREQ([2.61])
AC_INIT([lib313], [0.0.2], [vs@kp4.ru])
AC_CONFIG_AUX_DIR([build])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([-Wall -Werror])
AM_CONFIG_HEADER([config.h])

# Checks for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB

# Checks for header files
AC_HEADER_STDC

# Checks for functions
AH_TEMPLATE(HAVE_STRNLEN, [Define to 1 if you have the working strnlen() function.])
AC_FUNC_STRNLEN
if test "x$ac_cv_func_strnlen_working" = "xyes"; then
    AC_DEFINE([HAVE_STRNLEN])
else
    compat=yes
fi

AM_CONDITIONAL([COMPAT], [test x$compat = xyes])

AC_OUTPUT([Makefile src/Makefile])
