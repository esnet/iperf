lib_LIBRARIES           = libiperf.a                                    # Build and install a static iperf library
bin_PROGRAMS            = iperf3                                        # Build and install an iperf binary
noinst_PROGRAMS         = t_timer t_units t_uuid iperf3_profile         # Build, but don't install the test programs and a profiled version of iperf3
include_HEADERS         = iperf_api.h iperf_server_api.h                # Defines the headers that get installed with the program


# Specify the source files and flags for the iperf library
libiperf_a_SOURCES      = \
                        iperf_api.c \
                        iperf_server_api.c \
                        iperf_tcp.c \
                        iperf_udp.c \
                        timer.c \
                        net.c \
                        tcp_window_size.c \
                        units.c \
                        uuid.c \
                        tcp_info.c \
                        locale.c \
                        iperf_api.h \
                        iperf.h \
                        iperf_server_api.h \
                        iperf_tcp.h \
                        iperf_udp.h \
                        locale.h \
                        net.h \
                        tcp_window_size.h \
                        timer.h \
                        units.h \
                        uuid.h \
                        version.h
libiperf_a_CFLAGS       = -I../include

# Specify the sources and various flags for the iperf binary
iperf3_SOURCES          = main.c
iperf3_CFLAGS           = -g -Wall
iperf3_LDADD            = libiperf.a

# Linux installs require the uuid library explicitly linked in
if USE_UUID_LIBRARY
iperf3_LDFLAGS          = -luuid
else
iperf3_LDFLAGS          = 
endif

# Specify the sources and various flags for the profiled iperf binary. This
# binary recompiles all the source files to make sure they are all profiled.
iperf3_profile_SOURCES  = main.c \
                        iperf_api.c \
                        iperf_server_api.c \
                        iperf_tcp.c \
                        iperf_udp.c \
                        timer.c \
                        net.c \
                        tcp_window_size.c \
                        units.c \
                        uuid.c \
                        tcp_info.c \
                        locale.c \
                        iperf_api.h \
                        iperf.h \
                        iperf_server_api.h \
                        iperf_tcp.h \
                        iperf_udp.h \
                        locale.h \
                        net.h \
                        tcp_window_size.h \
                        timer.h \
                        units.h \
                        uuid.h \
                        version.h
iperf3_profile_CFLAGS   = -pg -Wall
iperf3_profile_LDADD    = libiperf.a

# Linux installs require the uuid library explicitly linked in
if USE_UUID_LIBRARY
iperf3_profile_LDFLAGS  = -luuid
else
iperf3_profile_LDFLAGS  =
endif

# Specify the sources and various flags for the test cases
t_timer_SOURCES         = t_timer.c
t_timer_CFLAGS          = -g -Wall
t_timer_LDFLAGS         =
t_timer_LDADD           = libiperf.a

t_units_SOURCES         = t_units.c
t_units_CFLAGS          = -g -Wall
t_units_LDFLAGS         =
t_units_LDADD           = libiperf.a

t_uuid_SOURCES          = t_uuid.c
t_uuid_CFLAGS           = -g -Wall
t_uuid_LDFLAGS          =
t_uuid_LDADD            = libiperf.a




# Specify which tests to run during a "make check"
TESTS                   = \
                        t_timer \
                        t_units \
                        t_uuid

dist_man1_MANS          = iperf.1
