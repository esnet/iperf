
/*
 * routines related to collection TCP_INFO using getsockopt()
 * 
 * Brian Tierney, ESnet  (bltierney@es.net)
 * 
 * Note that this is only supported on Linux and FreeBSD, and that for FreeBSD
 * only the following are returned: tcpi_snd_ssthresh, tcpi_snd_cwnd,
 * tcpi_rcv_space, tcpi_rtt
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
get_tcpinfo(struct iperf_test *test)
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
    printf("get TCP_INFP: %d, %d, %d, %d\n", tcpInfo.tcpi_snd_cwnd,
       tcpInfo.tcpi_snd_ssthresh, tcpInfo.tcpi_rcv_space, tcpInfo.tcpi_rtt);
    /*
     * memcpy(&temp.tcpInfo, &tcpInfo, sizeof(tcpInfo));
     */
    /* need to copy these results somewhere! */
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
    	   r->tcpInfo.tcpi_lost, r->tcpInfo.tcpi_retrans, r->tcpInfo.tcpi_fackets);
#endif
#if defined(__FreeBSD__)
    printf(report_tcpInfo, r->tcpInfo.tcpi_snd_cwnd, r->tcpInfo.tcpi_rcv_space,
	   r->tcpInfo.tcpi_snd_ssthresh, r->tcpInfo.tcpi_rtt);
#endif



}
