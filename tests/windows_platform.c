/* Exercise the real Windows provider with allocation and API failure tracking. */
#include <stdlib.h>
#include <windows.h>

static LONG outstanding_allocations;
static int fail_allocation;
static int fail_fls_alloc;
static int fail_fls_set;

static void *tracked_calloc(size_t count, size_t size) {
    void *allocation;

    if (fail_allocation)
        return NULL;
    allocation = calloc(count, size);
    if (allocation != NULL)
        InterlockedIncrement(&outstanding_allocations);
    return allocation;
}

static void tracked_free(void *allocation) {
    if (allocation != NULL) {
        InterlockedDecrement(&outstanding_allocations);
        free(allocation);
    }
}

static DWORD tracked_fls_alloc(PFLS_CALLBACK_FUNCTION callback) {
    return fail_fls_alloc ? FLS_OUT_OF_INDEXES : FlsAlloc(callback);
}

static BOOL tracked_fls_set(DWORD index, PVOID value) {
    return fail_fls_set ? FALSE : FlsSetValue(index, value);
}

/* Include the implementation to inject failures without changing the library API. */
#define calloc tracked_calloc
#define free tracked_free
#define FlsAlloc tracked_fls_alloc
#define FlsSetValue tracked_fls_set
#include "../src/platform_windows.c"
#undef calloc
#undef free
#undef FlsAlloc
#undef FlsSetValue

static DWORD WINAPI slot_worker(void *data) {
    void **slot = h_platform_current_graph_slot();

    if (slot == NULL || *slot != NULL)
        return 1;
    *slot = data;
    if (h_platform_current_graph_slot() != slot || *slot != data)
        return 1;
    return 0;
}

int main(void) {
    int marker;
    void **slot;

    /* Failed initialization must remain retryable. */
    fail_fls_alloc = 1;
    if (h_platform_current_graph_slot() != NULL)
        return 1;
    fail_fls_alloc = 0;

    fail_allocation = 1;
    if (h_platform_current_graph_slot() != NULL)
        return 1;
    fail_allocation = 0;

    /* A failed store must free its allocation and not return an untracked slot. */
    fail_fls_set = 1;
    if (h_platform_current_graph_slot() != NULL || outstanding_allocations != 0)
        return 1;
    fail_fls_set = 0;

    slot = h_platform_current_graph_slot();
    if (slot == NULL || *slot != NULL || outstanding_allocations != 1)
        return 1;
    *slot = &marker;

    /* Each worker's slot must be independent and freed before its thread exits. */
    for (unsigned i = 0; i < 64; ++i) {
        DWORD result;
        HANDLE thread = CreateThread(NULL, 0, slot_worker, &marker, 0, NULL);

        if (thread == NULL)
            return 1;
        if (WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0 ||
            !GetExitCodeThread(thread, &result) || result != 0) {
            CloseHandle(thread);
            return 1;
        }
        CloseHandle(thread);
        if (InterlockedCompareExchange(&outstanding_allocations, 0, 0) != 1 ||
            h_platform_current_graph_slot() != slot || *slot != &marker)
            return 1;
    }

    /* FlsFree also runs the callback for the calling thread's remaining slot. */
    if (!FlsFree(graph_fls) || outstanding_allocations != 0)
        return 1;
    return 0;
}
