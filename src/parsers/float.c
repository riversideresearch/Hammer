/* Copyright (c) 2026 Riverside Research */
#include "parser_internal.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

struct float_env {
    int bits;
};

static HParsedToken *reshape_float(const HParseResult *p, void *user_data) {
    struct float_env *env = user_data;
    assert(p->ast);
    assert(p->ast->token_type == TT_SEQUENCE);

    HCountedArray *seq = p->ast->token_data.seq;
    uint64_t bits = 0;
    for (size_t i = 0; i < seq->used; ++i) {
        HParsedToken *t = seq->elements[i];
        assert(t->token_type == TT_UINT);
        bits = (bits << 8) | (t->token_data.uint & 0xFF);
    }

    HParsedToken *ret = h_arena_malloc(p->arena, sizeof(HParsedToken));
    ret->index = p->ast->index;
    ret->bit_offset = p->ast->bit_offset;
    ret->bit_length = p->bit_length;

    if (env->bits == 16) {
        uint16_t sign = (bits >> 15) & 0x1;
        uint16_t exponent = (bits >> 10) & 0x1F;
        uint16_t fraction = bits & 0x3FF;

        double significand;
        float flt;

        if (exponent == 31) {
            // 11111 = infinity
            if (fraction != 0) { // NaN
                flt = NAN;
            } else if (sign == 1) { // negative
                flt = -INFINITY;
            } else {
                flt = INFINITY;
            }
        } else if (exponent != 0) {
            significand = 1.0 + (fraction / (double)(1u << 10));
            flt = (float)(pow(-1, sign)) * (float)(significand * pow(2.0, exponent - 15));
        } else {
            if (fraction == 0) {
                if (sign == 1){
                    flt = -0.0f;
                } else{
                    flt = 0.0f;
                }
            } else { // No implicit 1
                significand = fraction / (double)(1u << 10);
                flt = ((int16_t)pow(-1, sign)) * significand * pow(2, -14);
            }
        }
        ret->token_type = TT_FLOAT;
        ret->token_data.flt = flt;
        return ret;
    }

    if (env->bits == 32) {
        uint32_t sign = (bits >> 31) & 0x1;
        uint32_t exponent = (bits >> 23) & 0xFF;
        uint32_t fraction = bits & 0x7FFFFF;

        double significand;
        float flt;
        if (exponent == 255) {
            // 11111111 = infinity
            if (fraction != 0) { // NaN
                flt = NAN;
            } else if (sign == 1) { // negative
                flt = -INFINITY;
            } else {
                flt = INFINITY;
            }
        } else if (exponent != 0) {
            significand = 1.0 + (fraction / (double)(1u << 23));
            flt = (float)(pow(-1, sign)) * (float)(significand * pow(2.0, exponent - 127));
        } else {
            if (fraction == 0) {
                if (sign == 1){
                    flt = -0.0f;
                } else{
                    flt = 0.0f;
                }
            } else {
                // No implicit 1
                significand = fraction / (double)(1u << 23);
                flt = ((int32_t)pow(-1, sign)) * significand * pow(2, -126);
            }
        }

        ret->token_type = TT_FLOAT;
        ret->token_data.flt = flt;
        return ret;
    }

    if (env->bits == 64) {
        uint64_t sign = (bits >> 63) & 0x1;
        uint64_t exponent = (bits >> 52) & 0x7FF;
        uint64_t fraction = bits & 0xFFFFFFFFFFFFF;

        double significand;
        double dbl;
        if (exponent == 2047) {
            // 11111111111 = infinity
            if (fraction != 0) { // NaN
                dbl = NAN;
            } else if (sign == 1) { // negative
                dbl = -INFINITY;
            } else {
                dbl = INFINITY;
            }
        } else if (exponent != 0) {
            significand = 1.0 + (fraction / (double)(1ULL << 52));
            dbl = (float)(pow(-1, sign)) * significand * pow(2.0, exponent - 1023);
        } else {
            if (fraction == 0) {
                if (sign == 1){
                    dbl = -0.0;
                } else{
                    dbl = 0.0;
                }
            } else { // No implicit 1
                significand = fraction / (double)(1ULL << 52);
                dbl = (float)(pow(-1, sign)) * significand * pow(2.0, -1022);
            }
        }
        ret->token_type = TT_DOUBLE;
        ret->token_data.dbl = dbl;
        return ret;
    }

    return NULL;
}

