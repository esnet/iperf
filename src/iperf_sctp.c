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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/select.h>

#ifdef HAVE_NETINET_SCTP_H
#include <netinet/sctp.h>
#endif /* HAVE_NETINET_SCTP_H */

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_sctp.h"
#include "net.h"



/* iperf_sctp_recv
 *
 * receives the data for SCTP
 */
int
iperf_sctp_recv(struct iperf_stream *sp)
{
#if defined(HAVE_SCTP)
    int r;

    r = Nread(sp->socket, sp->buffer, sp->settings->blksize, Psctp);
    if (r < 0)
        return r;

    sp->result->bytes_received += r;
    sp->result->bytes_received_this_interval += r;

    return r;
#else
    i_errno = IENOSCTP;
    return -1;
#endif /* HAVE_SCTP */
}


/* iperf_sctp_send 
 *
 * sends the data for SCTP
 */
int
iperf_sctp_send(struct iperf_stream *sp)
{
#if defined(HAVE_SCTP)
    int r;

    r = Nwrite(sp->socket, sp->buffer, sp->settings->blksize, Psctp);
    if (r < 0)
        return r;    

    sp->result->bytes_sent += r;
    sp->result->bytes_sent_this_interval += r;

    return r;
#else
    i_errno = IENOSCTP;
    return -1;
#endif /* HAVE_SCTP */
}



/* iperf_sctp_accept
 *
 * accept a new SCTP stream connection
 */
int
iperf_sctp_accept(struct iperf_test * test)
{
#if defined(HAVE_SCTP)
    int     s;
    signed char rbuf = ACCESS_DENIED;
    char    cookie[COOKIE_SIZE];
    socklen_t len;
    struct sockaddr_storage addr;

    len = sizeof(addr);
    s = accept(test->listener, (struct sockaddr *) &addr, &len);
    if (s < 0) {
        i_errno = IESTREAMCONNECT;
        return -1;
    }

    if (Nread(s, cookie, COOKIE_SIZE, Psctp) < 0) {
        i_errno = IERECVCOOKIE;
        return -1;
    }

    if (strcmp(test->cookie, cookie) != 0) {
        if (Nwrite(s, (char*) &rbuf, sizeof(rbuf), Psctp) < 0) {
            i_errno = IESENDMESSAGE;
            return -1;
        }
        close(s);
    }

    return s;
#else
    i_errno = IENOSCTP;
    return -1;
#endif /* HAVE_SCTP */
}


/* iperf_sctp_listen
 *
 * start up a listener for SCTP stream connections
 */
