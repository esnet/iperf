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
    int port;
    struct iperf_test *test;
    int consecutive_errors;

    argv0 = strrchr( argv[0], '/' );
    if ( argv0 != (char*) 0 )
	++argv0;
    else
	argv0 = argv[0];

    if ( argc != 2 ) {
	fprintf( stderr, "usage: %s [port]\n", argv0 );
	exit( EXIT_FAILURE );
    }
    port = atoi( argv[1] );

    test = iperf_new_test();
    if ( test == NULL ) {
	fprintf( stderr, "%s: failed to create test\n", argv0 );
	exit( EXIT_FAILURE );
    }
    iperf_defaults( test );
    iperf_set_verbose( test, 1 );
    iperf_set_test_role( test, 's' );
    iperf_set_test_server_port( test, port );

    consecutive_errors = 0;
    for (;;) {
	if ( iperf_run_server( test ) < 0 ) {
	    fprintf( stderr, "%s: error - %s\n\n", argv0, iperf_strerror( i_errno ) );
	    ++consecutive_errors;
	    if (consecutive_errors >= 5) {
	        fprintf(stderr, "%s: too many errors, exiting\n", argv0);
		break;
	    }
	} else
	    consecutive_errors = 0;
	iperf_reset_test( test );
    }

    iperf_free_test( test );
    exit( EXIT_SUCCESS );
}
