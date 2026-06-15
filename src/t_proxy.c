/*
 * iperf, Copyright (c) 2026, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */

#include <assert.h>
#include <string.h>

#include "iperf.h"
#include "iperf_proxy.h"

static void
clear_settings(struct iperf_settings *settings)
{
    iperf_clear_proxy_settings(settings);
    memset(settings, 0, sizeof(*settings));
}

static void
test_parse_proxy_url(void)
{
    struct iperf_settings settings;

    memset(&settings, 0, sizeof(settings));
    assert(iperf_parse_proxy_url("socks5://127.0.0.1:1080", &settings) == 0);
    assert(settings.proxy_type == IPERF_PROXY_SOCKS5);
    assert(strcmp(settings.proxy_host, "127.0.0.1") == 0);
    assert(settings.proxy_port == 1080);
    clear_settings(&settings);

    assert(iperf_parse_proxy_url("http://proxy.example:8080", &settings) == 0);
    assert(settings.proxy_type == IPERF_PROXY_HTTP);
    assert(strcmp(settings.proxy_host, "proxy.example") == 0);
    assert(settings.proxy_port == 8080);
    clear_settings(&settings);

    assert(iperf_parse_proxy_url("https://proxy.example:443", &settings) == -1);
    assert(iperf_parse_proxy_url("socks5://proxy.example", &settings) == -1);
    assert(iperf_parse_proxy_url("http://proxy.example:70000", &settings) == -1);
}

int
main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    test_parse_proxy_url();
    return 0;
}
