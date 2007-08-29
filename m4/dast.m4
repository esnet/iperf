
dnl DAST_CHECK_BOOL
dnl Check for bool support. Defines bool, true, and false.

AH_TEMPLATE([HAVE_POSIX_THREAD], [])
AH_TEMPLATE([_REENTRANT], [])
AH_TEMPLATE([ssize_t], [Define to "int" if <sys/types.h> does not define.])
AH_TEMPLATE([bool])
AH_TEMPLATE([true])
AH_TEMPLATE([false])

AC_DEFUN(DAST_CHECK_BOOL, [

AC_CHECK_SIZEOF(bool)
if test "$ac_cv_sizeof_bool" = 0 ; then
  AC_DEFINE(bool, int)
fi

AC_CACHE_CHECK(if true is defined, ac_cv_have_true,
 [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([],
    [unsigned int i = true],
  ac_cv_have_true=yes,
  ac_cv_have_true=no)
  AC_LANG_RESTORE
 ])

if test "$ac_cv_have_true" != yes ; then
  AC_DEFINE(true,  1)
  AC_DEFINE(false, 0)
fi

])

dnl ===================================================================
dnl DAST_REPLACE_TYPE( type, sizeof )
dnl Check for the type as AC_CHECK_TYPE does. Define HAVE_<type>
dnl if type exists; don't define <type> to anything if it doesn't exist.
dnl Useful if there is no well-defined default type, such as int32_t

AC_DEFUN(DAST_REPLACE_TYPE, [

AC_CACHE_CHECK(for $1, ac_cv_type_$1,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[$ac_includes_default]],
    [[$1 foo]])],
  ac_cv_type_$1=yes,
  ac_cv_type_$1=no)

if test $ac_cv_type_$1 != yes ; then
  if test "$ac_cv_sizeof_char" = $2; then
    ac_cv_type_$1="char"
  elif test "$ac_cv_sizeof_short" = $2; then
    ac_cv_type_$1="short"
  elif test "$ac_cv_sizeof_int" = $2; then
    ac_cv_type_$1="int"
  elif test "$ac_cv_sizeof_long" = $2; then
    ac_cv_type_$1="long"
  elif test "$ac_cv_sizeof_long_long" = $2; then
    ac_cv_type_$1="long long"
  fi
fi)

if test "$ac_cv_type_$1" != no; then
  if test "$ac_cv_type_$1" != yes; then
    AC_DEFINE_UNQUOTED($1, $ac_cv_type_$1)
  fi
  AC_DEFINE_UNQUOTED(HAVE_`echo $1 | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`)
fi

])

AC_DEFUN(DAST_REPLACE_TYPE_UNSIGNED, [

AC_CACHE_CHECK(for $1, ac_cv_type_$1,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[$ac_includes_default]],
    [[$1 foo]])],
  ac_cv_type_$1=yes,
  ac_cv_type_$1=no)

if test $ac_cv_type_$1 != yes ; then
  if test "$ac_cv_sizeof_unsigned_char" = $2; then
    ac_cv_type_$1="unsigned char"
  elif test "$ac_cv_sizeof_unsigned_short" = $2; then
    ac_cv_type_$1="unsigned short"
  elif test "$ac_cv_sizeof_unsigned_int" = $2; then
    ac_cv_type_$1="unsigned int"
  elif test "$ac_cv_sizeof_unsigned_long" = $2; then
    ac_cv_type_$1="unsigned long"
  elif test "$ac_cv_sizeof_unsigned_long_long" = $2; then
    ac_cv_type_$1="unsigned long long"
  fi
fi)

if test "$ac_cv_type_$1" != no; then
  if test "$ac_cv_type_$1" != yes; then
    AC_DEFINE_UNQUOTED($1, $ac_cv_type_$1)
  fi
  AC_DEFINE_UNQUOTED(HAVE_`echo $1 | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`)
fi

])

dnl DAST_CHECK_ARG
dnl Check for the 3rd arguement to accept

AC_DEFUN(DAST_ACCEPT_ARG, [
  if test -z "$ac_cv_accept_arg" ; then
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS

    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
      [[$ac_includes_default
       #include <sys/socket.h>]],
      [[$1 length;
       accept( 0, 0, &length );]])],
    ac_cv_accept_arg=$1)

    AC_LANG_RESTORE
  fi
])

dnl Configure paths for Web100.  Based off of Owen Taylor's gtk.m4 from gtk+-1.2.10
dnl AM_PATH_WEB100([EXACT-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for Web100, and define WEB100_CFLAGS and WEB100_LIBS
dnl
AC_DEFUN(AM_PATH_WEB100,
[
web100_success=""

AC_PATH_PROG([WEB100_CONFIG], [web100-config], [no])

AC_MSG_CHECKING(for Web100)

if test "$WEB100_CONFIG" != "no"; then
  WEB100_CFLAGS=`$WEB100_CONFIG --cflags`
  WEB100_LIBS=`$WEB100_CONFIG --libs`
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  echo "*** The web100-config script installed by Web100 could not be found"
  echo "*** If Web100 was installed in PREFIX, make sure PREFIX/bin is in"
  echo "*** your path, or set the WEB100_CONFIG environment variable to the"
  echo "*** full path to web100-config"
  web100_success="no"
fi

if test x$web100_success = x; then
  if test "x$1" != "x"; then
    AC_MSG_CHECKING(for Web100 - version $1)

    WEB100_VERSION=`$WEB100_CONFIG --version`
    if test "$WEB100_VERSION" = "$1"; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
      echo "*** The requested ($1) and installed ($WEB100_VERSION) versions"
      echo "*** of Web100 do not match."
      web100_success="no"
    fi
  fi
fi

if test x$web100_success = x; then
  web100_success="yes"
fi

if test x$web100_success = xyes; then
  m4_if([$2], [], [:], [$2])
else
  WEB100_CFLAGS=""
  WEB100_LIBS=""
  m4_if([$3], [], [:], [$3])
fi

AC_SUBST(WEB100_CFLAGS)
AC_SUBST(WEB100_LIBS)
])
