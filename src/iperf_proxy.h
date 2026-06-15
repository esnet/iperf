/*
 * iperf, Copyright (c) 2026, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#ifndef __IPERF_PROXY_H
#define __IPERF_PROXY_H

struct iperf_settings;

enum iperf_proxy_type {
    IPERF_PROXY_NONE = 0,
    IPERF_PROXY_SOCKS5,
    IPERF_PROXY_HTTP
};

int iperf_parse_proxy_url(const char *url, struct iperf_settings *settings);
void iperf_clear_proxy_settings(struct iperf_settings *settings);
int iperf_proxy_handshake(int fd, const struct iperf_settings *settings, const char *target_host, int target_port);

#endif /* __IPERF_PROXY_H */
