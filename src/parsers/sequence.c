/* Copyright (c) 2026 Riverside Research */
#include "parser_internal.h"

#include <assert.h>
#include <stdarg.h>

static HParseResult *parse_sequence(void *env, HParseState *state) {
    HSequence *s = (HSequence *)env;
    HCountedArray *seq = h_carray_new_sized(state->arena, (s->len > 0) ? s->len : 4);
    for (size_t i = 0; i < s->len; ++i) {
        HParseResult *tmp = h_do_parse(s->p_array[i], state);
        // if the interim parse fails, the whole thing fails
        if (NULL == tmp) {
            return NULL;
        } else {
            if (tmp->ast)
                h_carray_append(seq, (void *)tmp->ast);
        }
    }
    HParsedToken *tok = a_new(HParsedToken, 1);
    tok->token_type = TT_SEQUENCE;
    tok->token_data.seq = seq;
    tok->index = 0;
    tok->bit_offset = 0;
    tok->bit_length = 0;
    return make_result(state->arena, tok);
}

static bool sequence_isValidRegular(void *env) {
    HSequence *s = (HSequence *)env;
    for (size_t i = 0; i < s->len; ++i) {
        if (!s->p_array[i]->vtable->isValidRegular(s->p_array[i]->env))
            return false;
    }
    return true;
}

static bool sequence_isValidCF(void *env) {
    HSequence *s = (HSequence *)env;
    for (size_t i = 0; i < s->len; ++i) {
        if (!s->p_array[i]->vtable->isValidCF(s->p_array[i]->env))
            return false;
    }
    return true;
}

static HParsedToken *reshape_sequence(const HParseResult *p, void *user_data) {
    assert(p->ast);
    assert(p->ast->token_type == TT_SEQUENCE);

    HCountedArray *seq = h_carray_new(p->arena);

    // drop all elements that are NULL
    for (size_t i = 0; i < p->ast->token_data.seq->used; i++) {
        if (p->ast->token_data.seq->elements[i] != NULL)
            h_carray_append(seq, p->ast->token_data.seq->elements[i]);
    }

    HParsedToken *res = a_new_(p->arena, HParsedToken, 1);
    res->token_type = TT_SEQUENCE;
    res->token_data.seq = seq;
    res->index = p->ast->index;
    res->bit_offset = p->ast->bit_offset;
    res->bit_length = p->bit_length;

    return res;
}

static void desugar_sequence(HAllocator *mm__, HCFStack *stk__, void *env) {
    HSequence *s = (HSequence *)env;
    HCFS_BEGIN_CHOICE() {
        HCFS_BEGIN_SEQ() {
            for (size_t i = 0; i < s->len; i++)
                HCFS_DESUGAR(s->p_array[i]);
        }
        HCFS_END_SEQ();
        HCFS_THIS_CHOICE->reshape = reshape_sequence;
        HCFS_THIS_CHOICE->user_data = NULL;
    }
    HCFS_END_CHOICE();
}

static bool sequence_ctrvm(HRVMProg *prog, void *env) {
    HSequence *s = (HSequence *)env;
    h_rvm_insert_insn(prog, RVM_PUSH, 0);
    for (size_t i = 0; i < s->len; ++i) {
        if (!h_compile_regex(prog, s->p_array[i]))
            return false;
    }
    h_rvm_insert_insn(prog, RVM_ACTION,
                      h_rvm_create_action(prog, h_svm_action_make_sequence, NULL));
    return true;
}

static const HParserVtable sequence_vt = {
    .name = "h_sequence",
    .parse = parse_sequence,
    .isValidRegular = sequence_isValidRegular,
    .isValidCF = sequence_isValidCF,
    .compile_to_rvm = sequence_ctrvm,
    .desugar = desugar_sequence,
    .higher = true,
};

typedef struct HRewriteSequence {
    HSequence sequence;
    HParser **owned_parsers;
    size_t owned_len;
} HRewriteSequence;

static void h_free_rewrite_sequence_env(HAllocator *mm__, void *env) {
    HRewriteSequence *rewrite = env;
    if (!rewrite)
        return;

    for (size_t i = 0; i < rewrite->owned_len; ++i)
        h_parser_free__m(mm__, rewrite->owned_parsers[i]);

    mm__->free(mm__, rewrite->owned_parsers);
    mm__->free(mm__, rewrite->sequence.p_array);
    mm__->free(mm__, rewrite);
}

HParser *h_sequence(HParser *p, ...) {
    va_list ap;
    va_start(ap, p);
    HParser *ret = h_sequence__mv(&system_allocator, p, ap);
    va_end(ap);
    return ret;
}

HParser *h_sequence__m(HAllocator *mm__, HParser *p, ...) {
    va_list ap;
    va_start(ap, p);
    HParser *ret = h_sequence__mv(mm__, p, ap);
    va_end(ap);
    return ret;
}

HParser *h_sequence__v(HParser *p, va_list ap) { return h_sequence__mv(&system_allocator, p, ap); }

HParser *h_sequence__mv(HAllocator *mm__, HParser *p, va_list ap_) {
    HSequence *s = h_new(HSequence, 1);
    s->len = 0;
    s->p_array = NULL;
    if (p) {
        // non-empty sequence
        const HParser *arg;
        size_t len = 0;
        va_list ap;

        va_copy(ap, ap_);
        do {
            len++;
            arg = va_arg(ap, HParser *);
        } while (arg);
        va_end(ap);
        s->p_array = h_new(HParser *, len);

        va_copy(ap, ap_);
        s->p_array[0] = p;
        for (size_t i = 1; i < len; i++) {
            s->p_array[i] = va_arg(ap, HParser *);
        }
        while (arg)
            ;
        va_end(ap);

        s->len = len;

        return h_new_parser_with_free(mm__, &sequence_vt, s, h_free_seq_env);
    }
    return h_new_parser_with_free(mm__, &sequence_vt, s, h_free_seq_env);
}

