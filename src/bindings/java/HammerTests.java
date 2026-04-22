import com.riversideresearch.hammer.*;

/**
 * Functional tests for the Hammer Java (JNI) bindings covering all exposed combinators.
 *
 * The JNI library must be loadable via java.library.path. The SConscript sets
 * this up automatically when running through the build system.
 */
public class HammerTests {

    static {
        System.loadLibrary("hammer_jni");
    }

    // Token type constants - mirror of HTokenType_ in hammer.h.
    static final int TT_NONE     = 1;
    static final int TT_BYTES    = 2;
    static final int TT_SINT     = 4;
    static final int TT_UINT     = 8;
    static final int TT_SEQUENCE = 16;

    static int passed = 0;
    static int failed = 0;

    static void assertTrue(String name, boolean cond) {
        if (cond) {
            passed++;
        } else {
            failed++;
            System.err.println("FAIL: " + name);
        }
    }

    static void assertNull(String name, Object obj) {
        assertTrue(name + " (expected null)", obj == null);
    }

    static void assertNotNull(String name, Object obj) {
        assertTrue(name + " (expected non-null)", obj != null);
    }

    static void assertEqual(String name, long expected, long actual) {
        if (expected != actual) {
            failed++;
            System.err.println("FAIL: " + name + " - expected " + expected + ", got " + actual);
        } else {
            passed++;
        }
    }

    // -------------------------------------------------------------------------

    static void testToken() {
        byte[] input = {(byte)0x39, (byte)0x35, (byte)0xa2};
        HParser p = hammer.h_token(input);

        HParseResult r = p.parse(input);
        assertNotNull("token:success", r);
        assertTrue("token:type",   r.getAst().tokenType() == TT_BYTES);
        assertEqual("token:length", 3, r.getAst().bytesLength());
        assertEqual("token:byte0",  0x39, r.getAst().byteAt(0));
        assertEqual("token:byte1",  0x35, r.getAst().byteAt(1));
        assertEqual("token:byte2",  0xa2, r.getAst().byteAt(2) & 0xff);

        assertNull("token:partial_fail", p.parse(new byte[]{(byte)0x39, (byte)0x35}));
    }

    static void testCh() {
        HParser p = hammer.h_ch((short)0xa2);

        HParseResult r = p.parse(new byte[]{(byte)0xa2});
        assertNotNull("ch:success", r);
        assertTrue("ch:type",  r.getAst().tokenType() == TT_UINT);
        assertEqual("ch:value", 0xa2L, r.getAst().uintValue());

        assertNull("ch:fail", p.parse(new byte[]{(byte)0xa3}));
    }

    static void testChRange() {
        HParser p = hammer.h_ch_range((short)'a', (short)'c');

        assertNotNull("ch_range:success",  p.parse(new byte[]{(byte)'b'}));
        assertNull("ch_range:fail",        p.parse(new byte[]{(byte)'d'}));
    }

    static void testInt8() {
        HParser p = hammer.h_int8();

        HParseResult r = p.parse(new byte[]{(byte)0x88});
        assertNotNull("int8:success", r);
        assertTrue("int8:type",   r.getAst().tokenType() == TT_SINT);
        assertEqual("int8:value", -0x78L, r.getAst().sintValue());

        assertNull("int8:fail", p.parse(new byte[]{}));
    }

    static void testInt16() {
        HParser p = hammer.h_int16();

        HParseResult r = p.parse(new byte[]{(byte)0xfe, (byte)0x00});
        assertNotNull("int16:success", r);
        assertTrue("int16:type",   r.getAst().tokenType() == TT_SINT);
        assertEqual("int16:value", -0x200L, r.getAst().sintValue());

        assertNull("int16:fail", p.parse(new byte[]{(byte)0xfe}));
    }

    static void testInt64() {
        HParser p = hammer.h_int64();
        byte[] input = {(byte)0xff,(byte)0xff,(byte)0xff,(byte)0xfe,
                        (byte)0x00,(byte)0x00,(byte)0x00,(byte)0x00};

        HParseResult r = p.parse(input);
        assertNotNull("int64:success", r);
        assertTrue("int64:type",   r.getAst().tokenType() == TT_SINT);
        assertEqual("int64:value", -0x200000000L, r.getAst().sintValue());

        assertNull("int64:fail", p.parse(new byte[]{
            (byte)0xff,(byte)0xff,(byte)0xff,(byte)0xfe,
            (byte)0x00,(byte)0x00,(byte)0x00}));
    }

