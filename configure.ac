dnl ===================================================================
dnl configure.ac
dnl Process this file with autogen.sh to produce configure files
dnl ===================================================================


AC_PREREQ(2.59)
AC_INIT(Iperf,2.0.4)
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE()


AC_ARG_ENABLE(ipv6, AC_HELP_STRING([--disable-ipv6],
	    [disable ipv6 support (default is autodetect)]), 
	ac_cv_have_ipv6=$enable_ipv6,)

AC_ARG_ENABLE(multicast, AC_HELP_STRING([--disable-multicast],
	    [disable multicast support (default is autodetect)]),
	ac_cv_multicast=$enable_multicast,)

AC_ARG_ENABLE(threads, AC_HELP_STRING([--disable-threads],
	    [disable thread support (default is autodetect)]))

AC_ARG_ENABLE(debuginfo, AC_HELP_STRING([--enable-debuginfo],
	    [enable debugging info for sockets (default is no)]),
	enable_debuginfo=$enableval,
	enable_debuginfo=no)

AC_ARG_ENABLE(web100, AC_HELP_STRING([--disable-web100],
	    [disable web100 support (default is autodetect)]))


dnl ===================================================================
dnl Checks for programs
dnl ===================================================================

AC_PROG_CXX
CXXFLAGS=`echo " $CXXFLAGS " | sed -e "s/ -g / /"` # do not want it implicitly
AC_PROG_CC
CFLAGS=`echo " $CFLAGS " | sed -e "s/ -g / /"` # do not want it implicitly
AC_ISC_POSIX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_LANG(C)
AC_CANONICAL_HOST

dnl ===================================================================
dnl Checks for libraries.
dnl ===================================================================

dnl check for -lpthread
 
if test "$enable_threads" != no; then
  ACX_PTHREAD()
    if test "$acx_pthread_ok" = yes; then
      AC_DEFINE([HAVE_POSIX_THREAD], 1,)
      AC_DEFINE([_REENTRANT], 1,)
    fi
fi

dnl check for -lnsl, -lsocket
AC_CHECK_FUNC(gethostbyname,,AC_CHECK_LIB(nsl, gethostbyname))
AC_CHECK_FUNC(socket,,AC_CHECK_LIB(socket, socket))

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h libintl.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h sys/time.h syslog.h unistd.h])

dnl ===================================================================
dnl Checks for typedefs, structures
dnl ===================================================================

AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES(ssize_t,,AC_DEFINE_UNQUOTED(ssize_t, int))
AC_HEADER_TIME
AC_STRUCT_TM

dnl these intXX_t and u_intXX_t need to be defined to be the right size.
AX_CREATE_STDINT_H(include/iperf-int.h)

AC_CACHE_CHECK(3rd argument of accept, ac_cv_accept_arg, [
  dnl Try socklen_t (POSIX)
  DAST_ACCEPT_ARG(socklen_t)

  dnl Try int (original BSD)
  DAST_ACCEPT_ARG(int)

  dnl Try size_t (older standard; AIX)
  DAST_ACCEPT_ARG(size_t)

  dnl Try short (shouldn't be)
  DAST_ACCEPT_ARG(short)

  dnl Try long (shouldn't be)
  DAST_ACCEPT_ARG(long)
])

if test -z "$ac_cv_accept_arg" ; then
  ac_cv_accept_arg=int
fi

AC_DEFINE_UNQUOTED([Socklen_t], $ac_cv_accept_arg, [Define 3rd arg of accept])

dnl Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gettimeofday memset pthread_cancel select strchr strerror strtol usleep])
AC_REPLACE_FUNCS(snprintf inet_pton inet_ntop gettimeofday)

dnl             Gotten from some NetBSD configure.in
dnl		We assume that if sprintf() supports %lld or %qd,
dnl		then all of *printf() does. If not, disable long long
dnl		support because we don't know how to display it.

AH_TEMPLATE(HAVE_QUAD_SUPPORT)
AH_TEMPLATE(HAVE_PRINTF_QD)

AC_MSG_CHECKING(*printf() support for %lld)
can_printf_longlong=no
AC_TRY_RUN([
        #include <stdio.h>
        int main() {
                char buf[100];
                sprintf(buf, "%lld", 21726587590LL);
                return (strcmp(buf, "21726587590"));
        }
        ], [
        AC_MSG_RESULT(yes)
        can_printf_longlong=yes
        ], [
        AC_MSG_RESULT(no)
        ], [ : ])

if test $can_printf_longlong != yes; then
        AC_MSG_CHECKING(*printf() support for %qd)
        AC_TRY_RUN([
                #include <stdio.h>
                int main() {
                        char buf[100];
                        sprintf(buf, "%qd", 21726587590LL);
                        return (strcmp(buf, "21726587590"));
                }
                ], [
                AC_MSG_RESULT(yes)
                can_printf_longlong=yes
                AC_DEFINE(HAVE_PRINTF_QD, 1)
                ], [
                AC_MSG_RESULT(no)
                ], [ : ])
