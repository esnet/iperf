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
#include <netinet/tcp.h>
#include <sys/time.h>

#include "iperf_api.h"
#include "timer.h"
#include "net.h"
#include "units.h"
#include "tcp_window_size.h"
#include "uuid.h"
#include "locale.h"

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
{"interval",        required_argument,      NULL,   'i'},
{"NoDelay",         no_argument,            NULL,   'N'},
{"Print-mss",       no_argument,            NULL,   'm'},
{"Set-mss",         required_argument,      NULL,   'M'},
{ NULL,             0,                      NULL,   0   }
};


void exchange_parameters(struct iperf_test *test)
{
    int result, size = DEFAULT_TCP_BLKSIZE;
    char *buf = (char *) malloc(size);
    
    struct iperf_test *temp;
    struct iperf_stream *sp;     
    struct param_exchange *param = (struct param_exchange *) buf;

    test->default_settings->cookie = get_uuid();
    
    //setting up exchange parameters 
    param->state = PARAM_EXCHANGE;
    param->blksize = test->default_settings->blksize;
    param->recv_window = test->default_settings->socket_rcv_bufsize;
    param->send_window = test->default_settings->socket_snd_bufsize;
    param->format = test->default_settings->unit_format;
    
    temp = iperf_new_test();
    iperf_defaults(temp);
    temp->role = 'c';
    temp->new_stream = iperf_new_tcp_stream;    
    temp->server_hostname = test->server_hostname;
    temp->server_port = test->server_port;
    
    
    iperf_init_test(temp);
        
    sp = temp->streams;
    sp->settings->state = PARAM_EXCHANGE;
   
    printf("Sending EXCHNG Request \n");
    result = send(sp->socket, buf, size , 0);
    
    do{
        result = recv(sp->socket, buf, size, 0);
    } while (result == -1 && errno == EINTR);
    
    if (result > 0 && buf[0] == ACCESS_DENIED)
    {
        printf("Busy server Detected\n");
        exit(0);
    }
    else
    {
        printf("New connection started \n");
    }   
    
    iperf_free_stream(temp, sp);
    iperf_free_test(temp);
    free(buf);    

}

int param_received(struct iperf_stream *sp, struct param_exchange *param)
{
    int size = sp->settings->blksize;
    char *buf = (char *) malloc(size);
    int result;
   
    result = 1;  /* TODO: actually check cookie */

    if(result)
    {
        sp->settings->blksize = param->blksize;
        sp->settings->socket_rcv_bufsize = param->recv_window;
        sp->settings->unit_format = param->format;
        
        return param->state;
    }
    
    else
    {
        printf("New connection denied\n");
        // send NO to client
        int size = DEFAULT_TCP_BLKSIZE;
        buf = (char *) malloc(size);
        buf[0] = -1;            
        result = send(sp->socket, buf, size, 0);
        return ACCESS_DENIED;
    }
}

void setnonblocking(int sock)
{
	int opts;
	/*
	 opts = fcntl(sock,F_GETFL);
	 if (opts < 0) {
	 perror("fcntl(F_GETFL)");
	 exit(EXIT_FAILURE);
	 }
	 */
	
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock,F_SETFL,opts) < 0)
	{
		perror("fcntl(F_SETFL)");
		exit(EXIT_FAILURE);
	}
	return;
}

void add_interval_list(struct iperf_stream_result *rp, struct iperf_interval_results temp)
{    
    struct iperf_interval_results *n;
    
    struct iperf_interval_results *ip = (struct iperf_interval_results *) malloc(sizeof(struct iperf_interval_results));       
    ip->bytes_transferred = temp.bytes_transferred;
    ip->interval_duration = temp.interval_duration; 
    
    if(!rp->interval_results)
    { 
        rp->interval_results = ip;
    }
    else
    {
        n = rp->interval_results;
        while(n->next)
           n = n->next;
        
         n->next = ip;         
    }
    ip->next = NULL;
}

void display_interval_list(struct iperf_stream_result *rp)
{
    struct iperf_interval_results *n;
    n = rp->interval_results;
    
    while(n)
    {
        printf("Interval = %f\tBytes transferred = %llu\n", n->interval_duration, n->bytes_transferred);
        n = n->next;
    }    
}

void send_result_to_client(struct iperf_stream *sp)
{    
    int result;
    int size = strlen((char *)sp->data) + 1;
    
    printf("BLKSIZE = %d \n", size);
    
    char *buf = (char *) malloc(size);
    if(!buf)
    {
        perror("malloc: unable to allocate transmit buffer");
    }
    
    // adding the string terminator to the message
    buf[strlen((char *)sp->data)] = '\0';
        
    memcpy(buf, sp->data, strlen((char *)sp->data));
    
    result = send(sp->socket, buf, size , 0);
    
    free(buf);
}

void receive_result_from_server(struct iperf_test *test)
{
    int result;
    struct iperf_stream *sp;
    int size =0;
    char *buf = NULL;
   
    //Overriding actual Test parameters for result exchange
    test->protocol = Ptcp;
    test->new_stream = iperf_new_tcp_stream;
    test->num_streams= 1;
    
    iperf_init_test(test);
    sp = test->streams;
       
    sp->settings->state = ALL_STREAMS_END;
    sp->snd(sp);
    
    sp->settings->state = RESULT_REQUEST;
    sp->snd(sp);
    
    // receive from server
    size = sp->settings->blksize;
    buf = (char *) malloc(size);
    
    do{
        result = recv(sp->socket, buf, size, 0);        
    } while (result == -1 && errno == EINTR);
    
    printf( server_reporting, sp->socket);        
    puts(buf);
    
    iperf_free_stream(test, sp);
}