    static void testInt32() {
        HParser p = hammer.h_int32();

        HParseResult r = p.parse(new byte[]{(byte)0xff,(byte)0xfe,(byte)0x00,(byte)0x00});
        assertNotNull("int32:success", r);
        assertEqual("int32:value", -0x20000L, r.getAst().sintValue());

        assertNull("int32:fail", p.parse(new byte[]{(byte)0xff,(byte)0xfe,(byte)0x00}));
    }

    static void testUint8() {
        HParser p = hammer.h_uint8();

        HParseResult r = p.parse(new byte[]{(byte)0x78});
        assertNotNull("uint8:success", r);
        assertEqual("uint8:value", 0x78L, r.getAst().uintValue());

        assertNull("uint8:fail", p.parse(new byte[]{}));
    }

    static void testUint16() {
        HParser p = hammer.h_uint16();

        HParseResult r = p.parse(new byte[]{(byte)0x02, (byte)0x00});
        assertNotNull("uint16:success", r);
        assertTrue("uint16:type",   r.getAst().tokenType() == TT_UINT);
        assertEqual("uint16:value", 0x200L, r.getAst().uintValue());

        assertNull("uint16:fail", p.parse(new byte[]{(byte)0x02}));
    }

    static void testUint32() {
        HParser p = hammer.h_uint32();

        HParseResult r = p.parse(new byte[]{(byte)0x00,(byte)0x02,(byte)0x00,(byte)0x00});
        assertNotNull("uint32:success", r);
        assertEqual("uint32:value", 0x20000L, r.getAst().uintValue());

        assertNull("uint32:fail", p.parse(new byte[]{(byte)0x00,(byte)0x02,(byte)0x00}));
    }

    static void testUint64() {
        HParser p = hammer.h_uint64();
        byte[] input = {(byte)0x00,(byte)0x00,(byte)0x00,(byte)0x02,
                        (byte)0x00,(byte)0x00,(byte)0x00,(byte)0x00};

        HParseResult r = p.parse(input);
        assertNotNull("uint64:success", r);
        assertTrue("uint64:type",   r.getAst().tokenType() == TT_UINT);
        assertEqual("uint64:value", 0x200000000L, r.getAst().uintValue());

        assertNull("uint64:fail", p.parse(new byte[]{
            (byte)0x00,(byte)0x00,(byte)0x00,(byte)0x02,
            (byte)0x00,(byte)0x00,(byte)0x00}));
    }

    static void testIntRange() {
        HParser p = hammer.h_int_range(hammer.h_uint8(), 3, 10);

        HParseResult r = p.parse(new byte[]{5});
        assertNotNull("int_range:success", r);
        assertEqual("int_range:value", 5L, r.getAst().uintValue());

        assertNull("int_range:fail", p.parse(new byte[]{11}));
    }

    static void testIn() {
        HParser p = hammer.h_in(new byte[]{(byte)'a', (byte)'b', (byte)'c'});

        HParseResult r = p.parse(new byte[]{(byte)'b'});
        assertNotNull("in:success", r);
        assertTrue("in:type", r.getAst().tokenType() == TT_UINT);
        assertEqual("in:value", 'b', r.getAst().uintValue());

        assertNull("in:fail", p.parse(new byte[]{(byte)'d'}));
    }

    static void testNotIn() {
        HParser p = hammer.h_not_in(new byte[]{(byte)'a', (byte)'b', (byte)'c'});

        HParseResult r = p.parse(new byte[]{(byte)'d'});
        assertNotNull("not_in:success", r);
        assertTrue("not_in:type", r.getAst().tokenType() == TT_UINT);
        assertEqual("not_in:value", 'd', r.getAst().uintValue());

        assertNull("not_in:fail", p.parse(new byte[]{(byte)'a'}));
    }

