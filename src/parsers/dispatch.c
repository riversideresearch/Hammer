/* Copyright (c) 2026 Riverside Research */
#include "parser_internal.h"

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>

#if defined(__STDC_VERSION__) &&                                                                   \
    ((__STDC_VERSION__ >= 201112L && !defined(__STDC_NO_VLA__)) || (__STDC_VERSION__ >= 199901L))
#define STACK_VLA(type, name, size) type name[size]
#else
#if defined(_MSC_VER)
#include <malloc.h> // for _alloca
#define STACK_VLA(type, name, size) type *name = _alloca(size)
#else
#error "Missing VLA implementation for this compiler"
#endif
#endif

typedef struct {
    uint32_t opcode;
    HParser *parser;
    bool used;
} DispatchBucket;

typedef struct {
    HParser *discriminator;
    HArena *arena;

    DispatchBucket *buckets;
    size_t bucket_count;
} HDispatch;

// Wrapper node for dispatch sequence (discriminator + body ASTs)
typedef struct {
    int tag; // identifies this as a dispatch sequence node
    uint32_t bit_offset;
    const HParsedToken *left;  // discriminator AST
    const HParsedToken *right; // body AST
} DispatchSeqNode;

#define DISPATCH_SEQ_NODE_TAG 0xD15F4C

// Helper functions
static size_t next_pow2(size_t n) {
    if (n == 0)
        return 1;
    // Check for overflow: if p would overflow, return SIZE_MAX to signal error
    size_t p = 1;
    while (p < n) {
        if (p > SIZE_MAX / 2) // would overflow
            return SIZE_MAX;
        p <<= 1;
    }
    return p;
}

static size_t extract_opcode(HParseResult *result) {
    if (!result || !result->ast)
        return (size_t)-1;
    size_t opcode;
    switch (result->ast->token_type) {
    case (TT_BYTES):
        // TODO: Properly handle bytes token
        opcode = -1;
        break;
    case (TT_SINT):
        opcode = (size_t)(result->ast->token_data.sint);
        break;
    case (TT_UINT):
        opcode = (size_t)(result->ast->token_data.uint);
        break;
    case (TT_DOUBLE):
        opcode = (size_t)(result->ast->token_data.dbl);
        break;
    case (TT_FLOAT):
        opcode = (size_t)(result->ast->token_data.flt);
        break;
    default:
        opcode = -1;
    }
    return opcode;
}

HParser *extract_parser(HDispatch *d, size_t h, size_t mask, size_t opcode) {
    while (d->buckets[h].used) {
        if (d->buckets[h].opcode == opcode)
            return d->buckets[h].parser;
        h = (h + 1) & mask;
    }
    return NULL;
}

static HParseResult *parse_dispatch(void *env, HParseState *state) {
    HDispatch *d = (HDispatch *)env;

    // Parse discriminator
    HParseResult *disc_result = h_do_parse(d->discriminator, state);
    if (!disc_result) {
        fprintf(stderr, "Discriminator is NULL");
        return NULL;
    }

    // Extract opcode value from discriminator
    size_t opcode = extract_opcode(disc_result);

    // O(1) lookup
    size_t mask = d->bucket_count - 1;
    size_t h = (opcode ^ (opcode >> 16)) & mask;

    HParser *body = extract_parser(d, h, mask, opcode);
    if (!body) {
        fprintf(stderr, "Parser Extraction failed!\n");
        return NULL;
    }

    HParseResult *body_result = h_do_parse(body, state);
    if (!body_result) {
        fprintf(stderr, "Body result failed!\n");
        return NULL;
    }

    HArena *target_arena = d->arena;
    if (!target_arena) {
        fprintf(stderr, "Target Arena allocation failed!\n");
        return NULL;
    }
    // Dispatch-owned result to avoid mutating cached body_result
    HParseResult *dispatch_result = a_new(HParseResult, 1);
    if (!dispatch_result) {
        fprintf(stderr, "No Dispatch Result! \n");
        return NULL;
    }

    dispatch_result->bit_length = body_result->bit_length + disc_result->bit_length;
    dispatch_result->arena = target_arena;

    // Wrapper node that references child ASTs without mutating them
    DispatchSeqNode *seq = (DispatchSeqNode *)h_arena_malloc(target_arena, sizeof(DispatchSeqNode));
    if (!seq) {
        fprintf(stderr, "No Dispatch Seq! \n");
        return NULL;
    }

    seq->tag = DISPATCH_SEQ_NODE_TAG;
    seq->bit_offset = disc_result->ast ? disc_result->ast->bit_offset : 0; // sequence start
    seq->left = disc_result->ast;
    seq->right = body_result->ast;

    // Store wrapper as the AST for the dispatch result
    dispatch_result->ast = (const HParsedToken *)seq;

    return dispatch_result;
}

