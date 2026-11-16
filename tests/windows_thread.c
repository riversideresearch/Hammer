#include "hammer.h"

#include <windows.h>

static LONG parser_frees;
static LONG factory_ready;
static LONG factory_failures;
static HANDLE factory_start;

enum { factory_workers = 4, factory_iterations = 64 };

static void *test_alloc(HAllocator *allocator, size_t size) {
    (void)allocator;
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static void *test_realloc(HAllocator *allocator, void *ptr, size_t size) {
    (void)allocator;
    if (ptr == NULL)
        return HeapAlloc(GetProcessHeap(), 0, size);
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}

static void test_free(HAllocator *allocator, void *ptr) {
    (void)allocator;
    if (ptr != NULL) {
        HeapFree(GetProcessHeap(), 0, ptr);
        InterlockedIncrement(&parser_frees);
    }
}

static DWORD WINAPI worker(void *graph) {
    h_parser_graph_free(graph);
    return 0;
}

static DWORD WINAPI factory_worker(void *data) {
    const uint8_t character = (uint8_t)(uintptr_t)data;

    InterlockedIncrement(&factory_ready);
    if (WaitForSingleObject(factory_start, INFINITE) != WAIT_OBJECT_0)
        return 1;

    for (unsigned i = 0; i < factory_iterations; ++i) {
        const uint8_t input[] = {character};
        HParserGraph *graph = h_parser_graph_begin();
        HParser *parser;
        HParseResult *result;

        if (graph == NULL) {
            InterlockedIncrement(&factory_failures);
            return 1;
        }
        parser = h_sequence(h_ch(character), h_end_p(), NULL);
        h_parser_graph_end(graph);
        result = h_parse(parser, input, sizeof(input));
        if (result == NULL) {
            InterlockedIncrement(&factory_failures);
            h_parser_graph_free(graph);
            return 1;
        }
        h_parse_result_free(result);
        h_parser_graph_free(graph);
    }

    return 0;
}

int main(void) {
    HANDLE threads[factory_workers];
    DWORD result;
    int index;
    HAllocator allocator = {
        .alloc = test_alloc,
        .realloc = test_realloc,
        .free = test_free,
        .vt = NULL,
        .env = NULL,
    };
    HParserGraph *graph;

    factory_start = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (factory_start == NULL)
        return 1;
    for (index = 0; index < factory_workers; ++index) {
        threads[index] =
            CreateThread(NULL, 0, factory_worker, (void *)(uintptr_t)('a' + index), 0, NULL);
        if (threads[index] == NULL)
            return 1;
    }
    while (InterlockedCompareExchange(&factory_ready, 0, 0) < factory_workers)
        Sleep(0);
    if (!SetEvent(factory_start))
        return 1;
    if (WaitForMultipleObjects(factory_workers, threads, TRUE, INFINITE) != WAIT_OBJECT_0)
        return 1;
    for (index = 0; index < factory_workers; ++index) {
        if (!GetExitCodeThread(threads[index], &result) || result != 0)
            return 1;
        CloseHandle(threads[index]);
    }
    CloseHandle(factory_start);
    if (InterlockedCompareExchange(&factory_failures, 0, 0) != 0)
        return 1;

    graph = h_parser_graph_begin();
    if (graph == NULL || h_ch__m(&allocator, 'x') == NULL)
        return 1;
    h_parser_graph_end(graph);

    for (index = 0; index < 2; ++index) {
        threads[index] = CreateThread(NULL, 0, worker, graph, 0, NULL);
        if (threads[index] == NULL)
            return 1;
    }
    if (WaitForMultipleObjects(2, threads, TRUE, INFINITE) != WAIT_OBJECT_0)
        return 1;
    for (index = 0; index < 2; ++index) {
        if (!GetExitCodeThread(threads[index], &result) || result != 0)
            return 1;
        CloseHandle(threads[index]);
    }
    return InterlockedCompareExchange(&parser_frees, 0, 0) == 1 ? 0 : 1;
}
