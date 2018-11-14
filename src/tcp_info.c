/*
 * iperf, Copyright (c) 2014, 2017, The Regents of the University of
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

/*
 * routines related to collection TCP_INFO using getsockopt()
 * 
 * Brian Tierney, ESnet  (bltierney@es.net)
 * 
 * Note that this is only really useful on Linux.
 * XXX: only standard on linux versions 2.4 and later
 #
 * FreeBSD has a limitted implementation that only includes the following:
 *   tcpi_snd_ssthresh, tcpi_snd_cwnd, tcpi_rcv_space, tcpi_rtt
 * Based on information on http://wiki.freebsd.org/8.0TODO, I dont think this will be
 * fixed before v8.1 at the earliest.
 *
 * OSX has no support.
 *
 * I think MS Windows does support TCP_INFO, but iperf3 does not currently support Windows.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_locale.h"

/*************************************************************/
int
has_tcpinfo(void)
{
#if (defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)) \
	&& defined(TCP_INFO)
    return 1;
#else
    return 0;
#endif
}

/*************************************************************/
int
has_tcpinfo_retransmits(void)
{
#if defined(linux) && defined(TCP_MD5SIG)
    /* TCP_MD5SIG doesn't actually have anything to do with TCP
    ** retransmits, it just showed up in the same rev of the header
    ** file.  If it's present then struct tcp_info has the 
    ** tcpi_total_retrans field that we need; if not, not.
    */
    return 1;
#else
#if defined(__FreeBSD__) && __FreeBSD_version >= 600000
    return 1; /* Should work now */
#elif defined(__NetBSD__) && defined(TCP_INFO)
    return 1;
#else
    return 0;
#endif
#endif
}

/*************************************************************/
void
save_tcpinfo(struct iperf_stream *sp, struct iperf_interval_results *irp)
{
#if (defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)) && \
	defined(TCP_INFO)
    socklen_t tcp_info_length = sizeof(struct tcp_info);

    if (getsockopt(sp->socket, IPPROTO_TCP, TCP_INFO, (void *)&irp->tcpInfo, &tcp_info_length) < 0)
	iperf_err(sp->test, "getsockopt - %s", strerror(errno));

    if (sp->test->debug) {
	printf("tcpi_snd_cwnd %u tcpi_snd_mss %u tcpi_rtt %u\n",
	       irp->tcpInfo.tcpi_snd_cwnd, irp->tcpInfo.tcpi_snd_mss,
	       irp->tcpInfo.tcpi_rtt);
    }

#endif
}

/*************************************************************/
long
get_total_retransmits(struct iperf_interval_results *irp)
{
#if defined(linux) && defined(TCP_MD5SIG)
    return irp->tcpInfo.tcpi_total_retrans;
#elif defined(__FreeBSD__) && __FreeBSD_version >= 600000
    return irp->tcpInfo.tcpi_snd_rexmitpack;
#elif defined(__NetBSD__) && defined(TCP_INFO)
    return irp->tcpInfo.tcpi_snd_rexmitpack;
#else
    return -1;
#endif
}

/*************************************************************/
/*
 * Return snd_cwnd in octets.
 */
long
get_snd_cwnd(struct iperf_interval_results *irp)
{
#if defined(linux) && defined(TCP_MD5SIG)
    return irp->tcpInfo.tcpi_snd_cwnd * irp->tcpInfo.tcpi_snd_mss;
#elif defined(__FreeBSD__) && __FreeBSD_version >= 600000
    return irp->tcpInfo.tcpi_snd_cwnd;
#elif defined(__NetBSD__) && defined(TCP_INFO)
    return irp->tcpInfo.tcpi_snd_cwnd * irp->tcpInfo.tcpi_snd_mss;
#else
    return -1;
#endif
}

/*************************************************************/
/*
 * Return rtt in usec.
 */
long
get_rtt(struct iperf_interval_results *irp)
{
#if defined(linux) && defined(TCP_MD5SIG)
    return irp->tcpInfo.tcpi_rtt;
#elif defined(__FreeBSD__) && __FreeBSD_version >= 600000
    return irp->tcpInfo.tcpi_rtt;
#elif defined(__NetBSD__) && defined(TCP_INFO)
    return irp->tcpInfo.tcpi_rtt;
#else
    return -1;
#endif
}

/*************************************************************/
/*
 * Return rttvar in usec.
 */
long
get_rttvar(struct iperf_interval_results *irp)
{
#if defined(linux) && defined(TCP_MD5SIG)
    return irp->tcpInfo.tcpi_rttvar;
#elif defined(__FreeBSD__) && __FreeBSD_version >= 600000
    return irp->tcpInfo.tcpi_rttvar;
#elif defined(__NetBSD__) && defined(TCP_INFO)
    return irp->tcpInfo.tcpi_rttvar;
#else
    return -1;
#endif
}

/*************************************************************/
/*
 * Return PMTU in bytes.
 */
long
get_pmtu(struct iperf_interval_results *irp)
{
#if defined(linux) && defined(TCP_MD5SIG)
    return irp->tcpInfo.tcpi_pmtu;
#else
    return -1;
#endif
}

/*************************************************************/
void
build_tcpinfo_message(struct iperf_interval_results *r, char *message)
{
#if defined(linux)
    sprintf(message, report_tcpInfo, r->tcpInfo.tcpi_snd_cwnd, r->tcpInfo.tcpi_snd_ssthresh,
	    r->tcpInfo.tcpi_rcv_ssthresh, r->tcpInfo.tcpi_unacked, r->tcpInfo.tcpi_sacked,
	    r->tcpInfo.tcpi_lost, r->tcpInfo.tcpi_retrans, r->tcpInfo.tcpi_fackets, 
	    r->tcpInfo.tcpi_rtt, r->tcpInfo.tcpi_reordering);
#endif
#if defined(__FreeBSD__)
    sprintf(message, report_tcpInfo, r->tcpInfo.tcpi_snd_cwnd,
	    r->tcpInfo.tcpi_rcv_space, r->tcpInfo.tcpi_snd_ssthresh, r->tcpInfo.tcpi_rtt);
#endif
#if defined(__NetBSD__) && defined(TCP_INFO)
    sprintf(message, report_tcpInfo, r->tcpInfo.tcpi_snd_cwnd,
	    r->tcpInfo.tcpi_rcv_space, r->tcpInfo.tcpi_snd_ssthresh, r->tcpInfo.tcpi_rtt);
#endif
}
