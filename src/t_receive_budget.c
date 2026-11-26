/*
 * Synchronous checks for the private receive-admission budget.
 * Distributed under the BSD style license in the project's LICENSE file.
 */
#include "iperf_config.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_receive_budget.h"
#include "timer.h"

/* Setup and checks must still execute when this file is built with NDEBUG. */
static unsigned checks;

#ifdef __ANDROID__
#define TCP_BUDGET_SUPPORTED 0
#else
#define TCP_BUDGET_SUPPORTED 1
#endif

#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define CHECK_RESULT(operation, expected) do { \
    int actual_result = (operation); \
    ++checks; \
    if (actual_result != (expected)) { \
        fprintf(stderr, "%s:%d: %s returned %d, expected %d (errno %d)\n", \
                __FILE__, __LINE__, #operation, actual_result, \
                (expected), errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static struct iperf_receive_budget *
new_budget(uint_fast64_t bytes, uint_fast64_t blocks)
{
    struct iperf_receive_budget *budget = NULL;

    CHECK_RESULT(iperf_receive_budget_create(&budget, bytes, blocks), 0);
    CHECK(budget != NULL);
    CHECK_RESULT(iperf_receive_budget_is_complete(budget), 0);
    return budget;
}

static void
activate(struct iperf_receive_budget *budget)
{
    struct iperf_receive_budget_guard guard;

    memset(&guard, 0, sizeof(guard));
    CHECK_RESULT(iperf_receive_budget_reset_begin(budget, &guard), 0);
    /* No nested calls, I/O or cancellation points during the reset pause. */
    CHECK_RESULT(iperf_receive_budget_reset_end(budget, &guard), 0);
    CHECK_RESULT(iperf_receive_budget_is_complete(budget), 0);
}

static struct iperf_receive_budget_token
admit(struct iperf_receive_budget *budget)
{
    struct iperf_receive_budget_token token;

    CHECK_RESULT(iperf_receive_budget_admit(budget, &token), 1);
    return token;
}

static void
charge(struct iperf_receive_budget *budget, ssize_t received, int complete)
{
    struct iperf_receive_budget_token token = admit(budget);

    CHECK_RESULT(iperf_receive_budget_finish(budget, &token, received), complete);
    CHECK(token.eligible == 0);
    CHECK_RESULT(iperf_receive_budget_is_complete(budget), complete);
}

static void
deny(struct iperf_receive_budget *budget)
{
    struct iperf_receive_budget_token token;

    CHECK_RESULT(iperf_receive_budget_admit(budget, &token), 0);
    CHECK(token.eligible == 0);
    CHECK_RESULT(iperf_receive_budget_is_complete(budget), 1);
}

static void
test_publication_and_reset(void)
{
    struct iperf_receive_budget *budget = new_budget(8, 0);
    struct iperf_receive_budget_token warmup;
    struct iperf_receive_budget_token old_active;
    uint_fast64_t first_epoch;

    charge(budget, 64, 0);
    warmup = admit(budget);
    CHECK(warmup.eligible == 0);
    activate(budget);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &warmup, 64), 0);
    charge(budget, 3, 0);
    old_active = admit(budget);
    CHECK(old_active.eligible != 0);
    first_epoch = old_active.epoch;

    activate(budget);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &old_active, 64), 0);
    CHECK(old_active.eligible == 0);
    old_active = admit(budget);
    CHECK(old_active.epoch != first_epoch);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &old_active, 7), 0);
    charge(budget, 1, 1);
    deny(budget);

    /* A completed context can be reset only through the explicit transition. */
    activate(budget);
    charge(budget, 8, 1);
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);
}

static void
test_finish_during_reset_pause(void)
{
    struct iperf_receive_budget *budget = new_budget(4, 0);
    struct iperf_receive_budget_guard guard;
    struct iperf_receive_budget_token old;

    activate(budget);
    old = admit(budget);
    memset(&guard, 0, sizeof(guard));
    CHECK_RESULT(iperf_receive_budget_reset_begin(budget, &guard), 0);
    /* Finishing an admitted callback is allowed while new admissions pause. */
    CHECK_RESULT(iperf_receive_budget_finish(budget, &old, 8), 0);
    CHECK(old.eligible == 0);
    CHECK_RESULT(iperf_receive_budget_reset_end(budget, &guard), 0);
    charge(budget, 3, 0);
    charge(budget, 1, 1);
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);
}

