#include "hammer.h"
#include "test_suite.h"

#include <glib.h>
#include <stdarg.h>
#include <stdlib.h>

typedef struct {
    size_t live_allocations;
} TrackingAllocator;

typedef struct {
    void *target;
    size_t target_frees;
} QuarantineAllocator;

static void *tracking_alloc(HAllocator *allocator, size_t size) {
    TrackingAllocator *tracking = allocator->env;
    void *ptr = malloc(size);

    if (ptr)
        ++tracking->live_allocations;
    return ptr;
}

static void *tracking_realloc(HAllocator *allocator, void *ptr, size_t size) {
    TrackingAllocator *tracking = allocator->env;
    void *result = realloc(ptr, size);

    if (!ptr && result)
        ++tracking->live_allocations;
    else if (ptr && !result && size == 0)
        --tracking->live_allocations;
    return result;
}

static void tracking_free(HAllocator *allocator, void *ptr) {
    TrackingAllocator *tracking = allocator->env;

    if (ptr) {
        free(ptr);
        --tracking->live_allocations;
    }
}

static void *quarantine_alloc(HAllocator *allocator, size_t size) {
    (void)allocator;
    return malloc(size);
}

static void *quarantine_realloc(HAllocator *allocator, void *ptr, size_t size) {
    (void)allocator;
    return realloc(ptr, size);
}

static void quarantine_free(HAllocator *allocator, void *ptr) {
    QuarantineAllocator *quarantine = allocator->env;

    if (ptr == quarantine->target)
        ++quarantine->target_frees;
}

static HAllocator install_tracking_allocator(TrackingAllocator *tracking) {
    HAllocator saved = system_allocator;

    system_allocator = (HAllocator){
        .alloc = tracking_alloc,
        .realloc = tracking_realloc,
        .free = tracking_free,
        .vt = NULL,
        .env = tracking,
    };
    return saved;
}

static HAllocator install_quarantine_allocator(QuarantineAllocator *quarantine) {
    HAllocator saved = system_allocator;

    system_allocator = (HAllocator){
        .alloc = quarantine_alloc,
        .realloc = quarantine_realloc,
        .free = quarantine_free,
        .vt = NULL,
        .env = quarantine,
    };
    return saved;
}

static HParser *sequence_from_v(HParser *p, ...) {
    va_list ap;
    va_start(ap, p);
    HParser *result = h_sequence__v(p, ap);
    va_end(ap);
    return result;
}

static HParser *drop_from_v(HParser *p, ...) {
    va_list ap;
    va_start(ap, p);
    HParser *result = h_drop_from___v(p, ap);
    va_end(ap);
    return result;
}

static HParser *make_sequence_variant(unsigned variant, HParser *p1, HParser *p2) {
    void *args[] = {p1, p2, NULL};

    switch (variant) {
    case 0:
        return h_sequence(p1, p2, NULL);
    case 1:
        return h_sequence__m(&system_allocator, p1, p2, NULL);
    case 2:
        return sequence_from_v(p1, p2, NULL);
    case 3:
        return h_sequence__a(args);
    default:
        return h_sequence__ma(&system_allocator, args);
    }
}

static HParser *make_drop_from_variant(unsigned variant, HParser *sequence) {
    int indices[] = {1, -1};
    void *args[] = {sequence, indices};

    switch (variant) {
    case 0:
        return h_drop_from_(sequence, 1, -1);
    case 1:
        return h_drop_from___m(&system_allocator, sequence, 1, -1);
    case 2:
        return drop_from_v(sequence, 1, -1);
    case 3:
        return h_drop_from___a(args);
    default:
        return h_drop_from___ma(&system_allocator, args);
    }
}

static HParsedToken *identity_action(const HParseResult *result, void *user_data) {
    (void)user_data;
    return (HParsedToken *)result->ast;
}

static bool accept_predicate(HParseResult *result, void *user_data) {
    (void)result;
    (void)user_data;
    return true;
}