int getsock_tcp_mss( int inSock )
{
    int mss = 0;
 
    int rc;
    socklen_t len;
    assert( inSock >= 0 );
    
    /* query for mss */
    len = sizeof( mss );
    rc = getsockopt( inSock, IPPROTO_TCP, TCP_MAXSEG, (char*) &mss, &len );   
  
    return mss;
}

int set_socket_options(struct iperf_stream *sp, struct iperf_test *tp)
{
    
    socklen_t len;
    
    // -N
    if(tp->no_delay == 1)
    {
        int no_delay = 1;
        len = sizeof(no_delay);
        int rc = setsockopt( sp->socket, IPPROTO_TCP, TCP_NODELAY,
                            (char*) &no_delay, len);
        if(rc == -1)
        {
            perror("TCP_NODELAY");
            return -1;
        }
    }
    
#ifdef TCP_MAXSEG        
    //-M
    if(tp->default_settings->mss > 0)
    {
        int rc;
        int new_mss;
        len = sizeof(new_mss);
        
        assert( sp->socket != -1);
        
            /* set */
        new_mss = tp->default_settings->mss;
        len = sizeof( new_mss );
        rc = setsockopt( sp->socket, IPPROTO_TCP, TCP_MAXSEG, (char*) &new_mss, len);
        if ( rc == -1) {
            perror("setsockoptBING");
            return -1;
        }
            
            /* verify results */
        rc = getsockopt( sp->socket, IPPROTO_TCP, TCP_MAXSEG, (char*) &new_mss, &len);
        if ( new_mss != tp->default_settings->mss )
        {
            perror("mismatch");
            return -1;
        }
    }
#endif    
    return 0;
}

void connect_msg(struct iperf_stream *sp)
{
    char ipl[512], ipr[512];
    
    inet_ntop(AF_INET, (void *) (&((struct sockaddr_in *) &sp->local_addr)->sin_addr), (void *) ipl, sizeof(ipl));
    inet_ntop(AF_INET, (void *) (&((struct sockaddr_in *) &sp->remote_addr)->sin_addr), (void *) ipr, sizeof(ipr));
    
    
        printf("[%3d] local %s port %d connected with %s port %d\n",
           sp->socket,
           ipl, ntohs(((struct sockaddr_in *) &sp->local_addr)->sin_port),
           ipr, ntohs(((struct sockaddr_in *) &sp->remote_addr)->sin_port) );
}

void Display(struct iperf_test *test)
{
    struct iperf_stream *n;
    n= test->streams;
    int count=1;
    
    printf("===============DISPLAY==================\n");
    
    while(1)
    {    
        if(n)
        {
            if(test->role == 'c')
                printf("position-%d\tsp=%d\tsocket=%d\tbytes sent=%llu\n",count++, (int)n, n->socket, n->result->bytes_sent);
            else
                printf("position-%d\tsp=%d\tsocket=%d\tbytes received=%llu\n",count++, (int)n, n->socket, n->result->bytes_received);
            
            if(n->next==NULL)
            {
                printf("=================END====================\n");
                fflush(stdout);
                break;
            }
            n=n->next;
        }
    }
}

int iperf_tcp_recv(struct iperf_stream *sp)
{
    int result, message;
    char ch;
    int size = sp->settings->blksize;
    char *buf = (char *) malloc(size);
    struct param_exchange *param = (struct param_exchange *) buf;
    if(!buf)
    {
        perror("malloc: unable to allocate receive buffer");
    }
    
    do{
        result = recv(sp->socket, buf, size, MSG_WAITALL);        
        
    } while (result == -1 && errno == EINTR);
    
    //interprete the type of message in packet
    //-TODO = change this for Cookie implementation
    if(result > 0)
    {
        ch = buf[0];
        message = (int) ch;
        
        // CHECK: packet length and state
        // printf("result = %d state = %d\n",result, buf[0]);
    }
    
    if(param->state == PARAM_EXCHANGE)
    {
        message = param_received(sp, param);
    }
    
    if(message == 6)
        printf("the blksize = %d\n", sp->settings->blksize);
    
    if(message == 3 || message == 8 || message == 9 )
    {
       // printf("count = %ld result = %d\n", strlen(buf), result);
        printf("Recieved %d from client\n", message);
    }
   
    if(message != STREAM_END)
        sp->result->bytes_received+= result;
    
   free(buf);
   return message;    
}

int iperf_udp_recv(struct iperf_stream *sp)
{   
    int result, message;
    int size = sp->settings->blksize;
    double transit = 0, d = 0;
    char *buf = (char *) malloc(size);
    struct udp_datagram *udp = (struct udp_datagram *) buf;
    struct timeval arrival_time;
    
    if(!buf)
    {
        perror("malloc: unable to allocate receive buffer");
    }
    
    do{
        result = recv(sp->socket, buf, size, 0);        
    
    } while (result == -1 && errno == EINTR);
    
    //interprete the type of message in packet
    if(result > 0)
    {
        message = udp->state;
    }
        
    if(message == STREAM_RUNNING && (sp->stream_id == udp->stream_id))
    {
        sp->result->bytes_received+= result;
        if(udp->packet_count == sp->packet_count + 1)
        sp->packet_count++;
        
        //jitter measurement
        if(gettimeofday(&arrival_time, NULL) < 0) {
            perror("gettimeofday");        
        }
        
        transit =  timeval_diff(&udp->sent_time, &arrival_time);
        d = transit - sp->prev_transit;
        if(d < 0)
            d = -d;
        sp->prev_transit = transit;        
        sp->jitter += (d - sp->jitter)/ 16.0;
        
        
        //OUT OF ORDER PACKETS
        if(udp->packet_count != sp->packet_count)
        {
            if( udp->packet_count < sp->packet_count + 1)
            {
                sp->outoforder_packets++;
                printf("OUT OF ORDER - incoming packet = %d and received packet = %d AND SP = %d\n", udp->packet_count, sp->packet_count, sp->socket);
            }
            else 
                sp->cnt_error += udp->packet_count - sp->packet_count;
        }
        
        // store the latest packet id
        if(udp->packet_count > sp->packet_count)
            sp->packet_count = udp->packet_count;
        
       //printf("incoming packet = %d and received packet = %d AND SP = %d\n", udp->packet_count, sp->packet_count, sp->socket);
        
    }
    
    if(message == 3 || message == 9 || message == 8)
        printf("Recieved %d from client\n", message);
    
    free(buf);
    return message;
    
}

