int netdial(int proto, char *client, int port);
int netannounce(int proto, char *local, int port);
int Nwrite(int fd, char *buf, int count, int prot);
int mread(int fd, char *bufp, int n);
int Nread(int fd, char *buf, int count, int prot);


