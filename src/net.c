/*
 * iperf, Copyright (c) 2014-2023, The Regents of the University of
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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <assert.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

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

#ifdef HAVE_POLL_H
#include <poll.h>
#endif /* HAVE_POLL_H */

#include "iperf.h"
#include "iperf_util.h"
#include "net.h"
#include "timer.h"

static int nread_read_timeout = 10;
static int nread_overall_timeout = 30;

/*
 * Declaration of gerror in iperf_error.c.  Most other files in iperf3 can get this
 * by including "iperf.h", but net.c lives "below" this layer.  Clearly the
 * presence of this declaration is a sign we need to revisit this layering.
 */
extern int gerror;

/*
 * timeout_connect adapted from netcat, via OpenBSD and FreeBSD
 * Copyright (c) 2001 Eric Jackson <ericj@monkey.org>
 */
int
timeout_connect(int s, const struct sockaddr *name, socklen_t namelen,
    int timeout)
{
	struct pollfd pfd;
	socklen_t optlen;
	int flags, optval;
	int ret;

	flags = 0;
	if (timeout != -1) {
		flags = fcntl(s, F_GETFL, 0);
		if (fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1)
			return -1;
	}

	if ((ret = connect(s, name, namelen)) != 0 && errno == EINPROGRESS) {
		pfd.fd = s;
		pfd.events = POLLOUT;
		if ((ret = poll(&pfd, 1, timeout)) == 1) {
			optlen = sizeof(optval);
			if ((ret = getsockopt(s, SOL_SOCKET, SO_ERROR,
			    &optval, &optlen)) == 0) {
				errno = optval;
				ret = optval == 0 ? 0 : -1;
			}
		} else if (ret == 0) {
			errno = ETIMEDOUT;
			ret = -1;
		} else
			ret = -1;
	}

	if (timeout != -1 && fcntl(s, F_SETFL, flags) == -1)
		ret = -1;

	return (ret);
}

/* netdial and netannouce code comes from libtask: http://swtch.com/libtask/
 * Copyright: http://swtch.com/libtask/COPYRIGHT
*/

/* create a socket */
int
create_socket(int domain, int proto, const char *local, const char *bind_dev, int local_port, const char *server, int port, struct addrinfo **server_res_out)
{
    struct addrinfo hints, *local_res = NULL, *server_res = NULL;
    int s, saved_errno;
    char portstr[6];

    if (local) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = domain;
        hints.ai_socktype = proto;
        if ((gerror = getaddrinfo(local, NULL, &hints, &local_res)) != 0)
            return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = domain;
    hints.ai_socktype = proto;
    snprintf(portstr, sizeof(portstr), "%d", port);
    if ((gerror = getaddrinfo(server, portstr, &hints, &server_res)) != 0) {
	if (local)
	    freeaddrinfo(local_res);
        return -1;
    }

    s = socket(server_res->ai_family, proto, 0);
    if (s < 0) {
	if (local)
	    freeaddrinfo(local_res);
	freeaddrinfo(server_res);
        return -1;
    }

    if (bind_dev) {
#if defined(HAVE_SO_BINDTODEVICE)
        if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE,
                       bind_dev, IFNAMSIZ) < 0)
#endif // HAVE_SO_BINDTODEVICE
        {
            saved_errno = errno;
            close(s);
            freeaddrinfo(local_res);
            freeaddrinfo(server_res);
            errno = saved_errno;
            return -1;
        }
    }

    /* Bind the local address if given a name (with or without --cport) */
    if (local) {
        if (local_port) {
            struct sockaddr_in *lcladdr;
            lcladdr = (struct sockaddr_in *)local_res->ai_addr;
            lcladdr->sin_port = htons(local_port);
        }

        if (bind(s, (struct sockaddr *) local_res->ai_addr, local_res->ai_addrlen) < 0) {
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(local_res);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            return -1;
	}
        freeaddrinfo(local_res);
    }
    /* No local name, but --cport given */
    else if (local_port) {
	size_t addrlen;
	struct sockaddr_storage lcl;

	/* IPv4 */
	if (server_res->ai_family == AF_INET) {
	    struct sockaddr_in *lcladdr = (struct sockaddr_in *) &lcl;
	    lcladdr->sin_family = AF_INET;
	    lcladdr->sin_port = htons(local_port);
	    lcladdr->sin_addr.s_addr = INADDR_ANY;
	    addrlen = sizeof(struct sockaddr_in);
	}
	/* IPv6 */
	else if (server_res->ai_family == AF_INET6) {
	    struct sockaddr_in6 *lcladdr = (struct sockaddr_in6 *) &lcl;
	    lcladdr->sin6_family = AF_INET6;
	    lcladdr->sin6_port = htons(local_port);
	    lcladdr->sin6_addr = in6addr_any;
	    addrlen = sizeof(struct sockaddr_in6);
	}
	/* Unknown protocol */
	else {
	    close(s);
	    freeaddrinfo(server_res);
	    errno = EAFNOSUPPORT;
            return -1;
	}

        if (bind(s, (struct sockaddr *) &lcl, addrlen) < 0) {
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            return -1;
        }
    }

    *server_res_out = server_res;
    return s;
}

