CFLAGS=-g -Wall
OBJS=main.o timer.o net.o tcp_window_size.o units.o

all: iperf 
	
iperf: $(OBJS)
	$(CC) -pthread -o iperf $(OBJS)
	#$(CC) -pg -pthread -o iperf-profile $(OBJS)

test: t_timer t_units
	./t_timer
	./t_units

t_timer: timer.o t_timer.o
	$(CC) -o t_timer timer.o t_timer.o

t_units: units.o t_units.o
	$(CC) -o t_units units.o t_units.o


clean:
	rm -f *.o iperf iperf-profile t_timer t_units