static void
test_quantity_units(void)
{
    struct iperf_receive_budget *budget = new_budget(8, 0);

    activate(budget);
    charge(budget, 0, 0);
    charge(budget, -1, 0);
    charge(budget, 3, 0);
    charge(budget, 4, 0);
    charge(budget, 1, 1);
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);

    budget = new_budget(0, 3);
    activate(budget);
    charge(budget, 0, 0);
    charge(budget, -1, 0);
    charge(budget, 1, 0);
    charge(budget, 999, 0);
    charge(budget, 2, 1);
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);

    budget = new_budget(0, 0);
    activate(budget);
    charge(budget, 4096, 0);
    charge(budget, 1, 0);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);
}

static void
test_either_limit(void)
{
    struct iperf_receive_budget *budget = new_budget(5, 2);

    activate(budget);
    charge(budget, 5, 1);
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);

    budget = new_budget(5, 2);
    activate(budget);
    charge(budget, 1, 0);
    charge(budget, 1, 1);
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);
}

static void
test_inflight_allowance(void)
{
    struct iperf_receive_budget *budget = new_budget(5, 0);
    struct iperf_receive_budget_token tokens[3];
    unsigned i;
    unsigned bytes = 0;
    unsigned calls = 0;

    activate(budget);
    for (i = 0; i < 3; ++i)
        tokens[i] = admit(budget);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &tokens[0], 4), 0);
    bytes += 4;
    tokens[0] = admit(budget);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &tokens[1], 4), 1);
    bytes += 4;
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &tokens[2], 4), 1);
    bytes += 4;
    CHECK_RESULT(iperf_receive_budget_finish(budget, &tokens[0], 4), 1);
    bytes += 4;
    CHECK(bytes == 5 + 3 * 4 - 1);
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);

    budget = new_budget(0, 2);
    activate(budget);
    for (i = 0; i < 3; ++i)
        tokens[i] = admit(budget);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &tokens[0], 1), 0);
    ++calls;
    tokens[0] = admit(budget);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &tokens[1], 7), 1);
    ++calls;
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &tokens[2], 2), 1);
    ++calls;
    CHECK_RESULT(iperf_receive_budget_finish(budget, &tokens[0], 3), 1);
    ++calls;
    CHECK(calls == 2 + 3 - 1);
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);
}

static void
test_large_budget(void)
{
    struct iperf_receive_budget *budget = new_budget(UINT_FAST64_MAX, 0);
    struct iperf_receive_budget_token late;

    activate(budget);
    late = admit(budget);
    if ((uintmax_t) SSIZE_MAX >= UINT_FAST64_MAX / 2) {
        ssize_t half = (ssize_t) (UINT_FAST64_MAX / 2);

        charge(budget, half, 0);
        charge(budget, half, 0);
        /* Crossing UINT_FAST64_MAX in total received must not wrap a sum. */
        charge(budget, 2, 1);
        CHECK_RESULT(iperf_receive_budget_finish(budget, &late, 1), 1);
        deny(budget);
    } else {
        /* Do not require billions of calls on a platform with narrow ssize_t. */
        charge(budget, SSIZE_MAX, 0);
        CHECK_RESULT(iperf_receive_budget_finish(budget, &late, 1), 0);
        puts("UINT_FAST64_MAX crossing not exercised: ssize_t is narrower");
    }
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);

    /* Saturation and late completion are checked on every supported width. */
    budget = new_budget(3, 0);
    activate(budget);
    late = admit(budget);
    charge(budget, 4, 1);
    CHECK_RESULT(iperf_receive_budget_finish(budget, &late, 1), 1);
    deny(budget);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);
}

