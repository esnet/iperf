/*
 * iperf, Copyright (c) 2014-2019, The Regents of the University of
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
#ifndef __WIN32__
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif
#include <assert.h>
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

/*
 * Declaration of gerror in iperf_error.c.  Most other files in iperf3 can get this
 * by including "iperf.h", but net.c lives "below" this layer.  Clearly the
 * presence of this declaration is a sign we need to revisit this layering.
 */
extern int gerror;

#ifdef __WIN32__

void nonblock(int s) {
    unsigned long nb = 1;
    if (ioctlsocket(s, FIONBIO, &nb) == SOCKET_ERROR) {
       //VLOG_ERR(VLOG << "Error setting Connection FIONBIO: "
       //         << WSAGetLastError());
    }
    else {
       //VLOG << "Made socket: " << s << " non-blocking: " << msg << endl;
    }
}
#else
void nonblock(int s) {
   //VLOG_TRC(VLOG << "in other nonblock, msg: " << msg << endl);
   if (fcntl(s, F_SETFL, O_NDELAY) == -1) {
      //VLOG << "ERROR:  fcntl (!LINUX), executing nonblock:  " 
      //     << LFSTRERROR << endl;
   }//if
}//nonblock
#endif

void print_fdset(int max_fd, fd_set* read_set, fd_set* write_set) {
    fprintf(stderr, "read/write FD sets: ");
    for (int i = 0; i<=max_fd; i++) {
        if (FD_ISSET(i, read_set)) {
            if (FD_ISSET(i, write_set)) {
                fprintf(stderr, "%i RW ", i);
            }
            else {
                fprintf(stderr, "%i RO ", i);
            }
        }
        else if (FD_ISSET(i, write_set)) {
            fprintf(stderr, "%i WO ", i);
        }
    }
    fprintf(stderr, "\n");
}

/*
 * timeout_connect adapted from netcat, via OpenBSD and FreeBSD
 * Copyright (c) 2001 Eric Jackson <ericj@monkey.org>
 */
int
timeout_connect(int s, const struct sockaddr *name, socklen_t namelen,
    int timeout)
{
	socklen_t optlen;
	int flags, optval;
	int ret;

	flags = 0;
	if (timeout != -1) {
#ifndef __WIN32__
           flags = fcntl(s, F_GETFL, 0);
#endif
           nonblock(s);
	}

	if ((ret = connect(s, name, namelen)) != 0 && errno == EINPROGRESS) {
#ifndef __WIN32__
                struct pollfd pfd;
		pfd.fd = s;
		pfd.events = POLLOUT;
		if ((ret = poll(&pfd, 1, timeout)) == 1)
#else
                fd_set write_fds;
                FD_ZERO(&write_fds);            //Zero out the file descriptor set
                FD_SET(s, &write_fds);     //Set the current socket file descriptor into the set

                //We are going to use select to wait for the socket to connect
                struct timeval tv;              //Time value struct declaration
                tv.tv_sec = timeout / 1000;                  //The second portion of the struct
                tv.tv_usec = (timeout % 1000) * 1000;        //The microsecond portion of the struct

                //DEBUG: This is ALWAYS 1
                int select_ret = select(s + 1, NULL, &write_fds, NULL, &tv);
                if (select_ret == 1)
#endif
                {
			optlen = sizeof(optval);
			if ((ret = getsockopt(s, SOL_SOCKET, SO_ERROR,
                                              (char*)&optval, &optlen)) == 0) {
				errno = optval;
				ret = optval == 0 ? 0 : -1;
			}
		} else if (ret == 0) {
			errno = ETIMEDOUT;
			ret = -1;
		} else
			ret = -1;
	}

	if (timeout != -1) {
#ifndef __WIN32__
           fcntl(s, F_SETFL, flags);
#endif
           /* TODO:  Implement this for Windows? */
        }

	return (ret);
}

/* netdial and netannouce code comes from libtask: http://swtch.com/libtask/
 * Copyright: http://swtch.com/libtask/COPYRIGHT
*/

/* make connection to server */
int
netdial(int domain, int proto, char *local, const char* bind_dev, int local_port, char *server, int port, int timeout,
        struct iperf_test *test)
{
    struct addrinfo hints, *local_res = NULL, *server_res = NULL;
    int s, saved_errno;

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
    if ((gerror = getaddrinfo(server, NULL, &hints, &server_res)) != 0)
        return -1;

