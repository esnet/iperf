# iperf, Copyright (c) 2014-2022, The Regents of the University of
# California, through Lawrence Berkeley National Laboratory (subject
# to receipt of any required approvals from the U.S. Dept. of
# Energy).  All rights reserved.
#
# If you have questions about your rights to use or distribute this
# software, please contact Berkeley Lab's Technology Transfer
# Department at TTD@lbl.gov.
#
# NOTICE.  This software is owned by the U.S. Department of Energy.
# As such, the U.S. Government has been granted for itself and others
# acting on its behalf a paid-up, nonexclusive, irrevocable,
# worldwide license in the Software to reproduce, prepare derivative
# works, and perform publicly and display publicly.  Beginning five
# (5) years after the date permission to assert copyright is obtained
# from the U.S. Department of Energy, and subject to any subsequent
# five (5) year renewals, the U.S. Government is granted for itself
# and others acting on its behalf a paid-up, nonexclusive,
# irrevocable, worldwide license in the Software to reproduce,
# prepare derivative works, distribute copies to the public, perform
# publicly and display publicly, and to permit others to do so.
#
# This code is distributed under a BSD style license, see the LICENSE
# file for complete information.

# Initialize the autoconf system for the specified tool, version and mailing list
AC_PREREQ([2.71])
AC_INIT([iperf],[3.15],[https://github.com/esnet/iperf],[iperf],[https://software.es.net/iperf/])
m4_include([config/ax_check_openssl.m4])
m4_include([config/iperf_config_static_bin.m4])
AC_LANG(C)

# Specify where the auxiliary files created by configure should go. The config
# directory is picked so that they don't clutter up more useful directories.
AC_CONFIG_AUX_DIR(config)


# Initialize the automake system
AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
LT_INIT

AM_MAINTAINER_MODE
AC_CONFIG_HEADERS(src/iperf_config.h)

AC_CANONICAL_HOST

# Checks for tools: c compiler, ranlib (used for creating static libraries),
# symlinks and libtool
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LN_S
LT_INIT

# Add -Wall if we are using GCC.
if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall"
fi

# Check if enable profiling
AC_ARG_ENABLE([profiling],
    AS_HELP_STRING([--enable-profiling], [Enable iperf3 profiling binary]))
AM_CONDITIONAL([ENABLE_PROFILING], [test x$enable_profiling = xyes])

# Check for the math library (needed by cjson on some platforms)
AC_SEARCH_LIBS(floor, [m], [], [
echo "floor()"
exit 1
])

# On illumos we need -lsocket
AC_SEARCH_LIBS(socket, [socket], [], [
echo "socket()"
exit 1
])

# On illumos inet_ntop in in -lnsl
AC_SEARCH_LIBS(inet_ntop, [nsl], [], [
echo "inet_ntop()"
exit 1
])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Check for poll.h (it's in POSIX so everyone should have it?)
AC_CHECK_HEADERS([poll.h])

# SCTP.  Allow user to disable SCTP support with --without-sctp.
# Otherwise we try to find whatever support is required.
try_sctp=true
AC_ARG_WITH([sctp],
    [AS_HELP_STRING([--without-sctp],
        [disable SCTP])],
    [
        case "$withval" in
	y | ye | yes)
	  ;;
	n | no)
	try_sctp=false
	  ;;
	*)
	AC_MSG_ERROR([Invalid --with-sctp value])
	  ;;
	esac
    ], [
        try_sctp=true
    ]
)

AC_CHECK_HEADERS([linux/tcp.h])

