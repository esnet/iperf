# Initialize the autoconf system for the specified tool, version and mailing list
AC_INIT(iperf, 3.0a5, https://code.google.com/p/iperf/)

# Specify where the auxiliary files created by configure should go. The config
# directory is picked so that they don't clutter up more useful directories.
AC_CONFIG_AUX_DIR(config)


# Initialize the automake system
AM_INIT_AUTOMAKE

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

AC_OUTPUT([Makefile src/Makefile])
