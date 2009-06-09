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

	 // should call internally init_stream - may be add_stream
	
	
	
	
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
	
	//initialise sp with 0
    memset(sp, 0, sizeof(struct iperf_stream));
	
	sp->local_port = 5001;
	sp->remote_port = 5001;
	
	memset(&sp->remote_addr, 0, sizeof(struct sockaddr_storage));
	memset(&sp->local_addr, 0, sizeof(struct sockaddr_storage));
	
	
	((struct iperf_settings *)(sp->settings))->options = sockopt;
	
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
	
	//initialise sp with 0
    memset(sp, 0, sizeof(struct iperf_stream));
	
	sp->local_port = 5001;
	sp->remote_port = 5001;
	
	memset(&sp->remote_addr, 0, sizeof(struct sockaddr_storage));
	memset(&sp->local_addr, 0, sizeof(struct sockaddr_storage));
	
	tcp_settings->options = sockopt;
	tcp_settings->window_size =window;
	
	sp->settings = tcp_settings;
	printf("memory allocated - TCP\n");	
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
	//initialise sp with 0
    memset(sp, 0, sizeof(struct iperf_stream));
	
	sp->local_port = 5001;
	sp->remote_port = 5001;
	
	memset(&sp->remote_addr, 0, sizeof(struct sockaddr_storage));
	memset(&sp->local_addr, 0, sizeof(struct sockaddr_storage));
	
	udp_settings->options = sockopt;
	udp_settings->rate = rate;
    udp_settings->packet_size = size;
	
	return sp;		
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




int
main(int argc, char **argv)
{
	int rc;
    char ch;
    struct iperf_test *test;
	struct iperf_stream *sp;
	struct iperf_sock_opts *sockopt=NULL;
	int window= 1024,rate,size;
	int i,bw, buffer_size;
	
	
	sockopt = (struct iperf_sock_opts *)malloc(sizeof(struct iperf_sock_opts));
	
	
	test= iperf_create_test();
		
	while( (ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:", longopts, NULL)) != -1 )
        switch (ch) {
            case 'c':
                test->role = 'c';
                //test->remote_ip_addr->ss_len =(int) malloc(strlen(optarg));
               //(struct sockaddr_in)(test->remote_ip_addr), (struct sockaddr_in)optarg;
                break;
            case 'p':
                test->streams->remote_port = atoi(optarg);
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
				iperf_add_stream(test,sp);
			}
			break;
				
		case Pudp:
			for(i=0;i<test->num_streams;i++)
			{
				sp = iperf_create_udp_stream(rate, size, sockopt); 
				iperf_add_stream(test,sp);
			}			
			break;
				
		default:
			for(i=0;i<test->num_streams;i++)
			{
				sp = iperf_create_stream(sockopt); 
				iperf_add_stream(test,sp);
			}			
			break;
	}
	
	printf("streams created \n");
	
	for(i=0;i<test->num_streams;i++)
	{
		printf(" %d is the windowsize for tcp\n",((struct iperf_tcp_settings *)(sp->settings))->window_size );
		if(sp->next!= NULL)
			sp = sp->next;
	}
		   
}
	