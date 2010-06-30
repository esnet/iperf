#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <netdb.h>
#include <string.h>
#include <sys/fcntl.h>

#include "net.h"
#include "timer.h"

/* netdial and netannouce code comes from libtask: http://swtch.com/libtask/
 * Copyright: http://swtch.com/libtask/COPYRIGHT
*/

/* make connection to server */
int
netdial(int proto, char *client, int port)
{
    int s;
    struct hostent *hent;
    struct sockaddr_in sa;
    socklen_t sn;

    /* XXX: This is not working for non-fully qualified host names use getaddrinfo() instead? */
    if ((hent = gethostbyname(client)) == 0) {
        perror("gethostbyname");
        return (-1);
    }

    s = socket(AF_INET, proto, 0);
    if (s < 0) {
        perror("socket");
        return (-1);
    }

    memset(&sa, 0, sizeof sa);
    memmove(&sa.sin_addr, hent->h_addr, 4);
    sa.sin_port = htons(port);
    sa.sin_family = AF_INET;

    if (connect(s, (struct sockaddr *) & sa, sizeof sa) < 0 && errno != EINPROGRESS) {
        perror("netdial: connect error");
        return (-1);
    }
    
    sn = sizeof sa;
    
    if (getpeername(s, (struct sockaddr *) & sa, &sn) >= 0) {
        return (s);
    }

    perror("getpeername error");
    return (-1);
}

/***************************************************************/

int
netannounce(int proto, char *local, int port)
{
    int s;
    struct sockaddr_in sa;
    /* XXX: implement binding to a local address rather than * */

    memset((void *) &sa, 0, sizeof sa);

    s = socket(AF_INET, proto, 0);
    if (s < 0) {
        perror("socket");
        return (-1);
    }
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    sa.sin_port = htons(port);
    sa.sin_family = AF_INET;

    if (bind(s, (struct sockaddr *) & sa, sizeof(struct sockaddr_in)) < 0) {
        close(s);
        perror("bind");
        return (-1);
    }
    
    if (proto == SOCK_STREAM)
        listen(s, 5);

    return s;
}


/*******************************************************************/
/* reads 'count' byptes from a socket  */
/********************************************************************/

int
Nread(int fd, void *buf, int count, int prot)
{
    struct sockaddr from;
    socklen_t len = sizeof(from);
    register int n;
    register int nleft = count;

    if (prot == SOCK_DGRAM) {
        // XXX: Does recvfrom guarantee all count bytes are sent at once?
        fprintf(stderr, "READING UDP DATA IN Nread SOCK_DGRAM\n");
        n = recvfrom(fd, buf, count, 0, &from, &len);
    } else {
        while (nleft > 0) {
            if ((n = read(fd, buf, nleft)) < 0) {
                if (errno == EINTR)
                    n = 0;
                else
                    return (-1);
            } else if (n == 0)
                break;
            
            nleft -= n;
            buf += n;
        }
    }
    return (count - nleft);
}


/*
 *                      N W R I T E
 *
 * XXX: After updating this function to use read/write, the only difference between
 *      TCP and UDP is that udp handles ENOBUFS. Should we merge the two?
 */

int
Nwrite(int fd, void *buf, int count, int prot)
{
    register int n;
    register int nleft = count;

    if (prot == SOCK_DGRAM) { /* UDP mode */
        while (nleft > 0) {
            if ((n = write(fd, buf, nleft)) < 0) {
                if (errno == EINTR) {
                    n = 0;
                } else if (errno == ENOBUFS) {
                    /* wait if run out of buffers */
                    /* XXX: but how long to wait? Start shorter and increase delay each time?? */
                    delay(18000);   // XXX: Fixme!
                    n = 0;
                } else {
                    return (-1);
                }
            }
            nleft -= n;
            buf += n;
        }
    } else {
        while (nleft > 0) {
            if ((n = write(fd, buf, nleft)) < 0) {
                if (errno == EINTR)
                    n = 0;
                else
                    return (-1);
            }
            nleft -= n;
            buf += n;
        }
    }
    return (count);
}

/*************************************************************************/

/**
 * getsock_tcp_mss - Returns the MSS size for TCP
 *
 */

int
getsock_tcp_mss(int inSock)
{
    int             mss = 0;

    int             rc;
    socklen_t       len;

    assert(inSock >= 0); /* print error and exit if this is not true */

    /* query for mss */
    len = sizeof(mss);
    rc = getsockopt(inSock, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, &len);

    return mss;
}



/*************************************************************/

/* sets TCP_NODELAY and TCP_MAXSEG if requested */

int
set_tcp_options(int sock, int no_delay, int mss)
{

    socklen_t       len;

    if (no_delay == 1) {
        int             no_delay = 1;

        len = sizeof(no_delay);
        int             rc = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
                             (char *)&no_delay, len);

        if (rc == -1) {
            perror("TCP_NODELAY");
            return -1;
        }
    }
#ifdef TCP_MAXSEG
    if (mss > 0) {
        int             rc;
        int             new_mss;

        len = sizeof(new_mss);

        assert(sock != -1);

        /* set */
        new_mss = mss;
        len = sizeof(new_mss);
        rc = setsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char *)&new_mss, len);
        if (rc == -1) {
            perror("setsockopt");
            return -1;
        }
        /* verify results */
        rc = getsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char *)&new_mss, &len);
        if (new_mss != mss) {
            perror("setsockopt value mismatch");
            return -1;
        }
    }
#endif
    return 0;
}

/****************************************************************************/

int
setnonblocking(int sock)
{
    int       opts = 0;

    opts = (opts | O_NONBLOCK);
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