# Check for SCTP support
if $try_sctp; then
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([netinet/sctp.h],
		 AC_DEFINE([HAVE_SCTP_H], [1], [Have SCTP support.])
		 AC_SEARCH_LIBS(sctp_bindx, [sctp])
		 AC_CHECK_TYPES([struct sctp_assoc_value], [], [],
				[[#include <netinet/sctp.h>]]),
		 [],
		 [#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])
fi

AC_CHECK_HEADER([endian.h],
		AC_DEFINE([HAVE_ENDIAN_H], [1], [Define to 1 if you have the <endian.h> header file.]),
		AC_CHECK_HEADER([sys/endian.h],
				AC_DEFINE([HAVE_SYS_ENDIAN_H], [1], [Define to 1 if you have the <sys/endian.h> header file.]),
				AC_MSG_WARN([Couldn't find endian.h or sys/endian.h files: doing compile-time tests.])
				)
		)

if test "x$with_openssl" = "xno"; then
    AC_MSG_WARN( [Building without OpenSSL; disabling iperf_auth functionality.] )
else
    # Check for OPENSSL support
    havs_ssl=false
    AX_CHECK_OPENSSL(
        [ AC_DEFINE([HAVE_SSL], [1], [OpenSSL Is Available])
          have_ssl=true ],
	[ if test "x$with_openssl" != "x"; then
	  AC_MSG_FAILURE([--with-openssl was given, but test for OpenSSL failed])
	  fi ]
    )
    if $have_ssl; then
        case $host in
           *-*-cygwin)
             CFLAGS="$CFLAGS -DNOCRYPT"
             ;;
        esac
        LDFLAGS="$LDFLAGS $OPENSSL_LDFLAGS"
        LIBS="$OPENSSL_LIBS $LIBS"
        CPPFLAGS="$OPENSSL_INCLUDES $CPPFLAGS"
    fi
fi

# Check for TCP_CONGESTION sockopt (believed to be Linux and FreeBSD only)
AC_CACHE_CHECK([TCP_CONGESTION socket option],
[iperf3_cv_header_tcp_congestion],
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <netinet/tcp.h>]],
                   [[int foo = TCP_CONGESTION;]])],
  iperf3_cv_header_tcp_congestion=yes,
  iperf3_cv_header_tcp_congestion=no))
if test "x$iperf3_cv_header_tcp_congestion" = "xyes"; then
    AC_DEFINE([HAVE_TCP_CONGESTION], [1], [Have TCP_CONGESTION sockopt.])
fi

# Check for TCP_USER_TIMEOUT sockopt (believed to be Linux 2.6.37+ only)
AC_CACHE_CHECK([TCP_USER_TIMEOUT socket option],
[iperf3_cv_header_tcp_user_timeout],
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <netinet/tcp.h>]],
                   [[int foo = TCP_USER_TIMEOUT;]])],
  iperf3_cv_header_tcp_user_timeout=yes,
  iperf3_cv_header_tcp_user_timeout=no))
if test "x$iperf3_cv_header_tcp_user_timeout" = "xyes"; then
    AC_DEFINE([HAVE_TCP_USER_TIMEOUT], [1], [Have TCP_USER_TIMEOUT sockopt.])
fi

# Check for IPv6 flowlabel support (believed to be Linux only)
# We check for IPV6_FLOWLABEL_MGR in <linux/in6.h> even though we
# don't use that file directly (we have our own stripped-down
# copy, see src/flowlabel.h for more details).
AC_CACHE_CHECK([IPv6 flowlabel support],
[iperf3_cv_header_flowlabel],
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <sys/types.h>
                     #include <linux/in6.h>]],
                   [[int foo = IPV6_FLOWLABEL_MGR;]])],
  iperf3_cv_header_flowlabel=yes,
  iperf3_cv_header_flowlabel=no))
if test "x$iperf3_cv_header_flowlabel" = "xyes"; then
    AC_DEFINE([HAVE_FLOWLABEL], [1], [Have IPv6 flowlabel support.])
fi

# Check for CPU affinity support.  FreeBSD and Linux do this differently
# unfortunately so we have to check separately for each of them.
# FreeBSD uses cpuset_setaffinity while Linux uses sched_setaffinity.
# Define HAVE_CPU_AFFINITY to indicate the CPU affinity setting as a
# generic concept is available.
AC_CHECK_FUNCS([cpuset_setaffinity sched_setaffinity SetProcessAffinityMask],
	       AC_DEFINE([HAVE_CPU_AFFINITY], [1], 
	 	         [Have CPU affinity support.]))

# Check for daemon().  Most systems have this but a few (IRIX) don't.
AC_CHECK_FUNCS([daemon])

# Check for sendfile support.  FreeBSD, Linux, and MacOS all support
# this system call, but they're all different in terms of what headers
# it needs and what arguments it expects.
AC_CHECK_FUNCS([sendfile])

# Check for getline support, used as a part of authenticated
# connections.
AC_CHECK_FUNCS([getline])

# Check for packet pacing socket option (Linux only for now).
AC_CACHE_CHECK([SO_MAX_PACING_RATE socket option],
[iperf3_cv_header_so_max_pacing_rate],
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <sys/socket.h>]],
                   [[int foo = SO_MAX_PACING_RATE;]])],
  iperf3_cv_header_so_max_pacing_rate=yes,
  iperf3_cv_header_so_max_pacing_rate=no))
if test "x$iperf3_cv_header_so_max_pacing_rate" = "xyes"; then
    AC_DEFINE([HAVE_SO_MAX_PACING_RATE], [1], [Have SO_MAX_PACING_RATE sockopt.])
