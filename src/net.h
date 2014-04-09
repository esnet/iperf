/*
 * Copyright (c) 2009-2014, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef __NET_H
#define __NET_H

int netdial(int domain, int proto, char *local, char *server, int port);
int netannounce(int domain, int proto, char *local, int port);
int Nread(int fd, char *buf, size_t count, int prot);
int Nwrite(int fd, const char *buf, size_t count, int prot) /* __attribute__((hot)) */;
int has_sendfile(void);
int Nsendfile(int fromfd, int tofd, const char *buf, size_t count) /* __attribute__((hot)) */;
int getsock_tcp_mss(int inSock);
int set_tcp_options(int sock, int no_delay, int mss);
int setnonblocking(int fd, int nonblocking);
int getsockdomain(int sock);

#define NET_SOFTERROR -1
#define NET_HARDERROR -2

#endif /* __NET_H */