int
iperf_sctp_listen(struct iperf_test *test)
{
#if defined(HAVE_SCTP)
    struct addrinfo hints, *res;
    char portstr[6];
    int s, opt;

    close(test->listener);
   
    snprintf(portstr, 6, "%d", test->server_port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = (test->settings->domain == AF_UNSPEC ? AF_INET6 : test->settings->domain);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(test->bind_address, portstr, &hints, &res) != 0) {
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    if ((s = socket(res->ai_family, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
        freeaddrinfo(res);
        i_errno = IESTREAMLISTEN;
        return -1;
    }

#if defined(IPV6_V6ONLY) && !defined(__OpenBSD__)
    if (test->settings->domain == AF_UNSPEC || test->settings->domain == AF_INET6) {
        if (test->settings->domain == AF_UNSPEC)
            opt = 0;
        else if (test->settings->domain == AF_INET6)
            opt = 1;
        if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, 
		       (char *) &opt, sizeof(opt)) < 0) {
	    close(s);
	    freeaddrinfo(res);
	    i_errno = IEPROTOCOL;
	    return -1;
	}
    }
#endif /* IPV6_V6ONLY */

    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(s);
        freeaddrinfo(res);
        i_errno = IEREUSEADDR;
        return -1;
    }

    /* servers must call sctp_bindx() _instead_ of bind() */
    if (!TAILQ_EMPTY(&test->xbind_addrs)) {
        freeaddrinfo(res);
        if (iperf_sctp_bindx(test, s, IPERF_SCTP_SERVER))
            return -1;
    } else
    if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
        close(s);
        freeaddrinfo(res);
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    freeaddrinfo(res);

    if (listen(s, 5) < 0) {
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    test->listener = s;
  
    return s;
#else
    i_errno = IENOSCTP;
    return -1;
#endif /* HAVE_SCTP */
}


/* iperf_sctp_connect
 *
 * connect to a SCTP stream listener
 */
int
iperf_sctp_connect(struct iperf_test *test)
{
#if defined(HAVE_SCTP)
    int s, opt;
    char portstr[6];
    struct addrinfo hints, *local_res, *server_res;

    if (test->bind_address) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = test->settings->domain;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(test->bind_address, NULL, &hints, &local_res) != 0) {
            i_errno = IESTREAMCONNECT;
            return -1;
        }
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = test->settings->domain;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", test->server_port);
    if (getaddrinfo(test->server_hostname, portstr, &hints, &server_res) != 0) {
	if (test->bind_address)
	    freeaddrinfo(local_res);
        i_errno = IESTREAMCONNECT;
        return -1;
    }

    s = socket(server_res->ai_family, SOCK_STREAM, IPPROTO_SCTP);
    if (s < 0) {
	if (test->bind_address)
	    freeaddrinfo(local_res);
	freeaddrinfo(server_res);
        i_errno = IESTREAMCONNECT;
        return -1;
    }

    if (test->no_delay != 0) {
         opt = 1;
         if (setsockopt(s, IPPROTO_SCTP, SCTP_NODELAY, &opt, sizeof(opt)) < 0) {
             close(s);
             freeaddrinfo(server_res);
             i_errno = IESETNODELAY;
             return -1;
         }
    }

    if ((test->settings->mss >= 512 && test->settings->mss <= 131072)) {

	/*
	 * Some platforms use a struct sctp_assoc_value as the
	 * argument to SCTP_MAXSEG.  Other (older API implementations)
	 * take an int.  FreeBSD 10 and CentOS 6 support SCTP_MAXSEG,
	 * but OpenSolaris 11 doesn't.
	 */
#ifdef HAVE_STRUCT_SCTP_ASSOC_VALUE
        struct sctp_assoc_value av;

	/*
	 * Some platforms support SCTP_FUTURE_ASSOC, others need to
	 * (equivalently) do 0 here.  FreeBSD 10 is an example of the
	 * former, CentOS 6 Linux is an example of the latter.
	 */
#ifdef SCTP_FUTURE_ASSOC
        av.assoc_id = SCTP_FUTURE_ASSOC;
#else
	av.assoc_id = 0;
#endif
        av.assoc_value = test->settings->mss;

        if (setsockopt(s, IPPROTO_SCTP, SCTP_MAXSEG, &av, sizeof(av)) < 0) {
            close(s);
            freeaddrinfo(server_res);
            i_errno = IESETMSS;
            return -1;
        }
#else
	opt = test->settings->mss;

	/*
	 * Solaris might not support this option.  If it doesn't work,
	 * ignore the error (at least for now).
	 */
        if (setsockopt(s, IPPROTO_SCTP, SCTP_MAXSEG, &opt, sizeof(opt)) < 0 &&
	    errno != ENOPROTOOPT) {
            close(s);
            freeaddrinfo(server_res);
            i_errno = IESETMSS;
            return -1;
        }
#endif HAVE_STRUCT_SCTP_ASSOC_VALUE
    }

    if (test->settings->num_ostreams > 0) {
        struct sctp_initmsg initmsg;

        memset(&initmsg, 0, sizeof(struct sctp_initmsg));
        initmsg.sinit_num_ostreams = test->settings->num_ostreams;

        if (setsockopt(s, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(struct sctp_initmsg)) < 0) {
                close(s);
                freeaddrinfo(server_res);
                i_errno = IESETSCTPNSTREAM;
                return -1;
        }
    }

    /* clients must call bind() followed by sctp_bindx() before connect() */
    if (!TAILQ_EMPTY(&test->xbind_addrs)) {
        if (iperf_sctp_bindx(test, s, IPERF_SCTP_CLIENT))
            return -1;
    }

    /* TODO support sctp_connectx() to avoid heartbeating. */
    if (connect(s, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen) < 0 && errno != EINPROGRESS) {
	close(s);
	freeaddrinfo(server_res);
        i_errno = IESTREAMCONNECT;
        return -1;
    }
    freeaddrinfo(server_res);

    /* Send cookie for verification */
    if (Nwrite(s, test->cookie, COOKIE_SIZE, Psctp) < 0) {
	close(s);
        i_errno = IESENDCOOKIE;
        return -1;
    }

    /*
     * We want to allow fragmentation.  But there's at least one
     * implementation (Solaris) that doesn't support this option,
     * even though it defines SCTP_DISABLE_FRAGMENTS.  So we have to
     * try setting the option and ignore the error, if it doesn't
     * work.
     */
    opt = 0;
    if (setsockopt(s, IPPROTO_SCTP, SCTP_DISABLE_FRAGMENTS, &opt, sizeof(opt)) < 0 &&
	errno != ENOPROTOOPT) {
        close(s);
        freeaddrinfo(server_res);
        i_errno = IESETSCTPDISABLEFRAG;
        return -1;
    }

    return s;
#else
    i_errno = IENOSCTP;
    return -1;
#endif /* HAVE_SCTP */
}



int
iperf_sctp_init(struct iperf_test *test)
{
#if defined(HAVE_SCTP)
    return 0;
#else
    i_errno = IENOSCTP;
    return -1;
#endif /* HAVE_SCTP */
}



/* iperf_sctp_bindx
 *
 * handle binding to multiple endpoints (-X parameters)
 */
