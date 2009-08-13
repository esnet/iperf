typedef uint64_t iperf_size_t;

struct iperf_interval_results
{
    iperf_size_t bytes_transferred;
    struct timeval  interval_time;
    float interval_duration;
    struct iperf_interval_results *next;    
    void * custom_data;
};

struct iperf_stream_result
{
    iperf_size_t  bytes_received;
    iperf_size_t  bytes_sent;
    struct timeval start_time;
    struct timeval end_time;
    struct iperf_interval_results *interval_results;    
    void *data;
};

struct iperf_settings
{
    int socket_bufsize;       // -w buffer size for setsockopt(), window size for TCP
    int socket_snd_bufsize;   // overrides bufsize in the send direction
    int socket_rcv_bufsize;   // overrides bufsize in the receive direction

    int blksize;              // -l size of each read/write, in UDP this relates directly to packet_size

    uint64_t rate;                 // target data rate, UDP only
    int mss;                  //for TCP MSS
    int ttl;
    int tos;
    iperf_size_t bytes;                 // -n option
    char  unit_format;                        // -f
    int state;              // This is state of a stream/test
    char cookie[37];
};

struct iperf_stream
{   
    /* configurable members */
    int local_port;                     // local port
    int remote_port;                    // remote machine port
    struct iperf_settings *settings;   // pointer to structure settings  
    int protocol;                       // protocol- TCP/UDP 
        
    /* non configurable members */
    struct iperf_stream_result *result; //structure pointer to result    
    int socket;                         // socket    
    struct timer *send_timer;
    
    char *buffer;
    
    /* for udp measurements - This can be a structure outside stream,
     and stream can have a pointer to this  */
    int packet_count;
    int stream_id;                      // stream identity
    double jitter;
    double prev_transit;
    int outoforder_packets;
    int cnt_error;
    uint64_t target;
    
    struct sockaddr_storage local_addr;
    struct sockaddr_storage remote_addr;
    
    int (*rcv)(struct iperf_stream *stream);
    int (*snd)(struct iperf_stream *stream);
    int (*update_stats)(struct iperf_stream *stream);
    
    struct iperf_stream *next;

    void *data;
};

struct iperf_test
{
    char role;                            // 'c'lient or 's'erver    -s / -c
    int protocol;
                                
    char *server_hostname;                // arg of -c 
    int server_port;                      // arg of -p
    int  duration;                        // total duration of test  -t    
    int listener_sock_tcp;
    int listener_sock_udp;    
    
    /*boolen variables for Options */
    int   daemon;                        // -D
    int   no_delay;                       // -N
    int   print_mss;                      // -m
    int   domain;                        // -V    
   
    /* Select related parameters */    
    int max_fd;
    fd_set read_set;
    fd_set temp_set;
    fd_set write_set;

    int (*accept)(struct iperf_test *);
    struct iperf_stream *(*new_stream)(struct iperf_test *);
    
    int  stats_interval;                             // time interval to gather stats -i
    void *(*stats_callback)(struct iperf_test *);    // callback function pointer for stats
    
    int  reporter_interval;                          // time interval for reporter
    char *(*reporter_callback)(struct iperf_test *); // callback function pointer for reporter
    int reporter_fd;                                 // file descriptor for reporter

    int  num_streams;                                // total streams in the test -P 
    struct iperf_stream *streams;                    // pointer to list of struct stream
    struct iperf_settings *default_settings;
};

struct udp_datagram
{   
    int state;
    int stream_id;
    int packet_count;
    struct timeval sent_time;
};

struct param_exchange
{
    int state;
    int stream_id;
    int blksize;
    int recv_window;
    int send_window;
    int mss;
    char format;
    char cookie[37];
};

enum {
    Ptcp = SOCK_STREAM,
    Pudp = SOCK_DGRAM,    
    uS_TO_NS = 1000,
    RATE = 1000000,
    MAX_BUFFER_SIZE =10,
    DEFAULT_UDP_BLKSIZE = 1470,
    DEFAULT_TCP_BLKSIZE = 8192,
    TEST_START = 1,
    TEST_RUNNING = 2,
    RESULT_REQUEST = 3,
    RESULT_RESPOND = 4,
    TEST_END = 5,
    STREAM_BEGIN = 6,
    STREAM_RUNNING = 7,
    STREAM_END = 8,
    ALL_STREAMS_END = 9,
    PARAM_EXCHANGE = 10,
    ACCESS_DENIED = -1,
    SEC_TO_US = 1000000,
};

#define SEC_TO_NS 1000000000 /* too big for enum on some platforms */
jmp_buf env;

/**
 * exchange_parameters - handles the param_Exchange part for client 
 *  
 */
void exchange_parameters(struct iperf_test *test);

