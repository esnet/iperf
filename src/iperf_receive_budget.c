/*
 * Private receive-admission budget prototype.
 * Distributed under the BSD style license in the project's LICENSE file.
 */
#include "iperf_config.h"
#include "iperf_receive_budget.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

struct iperf_receive_budget {
    pthread_mutex_t mutex;
    pthread_cond_t resumed;
    size_t references;
    uint_fast64_t byte_limit;
    uint_fast64_t block_limit;
    uint_fast64_t bytes_remaining;
    uint_fast64_t blocks_remaining;
    uint_fast64_t epoch;
    int bytes_enabled;
    int blocks_enabled;
    int active;
    int paused;
};

int
iperf_receive_budget_block_signals(sigset_t *previous_mask)
{
    sigset_t signals;
    int error;

    if (previous_mask == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (sigemptyset(&signals) < 0 ||
        sigaddset(&signals, SIGINT) < 0 ||
        sigaddset(&signals, SIGTERM) < 0 ||
        sigaddset(&signals, SIGHUP) < 0)
        return -1;
#ifdef __ANDROID__
    if (sigaddset(&signals, SIGUSR1) < 0)
        return -1;
#endif
    error = pthread_sigmask(SIG_BLOCK, &signals, previous_mask);
    if (error != 0) {
        errno = error;
        return -1;
    }
    return 0;
}

int
iperf_receive_budget_restore_signals(const sigset_t *previous_mask)
{
    int error;

    if (previous_mask == NULL) {
        errno = EINVAL;
        return -1;
    }
    error = pthread_sigmask(SIG_SETMASK, previous_mask, NULL);
    if (error != 0) {
        errno = error;
        return -1;
    }
    return 0;
}

static int
budget_lock(struct iperf_receive_budget *budget,
            struct iperf_receive_budget_guard *guard)
{
    int error;

    if (budget == NULL || guard == NULL) {
        errno = EINVAL;
        return -1;
    }
    guard->budget = NULL;
    guard->held = 0;
    guard->reset_pending = 0;
    if (iperf_receive_budget_block_signals(&guard->previous_mask) < 0)
        return -1;
    error = pthread_mutex_lock(&budget->mutex);
    if (error != 0) {
        if (iperf_receive_budget_restore_signals(&guard->previous_mask) < 0)
            return -1;
        errno = error;
        return -1;
    }
    guard->budget = budget;
    guard->held = 1;
    return 0;
}

/* The operation's result is published only after unlock and mask restoration. */
static int
budget_unlock(struct iperf_receive_budget_guard *guard, int result,
              int operation_error)
{
    int error;

    error = pthread_mutex_unlock(&guard->budget->mutex);
    if (error != 0) {
        /* The mutex may still be held: do not deliver a terminating signal. */
        errno = error;
        return -1;
    }
    guard->held = 0;
    guard->reset_pending = 0;
    guard->budget = NULL;
    if (iperf_receive_budget_restore_signals(&guard->previous_mask) < 0)
        return -1;
    if (operation_error != 0)
        errno = operation_error;
    return result;
}

static int
budget_complete(const struct iperf_receive_budget *budget)
{
    return budget->active && !budget->paused &&
        ((budget->bytes_enabled && budget->bytes_remaining == 0) ||
         (budget->blocks_enabled && budget->blocks_remaining == 0));
}

/* pthread_cond_wait reacquires the mutex before running cancellation cleanup. */
static void
budget_wait_cancel(void *argument)
{
    struct iperf_receive_budget_guard *guard = argument;
    /* Keep signals blocked until outer cleanup releases the worker reference. */
    if (pthread_mutex_unlock(&guard->budget->mutex) == 0)
        guard->held = 0;
}

static int
budget_wait(struct iperf_receive_budget_guard *guard)
{
    int error;

    pthread_cleanup_push(budget_wait_cancel, guard);
    error = pthread_cond_wait(&guard->budget->resumed,
                              &guard->budget->mutex);
    pthread_cleanup_pop(0);
    return error;
}

int
iperf_receive_budget_create(struct iperf_receive_budget **out,
                            uint_fast64_t bytes, uint_fast64_t blocks)
{
    struct iperf_receive_budget *budget;
    sigset_t previous_mask;
    int error;

    if (out == NULL) {
        errno = EINVAL;
        return -1;
    }
    *out = NULL;
    if (iperf_receive_budget_block_signals(&previous_mask) < 0)
        return -1;
    budget = calloc(1, sizeof(*budget));
    if (budget == NULL) {
        error = errno;
        if (iperf_receive_budget_restore_signals(&previous_mask) < 0)
            return -1;
        errno = error;
        return -1;
    }
    error = pthread_mutex_init(&budget->mutex, NULL);
    if (error != 0) {
        free(budget);
        if (iperf_receive_budget_restore_signals(&previous_mask) < 0)
            return -1;
        errno = error;
        return -1;
    }
    error = pthread_cond_init(&budget->resumed, NULL);
    if (error != 0) {
        if (pthread_mutex_destroy(&budget->mutex) == 0)
            free(budget);
        if (iperf_receive_budget_restore_signals(&previous_mask) < 0)
            return -1;
        errno = error;
        return -1;
    }
    budget->references = 1;
    budget->byte_limit = bytes;
    budget->block_limit = blocks;
    budget->bytes_remaining = bytes;
    budget->blocks_remaining = blocks;
    budget->bytes_enabled = bytes != 0;
    budget->blocks_enabled = blocks != 0;
    /* Publish ownership before an unmasked CLI signal can longjmp to cleanup. */
    *out = budget;
    if (iperf_receive_budget_restore_signals(&previous_mask) < 0) {
        error = errno;
        *out = NULL;
        if (pthread_cond_destroy(&budget->resumed) == 0 &&
            pthread_mutex_destroy(&budget->mutex) == 0)
            free(budget);
        errno = error;
        return -1;
    }
    return 0;
}

int
iperf_receive_budget_retain(struct iperf_receive_budget *budget)
{
    struct iperf_receive_budget_guard guard;

    if (budget_lock(budget, &guard) < 0)
        return -1;
    if (budget->references == 0)
        return budget_unlock(&guard, -1, EINVAL);
    if (budget->references == SIZE_MAX)
        return budget_unlock(&guard, -1, EOVERFLOW);
    ++budget->references;
    return budget_unlock(&guard, 0, 0);
}

int
iperf_receive_budget_release(struct iperf_receive_budget *budget)
{
    struct iperf_receive_budget_guard guard;
    int error;

    if (budget_lock(budget, &guard) < 0)
        return -1;
    if (budget->references == 0)
        return budget_unlock(&guard, -1, EINVAL);
    if (--budget->references != 0)
        return budget_unlock(&guard, 0, 0);

    /* No other valid caller remains. Keep signals masked through destruction. */
    error = pthread_mutex_unlock(&budget->mutex);
    if (error != 0) {
        errno = error;
        return -1;
    }
    error = pthread_cond_destroy(&budget->resumed);
    if (error == 0)
        error = pthread_mutex_destroy(&budget->mutex);
    if (error == 0)
        free(budget);
    if (iperf_receive_budget_restore_signals(&guard.previous_mask) < 0)
        return -1;
    if (error != 0) {
        errno = error;
        return -1;
    }
    return 0;
}

int
iperf_receive_budget_admit(struct iperf_receive_budget *budget,
                           struct iperf_receive_budget_token *token)
{
    struct iperf_receive_budget_guard guard;
    int error;
    int result;

    if (token == NULL) {
        errno = EINVAL;
        return -1;
    }
    token->epoch = 0;
    token->eligible = 0;
    if (budget_lock(budget, &guard) < 0)
        return -1;
    while (budget->paused) {
        error = budget_wait(&guard);
        if (error != 0)
            return budget_unlock(&guard, -1, error);
    }
    result = !budget_complete(budget);
    if (result) {
        token->epoch = budget->epoch;
        token->eligible = budget->active;
    }
    return budget_unlock(&guard, result, 0);
}

int
iperf_receive_budget_finish(struct iperf_receive_budget *budget,
                            struct iperf_receive_budget_token *token,
                            ssize_t received)
{
    struct iperf_receive_budget_guard guard;
    uintmax_t amount;
    int complete;

    if (token == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (budget_lock(budget, &guard) < 0)
        return -1;
    if (received > 0 && token->eligible && budget->active && !budget->paused &&
        token->epoch == budget->epoch) {
        amount = (uintmax_t) received;
        if (budget->bytes_enabled) {
            if (amount >= budget->bytes_remaining)
                budget->bytes_remaining = 0;
            else
                budget->bytes_remaining -= (uint_fast64_t) amount;
        }
        if (budget->blocks_enabled && budget->blocks_remaining > 0)
            --budget->blocks_remaining;
    }
    token->eligible = 0;
    complete = budget_complete(budget);
    return budget_unlock(&guard, complete, 0);
}

int
iperf_receive_budget_is_complete(struct iperf_receive_budget *budget)
{
    struct iperf_receive_budget_guard guard;
    int complete;

    if (budget_lock(budget, &guard) < 0)
        return -1;
    complete = budget_complete(budget);
    return budget_unlock(&guard, complete, 0);
}

int
iperf_receive_budget_reset_begin(struct iperf_receive_budget *budget,
                                 struct iperf_receive_budget_guard *guard)
{
    int error;

    if (budget_lock(budget, guard) < 0)
        return -1;
    if (budget->paused)
        return budget_unlock(guard, -1, EBUSY);
    if (budget->epoch == UINT_FAST64_MAX)
        return budget_unlock(guard, -1, EOVERFLOW);
    budget->paused = 1;
    guard->reset_pending = 1;
    error = pthread_mutex_unlock(&budget->mutex);
    if (error != 0) {
        errno = error;
        return -1;
    }
    guard->held = 0;
    /* Keep the controller's signals masked until this pause is ended. */
    return 0;
}

int
iperf_receive_budget_reset_end(struct iperf_receive_budget *budget,
                               struct iperf_receive_budget_guard *guard)
{
    int error;

    if (budget == NULL || guard == NULL || guard->held ||
        !guard->reset_pending ||
        guard->budget != budget) {
        errno = EINVAL;
        return -1;
    }
    error = pthread_mutex_lock(&budget->mutex);
    if (error != 0) {
        errno = error;
        return -1;
    }
    guard->held = 1;
    ++budget->epoch;
    budget->bytes_remaining = budget->byte_limit;
    budget->blocks_remaining = budget->block_limit;
    budget->active = 1;
    budget->paused = 0;
    error = pthread_cond_broadcast(&budget->resumed);
    return budget_unlock(guard, error == 0 ? 0 : -1, error);
}
