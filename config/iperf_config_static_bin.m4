# Also link binaries as static
AC_ARG_ENABLE([static-bin],
    AS_HELP_STRING([--enable-static-bin], [link iperf binary statically]),
    [enable_static_bin=yes
     AC_DISABLE_SHARED],
    [:])
AM_CONDITIONAL([ENABLE_STATIC_BIN], [test x$enable_static_bin = xno])

AS_IF([test "x$enable_static_bin" == xyes],
 [LDFLAGS="$LDFLAGS --static"]
 [])
