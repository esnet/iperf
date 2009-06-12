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


int iperf_tcp_recv(struct iperf_stream *sp)
{
	int result;
	int size = ((struct iperf_settings *)(sp->settings))->socket_bufsize;
	char buffer[DEFAULT_TCP_BUFSIZE];
	
	do{
		result = recv(sp->socket, buffer, DEFAULT_TCP_BUFSIZE, 0);				
	} while (result == -1 && errno == EINTR);
	
	sp->result->bytes_received+= result;
	
	return result;
}

int iperf_udp_recv(struct iperf_stream *sp)
{
	int result;
	int size = ((struct iperf_settings *)(sp->settings))->bufsize;
	char buffer[DEFAULT_UDP_BUFSIZE];
	
	
	do{
		result = recv(sp->socket, buffer, DEFAULT_UDP_BUFSIZE, 0);
		
	} while (result == -1 && errno == EINTR);
	
	sp->result->bytes_received+= result;
	
	return result;	
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
	
	testp->default_settings->socket_bufsize = 1024*1024;
	testp->default_settings->bufsize = DEFAULT_UDP_BUFSIZE;
	
	
}
	
void iperf_init_test(struct iperf_test *test)
{
	char ubuf[UNIT_LEN];
	struct iperf_stream *sp;
	int i;
	
	if(test->role == 's')
	{		
		printf("Into init\n");
		
		test->listener_sock = netannounce(test->protocol, NULL, ntohs(((struct sockaddr_in *)(test->local_addr))->sin_port));
		if( test->listener_sock < 0)
			exit(0);
				
		
		if(set_tcp_windowsize( test->listener_sock, test->default_settings->socket_bufsize, SO_RCVBUF) < 0) 
		{
			perror("unable to set window");
			return -1;
		}
		
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

void iperf_free_stream(struct iperf_test *test, struct iperf_stream *sp)
{
    free(sp->settings);
    free(sp->result);
    // TODO: free interval results
    free(test->remote_addr);
    free(test->local_addr);
    free(sp);
}

struct iperf_stream *iperf_new_stream(struct iperf_test *testp)
{
	
	struct iperf_stream *sp;
	
    sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if(!sp)
	{
        perror("malloc");
        return(NULL);
    }
	
	
    memset(sp, 0, sizeof(struct iperf_stream));
	
	sp->settings = (struct iperf_settings *) malloc(sizeof(struct iperf_settings));
    memcpy(sp->settings, testp->default_settings, sizeof(struct iperf_settings));
	
	sp->result = (struct iperf_stream_result *) malloc(sizeof(struct iperf_stream_result));
	//memset(&sp->result, 0, sizeof(struct iperf_stream_result));

    testp->remote_addr = (struct sockaddr_storage *) malloc(sizeof(struct sockaddr_storage));
	memset(testp->remote_addr, 0, sizeof(struct sockaddr_storage));

    testp->local_addr = (struct sockaddr_storage *) malloc(sizeof(struct sockaddr_storage));
	memset(testp->local_addr, 0, sizeof(struct sockaddr_storage));
		
	sp->socket = -1;
	
	printf("creating new stream in - iperf_new_stream()\n");
	
	sp->result->bytes_received = 0;
	sp->result->bytes_sent = 0;
		
	return sp;
}

struct iperf_stream *iperf_new_tcp_stream(struct iperf_test *testp)
{	
	struct iperf_stream *sp;
		
	sp = (struct iperf_stream *) iperf_new_stream(testp);
    if(!sp) {
        perror("malloc");
        return(NULL);
    }
	
	//sp->settings = testp->default_settings;

    sp->rcv = iperf_tcp_recv;
    //sp->snd = iperf_tcp_send;
    //sp->update_stats = iperf_tcp_update_stats;
	
	return sp;		
}

struct iperf_stream * iperf_new_udp_stream(struct iperf_test *testp)
{	
	struct iperf_stream *sp;
		
	sp = (struct iperf_stream *) iperf_new_stream(testp);
    if(!sp) {
        perror("malloc");
        return(NULL);
    }
	
	sp->settings = testp->default_settings;
	
    sp->rcv = &iperf_udp_recv;
    //sp->snd = iperf_udp_send;
    //sp->update_stats = iperf_udp_update_stats;	
   
	return sp;
}

int iperf_udp_accept(struct iperf_test *test)
{
	struct iperf_stream *sp;
	struct sockaddr_in sa_peer;	
	char *buf;
	socklen_t len;
	int sz;
	
    buf = (char *) malloc(test->default_settings->bufsize);
	
	sp = iperf_new_udp_stream(test);
	
	len = sizeof sa_peer;
	
	sz = recvfrom(test->listener_sock, buf, test->default_settings->bufsize, 0, (struct sockaddr *) &sa_peer, &len);
	
	
	if(!sz)
		return -1;
	
	if(connect(test->listener_sock, (struct sockaddr *) &sa_peer, len) < 0)
	{
		perror("connect");
		return -1;
	}
	
	sp->socket = test->listener_sock;
	sp->result->bytes_received += sz;
	
	iperf_init_stream(sp, test);	
	iperf_add_stream(test, sp);	
	
	sp->socket = netannounce(test->protocol, NULL,sp->local_port);
	if(sp->socket < 0) 
		return -1;
	
	FD_SET(sp->socket, &(test->read_set));
	test->max_fd = (test->max_fd < sp->socket)?sp->socket:test->max_fd;
	
	printf(" socket created for new UDP client \n");
	fflush(stdout);
	
	return 0;
	
}	

int iperf_tcp_accept(struct iperf_test *test)
{
	socklen_t len;
	struct sockaddr_in addr;	
	int peersock;
		
	struct iperf_stream *sp;
	
	len = sizeof(addr);
	peersock = accept(test->listener_sock,(struct sockaddr *) &addr, &len);
	if (peersock < 0) 
	{
		printf("Error in accept(): %s\n", strerror(errno));
		return -1;
	}
	else 
	{	
		
		
		 sp = iperf_new_tcp_stream(test);
		//setnonblocking(peersock);
		
		FD_SET(peersock,&(test->read_set));
		test->max_fd = (test->max_fd < peersock)?peersock:test->max_fd;
		
		sp->socket = peersock;		
		iperf_init_stream(sp, test);		
		iperf_add_stream(test, sp);					
		//connect_msg(sp);
		
		printf("%d socket created for new TCP client \n", sp->socket);
		
		return 0;
	}		
}

void iperf_init_stream(struct iperf_stream *sp, struct iperf_test *testp)
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
    }
	
	if(set_tcp_windowsize(sp->socket, testp->default_settings->socket_bufsize, 
						  testp->role == 's' ? SO_RCVBUF : SO_SNDBUF) < 0)
        fprintf(stderr, "unable to set window size\n");
	
	
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

struct iperf_stream * update_stream(struct iperf_test *test, int j, int add)
{
	struct iperf_stream *n;
	n=test->streams;
	
	
	
	//find the correct stream for update
	while(1)
	{
		if(n->socket == j)
		{	
			n->result->bytes_received+= add;		
			break;
		}
		
		if(n->next==NULL)
			break;
		
		n = n->next;
	}
	
	return n;	
}

int free_stream(struct iperf_test *test, struct iperf_stream *sp)
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


void iperf_run_server(struct iperf_test *test)
{
	printf("Server running now \n");
	struct timeval tv;
	struct iperf_stream *n;
	char ubuf[UNIT_LEN];
	int j;
	int result;
	
	FD_ZERO(&(test->read_set));
	FD_SET(test->listener_sock,&(test->read_set));
	test->max_fd = test->listener_sock;
	
	while(1)
	{	
		memcpy(&(test->temp_set), &(test->read_set),sizeof(test->temp_set));
		tv.tv_sec = 50;			
		tv.tv_usec = 0;
		
		// using select to check on multiple descriptors.
		result = select(test->max_fd + 1, &(test->temp_set), NULL, NULL, &tv);
		
		if (result == 0) 
			printf("select() timed out!\n");		
		else if (result < 0 && errno != EINTR)
			printf("Error in select(): %s\n", strerror(errno));
		
		// Accept a new connection
		else if(result >0)
		{			
			if (FD_ISSET(test->listener_sock, &(test->temp_set)))
			{			
				test->accept(test);
				
				FD_CLR(test->listener_sock, &(test->temp_set));
				
				//Display();
			}				
		}
		
		// test end message ?
		
		//result_request message ?		
		
		//Process the sockets for read operation
		for (j=0; j< test->max_fd+1; j++)
		{				
			n = test->streams;
			
			if (FD_ISSET(j, &(test->temp_set)))
			{
				// find the correct stream
				n = update_stream(test,j,0);					
				result = n->rcv(n);
				
				if(result == 0)
				{	
					// stream shutdown message
					unit_snprintf(ubuf, UNIT_LEN, (double) (n->result->bytes_received / n->result->duration), 'a');
					printf("%llu bytes received %s/sec for stream %d\n\n",n->result->bytes_received, ubuf,(int)n);						
					close(j);						
					free_stream(test, n);
					FD_CLR(j, &(test->read_set));	
				}
				else 
				{
					printf("Error in recv(): %s\n", strerror(errno));
				}
			}      // end if (FD_ISSET(j, &temp_set))
			
		}// end for (j=0;...)
		
	}// end while
}


int iperf_run(struct iperf_test *test)
{
	
	// don't see any other way of assigning this anywhere 
		
	if(test->role == 's')
	{
		iperf_run_server(test);
		
		return 0;
	}
				
			
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
		return 0;
	}
	
	return -1;
	
}

