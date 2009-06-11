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

struct iperf_settings
{
        int socket_bufsize;                             // -w buffer size for setsockopt(), window size for TCP
        int socket_snd_bufsize;                         // overrides bufsize in the send direction
        int socket_rcv_bufsize;                         // overrides bufsize in the receive direction

        int bufsize;                                    // -s size of each read/write, in UDP this relates directly to packet_size

        int rate;                                       // target data rate, UDP only

        int ttl;
        int tos;
};

struct iperf_stream
{
	/* configurable members */
	int local_port;					// local port
	int remote_port;				// remote machine port
	void *settings;	                // pointer to structure settings  
	int protocol;					// protocol- TCP/UDP 
		
	/* non configurable members */
	struct iperf_stream_result *result;		//structure pointer to result
	
	int socket;                                     // socket 
	
	struct sockaddr_storage local_addr;
	struct sockaddr_storage remote_addr;
	
	int *(*init)(struct iperf_stream *stream);	
	int (*rcv)(struct iperf_stream *stream);
	int *(*snd)(struct iperf_stream *stream);
	int *(*update_stats)(struct iperf_stream *stream);
	
	struct iperf_stream *next;
};

struct iperf_test
{
	char role;						// 'c'lient or 's'erver    -s / -c
	char protocol;

	struct sockaddr_storage *remote_addr;  // arg of -c 
	struct sockaddr_storage *local_addr;	
	int  duration;						// total duration of test  -t
	
	int listener_sock;
	
	int  stats_interval;					// time interval to gather stats -i
	void *(*stats_callback)(struct iperf_test *);		// callback function pointer for stats
	
	int  reporter_interval;				        // time interval for reporter
	void *(*reporter_callback)(struct iperf_test *);	// callback function pointer for reporter
	int reporter_fd;			                // file descriptor for reporter

	int  num_streams;					// total streams in the test -P 
	struct iperf_stream *streams;				// pointer to list of struct stream

        struct iperf_settings *default_settings;
	
	/*function pointers : moved here because of memeber passing problem */
};

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
struct iperf_stream *iperf_new_stream();

struct iperf_stream *iperf_new_tcp_stream(int window);
struct iperf_stream *iperf_new_udp_stream(int rate, int size);

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
 * iperf_free_stream -- free resources associated with test
 *
 */
void iperf_free_stream(struct iperf_stream *stream);


/**
 * iperf_run -- run iperf
 *
 * returns an exit status
 */
int iperf_run(struct iperf_test *test);
