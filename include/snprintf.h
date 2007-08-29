#ifndef SNPRINTF_H
#define SNPRINTF_H

/* ===================================================================
 * snprintf.h
 *
 * This is from
 * W. Richard Stevens, 'UNIX Network Programming', Vol 1, 2nd Edition,
 *   Prentice Hall, 1998.
 *
 * Mark Gates <mgates@nlanr.net>
 * July 1998
 *
 * to use this prototype, make sure HAVE_SNPRINTF is not defined
 *
 * =================================================================== */

/*
 * Throughout the book I use snprintf() because it's safer than sprintf().
 * But as of the time of this writing, not all systems provide this
 * function.  The function below should only be built on those systems
 * that do not provide a real snprintf().
 * The function below just acts like sprintf(); it is not safe, but it
 * tries to detect overflow.
 */

#ifndef HAVE_SNPRINTF

    #ifdef __cplusplus
extern "C" {
#endif

int snprintf(char *buf, size_t size, const char *fmt, ...);

#ifdef __cplusplus
} /* end extern "C" */
    #endif

#endif /* HAVE_SNPRINTF */
#endif /* SNPRINTF_H */
