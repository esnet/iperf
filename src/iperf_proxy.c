/*
 * iperf, Copyright (c) 2026, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */

#include "iperf_config.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iperf.h"
#include "iperf_proxy.h"

static int
proxy_protocol_error(void)
{
#ifdef EPROTO
    errno = EPROTO;
#else
    errno = EINVAL;
#endif
    return -1;
}

static int
read_full(int fd, void *buf, size_t len)
{
    char *p = buf;
    ssize_t got;

    while (len > 0) {
        got = read(fd, p, len);
        if (got <= 0)
            return -1;
        p += got;
        len -= (size_t) got;
    }
    return 0;
}

static int
write_full(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    ssize_t sent;

    while (len > 0) {
        sent = write(fd, p, len);
        if (sent <= 0)
            return -1;
        p += sent;
        len -= (size_t) sent;
    }
    return 0;
}

static int
parse_port(const char *s)
{
    char *end;
    long port;

    if (!s || !*s)
        return -1;
    errno = 0;
    port = strtol(s, &end, 10);
    if (errno || end == s || *end != '\0' || port < 1 || port > 65535)
        return -1;
    return (int) port;
}

void
iperf_clear_proxy_settings(struct iperf_settings *settings)
{
    if (!settings)
        return;
    free(settings->proxy_host);
    settings->proxy_host = NULL;
    settings->proxy_port = 0;
    settings->proxy_type = IPERF_PROXY_NONE;
}

int
iperf_parse_proxy_url(const char *url, struct iperf_settings *settings)
{
    const char *host_start;
    const char *host_end;
    const char *port_start;
    enum iperf_proxy_type type;
    char *host;
    size_t host_len;
    int port;

    if (!url || !settings)
        return -1;

    if (strncmp(url, "socks5://", 9) == 0) {
        type = IPERF_PROXY_SOCKS5;
        host_start = url + 9;
    } else if (strncmp(url, "http://", 7) == 0) {
        type = IPERF_PROXY_HTTP;
        host_start = url + 7;
    } else {
        return -1;
    }

    if (*host_start == '[') {
        host_start++;
        host_end = strchr(host_start, ']');
        if (!host_end || host_end[1] != ':')
            return -1;
        port_start = host_end + 2;
    } else {
        host_end = strrchr(host_start, ':');
        if (!host_end)
            return -1;
        port_start = host_end + 1;
    }

    host_len = (size_t) (host_end - host_start);
    if (host_len == 0 || host_len > 255)
        return -1;

    port = parse_port(port_start);
    if (port < 0)
        return -1;

    host = malloc(host_len + 1);
    if (!host)
        return -1;
    memcpy(host, host_start, host_len);
    host[host_len] = '\0';

    iperf_clear_proxy_settings(settings);
    settings->proxy_type = type;
    settings->proxy_host = host;
    settings->proxy_port = port;
    return 0;
}

static int
socks5_target_address(const char *target_host, unsigned char *buf, size_t *len)
{
    struct in_addr in4;
    struct in6_addr in6;
    size_t host_len;

    if (inet_pton(AF_INET, target_host, &in4) == 1) {
        buf[0] = 0x01;
        memcpy(buf + 1, &in4, sizeof(in4));
        *len = 1 + sizeof(in4);
        return 0;
    }
    if (inet_pton(AF_INET6, target_host, &in6) == 1) {
        buf[0] = 0x04;
        memcpy(buf + 1, &in6, sizeof(in6));
        *len = 1 + sizeof(in6);
        return 0;
    }

    host_len = strlen(target_host);
    if (host_len == 0 || host_len > 255)
        return -1;
    buf[0] = 0x03;
    buf[1] = (unsigned char) host_len;
    memcpy(buf + 2, target_host, host_len);
    *len = 2 + host_len;
    return 0;
}

static int
socks5_handshake(int fd, const char *target_host, int target_port)
{
    unsigned char greeting[] = { 0x05, 0x01, 0x00 };
    unsigned char method_reply[2];
    unsigned char request[4 + 1 + 255 + 2];
    unsigned char reply[4];
    unsigned char discard[255];
    size_t addr_len;
    size_t request_len;
    size_t bind_len;

    if (write_full(fd, greeting, sizeof(greeting)) < 0)
        return -1;
    if (read_full(fd, method_reply, sizeof(method_reply)) < 0)
        return -1;
    if (method_reply[0] != 0x05 || method_reply[1] != 0x00)
        return proxy_protocol_error();

    request[0] = 0x05;
    request[1] = 0x01;
    request[2] = 0x00;
    if (socks5_target_address(target_host, request + 3, &addr_len) < 0)
        return -1;
    request_len = 3 + addr_len;
    request[request_len++] = (unsigned char) ((target_port >> 8) & 0xff);
    request[request_len++] = (unsigned char) (target_port & 0xff);

    if (write_full(fd, request, request_len) < 0)
        return -1;
    if (read_full(fd, reply, sizeof(reply)) < 0)
        return -1;
    if (reply[0] != 0x05 || reply[1] != 0x00)
        return proxy_protocol_error();

    switch (reply[3]) {
        case 0x01:
            bind_len = 4;
            break;
        case 0x03:
            if (read_full(fd, discard, 1) < 0)
                return -1;
            bind_len = discard[0];
            break;
        case 0x04:
            bind_len = 16;
            break;
        default:
            return proxy_protocol_error();
    }
    if (read_full(fd, discard, bind_len + 2) < 0)
        return -1;
    return 0;
}

static int
http_connect_handshake(int fd, const char *target_host, int target_port)
{
    char request[1024];
    char response[4096];
    size_t used;
    int n;

    n = snprintf(request, sizeof(request),
                 "CONNECT %s:%d HTTP/1.1\r\n"
                 "Host: %s:%d\r\n"
                 "Proxy-Connection: Keep-Alive\r\n"
                 "\r\n",
                 target_host, target_port, target_host, target_port);
    if (n < 0 || (size_t) n >= sizeof(request))
        return -1;
    if (write_full(fd, request, (size_t) n) < 0)
        return -1;

    used = 0;
    while (used + 1 < sizeof(response)) {
        ssize_t got = read(fd, response + used, 1);
        if (got <= 0)
            return -1;
        used += (size_t) got;
        response[used] = '\0';
        if (used >= 4 && strstr(response, "\r\n\r\n") != NULL)
            break;
    }
    if (strncmp(response, "HTTP/", 5) != 0)
        return proxy_protocol_error();
    if (strstr(response, " 2") == NULL)
        return proxy_protocol_error();
    return 0;
}

int
iperf_proxy_handshake(int fd, const struct iperf_settings *settings, const char *target_host, int target_port)
{
    if (!settings || !target_host || target_port < 1 || target_port > 65535)
        return -1;

    switch (settings->proxy_type) {
        case IPERF_PROXY_SOCKS5:
            return socks5_handshake(fd, target_host, target_port);
        case IPERF_PROXY_HTTP:
            return http_connect_handshake(fd, target_host, target_port);
        default:
            errno = EINVAL;
            return -1;
    }
}
