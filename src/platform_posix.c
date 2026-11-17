#include "platform_provider.h"

#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t graph_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t graph_key;
static pthread_once_t graph_key_once = PTHREAD_ONCE_INIT;
static int graph_key_ready;

static void make_graph_key(void) { graph_key_ready = pthread_key_create(&graph_key, free) == 0; }

static void gettime(struct timespec *time) {
    if (time != NULL && clock_gettime(CLOCK_THREAD_CPUTIME_ID, time) != 0) {
        time->tv_sec = 0;
        time->tv_nsec = 0;
    }
}

void h_platform_stopwatch_reset(struct HStopWatch *stopwatch) {
    if (stopwatch != NULL)
        gettime(&stopwatch->start);
}

int64_t h_platform_stopwatch_ns(struct HStopWatch *stopwatch) {
    struct timespec now;

    if (stopwatch == NULL)
        return 0;
    gettime(&now);
    return ((int64_t)now.tv_sec - (int64_t)stopwatch->start.tv_sec) * INT64_C(1000000000) +
           ((int64_t)now.tv_nsec - (int64_t)stopwatch->start.tv_nsec);
}

int h_platform_is_terminal(FILE *stream) { return stream != NULL && isatty(fileno(stream)) != 0; }

void h_platform_graph_lock(void) { (void)pthread_mutex_lock(&graph_lock); }
void h_platform_graph_unlock(void) { (void)pthread_mutex_unlock(&graph_lock); }

void **h_platform_current_graph_slot(void) {
    void **slot;

    if (pthread_once(&graph_key_once, make_graph_key) != 0 || !graph_key_ready)
        return NULL;

    slot = pthread_getspecific(graph_key);
    if (slot == NULL) {
        slot = calloc(1, sizeof(*slot));
        if (slot != NULL && pthread_setspecific(graph_key, slot) != 0) {
            free(slot);
            slot = NULL;
        }
    }
    return slot;
}
