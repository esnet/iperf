/* iperf_error.h
 *
 * Iperf error handling
 */

#ifndef __IPERF_ERROR_H
#define __IPERF_ERROR_H

void ierror(char *);

extern int i_errno;

enum {
    IESERVCLIENT = 1,       // Iperf cannot be both server and client
    IENOROLE = 2,           // Iperf must either be a client (-c) or server (-s)
    IECLIENTONLY = 3,       // This option is client only
    IEDURATION = 4,         // test duration too long. Maximum value = %dMAX_TIME
    IENUMSTREAMS = 5,       // Number of parallel streams too large. Maximum value = %dMAX_STREAMS
    IEBLOCKSIZE = 6,        // Block size too large. Maximum value = %dMAX_BLOCKSIZE
    IEBUFSIZE = 7,          // Socket buffer size too large. Maximum value = %dMAX_TCP_BUFFER
    IEINTERVAL = 8,         // Report interval too large. Maxumum value = %dMAX_INTERVAL
    IEMSS = 9,              // MSS too large. Maximum value = %dMAX_MSS
    IECTRLWRITE = 10,        // Unable to write to the control socket (check perror)
    IECTRLREAD = 11,         // Unable to read from the control socket (check perror)
    IESOCKWRITE
};

#endif
