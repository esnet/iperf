/*
 * Copyright (c) 2009-2014, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
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
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>

#include "iperf.h"
#include "iperf_api.h"
#include "locale.h"

/*************************************************************/
int
has_tcpinfo(void)
{
#if defined(linux) || defined(__FreeBSD__)
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
    /* return 1; */
    return 0;	/* FreeBSD retransmit reporting doesn't actually work yet */
#else
    return 0;
#endif
#endif
}

/*************************************************************/
void
save_tcpinfo(struct iperf_stream *sp, struct iperf_interval_results *irp)
{
#if defined(linux) || defined(__FreeBSD__)
    socklen_t tcp_info_length = sizeof(struct tcp_info);

    if (getsockopt(sp->socket, IPPROTO_TCP, TCP_INFO, (void *)&irp->tcpInfo, &tcp_info_length) < 0)
	iperf_err(sp->test, "getsockopt - %s", strerror(errno));

    if (sp->test->debug) {
	printf("tcpi_snd_cwnd %u tcpi_snd_mss %u\n",
	       irp->tcpInfo.tcpi_snd_cwnd, irp->tcpInfo.tcpi_snd_mss);
    }

#endif
}

/*************************************************************/
long
get_total_retransmits(struct iperf_interval_results *irp)
{
#if defined(linux) && defined(TCP_MD5SIG)
    return irp->tcpInfo.tcpi_total_retrans;
#else
#if defined(__FreeBSD__) && __FreeBSD_version >= 600000
    return irp->tcpInfo.__tcpi_retransmits;
#else
    return -1;
#endif
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
#else
#if defined(__FreeBSD__) && __FreeBSD_version >= 600000
    return irp->tcpInfo.tcpi_snd_cwnd * irp->tcpInfo.tcpi_snd_mss;
#else
    return -1;
#endif
#endif
}

#ifdef notdef
/*************************************************************/
//print_tcpinfo(struct iperf_interval_results *r)
void
print_tcpinfo(struct iperf_test *test)
{
#if defined(linux)
    long int retransmits = 0;
    struct iperf_stream *sp;
    SLIST_FOREACH(sp, &test->streams, streams) {
        retransmits += TAILQ_LAST(&sp->result->interval_results, irlisthead)->tcpInfo.tcpi_retransmits;
    }
    printf("TCP Info\n");
    printf("  Retransmits: %ld\n", retransmits);

/* old test print_tcpinfo code
    printf(report_tcpInfo, r->tcpInfo.tcpi_snd_cwnd, r->tcpInfo.tcpi_snd_ssthresh,
	    r->tcpInfo.tcpi_rcv_ssthresh, r->tcpInfo.tcpi_unacked, r->tcpInfo.tcpi_sacked,
	    r->tcpInfo.tcpi_lost, r->tcpInfo.tcpi_retrans, r->tcpInfo.tcpi_fackets, 
	    r->tcpInfo.tcpi_rtt, r->tcpInfo.tcpi_reordering);
*/
#endif
#if defined(__FreeBSD__)
/*
    printf(report_tcpInfo, r->tcpInfo.tcpi_snd_cwnd, r->tcpInfo.tcpi_rcv_space,
	   r->tcpInfo.tcpi_snd_ssthresh, r->tcpInfo.tcpi_rtt);
*/
#endif
}
#endif


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
}
