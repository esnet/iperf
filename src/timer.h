struct timer {
    struct timeval begin;
    struct timeval end;
    int (*expired)(struct timer *timer);
};

struct timer *new_timer(time_t sec, suseconds_t usec);
int delay(int64_t ns);
