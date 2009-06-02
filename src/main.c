/*
 * iperfjd -- greatly simplified version of iperf with the same interface
 * semantics
 *
 * kprabhu - 1st june 2009 - server side code with select
 * with updated linked list functions.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>
#include <stdint.h>

#include <sys/time.h>

#include "iperf.h"
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

struct iperf_stream
{
    int sock;  /* local socket */
    struct sockaddr_in local; /* local address */
    struct sockaddr_in peer; /* peer address */
	
    uint64_t bytes_in;
    uint64_t bytes_out;
	
    pthread_t thread;
	
    char local_addr[512], peer_addr[512];
	
    void *stats; /* ptr to protocol specific stats */
	
    void *(*server)(void *sp);
    void *(*client)(void *sp);
	
    struct iperf_settings *settings;
	
    struct iperf_stream *next;
};

// Run routines for TCP and UDP- will be called by pthread_create() indirectly
void *udp_client_thread(struct iperf_stream *sp);
void *udp_server_thread(struct iperf_stream *sp);
void *tcp_client_thread(struct iperf_stream *sp);
void *tcp_server_thread(struct iperf_stream *sp);

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
{ NULL,             0,                      NULL,   0   }
};

struct iperf_settings
{
    char mode;
    char proto;
    char *client;
    int port;
    int sock;
    iperf_size_t bw;
    int duration;
    int threads;
    iperf_size_t bufsize;
    iperf_size_t window;
    struct sockaddr_in sa;
};

void
default_settings(struct iperf_settings *settings)
{
    settings->mode = Mundef;
    settings->proto = Ptcp;
    settings->client = NULL;
    settings->port = 5001;
    settings->sock = -1;
    settings->bw = 1000000;
    settings->duration = 10;
    settings->threads = 1;
    settings->bufsize = DEFAULT_UDP_BUFSIZE;
    settings->window = 1024*1024;
    memset(&settings->sa, 0, sizeof(struct sockaddr_in));
}

struct iperf_stream *streams;  /* head of list of streams */
int done = 0;




/*--------------------------------------------------------
 * Displays the current list of streams
 -------------------------------------------------------*/
void Display()
{
	struct iperf_stream *n;
	n= streams;
	int count=1;
	
	printf("===============DISPLAY==================\n");
	
	while(1)
	{
		if(n->next==NULL)
		{			
			printf("position-%d\tsp=%d\tsocket=%d\n",count++,(int)n,n->sock);
			break;
		}
		
		else
		{
			printf("position-%d\tsp=%d\tsocket=%d\n",count++,(int)n,n->sock);
			if(n->next==NULL)
			{
				printf("=================END====================\n");
				break;
			}
			n=n->next;
		}
	}
}



/*--------------------------------------------------------
 * sets the parameters for the new stream created
 -------------------------------------------------------*/