static void assert_parser_parses(HParser *parser, const uint8_t *input, size_t length) {
    HParseResult *result = h_parse(parser, input, length);
    g_assert_nonnull(result);
    h_parse_result_free(result);
}

static void parse_after_freeing_compiled_child(HParser *parent, HParser *child,
                                               HParserBackend backend, const uint8_t *input,
                                               size_t length) {
    g_assert_cmpint(h_compile(parent, backend, NULL), ==, 0);
    assert_parser_parses(parent, input, length);
    h_parser_free(child);

    assert_parser_parses(parent, input, length);
}

static void test_sequence_variants_free_root(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);

    for (unsigned variant = 0; variant < 5; ++variant) {
        HParser *p1 = h_ch('a');
        HParser *p2 = h_ch('b');
        size_t before_root = tracking.live_allocations;
        HParser *sequence = make_sequence_variant(variant, p1, p2);

        g_assert_nonnull(sequence);
        h_parser_free(sequence);
        g_assert_cmpuint(tracking.live_allocations, ==, before_root);

        h_parser_free(p1);
        h_parser_free(p2);
        g_assert_cmpuint(tracking.live_allocations, ==, 0);
    }

    system_allocator = saved;
}

static void test_drop_from_variants_free_root_and_wrappers(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);

    for (unsigned variant = 0; variant < 5; ++variant) {
        HParser *p1 = h_ch('a');
        HParser *p2 = h_ch('b');
        HParser *p3 = h_ch('c');
        HParser *sequence = h_sequence(p1, p2, p3, NULL);
        size_t before_rewrite = tracking.live_allocations;
        HParser *rewrite = make_drop_from_variant(variant, sequence);

        g_assert_nonnull(rewrite);
        h_parser_free(rewrite);
        g_assert_cmpuint(tracking.live_allocations, ==, before_rewrite);

        h_parser_free(sequence);
        h_parser_free(p1);
        h_parser_free(p2);
        h_parser_free(p3);
        g_assert_cmpuint(tracking.live_allocations, ==, 0);
    }

    system_allocator = saved;
}

static void test_lalr_desugaring_context_is_freed(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    HParser *a = h_ch('a');
    HParser *b = h_ch('b');
    HParser *c = h_ch('c');
    HParser *d = h_ch('d');
    HParser *choice = h_choice(b, c, NULL);
    HParser *sequence = h_sequence(a, choice, d, NULL);
    // HParseResult *res = h_parse(sequence, NULL, 0);
    g_assert_cmpint(h_compile(sequence, PB_LALR, NULL), ==, 0);
    // g_assert_nonnull(sequence->desugar_ctx);

    h_parser_free(sequence);
    h_parser_free(choice);
    h_parser_free(a);
    h_parser_free(b);
    h_parser_free(c);
    h_parser_free(d);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    system_allocator = saved;
}

static void test_lalr_desugaring_context_survives_child_frees(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    HParser *a = h_ch('a');
    HParser *b = h_ch('b');
    HParser *c = h_ch('c');
    HParser *d = h_ch('d');
    HParser *choice = h_choice(b, c, NULL);
    HParser *sequence = h_sequence(a, choice, d, NULL);

    g_assert_cmpint(h_compile(sequence, PB_LALR, NULL), ==, 0);

    h_parser_free(a);
    h_parser_free(b);
    h_parser_free(c);
    h_parser_free(d);
    h_parser_free(choice);
    h_parser_free(sequence);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    system_allocator = saved;
}

static void test_lalr_conflict_frees_table(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    HParser *d = h_ch('d');
    HParser *expr = h_indirect();
    HParser *plus = h_ch('+');
    HParser *sum = h_sequence(expr, plus, expr, NULL);
    HParser *choice = h_choice(sum, d, NULL);

    h_bind_indirect(expr, choice);
    g_assert_cmpint(h_compile(expr, PB_LALR, NULL), ==, -2);

    h_parser_free(expr);
    h_parser_free(choice);
    h_parser_free(sum);
    h_parser_free(plus);
    h_parser_free(d);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    system_allocator = saved;
}

