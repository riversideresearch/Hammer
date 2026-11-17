/* RTEMS provider.  RTEMS test builds use one serial parser-graph context. */
#include "platform_provider.h"

#include <rtems.h>

static void *current_graph;

static void gettime(struct timespec *time) {
    if (time != NULL && rtems_clock_get_uptime(time) != RTEMS_SUCCESSFUL) {
        time->tv_sec = 0;
        time->tv_nsec = 0;
    }
}

void h_platform_stopwatch_reset(struct HStopWatch *stopwatch) {
    struct timespec time;

    if (stopwatch == NULL)
        return;
    gettime(&time);
    stopwatch->words[0] = (int64_t)time.tv_sec;
    stopwatch->words[1] = (int64_t)time.tv_nsec;
}

int64_t h_platform_stopwatch_ns(struct HStopWatch *stopwatch) {
    struct timespec now;

    if (stopwatch == NULL)
        return 0;
    gettime(&now);
    return ((int64_t)now.tv_sec - stopwatch->words[0]) * INT64_C(1000000000) +
           ((int64_t)now.tv_nsec - stopwatch->words[1]);
}

int h_platform_is_terminal(FILE *stream) {
    (void)stream;
    return 0;
}

void h_platform_graph_lock(void) {}
void h_platform_graph_unlock(void) {}
void **h_platform_current_graph_slot(void) { return &current_graph; }
