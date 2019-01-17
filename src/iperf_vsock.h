/*
 * Copyright (c) 2019 Red Hat, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * (1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * (2) Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/ or other materials provided with the distribution.
 *
 * (3) Neither the name of Red Hat, Inc. nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef        IPERF_VSOCK_H
#define        IPERF_VSOCK_H

/**
 * iperf_vsock_accept -- accepts a new VSOCK connection
 * on the listener socket for VSOCK tests
 *
 *returns file descriptor on success, -1 on error
 *
 */
int iperf_vsock_accept(struct iperf_test *);

/**
 * iperf_vsock_recv -- receives the data for VSOCK tests
 *
 *returns bytes received on success, < 0 on error
 *
 */
int iperf_vsock_recv(struct iperf_stream *);

/**
 * iperf_vsock_send -- sends the data for VSOCK tests
 *
 *returns bytes sent on success, < 0 on error
 *
 */
int iperf_vsock_send(struct iperf_stream *);

/**
 * iperf_vsock_listen -- get listener socket for VSOCK tests
 * We use the same socket used for control path (test->listener)
 *
 *returns file descriptor of listener_socket
 *
 */
int iperf_vsock_listen(struct iperf_test *);

/**
 * iperf_vsock_connect -- create a new VSOCK connection to the server
 * for VSOCK tests
 *
 *returns file descriptor on success, -1 on error
 *
 */
int iperf_vsock_connect(struct iperf_test *);

/**
 * iperf_vsock_init -- nop
 *
 *returns 0
 */
int iperf_vsock_init(struct iperf_test *test);

#endif /* IPERF_VSOCK_H */
