#include "iperf_config.h"

#if defined(HAVE_PTHREAD)

#include <pthread.h>

#if defined(__ANDROID__)

/* Adding missing `pthread` related definitions in Android.
 */

#define PTHREAD_CANCEL_ASYNCHRONOUS 0
#define PTHREAD_CANCEL_ENABLE 0

int pthread_setcanceltype(int type, int *oldtype);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_cancel(pthread_t thread_id);

#endif // defined(__ANDROID__)

#endif // defined(HAVE_PTHREAD)
