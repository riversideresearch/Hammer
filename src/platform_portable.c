/* Hosted-C99 fallback provider. It supports one serial graph context. */
#include "platform_provider.h"

#include <time.h>

static void *current_graph;

void h_platform_stopwatch_reset(struct HStopWatch *stopwatch) {
    clock_t start;

    if (stopwatch == NULL)
        return;
    start = clock();
    if (start == (clock_t)-1 || CLOCKS_PER_SEC <= 0) {
        stopwatch->words[0] = 0;
        stopwatch->words[1] = 0;
    } else {
        stopwatch->words[0] = (int64_t)start;
        stopwatch->words[1] = (int64_t)CLOCKS_PER_SEC;
    }
}

int64_t h_platform_stopwatch_ns(struct HStopWatch *stopwatch) {
    clock_t now;
    int64_t elapsed;

    if (stopwatch == NULL || stopwatch->words[1] <= 0)
        return 0;
    now = clock();
    if (now == (clock_t)-1)
        return 0;
    elapsed = (int64_t)now - stopwatch->words[0];
    return (elapsed / stopwatch->words[1]) * INT64_C(1000000000) +
           ((elapsed % stopwatch->words[1]) * INT64_C(1000000000)) / stopwatch->words[1];
}

int h_platform_is_terminal(FILE *stream) {
    (void)stream;
    return 0;
}

void h_platform_graph_lock(void) {}
void h_platform_graph_unlock(void) {}
void **h_platform_current_graph_slot(void) { return &current_graph; }
