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
    IECTRLWRITE = 10,       // Unable to write to the control socket (check perror)
    IECTRLREAD = 11,        // Unable to read from the control socket (check perror)
    IECTRLCLOSE = 12,       // Control socket has closed unexpectedly
    IESTREAMWRITE = 13,     // Unable to write to stream socket (check perror)
    IESTREAMREAD = 14,      // Unable to read from stream (check perror)
    IESTREAMCLOSE = 15,     // Stream has closed unexpectedly
    IENEWTEST = 16,         // Unable to create a new test (check perror)
    IECONNECT = 17,         // Unable to connect to server (check herror/perror) [from netdial]
    IESELECT = 18,          // Select failed (check perror)
    IESENDPARAMS = 19,      // Unable to send parameters to server (check perror)
    IERECVPARAMS = 20,      // Unable to receive parameters from client (check perror)
    IECREATESTREAM = 21,    // Unable to create a new stream (check herror/perror)
    IEINITSTREAM = 22,      // Unable to initialize stream (check herror/perror)
    IESETWINDOWSIZE = 23,   // Unable to set socket window size (check perror)
    IEPACKAGERESULTS = 24,  // Unable to package results (check perror)
    IESENDRESULTS = 25,     // Unable to send results to client/server (check perror)
    IERECVRESULTS = 26,     // Unable to receive results from client/server (check perror)
    IESTREAMID = 27,        // Stream has invalid ID
    IESERVERTERM = 28,      // The server has terminated
    IEACCESSDENIED = 29,    // The server is busy running a test. Try again later.
    IEMESSAGE = 30,         // Received an unknown message
    IESENDMESSAGE = 31,     // Unable to send control message to client/server (check perror)
    IERECVMESSAGE = 32,     // Unable to receive control message from client/server (check perror)
};

#endif