fi

# Check for SO_BINDTODEVICE sockopt (believed to be Linux only)
AC_CACHE_CHECK([SO_BINDTODEVICE socket option],
[iperf3_cv_header_so_bindtodevice],
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <sys/socket.h>]],
                   [[int foo = SO_BINDTODEVICE;]])],
  iperf3_cv_header_so_bindtodevice=yes,
  iperf3_cv_header_so_bindtodevice=no))
if test "x$iperf3_cv_header_so_bindtodevice" = "xyes"; then
    AC_DEFINE([HAVE_SO_BINDTODEVICE], [1], [Have SO_BINDTODEVICE sockopt.])
fi

# Check for IP_MTU_DISCOVER (mostly on Linux)
AC_CACHE_CHECK([IP_MTU_DISCOVER socket option],
[iperf3_cv_header_ip_mtu_discover],
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <sys/types.h>
                     #include <sys/socket.h>
                     #include <netinet/in.h>]],
                   [[int foo = IP_MTU_DISCOVER;]])],
  iperf3_cv_header_ip_mtu_discover=yes,
  iperf3_cv_header_ip_mtu_discover=no))
if test "x$iperf3_cv_header_ip_mtu_discover" = "xyes"; then
    AC_DEFINE([HAVE_IP_MTU_DISCOVER], [1], [Have IP_MTU_DISCOVER sockopt.])
fi

# Check for IP_DONTFRAG (BSD?)
AC_CACHE_CHECK([IP_DONTFRAG socket option],
[iperf3_cv_header_ip_dontfrag],
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <sys/types.h>
                     #include <sys/socket.h>
                     #include <netinet/in.h>]],
                   [[int foo = IP_DONTFRAG;]])],
  iperf3_cv_header_ip_dontfrag=yes,
  iperf3_cv_header_ip_dontfrag=no))
if test "x$iperf3_cv_header_ip_dontfrag" = "xyes"; then
    AC_DEFINE([HAVE_IP_DONTFRAG], [1], [Have IP_DONTFRAG sockopt.])
fi

# Check for IP_DONTFRAGMENT (Windows?)
AC_CACHE_CHECK([IP_DONTFRAGMENT socket option],
[iperf3_cv_header_ip_dontfragment],
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <sys/types.h>
                     #include <sys/socket.h>
                     #include <netinet/in.h>]],
                   [[int foo = IP_DONTFRAGMENT;]])],
  iperf3_cv_header_ip_dontfragment=yes,
  iperf3_cv_header_ip_dontfragment=no))
if test "x$iperf3_cv_header_ip_dontfragment" = "xyes"; then
    AC_DEFINE([HAVE_IP_DONTFRAGMENT], [1], [Have IP_DONTFRAGMENT sockopt.])
fi

# Check for IP DF support
AC_CACHE_CHECK([any kind of DF socket option],
[iperf3_cv_header_dontfragment],
[if test "x$iperf3_cv_header_ip_mtu_discover" = "xyes" -o "x$iperf3_cv_header_ip_dontfrag" = "xyes" -o "x$iperf3_cv_header_ip_dontfragment" = "xyes"; then
  iperf3_cv_header_dontfragment=yes
else
  iperf3_cv_header_dontfragment=no
fi])

if test "x$iperf3_cv_header_dontfragment" = "xyes"; then
    AC_DEFINE([HAVE_DONT_FRAGMENT], [1], [Have IP_MTU_DISCOVER/IP_DONTFRAG/IP_DONTFRAGMENT sockopt.])
fi

#
# Check for tcpi_snd_wnd in struct tcp_info
#
AC_CHECK_MEMBER([struct tcp_info.tcpi_snd_wnd],
[iperf3_cv_header_tcp_info_snd_wnd=yes], [iperf3_cv_header_tcp_info_snd_wnd=no],
[#ifdef HAVE_LINUX_TCP_H
#include <linux/tcp.h>
#else
#include <sys/types.h>
#include <netinet/tcp.h>
#endif
])

if test "x$iperf3_cv_header_tcp_info_snd_wnd" = "xyes"; then
  AC_DEFINE([HAVE_TCP_INFO_SND_WND], [1], [Have tcpi_snd_wnd field in tcp_info.])
fi

# Check if we need -lrt for clock_gettime
AC_SEARCH_LIBS(clock_gettime, [rt posix4])
# Check for clock_gettime support
AC_CHECK_FUNCS([clock_gettime])

AC_CONFIG_FILES([Makefile src/Makefile src/version.h examples/Makefile iperf3.spec])
AC_OUTPUT
