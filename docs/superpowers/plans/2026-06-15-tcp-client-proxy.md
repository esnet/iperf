# TCP Client Proxy Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add SOCKS5 and HTTP CONNECT proxy support for iperf3 TCP clients.

**Architecture:** Store parsed proxy settings in `struct iperf_settings`. A new `iperf_proxy` helper owns proxy URL parsing and wire handshakes. The existing control and TCP stream connection paths connect to the proxy endpoint when configured, then ask the helper to establish a tunnel to the requested iperf server before sending iperf protocol bytes.

**Tech Stack:** C, autotools, existing iperf3 test binaries, POSIX sockets.

---

## File Structure

- Create `src/iperf_proxy.h`: proxy type definitions and helper declarations.
- Create `src/iperf_proxy.c`: URL parsing, SOCKS5 handshake, HTTP CONNECT handshake.
- Modify `src/Makefile.am`: compile the new helper into `libiperf`.
- Modify `src/iperf.h`: add proxy fields to `struct iperf_settings`.
- Modify `src/iperf_api.h`: add `OPT_PROXY`, `IEPROXYURL`, and `IEPROXYHANDSHAKE`.
- Modify `src/iperf_api.c`: parse `--proxy`, validate TCP-client-only scope, initialize and free proxy fields.
- Modify `src/iperf_client_api.c`: connect the control channel to proxy when configured and run proxy handshake.
- Modify `src/iperf_tcp.c`: connect TCP streams to proxy when configured and run proxy handshake before cookie write.
- Modify `src/iperf_error.c`: add user-facing proxy errors.
- Modify `src/iperf_locale.c`: add help text.
- Modify `src/t_api.c`: add parser tests for valid and invalid proxy combinations.
- Create `src/t_proxy.c`: unit tests for proxy URL parsing and handshake helpers using socketpair/fork.

### Task 1: Proxy URL Parser Tests

**Files:**
- Create: `src/iperf_proxy.h`
- Create: `src/iperf_proxy.c`
- Create: `src/t_proxy.c`
- Modify: `src/Makefile.am`

- [ ] **Step 1: Write the failing parser tests**

Create `src/t_proxy.c` with tests that call `iperf_parse_proxy_url()` for:

```c
assert(iperf_parse_proxy_url("socks5://127.0.0.1:1080", &settings) == 0);
assert(settings.proxy_type == IPERF_PROXY_SOCKS5);
assert(strcmp(settings.proxy_host, "127.0.0.1") == 0);
assert(settings.proxy_port == 1080);

assert(iperf_parse_proxy_url("http://proxy.example:8080", &settings) == 0);
assert(settings.proxy_type == IPERF_PROXY_HTTP);
assert(strcmp(settings.proxy_host, "proxy.example") == 0);
assert(settings.proxy_port == 8080);

assert(iperf_parse_proxy_url("https://proxy.example:443", &settings) == -1);
assert(iperf_parse_proxy_url("socks5://proxy.example", &settings) == -1);
assert(iperf_parse_proxy_url("http://proxy.example:70000", &settings) == -1);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `rtk make -C src t_proxy`

Expected: FAIL because `t_proxy` and `iperf_parse_proxy_url()` are not defined yet.

- [ ] **Step 3: Implement minimal parser**

Add `src/iperf_proxy.h` with:

```c
enum iperf_proxy_type {
    IPERF_PROXY_NONE = 0,
    IPERF_PROXY_SOCKS5,
    IPERF_PROXY_HTTP
};

int iperf_parse_proxy_url(const char *url, struct iperf_settings *settings);
void iperf_clear_proxy_settings(struct iperf_settings *settings);
```

Add `src/iperf_proxy.c` parser support for `socks5://host:port` and `http://host:port`.

- [ ] **Step 4: Run parser test to verify it passes**

Run: `rtk make -C src t_proxy && rtk ./src/t_proxy`

Expected: PASS with exit code 0.

### Task 2: Proxy Argument Parsing

**Files:**
- Modify: `src/iperf_api.h`
- Modify: `src/iperf_api.c`
- Modify: `src/iperf_error.c`
- Modify: `src/iperf_locale.c`
- Modify: `src/t_api.c`

- [ ] **Step 1: Write failing argument tests**

Extend `src/t_api.c` with tests that create a fresh `iperf_test`, call `iperf_parse_arguments()`, and assert:

