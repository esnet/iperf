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

/* TCP/UDP server functions */

/*--------------------------------------------------------
 * UDP Server new connection
 -------------------------------------------------------*/

int udp_server_accept(struct iperf_test *test, int maxfd, fd_set *read_set)
{
	struct iperf_stream *sp;
	struct sockaddr_in sa_peer;
	struct iperf_sock_opts *sockopt;
	char *buf;
	socklen_t len;
	int sz,size, rate;

        buf = (char *) malloc(test->default_settings->bufsize);
	
	sp = sp = iperf_new_udp_stream(test->default_settings->rate, test->default_settings->bufsize);
	
	len = sizeof sa_peer;
	
	// getting a new UDP packet
	printf("before rcvfrm \n");
	
	sz = recvfrom(test->listener_sock, buf, test->default_settings->bufsize, 0, (struct sockaddr *) &sa_peer, &len);
	
	printf(" after rcvfrm \n");
	
	if(!sz)
		return -1;
	
	if(connect(test->listener_sock, (struct sockaddr *) &sa_peer, len) < 0)
	{
		perror("connect");
		return -1;
	}
	
	sp->socket = test->listener_sock;
	
	iperf_init_stream(sp);
	
	sp->result->bytes_received += sz;
	iperf_add_stream(test, sp);	
	
	sp->socket = netannounce(test->protocol, NULL,sp->local_port);
	if(sp->socket < 0) 
		return -1;
	
	FD_SET(sp->socket, read_set);
	maxfd = (maxfd < sp->socket)?sp->socket:maxfd;
	
	printf(" socket created for new UDP client \n");
	fflush(stdout);
	return maxfd;
}	

/*--------------------------------------------------------
 * TCP new connection 
 * -------------------------------------------------------*/
int 
tcp_server_accept(struct iperf_test *test, int maxfd, fd_set *read_set)
{
	socklen_t len;
	struct sockaddr_in addr;
	struct iperf_sock_opts *sockopt;
	int peersock;
	int window = 1024*1024;
	
	struct iperf_stream *sp;
	
	len = sizeof(addr);
	peersock = accept(test->listener_sock,(struct sockaddr *) &addr, &len);
	if (peersock < 0) 
	{
		printf("Error in accept(): %s\n", strerror(errno));
		return 0;
	}
	else 
	{		
        sp = iperf_new_tcp_stream(test);
		//setnonblocking(peersock);
		
		FD_SET(peersock, read_set);
		maxfd = (maxfd < peersock)?peersock:maxfd;
		
		sp->socket = peersock;		
		iperf_init_stream(sp);		
		iperf_add_stream(test, sp);					
		//connect_msg(sp);
		
		printf(" socket created for new TCP client \n");
		
		return maxfd;
	}
	
	return -1;
}

struct iperf_test *iperf_new_test()
{
	struct iperf_test *testp;
	
	testp = (struct iperf_test *) malloc(sizeof(struct iperf_test));
	if(!testp)
	{
		perror("malloc");
		return(NULL);
	}
	
	// initialise everything to zero
	memset(testp, 0, sizeof(struct iperf_test));

    testp->remote_addr = (struct sockaddr_storage *) malloc(sizeof(struct sockaddr_storage));
	memset(testp->remote_addr, 0, sizeof(struct sockaddr_storage));

    testp->local_addr = (struct sockaddr_storage *) malloc(sizeof(struct sockaddr_storage));
	memset(testp->local_addr, 0, sizeof(struct sockaddr_storage));

    testp->default_settings = (struct iperf_settings *) malloc(sizeof(struct iperf_settings));
    memset(testp->default_settings, 0, sizeof(struct iperf_settings));
	
	// return an empty iperf_test* with memory alloted.
	return testp;
}

void iperf_defaults(struct iperf_test *testp)
{
	testp->protocol = Ptcp;
	testp->role = 's';
		
	testp->duration = 10;
	
	testp->stats_interval = 0;
	testp->reporter_interval = 0;
		
	testp->num_streams = 1;
}
	
