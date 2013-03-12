IPERFDIR =	..

CC =		gcc

CFLAGS =	-I$(IPERFDIR)/src
LDFLAGS =	-L$(IPERFDIR)/src
LIBS =		-liperf

all:		mic mis

mic:		mic.c $(IPERFDIR)/src/iperf_api.h $(IPERFDIR)/src/libiperf.a
	$(CC) $(CFLAGS) mic.c $(LDFLAGS) $(LIBS) -o mic

mis:		mis.c $(IPERFDIR)/src/iperf_api.h $(IPERFDIR)/src/libiperf.a
	$(CC) $(CFLAGS) mis.c $(LDFLAGS) $(LIBS) -o mis

clean:
	-rm -f mic mis *.o *.a a.out core core.* *.core
