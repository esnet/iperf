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
#include <stdlib.h>
#include <string.h>

#include "iperf.h"
#include "iperf_proxy.h"

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