void iperf_init_test(struct iperf_test *test)
{
	char ubuf[UNIT_LEN];
	struct iperf_stream *sp;
	int i;
	
	if(test->role == 's')
	{		
		printf("Into init\n");
		
		test->listener_sock = netannounce(test->protocol, NULL, ((struct sockaddr_in *)(test->local_addr))->sin_port);
		if( test->listener_sock < 0)
			exit(0);
				
		/*
		if(set_tcp_windowsize( test->streams->socket, (test->streams->settings->window_size, SO_RCVBUF) < 0) 
		{
			perror("unable to set window");
			return -1;
		}*/
		
		printf("-----------------------------------------------------------\n");
		printf("Server listening on %d\n",ntohs(((struct sockaddr_in *)(test->local_addr))->sin_port));		// need to change this port assignment
		int x;
		if((x = getsock_tcp_windowsize( test->listener_sock, SO_RCVBUF)) < 0) 
			perror("SO_RCVBUF");	
	
		
		unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
		printf("%s: %s\n",
			   test->protocol == Ptcp ? "TCP window size" : "UDP buffer size", ubuf);
		printf("-----------------------------------------------------------\n");
		
	}
	
	else if( test->role == 'c')
	{
		sp = test->streams;
		
		// initiate for each client stream
		for(i = 0; i < test->num_streams; i++)
		{
			
			sp->socket = netdial(test->protocol,"127.0.0.1", ntohs(((struct sockaddr_in *)(test->local_addr))->sin_port));
			if(sp->socket < 0) 
			{
				fprintf(stderr, "netdial failed\n");
				exit(0);
			}
			printf("The socket created for client at %d\n", sp->socket);
						
			if(sp->next == NULL)
				break;
			sp=sp->next;
		}
	}
								
}

void iperf_free_test(struct iperf_test *test)
{
	free(test);
}

struct iperf_stream *iperf_new_stream(struct iperf_test *testp)
{
	
	struct iperf_stream *sp;
	
    sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if(!sp) {
        perror("malloc");
        return(NULL);
    }
    memset(sp, 0, sizeof(struct iperf_stream));
	
	sp->settings = (struct iperf_settings *) malloc(sizeof(struct iperf_settings));
    memcpy(sp->settings, testp->default_settings, sizeof(struct iperf_settings));
	
	sp->result = (struct iperf_stream_result *) malloc(sizeof(struct iperf_stream_result));
	memset(&sp->result, 0, sizeof(struct iperf_stream_result));

    testp->remote_addr = (struct sockaddr_storage *) malloc(sizeof(struct sockaddr_storage));
	memset(testp->remote_addr, 0, sizeof(struct sockaddr_storage));

    testp->local_addr = (struct sockaddr_storage *) malloc(sizeof(struct sockaddr_storage));
	memset(testp->local_addr, 0, sizeof(struct sockaddr_storage));
		
	sp->socket = -1;
	
	sp->result->bytes_received = 0;
	sp->result->bytes_sent = 0;
		
	return sp;
}

void iperf_free_stream(struct iperf_stream *sp)
{
    free(sp->settings);
    free(sp->result);
    // TODO: free interval results
    free(sp->remote_addr);
    free(sp->local_addr);
    free(sp);
}

struct iperf_stream *iperf_new_tcp_stream(struct iperf_test *testp)
{	
	struct iperf_stream *sp;
	struct iperf_stream_result *result;
	
	sp = (struct iperf_stream *) iperf_new_stream(testp);
    if(!sp) {
        perror("malloc");
        return(NULL);
    }

    sp->recv = iperf_tcp_recv;
    sp->send = iperf_tcp_send;
    sp->update_stats = iperf_tcp_update_stats;
	
	return sp;		
}

struct iperf_stream * iperf_new_udp_stream(int rate, int size)
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

struct iperf_stream * 
update_stream(struct iperf_test *test, int j, int add)
{
	struct iperf_stream *n;
	n=test->streams;
	
	
	
	//find the correct stream for update
	while(1)
	{
		if(n->socket == j)
		{
			printf("In update 6\n");
			n->result->bytes_received+= add;		
			break;
		}
		
		if(n->next==NULL)
			break;
		
		n = n->next;
	}
	
	return n;	
}

int
free_stream(struct iperf_test *test, struct iperf_stream *sp)
{
	
	struct iperf_stream *prev,*start;
	prev = test->streams;
	start = test->streams;
	
	if(test->streams->socket == sp->socket){
		
		test->streams = test->streams->next;
		return 0;
	}
	else
	{	
		start= test->streams->next;
		
		while(1)
		{
			if(start->socket == sp->socket){
				
				prev->next = sp->next;
				free(sp);
				return 0;				
			}
			
			if(start->next!=NULL){
				
				start=start->next;
				prev=prev->next;
			}
		}
		
		return -1;
	}
	
}



int rcv(struct iperf_stream *sp)
{
	int result;
	if(sp->protocol == Ptcp)
	{
		char buffer[DEFAULT_TCP_BUFSIZE];
		
		do{
			result = recv(sp->socket, buffer,DEFAULT_TCP_BUFSIZE, 0);				
		} while (result == -1 && errno == EINTR);
		
		sp->result->bytes_received+= result;
		
		return result;
	}
	
	else if (sp->protocol == Pudp)
	{
		char buffer[DEFAULT_UDP_BUFSIZE];
		
		do{
			result = recv(sp->socket, buffer,DEFAULT_UDP_BUFSIZE, 0);				
		} while (result == -1 && errno == EINTR);
		
		sp->result->bytes_received+= result;
		
		return result;
	}	
}