static void
test_references_and_fresh_contexts(void)
{
    struct iperf_receive_budget *budget = new_budget(4, 0);
    struct iperf_receive_budget *other;

    /* Reserve a worker reference, then release the controller's reference. */
    CHECK_RESULT(iperf_receive_budget_retain(budget), 0);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);
    activate(budget);
    charge(budget, 4, 1);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);

    budget = new_budget(4, 0);
    other = new_budget(6, 0);
    /* A reservation returned before worker creation leaves the owner intact. */
    CHECK_RESULT(iperf_receive_budget_retain(budget), 0);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);
    charge(budget, 4, 0);
    activate(budget);
    activate(other);
    charge(budget, 4, 1);
    charge(other, 2, 0);
    CHECK_RESULT(iperf_receive_budget_release(budget), 0);
    charge(other, 4, 1);
    CHECK_RESULT(iperf_receive_budget_release(other), 0);
}

struct receive_script {
    int result;
    int error;
    unsigned calls;
};

static int
scripted_receive(struct iperf_stream *stream)
{
    struct receive_script *script = stream->data;

    ++script->calls;
    errno = script->error;
    return script->result;
}

static void
test_receive_integration(void)
{
    struct iperf_test *test = iperf_new_test();
    struct iperf_stream stream;
    struct receive_script script;
    int complete;

    CHECK(test != NULL);
    CHECK_RESULT(iperf_defaults(test), 0);
    memset(&stream, 0, sizeof(stream));
    memset(&script, 0, sizeof(script));
    stream.test = test;
    stream.rcv = scripted_receive;
    stream.data = &script;
    stream.receive_budget = new_budget(8, 0);
    activate(stream.receive_budget);

    script.result = 0;
    script.error = EAGAIN;
    complete = -1;
    CHECK_RESULT(iperf_recv_mt_budget(&stream, &complete), 0);
    CHECK(complete == 0);
    CHECK(errno == EAGAIN);
    CHECK(script.calls == 1);
    CHECK(test->bytes_received == 0 && test->blocks_received == 0);

    script.result = -1;
    script.error = ECONNRESET;
    complete = -1;
    CHECK_RESULT(iperf_recv_mt_budget(&stream, &complete), -1);
    CHECK(complete == 0);
    CHECK(errno == ECONNRESET);
    CHECK(i_errno == IESTREAMREAD);
    CHECK(script.calls == 2);
    CHECK(test->bytes_received == 0 && test->blocks_received == 0);
    CHECK_RESULT(iperf_receive_budget_is_complete(stream.receive_budget), 0);

    script.result = 3;
    script.error = EINTR;
    complete = -1;
    CHECK_RESULT(iperf_recv_mt_budget(&stream, &complete), 0);
    CHECK(complete == 0);
    CHECK(errno == EINTR);
    CHECK(script.calls == 3);
    CHECK(test->bytes_received == 3 && test->blocks_received == 1);

    /* The public wrapper retains its historical zero-on-success convention. */
    script.result = 5;
    script.error = 0;
    CHECK_RESULT(iperf_recv_mt(&stream), 0);
    CHECK(script.calls == 4);
    CHECK(test->bytes_received == 8 && test->blocks_received == 2);
    CHECK_RESULT(iperf_receive_budget_is_complete(stream.receive_budget), 1);

    script.result = 9;
    complete = -1;
    CHECK_RESULT(iperf_recv_mt_budget(&stream, &complete), 0);
    CHECK(complete == 1);
    CHECK_RESULT(iperf_recv_mt(&stream), 0);
    CHECK(script.calls == 4);
    CHECK(test->bytes_received == 8 && test->blocks_received == 2);
    CHECK_RESULT(iperf_receive_budget_release(stream.receive_budget), 0);
    stream.receive_budget = NULL;

    /* The ordinary receive path remains available without a budget. */
    script.result = 2;
    complete = -1;
    CHECK_RESULT(iperf_recv_mt_budget(&stream, &complete), 0);
    CHECK(complete == 0);
    CHECK(script.calls == 5);
    CHECK(test->bytes_received == 10 && test->blocks_received == 3);

    stream.receive_budget = new_budget(0, 2);
    activate(stream.receive_budget);
    script.result = 1;
    CHECK_RESULT(iperf_recv_mt_budget(&stream, &complete), 0);
    CHECK(complete == 0);
    script.result = 7;
    CHECK_RESULT(iperf_recv_mt_budget(&stream, &complete), 0);
    CHECK(complete == 1);
    CHECK(script.calls == 7);
    CHECK(test->bytes_received == 18 && test->blocks_received == 5);
    CHECK_RESULT(iperf_receive_budget_release(stream.receive_budget), 0);
    stream.receive_budget = NULL;
    iperf_free_test(test);
}

