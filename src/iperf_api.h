typedef uint64_t iperf_size_t;

struct iperf_interval_results
{
	iperf_size_t bytes_transferred;
	int  interval_duration;
	void * custom_data;
};

struct iperf_stream_result
{
        iperf_size_t  bytes_received;
        iperf_size_t  bytes_sent;
        int  duration;
        struct iperf_interval_results *interval_results;
        void *custom_data;
};

struct iperf_sock_opts
{
	int ttl;
	int tos;
};

struct iperf_udp_settings
{
	int  rate;
	int  packet_size;				// size of a packet/ MSS
};

struct iperf_tcp_settings
{
	int  window_size;				// TCP window size
};

struct iperf_settings
{
	struct iperf_sock_opts *options;		// structure to set socket options
        struct *proto;                                  // protocol specific settings
}

struct iperf_stream
{
        /* configurable members */
	int local_port;					// local port
	int remote_port;				// remote machine port
        void *settings;	                        	// pointer to structure settings  
	int protocol;					// protocol- TCP/UDP	

        /* non configurable members */
	struct iperf_stream_result *result;		//structure pointer to result

        int socket;                                     // socket 

        struct sockaddr_storage local_addr;
        struct sockaddr_storage remote_addr;

        int *(*init)(struct iperf_stream *stream);
        int *(*recv)(struct iperf_stream *stream);
        int *(*send)(struct iperf_stream *stream);
        int *(*update_stats)(struct iperf_stream *stream);

        struct iperf_stream *next;
};

struct iperf_test
{
	char role;						// 'c'lient or 's'erver
	struct sockaddr_storage *remote_ip_addr;
	struct sockaddr_storage *local_ip_addr;	
	int  duration;						// total duration of test

	int  stats_interval;					// time interval to gather stats
	void *(*stats_callback)(struct iperf_test *);		// callback function pointer for stats

        int  reporter_interval;				        // time interval for reporter
	void *(*reporter_callback)(struct iperf_test *);	// call back function pointer for reporter
        int reporter_fd;			                // file descriptor for reporter

        /* internal state */
        
	int  num_streams;					// total streams in the test
	struct iperf_stream *streams;				// pointer to list of struct stream
};

/**
 * iperf_create_test -- return a new iperf_test with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_test *iperf_create_test();

/**
 * iperf_init_test -- perform pretest initialization (listen on sockets, etc)
 *
 */
void iperf_init_test(struct iperf_test *test);

/**
 * iperf_destroy_test -- free resources used by test, calls iperf_destroy_stream to
 * free streams
 *
 */
void iperf_destroy_test(struct iperf_test *test);


/**
 * iperf_create_stream -- return a net iperf_test with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_stream *iperf_create_stream();

struct iperf_stream *iperf_create_tcp_stream(int window, struct iperf_sock_opts *sockopt);
struct iperf_stream *iperf_create_udp_stream(int rate, int size, struct iperf_sock_opts *sockopt);

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
void iperf_init_stream(struct iperf_stream *stream);

/**
 * iperf_destroy_stream -- free resources associated with test
 *
 */
void iperf_destroy_stream(struct iperf_stream *stream);


/**
 * iperf_run -- run iperf
 *
 * returns an exit status
 */
int iperf_run(struct iperf_test *test);
