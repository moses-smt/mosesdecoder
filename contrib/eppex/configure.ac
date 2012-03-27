# eppex - epochal phrase table extraction for Statistical Machine Translation
# Ceslav Przywara, UFAL MFF UK, Prague, 2011
# $Id: $

# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([eppex], [1.0], [ceslav at przywara.cz])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([eppex.cpp])
AC_CONFIG_HEADERS([config.h])
AC_LANG([C++])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AX_BOOST_BASE([1.35.0])

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