static void
set_scope(struct iperf_test *test, struct protocol *protocol,
          char role, int protocol_id, int reverse, int bidirectional,
          int omit, uint_fast64_t bytes, uint_fast64_t blocks)
{
    protocol->id = protocol_id;
    test->protocol = protocol;
    test->role = role;
    test->reverse = reverse;
    test->bidirectional = bidirectional;
    test->omit = omit;
    test->settings->bytes = bytes;
    test->settings->blocks = blocks;
}

static void
test_scope_and_timeout(void)
{
    static const struct {
        char role;
        int protocol_id, reverse, bidirectional, omit;
        uint_fast64_t bytes, blocks;
        int expected;
    } scopes[] = {
        {'c', Ptcp, 1, 0, 1, 8, 0, TCP_BUDGET_SUPPORTED},
        {'c', Ptcp, 1, 0, 1, 0, 2, TCP_BUDGET_SUPPORTED},
        {'c', Ptcp, 1, 0, 1, 8, 2, TCP_BUDGET_SUPPORTED},
        {'s', Ptcp, 1, 0, 1, 8, 0, 0},
        {  0, Ptcp, 1, 0, 1, 8, 0, 0},
        {'c', Pudp, 1, 0, 1, 8, 0, 0},
        {'c', Psctp, 1, 0, 1, 8, 0, 0},
        {'c', Ptcp, 0, 0, 1, 8, 0, 0},
        {'c', Ptcp, 1, 1, 1, 8, 0, 0},
        {'c', Ptcp, 1, 0, 0, 8, 0, 0},
        {'c', Ptcp, 1, 0, -1, 8, 0, 0},
        {'c', Ptcp, 1, 0, 1, 0, 0, 0},
        {'c', Ptcp, 1, 0, 1, UINT_FAST64_MAX, 0, TCP_BUDGET_SUPPORTED}
    };
    static const struct {
        int has_timer;
        long timer_sec, timer_usec;
        int64_t receive_us;
        long expected_sec, expected_usec;
    } timeouts[] = {
        {0, 0, 0, 0, 1, 0},
        {0, 0, 0, -1, 1, 0},
        {0, 0, 0, 250000, 0, 250000},
        {0, 0, 0, 1500000, 1, 0},
        {1, 0, 0, 0, 0, 0},
        {1, 0, 0, 1, 0, 0},
        {1, 0, 100, 0, 0, 100},
        {1, 0, 250000, 100000, 0, 100000},
        {1, 0, 250000, 500000, 0, 250000},
        {1, 1, 0, 0, 1, 0},
        {1, 9, 0, 0, 1, 0},
        {1, 9, 0, 50000, 0, 50000},
        {1, 0, 999999, 1000000, 0, 999999},
        {1, 9, 1, INT64_MAX, 1, 0},
        {1, LONG_MAX, 0, 0, 1, 0},
        {1, LONG_MAX, 999999, 250000, 0, 250000}
    };
    struct iperf_test *test = iperf_new_test();
    struct protocol protocol;
    struct protocol *saved_protocol;
    Timer ordinary_timer;
    struct timeval timer, storage;
    const struct timeval *selected;
    size_t i;

    CHECK(test != NULL);
    CHECK_RESULT(iperf_defaults(test), 0);
    CHECK_RESULT(iperf_receive_budget_scope(test), 0);
    saved_protocol = test->protocol;
    memset(&protocol, 0, sizeof(protocol));
    for (i = 0; i < sizeof(scopes) / sizeof(scopes[0]); ++i) {
        set_scope(test, &protocol, scopes[i].role, scopes[i].protocol_id,
                  scopes[i].reverse, scopes[i].bidirectional, scopes[i].omit,
                  scopes[i].bytes, scopes[i].blocks);
        CHECK_RESULT(iperf_receive_budget_scope(test), scopes[i].expected);
        timer.tv_sec = 9;
        timer.tv_usec = 0;
        selected = iperf_receive_budget_timeout(test, &timer, 10, &storage);
        CHECK(selected != NULL);
        if (scopes[i].expected) {
            CHECK(selected->tv_sec == 0 && selected->tv_usec == 10);
        } else {
            CHECK(selected == &timer);
            CHECK(selected->tv_sec == 9 && selected->tv_usec == 0);
            CHECK(iperf_receive_budget_timeout(test, NULL, 10, &storage) == NULL);
        }
        CHECK(timer.tv_sec == 9 && timer.tv_usec == 0);
    }

    set_scope(test, &protocol, 'c', Ptcp, 1, 0, 1, 8, 0);
    for (i = 0; i < sizeof(timeouts) / sizeof(timeouts[0]); ++i) {
        timer.tv_sec = timeouts[i].timer_sec;
        timer.tv_usec = timeouts[i].timer_usec;
        storage.tv_sec = -1;
        storage.tv_usec = -1;
        selected = iperf_receive_budget_timeout(test,
            timeouts[i].has_timer ? &timer : NULL,
            timeouts[i].receive_us, &storage);
        if (TCP_BUDGET_SUPPORTED) {
            CHECK(selected != NULL);
            CHECK(selected->tv_sec == timeouts[i].expected_sec);
            CHECK(selected->tv_usec == timeouts[i].expected_usec);
        } else {
            CHECK(selected == (timeouts[i].has_timer ? &timer : NULL));
            CHECK(storage.tv_sec == -1 && storage.tv_usec == -1);
        }
        CHECK(timer.tv_sec == timeouts[i].timer_sec);
        CHECK(timer.tv_usec == timeouts[i].timer_usec);
    }

    /* Polling must not advance an ordinary report/statistics interval. */
    memset(&ordinary_timer, 0, sizeof(ordinary_timer));
    for (i = 1; i <= 3; ++i) {
        test->stats_timer = (i & 1) ? &ordinary_timer : NULL;
        test->reporter_timer = (i & 2) ? &ordinary_timer : NULL;
        timer.tv_sec = 5;
        timer.tv_usec = 123456;
        storage.tv_sec = -1;
        storage.tv_usec = -1;
        selected = iperf_receive_budget_timeout(test, &timer, 100000, &storage);
        CHECK(selected == &timer);
        CHECK(selected->tv_sec == 5 && selected->tv_usec == 123456);
        CHECK(storage.tv_sec == -1 && storage.tv_usec == -1);

        /* An already selected shorter wait or due timer also stays intact. */
        timer.tv_sec = 0;
        timer.tv_usec = 50000;
        selected = iperf_receive_budget_timeout(test, &timer, 100000, &storage);
        CHECK(selected == &timer);
        CHECK(selected->tv_sec == 0 && selected->tv_usec == 50000);
        timer.tv_usec = 0;
        selected = iperf_receive_budget_timeout(test, &timer, 100000, &storage);
        CHECK(selected == &timer);
        CHECK(selected->tv_sec == 0 && selected->tv_usec == 0);

        timer.tv_sec = LONG_MAX;
        timer.tv_usec = 999999;
        selected = iperf_receive_budget_timeout(test, &timer, 250000, &storage);
        CHECK(selected == &timer);
        CHECK(selected->tv_sec == LONG_MAX && selected->tv_usec == 999999);
    }
    /* These are stack stand-ins, not registered timers; never cancel/free them. */
    test->stats_timer = NULL;
    test->reporter_timer = NULL;
    timer.tv_sec = 5;
    timer.tv_usec = 123456;
    selected = iperf_receive_budget_timeout(test, &timer, 0, &storage);
    CHECK(selected != NULL);
    if (TCP_BUDGET_SUPPORTED)
        CHECK(selected->tv_sec == 1 && selected->tv_usec == 0);
    else
        CHECK(selected == &timer && timer.tv_sec == 5 && timer.tv_usec == 123456);

    test->reverse = 0;
    timer.tv_sec = LONG_MAX;
    timer.tv_usec = 999999;
    selected = iperf_receive_budget_timeout(test, &timer, 250000, &storage);
    CHECK(selected == &timer);
    CHECK(selected->tv_sec == LONG_MAX && selected->tv_usec == 999999);
    test->protocol = saved_protocol;
    iperf_free_test(test);
}