static bool dispatch_isValidRegular(void *env) {
    HDispatch *d = (HDispatch *)env;

    if (!d->discriminator->vtable->isValidRegular(d->discriminator->env))
        return false;
    for (size_t i = 0; i < d->bucket_count; ++i) {
        if (!d->buckets[i].used)
            continue;
        HParser *p = d->buckets[i].parser;
        if (!p->vtable->isValidRegular(p->env))
            return false;
    }
    return true;
}

static void desugar_dispatch(HAllocator *mm__, HCFStack *stk__, void *env) {
    HDispatch *d = (HDispatch *)env;
    // Note: dispatch inherently encodes state-dependent branching (on discriminator value).
    // A proper CF representation would need to encode which opcode maps to which parser,
    // but the current CFG infrastructure has limited support for value-dependent constraints.
    // For now, desugar as a simple choice to enable basic CF analysis, though CF backends
    // may not fully capture the dispatch semantics.
    HCFS_BEGIN_CHOICE() {
        for (size_t i = 0; i < d->bucket_count; ++i) {
            if (!d->buckets[i].used)
                continue;
            HCFS_BEGIN_SEQ() {
                HCFS_DESUGAR(d->discriminator);
                HCFS_DESUGAR(d->buckets[i].parser);
            }
            HCFS_END_SEQ();
        }
        HCFS_THIS_CHOICE->reshape = h_act_first;
    }
    HCFS_END_CHOICE();
}

static const HParserVtable dispatch_vt = {
    .parse = parse_dispatch,
    .isValidRegular = dispatch_isValidRegular,
    .isValidCF = h_false,
    // Dispatch involves runtime value-dependent branching (based on discriminator opcode).
    // CF grammars cannot soundly represent this constraint: which opcode maps to which
    // parser body is determined at parse time, not statically in the grammar.
    // Desugaring would create choice(seq(discriminator, parser0), seq(discriminator, parser1), ...)
    // but CF backends would see all combinations as valid, leading to unsound analysis.
    // Therefore, dispatch is not valid for CF analysis.
    .desugar = desugar_dispatch,
    .higher = true,
};

HParser *h_dispatch__s(HParser *discriminator, const OpcodeMap *map, size_t size) {
    if (map == NULL || size == 0) {
        fprintf(stderr, "Map is NULL!\n");
        return NULL;
    }
    HParser *ret = h_dispatch__m(&system_allocator, discriminator, map, size);
    return ret;
}

HParser *h_dispatch__m(HAllocator *mm__, HParser *discriminator, const OpcodeMap *map,
                       size_t size) {

    HDispatch *env = h_new(HDispatch, 1);
    if (!env) {
        fprintf(stderr, "No Env! \n");
        return NULL;
    }
    env->discriminator = discriminator;
    env->arena = h_new_arena(mm__, 0);

    // build hash table
    env->bucket_count = next_pow2(size * 2);
    if (env->bucket_count == SIZE_MAX || env->bucket_count < size) {
        // Overflow detected in next_pow2 or size multiplication
        fprintf(stderr, "Overflow detected in next_pow2 or size multiplication! \n");
        h_free(env);
        return NULL;
    }
    env->buckets = h_alloc(mm__, env->bucket_count * sizeof(*env->buckets));
    if (!env->buckets) {
        fprintf(stderr, "No Buckets!\n");
        h_free(env);
        return NULL;
    }
    memset(env->buckets, 0, env->bucket_count * sizeof(*env->buckets));

    size_t mask = env->bucket_count - 1;

    for (size_t i = 0; i < size; ++i) {
        uint32_t op = map[i].opcode;
        HParser *p = map[i].parser;
        size_t h = (op ^ (op >> 16)) & mask;
        size_t probe_count = 0;
        // Linear probing with cycle detection to prevent infinite loops
        while (env->buckets[h].used && probe_count < env->bucket_count) {
            h = (h + 1) & mask;
            probe_count++;
        }
        if (probe_count >= env->bucket_count) {
            // Hash table is full, cannot insert
            fprintf(stderr, "Hash table is full, cannot insert!\n");
            // Note: buckets memory will be freed when env is freed by h_free
            h_free(env);
            return NULL;
        }
        env->buckets[h].opcode = op;
        env->buckets[h].parser = p;
        env->buckets[h].used = true;
    }

    return h_new_parser(mm__, &dispatch_vt, env);
}