    s = socket(server_res->ai_family, proto, 0);
    if (s < 0) {
	if (local)
	    freeaddrinfo(local_res);
	freeaddrinfo(server_res);
        return -1;
    }

    if (test->debug) {
        fprintf(stderr, "netdial, domain: %d  proto: %d  local: %s  bind-dev: %s local-port: %d  server: %s:%d timeout: %d, socket: %d\n",
                domain, proto, local, bind_dev, local_port, server, port, timeout, s);
    }

    if (bind_dev) {
#ifdef SO_BINDTODEVICE
        if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE,
                       bind_dev, IFNAMSIZ) < 0)
#endif
        {
            saved_errno = errno;
            closesocket(s);
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
	    closesocket(s);
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
	    errno = EAFNOSUPPORT;
            return -1;
	}

        if (bind(s, (struct sockaddr *) &lcl, addrlen) < 0) {
	    saved_errno = errno;
	    closesocket(s);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            return -1;
        }
    }

    ((struct sockaddr_in *) server_res->ai_addr)->sin_port = htons(port);
    if (timeout_connect(s, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen, timeout) < 0 && errno != EINPROGRESS) {
	saved_errno = errno;
	closesocket(s);
	freeaddrinfo(server_res);
	errno = saved_errno;
        return -1;
    }

    freeaddrinfo(server_res);
    return s;
}

/***************************************************************/

int
netannounce(int domain, int proto, char *local, const char* bind_dev, int port, struct iperf_test *test)
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

    if (test->debug) {
        fprintf(stderr, "netannounce, domain: %d  proto: %d  local: %s  bind-dev: %s port: %d fd: %d\n",
                domain, proto, local, bind_dev, port, s);
    }

    if (s < 0) {
	freeaddrinfo(res);
        return -1;
    }

    if (bind_dev) {
#ifdef SO_BINDTODEVICE
        if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE,
                       bind_dev, IFNAMSIZ) < 0)
#endif
        {
            saved_errno = errno;
            closesocket(s);
            freeaddrinfo(res);
            errno = saved_errno;
            return -1;
        }
    }

    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, 
		   (char *) &opt, sizeof(opt)) < 0) {
	saved_errno = errno;
	closesocket(s);
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
	    closesocket(s);
	    freeaddrinfo(res);
	    errno = saved_errno;
	    return -1;
	}
    }