fi

if test $can_printf_longlong = yes; then
        AC_DEFINE(HAVE_QUAD_SUPPORT, 1)
fi

dnl ===================================================================
dnl Check for compiler characteristics

DAST_CHECK_BOOL

AC_C_BIGENDIAN

dnl ===================================================================
dnl Check for system services

dnl check for multicast
if test "$ac_cv_multicast" != no; then
  AC_CHECK_TYPES(struct ip_mreq,,,[#include "compat/headers_slim.h"])
  AC_CHECK_DECLS(IP_ADD_MEMBERSHIP,,,[#include "compat/headers_slim.h"])
  AC_MSG_CHECKING(for multicast support)
  ac_cv_multicast=no
  if test "$ac_cv_have_decl_IP_ADD_MEMBERSHIP" = yes; then
    if test "$ac_cv_type_struct_ip_mreq" = yes; then
      ac_cv_multicast=yes
    fi
  fi
  AC_MSG_RESULT($ac_cv_multicast)
  if test "$ac_cv_multicast" = yes; then
    AC_DEFINE([HAVE_MULTICAST], 1, [Define to enable multicast support])
  fi
fi

dnl check for IPv6
if test "$ac_cv_have_ipv6" != no; then
  AC_CHECK_TYPES(struct sockaddr_storage,,,[#include "compat/headers_slim.h"])
  AC_CHECK_TYPES(struct sockaddr_in6,,,[#include "compat/headers_slim.h"])
  AC_CHECK_DECLS(AF_INET6,,,[#include "compat/headers_slim.h"])
  AC_MSG_CHECKING(for IPv6 headers and structures)
  ac_cv_have_ipv6=no
  if test "$ac_cv_type_struct_sockaddr_storage" = yes; then
    if test "$ac_cv_type_struct_sockaddr_in6" = yes; then
      if test "$ac_cv_have_decl_AF_INET6" = yes; then
        AC_DEFINE([HAVE_IPV6], 1, [Define to enable IPv6 support])
        ac_cv_have_ipv6=yes
      fi
    fi
  fi
  AC_MSG_RESULT($ac_cv_have_ipv6)
fi

if test "$ac_cv_have_ipv6" = yes; then
  if test "$ac_cv_multicast" = yes; then
    AC_CHECK_TYPES(struct ipv6_mreq,,,[#include "compat/headers_slim.h"])
    AC_CHECK_DECLS(IPV6_ADD_MEMBERSHIP,,,[#include "compat/headers_slim.h"])
    AC_CHECK_DECLS(IPV6_MULTICAST_HOPS,,,[#include "compat/headers_slim.h"])
    AC_MSG_CHECKING(for IPv6 multicast support)
    ac_cv_have_ipv6_multicast=no
    if test "$ac_cv_type_struct_ipv6_mreq" = yes; then
      if test "$ac_cv_have_decl_IPV6_ADD_MEMBERSHIP" = yes; then
        if test "$ac_cv_have_decl_IPV6_MULTICAST_HOPS" = yes; then
          AC_DEFINE([HAVE_IPV6_MULTICAST], 1, [Define to enable IPv6 multicast support])
          ac_cv_have_ipv6_multicast=yes
        fi
      fi
    fi
    AC_MSG_RESULT($ac_cv_have_ipv6_multicast)
  fi
fi

if test "$enable_debuginfo" = yes; then
AC_DEFINE([DBG_MJZ], 1, [Define if debugging info is desired])
fi

if test "$enable_web100" != no; then
if test -e "/proc/web100"; then
if test -d "/proc/web100"; then
if test -e "/proc/web100/header"; then
if test -f "/proc/web100/header"; then
if test -r "/proc/web100/header"; then
AM_PATH_WEB100()
if test "$web100_success" = yes; then 
AC_DEFINE([HAVE_WEB100], 1, [Define if Web100 is desired and available])
fi fi fi fi fi fi
fi

dnl GNU make allows us to use the $(strip ...) builtin which eliminates a
dnl large amount of extra whitespace in compile lines.
AC_MSG_CHECKING(whether make is GNU make)
STRIP_BEGIN=
STRIP_END=
if $ac_make --version 2> /dev/null | grep '^GNU Make ' > /dev/null ; then
    STRIP_BEGIN='$(strip $(STRIP_DUMMY)'
    STRIP_END=')'
    AC_MSG_RESULT(yes)
else
    AC_MSG_RESULT(no)
fi

dnl some Make 3.79 $(strip ) versions are broken and require an empty arg
STRIP_DUMMY=
AC_SUBST(STRIP_DUMMY)
AC_SUBST(STRIP_BEGIN)
AC_SUBST(STRIP_END)

AC_CONFIG_FILES([Makefile
		compat/Makefile
		doc/Makefile
		include/Makefile
		src/Makefile
                man/Makefile])
AC_OUTPUT
