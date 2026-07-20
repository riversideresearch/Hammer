/* Copyright (c) 2026 Riverside Research */
#include "parser_internal.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <float.h>

#if FLT_RADIX != 2 || FLT_MANT_DIG != 24 || FLT_MAX_EXP != 128
#error "float parser requires IEEE-754 binary32 float"
#endif

#if DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024
#error "double parser requires IEEE-754 binary64 double"
#endif

struct float_env {
    int bit_len;
};

/*
 * These parsers decode IEEE-754 interchange formats.  Copying the completed
 * bit pattern avoids libm entirely and preserves special values exactly.
 */
static float float32_from_bits(uint32_t raw_bits) {
    float value;
    assert_message(sizeof(float) == 4, "binary32 requires 32-bit float");
    memcpy(&value, &raw_bits, 4);
    return value;
}

static double float64_from_bits(uint64_t raw_bits) {
    double value;
    assert_message(sizeof(double) == 8, "binary64 requires 64-bit double");
    memcpy(&value, &raw_bits, 8);
    return value;
}

static float float16_from_bits(uint16_t raw_bits) {
    const uint32_t sign = (uint32_t)(raw_bits & UINT16_C(0x8000)) << 16;
    uint32_t exponent = (raw_bits >> 10) & UINT16_C(0x001f);
    uint32_t fraction = raw_bits & UINT16_C(0x03ff);
    uint32_t float_bits;

    if (exponent == 0) {
        if (fraction == 0) {
            /* Preserve both positive and negative zero. */
            float_bits = sign;
        } else {
            /* Normalize a binary16 subnormal for its binary32 representation. */
            int normalized_exponent = -14;
            while ((fraction & UINT32_C(0x0400)) == 0) {
                fraction <<= 1;
                --normalized_exponent;
            }
            fraction &= UINT32_C(0x03ff);
            float_bits = sign | ((uint32_t)(normalized_exponent + 127) << 23) | (fraction << 13);
        }
    } else if (exponent == UINT16_C(0x001f)) {
        /* Infinity or NaN; retain the payload and sign. */
        float_bits = sign | UINT32_C(0x7f800000) | (fraction << 13);
    } else {
        float_bits = sign | ((exponent + 112) << 23) | (fraction << 13);
    }

    return float32_from_bits(float_bits);
}

static HParsedToken *make_float_token(HArena *arena, const HParsedToken *source, int bit_len,
                                      uint64_t raw_bits) {
    HParsedToken *result = a_new_(arena, HParsedToken, 1);
    if (!result)
        return NULL;

    result->index = source->index;
    result->bit_offset = source->bit_offset;
    result->bit_length = source->bit_length;

    switch (bit_len) {
    case 16:
        result->token_type = TT_FLOAT;
        result->token_data.flt = float16_from_bits((uint16_t)raw_bits);
        return result;
    case 32:
        result->token_type = TT_FLOAT;
        result->token_data.flt = float32_from_bits((uint32_t)raw_bits);
        return result;
    case 64:
        result->token_type = TT_DOUBLE;
        result->token_data.dbl = float64_from_bits(raw_bits);
        return result;
    default:
        return NULL;
    }
}

static HParsedToken *reshape_float(const HParseResult *p, void *user_data) {
    struct float_env *env = user_data;
    assert(p->ast);
    assert(p->ast->token_type == TT_SEQUENCE);

    HCountedArray *seq = p->ast->token_data.seq;
    uint64_t bits = 0;
    for (size_t i = 0; i < seq->used; ++i) {
        HParsedToken *token = seq->elements[i];
        assert(token->token_type == TT_UINT);
        bits = (bits << 8) | (token->token_data.uint & UINT64_C(0xff));
    }

    HParsedToken source = {
        .index = p->ast->index,
        .bit_offset = p->ast->bit_offset,
        .bit_length = p->bit_length,
    };
    return make_float_token(p->arena, &source, env->bit_len, bits);
}

static HParseResult *parse_float(void *env_, HParseState *state) {
    struct float_env *env = env_;
    const size_t start_index = state->input_stream.index;
    const char start_bit_offset = state->input_stream.bit_offset;
    uint64_t raw_bits;

    switch (env->bit_len) {
    case 16:
        raw_bits = h_read_bits(&state->input_stream, 16, false);
        break;
    case 32:
        raw_bits = h_read_bits(&state->input_stream, 32, false);
        break;
    case 64:
        raw_bits = h_read_bits(&state->input_stream, 64, false);
        break;
    default:
        return NULL;
    }

    HParsedToken source = {
        .index = start_index,
        .bit_offset = start_bit_offset,
        .bit_length = (size_t)env->bit_len,
    };
    HParsedToken *result = make_float_token(state->arena, &source, env->bit_len, raw_bits);
    if (!result)
        return NULL;
    return make_result(state->arena, result);
}

static void desugar_float(HAllocator *mm__, HCFStack *stk__, void *env) {
    struct float_env *env_ = env;

    HCharset match_all = new_charset(mm__);
    for (int i = 0; i < 256; i++)
        charset_set(match_all, i, 1);

    HCFS_BEGIN_CHOICE() {
        HCFS_BEGIN_SEQ() {
            size_t n = env_->bit_len / 8;
            for (size_t i = 0; i < n; ++i)
                HCFS_ADD_CHARSET(match_all);
        }
        HCFS_END_SEQ();
        HCFS_THIS_CHOICE->reshape = reshape_float;
        HCFS_THIS_CHOICE->user_data = env_;
    }
    HCFS_END_CHOICE();
}

static const HParserVtable float_vt = {
    .parse = parse_float,
    .desugar = desugar_float,
    .isValidRegular = h_true,
    .isValidCF = h_true,
    .higher = false,
};

HParser *h_floating_point__m(HAllocator *mm__, int bit_len) {
    struct float_env *env = h_new(struct float_env, 1);
    env->bit_len = bit_len;
    return h_new_parser(mm__, &float_vt, env);
}

HParser *h_float16(void) { return h_floating_point__m(&system_allocator, 16); }
HParser *h_float(void) { return h_floating_point__m(&system_allocator, 32); }
HParser *h_double(void) { return h_floating_point__m(&system_allocator, 64); }