int iperf_tcp_send(struct iperf_stream *sp)
{
    int result,i;
    int size = sp->settings->blksize;
        
    char *buf = (char *) malloc(size);
    
    
    if(!buf)
    {
        perror("malloc: unable to allocate transmit buffer");
    }
    
    switch(sp->settings->state)
    {
            
        case STREAM_BEGIN:
            buf[0]= (STREAM_BEGIN);
            for(i=1; i < size; i++)
                buf[i] = 97;
            break;
            
        case STREAM_END:            
            buf[0]= (STREAM_END);
            for(i=1; i < size; i++)
                buf[i] = 97;
            break;
            
        case RESULT_REQUEST:
            buf[0]= (RESULT_REQUEST);
            for(i=1; i < size; i++)
                 buf[i] = 97;
            break;
            
        case ALL_STREAMS_END:
            buf[0]= (ALL_STREAMS_END);
            for(i=1; i < size; i++)
                 buf[i] = 97;
            break;
            
        case STREAM_RUNNING:
            buf[0]= STREAM_RUNNING;
            for(i=1; i < size; i++)
                 buf[i] = 97;
            break;
        default:
            printf("State of the stream can't be determined\n");
            break;
    }
    
    //applicable for 1st packet sent
    if(sp->settings->state == STREAM_BEGIN)
    {     
        sp->settings->state = STREAM_RUNNING;           
    }
        
    result = send(sp->socket, buf, size , 0);   
    
    if(buf[0] != STREAM_END)
        sp->result->bytes_sent+= size;
    
    free(buf);
    return result;    
}

int iperf_udp_send(struct iperf_stream *sp)
{ 
    int result = 0;
    struct timeval before, after;
    int64_t adjustus, dtargus;    
    
    // the || part ensures that last packet is sent to server - the STREAM_END MESSAGE
    if(sp->send_timer->expired(sp->send_timer) || sp->settings->state == STREAM_END)
    {
        int size = sp->settings->blksize;    
        char *buf = (char *) malloc(size);
        // this is for udp packet/jitter/lost packet measurements
        struct udp_datagram *udp = (struct udp_datagram *) buf;
        struct param_exchange *param =NULL; 
        
        if(!buf)
        {
            perror("malloc: unable to allocate transmit buffer");
        }
        
        dtargus = (int64_t)(sp->settings->blksize) * SEC_TO_US * 8;
        dtargus /= sp->settings->rate;
     
        assert(dtargus != 0);
        
        // record the packet sent time
        if(gettimeofday(&udp->sent_time, 0) < 0)
            perror("gettimeofday");
    
        switch(sp->settings->state)
        {           
            case STREAM_BEGIN:                
                udp->state = STREAM_BEGIN;
                udp->stream_id = (int)sp;
                //udp->packet_count = ++sp->packet_count;
                break;
                
            case STREAM_END:
                udp->state = STREAM_END;
                udp->stream_id  = (int) sp;
                break;
                
            case RESULT_REQUEST:
                udp->state = RESULT_REQUEST;
                udp->stream_id  = (int) sp;
                break;
                
            case ALL_STREAMS_END:
                udp->state = ALL_STREAMS_END;
                break;
           
            case STREAM_RUNNING:
                udp->state = STREAM_RUNNING;
                udp->stream_id  = (int) sp;
                udp->packet_count = ++sp->packet_count;
                break;                
        }        
               
        // applicable for 1st packet sent for each stream
        if(sp->settings->state == STREAM_BEGIN)
        {           
            sp->settings->state = STREAM_RUNNING;                        
        }
        
        if(gettimeofday(&before, 0) < 0)
            perror("gettimeofday");
                
        // TEST: lost packet
        //if(udp->packet_count == 4 ||  udp->packet_count == 5  || udp->packet_count == 6) ;
        //else
        result = send(sp->socket, buf, size, 0);
        
        // CHECK: Packet length and ID
        //if(sp->settings->state == STREAM_RUNNING)
        // printf("State = %d Outgoing packet = %d AND SP = %d\n",sp->settings->state, sp->packet_count, sp->socket);        
                
        if(sp->settings->state == STREAM_RUNNING)
            sp->result->bytes_sent+= result;
        
        if(gettimeofday(&after, 0) < 0)
            perror("gettimeofday");
        
        adjustus = dtargus;
        adjustus += (before.tv_sec - after.tv_sec) * SEC_TO_US ;
        adjustus += (before.tv_usec - after.tv_usec);
        
        if( adjustus > 0) {
           dtargus = adjustus;
        }
        memcpy(&before, &after, sizeof before);
        
        // RESET THE TIMER       
        update_timer(sp->send_timer, 0, dtargus);
        free(buf);
        param = NULL;
        
    } // timer_expired_micro
    
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
    testp->server_port = 5001;   
    
    testp->default_settings->unit_format = 'a';
    
    testp->stats_interval = 0;
    testp->reporter_interval = 0;
    testp->num_streams = 1;    
    testp->default_settings->socket_bufsize = 1024*1024;
    testp->default_settings->blksize = DEFAULT_TCP_BLKSIZE;
    testp->default_settings->rate = RATE;
    testp->default_settings->state = TEST_START;
    testp->default_settings->mss = 0;
}
    
