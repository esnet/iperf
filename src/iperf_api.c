#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include<fcntl.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h> 
#include <stdint.h>

#include <sys/time.h>

#include "iperf_api.h"
#include "timer.h"
#include "net.h"
#include "units.h"
#include "tcp_window_size.h"

enum {
    Mundef = 0,
    Mclient,
    Mserver,
	
    Ptcp = SOCK_STREAM,
    Pudp = SOCK_DGRAM,
	
    uS_TO_NS = 1000,
	
	MAX_BUFFER_SIZE =10,
    DEFAULT_UDP_BUFSIZE = 1470,
    DEFAULT_TCP_BUFSIZE = 8192
};
#define SEC_TO_NS 1000000000 /* too big for enum on some platforms */


static struct option longopts[] =
{
{ "client",         required_argument,      NULL,   'c' },
{ "server",         no_argument,            NULL,   's' },
{ "time",           required_argument,      NULL,   't' },
{ "port",           required_argument,      NULL,   'p' },
{ "parallel",       required_argument,      NULL,   'P' },
{ "udp",            no_argument,            NULL,   'u' },
{ "bandwidth",      required_argument,      NULL,   'b' },
{ "length",         required_argument,      NULL,   'l' },
{ "window",         required_argument,      NULL,   'w' },
{ NULL,             0,                      NULL,   0   },
{"interval",		required_argument,		NULL,	'i'}	
};


struct iperf_test *iperf_create_test()
{
	struct iperf_test * test;
	
	test = (struct iperf_test *) malloc(sizeof(struct iperf_test));
	if(!test)
	{
		perror("malloc");
		return(NULL);
	}
	
	// initialise everything to zero
	memset(test, 0, sizeof(struct iperf_test));
	
	test->role = 's';
	
	test->proto = Ptcp;
	
	memset(&test->remote_ip_addr, 0, sizeof(struct sockaddr_storage));
	memset(&test->local_ip_addr, 0, sizeof(struct sockaddr_storage));
	
	test->duration = 10;
	
	test->stats_interval = 1;
	test->reporter_interval = 1;
		
	test->num_streams = 1;
			
	// return an empty iperf_test* with memory alloted.
	return test;
	
}
	
void iperf_init_test(struct iperf_test *test)
{
	char ubuf[UNIT_LEN];
	struct iperf_stream *sp;
	int i;
	
	if(test->role == 's')
	{		
		 test->streams->socket = netannounce(test->proto, NULL, test->streams->local_port);
		if( test->streams->socket < 0)
			exit(0);
		
		//initiate for Server 
		iperf_init_stream(test->streams);
		
		/*
		if(set_tcp_windowsize( test->streams->socket, (test->streams->settings->window_size, SO_RCVBUF) < 0) 
		{
			perror("unable to set window");
			return -1;
		}*/
		
		printf("-----------------------------------------------------------\n");
		printf("Server listening on %d\n", test->streams->local_port);
		int x;
		if((x = getsock_tcp_windowsize( test->streams->socket, SO_RCVBUF)) < 0) 
			perror("SO_RCVBUF");	
	
		
		unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
		printf("%s: %s\n",
			   test->proto == Ptcp ? "TCP window size" : "UDP buffer size", ubuf);
		printf("-----------------------------------------------------------\n");
		
	}
	
	else if( test->role == 'c')
	{
		sp = test->streams;
		
		// initiate for each client stream
		for(i = 0; i < test->num_streams; i++)
		{
			// need to pass the client ip address currently HARDCODED - kprabhu
			sp->socket = netdial(test->proto, "127.0.0.1", sp->local_port);
			if(sp->socket < 0) 
			{
				fprintf(stderr, "netdial failed\n");
				exit(0);
			}
			printf("The socket created for client at %d\n", sp->socket);
			
			iperf_init_stream(sp);
			
			if(sp->next == NULL)
				break;
			sp=sp->next;
		}
	}
	
}

void iperf_destroy_test(struct iperf_test *test)
{
	free(test);
}

struct iperf_stream * iperf_create_stream(struct iperf_sock_opts *sockopt)
{
	