struct iperf_stream *
new_stream(int s, struct iperf_settings *settings)
{
    struct iperf_stream *sp;
    socklen_t len;
	
    sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if(!sp) {
        perror("malloc");
        return(NULL);
    }
	
	//initialise sp with 0
    memset(sp, 0, sizeof(struct iperf_stream));
	
	// copy settings and passed socket into stream
    sp->settings = settings;
    sp->sock = s;
	
	
    len = sizeof sp->local;
    if(getsockname(sp->sock, (struct sockaddr *) &sp->local, &len) < 0) {
        perror("getsockname");
        free(sp);
        return(NULL);
    }
	
	//converts the local ip into string address
    if(inet_ntop(AF_INET, (void *) &sp->local.sin_addr,
				 (void *) &sp->local_addr, 512) == NULL) {
        perror("inet_pton");
    }
	
	//stores the socket id.
    if(getpeername(sp->sock, (struct sockaddr *) &sp->peer, &len) < 0) {
        perror("getpeername");
        free(sp);
        return(NULL);
    }
	
	// converts the remote ip into string address
    if(inet_ntop(AF_INET, (void *) &sp->peer.sin_addr,
				 (void *) &sp->peer_addr, 512) == NULL) {
        perror("inet_pton");
    }
	
	// sets appropriate function pointer
    switch (settings->proto) {
        case Ptcp:
            sp->client = (void *) tcp_client_thread;
            sp->server = (void *) tcp_server_thread;
            break;
        case Pudp:
            sp->client = (void *) udp_client_thread;
            sp->server = (void *) udp_server_thread;
            break;
        default:
            assert(0);
            break;
    }
	
    if(set_tcp_windowsize(sp->sock, settings->window, 
						  settings->mode == Mserver ? SO_RCVBUF : SO_SNDBUF) < 0)
        fprintf(stderr, "unable to set window size\n");
	
    int x;
    x = getsock_tcp_windowsize(sp->sock, SO_RCVBUF);
    if(x < 0)
        perror("SO_RCVBUF");
	
    printf("RCV: %d\n", x);
	
    x = getsock_tcp_windowsize(sp->sock, SO_SNDBUF);
    if(x < 0)
        perror("SO_SNDBUF");
	
    printf("SND: %d\n", x);
	
    return(sp);
}



/*--------------------------------------------------------
 * add a stream into stream_list linked list
 -------------------------------------------------------*/
void
add_stream(struct iperf_stream *sp)
{
    struct iperf_stream *n;
	
    if(!streams)
        streams = sp;
    else {
        n = streams;
        while(n->next)
            n = n->next;
        n->next = sp;
    }
}


/*--------------------------------------------------------
 * delete the stream
 -------------------------------------------------------*/
int
free_stream(struct iperf_stream *sp)
{
	
	struct iperf_stream *prev,*start;
	prev = streams;
	start = streams;
	
	if(streams->sock==sp->sock)
	{
		streams=streams->next;
		return 0;
	}
	else
	{	
		start= streams->next;
		
		while(1)
		{
			if(start->sock==sp->sock)
			{				
				prev->next = sp->next;
				free(sp);
				return 0;				
			}
			
			if(start->next!=NULL)
			{
				start=start->next;
				prev=prev->next;
			}
		}
		
		return -1;
	}
	
}


/*--------------------------------------------------------
 * update the stream
 -------------------------------------------------------*/
struct iperf_stream * 
find_update_stream(int j, int result)
{
	struct iperf_stream *n;
	n=streams;
	//find the correct stream for update
	while(1)
	{
		if(n->sock==j)
		{
			n->bytes_in+= result;	//update the byte count
			break;
		}
		
		if(n->next==NULL)
			break;
		n = n->next;
	}
	
	return n;	
}

/*--------------------------------------------------------
 * Display connected message
 -------------------------------------------------------*/
void connect_msg(struct iperf_stream *sp)
{
    char *ipl, *ipr;
	
    ipl = (char *) &sp->local.sin_addr;
    ipr = (char *) &sp->peer.sin_addr;
	
    printf("[%3d] local %s port %d connected with %s port %d\n",
		   sp->sock, 
		   sp->local_addr, htons(sp->local.sin_port),
		   sp->peer_addr, htons(sp->peer.sin_port));
}


/*--------------------------------------------------------
 * UDP client functionality.
 -------------------------------------------------------*/
