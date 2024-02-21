LOCAL_PATH := /tmp/iperf

include $(CLEAR_VARS)
include $(BUILD_HOST_SHARED_LIBRARY)

LOCAL_MODULE := iperf3.16
LOCAL_MODULE_TAGS := dev
LOCAL_CFLAGS := -DHAVE_CONFIG_H -UAF_INET6 -w -Wno-error=format-security -funwind-tables -D_GNU_SOURCE
LOCAL_LDLIBS := -llog
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

# Define the source files
LOCAL_SRC_FILES := \
    src/main.c \
    src/cjson.c \
    src/dscp.c \
    src/iperf_api.c \
    src/iperf_auth.c \
    src/iperf_client_api.c \
    src/iperf_error.c \
    src/iperf_locale.c \
    src/iperf_sctp.c \
    src/iperf_server_api.c \
    src/iperf_tcp.c \
    src/iperf_time.c \
    src/iperf_pthread.c \
    src/iperf_udp.c \
    src/iperf_util.c \
    src/net.c \
    src/tcp_info.c \
    src/timer.c \
    src/units.c \
    src/de_fraunhofer_fokus_OpenMobileNetworkToolkit_Iperf3Worker.c

LOCAL_DISABLE_FORMAT_STRING_CHECKS := true

include $(BUILD_SHARED_LIBRARY)
