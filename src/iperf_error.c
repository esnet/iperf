/*
 * iperf, Copyright (c) 2014-2019, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "iperf.h"
#include "iperf_api.h"

int gerror;

/* Do a printf to stderr. */
void
iperf_err(struct iperf_test *test, const char *format, ...)
{
    va_list argp;
    char str[1000];

    va_start(argp, format);
    vsnprintf(str, sizeof(str), format, argp);
    if (test != NULL && test->json_output && test->json_top != NULL)
	cJSON_AddStringToObject(test->json_top, "error", str);
    else
	if (test && test->outfile && test->outfile != stdout) {
	    fprintf(test->outfile, "iperf3: %s\n", str);
	}
	else {
	    fprintf(stderr, "iperf3: %s\n", str);
	}
    va_end(argp);
}

/* Do a printf to stderr or log file as appropriate, then exit. */
void
iperf_errexit(struct iperf_test *test, const char *format, ...)
{
    va_list argp;
    char str[1000];

    va_start(argp, format);
    vsnprintf(str, sizeof(str), format, argp);
    if (test != NULL && test->json_output && test->json_top != NULL) {
	cJSON_AddStringToObject(test->json_top, "error", str);
	iperf_json_finish(test);
    } else
	if (test && test->outfile && test->outfile != stdout) {
	    fprintf(test->outfile, "iperf3: %s\n", str);
	}
	else {
	    fprintf(stderr, "iperf3: %s\n", str);
	}
    va_end(argp);
    if (test)
        iperf_delete_pidfile(test);
    exit(1);
}

int i_errno;