	struct iperf_stream *sp;
   	struct iperf_settings *settings;
	
    sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if(!sp) {
        perror("malloc");
        return(NULL);
    }
	
	sp->settings = (struct iperf_settings *)malloc(sizeof(struct iperf_settings));
	settings = (struct iperf_settings *)malloc(sizeof(struct iperf_settings));	
	
	sp->result = (struct iperf_stream_result *)malloc(sizeof(struct iperf_stream_result));
	
	//initialise sp with 0
    memset(sp, 0, sizeof(struct iperf_stream));
	
	memset(&sp->result, 0, sizeof(struct iperf_stream_result));
	
	sp->local_port = 5001;
	sp->remote_port = 5001;
	
	memset(&sp->remote_addr, 0, sizeof(struct sockaddr_storage));
	memset(&sp->local_addr, 0, sizeof(struct sockaddr_storage));
		
	((struct iperf_settings *)(sp->settings))->options = sockopt;
	
	sp->socket = -1;
	
	sp->result->bytes_received = 0;
	sp->result->bytes_sent = 0;
		
	return sp;
}

struct iperf_stream * iperf_create_tcp_stream(int window, struct iperf_sock_opts *sockopt)
{	
	struct iperf_stream *sp;
   	struct iperf_tcp_settings * tcp_settings;
	
	sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if(!sp) {
        perror("malloc");
        return(NULL);
    }
	
	sp->settings = (struct iperf_tcp_settings *)malloc(sizeof(struct iperf_tcp_settings));
	tcp_settings = (struct iperf_tcp_settings *)malloc(sizeof(struct iperf_tcp_settings));
	
	sp->result = (struct iperf_stream_result *)malloc(sizeof(struct iperf_stream_result));
	
	//initialise sp with 0
    memset(sp, 0, sizeof(struct iperf_stream));
	
	memset(&sp->result, 0, sizeof(struct iperf_stream_result));
	
	sp->local_port = 5001;
	sp->remote_port = 5001;
	
	memset(&sp->remote_addr, 0, sizeof(struct sockaddr_storage));
	memset(&sp->local_addr, 0, sizeof(struct sockaddr_storage));
	
	tcp_settings->options = sockopt;
	tcp_settings->window_size =window;
	sp->settings = tcp_settings;
	
	sp->socket = -1;
	

	return sp;		
}

struct iperf_stream * iperf_create_udp_stream(int rate, int size, struct iperf_sock_opts *sockopt)
{	
	struct iperf_stream *sp;
   	struct iperf_udp_settings *udp_settings;
    sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if(!sp) {
        perror("malloc");
        return(NULL);
    }
	
	sp->settings = (struct iperf_udp_settings *)malloc(sizeof(struct iperf_udp_settings));
	udp_settings = (struct iperf_udp_settings *)malloc(sizeof(struct iperf_udp_settings));
	
	sp->result = (struct iperf_stream_result *)malloc(sizeof(struct iperf_stream_result));
	//initialise sp with 0
    memset(sp, 0, sizeof(struct iperf_stream));
	
	memset(&sp->result, 0, sizeof(struct iperf_stream_result));
	
	sp->local_port = 5001;
	sp->remote_port = 5001;
	
	memset(&sp->remote_addr, 0, sizeof(struct sockaddr_storage));
	memset(&sp->local_addr, 0, sizeof(struct sockaddr_storage));
	
	udp_settings->options = sockopt;
	udp_settings->rate = rate;
    udp_settings->packet_size = size;
	
	sp->settings = udp_settings;	
	
	sp->socket = -1;
		
	return sp;		
}

