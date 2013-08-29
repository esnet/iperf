#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sysexits.h>
#include <stdint.h>

#include <iperf_api.h>

int
main( int argc, char** argv )
{
    char* argv0;
    char* host;
    int port;
    struct iperf_test *test;

    argv0 = strrchr( argv[0], '/' );
    if ( argv0 != (char*) 0 )
	++argv0;
    else
	argv0 = argv[0];

    if ( argc != 3 ) {
	fprintf( stderr, "usage: %s [host] [port]\n", argv0 );
	exit( EXIT_FAILURE );
    }
    host = argv[1];
    port = atoi( argv[2] );

    test = iperf_new_test();
    if ( test == NULL ) {
	fprintf( stderr, "%s: failed to create test\n", argv0 );
	exit( EXIT_FAILURE );
    }
    iperf_defaults( test );
    iperf_set_verbose( test, 1 );

    /* This main program doesn't use SIGALRM, so the iperf API may use it. */
    iperf_set_test_may_use_sigalrm(test, 1);

    iperf_set_test_role( test, 'c' );
    iperf_set_test_server_hostname( test, host );
    iperf_set_test_server_port( test, port );
    /* iperf_set_test_reverse( test, 1 ); */
    iperf_set_test_omit( test, 3 );
    iperf_set_test_duration( test, 5 );
    iperf_set_test_reporter_interval( test, 1 );
    iperf_set_test_stats_interval( test, 1 );

    if ( iperf_run_client( test ) < 0 ) {
	fprintf( stderr, "%s: error - %s\n", argv0, iperf_strerror( i_errno ) );
	exit( EXIT_FAILURE );
    }

    iperf_free_test( test );
    exit( EXIT_SUCCESS );
}