void iperf_init_test(struct iperf_test *test)
{
    char ubuf[UNIT_LEN];
    struct iperf_stream *sp;
    int i, s=0;
    
    if(test->role == 's')
    {
        test->listener_sock_udp = netannounce(Pudp, NULL, test->server_port);
        if( test->listener_sock_udp < 0)
            exit(0);
        
        if(set_tcp_windowsize( test->listener_sock_udp, test->default_settings->socket_bufsize, SO_RCVBUF) < 0) 
        {
            perror("unable to set window");
        }
        
        test->listener_sock_tcp = netannounce(Ptcp, NULL, test->server_port);
        if( test->listener_sock_tcp < 0)
            exit(0);                
        
        if(set_tcp_windowsize( test->listener_sock_tcp, test->default_settings->socket_bufsize, SO_RCVBUF) < 0) 
        {
            perror("unable to set window");
        }
        
        printf("-----------------------------------------------------------\n");
        printf("Server listening on %d\n", test->server_port);
        int x;
        if((x = getsock_tcp_windowsize( test->listener_sock_tcp, SO_RCVBUF)) < 0) 
            perror("SO_RCVBUF");
        
        // TODO - CHANGE THIS 
        unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
        printf("%s: %s\n",
               test->protocol == Ptcp ? "TCP window size" : "UDP buffer size", ubuf);
        printf("-----------------------------------------------------------\n");
        
    }
    
    else if( test->role == 'c')
    {   
        FD_ZERO(&test->write_set);
        FD_SET(s, &test->write_set);
        
        for(i = 0; i < test->num_streams; i++)
        {        
            s = netdial(test->protocol, test->server_hostname, test->server_port);
            if(s < 0) 
            {
                fprintf(stderr, "netdial failed\n");
                exit(0);                
            }
            
            FD_SET(s, &test->write_set);
            test->max_fd = (test->max_fd < s) ? s : test->max_fd;
            
            if( set_tcp_windowsize(s, test->default_settings->socket_bufsize, SO_SNDBUF)) 
            {
                perror("unable to set window");
            }
            
            //setnonblocking(s);
            
            sp = test->new_stream(test);
            sp->socket = s;
            iperf_init_stream(sp, test);
            iperf_add_stream(test, sp);
            

            
            if(test->default_settings->state != RESULT_REQUEST)
            connect_msg(sp);
            
        }
    }                        
}

void iperf_free_test(struct iperf_test *test)
{    
    free(test->default_settings);
    
    close(test->listener_sock_tcp);
    close(test->listener_sock_udp);
    
    test->streams = NULL;
    test->accept = NULL;
    test->stats_callback = NULL;
    test->reporter_callback = NULL;
    test->new_stream = NULL;
    free(test);
}

void *iperf_stats_callback(struct iperf_test *test)
{    
    iperf_size_t cumulative_bytes = 0;
    int i;
    struct iperf_stream *sp = test->streams;
    struct iperf_stream_result *rp = test->streams->result;
    struct iperf_interval_results *ip, temp;    
    
    for(i=0; i< test->num_streams; i++)
    {
        rp = sp->result;
        
        if(!rp->interval_results)
        {            
            if(test ->role == 'c')
                temp.bytes_transferred = rp->bytes_sent;
            else
                temp.bytes_transferred = rp->bytes_received;
            
            gettimeofday(&temp.interval_time, NULL);
            
            temp.interval_duration = timeval_diff(&sp->result->start_time, &temp.interval_time);
            
            gettimeofday( &sp->result->end_time, NULL);
            add_interval_list(rp, temp); 
        }
        
        else
        {
            ip = sp->result->interval_results;
            while(1)
            {
                cumulative_bytes+= ip->bytes_transferred;
                if(ip->next!= NULL)
                    ip = ip->next;
                else
                    break;
            }
            
            if(test ->role == 'c')            
                temp.bytes_transferred = rp->bytes_sent - cumulative_bytes;
            else
                temp.bytes_transferred = rp->bytes_received - cumulative_bytes;
            
            gettimeofday(&temp.interval_time, NULL);
            temp.interval_duration = timeval_diff(&sp->result->start_time, &temp.interval_time);
            
            gettimeofday( &sp->result->end_time, NULL);            
            add_interval_list(rp, temp);            
        }
        
        //display_interval_list(rp);
        cumulative_bytes =0;
        sp= sp->next;        
    }
    
    return 0;
}

