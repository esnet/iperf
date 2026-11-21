/*
 * Private receive-admission budget prototype.
 * Distributed under the BSD style license in the project's LICENSE file.
 */
#ifndef IPERF_RECEIVE_BUDGET_H
#define IPERF_RECEIVE_BUDGET_H

#include <stdint.h>
#include <signal.h>
#include <sys/types.h>

struct iperf_receive_budget;

struct iperf_receive_budget_token {
    uint_fast64_t epoch;
    int eligible;
};

/* Caller-local storage; never share or copy a held guard. */
struct iperf_receive_budget_guard {
    sigset_t previous_mask;
    struct iperf_receive_budget *budget;
    int held;
    int reset_pending;
};

/*
 * This is a private prototype, not a wire-byte quota or reporting counter.
 * Zero disables that limit. Positive receive returns consume bytes and one
 * call; either enabled limit reaching zero completes an active budget.
 * Creation starts inactive, allowing uncharged warm-up admissions.
 *
 * Every caller must own a reference throughout an operation and throughout
 * an admitted receive/finish pair. Creation gives the controller one reference.
 * Retain a worker's reference BEFORE pthread_create; release it on creation
 * failure or from the worker's registered cleanup handler. Do not infer object
 * lifetime from a stream's done flag or from an unsuccessful join.
 *
 * Calls mask SIGINT, SIGTERM and SIGHUP, plus SIGUSR1 on Android, while holding
 * the mutex. Paused admissions sleep on a condition variable, releasing the
 * mutex; cancellation cleanup unlocks and leaves signals blocked until the
 * outer worker cleanup releases its reference and the thread exits.
 * Normal pthread cancellation must be deferred. The worker launcher
 * must also prevent Android SIGUSR1 from arriving before cleanup registration;
 * the two mask helpers below support that separate bootstrap responsibility.
 * This is not a general async-signal-safe API. Android cancellation relies on
 * the existing pthread_exit shim and the masking/cleanup protocol above.
 *
 * Unless stated otherwise, return 0 on success and -1 with errno on error.
 * A guard/teardown failure is fatal to the caller's current operation: do not
 * retry it. A failed unlock leaves the protected signals masked. Failed
 * retain/release teardown may have changed ownership; conservatively leak an
 * uncertain reference rather than retrying or freeing it. A failed last release
 * never frees an object whose mutex could not be unlocked and destroyed.
 */
int iperf_receive_budget_create(struct iperf_receive_budget **out,
                               uint_fast64_t bytes, uint_fast64_t blocks);
int iperf_receive_budget_retain(struct iperf_receive_budget *budget);
int iperf_receive_budget_release(struct iperf_receive_budget *budget);

/*
 * admit: 1 grants one receive callback, 0 denies it because the active budget
 * is complete, -1 is an error. Only use its token with this same context.
 * finish consumes the token; a positive return is charged only if the token
 * was eligible and its epoch still matches. Zero/negative returns do not
 * charge. finish and is_complete return 1 if complete, 0 otherwise, -1 on error.
 * Budget completion prevents later admissions; already admitted callbacks may
 * still return data. Raw receive/reporting counters remain the caller's concern.
 */
int iperf_receive_budget_admit(struct iperf_receive_budget *budget,
                              struct iperf_receive_budget_token *token);
int iperf_receive_budget_finish(struct iperf_receive_budget *budget,
                               struct iperf_receive_budget_token *token,
                               ssize_t received);
int iperf_receive_budget_is_complete(struct iperf_receive_budget *budget);

/*
 * A successful reset_begin pauses admissions and retains the protected signal
 * mask, but releases the mutex. Perform only the selected receiver's pure
 * statistics reset before reset_end: no I/O,
 * callbacks, cancellation points, nested resets/admissions or early exits.
 * Existing tokens may finish during the pause; their returns are not charged.
 * The guard
 * must be ended by the same thread. reset_end acquires the mutex, advances the
 * epoch, replenishes both limits, activates the budget and wakes admissions,
 * then unlocks/restores the mask. Finishes during the pause consume their token
 * without charging or reporting old completion. A token
 * from before this reset can never charge the new epoch. Repeated resets are
 * supported; epoch exhaustion is rejected by begin, before any caller reset.
 */
int iperf_receive_budget_reset_begin(struct iperf_receive_budget *budget,
                                    struct iperf_receive_budget_guard *guard);
int iperf_receive_budget_reset_end(struct iperf_receive_budget *budget,
                                  struct iperf_receive_budget_guard *guard);

int iperf_receive_budget_block_signals(sigset_t *previous_mask);
int iperf_receive_budget_restore_signals(const sigset_t *previous_mask);

#endif
