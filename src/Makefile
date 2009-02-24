CFLAGS=-g -Wall
OBJS=main.o timer.o net.o tcp_window_size.o

all: iperf 
	
iperf: $(OBJS)
	$(CC) -pthread -o iperf $(OBJS)
	#$(CC) -pg -pthread -o iperf-profile $(OBJS)

test: t_timer
	./t_timer

t_timer: timer.o t_timer.o
	$(CC) -o t_timer timer.o t_timer.o

clean:
	rm -f *.o iperf iperf-profile t_timer
