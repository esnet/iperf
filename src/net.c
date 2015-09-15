/*
 * iperf, Copyright (c) 2014, 2015, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include "iperf_config.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <netdb.h>
#include <string.h>
#include <sys/fcntl.h>

#ifdef HAVE_SENDFILE
#ifdef linux
#include <sys/sendfile.h>
#else
#ifdef __FreeBSD__
#include <sys/uio.h>
#else
#if defined(__APPLE__) && defined(__MACH__)	/* OS X */
#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_10_6)
#include <sys/uio.h>
#endif
#endif
#endif
#endif
#endif /* HAVE_SENDFILE */

#include "iperf_util.h"
#include "net.h"
#include "timer.h"

/* netdial and netannouce code comes from libtask: http://swtch.com/libtask/
 * Copyright: http://swtch.com/libtask/COPYRIGHT
*/

/* make connection to server */
int
netdial(int domain, int proto, char *local, int local_port, char *server, int port)
{
    struct addrinfo hints, *local_res, *server_res;
    int s;

    if (local) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = domain;
        hints.ai_socktype = proto;
        if (getaddrinfo(local, NULL, &hints, &local_res) != 0)
            return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = domain;
    hints.ai_socktype = proto;
    if (getaddrinfo(server, NULL, &hints, &server_res) != 0)
        return -1;

    s = socket(server_res->ai_family, proto, 0);
    if (s < 0) {
	if (local)
	    freeaddrinfo(local_res);
	freeaddrinfo(server_res);
        return -1;
    }

    if (local) {
        if (local_port) {
            struct sockaddr_in *lcladdr;
            lcladdr = (struct sockaddr_in *)local_res->ai_addr;
            lcladdr->sin_port = htons(local_port);
            local_res->ai_addr = (struct sockaddr *)lcladdr;
        }

        if (bind(s, (struct sockaddr *) local_res->ai_addr, local_res->ai_addrlen) < 0) {
	    close(s);
	    freeaddrinfo(local_res);
	    freeaddrinfo(server_res);
            return -1;
	}
        freeaddrinfo(local_res);
    }

    ((struct sockaddr_in *) server_res->ai_addr)->sin_port = htons(port);
    if (connect(s, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen) < 0 && errno != EINPROGRESS) {
	close(s);
	freeaddrinfo(server_res);
        return -1;
    }

    freeaddrinfo(server_res);
    return s;
}

/***************************************************************/

int
netannounce(int domain, int proto, char *local, int port)
{
    struct addrinfo hints, *res;
    char portstr[6];
    int s, opt;

    snprintf(portstr, 6, "%d", port);
    memset(&hints, 0, sizeof(hints));
    /* 
     * If binding to the wildcard address with no explicit address
     * family specified, then force us to get an AF_INET6 socket.  On
     * CentOS 6 and MacOS, getaddrinfo(3) with AF_UNSPEC in ai_family,
     * and ai_flags containing AI_PASSIVE returns a result structure
     * with ai_family set to AF_INET, with the result that we create
     * and bind an IPv4 address wildcard address and by default, we
     * can't accept IPv6 connections.
     *
     * On FreeBSD, under the above circumstances, ai_family in the
     * result structure is set to AF_INET6.
     */
    if (domain == AF_UNSPEC && !local) {
	hints.ai_family = AF_INET6;
    }
    else {
	hints.ai_family = domain;
    }
    hints.ai_socktype = proto;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(local, portstr, &hints, &res) != 0)
        return -1; 

    s = socket(res->ai_family, proto, 0);
    if (s < 0) {
	freeaddrinfo(res);
        return -1;
    }

    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, 
		   (char *) &opt, sizeof(opt)) < 0) {
	close(s);
	freeaddrinfo(res);
	return -1;
    }
    /*
     * If we got an IPv6 socket, figure out if it should accept IPv4
     * connections as well.  We do that if and only if no address
     * family was specified explicitly.  Note that we can only
     * do this if the IPV6_V6ONLY socket option is supported.  Also,
     * OpenBSD explicitly omits support for IPv4-mapped addresses,
     * even though it implements IPV6_V6ONLY.
     */
#if defined(IPV6_V6ONLY) && !defined(__OpenBSD__)
    if (res->ai_family == AF_INET6 && (domain == AF_UNSPEC || domain == AF_INET6)) {
	if (domain == AF_UNSPEC)
	    opt = 0;
	else
	    opt = 1;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, 
		       (char *) &opt, sizeof(opt)) < 0) {
	    close(s);
	    freeaddrinfo(res);
	    return -1;
	}
    }
#endif /* IPV6_V6ONLY */

    if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
        close(s);
	freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    
    if (proto == SOCK_STREAM) {
        if (listen(s, 5) < 0) {
	    close(s);
            return -1;
        }
    }

    return s;
}


/*******************************************************************/
/* reads 'count' bytes from a socket  */
/********************************************************************/

int
Nread(int fd, char *buf, size_t count, int prot)
{
    register ssize_t r;
    register size_t nleft = count;

    while (nleft > 0) {
        r = read(fd, buf, nleft);
        if (r < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return NET_HARDERROR;
        } else if (r == 0)
            break;

        nleft -= r;
        buf += r;
    }
    return count - nleft;
}


