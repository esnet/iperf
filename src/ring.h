#ifndef _RING_H_
#define _RING_H_

#include <sys/types.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdatomic.h>
#include <semaphore.h>
#include "queue.h"
#include "iperf.h"

#define BUF_SIZE 9001 /* Maximum size for a buffer */
#define RING_SIZE 10000

typedef struct Packet_Entry {
	size_t size;
	size_t cap;
	uint8_t buf[BUF_SIZE];
} PacketEntry;

typedef struct Packet_Ring {
	atomic_int rd_index;
	atomic_int wt_index;
	sem_t doorbell;
	size_t ring_size;
	PacketEntry entries[RING_SIZE];
} PacketRing;

struct iperf_test;

typedef int (*packet_ring_func) (struct iperf_test *test, uint8_t* payload, int len);


void init_packet_ring();
void enqueue_packet_ring(void* data, size_t size);
void dequeue_packet_ring(struct iperf_test *, packet_ring_func func);

#endif
