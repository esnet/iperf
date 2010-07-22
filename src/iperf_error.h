/* iperf_error.h
 *
 * Iperf error handling
 */

#ifndef __IPERF_ERROR_H
#define __IPERF_ERROR_H

void iperf_error(char *);

char *iperf_strerror(int);

extern int i_errno;

enum {
    /* Parameter errors */
    IENONE = 0,             // No error
    IESERVCLIENT = 1,       // Iperf cannot be both server and client
    IENOROLE = 2,           // Iperf must either be a client (-c) or server (-s)
    IECLIENTONLY = 3,       // This option is client only
    IEDURATION = 4,         // test duration too long. Maximum value = %dMAX_TIME
    IENUMSTREAMS = 5,       // Number of parallel streams too large. Maximum value = %dMAX_STREAMS
    IEBLOCKSIZE = 6,        // Block size too large. Maximum value = %dMAX_BLOCKSIZE
    IEBUFSIZE = 7,          // Socket buffer size too large. Maximum value = %dMAX_TCP_BUFFER
    IEINTERVAL = 8,         // Report interval too large. Maxumum value = %dMAX_INTERVAL
    IEMSS = 9,              // MSS too large. Maximum value = %dMAX_MSS

    /* Test errors */
    IENEWTEST = 10,         // Unable to create a new test (check perror)
    IEINITTEST = 11,        // Test initialization failed (check perror)
    IELISTEN = 12,          // Unable to listen for connections (check perror)
    IECONNECT = 13,         // Unable to connect to server (check herror/perror) [from netdial]
    IEACCEPT = 14,          // Unable to accept connection from client (check herror/perror)
    IESENDCOOKIE = 15,      // Unable to send cookie to server (check perror)
    IERECVCOOKIE = 16,      // Unable to receive cookie from client (check perror)
    IECTRLWRITE = 17,       // Unable to write to the control socket (check perror)
    IECTRLREAD = 18,        // Unable to read from the control socket (check perror)
    IECTRLCLOSE = 19,       // Control socket has closed unexpectedly
    IEMESSAGE = 20,         // Received an unknown message
    IESENDMESSAGE = 21,     // Unable to send control message to client/server (check perror)
    IERECVMESSAGE = 22,     // Unable to receive control message from client/server (check perror)
    IESENDPARAMS = 23,      // Unable to send parameters to server (check perror)
    IERECVPARAMS = 24,      // Unable to receive parameters from client (check perror)
    IEPACKAGERESULTS = 25,  // Unable to package results (check perror)
    IESENDRESULTS = 26,     // Unable to send results to client/server (check perror)
    IERECVRESULTS = 27,     // Unable to receive results from client/server (check perror)
    IESELECT = 28,          // Select failed (check perror)
    IESERVERTERM = 29,      // The server has terminated
    IEACCESSDENIED = 30,    // The server is busy running a test. Try again later.
    IESETNODELAY = 31,      // Unable to set TCP NODELAY (check perror)
    IESETMSS = 32,          // Unable to set TCP MSS (check perror)
    IEREUSEADDR = 33,       // Unable to set reuse address on socket (check perror)
    IENONBLOCKING = 34,     // Unable to set socket to non-blocking (check perror)
    IESETWINDOWSIZE = 35,   // Unable to set socket window size (check perror)

    /* Stream errors */
    IECREATESTREAM = 36,    // Unable to create a new stream (check herror/perror)
    IEINITSTREAM = 37,      // Unable to initialize stream (check herror/perror)
    IESTREAMLISTEN = 38,    // Unable to start stream listener (check perror) 
    IESTREAMCONNECT = 39,   // Unable to connect stream (check herror/perror)
    IESTREAMACCEPT = 40,    // Unable to accepte stream connection (check perror)
    IESTREAMWRITE = 41,     // Unable to write to stream socket (check perror)
    IESTREAMREAD = 42,      // Unable to read from stream (check perror)
    IESTREAMCLOSE = 43,     // Stream has closed unexpectedly
    IESTREAMID = 44,        // Stream has invalid ID

    /* Timer errors */
    IENEWTIMER = 45,        // Unable to create new timer (check perror)
    IEUPDATETIMER = 46,     // Unable to update timer (check perror)
};

#endif
