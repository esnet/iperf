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
}


void display_interval_list(struct iperf_stream_result *rp)
{
    struct iperf_interval_results *n;
    n = rp->interval_results;
    
    while(n)
    {
        printf("Interval = %d\tBytes transferred = %llu\n", n->interval_duration, n->bytes_transferred);
        n = n->next;
    }    
}

void send_result_to_client(struct iperf_stream *sp)
{    
    int result;
    int size = sp->settings->blksize;
    
    char *buf = (char *) malloc(size);
    if(!buf)
    {
        perror("malloc: unable to allocate transmit buffer");
    }    
    
    memcpy(buf, sp->data, strlen((char *)sp->data));
    
    result = send(sp->socket, buf, size , 0);
    
}

void receive_result_from_server(struct iperf_test *test)
{
    int result;
    struct iperf_stream *sp;
    int size =0;
    char *buf = NULL;
    
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
    
    printf("RESULT FROM SERVER -\n");
    puts(buf);    
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
                perror("setsockopt");
                return -1;
            }
            
            /* verify results */
            rc = getsockopt( sp->socket, IPPROTO_TCP, TCP_MAXSEG, (char*) &new_mss, &len);
              if ( new_mss != tp->default_settings->mss ) 
                perror("mismatch");           
    }        
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
        ch = buf[0];
        message = (int) ch;  
    }
    
    if(message == 3 || message == 8)
        printf(" message is %d \n", message);
    
    
   sp->result->bytes_received+= result;
    
   free(buf);
   return message;    
}

int iperf_udp_recv(struct iperf_stream *sp)
{ 
    int result, message;
    char ch;
    int size = sp->settings->blksize;
    char *buf = (char *) malloc(size);
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
        ch = buf[0];
        message = (int) ch;  
    }
    
    sp->result->bytes_received+= result;
    
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
            buf[0]= STREAM_BEGIN;
            break;
            
        case STREAM_END:
            buf[0]=  STREAM_END;           
            break;            
        case RESULT_REQUEST:
            buf[0]= RESULT_REQUEST;             
            break;
        case ALL_STREAMS_END:
            buf[0]= ALL_STREAMS_END;             
            break;
        default:
            buf[0]= 0;
            break;
    }  
    
    for(i=1; i < size; i++)
        buf[i] =  i % 37;
    
    //applicable for 1st packet sent
    if(sp->settings->state == STREAM_BEGIN)
    {     
        sp->settings->state = TEST_RUNNING;           
    }
    
    result = send(sp->socket, buf, size , 0);    
    sp->result->bytes_sent+= size;
    
    free(buf);
    return result;    
}

