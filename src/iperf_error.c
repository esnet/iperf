/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "iperf.h"
#include "iperf_api.h"

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
	fprintf(stderr, "iperf3: %s\n", str);
    va_end(argp);
}

/* Do a printf to stderr, then exit. */
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
	fprintf(stderr, "iperf3: %s\n", str);
    va_end(argp);
    exit(1);
}

int i_errno;

char *
iperf_strerror(int i_errno)
{
    static char errstr[256];
    int len, perr, herr;
    perr = herr = 0;

    len = sizeof(errstr);
    memset(errstr, 0, len);

    switch (i_errno) {
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
            snprintf(errstr, len, "report interval too large (maximum = %d seconds)", MAX_INTERVAL);
            break;
        case IEMSS:
            snprintf(errstr, len, "TCP MSS too large (maximum = %d bytes)", MAX_MSS);
            break;
        case IENOSENDFILE:
            snprintf(errstr, len, "this OS does not support sendfile");
            break;
        case IENEWTEST:
            snprintf(errstr, len, "unable to create a new test");
            perr = 1;
            break;
        case IEINITTEST:
            snprintf(errstr, len, "test initialization failed");
            perr = 1;
            break;
        case IELISTEN:
            snprintf(errstr, len, "unable to start listener for connections");
            perr = 1;
            break;
        case IECONNECT:
            snprintf(errstr, len, "unable to connect to server");
            perr = 1;
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
            snprintf(errstr, len, "unable to receive cookie to server");
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
            snprintf(errstr, len, "unable to set TCP NODELAY");
            perr = 1;
            break;
        case IESETMSS:
            snprintf(errstr, len, "unable to set TCP MSS");
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
    }

    if (herr || perr)
        strncat(errstr, ": ", len);
    if (h_errno && herr) {
        strncat(errstr, hstrerror(h_errno), len);
    } else if (errno && perr) {
        strncat(errstr, strerror(errno), len);
    }

    return errstr;
}