int
main(int argc, char **argv)
{
	char ch;
	int i;
    struct iperf_test *test;
	struct iperf_stream *sp, temp;
	struct sockaddr_in *addr_local, *addr_remote;
	
		
	addr_local = (struct sockaddr_in *)malloc(sizeof (struct sockaddr_in));
	addr_remote = (struct sockaddr_in *)malloc(sizeof (struct sockaddr_in));
	
	
	test= iperf_new_test();
	
	// need to set the defaults for default_settings too
	iperf_defaults(test);
		
	while( (ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:", longopts, NULL)) != -1 )
        switch (ch) {
            case 'c':
                test->role = 'c';	
				//remote_addr
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
                test->protocol = Pudp;
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
	printf("protocol = %s\n", (test->protocol == Ptcp)?"TCP":"UDP");
	printf("interval = %d\n", test->stats_interval);	
	printf("Parallel streams = %d\n", test->num_streams);
	
	test->local_addr = (struct sockaddr_storage *) addr_local;	
	test->remote_addr = (struct sockaddr_storage *) addr_remote;	
	

    switch(test->protocol)
    {
        case Ptcp:
            test->accept = iperf_tcp_accept;
            test->new_stream = iperf_new_tcp_stream;
            break;
        case Pudp:
            test->accept = iperf_udp_accept;
            test->new_stream = iperf_new_udp_stream;
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