int iperf_udp_send(struct iperf_stream *sp)
{ 
    int result, i;
    struct timeval before, after;
     int64_t adjustus, dtargus; 
    
    
    // the || part ensures that last packet is sent to server - the STREAM_END MESSAGE
    if(((struct timer *) sp->data)->expired((struct timer *) sp->data) || sp->settings->state == STREAM_END)
    {
        int size = sp->settings->blksize;    
        char *buf = (char *) malloc(size);
        if(!buf)
        {
            perror("malloc: unable to allocate transmit buffer");
        }    
    
        dtargus = (int64_t)(sp->settings->blksize) * SEC_TO_US * 8;
        dtargus /= sp->settings->rate;
     
        assert(dtargus != 0);
    
        switch(sp->settings->state)
        {           
            case STREAM_BEGIN:
                buf[0]= STREAM_BEGIN;
                break;            
            case STREAM_END:
                buf[0]=  STREAM_END;           
                break;            
            case RESULT_REQUEST:
                buf[0]= RESULT_REQUEST;             
                break;
            case ALL_STREAMS_END:
                buf[0]= ALL_STREAMS_END;
            default:
                buf[0]= 0;
                break;
        }  
        
        for(i=1; i < size; i++)
            buf[i] =  i % 37;
        
        // applicable for 1st packet sent 
        if(sp->settings->state == STREAM_BEGIN)
        {           
            sp->settings->state = TEST_RUNNING;                        
        }
        
        
        if(gettimeofday(&before, 0) < 0)
            perror("gettimeofday");
        
        result = send(sp->socket, buf, size, 0);    
        sp->result->bytes_sent+= result;
        
        if(gettimeofday(&after, 0) < 0)
            perror("gettimeofday");
        
        adjustus = dtargus;
        adjustus += (before.tv_sec - after.tv_sec) * SEC_TO_US ;
        adjustus += (before.tv_usec - after.tv_usec);
        
      //printf(" the adjust time = %lld \n",dtargus- adjustus);        
        if( adjustus > 0) {
           dtargus = adjustus;
        }
        memcpy(&before, &after, sizeof before);
        
        // RESET THE TIMER
        sp->data = new_timer(0, dtargus);
        //printf(" new timer is %lld usec\n", dtargus);
        
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
    
    testp->unit_format = 'a';    
    
    testp->stats_interval = testp->duration;
    testp->reporter_interval = testp->duration;        
    testp->num_streams = 1;    
    testp->default_settings->socket_bufsize = 1024*1024;
    testp->default_settings->blksize = DEFAULT_TCP_BLKSIZE;
    testp->default_settings->rate = RATE;
    testp->default_settings->state = TEST_START;
}
    
void iperf_init_test(struct iperf_test *test)
{
    char ubuf[UNIT_LEN];
    struct iperf_stream *sp;
    int i, s=0;
        
    if(test->role == 's')
    {                
        test->listener_sock = netannounce(test->protocol, NULL, test->server_port);
        if( test->listener_sock < 0)
            exit(0);
                
        
        if(set_tcp_windowsize( test->listener_sock, test->default_settings->socket_bufsize, SO_RCVBUF) < 0) 
        {
            perror("unable to set window");
        }
        
        printf("-----------------------------------------------------------\n");
        printf("Server listening on %d\n", test->server_port);
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
            
            set_tcp_windowsize(sp->socket, test->default_settings->socket_bufsize, SO_SNDBUF);
            
            if(s < 0)
                exit(0);
            
            //setting noblock causes error in byte count -kprabhu
            //setnonblocking(s);
            
            sp = test->new_stream(test);
            sp->socket = s;
            iperf_init_stream(sp, test);
            iperf_add_stream(test, sp);
            connect_msg(sp);        
        }
    }                        
}