static HParseResult *parse_float(void *env_, HParseState *state) {
    struct float_env *env = env_;
    HParsedToken *result = a_new(HParsedToken, 1);
    double significand;

    if (env->bits == 16) {
        float flt;
        uint16_t bits = h_read_bits(&state->input_stream, 16, false);
        uint16_t sign = bits >> 15;              // 1 bit
        uint16_t exponent = (bits >> 10) & 0x1F; // 5 bits
        uint16_t fraction = bits & 0x3FF;        // 10 bits

        if (exponent == 31) {
            // 11111 = infinity
            if (fraction != 0) { // NaN
                flt = NAN;
            } else if (sign == 1) { // negative
                flt = -INFINITY;
            } else {
                flt = INFINITY;
            }
        } else if (exponent != 0) {
            // Add leading 1
            significand = 1.0 + (fraction / (double)(1u << 10));
            flt = ((int16_t)pow(-1, sign)) * significand * pow(2, (exponent - 15));
        } else {
            if (fraction == 0) {
                if (sign == 1){
                    flt = -0.0f;
                } else{
                    flt = 0.0f;
                }
            } else { // No implicit 1
                significand = fraction / (double)(1u << 10);
                flt = ((int16_t)pow(-1, sign)) * significand * pow(2, -14);
            }
        }
        result->token_type = TT_FLOAT;
        result->token_data.flt = flt;
        return make_result(state->arena, result);
    } else if (env->bits == 32) {
        float flt;
        uint32_t bits = h_read_bits(&state->input_stream, 32, false);
        uint32_t sign = bits >> 31;              // 1 bit
        uint32_t exponent = (bits >> 23) & 0xFF; // 8 bits
        uint32_t fraction = bits & 0x7FFFFF;     // 23 bits

        if (exponent == 255) {
            // 11111111 = infinity
            if (fraction != 0) { // NaN
                flt = NAN;
            } else if (sign == 1) { // negative
                flt = -INFINITY;
            } else {
                flt = INFINITY;
            }
        } else if (exponent != 0) {
            // Add leading 1
            significand = 1.0 + (fraction / (double)(1u << 23));
            flt = ((int32_t)pow(-1, sign)) * significand * pow(2, (exponent - 127));
        } else {
            if (fraction == 0) {
                if (sign == 1){
                    flt = -0.0f;
                } else{
                    flt = 0.0f;
                }
            } else {
                // No implicit 1
                significand = fraction / (double)(1u << 23);
                flt = ((int32_t)pow(-1, sign)) * significand * pow(2, -126);
            }
        }

        result->token_type = TT_FLOAT;
        result->token_data.flt = flt;
        return make_result(state->arena, result);
    } else if (env->bits == 64) // double is 64 bits,
    {
        double dbl;
        uint64_t bits = h_read_bits(&state->input_stream, 64, false);
        uint64_t sign = bits >> 63;                 // 1 bit
        uint64_t exponent = (bits >> 52) & 0x7FF;   // 11 bits
        uint64_t fraction = bits & 0xFFFFFFFFFFFFF; // 52 bits

        if (exponent == 2047) {
            // 11111111111 = infinity
            if (fraction != 0) { // NaN
                dbl = NAN;
            } else if (sign == 1) { // negative
                dbl = -INFINITY;
            } else {
                dbl = INFINITY;
            }
        } else if (exponent != 0) {
            // Add leading 1
            significand = 1.0 + (fraction / (double)(1ULL << 52));
            dbl = ((int64_t)pow(-1, sign)) * significand * pow(2, (exponent - 1023));
        } else {
            if (fraction == 0) {
                if (sign == 1){
                    dbl = -0.0;
                } else{
                    dbl = 0.0;
                }
            } else { // No implicit 1
                significand = fraction / (double)(1ULL << 52);
                dbl = ((int64_t)pow(-1, sign)) * significand * pow(2, - 1022);
            }
        }

        result->token_type = TT_DOUBLE;
        result->token_data.dbl = dbl;
        return make_result(state->arena, result);
    } else
        return NULL;
}

static void desugar_float(HAllocator *mm__, HCFStack *stk__, void *env) {
    // not fixed yet
    struct float_env *env_ = env;

    HCharset match_all = new_charset(mm__);
    for (int i = 0; i < 256; i++)
        charset_set(match_all, i, 1);

    HCFS_BEGIN_CHOICE() {
        HCFS_BEGIN_SEQ() {
            size_t n = env_->bits / 8;
            for (size_t i = 0; i < n; ++i) {
                HCFS_ADD_CHARSET(match_all);
            }
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

HParser *h_floating_point__m(HAllocator *mm__, int bits) {
    struct float_env *env = h_new(struct float_env, 1);
    env->bits = bits;
    return h_new_parser(mm__, &float_vt, env);
}

HParser *h_float16(void) { return h_floating_point__m(&system_allocator, 16); }
HParser *h_float(void) { return h_floating_point__m(&system_allocator, 32); }
HParser *h_double(void) { return h_floating_point__m(&system_allocator, 64); }