char *
iperf_strerror(int int_errno)
{
    static char errstr[256];
    int len, perr, herr;
    perr = herr = 0;

    len = sizeof(errstr);
    memset(errstr, 0, len);

    switch (int_errno) {
        case IENONE:
            snprintf(errstr, len, "no error");
            break;
        case IESERVCLIENT:
            snprintf(errstr, len, "cannot be both server and client");
            break;
        case IENOROLE:
            snprintf(errstr, len, "must either be a client (-c) or server (-s)");
            break;
        case IESERVERONLY:
            snprintf(errstr, len, "some option you are trying to set is server only");
            break;
        case IECLIENTONLY:
            snprintf(errstr, len, "some option you are trying to set is client only");
            break;
        case IEDURATION:
            snprintf(errstr, len, "test duration too long (maximum = %d seconds)", MAX_TIME);
            break;
        case IENUMSTREAMS:
            snprintf(errstr, len, "number of parallel streams too large (maximum = %d)", MAX_STREAMS);
            break;
        case IEBLOCKSIZE:
            snprintf(errstr, len, "block size too large (maximum = %d bytes)", MAX_BLOCKSIZE);
            break;
        case IEBUFSIZE:
            snprintf(errstr, len, "socket buffer size too large (maximum = %d bytes)", MAX_TCP_BUFFER);
            break;
        case IEINTERVAL:
            snprintf(errstr, len, "invalid report interval (min = %g, max = %g seconds)", MIN_INTERVAL, MAX_INTERVAL);
            break;
    case IEBIND: /* UNUSED */
            snprintf(errstr, len, "--bind must be specified to use --cport");
            break;
        case IEUDPBLOCKSIZE:
            snprintf(errstr, len, "block size invalid (minimum = %d bytes, maximum = %d bytes)", MIN_UDP_BLOCKSIZE, MAX_UDP_BLOCKSIZE);
            break;
        case IEBADTOS:
            snprintf(errstr, len, "bad TOS value (must be between 0 and 255 inclusive)");
            break;
        case IESETCLIENTAUTH:
             snprintf(errstr, len, "you must specify username (max 20 chars), password (max 20 chars) and a path to a valid public rsa client to be used");
            break;
        case IESETSERVERAUTH:
             snprintf(errstr, len, "you must specify path to a valid private rsa server to be used and a user credential file");
            break;
	case IEBADFORMAT:
	    snprintf(errstr, len, "bad format specifier (valid formats are in the set [kmgtKMGT])");
	    break;
	case IEBADPORT:
	    snprintf(errstr, len, "port number must be between 1 and 65535 inclusive");
	    break;
        case IEMSS:
            snprintf(errstr, len, "TCP MSS too large (maximum = %d bytes)", MAX_MSS);
            break;
        case IENOSENDFILE:
            snprintf(errstr, len, "this OS does not support sendfile");
            break;
        case IEOMIT:
            snprintf(errstr, len, "bogus value for --omit");
            break;
        case IEUNIMP:
            snprintf(errstr, len, "an option you are trying to set is not implemented yet");
            break;
        case IEFILE:
            snprintf(errstr, len, "unable to open -F file");
            perr = 1;
            break;
        case IEBURST:
            snprintf(errstr, len, "invalid burst count (maximum = %d)", MAX_BURST);
            break;
        case IEENDCONDITIONS:
            snprintf(errstr, len, "only one test end condition (-t, -n, -k) may be specified");
            break;
	case IELOGFILE:
	    snprintf(errstr, len, "unable to open log file");
	    perr = 1;
	    break;
	case IENOSCTP:
	    snprintf(errstr, len, "no SCTP support available");
	    break;
        case IENEWTEST:
            snprintf(errstr, len, "unable to create a new test");
            perr = 1;
            break;
        case IEINITTEST:
            snprintf(errstr, len, "test initialization failed");
            perr = 1;
            break;
        case IEAUTHTEST:
            snprintf(errstr, len, "test authorization failed");
            break;
        case IELISTEN:
            snprintf(errstr, len, "unable to start listener for connections");
	    herr = 1;
            perr = 1;
            break;
        case IECONNECT:
            snprintf(errstr, len, "unable to connect to server");
            perr = 1;
	    herr = 1;
            break;
        case IEACCEPT:
            snprintf(errstr, len, "unable to accept connection from client");
            herr = 1;
            perr = 1;
            break;
        case IESENDCOOKIE:
            snprintf(errstr, len, "unable to send cookie to server");
            perr = 1;
            break;
        case IERECVCOOKIE:
            snprintf(errstr, len, "unable to receive cookie at server");
            perr = 1;
            break;
        case IECTRLWRITE:
            snprintf(errstr, len, "unable to write to the control socket");
            perr = 1;
            break;
        case IECTRLREAD:
            snprintf(errstr, len, "unable to read from the control socket");
            perr = 1;
            break;
        case IECTRLCLOSE:
            snprintf(errstr, len, "control socket has closed unexpectedly");
            break;
        case IEMESSAGE:
            snprintf(errstr, len, "received an unknown control message");
            break;
        case IESENDMESSAGE:
            snprintf(errstr, len, "unable to send control message");
            perr = 1;
            break;
        case IERECVMESSAGE:
            snprintf(errstr, len, "unable to receive control message");
            perr = 1;
            break;
        case IESENDPARAMS:
            snprintf(errstr, len, "unable to send parameters to server");
            perr = 1;
            break;
        case IERECVPARAMS:
            snprintf(errstr, len, "unable to receive parameters from client");
            perr = 1;
            break;
        case IEPACKAGERESULTS:
            snprintf(errstr, len, "unable to package results");
            perr = 1;
            break;
        case IESENDRESULTS:
            snprintf(errstr, len, "unable to send results");
            perr = 1;
            break;
        case IERECVRESULTS:
            snprintf(errstr, len, "unable to receive results");
            perr = 1;
            break;
        case IESELECT:
            snprintf(errstr, len, "select failed");
            perr = 1;
            break;
        case IECLIENTTERM:
            snprintf(errstr, len, "the client has terminated");
            break;
        case IESERVERTERM:
            snprintf(errstr, len, "the server has terminated");
            break;
        case IEACCESSDENIED:
            snprintf(errstr, len, "the server is busy running a test. try again later");
            break;
        case IESETNODELAY:
            snprintf(errstr, len, "unable to set TCP/SCTP NODELAY");
            perr = 1;
            break;
        case IESETMSS:
            snprintf(errstr, len, "unable to set TCP/SCTP MSS");
            perr = 1;
            break;
        case IESETBUF:
            snprintf(errstr, len, "unable to set socket buffer size");
            perr = 1;
            break;
        case IESETTOS:
            snprintf(errstr, len, "unable to set IP TOS");
            perr = 1;
            break;
        case IESETCOS:
            snprintf(errstr, len, "unable to set IPv6 traffic class");
            perr = 1;
            break;
        case IESETFLOW:
            snprintf(errstr, len, "unable to set IPv6 flow label");
            break;
        case IEREUSEADDR:
            snprintf(errstr, len, "unable to reuse address on socket");
            perr = 1;
            break;
        case IENONBLOCKING:
            snprintf(errstr, len, "unable to set socket to non-blocking");
            perr = 1;
            break;
        case IESETWINDOWSIZE:
            snprintf(errstr, len, "unable to set socket window size");
            perr = 1;
            break;
        case IEPROTOCOL:
            snprintf(errstr, len, "protocol does not exist");
            break;
        case IEAFFINITY:
            snprintf(errstr, len, "unable to set CPU affinity");
            perr = 1;
            break;
	case IEDAEMON:
	    snprintf(errstr, len, "unable to become a daemon");
	    perr = 1;
	    break;
        case IECREATESTREAM:
            snprintf(errstr, len, "unable to create a new stream");
            herr = 1;
            perr = 1;
            break;
        case IEINITSTREAM:
            snprintf(errstr, len, "unable to initialize stream");
            herr = 1;
            perr = 1;
            break;
        case IESTREAMLISTEN:
            snprintf(errstr, len, "unable to start stream listener");
	    herr = 1;
            perr = 1;
            break;
        case IESTREAMCONNECT:
            snprintf(errstr, len, "unable to connect stream");
            herr = 1;
            perr = 1;
            break;
        case IESTREAMACCEPT:
            snprintf(errstr, len, "unable to accept stream connection");
            perr = 1;
            break;
        case IESTREAMWRITE:
            snprintf(errstr, len, "unable to write to stream socket");
            perr = 1;
            break;
        case IESTREAMREAD:
            snprintf(errstr, len, "unable to read from stream socket");
            perr = 1;
            break;
        case IESTREAMCLOSE:
            snprintf(errstr, len, "stream socket has closed unexpectedly");
            break;
        case IESTREAMID:
            snprintf(errstr, len, "stream has an invalid id");
            break;
        case IENEWTIMER:
            snprintf(errstr, len, "unable to create new timer");
            perr = 1;
            break;
        case IEUPDATETIMER:
            snprintf(errstr, len, "unable to update timer");
            perr = 1;
            break;
        case IESETCONGESTION:
            snprintf(errstr, len, "unable to set TCP_CONGESTION: " 
                                  "Supplied congestion control algorithm not supported on this host");
            break;
	case IEPIDFILE:
            snprintf(errstr, len, "unable to write PID file");
            perr = 1;
            break;
	case IEV6ONLY:
	    snprintf(errstr, len, "Unable to set/reset IPV6_V6ONLY");
	    perr = 1;
	    break;
        case IESETSCTPDISABLEFRAG:
            snprintf(errstr, len, "unable to set SCTP_DISABLE_FRAGMENTS");
            perr = 1;
            break;
        case IESETSCTPNSTREAM:
            snprintf(errstr, len, "unable to set SCTP_INIT num of SCTP streams\n");
            perr = 1;
            break;
	case IESETPACING:
	    snprintf(errstr, len, "unable to set socket pacing");
	    perr = 1;
	    break;
	case IESETBUF2:
	    snprintf(errstr, len, "socket buffer size not set correctly");
	    break;
	case IEREVERSEBIDIR:
	    snprintf(errstr, len, "cannot be both reverse and bidirectional");
            break;
	
    }

    /* Append the result of strerror() or gai_strerror() if appropriate */
    if (herr || perr)
        strncat(errstr, ": ", len - strlen(errstr) - 1);
    if (errno && perr)
        strncat(errstr, strerror(errno), len - strlen(errstr) - 1);
    else if (herr && gerror) {
        strncat(errstr, gai_strerror(gerror), len - strlen(errstr) - 1);
	gerror = 0;
    }

    return errstr;
}
