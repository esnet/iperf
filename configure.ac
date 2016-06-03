# iperf, Copyright (c) 2014, 2015, The Regents of the University of
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
AC_INIT(iperf, 3.0.11, https://github.com/esnet/iperf, iperf, http://software.es.net/iperf/)

# Specify where the auxiliary files created by configure should go. The config
# directory is picked so that they don't clutter up more useful directories.
AC_CONFIG_AUX_DIR(config)


# Initialize the automake system
AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE
AM_CONFIG_HEADER(src/config.h)

AC_CANONICAL_HOST

# Checks for tools: c compiler, ranlib (used for creating static libraries),
# symlinks and libtool
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LN_S
AC_PROG_LIBTOOL


# Sets a conditional makefile variable so that certain Makefile tasks will be
# performed only on linux (currently, add -luuid to LD_FLAGS)
AM_CONDITIONAL(LINUX, [case $host_os in linux*) true;; *) false;; esac]) 

# Add -Wall if we are using GCC.
if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall"
fi

# Checks for header files.
AC_HEADER_STDC

# Check for systems which need -lsocket and -lnsl
#AX_LIB_SOCKET_NSL

# Check for the math library (needed by cjson on some platforms)
AC_SEARCH_LIBS(floor, [m], [], [
echo "floor()"
exit 1
])

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

AC_OUTPUT([Makefile src/Makefile src/version.h examples/Makefile iperf3.spec])