```c
char *ok_argv[] = {"iperf3", "-c", "server.example", "--proxy", "socks5://127.0.0.1:1080", NULL};
assert(iperf_parse_arguments(test, 5, ok_argv) == 0);
assert(test->settings->proxy_type == IPERF_PROXY_SOCKS5);

char *udp_argv[] = {"iperf3", "-c", "server.example", "-u", "--proxy", "http://127.0.0.1:8080", NULL};
assert(iperf_parse_arguments(test, 6, udp_argv) == -1);

char *server_argv[] = {"iperf3", "-s", "--proxy", "http://127.0.0.1:8080", NULL};
assert(iperf_parse_arguments(test, 4, server_argv) == -1);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `rtk make -C src t_api && rtk ./src/t_api`

Expected: FAIL because `--proxy` is unknown and proxy fields do not exist.

- [ ] **Step 3: Implement CLI wiring**

Add `OPT_PROXY`, include `iperf_proxy.h`, add `{"proxy", required_argument, NULL, OPT_PROXY}`, parse via `iperf_parse_proxy_url()`, mark `client_flag = 1`, and reject proxy use unless `test->role == 'c'` and protocol is TCP.

- [ ] **Step 4: Run argument tests to verify they pass**

Run: `rtk make -C src t_api && rtk ./src/t_api`

Expected: PASS with exit code 0.

### Task 3: Proxy Handshake Tests

**Files:**
- Modify: `src/t_proxy.c`
- Modify: `src/iperf_proxy.c`
- Modify: `src/iperf_proxy.h`

- [ ] **Step 1: Write failing SOCKS5 and HTTP handshake tests**

Use `socketpair(AF_UNIX, SOCK_STREAM, 0, fds)` and `fork()` a child proxy stub:

```c
assert(iperf_proxy_handshake(fds[0], &settings, "target.example", 5201) == 0);
```

The SOCKS5 stub verifies greeting `05 01 00`, replies `05 00`, verifies CONNECT command for `target.example:5201`, and replies success.

The HTTP stub reads a request containing `CONNECT target.example:5201 HTTP/1.1`, writes `HTTP/1.1 200 Connection Established\r\n\r\n`, and exits.

- [ ] **Step 2: Run test to verify it fails**

Run: `rtk make -C src t_proxy && rtk ./src/t_proxy`

Expected: FAIL because `iperf_proxy_handshake()` is not implemented.

- [ ] **Step 3: Implement handshakes**

Implement:

```c
int iperf_proxy_handshake(int fd, const struct iperf_settings *settings, const char *target_host, int target_port);
```

SOCKS5 supports no-auth CONNECT with IPv4, IPv6, and domain-name target encodings. HTTP sends a CONNECT request and accepts any 2xx status.

- [ ] **Step 4: Run handshake tests to verify they pass**

Run: `rtk make -C src t_proxy && rtk ./src/t_proxy`

Expected: PASS with exit code 0.

### Task 4: Connect Path Integration

**Files:**
- Modify: `src/iperf_client_api.c`
- Modify: `src/iperf_tcp.c`

- [ ] **Step 1: Write failing integration-oriented tests**

Extend `src/t_proxy.c` with helper checks that `iperf_proxy_handshake()` can be called repeatedly for control and stream-like sockets against the same proxy stub. This verifies independent tunnel setup per connection.

- [ ] **Step 2: Run test to verify it fails if handshake is skipped**

Run: `rtk make -C src t_proxy && rtk ./src/t_proxy`

Expected: FAIL before connection paths call the helper in production code review; unit test remains the executable safety net for repeated tunnels.

- [ ] **Step 3: Implement control connection integration**

In `iperf_connect()`, when `settings->proxy_type != IPERF_PROXY_NONE`, call `netdial()` with `proxy_host` and `proxy_port`, then call `iperf_proxy_handshake()` with `server_hostname` and `server_port`.

- [ ] **Step 4: Implement TCP stream integration**

In `iperf_tcp_connect()`, when proxy is configured, create/connect the socket to `proxy_host`/`proxy_port` while still applying current socket options. After `timeout_connect()`, call `iperf_proxy_handshake()` before writing the stream cookie.

- [ ] **Step 5: Run full verification**

Run: `rtk make -C src t_proxy t_api && rtk ./src/t_proxy && rtk ./src/t_api && rtk make check`

Expected: all commands pass.