// This function would be big.			   
int iperf_run(struct iperf_test *test)
{

	int rc;
	struct timer *timer;
			
	// don't see any other way of assigning this anywhere 
		
	if(test->role == 's')
	{
		printf("Server running now \n");
		struct timeval tv;
		struct iperf_stream *n;
		char ubuf[UNIT_LEN];
		fd_set read_set, temp_set;
		int maxfd,j;
		int result;
		
		FD_ZERO(&read_set);
		FD_SET(test->listener_sock, &read_set);
		maxfd = test->listener_sock;
		
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
			
			else if(result >0)
			{
				/*accept new connections
				/ if we create a new function for this part, we need to return/ pass select related
				/ parameters like maxfd, read_set etc */
				
				if (FD_ISSET(test->listener_sock, &temp_set))
				{
					if(test->proto == Ptcp)	
					{
						printf(" called TCP accept \n");
						maxfd = tcp_server_accept(test, maxfd, &read_set);
						printf(" called TCP accept \n");
						fflush(stdout);
					}
					else if(test->proto == Pudp)
					{
						printf(" calling udp accept \n");
						maxfd = udp_server_accept(test, maxfd, &read_set);
					}
					
					FD_CLR(test->listener_sock, &temp_set);
					
					//Display();
				}				
			}
					
			for (j=0; j<maxfd+1; j++)
			{				
				n = test->streams;
				
				if (FD_ISSET(j, &temp_set))
				{
					// find the correct stream
					n = update_stream(test,j,0);					
					result = rcv(n);
					
					if(result == 0)
					{	
						unit_snprintf(ubuf, UNIT_LEN, (double) (n->result->bytes_received / n->result->duration), 'a');
						printf("%llu bytes received %s/sec for stream %d\n\n",n->result->bytes_received, ubuf,(int)n);						
						close(j);						
						free_stream(test, n);
						FD_CLR(j, &read_set);	
					}
					else 
					{
						printf("Error in recv(): %s\n", strerror(errno));
					}
				}      // end if (FD_ISSET(j, &temp_set))
				
			}// end for (j=0;...)
			
		}// end while
		
	}// end if(test->role == 's')
				
			
	else if ( test->role == 'c')
	{
		/*
		timer = new_timer(test->duration, 0);
 
		while(!timer->expired(timer))
		{
 
			init(test->streams, test);
 
			// need change the send_Stream() arguements to accept test/ only test
			rc = send_stream(test->streams,test);
 						
		}
		 */
		;
	}
	
}


int
main(int argc, char **argv)
{
	char ch;
    struct iperf_test *test;
	struct iperf_stream *sp, temp;
	struct sockaddr_in *addr_local, *addr_remote;
	int window= 1024*1024, size;
	int i, bw = 100000, buffer_size;
		
	addr_local = (struct sockaddr_in *)malloc(sizeof (struct sockaddr_in));
	addr_remote = (struct sockaddr_in *)malloc(sizeof (struct sockaddr_in));
	
	test= iperf_new_test();
		
	while( (ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:", longopts, NULL)) != -1 )
        switch (ch) {
            case 'c':
                test->role = 'c';	
				// remote_addr
				inet_pton(AF_INET, optarg, &addr_remote->sin_addr);
                break;
				
            case 'p':
                addr_remote->sin_port = htons(atoi(optarg));
                break;
            case 's':
				test->role = 's';		
				addr_local->sin_port = htons(5001);
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
				test->default_settings->rate = atoi(optarg);
                break;
            case 'l':
                test->default_settings->bufsize = atol(optarg);
                break;
            case 'w':
                test->default_settings->socket_bufsize = atoi(optarg);
			case 'i':
				test->stats_interval = atoi(optarg);
				test->reporter_interval = atoi(optarg);
                break;
        }
	
	printf("role = %s\n", (test->role == 's')?"Server":"Client");
	printf("duration = %d\n", test->duration);
	printf("protocol = %s\n", (test->proto == Ptcp)?"TCP":"UDP");
	printf("interval = %d\n", test->stats_interval);	
	printf("Parallel streams = %d\n", test->num_streams);
	
	test->local_addr = (struct sockaddr_storage *) addr_local;	
	test->remote_addr = (struct sockaddr_storage *) addr_remote;	

    switch(test->proto)
    {
        case Ptcp:
            test->accept = iperf_tcp_accept;
            test->new_stream = iperf_new_tcp_stream;
            break;
        case Pudp:
            test->accept = iperf_udp_accept;
            test->new_stream = ipref_new_udp_stream;
            break;
    }

	if(test->role == 'c')
	{
        for(i=0;i<test->num_streams;i++)
        {
            sp = test->new_stream(test);
            sp->remote_port = temp.remote_port;
            iperf_add_stream(test,sp);
        }
    }
		
	printf("streams created \n");
	
		
	// depending whether client or server, we would call function ?
	iperf_init_test(test);
	
	iperf_run(test);
	
	return 0;
}