void *
udp_client_thread(struct iperf_stream *sp)
{
    int i;
    int64_t delayns, adjustns, dtargns;
    char *buf;
    struct timeval before, after;
	
    buf = (char *) malloc(sp->settings->bufsize);
    if(!buf) {
        perror("malloc: unable to allocate transmit buffer");
        pthread_exit(NULL);
    }
	
    for(i=0; i < sp->settings->bufsize; i++)
        buf[i] = i % 37;
	
    dtargns = (int64_t) sp->settings->bufsize * SEC_TO_NS * 8;
    dtargns /= sp->settings->bw;
	
    assert(dtargns != 0);
	
    if(gettimeofday(&before, 0) < 0) {
        perror("gettimeofday");
    }
	
    delayns = dtargns;
    adjustns = 0;
    printf("%lld adj %lld delay\n", adjustns, delayns);
    while(!done) {
        send(sp->sock, buf, sp->settings->bufsize, 0);
        sp->bytes_out += sp->settings->bufsize;
		
        if(delayns > 0)
            delay(delayns);
		
        if(gettimeofday(&after, 0) < 0) {
            perror("gettimeofday");
        }
		
        adjustns = dtargns;
        adjustns += (before.tv_sec - after.tv_sec) * SEC_TO_NS;
        adjustns += (before.tv_usec - after.tv_usec) * uS_TO_NS;
		
        if( adjustns > 0 || delayns > 0) {
            //printf("%lld adj %lld delay\n", adjustns, delayns);
            delayns += adjustns;
        }
		
        memcpy(&before, &after, sizeof before);
    }
	
    /* a 0 byte packet is the server's cue that we're done */
    send(sp->sock, buf, 0, 0);
	
    /* XXX: wait for response with server counts */
	
    printf("%llu bytes sent\n", sp->bytes_out);
	
    close(sp->sock);
    pthread_exit(NULL);
}


/*--------------------------------------------------------
 * UDP Server functionality.
 -------------------------------------------------------*/
void *
udp_server_thread(struct iperf_stream *sp)
{
    char *buf, ubuf[UNIT_LEN];
    ssize_t sz;
	
    buf = (char *) malloc(sp->settings->bufsize);
    if(!buf) {
        perror("malloc: unable to allocate receive buffer");
        pthread_exit(NULL);
    }
	
    while((sz = recv(sp->sock, buf, sp->settings->bufsize, 0)) > 0) {
        sp->bytes_in += sz;
    }
	
    close(sp->sock);
    unit_snprintf(ubuf, UNIT_LEN, (double) sp->bytes_in / sp->settings->duration, 'a');
    printf("%llu bytes received %s/sec\n", sp->bytes_in, ubuf);
    pthread_exit(NULL);
}


/*--------------------------------------------------------
 * UDP Reporting routine
 -------------------------------------------------------*/
void
udp_report(int final)
{
}



/*--------------------------------------------------------
 * UDP Reporting routine
 -------------------------------------------------------*/
void
tcp_report(int final)
{
}


/*--------------------------------------------------------
 * TCP client functionality
 * -------------------------------------------------------*/

void *
tcp_client_thread(struct iperf_stream *sp)
{
    int i;
    char *buf;
	
    buf = (char *) malloc(sp->settings->bufsize);
    if(!buf) {
        perror("malloc: unable to allocate transmit buffer");
        pthread_exit(NULL);
    }
	
    printf("window: %d\n", getsock_tcp_windowsize(sp->sock, SO_SNDBUF));
	
    for(i=0; i < sp->settings->bufsize; i++)
        buf[i] = i % 37;
	
    while(!done) {
        send(sp->sock, buf, sp->settings->bufsize, 0);
        sp->bytes_out += sp->settings->bufsize;
    }
	
    /* a 0 byte packet is the server's cue that we're done */
    send(sp->sock, buf, 0, 0);
	
    /* XXX: wait for response with server counts */
	
    printf("%llu bytes sent\n", sp->bytes_out);
	
    close(sp->sock);
    pthread_exit(NULL);
}

/*--------------------------------------------------------
 * TCP Server functionality
 * -------------------------------------------------------*/
void *
tcp_server_thread(struct iperf_stream *sp)
{
    char *buf, ubuf[UNIT_LEN];
    ssize_t sz;
	
    buf = (char *) malloc(sp->settings->bufsize);
    if(!buf) {
        perror("malloc: unable to allocate receive buffer");
        pthread_exit(NULL);
    }
	
    printf("window: %d\n", getsock_tcp_windowsize(sp->sock, SO_RCVBUF));
	
    while( (sz = recv(sp->sock, buf, sp->settings->bufsize, 0)) > 0) {
        sp->bytes_in += sz;
    }
	
    close(sp->sock);
    unit_snprintf(ubuf, UNIT_LEN, (double) sp->bytes_in / sp->settings->duration, 'a');
    printf("%llu bytes received %s/sec\n", sp->bytes_in, ubuf);
    pthread_exit(NULL);
}