/* make connection to server */
int
netdial(int domain, int proto, const char *local, const char *bind_dev, int local_port, const char *server, int port, int timeout)
{
    struct addrinfo *server_res = NULL;
    int s, saved_errno;

    s = create_socket(domain, proto, local, bind_dev, local_port, server, port, &server_res);
    if (s < 0) {
      return -1;
    }

    if (timeout_connect(s, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen, timeout) < 0 && errno != EINPROGRESS) {
	saved_errno = errno;
	close(s);
	freeaddrinfo(server_res);
	errno = saved_errno;
        return -1;
    }

    freeaddrinfo(server_res);
    return s;
}

/***************************************************************/

int
netannounce(int domain, int proto, const char *local, const char *bind_dev, int port)
{
    struct addrinfo hints, *res;
    char portstr[6];
    int s, opt, saved_errno;

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
    if ((gerror = getaddrinfo(local, portstr, &hints, &res)) != 0)
        return -1;

    s = socket(res->ai_family, proto, 0);
    if (s < 0) {
	freeaddrinfo(res);
        return -1;
    }

    if (bind_dev) {
#if defined(HAVE_SO_BINDTODEVICE)
        if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE,
                       bind_dev, IFNAMSIZ) < 0)
#endif // HAVE_SO_BINDTODEVICE
        {
            saved_errno = errno;
            close(s);
            freeaddrinfo(res);
            errno = saved_errno;
            return -1;
        }
    }

    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
		   (char *) &opt, sizeof(opt)) < 0) {
	saved_errno = errno;
	close(s);
	freeaddrinfo(res);
	errno = saved_errno;
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
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(res);
	    errno = saved_errno;
	    return -1;
	}
    }
#endif /* IPV6_V6ONLY */

    if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
        saved_errno = errno;
        close(s);
	freeaddrinfo(res);
        errno = saved_errno;
        return -1;
    }

    freeaddrinfo(res);

    if (proto == SOCK_STREAM) {
        if (listen(s, INT_MAX) < 0) {
	    saved_errno = errno;
	    close(s);
	    errno = saved_errno;
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
    struct iperf_time ftimeout = { 0, 0 };

    fd_set rfdset;
    struct timeval timeout = { nread_read_timeout, 0 };

    /*
     * fd might not be ready for reading on entry. Check for this
     * (with timeout) first.
     *
     * This check could go inside the while() loop below, except we're
     * currently considering whether it might make sense to support a
     * codepath that bypassese this check, for situations where we
     * already know that fd has data on it (for example if we'd gotten
     * to here as the result of a select() call.
     */
    {
        FD_ZERO(&rfdset);
        FD_SET(fd, &rfdset);
        r = select(fd + 1, &rfdset, NULL, NULL, &timeout);
        if (r < 0) {
            return NET_HARDERROR;
        }
        if (r == 0) {
            return 0;
        }
    }

    while (nleft > 0) {
        r = read(fd, buf, nleft);
        if (r < 0) {
            /* XXX EWOULDBLOCK can't happen without non-blocking sockets */
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return NET_HARDERROR;
        } else if (r == 0)
            break;

        nleft -= r;
        buf += r;

        /*
         * We need some more bytes but don't want to wait around
         * forever for them. In the case of partial results, we need
         * to be able to read some bytes every nread_timeout seconds.
         */
        if (nleft > 0) {
            struct iperf_time now;

            /*
             * Also, we have an approximate upper limit for the total time
             * that a Nread call is supposed to take. We trade off accuracy
             * of this timeout for a hopefully lower performance impact.
             */
            iperf_time_now(&now);
            if (ftimeout.secs == 0) {
                ftimeout = now;
                iperf_time_add_usecs(&ftimeout, nread_overall_timeout * 1000000L);
            }
            if (iperf_time_compare(&ftimeout, &now) < 0) {
                break;
            }

            FD_ZERO(&rfdset);
            FD_SET(fd, &rfdset);
            r = select(fd + 1, &rfdset, NULL, NULL, &timeout);
            if (r < 0) {
                return NET_HARDERROR;
            }
            if (r == 0) {
                break;
            }
        }
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
                    /* XXX EWOULDBLOCK can't happen without non-blocking sockets */
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
#if defined(HAVE_SENDFILE)
    off_t offset;
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
                    /* XXX EWOULDBLOCK can't happen without non-blocking sockets */
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
