#include "iperf_config.h"

#if defined(HAVE_PTHREAD)

#include <pthread.h>

#if defined(__ANDROID__)

/* Adding missing `pthread` related definitions in Android.
 */

 enum
 {
   PTHREAD_CANCEL_ENABLE,
 #define PTHREAD_CANCEL_ENABLE   PTHREAD_CANCEL_ENABLE
   PTHREAD_CANCEL_DISABLE
 #define PTHREAD_CANCEL_DISABLE  PTHREAD_CANCEL_DISABLE
 };
 enum
 {
   PTHREAD_CANCEL_DEFERRED,
 #define PTHREAD_CANCEL_DEFERRED	PTHREAD_CANCEL_DEFERRED
   PTHREAD_CANCEL_ASYNCHRONOUS
 #define PTHREAD_CANCEL_ASYNCHRONOUS	PTHREAD_CANCEL_ASYNCHRONOUS
 };

int pthread_setcanceltype(int type, int *oldtype);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_cancel(pthread_t thread_id);

#endif // defined(__ANDROID__)

#endif // defined(HAVE_PTHREAD)
