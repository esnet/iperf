
/*
 * routines related to collection TCP_INFO using getsockopt()
 * 
 * Brian Tierney, ESnet  (bltierney@es.net)
 * 
 * Note that this is only really useful on Linux.
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>
#include <netinet/in.h>

#include "iperf_api.h"
#include "locale.h"

/*************************************************************/
void
get_tcpinfo(struct iperf_test *test, struct iperf_interval_results *rp)
{
#if defined(linux) || defined(__FreeBSD__)
    socklen_t	    tcp_info_length;
    struct tcp_info tcpInfo;
    struct iperf_stream *sp = test->streams;

    tcp_info_length = sizeof(tcpInfo);
    memset((char *)&tcpInfo, 0, tcp_info_length);
    //printf("getting TCP_INFO for socket %d \n", sp->socket);
    if (getsockopt(sp->socket, IPPROTO_TCP, TCP_INFO, (void *)&tcpInfo, &tcp_info_length) < 0) {
	perror("getsockopt");
    }
    memcpy(&(rp->tcpInfo), &tcpInfo, sizeof(tcpInfo));
    /* for debugging
    printf("   got TCP_INFO: %d, %d, %d, %d\n", rp->tcpInfo.tcpi_snd_cwnd,
	   rp->tcpInfo.tcpi_snd_ssthresh, rp->tcpInfo.tcpi_rcv_space, rp->tcpInfo.tcpi_rtt);
    */
    return;
#else
    return;
#endif
}

/*************************************************************/
void
print_tcpinfo(struct iperf_interval_results *r)
{
#if defined(linux)
    printf(report_tcpInfo, r->tcpInfo.tcpi_snd_cwnd, r->tcpInfo.tcpi_snd_ssthresh,
	    r->tcpInfo.tcpi_rcv_ssthresh, r->tcpInfo.tcpi_unacked, r->tcpInfo.tcpi_sacked,
	    r->tcpInfo.tcpi_lost, r->tcpInfo.tcpi_retrans, r->tcpInfo.tcpi_fackets, 
	    r->tcpInfo.tcpi_rtt, r->tcpInfo.tcpi_reordering);
#endif
#if defined(__FreeBSD__)
    printf(report_tcpInfo, r->tcpInfo.tcpi_snd_cwnd, r->tcpInfo.tcpi_rcv_space,
	   r->tcpInfo.tcpi_snd_ssthresh, r->tcpInfo.tcpi_rtt);
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
