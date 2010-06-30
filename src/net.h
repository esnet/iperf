#ifndef __NET_H
#define __NET_H

int netdial(int, char *, int);
int netannounce(int, char *, int);
int Nwrite(int, void *, int, int);
int Nread(int, void *, int, int);
int getsock_tcp_mss(int);
int set_tcp_options(int, int, int);
int setnonblocking(int);

#endif

