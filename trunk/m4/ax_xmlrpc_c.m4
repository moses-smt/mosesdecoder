AC_DEFUN([AX_XMLRPC_C], [
  AC_MSG_CHECKING(for XMLRPC-C)

  AC_ARG_WITH(xmlrpc-c,
  [  --with-xmlrpc-c=PATH     Enable XMLRPC-C support.],
  [
    if test "$withval" = "no"; then
      AC_MSG_RESULT(no)

    else
      if test "$withval" = "yes"; then
        xmlrpc_cc_prg="xmlrpc-c-config"
      else
        xmlrpc_cc_prg="$withval"
      fi
      
      if eval $xmlrpc_cc_prg --version 2>/dev/null >/dev/null; then
        XMLRPC_C_CPPFLAGS=`$xmlrpc_cc_prg --cflags c++2 abyss-server`
        XMLRPC_C_LIBS=`$xmlrpc_cc_prg c++2 abyss-server --libs`
        CXXFLAGS_SAVED=$CXXFLAGS
        CXXFLAGS="$CXXFLAGS $XMLRPC_C_CPPFLAGS"
        LIBS_SAVED=$LIBS
        LIBS="$LIBS $XMLRPC_C_LIBS"

        AC_TRY_LINK(
        [ #include <xmlrpc-c/server.h>
        ],[ xmlrpc_registry_new(NULL); ],
        [
          AC_MSG_RESULT(ok)
        ], [
          AC_MSG_RESULT(failed)
          AC_MSG_ERROR(Could not compile XMLRPC-C test.)
        ])

dnl        AC_DEFINE(HAVE_XMLRPC_C, 1, Support for XMLRPC-C.)
        have_xmlrpc_c=yes
        AC_SUBST(XMLRPC_C_LIBS)
        AC_SUBST(XMLRPC_C_CPPFLAGS)
    
        LIBS=$LIBS_SAVED
        CXXFLAGS=$CXXFLAGS_SAVED

      else
        AC_MSG_RESULT(failed)
        AC_MSG_ERROR(Could not compile XMLRPC-C test.)
      fi
    fi

  ],[
    AC_MSG_RESULT(ignored)
  ])
])
