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
#include "iperf_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "iperf.h"
#include "iperf_api.h"
#include "iperf_vsock.h"


#if !defined(HAVE_VSOCK)

static int
iperf_vsock_notsupported(void)
{
	fprintf(stderr, "VSOCK not supported\n");
	i_errno = IENOVSOCK;
	return -1;
}

int
iperf_vsock_accept(struct iperf_test *test)
{
	return iperf_vsock_notsupported();
}

int
iperf_vsock_recv(struct iperf_stream *sp)
{
	return iperf_vsock_notsupported();
}

int
iperf_vsock_send(struct iperf_stream *sp)
{
	return iperf_vsock_notsupported();
}

int
iperf_vsock_listen(struct iperf_test *test)
{
	return iperf_vsock_notsupported();
}

int
iperf_vsock_connect(struct iperf_test *test)
{
	return iperf_vsock_notsupported();
}

int
iperf_vsock_init(struct iperf_test *test)
{
	return iperf_vsock_notsupported();
}

#else /* HAVE_VSOCK */

#include <sys/socket.h>
#include <linux/vm_sockets.h>

#include "net.h"

static int
parse_cid(const char *cid_str)
{
	char *end = NULL;
	long cid = strtol(cid_str, &end, 10);
	if (cid_str != end && *end == '\0') {
		return cid;
	} else {
		fprintf(stderr, "VSOCK - invalid cid: %s\n", cid_str);
		return -1;
	}
}

int
iperf_vsock_accept(struct iperf_test *test)
{
	int fd;
	signed char rbuf = ACCESS_DENIED;
	char cookie[COOKIE_SIZE];
	struct sockaddr_vm sa_client;
	socklen_t socklen_client = sizeof(sa_client);

	fd = accept(test->listener, (struct sockaddr *) &sa_client,
		    &socklen_client);
	if (fd < 0) {
		i_errno = IESTREAMCONNECT;
		return -1;
	}

	if (Nread(fd, cookie, COOKIE_SIZE, Pvsock) < 0) {
		i_errno = IERECVCOOKIE;
		return -1;
	}

	if (strcmp(test->cookie, cookie) != 0) {
		if (Nwrite(fd, (char*) &rbuf, sizeof(rbuf), Pvsock) < 0) {
			i_errno = IESENDMESSAGE;
			return -1;
		}
		close(fd);
	}

	return fd;
}

int
iperf_vsock_recv(struct iperf_stream *sp)
{
	int r;

	r = Nread(sp->socket, sp->buffer, sp->settings->blksize, Pvsock);
	if (r < 0) {
		/*
		 * VSOCK can return -1 with errno = ENOTCONN if the remote host
		 * closes the connection, but in the iperf3 code we expect
		 * return 0 in this case.
		 */
		if (errno == ENOTCONN)
			return 0;
		else
			return r;
	}

	/* Only count bytes received while we're in the correct state. */
	if (sp->test->state == TEST_RUNNING) {
		sp->result->bytes_received += r;
		sp->result->bytes_received_this_interval += r;
	}
	else if (sp->test->debug) {
			printf("Late receive, state = %d\n", sp->test->state);
	}

	return r;
}

int
iperf_vsock_send(struct iperf_stream *sp)
{
	int r;

	r = Nwrite(sp->socket, sp->buffer, sp->settings->blksize, Pvsock);
	if (r < 0) {
		/*
		 * VSOCK can return -1 with errno = ENOTCONN if the remote host
		 * closes the connection, but in the iperf3 code we expect
		 * return 0 in this case.
		 */
		if (errno == ENOTCONN)
			return 0;
		else
			return r;
	}

	sp->result->bytes_sent += r;
	sp->result->bytes_sent_this_interval += r;

	return r;
}

int
iperf_vsock_listen(struct iperf_test *test)
{
	/* We use the same socket used for control path (test->listener) */
	return test->listener;
}

int
iperf_vsock_connect(struct iperf_test *test)
{
	int fd, cid;
	struct sockaddr_vm sa = {
		.svm_family = AF_VSOCK,
	};

	cid = parse_cid(test->server_hostname);
	if (cid < 0) {
		return -1;
	}

	sa.svm_cid = cid;
	sa.svm_port = test->server_port;

	fd = socket(AF_VSOCK, SOCK_STREAM, 0);
	if (fd < 0) {
		goto err;
	}

	if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
		goto err_close;
	}

	/* Send cookie for verification */
	if (Nwrite(fd, test->cookie, COOKIE_SIZE, Pvsock) < 0) {
		goto err_close;
	}

	return fd;

err_close:
	close(fd);
err:
	i_errno = IESTREAMCONNECT;
	return -1;
}

int
iperf_vsock_init(struct iperf_test *test)
{
	return 0;
}

#endif /* HAVE_VSOCK */
