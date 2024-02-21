/* Replacement for `<pthread.h>` for the Android Workaround of using `pthread_kill()`
* instead of `pthread_cancel()` that os not supported by Android NDK.
 */
#include <pthread.h>

#define PTHREAD_CANCEL_ASYNCHRONOUS 0
#define PTHREAD_CANCEL_ENABLE 0

int pthread_setcanceltype(int type, int *oldtype);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_cancel(pthread_t thread_id);