#include "../src/hammer.h"
#include "test_suite.h"

#include <glib.h>
#include <string.h>
#include "parser_internal.h"
#include <float.h>
#include <stdio.h>
// #include <math.h>
/* math.h*/
double power(double base, int32_t exp)
{
    double result = 1.0f;
    int32_t e = exp;

    if (e < 0) {
        base = 1.0f / base;
        e = -e;
    }

    while (e > 0) {
        if (e & 1)
            result *= base;
        base *= base;
        e >>= 1;
    }

    return result;
}

/* float.c */
#include "parser_internal.h"
#include <float.h>
#include <stdio.h>

struct float_env {
    int bits;
    int radix;
    //int p; //precision
    int digits;
};

static HParseResult *parse_float(void *env_, HParseState *state) {
    struct float_env *env = env_;
    HParsedToken *result = a_new(HParsedToken, 1);

    if(env->bits == 16){ 
       // ... 
    }
    else if(env->bits == 32) { 
        // radix is either 2 (binary) or 10 (decimal)
        if(env->radix == 2) // binary
        {
            float flt;
            bool negative;
            uint32_t bits = h_read_bits(&state->input_stream, 32, false);
            uint32_t sign = bits >> 31;
            uint32_t exponent = (bits >> 23) & 0xFF;
            uint32_t fraction = bits & 0x7FFFFF;   // 23 bits

            double significand;

            if (exponent != 0) {
                // Add leading 1
                significand = 1.0 + (fraction / (double)(1u << 23));
            } else {
                // No implicit 1
                significand = fraction / (double)(1u << 23);
            }
            
            flt = ((uint32_t)power(-1, sign))*significand*power(2,(exponent-127));
            result->token_type = TT_FLOAT;
            result->token_data.flt = flt ;
        }
    }
    else if(env->bits == 64) // double is 64 bits, 
    {

    }
    else if(env->bits == 80) // x87 long double extended precision
    {
        // 1 sign bit
        // 15 bit exponent
        // 1 bit explicit integer bit
        // 63 bit fraction

    }
    else if(env->bits == 128) // long double quadruple precision. IEEE‑754 quad
    {
        // 
    }
    else // unknown type
    {

    }
    return make_result(state->arena, result);
}

static bool float_ctrvm(HRVMProg *prog, void *env) {
    // ...
    return false;
}

static void desugar_float(HAllocator *mm__, HCFStack *stk__, void *env) {
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
        HCFS_THIS_CHOICE->reshape = h_act_first;
        HCFS_THIS_CHOICE->user_data = NULL;
    }
    HCFS_END_CHOICE();
}

static const HParserVtable float_vt = {
    .parse = parse_float,
    .desugar = desugar_float,
    .isValidRegular = h_true, 
    .isValidCF = h_true,    
    .compile_to_rvm = float_ctrvm,
    .higher = false,
};

HParser *h_float__m(HAllocator *mm__, int bits, int radix, int digits) {
    struct float_env *env = h_new(struct float_env, 1);
    env->bits = bits;
    env->radix = radix;
    env->digits = digits;
    return h_new_parser(mm__, &float_vt, env);
}

HParser *h_float(int bits, int radix, int digits) {
     return h_float__m(&system_allocator, bits, radix, digits); 
}

HParser *h_float32(void) {
    return h_float(32, 2, 23);
}
HParser *h_float32__m(HAllocator *mm__){
    return h_float__m(&system_allocator, 32, 2, 23);
}

static void test_float(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    const HParser *p = h_float32();
    const uint8_t input[4] = { 0x40, 0x49, 0x0F, 0xDB }; // pi
    HParseResult *result = h_parse(p, input, 4);
    float pi = result->ast->token_data.flt;
    fprintf(stderr, "pi is approx: %f\n", pi);
    g_check_parse_match(p, be,input, 4, "f0x1.921fb6p+1"); 
}

void register_floating_point_parser_tests(void) {
    g_test_add_data_func("/core/parser/float/float", GINT_TO_POINTER(PB_PACKRAT), test_float);
}