void iperf_init_stream(struct iperf_stream *sp)
{
	socklen_t len;
	int x;

	len = sizeof(struct sockaddr_in);
	
	if(getsockname(sp->socket, (struct sockaddr *) &sp->local_addr, &len) < 0) 
	{
        perror("getsockname");        
    }
	
	
	//stores the socket id.
    if(getpeername(sp->socket, (struct sockaddr *) &sp->remote_addr, &len) < 0)
	{
        perror("getpeername");
        free(sp);
        return(NULL);
    }
	
	x = getsock_tcp_windowsize(sp->socket, SO_RCVBUF);
    if(x < 0)
        perror("SO_RCVBUF");
	
    printf("RCV: %d\n", x);
	
    x = getsock_tcp_windowsize(sp->socket, SO_SNDBUF);
    if(x < 0)
        perror("SO_SNDBUF");
	
    printf("SND: %d\n", x);	
	
}

int iperf_add_stream(struct iperf_test *test, struct iperf_stream *sp)
{
	struct iperf_stream *n;
	
    if(!test->streams){
        test->streams = sp;
		return 1;
	}
    else {
        n = test->streams;
        while(n->next)
            n = n->next;
        n->next = sp;
		return 1;
    }
	
	return 0;
}


int *recv_stream(struct iperf_stream *sp)
{
/*
    struct timeval tv;
    char ubuf[UNIT_LEN];
	fd_set read_set, temp_set;
	int maxfd,result;
	
	FD_ZERO(&read_set);
	FD_SET(sp->socket, &read_set);
	maxfd = sp->socket;
	
	while(1)
	{	
		memcpy(&temp_set, &read_set,sizeof(temp_set));
		tv.tv_sec = 50;			// timeout interval in seconds
		tv.tv_usec = 0;
			   
		// using select to check on multiple descriptors.
		result = select(maxfd + 1, &temp_set, NULL, NULL, &tv);
		
		if (result == 0) 
			printf("select() timed out!\n");
		
		else if (result < 0 && errno != EINTR)
			printf("Error in select(): %s\n", strerror(errno));
		
		else if (result > 0) 
		{
			if (FD_ISSET(sp->socket, &temp_set))
			{
				if(sp->protocol == Ptcp)		
					maxfd = tcp_server_accept(&sp, maxfd, &read_set);		// need to change the arguements
				
				else if(sp->protocol == Pudp)	
					maxfd = udp_server_accept(&sp, maxfd, &read_set);		// need to change the arguements				
				
				FD_CLR(sp->socket, &temp_set);
				
				Display();
			}			
			
		if(sp->protocol == Ptcp)
			tcp_server_thread(maxfd, &temp_set, &read_set, sp);			// need to change the arguements			
							
		else if( sp->protocol == Pudp)
			udp_server_thread(maxfd, &temp_set, &read_set, sp);			// need to change the arguements
			
		} // end else if (result > 0)
	}	
	
	// return ?
*/ 
}


int *send_stream( struct iperf_stream *sp)
{
 
/*	
	int s, i;
	struct iperf_stream *sp;
	char *buf;
	int64_t delayns, adjustns, dtargns; 
	struct timeval before, after;
	fd_set write_set;
	struct timeval tv;
	int maxfd,ret=0; 
 
	if (test->proto == Pudp)
	{
		dtargns = (int64_t)settings->bufsize * SEC_TO_NS * 8;
		dtargns /= settings->bw;
 
		assert(dtargns != 0);
 
		if(gettimeofday(&before, 0) < 0) {
			perror("gettimeofday");
		}
 
		delayns = dtargns;
		adjustns = 0;
		printf("%lld adj %lld delay\n", adjustns, delayns);			
 }
 
 
 
   // most of the current Client function goes here.
	ret = select(maxfd+1, NULL, &write_set, NULL, &tv);
 
	if(ret<0)
		continue;
 
	sp = test->streams;	
	for(i=0;i<settings->threads;i++)
	{
		if(FD_ISSET(sp->sock, &write_set))
		{
			send(sp->sock, buf, sp->settings->bufsize, 0);
			sp->result->bytes_out += sp->settings->bufsize;								
 
			if (test->proto == Pudp)
			{
				if(delayns > 0)
					delay(delayns);
 
				if(gettimeofday(&after, 0) < 0) {
					perror("gettimeofday");
				}
 
 
				adjustns = dtargns;
				adjustns += (before.tv_sec - after.tv_sec) * SEC_TO_NS;
				adjustns += (before.tv_usec - after.tv_usec) * uS_TO_NS;
 
				if( adjustns > 0 || delayns > 0) {
				printf("%lld adj %lld delay\n", adjustns, delayns);
				delayns += adjustns;
				}
 
			memcpy(&before, &after, sizeof before);
			}
 
			if(sp->next==NULL)
			break;
			sp=sp->next;
		}
	}		
 
   return 0;
*/
}
			   