char *iperf_reporter_callback(struct iperf_test *test)
{
    int count = 0, total_packets = 0, lost_packets = 0;
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    struct iperf_stream *sp = test->streams;
    iperf_size_t bytes=0;
    double start_time, end_time;
    // need to reassign this
    char *message = (char *) malloc(500);    
    // used to determine the length of reporter buffer
    while(sp)
    {
        count ++;
        sp = sp->next;
    }
      
    char *message_final = (char *) malloc((count+1) * (strlen(report_bw_jitter_loss_header) 
                                                       + strlen(report_bw_jitter_loss_format) + strlen(report_sum_bw_jitter_loss_format)));
    memset(message_final,0, strlen(message_final));
    
    struct iperf_interval_results *ip = test->streams->result->interval_results;
    
    struct iperf_interval_results *ip_prev = ip;
    
    sp = test->streams;
    
    if(test->default_settings->state == TEST_RUNNING) 
    {
        while(sp)
        {
            ip = sp->result->interval_results;
            
            while(ip->next!= NULL)
            {
                ip_prev = ip;
                ip = ip->next;
            }
            
            bytes+= ip->bytes_transferred;
            unit_snprintf(ubuf, UNIT_LEN, (double) (ip->bytes_transferred), 'A');
            
            test->stats_interval = test->stats_interval== 0 ? test->duration : test->stats_interval;            
            
            sprintf(message,report_bw_header);
            strcat(message_final, message);
            
            if(test->streams->result->interval_results->next != NULL)
            {
                unit_snprintf(nbuf, UNIT_LEN, (double) (ip->bytes_transferred / (ip->interval_duration -ip_prev->interval_duration)),
                                                   test->default_settings->unit_format);
                sprintf(message, report_bw_format, sp->socket,ip_prev->interval_duration, ip->interval_duration, ubuf, nbuf);
            }                
            else
            {
                unit_snprintf(nbuf, UNIT_LEN, (double) (ip->bytes_transferred /ip->interval_duration), test->default_settings->unit_format);            
                sprintf(message, report_bw_format, sp->socket, 0.0, ip->interval_duration, ubuf, nbuf);
            }
            strcat(message_final, message);            
            sp = sp->next;            
        }   
       
        unit_snprintf(ubuf, UNIT_LEN, (double) ( bytes), 'A');
        
        if(test->streams->result->interval_results->next != NULL)
        {
            unit_snprintf(nbuf, UNIT_LEN, (double) (ip->bytes_transferred / (ip->interval_duration -ip_prev->interval_duration)),
                          test->default_settings->unit_format);
            sprintf(message, report_sum_bw_format, ip_prev->interval_duration, ip->interval_duration, ubuf, nbuf);
        }            
        else
        {
            unit_snprintf(nbuf, UNIT_LEN, (double) (ip->bytes_transferred /ip->interval_duration), test->default_settings->unit_format);
            sprintf(message, report_sum_bw_format, 0.0, ip->interval_duration, ubuf, nbuf);
        }
            
        strcat(message_final, message);
        free(message);
    }
      
    if(test->default_settings->state == RESULT_REQUEST)    
    {
        sp= test->streams;
        
        while(sp)
        { 
            if(sp->settings->state == STREAM_END)
            {
                if(test->role == 'c')
                    bytes+= sp->result->bytes_sent;
                else               
                    bytes+= sp->result->bytes_received;
                
                if(test->protocol == Pudp)
                {
                    total_packets+= sp->packet_count;
                    lost_packets += sp->cnt_error;
                }
               
                
                if(test->protocol == Ptcp)
                {
                    sprintf(message, report_bw_header);
                    strncat(message_final, message, strlen(report_bw_header));
                }
                else
                {
                    sprintf(message,report_bw_jitter_loss_header);
                    strncat(message_final, message, strlen(report_bw_jitter_loss_header));
                }                
                    
                start_time = timeval_diff(&sp->result->start_time, &sp->result->start_time);
                end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);
                    
                if(test->role == 'c')
                {
                    unit_snprintf(ubuf, UNIT_LEN, (double) (sp->result->bytes_sent), 'A');
                    unit_snprintf(nbuf, UNIT_LEN, (double) (sp->result->bytes_sent / end_time),test->default_settings->unit_format);
                    
                }
                else
                {
                    unit_snprintf(ubuf, UNIT_LEN, (double) (sp->result->bytes_received), 'A');
                    unit_snprintf(nbuf, UNIT_LEN, (double) (sp->result->bytes_received / end_time), test->default_settings->unit_format);
                }            
            
                if( test->protocol == Ptcp)
                {
                    sprintf(message, report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
                    strcat(message_final, message);
                }
                else
                {
                    sprintf(message, report_bw_jitter_loss_format, sp->socket, start_time,
                           end_time, ubuf, nbuf, sp->jitter * 1000, sp->cnt_error, sp->packet_count, (double)(100.0 * sp->cnt_error / sp->packet_count));
                    strcat(message_final, message);
                    
                    if(test->role == 'c')
                    {
                        sprintf(message, report_datagrams, sp->socket, sp->packet_count);
                        strcat(message_final, message);
                    }                    
                    if ( sp->outoforder_packets > 0 )
                        printf( report_sum_outoforder, start_time, end_time, sp->cnt_error);                        
                }
            }
                    
            sp = sp->next;
        }
    
        sp = test->streams;
        
        start_time = timeval_diff(&sp->result->start_time, &sp->result->start_time);
        end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);
        
        unit_snprintf(ubuf, UNIT_LEN, (double) bytes, 'A');
        unit_snprintf(nbuf, UNIT_LEN, (double) bytes / end_time, test->default_settings->unit_format);
        
        if(test->protocol == Ptcp)
        {
            sprintf(message, report_sum_bw_format, start_time, end_time, ubuf, nbuf);
            strcat(message_final, message);
        }
        else
        {
            sprintf(message, report_sum_bw_jitter_loss_format, start_time, end_time, ubuf, nbuf,sp->jitter, lost_packets, total_packets, (double)(100.0 * lost_packets/total_packets));
            strcat(message_final, message);
        }
        
        
        // -m option        
        if((test->print_mss != 0)  && (test->role == 'c'))
        {
            sprintf(message,"\nThe TCP maximum segment size mss = %d \n", getsock_tcp_mss(sp->socket));
            strcat(message_final, message);
        }
        
         free(message);
    }
    
    return message_final;
}

