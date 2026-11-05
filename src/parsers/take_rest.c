/* Copyright (c) 2026 Riverside Research */
#include "parser_internal.h"

/*
 * Capture the remaining final, byte-aligned input as one zero-copy token.
 *
 * This is a terminal parser: it cannot decide how many bytes
 * belong to a non-final streaming chunk, and it cannot represent a partial
 * byte as HBytes without copying or changing its meaning.
 */
static HParseResult *parse_take_rest(void *env, HParseState *state) {
    HInputStream *stream = &state->input_stream;
    HParsedToken *result;
    size_t length;

    (void)env;

    if (!stream->last_chunk) {
        stream->overrun = true;
        return NULL;
    }

    if (stream->bit_offset != 0 || stream->margin != 0)
        return NULL;

    length = stream->length - stream->index;
    result = a_new(HParsedToken, 1);
    result->token_type = TT_BYTES;
    result->token_data.bytes.token = stream->input + stream->index;
    result->token_data.bytes.len = length;
    result->index = stream->index;
    result->bit_length = 0;
    result->bit_offset = 0;

    stream->index = stream->length;
    return make_result(state->arena, result);
}

/*
 * Compile an anchored match-all capture:
 *
 *     PUSH
 * loop:
 *     FORK consume
 *     EOF
 *     CAPTURE
 *     GOTO done
 * consume:
 *     MATCH any-byte
 *     STEP
 *     GOTO loop
 * done:
 *
 * The EOF branch is evaluated at every input position, but can succeed only
 * at the final byte. The RVM capture is a zero-copy input span.
 */
static bool take_rest_ctrvm(HRVMProg *prog, void *env) {
    uint16_t consume;
    uint16_t done;
    uint16_t fork;
    uint16_t loop;
    uint16_t skip_consume;

    (void)env;

    h_rvm_insert_insn(prog, RVM_PUSH, 0);
    loop = h_rvm_get_ip(prog);
    fork = h_rvm_insert_insn(prog, RVM_FORK, 0);
    h_rvm_insert_insn(prog, RVM_EOF, 0);
    h_rvm_insert_insn(prog, RVM_CAPTURE, 0);
    skip_consume = h_rvm_insert_insn(prog, RVM_GOTO, 0);

    consume = h_rvm_get_ip(prog);
    h_rvm_insert_insn(prog, RVM_MATCH, 0xff00);
    h_rvm_insert_insn(prog, RVM_STEP, 0);
    h_rvm_insert_insn(prog, RVM_GOTO, loop);

    done = h_rvm_get_ip(prog);
    h_rvm_patch_arg(prog, fork, consume);
    h_rvm_patch_arg(prog, skip_consume, done);
    return true;
}

static const HParserVtable take_rest_vt = {
    .name = "h_take_rest",
    .parse = parse_take_rest,
    .isValidRegular = h_true,
    .isValidCF = h_false,
    .compile_to_rvm = take_rest_ctrvm,
    .higher = false,
};

HParser *h_take_rest(void) { return h_take_rest__m(&system_allocator); }

HParser *h_take_rest__m(HAllocator *mm__) {
    return h_new_parser_with_free(mm__, &take_rest_vt, NULL, h_no_free_env);
}