static void test_lalr_parent_retains_independently_desugared_child(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    HParser *child = h_ch('a');
    HParser *tail = h_ch('b');
    HParser *parent = h_sequence(child, tail, NULL);

    g_assert_cmpint(h_compile(child, PB_LALR, NULL), ==, 0);
    g_assert_cmpint(h_compile(parent, PB_LALR, NULL), ==, 0);

    h_parser_free(child);

    HParseResult *result = h_parse(parent, (const uint8_t *)"ab", 2);
    g_assert_nonnull(result);
    h_parse_result_free(result);

    h_parser_free(parent);
    h_parser_free(tail);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    system_allocator = saved;
}

static void test_llk_terminal_start_is_freed(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    HParser *parser = h_ch('a');
    const uint8_t input[] = {'a'};

    g_assert_cmpint(h_compile(parser, PB_LL, NULL), ==, 0);

    HParseResult *result = h_parse(parser, input, sizeof(input));
    g_assert_nonnull(result);
    h_parse_result_free(result);

    h_parser_free(parser);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    system_allocator = saved;
}

static void test_contextfree_parent_survives_freed_child_env(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);

    {
        HParser *child = h_ch_range('a', 'c');
        HParser *tail = h_ch('x');
        HParser *parent = h_sequence(child, tail, NULL);
        const uint8_t input[] = {'b', 'x'};

        g_assert_cmpint(h_compile(child, PB_LALR, NULL), ==, 0);
        parse_after_freeing_compiled_child(parent, child, PB_LALR, input, sizeof(input));
        h_parser_free(parent);
        h_parser_free(tail);
        g_assert_cmpuint(tracking.live_allocations, ==, 0);
    }

    {
        HParser *child = h_float32();
        HParser *tail = h_ch('x');
        HParser *parent = h_sequence(child, tail, NULL);
        const uint8_t input[] = {0, 0, 0, 0, 'x'};

        parse_after_freeing_compiled_child(parent, child, PB_LALR, input, sizeof(input));
        h_parser_free(parent);
        h_parser_free(tail);
        g_assert_cmpuint(tracking.live_allocations, ==, 0);
    }

    system_allocator = saved;
}

static void test_regex_parent_survives_freed_child_env(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);

    {
        HParser *child = h_uint8();
        HParser *tail = h_ch('x');
        HParser *parent = h_sequence(child, tail, NULL);
        const uint8_t input[] = {'a', 'x'};

        parse_after_freeing_compiled_child(parent, child, PB_REGULAR, input, sizeof(input));
        h_parser_free(parent);
        h_parser_free(tail);
        g_assert_cmpuint(tracking.live_allocations, ==, 0);
    }

    {
        HParser *atom = h_ch('a');
        HParser *child = h_action(atom, identity_action, NULL);
        HParser *tail = h_ch('x');
        HParser *parent = h_sequence(child, tail, NULL);
        const uint8_t input[] = {'a', 'x'};

        parse_after_freeing_compiled_child(parent, child, PB_REGULAR, input, sizeof(input));
        h_parser_free(parent);
        h_parser_free(tail);
        h_parser_free(atom);
        g_assert_cmpuint(tracking.live_allocations, ==, 0);
    }

    {
        HParser *atom = h_ch('a');
        HParser *child = h_attr_bool(atom, accept_predicate, NULL);
        HParser *tail = h_ch('x');
        HParser *parent = h_sequence(child, tail, NULL);
        const uint8_t input[] = {'a', 'x'};

        parse_after_freeing_compiled_child(parent, child, PB_REGULAR, input, sizeof(input));
        h_parser_free(parent);
        h_parser_free(tail);
        h_parser_free(atom);
        g_assert_cmpuint(tracking.live_allocations, ==, 0);
    }

    {
        HParser *left = h_ch('a');
        HParser *right = h_ch('b');
        HParser *child = h_left(left, right);
        HParser *tail = h_ch('x');
        HParser *parent = h_sequence(child, tail, NULL);
        const uint8_t input[] = {'a', 'b', 'x'};

        parse_after_freeing_compiled_child(parent, child, PB_REGULAR, input, sizeof(input));
        h_parser_free(parent);
        h_parser_free(tail);
        h_parser_free(left);
        h_parser_free(right);
        g_assert_cmpuint(tracking.live_allocations, ==, 0);
    }

    {
        HParser *integer = h_uint8();
        HParser *child = h_int_range(integer, 1, 9);
        HParser *tail = h_ch('x');
        HParser *parent = h_sequence(child, tail, NULL);
        const uint8_t input[] = {5, 'x'};

        parse_after_freeing_compiled_child(parent, child, PB_REGULAR, input, sizeof(input));
        h_parser_free(parent);
        h_parser_free(tail);
        h_parser_free(integer);
        g_assert_cmpuint(tracking.live_allocations, ==, 0);
    }

    system_allocator = saved;
}