// This function would be big.			   
int iperf_run(struct iperf_test *test)
{
/*
	int rc;
	struct *timer timer;
	
	// don't see any other way of assigning this anywhere 
	test->streams->protocol = test->proto;
	
	if(test->role == 's')
	{
		init(test->streams, test);
 
		rc = recv_stream(test->streams);
			
	}
	else (if test->role == 'c')
	{
		timer = new_timer(test->duration, 0);
 
		while(!timer->expired(timer))
		{
 
			init(test->streams, test);
 
			// need change the send_Stream() arguements to accept test/ only test
			rc = send_stream(test->streams,test);
 						
		}
 
 
	}
 */	
}







int
main(int argc, char **argv)
{
	char ch;
    struct iperf_test *test;
	struct iperf_stream *sp, temp;
	struct iperf_sock_opts *sockopt=NULL;
	int window= 1024*1024, size;
	int i, bw = 100000, buffer_size;
	
	
	sockopt = (struct iperf_sock_opts *)malloc(sizeof(struct iperf_sock_opts));
	
	
	test= iperf_create_test();
		
	while( (ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:", longopts, NULL)) != -1 )
        switch (ch) {
            case 'c':
                test->role = 'c';	
				// remote_ip_addr
				
                break;
				
            case 'p':
               temp.remote_port = atoi(optarg);
                break;
            case 's':
				test->role = 's';
                break;
            case 't':
                test->duration = atoi(optarg);
                break;
            case 'u':
                test->proto = Pudp;
                break;
            case 'P':
                test->num_streams = atoi(optarg);
                break;
            case 'b':
				bw = atoi(optarg);
                break;
            case 'l':
               buffer_size = atol(optarg);
                break;
            case 'w':
                window = atoi(optarg);
			case 'i':
				test->stats_interval= atoi(optarg);
                break;
        }
	
	
		switch(test->proto)	{				
		case Ptcp:
			for(i=0;i<test->num_streams;i++)
			{					
				sp = iperf_create_tcp_stream(window, sockopt); 
				sp->remote_port = temp.remote_port;
				iperf_add_stream(test,sp);
			}
			break;
				
		case Pudp:
			for(i=0;i<test->num_streams;i++)
			{
				sp = iperf_create_udp_stream(bw, size, sockopt); 
				sp->remote_port = temp.remote_port;
				iperf_add_stream(test,sp);
			}			
			break;
				
		default:
			for(i=0;i<test->num_streams;i++)
			{
				sp = iperf_create_stream(sockopt); 
				sp->remote_port = temp.remote_port;
				iperf_add_stream(test,sp);
			}			
			break;
	}
	
	printf("streams created \n");
	
	for(i=0;i<test->num_streams;i++)
	{
		if(test->proto == Ptcp)
		printf(" %d is the windowsize for tcp\n",((struct iperf_tcp_settings *)(sp->settings))->window_size );
		
		else
		printf(" %d is the rate for udp\n",((struct iperf_udp_settings *)(sp->settings))->rate );	
		if(sp->next!= NULL)
			sp = sp->next;
	}
	
	
	printf("role = %s\n", (test->role == 's')?"Server":"Client");
	printf("duration = %d\n", test->duration);
	printf("protocol = %s\n", (test->proto == Ptcp)?"TCP":"UDP");
	printf("interval = %d\n", test->stats_interval);
	printf("Local port = %d\n", test->streams->local_port);
	printf("Remote port = %d\n", test->streams->remote_port);
	printf("Parallel streams = %d\n", test->num_streams);
	
	// depending whether client or server, we would call function ?
	iperf_init_test(test);
	// init_test and init_stream functions 
	//run function 		
	
	return 0;
}
	