    static void testBytes() {
        HParser p = hammer.h_bytes(2);

        HParseResult r = p.parse(new byte[]{(byte)'a', (byte)'b'});
        assertNotNull("bytes:success", r);
        assertTrue("bytes:type", r.getAst().tokenType() == TT_BYTES);
        assertEqual("bytes:length", 2L, r.getAst().bytesLength());
        assertEqual("bytes:byte0", 'a', r.getAst().byteAt(0));
        assertEqual("bytes:byte1", 'b', r.getAst().byteAt(1));

        assertNull("bytes:fail", p.parse(new byte[]{(byte)'a'}));
    }

    static void testWhitespace() {
        HParser p = hammer.h_whitespace(hammer.h_ch((short)'a'));

        assertNotNull("whitespace:success",      p.parse(new byte[]{(byte)'a'}));
        assertNotNull("whitespace:leading_space", p.parse(new byte[]{(byte)' ', (byte)'a'}));
        assertNotNull("whitespace:leading_tab",   p.parse(new byte[]{(byte)'\t', (byte)'a'}));
        assertNull("whitespace:fail",            p.parse(new byte[]{(byte)'_', (byte)'a'}));
    }

    static void testLeft() {
        HParser p = hammer.h_left(hammer.h_ch((short)'a'), hammer.h_ch((short)' '));

        HParseResult r = p.parse(new byte[]{(byte)'a', (byte)' '});
        assertNotNull("left:success", r);
        assertEqual("left:value", 'a', r.getAst().uintValue());

        assertNull("left:fail_no_right", p.parse(new byte[]{(byte)'a'}));
        assertNull("left:fail",          p.parse(new byte[]{(byte)'b', (byte)' '}));
    }

    static void testRight() {
        HParser p = hammer.h_right(hammer.h_ch((short)' '), hammer.h_ch((short)'a'));

        HParseResult r = p.parse(new byte[]{(byte)' ', (byte)'a'});
        assertNotNull("right:success", r);
        assertEqual("right:value", 'a', r.getAst().uintValue());

        assertNull("right:fail_no_left", p.parse(new byte[]{(byte)'a'}));
        assertNull("right:fail",         p.parse(new byte[]{(byte)'b', (byte)'a'}));
    }

    static void testMiddle() {
        HParser p = hammer.h_middle(
            hammer.h_ch((short)' '), hammer.h_ch((short)'a'), hammer.h_ch((short)' '));

        HParseResult r = p.parse(new byte[]{(byte)' ', (byte)'a', (byte)' '});
        assertNotNull("middle:success", r);
        assertEqual("middle:value", 'a', r.getAst().uintValue());

        assertNull("middle:fail_no_right", p.parse(new byte[]{(byte)' ', (byte)'a'}));
        assertNull("middle:fail",          p.parse(new byte[]{(byte)'a'}));
    }

