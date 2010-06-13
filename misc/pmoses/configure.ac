#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.52)

AC_INIT([pmoses], [0.1.0], [fsanchez@dlsi.ua.es])

AC_CONFIG_HEADERS([config.h])

# Release versioning
GENERIC_MAJOR_VERSION=0
GENERIC_MINOR_VERSION=1
GENERIC_MICRO_VERSION=0

GENERIC_API_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_API_VERSION)

PACKAGE=pmoses

GENERIC_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION.$GENERIC_MICRO_VERSION
GENERIC_RELEASE=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)

VERSION=$GENERIC_VERSION

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)

# Checks for programs.

AC_MSG_CHECKING([Compilation architecture: PPC, i686, Other])
ARCH=$(/bin/uname -m)
if test x$ARCH = xppc
then
  AC_MSG_RESULT([PowerPC])
  CFLAGS="$CFLAGS -Wall -ansi -mpowerpc -maltivec -fno-pic -fomit-frame-pointer"
  CXXFLAGS="$CXXFLAGS -Wall -ansi -mpowerpc -maltivec -fno-pic -fomit-frame-pointer"
else
  if test x$ARCH = xi686
  then
    AC_MSG_RESULT([i686])
    CFLAGS="$CFLAGS -Wall -ansi -march=i686 -O3 -fno-pic -fomit-frame-pointer"
    CXXFLAGS="$CXXFLAGS -Wall -ansi -march=i686 -O3 -fno-pic -fomit-frame-pointer"
  else
    AC_MSG_RESULT([Other])
    CFLAGS="$CFLAGS -Wall -ansi -O3"
    CXXFLAGS="$CXXFLAGS -Wall -ansi -O3"
  fi
fi

AC_PROG_CXX
AM_SANITY_CHECK
AC_LANG_CPLUSPLUS

AC_ARG_ENABLE(debug,
              [  --enable-debug    Enable "-g -Wall" compiler options], 
              [CXXFLAGS="-g -Wall"; CFLAGS="-g -Wall"])

AC_ARG_ENABLE(profile,
              [  --enable-profile  Enable "-pg -g -Wall" compiler options],
              [CXXFLAGS="-pg -g -Wall"; CFLAGS="-pg -g -Wall"; LDFLAGS="-pg"])

AM_CONDITIONAL([SRI_LM], false)

AC_ARG_WITH(srilm,
            [AC_HELP_STRING([--with-srilm=PATH], [(required) path to the SRI LM toolkit])],
            [with_srilm=$withval],
            [with_srilm=no]
           )

if test "x$with_srilm" != 'xno'
then
  CXXFLAGS="$CXXFLAGS -I${with_srilm}/include"

  AC_CHECK_HEADER(Ngram.h,
                 [AC_DEFINE([HAVE_SRILM], [], [flag for SRILM])],
                 [AC_MSG_ERROR([Cannot find SRILM!])])

  LIB_SRILM="-loolm -ldstruct -lmisc"
  MY_ARCH=`${with_srilm}/sbin/machine-type`
  LDFLAGS="$LDFLAGS -L${with_srilm}/lib/${MY_ARCH}"
  LIBS="$LIBS $LIB_SRILM"
  FMTLIBS="$FMTLIBS liboolm.a libdstruct.a libmisc.a"
  AM_CONDITIONAL([SRI_LM], true)
fi

if test "x$with_moses" != 'xno'
then
  CXXFLAGS="$CXXFLAGS -I${with_moses}/moses/src"

  AC_CHECK_HEADER(PhraseDictionaryTreeAdaptor.h,
                 [AC_DEFINE([HAVE_MOSES], [], [flag for MOSES])],
                 [AC_MSG_ERROR([Cannot find MOSES!])])

  LIB_MOSES="-lmoses"
  LDFLAGS="$LDFLAGS -L${with_moses}/moses/src"
  LIBS="$LIB_MOSES $LIBS"
  FMTLIBS="$FMTLIBS libmoses.a"
  AM_CONDITIONAL([MOSES], true)
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([getopt.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

#AC_CHECK_DECLS([fread_unlocked, fwrite_unlocked, fgetc_unlocked, fputc_unlocked, fputs_unlocked, getopt_long])
AC_CHECK_DECLS([getopt_long])

AC_CHECK_FUNCS([setlocale])

AC_OUTPUT([Makefile])