HParser *h_sequence__a(void *args[]) { return h_sequence__ma(&system_allocator, args); }

HParser *h_sequence__ma(HAllocator *mm__, void *args[]) {
    size_t len = (size_t)-1; // because do...while
    const HParser *arg;

    do {
        arg = ((HParser **)args)[++len];
    } while (arg);

    HSequence *s = h_new(HSequence, 1);
    s->p_array = h_new(HParser *, len);

    for (size_t i = 0; i < len; i++) {
        s->p_array[i] = ((HParser **)args)[i];
    }

    s->len = len;
    return h_new_parser_with_free(mm__, &sequence_vt, s, h_free_seq_env);
}

HParser *h_drop_from_(HParser *p, ...) {
    assert_message(p->vtable == &sequence_vt, "drop_from requires a sequence parser");
    va_list ap;
    va_start(ap, p);
    HParser *ret = h_drop_from___mv(&system_allocator, p, ap);
    va_end(ap);
    return ret;
}

HParser *h_drop_from___m(HAllocator *mm__, HParser *p, ...) {
    if (!p)
        return NULL;
    assert_message(p->vtable == &sequence_vt, "drop_from requires a sequence parser");
    va_list ap;
    va_start(ap, p);
    HParser *ret = h_drop_from___mv(mm__, p, ap);
    va_end(ap);
    return ret;
}

HParser *h_drop_from___v(HParser *p, va_list ap) {
    assert_message(p->vtable == &sequence_vt, "drop_from requires a sequence parser");
    return h_drop_from___mv(&system_allocator, p, ap);
}

static void validate_drop_indices(const HSequence *s, const size_t *indices, size_t count) {
    HAMMER_ASSERT(count <= s->len);
    for (size_t i = 0; i < count; ++i) {
        HAMMER_ASSERT(indices[i] < s->len);
        for (size_t j = 0; j < i; ++j)
            HAMMER_ASSERT(indices[j] != indices[i]);
    }
}

static bool drop_index_requested(const size_t *indices, size_t count, size_t index) {
    for (size_t i = 0; i < count; ++i) {
        if (indices[i] == index)
            return true;
    }
    return false;
}

static HParser *drop_from_indices(HAllocator *mm__, HSequence *s, const size_t *indices,
                                  size_t count) {
    validate_drop_indices(s, indices, count);

    HRewriteSequence *rewrite = h_new(HRewriteSequence, 1);
    rewrite->sequence.p_array = h_new(HParser *, s->len);
    rewrite->sequence.len = s->len;
    rewrite->owned_parsers = count ? h_new(HParser *, count) : NULL;
    rewrite->owned_len = 0;
    for (size_t i = 0; i < s->len; ++i) {
        if (drop_index_requested(indices, count, i)) {
            HParser *ignored = h_ignore__m(mm__, s->p_array[i]);
            rewrite->sequence.p_array[i] = ignored;
            rewrite->owned_parsers[rewrite->owned_len++] = ignored;
        } else {
            rewrite->sequence.p_array[i] = s->p_array[i];
        }
    }

    return h_new_parser_with_free(mm__, &sequence_vt, rewrite, h_free_rewrite_sequence_env);
}

HParser *h_drop_from___mv(HAllocator *mm__, HParser *p, va_list ap) {
    /* Saying `h_drop_from(h_sequence(a, b, c, d, e, NULL), 0, 4, -1)` is functionally
     * equivalent to `h_sequence(h_ignore(a), b, c, d, h_ignore(e), NULL)`. Thus, this
     * term rewrites itself, becoming an h_sequence where some parsers are ignored.
     */
    if (!p)
        return NULL;
    HSequence *s = (HSequence *)(p->env);
    size_t count = 0;
    int arg;

    va_list count_ap;
    va_copy(count_ap, ap);
    for (arg = va_arg(count_ap, int); arg >= 0; arg = va_arg(count_ap, int)) {
        HAMMER_ASSERT((size_t)arg < s->len);
        HAMMER_ASSERT(count < s->len);
        count++;
    }
    va_end(count_ap);

    size_t *indices = count ? h_new(size_t, count) : NULL;
    size_t index = 0;
    if (count > 0) {
        va_list index_ap;
        va_copy(index_ap, ap);
        for (arg = va_arg(index_ap, int); arg >= 0; arg = va_arg(index_ap, int))
            indices[index++] = (size_t)arg;
        va_end(index_ap);
    }

    HParser *rewrite = drop_from_indices(mm__, s, indices, count);

    if (indices)
        h_free(indices);
    return rewrite;
}

HParser *h_drop_from___a(void *args[]) { return h_drop_from___ma(&system_allocator, args); }

HParser *h_drop_from___ma(HAllocator *mm__, void *args[]) {
    HParser *p = (HParser *)(args[0]);
    assert_message(p->vtable == &sequence_vt, "drop_from requires a sequence parser");
    HSequence *s = (HSequence *)(p->env);
    size_t count = 0;
    const int *arg_indices = args[1];
    for (const int *arg = arg_indices; *arg >= 0; ++arg) {
        HAMMER_ASSERT((size_t)*arg < s->len);
        HAMMER_ASSERT(count < s->len);
        ++count;
    }

    size_t *indices = count ? h_new(size_t, count) : NULL;
    for (size_t i = 0; i < count; ++i)
        indices[i] = (size_t)arg_indices[i];

    HParser *rewrite = drop_from_indices(mm__, s, indices, count);
    if (indices)
        h_free(indices);
    return rewrite;
}
