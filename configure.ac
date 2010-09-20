# Initialize the autoconf system for the specified tool, version and mailing list
AC_INIT(iperf, 3.0a4, iperf-users@lists.sourceforge.net)

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
AX_LIB_SOCKET_NSL

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

# Check for uuid.h and a valid libuuid
AC_CHECK_FUNC(uuid_create)
if test "${ac_cv_func_uuid_create}" = yes ; then
    AC_DEFINE(HAVE_UUID_CREATE, [], "specifies if the uuid_create function defined")
    use_uuid_library="no"
else
    AC_CHECK_FUNC(uuid_generate)
    if test "${ac_cv_func_uuid_generate}" = yes ; then
        AC_DEFINE(HAVE_UUID_GENERATE, [], "specifies if the uuid_generate function defined")
        use_uuid_library="no"
    else
        AC_CHECK_LIB(uuid, uuid_generate, ,
                        AC_MSG_ERROR([libuuid is not available]))
        AC_DEFINE(HAVE_UUID_GENERATE, [], "specifies if the uuid_generate function defined")
        use_uuid_library="yes"
    fi
fi

AM_CONDITIONAL(USE_UUID_LIBRARY, test "${use_uuid_library}" = yes)

AC_CHECK_HEADER(uuid.h)
if test "${ac_cv_header_uuid_h}" = yes ; then
    AC_DEFINE(HAVE_UUID_H, [], "specifies if the uuid.h header exists")
else
    AC_CHECK_HEADER(uuid/uuid.h)
    if test "${ac_cv_header_uuid_uuid_h}" = yes ; then
        AC_DEFINE(HAVE_UUID_UUID_H, [], "specifies if the uuid/uuid.h header exists")
    fi
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AC_OUTPUT([Makefile src/Makefile])
