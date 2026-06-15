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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

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

static void
read_exact(int fd, void *buf, size_t len)
{
    char *p = buf;
    ssize_t got;

    while (len > 0) {
        got = read(fd, p, len);
        assert(got > 0);
        p += got;
        len -= (size_t) got;
    }
}

static void
write_exact(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    ssize_t sent;

    while (len > 0) {
        sent = write(fd, p, len);
        assert(sent > 0);
        p += sent;
        len -= (size_t) sent;
    }
}

static void
wait_for_child(pid_t pid)
{
    int status;

    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
}

static void
socks5_proxy_stub(int fd)
{
    unsigned char greeting[3];
    unsigned char method_reply[] = { 0x05, 0x00 };
    unsigned char header[5];
    unsigned char host[14];
    unsigned char port[2];
    unsigned char success[] = {
        0x05, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00
    };

    read_exact(fd, greeting, sizeof(greeting));
    assert(memcmp(greeting, "\x05\x01\x00", sizeof(greeting)) == 0);
    write_exact(fd, method_reply, sizeof(method_reply));

    read_exact(fd, header, sizeof(header));
    assert(memcmp(header, "\x05\x01\x00\x03\x0e", sizeof(header)) == 0);
    read_exact(fd, host, sizeof(host));
    assert(memcmp(host, "target.example", sizeof(host)) == 0);
    read_exact(fd, port, sizeof(port));
    assert(port[0] == 0x14);
    assert(port[1] == 0x51);

    write_exact(fd, success, sizeof(success));
}

static void
http_proxy_stub(int fd)
{
    char request[256];
    char response[] = "HTTP/1.1 200 Connection Established\r\n\r\n";
    ssize_t got;

    got = read(fd, request, sizeof(request) - 1);
    assert(got > 0);
    request[got] = '\0';
    assert(strstr(request, "CONNECT target.example:5201 HTTP/1.1\r\n") != NULL);
    assert(strstr(request, "Host: target.example:5201\r\n") != NULL);

    write_exact(fd, response, strlen(response));
}

static void
run_proxy_stub(void (*stub)(int), struct iperf_settings *settings)
{
    int fds[2];
    pid_t pid;

    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        stub(fds[1]);
        close(fds[1]);
        exit(0);
    }

    close(fds[1]);
    assert(iperf_proxy_handshake(fds[0], settings, "target.example", 5201) == 0);
    close(fds[0]);
    wait_for_child(pid);
}

static void
test_proxy_handshake(void)
{
    struct iperf_settings settings;

    memset(&settings, 0, sizeof(settings));
    assert(iperf_parse_proxy_url("socks5://127.0.0.1:1080", &settings) == 0);
    run_proxy_stub(socks5_proxy_stub, &settings);
    clear_settings(&settings);

    assert(iperf_parse_proxy_url("http://127.0.0.1:8080", &settings) == 0);
    run_proxy_stub(http_proxy_stub, &settings);
    clear_settings(&settings);
}

int
main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    test_parse_proxy_url();
    test_proxy_handshake();
    return 0;
}
