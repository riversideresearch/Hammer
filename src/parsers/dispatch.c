/* Copyright (c) 2026 Riverside Research */
#include "parser_internal.h"

#include <stdarg.h>

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
    const OpcodeMap *map;
    size_t size;

    DispatchBucket *buckets;
    size_t bucket_count;
} HDispatch;

// Helper functions
static size_t next_pow2(size_t n) {
    size_t p = 1;
    while (p < n)
        p <<= 1;
    return p;
}

static size_t extract_opcode(HParseResult *result) {
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
    if (!body)
        return NULL;

    HParseResult *body_result = h_do_parse(body, state);
    if (!body_result) {
        h_parse_result_free(body_result);
        return NULL;
    }
    body_result->bit_length += disc_result->bit_length; // merge bit length

    return body_result;
}

static bool dispatch_isValidRegular(void *env) {
    HDispatch *d = (HDispatch *)env;

    if (!d->discriminator->vtable->isValidRegular(d->discriminator->env))
        return false;
    for (size_t i = 0; i < d->size; ++i) {
        if (!d->map[i].parser->vtable->isValidRegular(d->map[i].parser->env))
            return false;
    }
    return true;
}

static bool dispatch_isValidCF(void *env) {
    HDispatch *d = (HDispatch *)env;

    if (!d->discriminator->vtable->isValidCF(d->discriminator->env))
        return false;
    for (size_t i = 0; i < d->size; ++i) {
        if (!d->map[i].parser->vtable->isValidCF(d->map[i].parser->env))
            return false;
    }
    return true;
}

// Predicate to validate that discriminator result matches expected opcode
static bool check_dispatch_opcode(HParseResult *p, void *user_data) {
    if (!p || !p->ast)
        return false;
    uint32_t expected_opcode = (uint32_t)(uintptr_t)user_data;
    size_t actual_opcode = extract_opcode(p);
    return actual_opcode == expected_opcode;
}

static void desugar_dispatch(HAllocator *mm__, HCFStack *stk__, void *env) {
    HDispatch *d = (HDispatch *)env;
    HCFS_BEGIN_CHOICE() {
        for (size_t i = 0; i < d->bucket_count; ++i) {
            if (!d->buckets[i].used)
                continue;
            HCFS_BEGIN_SEQ() {
                HCFS_DESUGAR(d->discriminator);
                HCFS_DESUGAR(d->buckets[i].parser);
            }
            HCFS_END_SEQ();
            // Encode the opcode constraint via predicate so grammar/backends
            // can understand which (opcode, parser) combinations are valid
            HCFS_THIS_CHOICE->pred = check_dispatch_opcode;
            HCFS_THIS_CHOICE->user_data = (void *)(uintptr_t)d->buckets[i].opcode;
        }
        HCFS_THIS_CHOICE->reshape = h_act_first;
    }
    HCFS_END_CHOICE();
}

static const HParserVtable dispatch_vt = {
    .parse = parse_dispatch,
    .isValidRegular = dispatch_isValidRegular,
    .isValidCF = dispatch_isValidCF,
    .desugar = desugar_dispatch,
    .higher = true,
};

HParser *h_dispatch__s(HParser *discriminator, const OpcodeMap *map, size_t size) {
    if (map == NULL || size == 0) {
        return NULL;
    }
    HParser *ret = h_dispatch__m(&system_allocator, discriminator, map, size);
    return ret;
}

HParser *h_dispatch__m(HAllocator *mm__, HParser *discriminator, const OpcodeMap *map,
                       size_t size) {
    HDispatch *env = h_new(HDispatch, 1);
    env->discriminator = discriminator;
    env->map = map;
    env->size = size;

    // build hash table
    env->bucket_count = next_pow2(size * 2);
    env->buckets = h_alloc(mm__, env->bucket_count * sizeof(*env->buckets));
    memset(env->buckets, 0, env->bucket_count * sizeof(*env->buckets));

    size_t mask = env->bucket_count - 1;
    for (size_t i = 0; i < size; ++i) {
        uint32_t op = map[i].opcode;
        HParser *p = map[i].parser;
        size_t h = (op ^ (op >> 16)) & mask;
        while (env->buckets[h].used)
            h = (h + 1) & mask;
        env->buckets[h].opcode = op;
        env->buckets[h].parser = p;
        env->buckets[h].used = true;
    }

    return h_new_parser(mm__, &dispatch_vt, env);
}