void iperf_free_stream(struct iperf_test *test, struct iperf_stream *sp)
{
    struct iperf_stream *prev,*start;
    prev = test->streams;
    start = test->streams;
    
    if(test->streams->socket == sp->socket)
    {
        test->streams = test->streams->next;        
    }    
    else
    {    
        start= test->streams->next;        
        while(1)
        {
            if(start->socket == sp->socket){                
                prev->next = sp->next;                
                break;
            }
            
            if(start->next!=NULL){
                start=start->next;
                prev=prev->next;
            }
        }
    }
    
    free(sp->settings);
    free(sp->result);
    free(sp->send_timer);
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
       
    sp->socket = -1;
    
    sp->packet_count =0;
    sp->stream_id = (int)sp;
    sp->jitter = 0.0;
    sp->prev_transit = 0.0;
    sp->outoforder_packets = 0;
    sp->cnt_error = 0;
    
    sp->send_timer = NULL;
    
    sp->result->interval_results = NULL;    
    sp->result->bytes_received = 0;
    sp->result->bytes_sent = 0;
    gettimeofday(&sp->result->start_time, NULL);
    
    sp->settings->state = STREAM_BEGIN;
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

    sp->rcv = iperf_tcp_recv;
    sp->snd = iperf_tcp_send;
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
        
    sp->rcv = iperf_udp_recv;
    sp->snd = iperf_udp_send;
       
    return sp;
}

int iperf_udp_accept(struct iperf_test *test)
{
        
    struct iperf_stream *sp;
    struct sockaddr_in sa_peer;    
    char *buf;
    socklen_t len;
    int sz;
                
    buf = (char *) malloc(test->default_settings->blksize);    
    struct udp_datagram *udp = (struct udp_datagram *) buf;
    len = sizeof sa_peer;
    
    sz = recvfrom(test->listener_sock_udp, buf, test->default_settings->blksize, 0, (struct sockaddr *) &sa_peer, &len);
        
    if(!sz)
        return -1;
        
    if(connect(test->listener_sock_udp, (struct sockaddr *) &sa_peer, len) < 0)
    {
        perror("connect");
        return -1;
    }
    
    sp = test->new_stream(test);
    sp->socket = test->listener_sock_udp;
    
    //setting noblock doesn't report back to client
    setnonblocking( sp->socket);
    
    iperf_init_stream(sp, test);    
    iperf_add_stream(test, sp);
       
        
    test->listener_sock_udp = netannounce(Pudp, NULL, test->server_port);
    if(test->listener_sock_udp < 0) 
        return -1;
    
    FD_SET(test->listener_sock_udp, &test->read_set);
    test->max_fd = (test->max_fd < test->listener_sock_udp) ? test->listener_sock_udp : test->max_fd;
    
    if(test->default_settings->state != RESULT_REQUEST)
        connect_msg(sp);
    
    printf("1st UDP data  packet for socket %d has arrived \n", sp->socket);
    sp->stream_id = udp->stream_id;
    sp->result->bytes_received+= sz;
    
    //OUT OF ORDER PACKETS
    if(udp->packet_count != 0)
    {
        if( udp->packet_count < sp->packet_count + 1)
            sp->outoforder_packets++;    
        else 
            sp->cnt_error += udp->packet_count - sp->packet_count;                         
    }
    
    // store the latest packet id
    if(udp->packet_count > sp->packet_count)
        sp->packet_count = udp->packet_count;
    
    //printf("incoming packet = %d and received packet = %d AND SP = %d\n", udp->packet_count, sp->packet_count, sp->socket);
    
    free(buf);    
    return 0;    
}    

int iperf_tcp_accept(struct iperf_test *test)
{
    socklen_t len;
    struct sockaddr_in addr;    
    int peersock;
    struct iperf_stream *sp;
    
    len = sizeof(addr);
    peersock = accept(test->listener_sock_tcp, (struct sockaddr *) &addr, &len);
    if (peersock < 0) 
    {
        printf("Error in accept(): %s\n", strerror(errno));
        return -1;
    }
    else 
    {    
        sp = test->new_stream(test);
        
        //setting noblock doesn't report back to client
        setnonblocking(peersock);
        
        FD_SET(peersock, &test->read_set);
        test->max_fd = (test->max_fd < peersock) ? peersock : test->max_fd;
        
        sp->socket = peersock;
        iperf_init_stream(sp, test);
        iperf_add_stream(test, sp);
        
        if(test->default_settings->state != RESULT_REQUEST)
        connect_msg(sp);
        
        return 0;
    }        
}