void iperf_free_test(struct iperf_test *test)
{
    
    free(test->default_settings);    
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
            
            temp.interval_duration = test->stats_interval;
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
            
            temp.interval_duration = test->stats_interval + ip->interval_duration;
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
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    struct iperf_stream *sp = test->streams;
    iperf_size_t bytes=0;
    double start_time, end_time;
    // need to reassign this
    char *message = (char *) malloc(300);
    char *message_final = (char *) malloc(test->num_streams * 50 + 200);
    
    struct iperf_interval_results *ip = test->streams->result->interval_results;
    
    if(test->default_settings->state == TEST_RUNNING) 
    {
        while(sp)
        {
            if(test->protocol == Ptcp)
            {
                while(ip->next!= NULL)
                    ip = ip->next;                
                               
                sprintf(message,report_bw_header);        
                strcat(message_final, message);

                bytes+= ip->bytes_transferred;
                unit_snprintf(ubuf, UNIT_LEN, (double) (ip->bytes_transferred), test->unit_format);
                unit_snprintf(nbuf, UNIT_LEN, (double) (ip->bytes_transferred / test->stats_interval), test->unit_format);
                
                sprintf(message, report_bw_format, sp->socket, (double)ip->interval_duration - test->stats_interval, (double)ip->interval_duration, ubuf, nbuf);
                strcat(message_final, message);               
            }
            
            // UDP
            else 
            {   
               bytes+= sp->result->interval_results->bytes_transferred;
               unit_snprintf(ubuf, UNIT_LEN, (double) ( sp->result->interval_results->bytes_transferred / test->stats_interval), test->unit_format);
               sprintf(message,"[%d]\t %llu bytes received \t %s per sec \n",sp->socket, sp->result->interval_results->bytes_transferred , ubuf);            
               strcat(message_final, message);
            }
        
            sp = sp->next;                        
        }    
       
        unit_snprintf(ubuf, UNIT_LEN, (double) ( bytes), test->unit_format);
        unit_snprintf(nbuf, UNIT_LEN, (double) ( bytes / test->stats_interval), test->unit_format);
        sprintf(message, report_sum_bw_format, (double)ip->interval_duration - test->stats_interval, (double)ip->interval_duration, ubuf, nbuf);
        strcat(message_final, message);         
       
    }
    
      
    if(test->default_settings->state == RESULT_REQUEST)    
    {   
        sp= test->streams;
        
        while(sp)
        {            
            if(test->protocol == Ptcp)
            {
                if(sp->settings->state == STREAM_END)
                {
                    if(test->role == 'c')
                        bytes+= sp->result->bytes_sent;
                    else
                        bytes+= sp->result->bytes_received;
                    
                    sprintf(message,report_bw_header);        
                    strcat(message_final, message);                    
                    
                    start_time = timeval_diff(&sp->result->start_time, &sp->result->start_time);
                    end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);
                    
                    if(test->role == 'c')
                    {
                        unit_snprintf(ubuf, UNIT_LEN, (double) (sp->result->bytes_sent), test->unit_format);
                        unit_snprintf(nbuf, UNIT_LEN, (double) (sp->result->bytes_sent / end_time), test->unit_format);
                    }
                    else
                    {
                        unit_snprintf(ubuf, UNIT_LEN, (double) (sp->result->bytes_received), test->unit_format);
                        unit_snprintf(nbuf, UNIT_LEN, (double) (sp->result->bytes_received / end_time), test->unit_format);
                    }
                        
                    sprintf(message, report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
                    strcat(message_final, message);
                }
                
            }
            else //UDP
            {
                if(sp->settings->state == STREAM_END)
                {
                    bytes+= sp->result->bytes_received;                           
                               
                    unit_snprintf(ubuf, UNIT_LEN, (double) sp->result->bytes_received /(sp->result->end_time.tv_sec - sp->result->start_time.tv_sec), test->unit_format);
                    sprintf(message,"[%d]\t %llu bytes received %s per sec\n", sp->socket, sp->result->bytes_received, ubuf);
                    strcat(message_final, message);
                }
            }            
            sp = sp->next;        
        }
    
        sp = test->streams;
        
        start_time = timeval_diff(&sp->result->start_time, &sp->result->start_time);
        end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);
        
        unit_snprintf(ubuf, UNIT_LEN, (double) bytes, test->unit_format);
        unit_snprintf(nbuf, UNIT_LEN, (double) bytes / end_time, test->unit_format);
        
        sprintf(message, report_sum_bw_format, start_time, end_time, ubuf, nbuf);
        strcat(message_final, message);
        // -m option        
        if((test->print_mss != 0)  && (test->role == 'c'))
        {
            sprintf(message,"the TCP maximum segment size mss = %d \n", getsock_tcp_mss(sp->socket));
            strcat(message_final, message);
        }
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
                
    buf = (char *) malloc(test->default_settings->blksize);    
    len = sizeof sa_peer;
    
    sz = recvfrom(test->listener_sock, buf, test->default_settings->blksize, 0, (struct sockaddr *) &sa_peer, &len);
        
    if(!sz)
        return -1;
        
    if(connect(test->listener_sock, (struct sockaddr *) &sa_peer, len) < 0)
    {
        perror("connect");
        return -1;
    }
    
    sp = test->new_stream(test);
    sp->socket = test->listener_sock;
    
    iperf_init_stream(sp, test);
    sp->result->bytes_received+= sz;
    iperf_add_stream(test, sp);    
        
    test->listener_sock = netannounce(test->protocol, NULL, test->server_port);
    if(test->listener_sock < 0) 
        return -1;    
    
    
    FD_SET(test->listener_sock, &test->read_set);
    test->max_fd = (test->max_fd < test->listener_sock) ? test->listener_sock : test->max_fd;
    
    connect_msg(sp);
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
    peersock = accept(test->listener_sock, (struct sockaddr *) &addr, &len);
    if (peersock < 0) 
    {
        printf("Error in accept(): %s\n", strerror(errno));
        return -1;
    }
    else 
    {    
        sp = test->new_stream(test);
        //setnonblocking(peersock);
        
        FD_SET(peersock, &test->read_set);
        test->max_fd = (test->max_fd < peersock) ? peersock : test->max_fd;
        
        sp->socket = peersock;        
        iperf_init_stream(sp, test);        
        iperf_add_stream(test, sp);                    
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
        free(sp);        
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
    struct iperf_stream *n;   
    int j,result, message;
    char *read = NULL;
    
    FD_ZERO(&test->read_set);
    FD_SET(test->listener_sock, &test->read_set);
    
    test->max_fd = test->listener_sock;
    test->num_streams = 0;
    test->default_settings->state = TEST_RUNNING;
        
    while(1)
    {    
        memcpy(&test->temp_set, &test->read_set,sizeof(test->temp_set));
        tv.tv_sec = 50;            
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
             // Accept a new connection
            if (FD_ISSET(test->listener_sock, &test->temp_set))
            {            
                test->accept(test);
                test->default_settings->state = TEST_RUNNING;                
                FD_CLR(test->listener_sock, &test->temp_set);                                
            }
            
            
        //Process the sockets for read operation
            for (j=0; j< test->max_fd+1; j++)
            {                
                if (FD_ISSET(j, &test->temp_set))
                {                    
                    // find the correct stream
                    n = find_stream_by_socket(test,j);
                    message = n->rcv(n);
                
                    if(message == STREAM_END)
                    {                       
                        n->settings->state = STREAM_END;
                        gettimeofday(&n->result->end_time, NULL);                                             
                        FD_CLR(j, &test->read_set);                        
                    }
                    
                    if(message == RESULT_REQUEST)
                    {
                        n->settings->state = RESULT_RESPOND;
                        n->data = read;
                        send_result_to_client(n);
                        
                        // FREE ALL STREAMS
                        n = test->streams;
                        while(n)
                        {
                            close(n->socket);            
                            iperf_free_stream(test, n);
                            n= n->next;
                        }                       
                        printf("TEST_END\n\n\n");
                                               
                        // reset TEST params
                        iperf_defaults(test);
                        test->max_fd = test->listener_sock;                              
                    }
                    
                    if( message == ALL_STREAMS_END )
                    {   
                        test->default_settings->state = RESULT_REQUEST;
                        
                        read = test->reporter_callback(test);
                        
                        printf("Reporter has been called\n");
                        printf("ALL_STREAMS_END\n"); 
                        //change UDP listening socket to TCP listening socket for 
                        //accepting the result request
                        if(test->protocol == Pudp)
                        {
                            close(test->listener_sock);
                            
                            test->protocol = Ptcp;
                            test->accept = iperf_tcp_accept;
                            iperf_defaults(test);
                            iperf_init_test(test);
                            test->max_fd = test->listener_sock;                           
                        }
                        
                        
                    }
                   
                }// end if (FD_ISSET(j, &temp_set))
            
            }// end for (j=0;...)            
                       
        }// end else (result>0)        
               
    }// end while    
    
}