static void test_parser_graph_frees_complete_graph(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    const uint8_t input[] = {'a', 'b'};
    HParserGraph *graph = h_parser_graph_begin();
    g_assert_nonnull(graph);

    HParser *a = h_ch('a');
    HParser *b = h_ch('b');
    HParser *root = h_sequence(a, b, NULL);
    h_parser_graph_end(graph);

    assert_parser_parses(root, input, sizeof(input));
    h_parser_graph_free(graph);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    system_allocator = saved;
}

static void test_parser_graph_free_ends_collection(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    HParserGraph *graph = h_parser_graph_begin();
    g_assert_nonnull(graph);

    h_ch('a');
    h_parser_graph_free(graph);

    HParser *untracked = h_ch('b');
    h_parser_free(untracked);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    system_allocator = saved;
}

static void test_parser_graph_handles_shared_and_owned_nodes(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    HParserGraph *graph = h_parser_graph_begin();
    g_assert_nonnull(graph);

    HParser *shared = h_ch('a');
    HParser *left = h_sequence(shared, h_ch('b'), NULL);
    HParser *right = h_sequence(shared, h_ch('c'), NULL);
    HParser *choice = h_choice(left, right, NULL);
    HParser *sequence = h_sequence(choice, h_ch('d'), NULL);
    HParser *rewrite = h_drop_from_(sequence, 0, -1);
    h_parser_graph_end(graph);

    h_parser_graph_free(graph);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    system_allocator = saved;
}

static void test_parser_graph_nested_lifetimes(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    HParserGraph *outer = h_parser_graph_begin();
    g_assert_nonnull(outer);
    HParser *outer_parser = h_ch('a');

    HParserGraph *inner = h_parser_graph_begin();
    g_assert_nonnull(inner);
    HParser *inner_parser = h_ch('b');

    h_parser_graph_end(outer);
    h_parser_graph_free(outer);
    HParser *still_inner = h_ch('c');
    h_parser_graph_end(inner);
    h_parser_graph_free(inner);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    (void)outer_parser;
    (void)inner_parser;
    (void)still_inner;
    system_allocator = saved;
}

static void test_parser_graph_uses_parser_allocator(void) {
    TrackingAllocator tracking = {0};
    HAllocator allocator = {
        .alloc = tracking_alloc,
        .realloc = tracking_realloc,
        .free = tracking_free,
        .vt = NULL,
        .env = &tracking,
    };
    HParserGraph *graph = h_parser_graph_begin();
    g_assert_nonnull(graph);

    HParser *a = h_ch__m(&allocator, 'a');
    HParser *b = h_ch__m(&allocator, 'b');
    h_sequence__m(&allocator, a, b, NULL);
    h_parser_graph_end(graph);

    h_parser_graph_free(graph);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);
}

