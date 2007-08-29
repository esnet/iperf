noinst_LIBRARIES = libcompat.a

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-I$(top_srcdir)/include \
	@STRIP_END@

libcompat_a_SOURCES = \
		      Thread.c \
		      error.c \
		      delay.cpp \
		      gettimeofday.c \
		      inet_ntop.c \
		      inet_pton.c \
		      signal.c \
		      snprintf.c \
		      string.c
