#include "iperf_config.h"

#if defined(HAVE_PTHREAD) && defined(__ANDROID__)

/* Workaround for `pthread_cancel()` in Android, using `pthread_kill()` instead,
 * as Android NDK does not support `pthread_cancel()`.
 */

#include <string.h>
#include <signal.h>
#include "iperf_pthread.h"

void iperf_thread_exit_handler(int sig)
{
    pthread_exit(0);
}

int iperf_set_thread_exit_handler() {
    int rc;
    struct sigaction actions;

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = iperf_thread_exit_handler;

    rc = sigaction(SIGUSR1, &actions, NULL);
    return rc;
}

int pthread_setcanceltype(int type, int *oldtype) { return 0; }
int pthread_setcancelstate(int state, int *oldstate) { return 0; }
int pthread_cancel(pthread_t thread_id) {
    int status;
    if ((status = iperf_set_thread_exit_handler()) == 0) {
        status = pthread_kill(thread_id, SIGUSR1);
    }
    return status;
}

#endif // defined(HAVE_PTHREAD) && defined(__ANDROID__)
