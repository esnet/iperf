/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
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
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>
#include <netinet/in.h>

#include "iperf_api.h"
#include "locale.h"

/*************************************************************/
void
get_tcpinfo(struct iperf_stream *sp, struct iperf_interval_results *rp)
{
#if defined(linux) || defined(__FreeBSD__)
    socklen_t tcp_info_length;
//    struct tcp_info tempinfo;
//    struct iperf_stream *sp = SLIST_FIRST(&test->streams);

    tcp_info_length = sizeof(struct tcp_info);

    if (getsockopt(sp->socket, IPPROTO_TCP, TCP_INFO, (void *)&rp->tcpInfo, &tcp_info_length) < 0) {
        perror("getsockopt");
    }

/*
    for (sp = SLIST_NEXT(sp, streams); sp != SLIST_END(&test->streams);
            sp = SLIST_NEXT(sp, streams)) {
        tcp_info_length = sizeof(struct tcp_info);
        if (getsockopt(sp->socket, IPPROTO_TCP, TCP_INFO, (void *) &tempinfo, &tcp_info_length) < 0) {
            perror("getsockopt");
        }
        rp->tcpInfo.tcpi_retransmits += tempinfo.tcpi_retransmits;
    }
*/
#endif
    return;
}

/*************************************************************/
//print_tcpinfo(struct iperf_interval_results *r)
void
print_tcpinfo(struct iperf_test *test)
{
    long int retransmits = 0;
    struct iperf_stream *sp;
#if defined(linux)
    SLIST_FOREACH(sp, &test->streams, streams) {
        retransmits += sp->result->last_interval_results->tcpInfo.tcpi_retransmits;
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