void iperf_init_stream(struct iperf_stream *sp, struct iperf_test *testp)
{
    socklen_t len;
    int x;
    len = sizeof(struct sockaddr_in);    
    
    sp->protocol = testp->protocol;
    
    if(getsockname(sp->socket, (struct sockaddr *) &sp->local_addr, &len) < 0) 
    {
        perror("getsockname");        
    }
    
        
    //stores the socket id.
    if(getpeername(sp->socket, (struct sockaddr *) &sp->remote_addr, &len) < 0)
    {
        perror("getpeername");        
    }
    
    
    //set socket options
    
    if(set_tcp_windowsize(sp->socket, testp->default_settings->socket_bufsize, 
                          testp->role == 's' ? SO_RCVBUF : SO_SNDBUF) < 0)
        fprintf(stderr, "unable to set window size\n");
    
    
    x = getsock_tcp_windowsize(sp->socket, SO_RCVBUF);
    if(x < 0)
        perror("SO_RCVBUF");
    // printf("RCV: %d\n", x);
    
    x = getsock_tcp_windowsize(sp->socket, SO_SNDBUF);
    if(x < 0)
        perror("SO_SNDBUF");
    // printf("SND: %d\n", x);
  
    set_socket_options(sp, testp);
    
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

struct iperf_stream * find_stream_by_socket(struct iperf_test *test, int sock)
{
    struct iperf_stream *n;
    n=test->streams;
        
    //find the correct stream for update
    while(1)
    {
        if(n->socket == sock)
        {                    
            break;
        }
        
        if(n->next==NULL)
            break;
        
        n = n->next;
    }
    
    return n;    
}

void iperf_run_server(struct iperf_test *test)
{
    struct timeval tv;
    struct iperf_stream *np, *sp;   
    int j,result, message;
    char *read = NULL;
    
    FD_ZERO(&test->read_set);
    FD_SET(test->listener_sock_tcp, &test->read_set);
    FD_SET(test->listener_sock_udp, &test->read_set);
    
    test->max_fd = test->listener_sock_tcp > test->listener_sock_udp ? test->listener_sock_tcp : test->listener_sock_udp;
    
    test->num_streams = 0;
    test->default_settings->state = TEST_RUNNING;
        
    while(test->default_settings->state != TEST_END)
    {
        FD_COPY(&test->read_set, &test->temp_set);
        tv.tv_sec = 15;            
        tv.tv_usec = 0;
        
        // using select to check on multiple descriptors.
        result = select(test->max_fd + 1, &test->temp_set, NULL, NULL, &tv);
        
        if (result == 0)
            printf("SERVER IDLE : %d sec\n", (int)tv.tv_sec);
            
        
        else if (result < 0 && errno != EINTR)
        {
            printf("Error in select(): %s\n", strerror(errno));
            exit(0);
        }
       
        else if(result >0)
        {
             // Accept a new TCP connection
            if (FD_ISSET(test->listener_sock_tcp, &test->temp_set))
            {
                test->protocol = Ptcp;                
                test->accept = iperf_tcp_accept;
                test->new_stream = iperf_new_tcp_stream;                
                test->accept(test);
                test->default_settings->state = TEST_RUNNING;
                FD_CLR(test->listener_sock_tcp, &test->temp_set);
            }
            
             // Accept a new UDP connection
            else if (FD_ISSET(test->listener_sock_udp, &test->temp_set) )
            {
                test->protocol = Pudp;                
                test->accept = iperf_udp_accept;
                test->new_stream = iperf_new_udp_stream;
                test->accept(test);                
                test->default_settings->state = TEST_RUNNING;
                FD_CLR(test->listener_sock_udp, &test->temp_set);
            }
            
            
        //Process the sockets for read operation
            for (j=0; j< test->max_fd+1; j++)
            {
                if (FD_ISSET(j, &test->temp_set))
                {
                    // find the correct stream
                    np = find_stream_by_socket(test,j);
                    message = np->rcv(np);
                                        
                    if(message == PARAM_EXCHANGE || message == ACCESS_DENIED)
                    {
                        //copy the received settings into test
                        if(message != ACCESS_DENIED)
                            memcpy(test->default_settings, test->streams->settings, sizeof(struct iperf_settings));
                        // FREE ALL STREAMS                        
                         close(np->socket);
                         FD_CLR(np->socket, &test->read_set);
                         iperf_free_stream(test, np);                        
                    }   
                
                    if(message == STREAM_END)
                    {
                        np->settings->state = STREAM_END;                        
                        gettimeofday(&np->result->end_time, NULL);
                        FD_CLR(j, &test->read_set);
                    }
                    
                    if(message == RESULT_REQUEST)
                    {
                        np->settings->state = RESULT_RESPOND;
                        np->data = read;
                        send_result_to_client(np);
                        // FREE ALL STREAMS                        
                        np = test->streams;                        
                        do
                        {
                            sp = np;
                            close(sp->socket);
                            FD_CLR(sp->socket, &test->read_set);
                            np = sp->next;
                            iperf_free_stream(test, sp);
                        } while (np!= NULL);
                        
                        printf("TEST_END\n\n");                        
                       test->default_settings->state = TEST_END;                        
                    }
                    
                    if(message == ALL_STREAMS_END)
                    {
                        //sometimes the server is not getting the STREAM_END message
                        // hence changing the state of all but last stream forcefully
                        np = test->streams;
                        while(np->next)
                        {
                            if(np->settings->state == STREAM_BEGIN)
                            {
                                np->settings->state = STREAM_END;
                                gettimeofday(&np->result->end_time, NULL);
                            }
                            
                            np = np->next;                             
                        }
                        
                        // This is necessary to preserve reporting format
                        test->protocol = test->streams->protocol;
                        
                        test->default_settings->state = RESULT_REQUEST;
                        read = test->reporter_callback(test);
                        puts(read);                        
                        printf("REPORTER CALL + ALL_STREAMS_END\n"); 
                    }
                   
                }// end if (FD_ISSET(j, &temp_set))
            
            }// end for (j=0;...)            
                       
        }// end else (result>0)        
               
    }// end while
    
}

void iperf_run_client(struct iperf_test *test)
{
    int i,result=0;
    struct iperf_stream *sp, *np;
    struct timer *timer, *stats_interval, *reporter_interval;    
    char *buf, *read= NULL;
    int64_t delayus, adjustus, dtargus, remaining, min; 
    struct timeval tv;
    int ret=0;
    
    tv.tv_sec = 15;            // timeout interval in seconds
    tv.tv_usec = 0;
    
    buf = (char *) malloc(test->default_settings->blksize);
            
    if (test->protocol == Pudp)
    {
        dtargus = (int64_t)(test->default_settings->blksize) * SEC_TO_US * 8;
        dtargus /= test->default_settings->rate;
        
        assert(dtargus != 0);
        
        delayus = dtargus;
        adjustus = 0;
        printf("%lld adj %lld delay\n", adjustus, delayus);
        
        sp = test->streams;
        for(i=0; i< test->num_streams; i++)
        {
            sp->send_timer = new_timer(0, dtargus);
            sp= sp->next;
        }        
    }
    
    timer = new_timer(test->duration, 0);
    
    if(test->stats_interval != 0)
        stats_interval = new_timer(test->stats_interval, 0);
    
    if(test->reporter_interval != 0)
        reporter_interval = new_timer(test->reporter_interval, 0);
        
    // send data till the timer expires
    while(!timer->expired(timer))
    {
        FD_COPY(&test->write_set, &test->temp_set);
        ret = select(test->max_fd+1, NULL, &test->write_set, NULL, &tv);
        if(ret < 0)
            continue;
        
        sp= test->streams;        
        for(i=0;i<test->num_streams;i++)
        {
            if(FD_ISSET(sp->socket, &test->temp_set))
            {  
               result+= sp->snd(sp);
                
                if(sp->next==NULL)
                    break;
                sp=sp->next;
                
            }// FD_ISSET
        }
        
        //result =0 hence no data sent, hence sleep
        if(result == 0 && test->protocol == Pudp)
        {
           sp = test->streams;
           min = timer_remaining(sp->send_timer);
           while(sp)
           {
               remaining = timer_remaining(sp->send_timer);
               min = min < remaining ? min : remaining;
               sp = sp->next;
           }
           usleep (min/2);
        }
        else
            result = 0;

         
        if((test->stats_interval!= 0) && stats_interval->expired(stats_interval))
        {    
            test->stats_callback(test);            
            stats_interval = new_timer(test->stats_interval,0);
        }
        
        if((test->reporter_interval!= 0) && reporter_interval->expired(reporter_interval))
        {
            read = test->reporter_callback(test);
            puts(read); 
            reporter_interval = new_timer(test->reporter_interval,0);
        }
        
    }// while outer timer    
    
   // for last interval    
    test->stats_callback(test);
    read = test->reporter_callback(test);
    puts(read);
    
    // sending STREAM_END packets
    sp = test->streams;
    np = sp;
    do
    {
        sp = np;
        sp->settings->state = STREAM_END;
        printf("sp blksize = %d \n", sp->settings->blksize);
        sp->snd(sp);
        np = sp->next;
    } while (np);
    
    test->default_settings->state = RESULT_REQUEST;    
    read = test->reporter_callback(test);
    puts(read);
    
    // Deleting all streams - CAN CHANGE FREE_STREAM FN
    sp = test->streams;
    np = sp;
    do
    {
        sp = np;
        close(sp->socket);
        np = sp->next;
        iperf_free_stream(test, sp);
        
    } while (np);
    
    // Requesting for result from Server
    receive_result_from_server(test);
    
    if(test->stats_interval!= 0)
        free_timer(stats_interval);
    if(test->reporter_interval!= 0)
        free_timer(reporter_interval);
    free_timer(timer);
}    

int iperf_run(struct iperf_test *test)
{
    test->default_settings->state = TEST_RUNNING;
    
    if(test->role == 's')
    {
        iperf_run_server(test);
        return 0;
    }
                
            
    else if ( test->role == 'c')
    {
        iperf_run_client(test);
        return 0;
    }
        
    return -1;
}

int
main(int argc, char **argv)
{
    char ch, role;
    struct iperf_test *test;
    int port= -1;

    test = iperf_new_test();
    iperf_defaults(test);
        
    while( (ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:i:mNM:f:", longopts, NULL)) != -1 )
        switch (ch) {
            case 'c':
                test->role = 'c';
                role = test->role;
                test->server_hostname = (char *) malloc(strlen(optarg));
                strncpy(test->server_hostname, optarg, strlen(optarg));
                break;                
            case 'p':
                test->server_port = atoi(optarg);
                port = test->server_port;
                
                break;
            case 's':
                test->role = 's';
                role = test->role;
                break;
            case 't':
                test->duration = atoi(optarg);
                break;
            case 'u':
                test->protocol = Pudp;
                test->default_settings->blksize = DEFAULT_UDP_BLKSIZE;
                break;
            case 'P':
                test->num_streams = atoi(optarg);
                break;
            case 'b':
                test->default_settings->rate = unit_atof(optarg);
                break;
            case 'l':
                test->default_settings->blksize = unit_atoi(optarg);
                printf("%d is the blksize\n", test->default_settings->blksize);
                break;
            case 'w':
                test->default_settings->socket_bufsize = unit_atof(optarg);
            case 'i':
                test->stats_interval = atoi(optarg);
                test->reporter_interval = atoi(optarg);
                break;
            case 'm':
                test->print_mss = 1;
                break;
            case 'N':
                test->no_delay = 1;
                break;
            case 'M':
                test->default_settings->mss = atoi(optarg);
                break;
            case 'f':
                test->default_settings->unit_format = *optarg;               
                break;      
        }
    
    test->stats_callback = iperf_stats_callback;    
    test->reporter_callback = iperf_reporter_callback;

    switch(test->protocol)
    {
        case Ptcp:
           test->new_stream = iperf_new_tcp_stream;
            break;
            
        case Pudp:            
            test->new_stream = iperf_new_udp_stream;
            break;
    }
    
    // param exchange
    if(test->role == 'c')
        exchange_parameters(test);
    
    // actual test begins
    iperf_init_test(test);    
    fflush(stdout);
    iperf_run(test);
    iperf_free_test(test);
    
    if(role == 's')
    {
        while(1)
        {
            //start a new test 
            struct iperf_test *test;
            test = iperf_new_test();        
            iperf_defaults(test);
            
            test->stats_callback = iperf_stats_callback;    
            test->reporter_callback = iperf_reporter_callback;
            
            switch(test->protocol)
            {
                case Ptcp:
                    test->new_stream = iperf_new_tcp_stream;
                    break;                    
                case Pudp:            
                    test->new_stream = iperf_new_udp_stream;
                    break;
            }
            
            if(port != -1)
                test->server_port = port;
            
            iperf_init_test(test);
            iperf_run(test);
            iperf_free_test(test);
        }
    }
        
    return 0;
}
