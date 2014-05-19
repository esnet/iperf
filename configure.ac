# Initialize the autoconf system for the specified tool, version and mailing list
AC_INIT(iperf, 3-CURRENT, https://github.com/esnet/iperf, iperf, http://software.es.net/iperf/)

# Specify where the auxiliary files created by configure should go. The config
# directory is picked so that they don't clutter up more useful directories.
AC_CONFIG_AUX_DIR(config)


# Initialize the automake system
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(src/iperf_config.h)

AC_CANONICAL_HOST

# Checks for tools: c compiler, ranlib (used for creating static libraries),
# symlinks and libtool
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LN_S
AC_PROG_LIBTOOL

# Add -Wall if we are using GCC.
if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall"
fi

# Checks for header files.
AC_HEADER_STDC

# Check for systems which need -lsocket and -lnsl
#AX_LIB_SOCKET_NSL

# Solaris puts nanosleep in -lrt
AC_SEARCH_LIBS(nanosleep, [rt], [], [
echo "nanosleep() required for timing operations."
exit 1
])

# Solaris puts hstrerror in -lresolv
AC_SEARCH_LIBS(hstrerror, [resolv], [], [
echo "nanosleep() required for timing operations."
exit 1
])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Check for SCTP support
AC_CHECK_HEADERS([netinet/sctp.h],
		 AC_DEFINE([HAVE_SCTP], [1], [Have SCTP support.]))

# Check for TCP_CONGESTION sockopt (believed to be Linux only)
AC_CACHE_CHECK([TCP_CONGESTION socket option],
[iperf3_cv_header_tcp_congestion],
AC_EGREP_CPP(yes,
[#include <netinet/tcp.h>
#ifdef TCP_CONGESTION
  yes
#endif
],iperf3_cv_header_tcp_congestion=yes,iperf3_cv_header_tcp_congestion=no))
if test "x$iperf3_cv_header_tcp_congestion" = "xyes"; then
    AC_DEFINE([HAVE_TCP_CONGESTION], [1], [Have TCP_CONGESTION sockopt.])
fi

# Check for IPv6 flowlabel support (believed to be Linux only)
# We check for IPV6_FLOWLABEL_MGR in <linux/in6.h> even though we
# don't use that file directly (we have our own stripped-down
# copy, see src/flowlabel.h for more details).
AC_CACHE_CHECK([IPv6 flowlabel support],
[iperf3_cv_header_flowlabel],
AC_EGREP_CPP(yes,
[#include <sys/types.h>
#include <linux/in6.h>
#ifdef IPV6_FLOWLABEL_MGR
  yes
#endif
],iperf3_cv_header_flowlabel=yes,iperf3_cv_header_flowlabel=no))
if test "x$iperf3_cv_header_flowlabel" = "xyes"; then
    AC_DEFINE([HAVE_FLOWLABEL], [1], [Have IPv6 flowlabel support.])
fi

# Check for CPU affinity support.  FreeBSD and Linux do this differently
# unfortunately so we have to check separately for each of them.
# FreeBSD uses cpuset_setaffinity while Linux uses sched_setaffinity.
# Define HAVE_CPU_AFFINITY to indicate the CPU affinity setting as a
# generic concept is available.
AC_CHECK_FUNCS([cpuset_setaffinity sched_setaffinity],
	       AC_DEFINE([HAVE_CPU_AFFINITY], [1], 
	 	         [Have CPU affinity support.]))

# Check for sendfile support.  FreeBSD, Linux, and MacOS all support
# this system call, but they're all different in terms of what headers
# it needs and what arguments it expects.
AC_CHECK_FUNCS([sendfile])

AC_OUTPUT([Makefile src/Makefile src/version.h examples/Makefile])