/*--------------------------------------------------------
 * This is code for Client 
 * -------------------------------------------------------*/
int
client(struct iperf_settings *settings)
{
    int s, i;
    struct iperf_stream *sp;
    struct timer *timer;
	
    for(i = 0; i < settings->threads; i++) {
        s = netdial(settings->proto, settings->client, settings->port);
        if(s < 0) {
            fprintf(stderr, "netdial failed\n");
            return -1;
        }
		
        set_tcp_windowsize(s, settings->window, SO_SNDBUF);
		
        if(s < 0)
            return -1;
		
        sp = new_stream(s, settings);
        add_stream(sp);
        connect_msg(sp);
		// need to replace this with Select
        pthread_create(&sp->thread, NULL, sp->client, (void *) sp);
    }
	
    timer = new_timer(settings->duration, 0);
	
	// wait till the timer expires
    while(!timer->expired(timer))
        sleep(settings->duration);
	
	// this is checked in UDP/TCP while loop
	// to stop sending packets. Global member
    done = 1;
	
    /* XXX: report */
    sp = streams;
    do {
        pthread_join(sp->thread, NULL);
        sp = sp->next;
    } while (sp);
	
    return 0;
}

/*--------------------------------------------------------
 * This is code for Server 
 * -------------------------------------------------------*/