/*
 *                      N W R I T E
 */

int
Nwrite(int fd, const char *buf, size_t count, int prot)
{
    register ssize_t r;
    register size_t nleft = count;

    while (nleft > 0) {
	r = write(fd, buf, nleft);
	if (r < 0) {
	    switch (errno) {
		case EINTR:
		case EAGAIN:
#if (EAGAIN != EWOULDBLOCK)
		case EWOULDBLOCK:
#endif
		return count - nleft;

		case ENOBUFS:
		return NET_SOFTERROR;

		default:
		return NET_HARDERROR;
	    }
	} else if (r == 0)
	    return NET_SOFTERROR;
	nleft -= r;
	buf += r;
    }
    return count;
}


int
has_sendfile(void)
{
#if defined(HAVE_SENDFILE)
    return 1;
#else /* HAVE_SENDFILE */
    return 0;
#endif /* HAVE_SENDFILE */

}


/*
 *                      N S E N D F I L E
 */

int
Nsendfile(int fromfd, int tofd, const char *buf, size_t count)
{
    off_t offset;
#if defined(HAVE_SENDFILE)
#if defined(__FreeBSD__) || (defined(__APPLE__) && defined(__MACH__) && defined(MAC_OS_X_VERSION_10_6))
    off_t sent;
#endif
    register size_t nleft;
    register ssize_t r;

    nleft = count;
    while (nleft > 0) {
	offset = count - nleft;
#ifdef linux
	r = sendfile(tofd, fromfd, &offset, nleft);
	if (r > 0)
	    nleft -= r;
#elif defined(__FreeBSD__)
	r = sendfile(fromfd, tofd, offset, nleft, NULL, &sent, 0);
	nleft -= sent;
#elif defined(__APPLE__) && defined(__MACH__) && defined(MAC_OS_X_VERSION_10_6)	/* OS X */
	sent = nleft;
	r = sendfile(fromfd, tofd, offset, &sent, NULL, 0);
	nleft -= sent;
#else
	/* Shouldn't happen. */
	r = -1;
	errno = ENOSYS;
#endif
	if (r < 0) {
	    switch (errno) {
		case EINTR:
		case EAGAIN:
#if (EAGAIN != EWOULDBLOCK)
		case EWOULDBLOCK:
#endif
		if (count == nleft)
		    return NET_SOFTERROR;
		return count - nleft;

		case ENOBUFS:
		case ENOMEM:
		return NET_SOFTERROR;

		default:
		return NET_HARDERROR;
	    }
	}
#ifdef linux
	else if (r == 0)
	    return NET_SOFTERROR;
#endif
    }
    return count;
#else /* HAVE_SENDFILE */
    errno = ENOSYS;	/* error if somehow get called without HAVE_SENDFILE */
    return NET_HARDERROR;
#endif /* HAVE_SENDFILE */
}

/*************************************************************************/

/**
 * getsock_tcp_mss - Returns the MSS size for TCP
 *
 */

int
getsock_tcp_mss(int inSock)
{
    int             mss = 0;

    int             rc;
    socklen_t       len;

    assert(inSock >= 0); /* print error and exit if this is not true */

    /* query for mss */
    len = sizeof(mss);
    rc = getsockopt(inSock, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, &len);
    if (rc == -1) {
	perror("getsockopt TCP_MAXSEG");
	return -1;
    }

    return mss;
}



/*************************************************************/

/* sets TCP_NODELAY and TCP_MAXSEG if requested */
// XXX: This function is not being used.

int
set_tcp_options(int sock, int no_delay, int mss)
{
    socklen_t len;
    int rc;
    int new_mss;

    if (no_delay == 1) {
        len = sizeof(no_delay);
        rc = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&no_delay, len);
        if (rc == -1) {
            perror("setsockopt TCP_NODELAY");
            return -1;
        }
    }
#ifdef TCP_MAXSEG
    if (mss > 0) {
        len = sizeof(new_mss);
        assert(sock != -1);

        /* set */
        new_mss = mss;
        len = sizeof(new_mss);
        rc = setsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char *)&new_mss, len);
        if (rc == -1) {
            perror("setsockopt TCP_MAXSEG");
            return -1;
        }
        /* verify results */
        rc = getsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char *)&new_mss, &len);
        if (rc == -1) {
            perror("getsockopt TCP_MAXSEG");
            return -1;
        }
        if (new_mss != mss) {
            perror("setsockopt value mismatch");
            return -1;
        }
    }
#endif
    return 0;
}

/****************************************************************************/

int
setnonblocking(int fd, int nonblocking)
{
    int flags, newflags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    if (nonblocking)
	newflags = flags | (int) O_NONBLOCK;
    else
	newflags = flags & ~((int) O_NONBLOCK);
    if (newflags != flags)
	if (fcntl(fd, F_SETFL, newflags) < 0) {
	    perror("fcntl(F_SETFL)");
	    return -1;
	}
    return 0;
}

/****************************************************************************/

int
getsockdomain(int sock)
{
    struct sockaddr_storage sa;
    socklen_t len = sizeof(sa);

    if (getsockname(sock, (struct sockaddr *)&sa, &len) < 0) {
        return -1;
    }
    return ((struct sockaddr *) &sa)->sa_family;
}
