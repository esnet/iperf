#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "timer.h"

int main(int argc, char **argv)
{
    struct timer *tp;
    tp = new_timer(3, 0);

    sleep(2);

    if(tp->expired(tp)) {
        printf("timer should not have expired\n");
        exit(-1);
    }

    sleep(1);

    if(!tp->expired(tp)) {
        printf("timer should have expired\n");
        exit(-2);
    }

    exit(0);
}
