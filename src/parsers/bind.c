/* Copyright (c) 2026 Riverside Research */
#include "parser_internal.h"

typedef struct {
    const HParser *p;
    HContinuation k;
    void *env;
} BindEnv;

static HParseResult *parse_bind(void *be_, HParseState *state) {
    BindEnv *be = be_;

    HParseResult *res = h_do_parse(be->p, state);
    if (!res)
        return NULL;

    // use the system allocator so frees actually work.
    HAllocator *mm__ = &system_allocator;

    HParser *kx = be->k(mm__, res->ast, be->env);
    if (!kx) {
        return NULL;
    }

    HParseResult *res2 = h_do_parse(kx, state);
    if (res2)
        res2->bit_length = 0; // recalculate

    return res2;
}

static const HParserVtable bind_vt = {
    .parse = parse_bind,
    .isValidRegular = h_false,
    .isValidCF = h_false,
    .higher = true,
};

HParser *h_bind(const HParser *p, HContinuation k, void *env) {
    return h_bind__m(&system_allocator, p, k, env);
}

HParser *h_bind__m(HAllocator *mm__, const HParser *p, HContinuation k, void *env) {
    BindEnv *be = h_new(BindEnv, 1);

    be->p = p;
    be->k = k;
    be->env = env;

    return h_new_parser(mm__, &bind_vt, be);
}
