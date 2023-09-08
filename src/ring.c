#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "ring.h"
#include "iperf.h"
#include "iperf_api.h"

static PacketRing packetring;

static bool is_ring_empty()
{
    int cur_rd_index = atomic_load(&packetring.rd_index);
    int next_rd_index = (cur_rd_index + 1) % packetring.ring_size;

    if (next_rd_index == atomic_load(&packetring.wt_index)) {
        return true;
    }

    return false;
}

void init_packet_ring()
{
    packetring.ring_size = RING_SIZE;
    atomic_init(&packetring.rd_index, 0);
    atomic_init(&packetring.wt_index, 0);
    sem_init(&packetring.doorbell, 0, 0);

    for (int i = 1; i < RING_SIZE; i++) {
        packetring.entries[i].cap = BUF_SIZE;
        packetring.entries[i].size = 0;
    }
}

void enqueue_packet_ring(void* data, size_t size)
{
    int remain = size;
    int offset = 0;
    int cur_wt_index;
    int next_wt_index;
    PacketEntry* wt_entry;

    // If the ring was empty, ring the doorbell.
    if (is_ring_empty()) {
        sem_post(&packetring.doorbell);
    }

    while (remain > 0) {
        do {
            cur_wt_index = atomic_load(&packetring.wt_index);
            next_wt_index = (cur_wt_index + 1) % packetring.ring_size;
            if (next_wt_index == atomic_load(&packetring.rd_index))
                // Ring is full, can't keep on processing, throw away the data
                return;
            } while (!atomic_compare_exchange_weak(&packetring.wt_index, &cur_wt_index, next_wt_index));

            wt_entry = &packetring.entries[cur_wt_index];
            wt_entry->size = remain > wt_entry->cap ? wt_entry->cap : remain;
            memcpy(wt_entry->buf, data + offset, wt_entry->size);

            remain -= wt_entry->size;
            offset += wt_entry->size;
    }
}

void dequeue_packet_ring(struct iperf_test* test, packet_ring_func func)
{
    int is_corrupted;
    int cur_rd_index;
    int next_rd_index;
    PacketEntry* rd_entry;

    do {
        cur_rd_index = atomic_load(&packetring.rd_index);
        next_rd_index = (cur_rd_index + 1) % packetring.ring_size;
        if (next_rd_index == atomic_load(&packetring.wt_index)) {
            // Ring is empty, wait for doorbell
            sem_wait(&packetring.doorbell);	
        }
    } while (!atomic_compare_exchange_weak(&packetring.rd_index, &cur_rd_index, next_rd_index));

    rd_entry = &packetring.entries[cur_rd_index];

    is_corrupted = func(test, rd_entry->buf, rd_entry->size);
    if (is_corrupted) {
        test->corrupt_packets++;
        iperf_report_corrupt_stat(test, test->corrupt_packets);
        iperf_dump_payload(test, rd_entry->buf, rd_entry->size);
    }
}