static void test_parser_graph_grows(void) {
    TrackingAllocator tracking = {0};
    HAllocator saved = install_tracking_allocator(&tracking);
    HParserGraph *graph = h_parser_graph_begin();
    g_assert_nonnull(graph);

    for (size_t i = 0; i < 33; ++i)
        h_ch((uint8_t)i);
    h_parser_graph_end(graph);

    h_parser_graph_free(graph);
    g_assert_cmpuint(tracking.live_allocations, ==, 0);

    system_allocator = saved;
}

static gpointer free_parser_from_worker(gpointer data) {
    h_parser_free(data);
    return NULL;
}

static gpointer free_graph_from_worker(gpointer data) {
    h_parser_graph_free(data);
    return NULL;
}

enum { concurrent_factory_workers = 4, concurrent_factory_iterations = 64 };

typedef struct {
    GMutex mutex;
    GCond ready_condition;
    GCond start_condition;
    unsigned ready;
    bool start;
    gint failed;
} ConcurrentFactoryState;

typedef struct {
    ConcurrentFactoryState *state;
    uint8_t character;
} ConcurrentFactoryWorker;

static gpointer construct_graphs_from_worker(gpointer data) {
    ConcurrentFactoryWorker *worker = data;
    ConcurrentFactoryState *state = worker->state;

    g_mutex_lock(&state->mutex);
    ++state->ready;
    g_cond_signal(&state->ready_condition);
    while (!state->start)
        g_cond_wait(&state->start_condition, &state->mutex);
    g_mutex_unlock(&state->mutex);

    for (unsigned i = 0; i < concurrent_factory_iterations; ++i) {
        const uint8_t input[] = {worker->character};
        HParserGraph *graph = h_parser_graph_begin();
        HParser *parser;
        HParseResult *result;

        if (graph == NULL) {
            g_atomic_int_set(&state->failed, true);
            return NULL;
        }
        parser = h_sequence(h_ch(worker->character), h_end_p(), NULL);
        h_parser_graph_end(graph);
        result = h_parse(parser, input, sizeof(input));
        if (result == NULL) {
            g_atomic_int_set(&state->failed, true);
            h_parser_graph_free(graph);
            return NULL;
        }
        h_parse_result_free(result);
        h_parser_graph_free(graph);
    }

    return NULL;
}

static void test_parser_graph_concurrent_factory_construction(void) {
    ConcurrentFactoryState state = {0};
    ConcurrentFactoryWorker workers[concurrent_factory_workers];
    GThread *threads[concurrent_factory_workers];

    g_mutex_init(&state.mutex);
    g_cond_init(&state.ready_condition);
    g_cond_init(&state.start_condition);
    for (unsigned i = 0; i < concurrent_factory_workers; ++i) {
        workers[i] = (ConcurrentFactoryWorker){
            .state = &state,
            .character = (uint8_t)('a' + i),
        };
        threads[i] = g_thread_new("parser-factory", construct_graphs_from_worker, &workers[i]);
    }

    g_mutex_lock(&state.mutex);
    while (state.ready < concurrent_factory_workers)
        g_cond_wait(&state.ready_condition, &state.mutex);
    state.start = true;
    g_cond_broadcast(&state.start_condition);
    g_mutex_unlock(&state.mutex);

    for (unsigned i = 0; i < concurrent_factory_workers; ++i)
        g_thread_join(threads[i]);
    g_assert_false(g_atomic_int_get(&state.failed));
    g_cond_clear(&state.start_condition);
    g_cond_clear(&state.ready_condition);
    g_mutex_clear(&state.mutex);
}

