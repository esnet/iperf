CFLAGS=-g -Wall
OBJS=main.o iperf_api.o iperf_server_api.o iperf_tcp.o iperf_udp.o timer.o net.o tcp_window_size.o units.o uuid.o tcp_info.o locale.o
LDFLAGS=

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LDFLAGS= -luuid
endif

all: iperf 
	
iperf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o iperf3 $(OBJS)

profile: iperf
	$(CC) -pg -o iperf-profile $(OBJS)

dist:
	tar cvfz iperf3.0a2.tar.gz *.c *.h *.txt Makefile AUTHORS 

test: t_timer t_units t_uuid
	./t_timer
	./t_units
	./t_uuid

t_timer: timer.o t_timer.o
	$(CC) -o t_timer timer.o t_timer.o

t_units: units.o t_units.o
	$(CC) -o t_units units.o t_units.o

t_uuid: uuid.o t_uuid.o
	$(CC) -o t_uuid uuid.o t_uuid.o

clean:
	rm -f *.o iperf iperf-profile t_timer t_units t_uuid