void iperf_run_client(struct iperf_test *test)
{
    int i,result;
    struct iperf_stream *sp, *np;
    struct timer *timer, *stats_interval, *reporter_interval;    
    char *buf, *read= NULL;
    int64_t delayus, adjustus, dtargus; 
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
            sp->data = new_timer(0, dtargus);
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
        ret = select(test->max_fd+1, NULL, &test->write_set, NULL, &tv);
        if(ret<0)
            continue;
        
        sp= test->streams;    
        
        for(i=0;i<test->num_streams;i++)
        {
            if(FD_ISSET(sp->socket, &test->write_set))
            {  
               result = sp->snd(sp);             
                
                if(sp->next==NULL)
                    break;
                sp=sp->next;
                
            }// FD_ISSET
        }// for
        
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
    
    // sending STREAM_END packets
    sp = test->streams;
    np = sp;    
    do
    {
        sp = np;
        sp->settings->state = STREAM_END;
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
        iperf_free_stream(test, sp);
        np = sp->next;                    
    } while (np);
    
    
    // Requesting for result from Server
    receive_result_from_server(test);
                
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
    char ch;
    struct iperf_test *test;
            
    test= iperf_new_test();        
    iperf_defaults(test);
        
    while( (ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:i:mNM:f:", longopts, NULL)) != -1 )
        switch (ch) {
            case 'c':
                test->role = 'c';    
                test->server_hostname = (char *) malloc(strlen(optarg));
                strncpy(test->server_hostname, optarg, strlen(optarg));
                break;                
            case 'p':
                test->server_port = atoi(optarg);                
                break;
            case 's':
                test->role = 's';                
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
                test->default_settings->blksize = atol(optarg);
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
            case 'f':
                test->unit_format = *optarg;
                break;      
        }
    
    printf("ROLE = %s\n", (test->role == 's') ? "Server" : "Client");
    
    test->stats_callback = iperf_stats_callback;    
    test->reporter_callback = iperf_reporter_callback;

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
        
    iperf_init_test(test);
    
    iperf_run(test);
    
    return 0;
}
