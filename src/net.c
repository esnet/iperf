#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#include "net.h"
#include "timer.h"

/* make connection to server */
int
netdial(int proto, char *client, int port)
{
    int       s;
    struct hostent *hent;
    struct sockaddr_in sa;
    socklen_t sn;

    /* XXX: This is not working for non-fully qualified host names 
	use getaddrinfo() instead?
    */
    if ((hent = gethostbyname(client)) == 0)
    {
	perror("gethostbyname");
	return (-1);
    }
    s = socket(AF_INET, proto, 0);
    if (s < 0)
    {
	perror("socket");
	return (-1);
    }
    memset(&sa, 0, sizeof sa);
    memmove(&sa.sin_addr, hent->h_addr, 4);
    sa.sin_port = htons(port);
    sa.sin_family = AF_INET;

    if (connect(s, (struct sockaddr *) & sa, sizeof sa) < 0 && errno != EINPROGRESS)
    {
	perror("connect");
	return (-1);
    }
    sn = sizeof sa;
    if (getpeername(s, (struct sockaddr *) & sa, &sn) >= 0)
    {
	return (s);
    }
    perror("connect");
    return (-1);
}

int
netannounce(int proto, char *local, int port)
{
    int       s;
    struct sockaddr_in sa;
    /* XXX: implement binding to a local address rather than * */

    memset((void *) &sa, 0, sizeof sa);

    s = socket(AF_INET, proto, 0);
    if (s < 0)
    {
	perror("socket");
	return (-1);
    }
    int       opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    sa.sin_port = htons(port);
    sa.sin_family = AF_INET;

    if (bind(s, (struct sockaddr *) & sa, sizeof(struct sockaddr_in)) < 0)
    {
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
Nread(int fd, char *buf, int count, int udp)
{
    struct sockaddr from;
    socklen_t len = sizeof(from);
    register int cnt;
    if (udp)
    {
	cnt = recvfrom(fd, buf, count, 0, &from, &len);
    } else
    {
	cnt = mread(fd, buf, count);
    }
    return (cnt);
}


/*
 *                      N W R I T E
 */
int
Nwrite(int fd, char *buf, int count, int udp, struct sockaddr dest)
{
    register int cnt;
    if (udp)
    {
again:
	cnt = sendto(fd, buf, count, 0, &dest, (socklen_t) sizeof(dest));
	if (cnt < 0 && errno == ENOBUFS)
	{
	    delay(18000);	/* XXX: Fixme! */
	    errno = 0;
	    goto again;
	}
    } else
    {
	//printf("Nwrite: writing %d bytes to socket %d \n", count, fd);
	cnt = write(fd, buf, count);
    }
    //printf("Nwrite: wrote %d bytes \n", cnt);
    return (cnt);
}


/*
 *  mread: keep reading until have expected read size
 */
int
mread(int fd, char *bufp, int n)
{
    register unsigned count = 0;
    register int nread;

    do
    {
	nread = read(fd, bufp, n - count);
	if (nread < 0)		/* if get back -1, just keep trying */
	{
	    continue;
	} else
	{
	    //printf("mread: got %d bytes \n", nread);
	    if (nread == 0)
		return ((int) count);
	    count += (unsigned) nread;
	    bufp += nread;
	}
    } while (count < n);

    return ((int) count);
}
