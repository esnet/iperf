# TCP Client Proxy Design

## Goal

Add proxy support to the iperf3 client for TCP tests only. The client can tunnel its control connection and TCP data streams through either a SOCKS5 proxy or an HTTP CONNECT proxy.

## User Interface

Add one client option:

```text
--proxy socks5://host:port
--proxy http://host:port
```

The first version does not support proxy authentication. Proxy use with server mode, UDP mode, or SCTP mode is rejected during argument validation.

## Architecture

The existing TCP client path creates sockets, applies socket options, connects, then sends iperf protocol data. Proxy support should keep that structure:

1. Resolve and connect the socket to the proxy endpoint.
2. Run the proxy handshake on the connected socket, asking the proxy to connect to the iperf server host and port.
3. Return the same socket to the existing iperf control or TCP stream code.

This keeps existing bind, client port, buffer, MSS, keepalive, and TCP option behavior attached to the actual local socket while changing only the peer endpoint and initial handshake.

## Components

- `struct iperf_settings` stores parsed proxy configuration: type, host, and port.
- `iperf_parse_arguments()` parses `--proxy`, validates mode compatibility, and exposes help text.
- A small proxy helper handles parsing and handshakes:
  - SOCKS5 no-auth greeting and CONNECT request.
  - HTTP/1.1 CONNECT request and status-line validation for 2xx responses.
- TCP client control connection and TCP stream connection call the helper after connecting to the proxy and before sending iperf protocol bytes.

## Error Handling

Invalid proxy URLs are reported as parameter errors. Network failures to the proxy reuse existing connect errors where possible. Proxy negotiation failures use a dedicated iperf error message so users can distinguish proxy rejection from a normal server connection failure.

## Testing

Add unit-style tests for proxy URL parsing and request generation/response handling where practical. Add argument parsing tests for accepted proxy values and rejected UDP/SCTP/server combinations. Run the existing `make check` suite after implementation.
