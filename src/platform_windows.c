#include "platform_provider.h"

#include <io.h>
#include <stdlib.h>
#include <windows.h>

static CRITICAL_SECTION graph_lock;
static INIT_ONCE graph_once = INIT_ONCE_STATIC_INIT;
static DWORD graph_tls = TLS_OUT_OF_INDEXES;

static BOOL CALLBACK init_provider(PINIT_ONCE once, PVOID parameter, PVOID *context) {
    (void)once;
    (void)parameter;
    (void)context;
    InitializeCriticalSection(&graph_lock);
    graph_tls = TlsAlloc();
    return graph_tls != TLS_OUT_OF_INDEXES;
}

static int provider_ready(void) {
    return InitOnceExecuteOnce(&graph_once, init_provider, NULL, NULL) != 0;
}

void h_platform_stopwatch_reset(struct HStopWatch *stopwatch) {
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;

    if (stopwatch != NULL && QueryPerformanceCounter(&counter) &&
        QueryPerformanceFrequency(&frequency)) {
        stopwatch->words[0] = counter.QuadPart;
        stopwatch->words[1] = frequency.QuadPart;
    }
}

int64_t h_platform_stopwatch_ns(struct HStopWatch *stopwatch) {
    LARGE_INTEGER now;
    int64_t elapsed;

    if (stopwatch == NULL || stopwatch->words[1] == 0 || !QueryPerformanceCounter(&now))
        return 0;
    elapsed = now.QuadPart - stopwatch->words[0];
    return (elapsed / stopwatch->words[1]) * INT64_C(1000000000) +
           ((elapsed % stopwatch->words[1]) * INT64_C(1000000000)) / stopwatch->words[1];
}

int h_platform_is_terminal(FILE *stream) {
    intptr_t handle;
    DWORD mode;

    if (stream == NULL)
        return 0;
    handle = _get_osfhandle(_fileno(stream));
    return handle != -1 && GetConsoleMode((HANDLE)handle, &mode) != 0;
}

void h_platform_graph_lock(void) {
    if (provider_ready())
        EnterCriticalSection(&graph_lock);
}

void h_platform_graph_unlock(void) {
    if (provider_ready())
        LeaveCriticalSection(&graph_lock);
}

void **h_platform_current_graph_slot(void) {
    void **slot;

    if (!provider_ready())
        return NULL;
    slot = TlsGetValue(graph_tls);
    if (slot == NULL) {
        slot = calloc(1, sizeof(*slot));
        if (slot != NULL)
            (void)TlsSetValue(graph_tls, slot);
    }
    return slot;
}
