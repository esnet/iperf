#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include "iperf.h"
#include "iperf_error.h"

int i_errno;

void
ierror(char *estr)
{
    fprintf(stderr, "%s: ", estr);

    switch (i_errno) {
        case IESERVCLIENT:
            fprintf(stderr, "iperf cannot be both server and client\n");
            break;
        case IENOROLE:
            fprintf(stderr, "iperf instance must either be a client (-c) or server (-s)\n");
            break;
        case IECLIENTONLY:
            fprintf(stderr, "some option you are trying to set is client only\n");
            break;
        case IEDURATION:
            fprintf(stderr, "test duration too long (maximum = %d seconds)\n", MAX_TIME);
            break;
        case IENUMSTREAMS:
            fprintf(stderr, "number of parallel streams too large (maximum = %d)\n", MAX_STREAMS);
            break;
        case IEBLOCKSIZE:
            fprintf(stderr, "block size too large (maximum = %d bytes)\n", MAX_BLOCKSIZE);
            break;
        case IEBUFSIZE:
            fprintf(stderr, "socket buffer size too large (maximum = %d bytes)\n", MAX_TCP_BUFFER);
            break;
        case IEINTERVAL:
            fprintf(stderr, "report interval too large (maximum = %d seconds)\n", MAX_INTERVAL);
            break;
        case IEMSS:
            fprintf(stderr, "TCP MSS too large (maximum = %d bytes)\n", MAX_MSS);
            break;
        case IECTRLWRITE:
            if (errno)
                fprintf(stderr, "unable to write to the control socket: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to write to the control socket\n");
            break;
        case IECTRLREAD:
            if (errno)
                fprintf(stderr, "unable to read from the control socket: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to read from the control socket\n");
            break;
        case IECTRLCLOSE:
            fprintf(stderr, "control socket has closed unexpectedly\n");
            break;
        case IESTREAMWRITE:
            if (errno)
                fprintf(stderr, "unable to write to stream socket: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to write to stream socket\n");
            break;
        case IESTREAMREAD:
            if (errno)
                fprintf(stderr, "unable to read from stream socket: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to read from stream socket\n");
            break;
        case IESTREAMCLOSE:
            fprintf(stderr, "stream socket has closed unexpectedly\n");
            break;
        case IENEWTEST:
            if (errno)
                fprintf(stderr, "unable to create a new test: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to create a new test\n");
            break;
        case IECONNECT:
            if (errno)
                fprintf(stderr, "unable to connect to server: %s\n", strerror(errno));
            else if (h_errno)
                fprintf(stderr, "unable to connect to server: %s\n", hstrerror(h_errno));
            else
                fprintf(stderr, "unable to connect to server\n");
            break;
        case IESELECT:
            if (errno)
                fprintf(stderr, "select failed: %s\n", strerror(errno));
            else
                fprintf(stderr, "select failed\n");
            break;
        case IESENDPARAMS:
            if (errno)
                fprintf(stderr, "unable to send parameters to server: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to send parameters to server\n");
            break;
        case IERECVPARAMS:
            if (errno)
                fprintf(stderr, "unable to receive parameters from client: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to receive parameters from client\n");
            break;
        case IECREATESTREAM:
            if (errno)
                fprintf(stderr, "unable to create a new stream: %s\n", strerror(errno));
            else if (h_errno)
                fprintf(stderr, "unable to create a new stream: %s\n", hstrerror(h_errno));
            else
                fprintf(stderr, "unable to create a new stream\n");
            break;
        case IEINITSTREAM:
            if (errno)
                fprintf(stderr, "unable to initialize stream: %s\n", strerror(errno));
            else if (h_errno)
                fprintf(stderr, "unable to initialize stream: %s\n", hstrerror(h_errno));
            else
                fprintf(stderr, "unable to initialize stream\n");
            break;
        case IESETWINDOWSIZE:
            if (errno)
                fprintf(stderr, "unable to set socket window size: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to set socket window size\n");
            break;
        case IEPACKAGERESULTS:
            if (errno)
                fprintf(stderr, "unable to package results: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to package results\n");
            break;
        case IESENDRESULTS:
            if (errno)
                fprintf(stderr, "unable to send results: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to send results\n");
            break;
        case IERECVRESULTS:
            if (errno)
                fprintf(stderr, "unable to receive results: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to receive results\n");
            break;
        case IESTREAMID:
            fprintf(stderr, "stream has an invalid id\n");
            break;
        case IESERVERTERM:
            fprintf(stderr, "the server has terminated\n");
            break;
        case IEACCESSDENIED:
            fprintf(stderr, "the server is busy running a test. try again later.\n");
            break;
        case IEMESSAGE:
            fprintf(stderr, "received an unknown control message\n");
            break;
        case IESENDMESSAGE:
            if (errno)
                fprintf(stderr, "unable to send control message: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to send control message\n");
            break;
        case IERECVMESSAGE:
            if (errno)
                fprintf(stderr, "unable to receive control message: %s\n", strerror(errno));
            else
                fprintf(stderr, "unable to receive control message\n");
            break;
    }
}