int
iperf_sctp_bindx(struct iperf_test *test, int s, int is_server)
{
#if defined(HAVE_SCTP)
    struct addrinfo hints;
    char portstr[6];
    char *servname;
    struct addrinfo *ai, *ai0;
    struct sockaddr *xaddrs;
    struct xbind_entry *xbe, *xbe0;
    char *bp;
    size_t xaddrlen;
    int nxaddrs;
    int retval;
    int domain;

    domain = test->settings->domain;
    xbe0 = NULL;
    retval = 0;

    if (TAILQ_EMPTY(&test->xbind_addrs))
        return retval; /* nothing to do */

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = (domain == AF_UNSPEC ? AF_INET6 : domain);
    hints.ai_socktype = SOCK_STREAM;
    servname = NULL;
    if (is_server) {
        hints.ai_flags |= AI_PASSIVE;
        snprintf(portstr, 6, "%d", test->server_port);
        servname = portstr;
    }

    /* client: must pop first -X address and call bind().
     * sctp_bindx() must see the ephemeral port chosen by bind().
     * we deliberately ignore the -B argument in this case.
     */
    if (!is_server) {
        struct sockaddr *sa;
        struct sockaddr_in *sin;
        struct sockaddr_in6 *sin6;
        int eport;

        xbe0 = TAILQ_FIRST(&test->xbind_addrs);
        TAILQ_REMOVE(&test->xbind_addrs, xbe0, link);

        if (getaddrinfo(xbe0->name, servname, &hints, &xbe0->ai) != 0) {
            i_errno = IESETSCTPBINDX;
            retval = -1;
            goto out;
        }

        ai = xbe0->ai;
        if (domain != AF_UNSPEC && domain != ai->ai_family) {
            i_errno = IESETSCTPBINDX;
            retval = -1;
            goto out;
        }
        if (bind(s, (struct sockaddr *)ai->ai_addr, ai->ai_addrlen) < 0) {
            i_errno = IESETSCTPBINDX;
            retval = -1;
            goto out;
        }

        /* if only one -X argument, nothing more to do */
        if (TAILQ_EMPTY(&test->xbind_addrs))
            goto out;

        sa = (struct sockaddr *)ai->ai_addr;
        if (sa->sa_family == AF_INET) {
            sin = (struct sockaddr_in *)ai->ai_addr;
            eport = sin->sin_port;
        } else if (sa->sa_family == AF_INET6) {
            sin6 = (struct sockaddr_in6 *)ai->ai_addr;
            eport = sin6->sin6_port;
        } else {
            i_errno = IESETSCTPBINDX;
            retval = -1;
            goto out;
        }
        snprintf(portstr, 6, "%d", ntohs(eport));
        servname = portstr;
    }

    /* pass 1: resolve and compute lengths. */
    nxaddrs = 0;
    xaddrlen = 0;
    TAILQ_FOREACH(xbe, &test->xbind_addrs, link) {
        if (xbe->ai != NULL)
            freeaddrinfo(xbe->ai);
        if (getaddrinfo(xbe->name, servname, &hints, &xbe->ai) != 0) {
            i_errno = IESETSCTPBINDX;
            retval = -1;
            goto out;
        }
        ai0 = xbe->ai;
        for (ai = ai0; ai; ai = ai->ai_next) {
            if (domain != AF_UNSPEC && domain != ai->ai_family)
                continue;
            xaddrlen += ai->ai_addrlen;
            ++nxaddrs;
        }
    }

    /* pass 2: copy into flat buffer. */
    xaddrs = (struct sockaddr *)malloc(xaddrlen);
    if (!xaddrs) {
            i_errno = IESETSCTPBINDX;
            retval = -1;
            goto out;
    }
    bp = (char *)xaddrs;
    TAILQ_FOREACH(xbe, &test->xbind_addrs, link) {
        ai0 = xbe->ai;
        for (ai = ai0; ai; ai = ai->ai_next) {
            if (domain != AF_UNSPEC && domain != ai->ai_family)
                continue;
	    memcpy(bp, ai->ai_addr, ai->ai_addrlen);
            bp += ai->ai_addrlen;
        }
    }

    if (sctp_bindx(s, xaddrs, nxaddrs, SCTP_BINDX_ADD_ADDR) == -1) {
        close(s);
        free(xaddrs);
        i_errno = IESETSCTPBINDX;
        retval = -1;
        goto out;
    }

    free(xaddrs);
    retval = 0;

out:
    /* client: put head node back. */
    if (!is_server && xbe0)
        TAILQ_INSERT_HEAD(&test->xbind_addrs, xbe0, link);

    TAILQ_FOREACH(xbe, &test->xbind_addrs, link) {
        if (xbe->ai) {
            freeaddrinfo(xbe->ai);
            xbe->ai = NULL;
        }
    }

    return retval;
#else
    i_errno = IENOSCTP;
    return -1;
#endif /* HAVE_SCTP */
}