    static void testSequence() {
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_ch((short)'a'),
            hammer.h_ch((short)'b'),
        });

        HParseResult r = p.parse(new byte[]{(byte)'a', (byte)'b'});
        assertNotNull("sequence:success", r);
        assertTrue("sequence:type",   r.getAst().tokenType() == TT_SEQUENCE);
        assertEqual("sequence:length", 2L, r.getAst().seqLength());
        assertEqual("sequence:elem0", 'a', r.getAst().seqElement(0).uintValue());
        assertEqual("sequence:elem1", 'b', r.getAst().seqElement(1).uintValue());

        assertNull("sequence:fail_partial", p.parse(new byte[]{(byte)'a'}));
        assertNull("sequence:fail_wrong",   p.parse(new byte[]{(byte)'b'}));
    }

    static void testChoice() {
        HParser p = hammer.h_choice__a(new HParser[]{
            hammer.h_ch((short)'a'),
            hammer.h_ch((short)'b'),
        });

        HParseResult r1 = p.parse(new byte[]{(byte)'a'});
        assertNotNull("choice:a_success", r1);
        assertEqual("choice:a_value", 'a', r1.getAst().uintValue());

        HParseResult r2 = p.parse(new byte[]{(byte)'b'});
        assertNotNull("choice:b_success", r2);
        assertEqual("choice:b_value", 'b', r2.getAst().uintValue());

        assertNull("choice:fail", p.parse(new byte[]{(byte)'c'}));
    }

    static void testButNot() {
        HParser p = hammer.h_butnot(
            hammer.h_ch((short)'a'),
            hammer.h_token(new byte[]{(byte)'a', (byte)'b'}));

        assertNotNull("butnot:success", p.parse(new byte[]{(byte)'a'}));
        assertNull("butnot:fail",       p.parse(new byte[]{(byte)'a', (byte)'b'}));

        HParser q = hammer.h_butnot(
            hammer.h_ch_range((short)'0', (short)'9'),
            hammer.h_ch((short)'6'));

        assertNotNull("butnot_range:success", q.parse(new byte[]{(byte)'4'}));
        assertNull("butnot_range:fail",       q.parse(new byte[]{(byte)'6'}));
    }

    static void testDifference() {
        HParser p = hammer.h_difference(
            hammer.h_token(new byte[]{(byte)'a', (byte)'b'}),
            hammer.h_ch((short)'a'));

        assertNotNull("difference:success", p.parse(new byte[]{(byte)'a', (byte)'b'}));
        assertNull("difference:fail",       p.parse(new byte[]{(byte)'a'}));
    }

    static void testXor() {
        HParser p = hammer.h_xor(
            hammer.h_ch_range((short)'0', (short)'6'),
            hammer.h_ch_range((short)'5', (short)'9'));

        assertNotNull("xor:success_0", p.parse(new byte[]{(byte)'0'}));
        assertNotNull("xor:success_9", p.parse(new byte[]{(byte)'9'}));
        assertNull("xor:fail_overlap", p.parse(new byte[]{(byte)'5'}));
        assertNull("xor:fail_neither", p.parse(new byte[]{(byte)'a'}));
    }

    static void testMany() {
        HParser p = hammer.h_many(hammer.h_ch((short)'a'));

        HParseResult r = p.parse(new byte[]{(byte)'a',(byte)'a',(byte)'a'});
        assertNotNull("many:success", r);
        assertEqual("many:count", 3L, r.getAst().seqLength());

        HParseResult r0 = p.parse(new byte[]{});
        assertNotNull("many:empty", r0);
        assertEqual("many:empty_count", 0L, r0.getAst().seqLength());
    }

    static void testMany1() {
        HParser p = hammer.h_many1(hammer.h_ch((short)'a'));

        HParseResult r = p.parse(new byte[]{(byte)'a',(byte)'a'});
        assertNotNull("many1:success", r);
        assertEqual("many1:count", 2L, r.getAst().seqLength());

        assertNull("many1:fail_empty", p.parse(new byte[]{}));
    }

    static void testRepeatN() {
        HParser p = hammer.h_repeat_n(hammer.h_choice__a(new HParser[]{
            hammer.h_ch((short)'a'),
            hammer.h_ch((short)'b'),
        }), 2);

        HParseResult r = p.parse(new byte[]{(byte)'a', (byte)'b', (byte)'c'});
        assertNotNull("repeat_n:success", r);
        assertTrue("repeat_n:seq", r.getAst().tokenType() == TT_SEQUENCE);
        assertEqual("repeat_n:count", 2L, r.getAst().seqLength());
        assertEqual("repeat_n:elem0", 'a', r.getAst().seqElement(0).uintValue());
        assertEqual("repeat_n:elem1", 'b', r.getAst().seqElement(1).uintValue());

        assertNull("repeat_n:fail_wrong", p.parse(new byte[]{(byte)'a', (byte)'d'}));
    }

    static void testOptional() {
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_ch((short)'a'),
            hammer.h_optional(hammer.h_ch((short)'b')),
            hammer.h_ch((short)'c'),
        });

        HParseResult r1 = p.parse(new byte[]{(byte)'a',(byte)'b',(byte)'c'});
        assertNotNull("optional:abc_success", r1);
        assertEqual("optional:abc_length", 3L, r1.getAst().seqLength());

        HParseResult r2 = p.parse(new byte[]{(byte)'a',(byte)'c'});
        assertNotNull("optional:ac_success", r2);
        assertEqual("optional:ac_length", 3L, r2.getAst().seqLength());
        assertTrue("optional:ac_middle_none",
            r2.getAst().seqElement(1).tokenType() == TT_NONE);

        assertNull("optional:fail", p.parse(new byte[]{(byte)'a',(byte)'e',(byte)'c'}));
    }

    static void testIgnore() {
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_ch((short)'a'),
            hammer.h_ignore(hammer.h_ch((short)'b')),
            hammer.h_ch((short)'c'),
        });

        HParseResult r = p.parse(new byte[]{(byte)'a', (byte)'b', (byte)'c'});
        assertNotNull("ignore:success", r);
        // h_ignore strips result from sequence
        assertEqual("ignore:length", 2L, r.getAst().seqLength());
        assertEqual("ignore:elem0", 'a', r.getAst().seqElement(0).uintValue());
        assertEqual("ignore:elem1", 'c', r.getAst().seqElement(1).uintValue());

        assertNull("ignore:fail", p.parse(new byte[]{(byte)'a', (byte)'c'}));
    }

    static void testSepBy() {
        HParser p = hammer.h_sepBy(
            hammer.h_choice__a(new HParser[]{
                hammer.h_ch((short)'1'),
                hammer.h_ch((short)'2'),
                hammer.h_ch((short)'3'),
            }),
            hammer.h_ch((short)',')
        );

        HParseResult r = p.parse(new byte[]{(byte)'1',(byte)',',(byte)'2',(byte)',',(byte)'3'});
        assertNotNull("sepBy:success", r);
        assertEqual("sepBy:count", 3L, r.getAst().seqLength());

        HParseResult r0 = p.parse(new byte[]{});
        assertNotNull("sepBy:empty", r0);
        assertEqual("sepBy:empty_count", 0L, r0.getAst().seqLength());
    }

    static void testSepBy1() {
        HParser p = hammer.h_sepBy1(
            hammer.h_choice__a(new HParser[]{
                hammer.h_ch((short)'1'),
                hammer.h_ch((short)'2'),
                hammer.h_ch((short)'3'),
            }),
            hammer.h_ch((short)',')
        );

        HParseResult r = p.parse(new byte[]{(byte)'1',(byte)',',(byte)'2',(byte)',',(byte)'3'});
        assertNotNull("sepBy1:success", r);
        assertEqual("sepBy1:count", 3L, r.getAst().seqLength());

        HParseResult r1 = p.parse(new byte[]{(byte)'3'});
        assertNotNull("sepBy1:single", r1);
        assertEqual("sepBy1:single_count", 1L, r1.getAst().seqLength());

        assertNull("sepBy1:fail_empty", p.parse(new byte[]{}));
    }

    static void testEpsilonP() {
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_ch((short)'a'),
            hammer.h_epsilon_p(),
            hammer.h_ch((short)'b'),
        });

        HParseResult r = p.parse(new byte[]{(byte)'a', (byte)'b'});
        assertNotNull("epsilon_p:success", r);
        // epsilon_p TT_NONE is stripped from sequence result
        assertEqual("epsilon_p:length", 2L, r.getAst().seqLength());
        assertEqual("epsilon_p:elem0", 'a', r.getAst().seqElement(0).uintValue());
        assertEqual("epsilon_p:elem1", 'b', r.getAst().seqElement(1).uintValue());
    }

    static void testEndP() {
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_ch((short)'a'),
            hammer.h_end_p(),
        });

        assertNotNull("end_p:success",     p.parse(new byte[]{(byte)'a'}));
        assertNull("end_p:fail_trailing",  p.parse(new byte[]{(byte)'a', (byte)'a'}));
    }

    static void testNothingP() {
        HParser p = hammer.h_nothing_p();

        assertNull("nothing_p:fail_char",  p.parse(new byte[]{(byte)'a'}));
        assertNull("nothing_p:fail_empty", p.parse(new byte[]{}));
    }

    static void testPermutation() {
        HParser p = hammer.h_permutation__a(new HParser[]{
            hammer.h_ch((short)'a'),
            hammer.h_ch((short)'b'),
        });

        HParseResult r1 = p.parse(new byte[]{(byte)'a', (byte)'b'});
        assertNotNull("permutation:ab", r1);
        assertTrue("permutation:ab_seq", r1.getAst().tokenType() == TT_SEQUENCE);
        assertEqual("permutation:ab_length", 2L, r1.getAst().seqLength());
        assertEqual("permutation:ab_elem0", 'a', r1.getAst().seqElement(0).uintValue());
        assertEqual("permutation:ab_elem1", 'b', r1.getAst().seqElement(1).uintValue());

        // Result is always in argument order regardless of parse order
        HParseResult r2 = p.parse(new byte[]{(byte)'b', (byte)'a'});
        assertNotNull("permutation:ba", r2);
        assertEqual("permutation:ba_elem0", 'a', r2.getAst().seqElement(0).uintValue());
        assertEqual("permutation:ba_elem1", 'b', r2.getAst().seqElement(1).uintValue());

        assertNull("permutation:fail_aa", p.parse(new byte[]{(byte)'a', (byte)'a'}));
    }

    static void testLengthValue() {
        // uint8 for length, then that many uint8 values
        HParser p = hammer.h_length_value(hammer.h_uint8(), hammer.h_uint8());

        HParseResult r = p.parse(new byte[]{3, (byte)'a', (byte)'b', (byte)'c'});
        assertNotNull("length_value:success", r);
        assertTrue("length_value:seq", r.getAst().tokenType() == TT_SEQUENCE);
        assertEqual("length_value:count", 3L, r.getAst().seqLength());

        assertNull("length_value:fail", p.parse(new byte[]{3, (byte)'a', (byte)'b'}));
    }

    static void testAnd() {
        // h_and succeeds without consuming input
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_and(hammer.h_ch((short)'0')),
            hammer.h_ch((short)'0'),
        });
        HParseResult r = p.parse(new byte[]{(byte)'0'});
        assertNotNull("and:success", r);
        // h_and TT_NONE is stripped; only Ch('0') remains
        assertEqual("and:length", 1L, r.getAst().seqLength());
        assertEqual("and:value", '0', r.getAst().seqElement(0).uintValue());

        // and fails if lookahead fails
        HParser q = hammer.h_sequence__a(new HParser[]{
            hammer.h_and(hammer.h_ch((short)'0')),
            hammer.h_ch((short)'1'),
        });
        assertNull("and:lookahead_mismatch", q.parse(new byte[]{(byte)'0'}));

        // and does not consume; trailing input is parsed by next parser
        HParser r2 = hammer.h_sequence__a(new HParser[]{
            hammer.h_ch((short)'1'),
            hammer.h_and(hammer.h_ch((short)'2')),
        });
        HParseResult res = r2.parse(new byte[]{(byte)'1', (byte)'2'});
        assertNotNull("and:no_consume", res);
        assertEqual("and:no_consume_length", 1L, res.getAst().seqLength());
    }

    static void testNot() {
        // h_not succeeds if inner parser fails (negative lookahead)
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_ch((short)'+'),
            hammer.h_not(hammer.h_ch((short)'+')),
        });

        assertNotNull("not:success", p.parse(new byte[]{(byte)'+', (byte)'x'}));
        assertNull("not:fail",       p.parse(new byte[]{(byte)'+', (byte)'+'}));
    }

    static void testRightrec() {
        // Recursive parser: a* via indirect
        HParser p = hammer.h_indirect();
        HParser a = hammer.h_ch((short)'a');
        hammer.h_bind_indirect(p, hammer.h_choice__a(new HParser[]{
            hammer.h_sequence__a(new HParser[]{a, p}),
            hammer.h_epsilon_p(),
        }));

        assertNotNull("rightrec:a",   p.parse(new byte[]{(byte)'a'}));
        assertNotNull("rightrec:aa",  p.parse(new byte[]{(byte)'a', (byte)'a'}));
        assertNotNull("rightrec:aaa", p.parse(new byte[]{(byte)'a', (byte)'a', (byte)'a'}));
    }

    static void testWithEndianness() {
        // BYTE_LITTLE_ENDIAN | BIT_BIG_ENDIAN = 0 | 2 = 2
        // In little-endian byte order, {0x01, 0x00} = 0x0001 = 1
        HParser p = hammer.h_with_endianness(
            (char)(hammerConstants.BYTE_LITTLE_ENDIAN | hammerConstants.BIT_BIG_ENDIAN),
            hammer.h_uint16());

        HParseResult r = p.parse(new byte[]{(byte)0x01, (byte)0x00});
        assertNotNull("with_endianness:success", r);
        assertEqual("with_endianness:value", 1L, r.getAst().uintValue());

        assertNull("with_endianness:fail", p.parse(new byte[]{(byte)0x01}));
    }

    static void testPutGetValue() {
        // put_value stashes the result; get_value retrieves it
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_put_value(hammer.h_ch((short)'a'), "c"),
            hammer.h_get_value("c"),
        });

        HParseResult r = p.parse(new byte[]{(byte)'a'});
        assertNotNull("put_get_value:success", r);
        assertTrue("put_get_value:seq", r.getAst().tokenType() == TT_SEQUENCE);
        assertEqual("put_get_value:length", 2L, r.getAst().seqLength());
        assertEqual("put_get_value:elem0", 'a', r.getAst().seqElement(0).uintValue());
        assertEqual("put_get_value:elem1", 'a', r.getAst().seqElement(1).uintValue());

        assertNull("put_get_value:fail", p.parse(new byte[]{(byte)'b'}));
    }

    static void testFreeValue() {
        // free_value retrieves and removes the stashed value
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_put_value(hammer.h_ch((short)'a'), "c"),
            hammer.h_free_value("c"),
        });

        HParseResult r = p.parse(new byte[]{(byte)'a'});
        assertNotNull("free_value:success", r);
        assertEqual("free_value:length", 2L, r.getAst().seqLength());
        assertEqual("free_value:elem0", 'a', r.getAst().seqElement(0).uintValue());
        assertEqual("free_value:elem1", 'a', r.getAst().seqElement(1).uintValue());

        assertNull("free_value:fail", p.parse(new byte[]{(byte)'b'}));
    }

    static void testSkip() {
        // h_skip consumes n bits without adding to the parse result
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_skip(8),
            hammer.h_ch((short)'b'),
        });

        HParseResult r = p.parse(new byte[]{(byte)'a', (byte)'b'});
        assertNotNull("skip:success", r);
        // h_skip TT_NONE is stripped from sequence
        assertEqual("skip:length", 1L, r.getAst().seqLength());
        assertEqual("skip:value", 'b', r.getAst().seqElement(0).uintValue());

        // Only 8 bits available; skip consumes them all, leaving nothing for h_ch
        assertNull("skip:fail", p.parse(new byte[]{(byte)'b'}));
    }

    static void testTell() {
        // h_tell reports the current bit position as TT_UINT
        HParser p = hammer.h_sequence__a(new HParser[]{
            hammer.h_ch((short)'a'),
            hammer.h_tell(),
        });

        HParseResult r = p.parse(new byte[]{(byte)'a'});
        assertNotNull("tell:success", r);
        assertTrue("tell:seq", r.getAst().tokenType() == TT_SEQUENCE);
        // Tell's TT_UINT is included in the sequence
        assertEqual("tell:length", 2L, r.getAst().seqLength());
        // After reading 'a' (8 bits), position is 8
        assertEqual("tell:position", 8L, r.getAst().seqElement(1).uintValue());
    }

    // -------------------------------------------------------------------------

    public static void main(String[] args) {
        testToken();
        testCh();
        testChRange();
        testInt8();
        testInt16();
        testInt32();
        testInt64();
        testUint8();
        testUint16();
        testUint32();
        testUint64();
        testIntRange();
        testIn();
        testNotIn();
        testBytes();
        testWhitespace();
        testLeft();
        testRight();
        testMiddle();
        testSequence();
        testChoice();
        testButNot();
        testDifference();
        testXor();
        testMany();
        testMany1();
        testRepeatN();
        testOptional();
        testIgnore();
        testSepBy();
        testSepBy1();
        testEpsilonP();
        testEndP();
        testNothingP();
        testPermutation();
        testLengthValue();
        testAnd();
        testNot();
        testRightrec();
        testWithEndianness();
        testPutGetValue();
        testFreeValue();
        testSkip();
        testTell();

        System.out.printf("Results: %d passed, %d failed%n", passed, failed);
        if (failed > 0) System.exit(1);
    }
}