int
server(struct iperf_settings *settings)
{
    int s,sz;
    struct iperf_stream *sp;
    struct sockaddr_in sa_peer;
    socklen_t len;
    char buf[settings->bufsize], ubuf[UNIT_LEN];
	fd_set readset, tempset;
	int maxfd;
	int peersock, j, result;	
	struct timeval tv;
	
 	char buffer[DEFAULT_TCP_BUFSIZE];
	
	struct sockaddr_in addr;
	
    s = netannounce(settings->proto, NULL, settings->port);
    if(s < 0)
        return -1;
	
    if(set_tcp_windowsize(s, settings->window, SO_RCVBUF) < 0) {
        perror("unable to set window");
        return -1;
    }
	
    printf("-----------------------------------------------------------\n");
    printf("Server listening on %d\n", settings->port);
    int x;
    if((x = getsock_tcp_windowsize(s, SO_RCVBUF)) < 0) 
        perror("SO_RCVBUF");
	
	
    unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
    printf("%s: %s\n",
		   settings->proto == Ptcp ? "TCP window size" : "UDP buffer size", ubuf);
	
    printf("-----------------------------------------------------------\n");
	
    len = sizeof sa_peer;
	
	FD_ZERO(&readset);
	FD_SET(s, &readset);
	maxfd = s;
	
	do {
		
		memcpy(&tempset, &readset, sizeof(tempset));
		tv.tv_sec = 15;			// timeout interval in seconds
		tv.tv_usec = 0;
		
		// using select to check on multiple descriptors.
		result = select(maxfd + 1, &tempset, NULL, NULL, &tv);
		
		if (result == 0) 
		{
			printf("select() timed out!\n");
		}
		else if (result < 0 && errno != EINTR)
		{
			printf("Error in select(): %s\n", strerror(errno));
		}
		else if (result > 0) 
		{
			
			if (FD_ISSET(s, &tempset))
			{
				if(settings->proto== Ptcp)		// New TCP Connection
				{
					len = sizeof(addr);
					peersock = accept(s,(struct sockaddr *) &addr, &len);
					if (peersock < 0) 
					{
						printf("Error in accept(): %s\n", strerror(errno));
					}
					else 
					{							
						FD_SET(peersock, &readset);
						maxfd = (maxfd < peersock)?peersock:maxfd;
						// creating a new stream
						sp = new_stream(peersock, settings);
						add_stream(sp);					
						connect_msg(sp);
												
					}		
					
				}
				
				else if ( settings->proto== Pudp)	//New UDP Connection
				{
					// getting a new UDP packet
					sz = recvfrom(s, buf, settings->bufsize, 0, (struct sockaddr *) &sa_peer, &len);
					if(!sz)
						break;
					
					if(connect(s, (struct sockaddr *) &sa_peer, len) < 0)
					{
						perror("connect");
						return -1;
					}
					
					// get a new socket to connect to client
					
					sp = new_stream(s, settings);
					sp->bytes_in += sz;	
					add_stream(sp);
					
					
					s = netannounce(settings->proto, NULL, settings->port);
					if(s < 0) 
						return -1;
					
					// same as TCP -repetation
					FD_SET(s, &readset);
					maxfd = (maxfd < s)?s:maxfd;
					
				}
				
				FD_CLR(s, &tempset);
				
				Display();
			}
			
			
			// scanning all socket descriptors for read
			for (j=0; j<maxfd+1; j++)
			{
				if (FD_ISSET(j, &tempset))
				{					
					do
					{	
						if(settings->proto==Ptcp)
							result = recv(j, buffer,DEFAULT_TCP_BUFSIZE, 0);
						else
							result = recv(j, buffer,DEFAULT_UDP_BUFSIZE, 0);
						
					} while (result == -1 && errno == EINTR);
					
					
					if (result > 0)
					{
						sp=find_update_stream(j,result);
						
					}
					else if (result == 0)
					{
											
						//just find the stream with zero update
						sp = find_update_stream(j,0);											
						
						if(settings->proto == Ptcp)
						{
							printf("window: %d\n", getsock_tcp_windowsize(sp->sock, SO_RCVBUF));
						}
						
						
						unit_snprintf(ubuf, UNIT_LEN, (double) sp->bytes_in / sp->settings->duration, 'a');
						printf("%llu bytes received %s/sec for stream %d\n\n", sp->bytes_in, ubuf,(int)sp);
						close(j);
						FD_CLR(j, &readset);
						free_stream(sp);	// this needs to be a linked list delete
						
					}
					else 
					{
						printf("Error in recv(): %s\n", strerror(errno));
					}
				}      // end if (FD_ISSET(j, &tempset))
			}      // end for (j=0;...)
		}      // end else if (result > 0)
	} while (1);
	
	
	
    return 0;
}

int
main(int argc, char **argv)
{
    int rc;
    char ch;
    struct iperf_settings settings;
	
    default_settings(&settings);
	
    while( (ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:", longopts, NULL)) != -1 )
        switch (ch) {
            case 'c':
                settings.mode = Mclient;
                settings.client = malloc(strlen(optarg));
                strcpy(settings.client, optarg);
                break;
            case 'p':
                settings.port = atoi(optarg);
                break;
            case 's':
                settings.mode = Mserver;
                break;
            case 't':
                settings.duration = atoi(optarg);
                break;
            case 'u':
                settings.proto = Pudp;
                break;
            case 'P':
                settings.threads = atoi(optarg);
                break;
            case 'b':
                settings.bw = unit_atoi(optarg);
                break;
            case 'l':
                settings.bufsize = atol(optarg);
                break;
            case 'w':
                settings.window = unit_atoi(optarg);
                break;
        }
	
    if (settings.proto == Ptcp && settings.bufsize == DEFAULT_UDP_BUFSIZE)
        settings.bufsize = DEFAULT_TCP_BUFSIZE; /* XXX: this might be evil */
	
    switch (settings.mode) {
        case Mclient:
            rc = client(&settings);
            break;
        case Mserver:
            rc = server(&settings);
            break;
        case Mundef:
            /* FALLTHRU */
        default:
            printf("must specify one of -s or -c\n");
            rc = -1;
            break;
    }
	
    return rc;
}