#endif /* IPV6_V6ONLY */

    if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
        saved_errno = errno;
        closesocket(s);
	freeaddrinfo(res);
        errno = saved_errno;
        return -1;
    }

    freeaddrinfo(res);
    
    if (proto == SOCK_STREAM) {
        if (listen(s, INT_MAX) < 0) {
	    saved_errno = errno;
	    closesocket(s);
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
Nread(int fd, char *buf, size_t count, int prot, struct iperf_test *test)
{
    char* oldbuf = buf;
    register ssize_t r;
    register size_t nleft = count;

    while (nleft > 0) {
        errno = 0;
#ifndef __WIN32__
        r = read(fd, buf, nleft);
#else
        r = recv(fd, buf, nleft, 0);
#endif
        if (r < 0) {
#ifndef __WIN32__
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
#else
            if (WSAGetLastError() == WSAEWOULDBLOCK)
#endif
            {
                break;
            }
            else {
                fprintf(stderr, "Error in Nread (%s)  fd: %d\n",
                        STRERROR, fd);
                return NET_HARDERROR;
            }
        } else if (r == 0)
            break;

        nleft -= r;
        buf += r;
    }
    if (test && test->debug > 1) {
        fprintf(stderr, "Nread:\n%s", hexdump((const unsigned char*)oldbuf, count - nleft, 1, 1));
    }
    return count - nleft;
}


/*
 *                      N W R I T E
 */

int
Nwrite(int fd, const char *buf, size_t count, int prot, struct iperf_test *test)
{
    register ssize_t r;
    register size_t nleft = count;

    if (test && test->debug > 1) {
        fprintf(stderr, "Nwrite:\n%s", hexdump((const unsigned char*)buf, count, 1, 1));
    }

    while (nleft > 0) {
        errno = 0;
#ifndef __WIN32__
	r = write(fd, buf, nleft);
#else
        r = send(fd, buf, nleft, 0);
#endif
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
#ifdef __WIN32__
                    if (WSAGetLastError() == WSAEWOULDBLOCK)
                        return count - nleft;
#endif
                    return NET_HARDERROR;
	    }
	} else if (r == 0) {
            if ((count - nleft) == 0)
                return NET_SOFTERROR;
            else
                return (count - nleft); /* already wrote some */
        }
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

int
setnonblocking(int fd, int nonblocking)
{
#ifdef __WIN32__
   if (nonblocking) {
      nonblock(fd);
      return 0;
   }
   else
      return -1; /*not supported currently */
#else
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
#endif
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


#ifdef __WIN32__

// From 'mairix', under LGPL evidently

/* The mmap/munmap implementation was shamelessly stolen, with minimal
   changes, from libgwc32, a Windows port of glibc.  */

static DWORD granularity = 0;
static int isw9x = -1;

void *
mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  void *map = NULL;
  char *gran_addr = addr;
  HANDLE handle = INVALID_HANDLE_VALUE;
  DWORD cfm_flags = 0, mvf_flags = 0, sysgran;
  off_t gran_offset = offset, filelen = _filelength(fd);
  off_t mmlen = len;

  if (!granularity)
    {
      SYSTEM_INFO si;

      GetSystemInfo (&si);
      granularity = si.dwAllocationGranularity;
    }
  sysgran = granularity;

  switch (prot) {
    case PROT_READ | PROT_WRITE | PROT_EXEC:
    case PROT_WRITE | PROT_EXEC:
      cfm_flags = PAGE_EXECUTE_READWRITE;
      mvf_flags = FILE_MAP_ALL_ACCESS;
      break;
    case PROT_READ | PROT_WRITE:
      cfm_flags = PAGE_READWRITE;
      mvf_flags = FILE_MAP_ALL_ACCESS;
      break;
    case PROT_WRITE:
      cfm_flags = PAGE_READWRITE;
      mvf_flags = FILE_MAP_WRITE;
      break;
    case PROT_READ:
      cfm_flags = PAGE_READONLY;
      mvf_flags = FILE_MAP_READ;
      break;
    case PROT_NONE:
      cfm_flags = PAGE_NOACCESS;
      mvf_flags = FILE_MAP_READ;
      break;
    case PROT_EXEC:
      cfm_flags = PAGE_EXECUTE;
      mvf_flags = FILE_MAP_READ;
      break;
  }
  if (flags & MAP_PRIVATE)
    {
      if (isw9x == -1)
	isw9x = ((DWORD)(LOBYTE (LOWORD (GetVersion()))) < 5);
      if (isw9x == 1)
	cfm_flags = PAGE_WRITECOPY;
      mvf_flags = FILE_MAP_COPY;
    }
  if (flags & MAP_FIXED)
    {
      gran_offset = offset;
      gran_addr = addr;
    }
  else
    {
      gran_offset = offset & ~(sysgran - 1);
      gran_addr = (char *) (((DWORD) gran_addr / sysgran) * sysgran);
    }
  mmlen = (filelen < gran_offset + len ? filelen - gran_offset : len);

  handle = CreateFileMapping ((HANDLE) _get_osfhandle(fd), NULL, cfm_flags,
			      0, mmlen, NULL);
  if (!handle)
    {
      errno = EINVAL;	/* FIXME */
      return MAP_FAILED;
    }
  map = MapViewOfFileEx (handle, mvf_flags, HIDWORD(gran_offset),
			 LODWORD(gran_offset), (SIZE_T) mmlen,
			 (LPVOID) gran_addr);
  if (map == NULL && (flags & MAP_FIXED))
    {
      map = MapViewOfFileEx (handle, mvf_flags, HIDWORD(gran_offset),
			     LODWORD(gran_offset), (SIZE_T) mmlen,
			     (LPVOID) NULL);
    }
  CloseHandle(handle);

  if (map == NULL)
    {
      errno = EINVAL; 	/* FIXME */
      return MAP_FAILED;
    }
  return map;
}

int munmap (void *addr, size_t len)
{
  if (!UnmapViewOfFile (addr))
    return -1;
  return 0;
}

/* End of ISC stuff */
#endif
