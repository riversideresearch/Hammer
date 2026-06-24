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
    HParser *discriminator;
    HParser **parsers;
    size_t count;
} HDispatch;

// Helper function

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

static HParseResult *parse_dispatch(void *env, HParseState *state) {
    HDispatch *d = (HDispatch *)env;

    // Parse discriminator
    HParseResult *disc_result = h_do_parse(d->discriminator, state);
    if (!disc_result) {
        //fprintf(stderr, "Discriminator is NULL");
        return NULL;
    }

    // Extract opcode value from discriminator
    size_t opcode = extract_opcode(disc_result);

    // Validate range
    if (opcode >= d->count || !d->parsers[opcode] || opcode == (size_t)-1) {
        //fprintf(stderr, "Invalid discriminator or no parser for value: %lu", opcode);
        return NULL; // Invalid discriminator or no parser for this value
    }

    // Dispatch to specific parser
    return h_do_parse(d->parsers[opcode], state);
}

static bool dispatch_isValidRegular(void *env) {
    HDispatch *s = (HDispatch *)env;
    for (size_t i = 0; i < s->count; ++i) {
        if (!s->parsers[i]->vtable->isValidRegular(s->parsers[i]->env))
            return false;
    }
    return true;
}

static bool dispatch_isValidCF(void *env) {
    HDispatch *s = (HDispatch *)env;
    for (size_t i = 0; i < s->count; ++i) {
        if (!s->parsers[i]->vtable->isValidCF(s->parsers[i]->env))
            return false;
    }
    return true;
}

static void desugar_dispatch(HAllocator *mm__, HCFStack *stk__, void *env) {
    HDispatch *s = (HDispatch *)env;
    HCFS_BEGIN_CHOICE() {
        for (size_t i = 0; i < s->count; i++) {
            HCFS_BEGIN_SEQ() { HCFS_DESUGAR(s->parsers[i]); }
            HCFS_END_SEQ();
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

HParser *h_dispatch(HParser *discriminator, HParser **parsers, size_t count) {
    HParser *ret = h_dispatch__m(&system_allocator, discriminator, parsers, count);
    return ret;
}

HParser *h_dispatch__m(HAllocator *mm__, HParser *discriminator, HParser **parsers, size_t count) {
    HDispatch *env = h_new(HDispatch, 1);
    env->discriminator = discriminator;
    env->parsers = parsers;
    env->count = count;

    return h_new_parser(mm__, &dispatch_vt, env);
}

// TODO: h_dispatch_static