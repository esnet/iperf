#include <stdio.h>
#include "iperf.h"
#include "iperf_error.h"

int ierrno;

void
ierror(char *estr)
{
    fprintf(stderr, "%s: ", estr);

    switch (ierrno) {
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
    } 
}
