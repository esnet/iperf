#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>

int
netdial(int proto, char *client, int port)
{
    int s;
    struct hostent *hent;
    struct sockaddr_in sa;
    socklen_t sn;

    if((hent = gethostbyname(client)) == 0) {
        perror("gethostbyname");
        return(-1);
    }

    s = socket(AF_INET, proto, 0);
    if(s < 0) {
        perror("socket");
        return(-1);
    }

    memset(&sa, 0, sizeof sa);
    memmove(&sa.sin_addr, hent->h_addr, 4);
    sa.sin_port = htons(port);
    sa.sin_family = AF_INET;

    if(connect(s, (struct sockaddr *) &sa, sizeof sa) < 0 && errno != EINPROGRESS) {
        perror("connect");
        return(-1);
    }

    /* XXX: is this necessary? */
    sn = sizeof sa;
    if(getpeername(s, (struct sockaddr *) &sa, &sn) >= 0) {
        return(s);
    }

    perror("connect");
    return(-1);
}

int
netannounce(int proto, char *local, int port)
{
    int s;
    struct sockaddr_in sa;
    /* XXX: implement binding to a local address rather than * */

    memset((void *) &sa, 0, sizeof sa);

    s = socket(AF_INET, proto, 0);
    if(s < 0) {
        perror("socket");
        return(-1);
    }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    sa.sin_port = htons(port);
    sa.sin_family = AF_INET;

    if(bind(s, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
        close(s);
        perror("bind");
        return(-1);
    }
   
    if(proto == SOCK_STREAM)
        listen(s, 5);

    return s;
}

