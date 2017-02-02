# memscore - in-memory phrase scoring for Statistical Machine Translation
# Christian Hardmeier, FBK-irst, Trento, 2010
# $Id$

# Process this file with autoconf to produce a configure script.

AC_INIT([memscore], [1.0], [hardmeier at fbk.eu])
AM_INIT_AUTOMAKE
AC_LANG([C++])

AC_ARG_WITH(irstlm,
	[AC_HELP_STRING([--with-irstlm=PATH], [(optional) path to the IRSTLM toolkit])],
	[with_irstlm=$withval],
	[with_irstlm=check])

AC_ARG_WITH([gsl],
	[AC_HELP_STRING([--with-gsl=PATH], [path to the GSL library])],
	[with_gsl=$withval
	 CPPFLAGS="$CPPFLAGS -I$with_gsl/include"
	 LDFLAGS="$LDFLAGS -L$with_gsl/lib"],
	[with_gsl=check])

AC_ARG_ENABLE([channel],
	[AC_HELP_STRING([--enable-channel], [feature not yet publicly available])],
	[AC_DEFINE(ENABLE_CHANNEL_SCORER, [], [Define to enable channel scorer])],
	[enable_channel=no])

AC_PREREQ([2.63])
AC_CONFIG_SRCDIR([memscore.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AX_BOOST_BASE([1.35.0])

AC_CHECK_LIB([m], [cos])
AC_CHECK_LIB([z], [gzopen])

have_gsl=yes
AC_CHECK_LIB([gslcblas],[cblas_dgemm], [], [have_gsl=no])
AC_CHECK_LIB([gsl],[gsl_blas_dgemm], [], [have_gsl=no])

AS_IF([test x$with_irstlm = xcheck],
	  [AC_CHECK_HEADER([n_gram.h],
                 [AC_DEFINE([HAVE_IRSTLM], [], [flag for IRSTLM])],
                 [with_irstlm=no])]
,
	  [SAVE_CPPFLAGS="$CPPFLAGS"
	  CPPFLAGS="$CPPFLAGS -I${with_irstlm}/include"

	  AC_CHECK_HEADER(n_gram.h,
			 [AC_DEFINE([HAVE_IRSTLM], [], [flag for IRSTLM])],
			 [AC_MSG_ERROR([Cannot find IRSTLM!])])

	  MY_ARCH=`uname -m`
	  LIB_IRSTLM="-lirstlm"
	  LDFLAGS="$LDFLAGS -L${with_irstlm}/lib/${MY_ARCH}"
	  LIBS="$LIBS $LIB_IRSTLM"
	  FMTLIBS="$FMTLIBS libirstlm.a"]
)
AM_CONDITIONAL([IRSTLM], [test x$with_irstlm != xno])

AS_IF([test x$enable_channel = xyes],
	[AS_IF([test x$with_irstlm = xno || test x$have_gsl = xno],
		[AC_MSG_ERROR([The channel scorer needs both GSL and irstlm.])])])

# Checks for header files.
#AC_CHECK_HEADERS([fenv.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
#AC_FUNC_MALLOC
#AC_CHECK_FUNCS([getpagesize gettimeofday])

AM_CONDITIONAL(CHANNEL_SCORER, test x$enable_channel = xyes)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