static void test_parser_graph_untracks_cross_thread_parser_free(void) {
    QuarantineAllocator quarantine = {0};
    HAllocator saved = install_quarantine_allocator(&quarantine);
    HParserGraph *graph = h_parser_graph_begin();
    g_assert_nonnull(graph);

    HParser *parser = h_ch('a');
    h_parser_graph_end(graph);
    quarantine.target = parser;

    GThread *worker = g_thread_new("parser-free", free_parser_from_worker, parser);
    g_thread_join(worker);
    g_assert_cmpuint(quarantine.target_frees, ==, 1);

    h_parser_graph_free(graph);
    g_assert_cmpuint(quarantine.target_frees, ==, 1);

    system_allocator = saved;
}

static void test_parser_graph_foreign_free_removes_registry_entry(void) {
    QuarantineAllocator quarantine = {0};
    HAllocator saved = install_quarantine_allocator(&quarantine);
    HParserGraph *graph = h_parser_graph_begin();
    g_assert_nonnull(graph);
    h_ch('a');
    h_parser_graph_end(graph);

    GThread *worker = g_thread_new("graph-free", free_graph_from_worker, graph);
    GThread *duplicate_worker = g_thread_new("graph-free-duplicate", free_graph_from_worker, graph);
    g_thread_join(worker);
    g_thread_join(duplicate_worker);

    /*
     * These graphs are created and destroyed on the construction thread after
     * the foreign free. The untracked parser makes registry traversal inspect
     * the old graph if the foreign free left a stale entry behind.
     */
    for (unsigned i = 0; i < 2; ++i) {
        HParserGraph *subsequent = h_parser_graph_begin();
        g_assert_nonnull(subsequent);
        h_parser_graph_end(subsequent);
        h_parser_graph_free(subsequent);

        HParser *untracked = h_ch((uint8_t)('b' + i));
        h_parser_free(untracked);
    }

    system_allocator = saved;
}

void register_parser_free_tests(void) {
    g_test_add_func("/core/parser/free/sequence_variants", test_sequence_variants_free_root);
    g_test_add_func("/core/parser/free/drop_from_variants",
                    test_drop_from_variants_free_root_and_wrappers);
    g_test_add_func("/core/parser/free/lalr_desugaring_context",
                    test_lalr_desugaring_context_is_freed);
    g_test_add_func("/core/parser/free/lalr_desugaring_context_child_first",
                    test_lalr_desugaring_context_survives_child_frees);
    g_test_add_func("/core/parser/free/lalr_conflict_table", test_lalr_conflict_frees_table);
    g_test_add_func("/core/parser/free/lalr_independently_desugared_child",
                    test_lalr_parent_retains_independently_desugared_child);
    g_test_add_func("/core/parser/free/llk_terminal_start", test_llk_terminal_start_is_freed);
    g_test_add_func("/core/parser/free/contextfree_child_env",
                    test_contextfree_parent_survives_freed_child_env);
    g_test_add_func("/core/parser/free/regex_child_env",
                    test_regex_parent_survives_freed_child_env);
    g_test_add_func("/core/parser/free/graph/complete", test_parser_graph_frees_complete_graph);
    g_test_add_func("/core/parser/free/graph/ends_collection",
                    test_parser_graph_free_ends_collection);
    g_test_add_func("/core/parser/free/graph/shared_and_owned",
                    test_parser_graph_handles_shared_and_owned_nodes);
    g_test_add_func("/core/parser/free/graph/nested", test_parser_graph_nested_lifetimes);
    g_test_add_func("/core/parser/free/graph/parser_allocator",
                    test_parser_graph_uses_parser_allocator);
    g_test_add_func("/core/parser/free/graph/grows", test_parser_graph_grows);
    g_test_add_func("/core/parser/free/graph/concurrent_factory_construction",
                    test_parser_graph_concurrent_factory_construction);
    g_test_add_func("/core/parser/free/graph/cross_thread_parser",
                    test_parser_graph_untracks_cross_thread_parser_free);
    g_test_add_func("/core/parser/free/graph/cross_thread_graph",
                    test_parser_graph_foreign_free_removes_registry_entry);
}