/**
 * param_received - handles the param_Exchange part for server 
 * returns state on success, -1 on failure  
 *
 */
int param_received(struct iperf_stream *sp, struct param_exchange *param);


/**
 * add_interval_list -- adds new interval to the interval_list
 *  
 */
void add_interval_list(struct iperf_stream_result *rp, struct iperf_interval_results temp);

/**
 * Display -- Displays interval results for test
 * Mainly for DEBUG purpose
 *  
 */
void display_interval_list(struct iperf_stream_result *rp);

/**
 * send_result_to_client - sends result to client via 
 *  a new TCP connection
 */
void send_result_to_client(struct iperf_stream *sp);

/**
 * receive_result_from_server - Receives result from server via 
 *  a new TCP connection
 */
void receive_result_from_server(struct iperf_test *test);

/**
 * getsock_tcp_mss - Returns the MSS size for TCP
 *  
 */
int getsock_tcp_mss( int inSock );


/**
 * set_socket_options - used for setsockopt()
 * 
 *  
 */
int set_socket_options(struct iperf_stream *sp, struct iperf_test *test);

/**
 * connect_msg -- displays connection message
 * denoting senfer/receiver details
 *  
 */
void connect_msg(struct iperf_stream *sp);



/**
 * Display -- Displays streams in a test
 * Mainly for DEBUG purpose
 *  
 */
void Display(struct iperf_test *test);


/**
 * iperf_tcp_accept -- accepts a new TCP connection
 * on tcp_listener_socket for TCP data and param/result
 * exchange messages
 *returns 0 on success
 *
 */
int iperf_tcp_accept(struct iperf_test *test);

/**
 * iperf_udp_accept -- accepts a new UDP connection
 * on udp_listener_socket
 *returns 0 on success
 *
 */
int iperf_udp_accept(struct iperf_test *test);


/**
 * iperf_tcp_recv -- receives the data for TCP 
 * and the Param/result message exchange
 *returns state of packet received
 *
 */
int iperf_tcp_recv(struct iperf_stream *sp);

/**
 * iperf_udp_recv -- receives the client data for UDP 
 * 
 *returns state of packet received
 *
 */
int iperf_udp_recv(struct iperf_stream *sp);

/**
 * iperf_tcp_send -- sends the client data for TCP 
 * and the  Param/result message exchanges
 *returns bytes sent
 *
 */
int iperf_tcp_send(struct iperf_stream *sp);

/**
 * iperf_udp_send -- sends the client data for UDP
 *
 *returns bytes sent
 *
 */
int iperf_udp_send(struct iperf_stream *sp);

/**
 * iperf_stats_callback -- handles the statistic gathering
 *
 *returns void *
 *
 */
void *iperf_stats_callback(struct iperf_test *test);


/**
 * iperf_reporter_callback -- handles the report printing
 *
 *returns report
 *
 */
char *iperf_reporter_callback(struct iperf_test *test);


/**
 * find_stream_by_socket -- finds the stream based on socket
 *
 *returns stream
 *
 */
struct iperf_stream * find_stream_by_socket(struct iperf_test *test, int sock);

/**
 * iperf_run_server -- Runs the server portion of a test
 *
 */
void iperf_run_server(struct iperf_test *test);

/**
 * iperf_run_client -- Runs the client portion of a test
 *
 */
void iperf_run_client(struct iperf_test *test);

/**
 * iperf_run -- runs the test either as client or server
 *
 * returns status 
 *
 */
int iperf_run(struct iperf_test *test);

/**
 * iperf_new_test -- return a new iperf_test with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_test *iperf_new_test();

void iperf_defaults(struct iperf_test *testp);

/**
 * iperf_init_test -- perform pretest initialization (listen on sockets, etc)
 *
 */
void iperf_init_test(struct iperf_test *testp);

/**
 * iperf_free_test -- free resources used by test, calls iperf_free_stream to
 * free streams
 *
 */
void iperf_free_test(struct iperf_test *testp);


/**
 * iperf_new_stream -- return a net iperf_stream with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_stream *iperf_new_stream(struct iperf_test *testp);

struct iperf_stream *iperf_new_tcp_stream(struct iperf_test *testp);
struct iperf_stream *iperf_new_udp_stream(struct iperf_test *testp);

/**
 * iperf_add_stream -- add a stream to a test
 *
 * returns 1 on success 0 on failure
 */
int iperf_add_stream(struct iperf_test *test, struct iperf_stream *stream);

/**
 * iperf_init_stream -- init resources associated with test
 *
 */
void iperf_init_stream(struct iperf_stream *sp, struct iperf_test *testp);

/**
 * iperf_free_stream -- free resources associated with test
 *
 */
void iperf_free_stream(struct iperf_test *test, struct iperf_stream *sp);