static void
make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    CHECK(flags >= 0);
    CHECK_RESULT(fcntl(fd, F_SETFL, flags | O_NONBLOCK), 0);
}

static void
read_frame_bytes(int fd, void *buffer, size_t size)
{
    char *cursor = buffer;

    while (size != 0) {
        ssize_t received = read(fd, cursor, size);

        if (received < 0 && errno == EINTR)
            continue;
        /* Nonblocking endpoints turn missing frame data into a failed check. */
        CHECK(received > 0);
        cursor += received;
        size -= (size_t) received;
    }
}

static void
test_parameter_serialization(void)
{
    static const struct {
        int protocol_id, reverse, bidirectional, omit;
        uint_fast64_t bytes, blocks;
        int expected_budget;
    } cases[] = {
        {Ptcp, 1, 0, 1, 64, 0, TCP_BUDGET_SUPPORTED},
        {Ptcp, 1, 0, 1, 0, 3, TCP_BUDGET_SUPPORTED},
        {Ptcp, 1, 0, 1, 64, 3, TCP_BUDGET_SUPPORTED},
        {Ptcp, 1, 0, 0, 64, 3, 0},
        {Ptcp, 0, 0, 1, 64, 3, 0},
        {Ptcp, 1, 1, 1, 64, 3, 0},
        {Pudp, 1, 0, 1, 64, 3, 0},
        {Psctp, 1, 0, 1, 64, 3, 0},
        {Ptcp, 1, 0, 1, 0, 0, 0}
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        struct iperf_test *test = iperf_new_test();
        struct protocol protocol;
        struct protocol *saved_protocol;
        int sockets[2];
        uint32_t wire_size, size;
        char payload[4097];
        cJSON *json, *bytes, *blocks;

        CHECK(test != NULL);
        CHECK_RESULT(iperf_defaults(test), 0);
        saved_protocol = test->protocol;
        memset(&protocol, 0, sizeof(protocol));
        set_scope(test, &protocol, 'c', cases[i].protocol_id, cases[i].reverse,
                  cases[i].bidirectional, cases[i].omit,
                  cases[i].bytes, cases[i].blocks);
        CHECK_RESULT(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);
        make_nonblocking(sockets[0]);
        make_nonblocking(sockets[1]);
        test->ctrl_sck = sockets[0];

        /* Client parameter exchange writes one small frame and never awaits a
         * peer response. No worker or peer thread is necessary for this check. */
        CHECK_RESULT(iperf_exchange_parameters(test), 0);
        CHECK((test->receive_budget != NULL) == cases[i].expected_budget);
        CHECK(test->settings->bytes == cases[i].bytes);
        CHECK(test->settings->blocks == cases[i].blocks);
        if (test->receive_budget)
            CHECK_RESULT(iperf_receive_budget_is_complete(test->receive_budget), 0);

        read_frame_bytes(sockets[1], &wire_size, sizeof(wire_size));
        size = ntohl(wire_size);
        CHECK(size > 0 && size < sizeof(payload));
        read_frame_bytes(sockets[1], payload, size);
        payload[size] = '\0';
        json = cJSON_Parse(payload);
        CHECK(json != NULL);
        bytes = cJSON_GetObjectItem(json, "num");
        blocks = cJSON_GetObjectItem(json, "blockcount");
        CHECK(cJSON_IsNumber(bytes));
        CHECK(cJSON_IsNumber(blocks));
        CHECK(bytes->valuedouble == (cases[i].expected_budget ? 0 : cases[i].bytes));
        CHECK(blocks->valuedouble == (cases[i].expected_budget ? 0 : cases[i].blocks));
        cJSON_Delete(json);
        CHECK_RESULT(close(sockets[0]), 0);
        CHECK_RESULT(close(sockets[1]), 0);
        test->ctrl_sck = -1;
        test->protocol = saved_protocol;
        iperf_free_test(test);
    }
}

int
main(void)
{
    test_publication_and_reset();
    test_finish_during_reset_pause();
    test_quantity_units();
    test_either_limit();
    test_inflight_allowance();
    test_large_budget();
    test_references_and_fresh_contexts();
    test_receive_integration();
    test_scope_and_timeout();
    test_parameter_serialization();
    printf("receive budget: %u checks passed\n", checks);
    return EXIT_SUCCESS;
}
