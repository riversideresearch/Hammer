
import os
import tempfile
import unittest

import hammer as h


class TestTokenParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.token(b"95\xa2")

    def test_success(self):
        self.assertEqual(self.parser.parse(b"95\xa2"), b"95\xa2")

    def test_partial_fails(self):
        self.assertEqual(self.parser.parse(b"95"), None)


class TestChParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser_int = h.ch(0xa2)
        cls.parser_chr = h.ch(b"\xa2")

    def test_success(self):
        self.assertEqual(self.parser_int.parse(b"\xa2"), 0xa2)
        self.assertEqual(self.parser_chr.parse(b"\xa2"), b"\xa2")

    def test_failure(self):
        self.assertEqual(self.parser_int.parse(b"\xa3"), None)
        self.assertEqual(self.parser_chr.parse(b"\xa3"), None)


class TestChRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.ch_range(b"a", b"c")

    def test_success(self):
        self.assertEqual(self.parser.parse(b"b"), b"b")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"d"), None)


class TestInt64(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int64()

    def test_success(self):
        self.assertEqual(self.parser.parse(b"\xff\xff\xff\xfe\x00\x00\x00\x00"), -0x200000000)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"\xff\xff\xff\xfe\x00\x00\x00"), None)


class TestInt32(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int32()

    def test_success(self):
        self.assertEqual(self.parser.parse(b"\xff\xfe\x00\x00"), -0x20000)
        self.assertEqual(self.parser.parse(b"\x00\x02\x00\x00"), 0x20000)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"\xff\xfe\x00"), None)
        self.assertEqual(self.parser.parse(b"\x00\x02\x00"), None)


class TestInt16(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int16()

    def test_success(self):
        self.assertEqual(self.parser.parse(b"\xfe\x00"), -0x200)
        self.assertEqual(self.parser.parse(b"\x02\x00"), 0x200)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"\xfe"), None)
        self.assertEqual(self.parser.parse(b"\x02"), None)


class TestInt8(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int8()

    def test_success(self):
        self.assertEqual(self.parser.parse(b"\x88"), -0x78)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b""), None)


class TestUint64(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.uint64()

    def test_success(self):
        self.assertEqual(self.parser.parse(b"\x00\x00\x00\x02\x00\x00\x00\x00"), 0x200000000)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"\x00\x00\x00\x02\x00\x00\x00"), None)


class TestUint32(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.uint32()

    def test_success(self):
        self.assertEqual(self.parser.parse(b"\x00\x02\x00\x00"), 0x20000)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"\x00\x02\x00"), None)


class TestUint16(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.uint16()

    def test_success(self):
        self.assertEqual(self.parser.parse(b"\x02\x00"), 0x200)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"\x02"), None)


class TestUint8(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.uint8()

    def test_success(self):
        self.assertEqual(self.parser.parse(b"\x78"), 0x78)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b""), None)


class TestIntRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int_range(h.uint8(), 3, 10)

    def test_success(self):
        self.assertEqual(self.parser.parse(b"\x05"), 5)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"\x0b"), None)


class TestWhitespace(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.whitespace(h.ch(b"a"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a"), b"a")
        self.assertEqual(self.parser.parse(b" a"), b"a")
        self.assertEqual(self.parser.parse(b"  a"), b"a")
        self.assertEqual(self.parser.parse(b"\ta"), b"a")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"_a"), None)


class TestWhitespaceEnd(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.whitespace(h.end_p())

    def test_success(self):
        self.assertEqual(self.parser.parse(b""), None)
        self.assertEqual(self.parser.parse(b"  "), None)

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"  x"), None)


class TestLeft(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.left(h.ch(b"a"), h.ch(b" "))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a "), b"a")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"a"), None)
        self.assertEqual(self.parser.parse(b" "), None)
        self.assertEqual(self.parser.parse(b"ab"), None)


class TestRight(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.right(h.ch(b" "), h.ch(b"a"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b" a"), b"a")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"a"), None)
        self.assertEqual(self.parser.parse(b" "), None)
        self.assertEqual(self.parser.parse(b"ba"), None)


class TestMiddle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.middle(h.ch(b" "), h.ch(b"a"), h.ch(b" "))

    def test_success(self):
        self.assertEqual(self.parser.parse(b" a "), b"a")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"a"), None)
        self.assertEqual(self.parser.parse(b" "), None)
        self.assertEqual(self.parser.parse(b" a"), None)
        self.assertEqual(self.parser.parse(b"a "), None)
        self.assertEqual(self.parser.parse(b" b "), None)
        self.assertEqual(self.parser.parse(b"ba "), None)
        self.assertEqual(self.parser.parse(b" ab"), None)


class TestAction(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.action(
            h.sequence(
                h.choice(h.ch(b"a"), h.ch(b"A")),
                h.choice(h.ch(b"b"), h.ch(b"B")),
            ),
            lambda x: [y.upper() for y in x],
        )

    def test_success(self):
        self.assertEqual(self.parser.parse(b"ab"), [b"A", b"B"])
        self.assertEqual(self.parser.parse(b"AB"), [b"A", b"B"])

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"XX"), None)


class TestCallbackExceptions(unittest.TestCase):
    def test_action_exception_propagates(self):
        parser = h.action(h.ch(b"a"), lambda _: 1 / 0)
        with self.assertRaises(ZeroDivisionError):
            parser.parse(b"a")

    def test_action_exception_propagates_from_parse_debug(self):
        parser = h.action(h.ch(b"a"), lambda _: 1 / 0)
        with self.assertRaises(ZeroDivisionError):
            parser.parse_debug(b"a")

    def test_predicate_exception_propagates(self):
        parser = h.attr_bool(h.ch(b"a"), lambda _: 1 / 0)
        with self.assertRaises(ZeroDivisionError):
            parser.parse(b"a")

    def test_deferred_action_exception_propagates(self):
        collection = h.action_collection()
        stashed = h.action_stash(h.ch(b"a"), lambda _: 1 / 0, collection)
        parser = h.action_apply(stashed, collection)
        with self.assertRaises(ZeroDivisionError):
            parser.parse(b"a")


class TestIn(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.in_(b"abc")

    def test_success(self):
        self.assertEqual(self.parser.parse(b"b"), b"b")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"d"), None)


class TestNotIn(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.not_in(b"abc")

    def test_success(self):
        self.assertEqual(self.parser.parse(b"d"), b"d")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"a"), None)


class TestEndP(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch(b"a"), h.end_p())

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a"), (b"a",))

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"aa"), None)


class TestNothingP(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.nothing_p()

    def test_success(self):
        pass

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"a"), None)


class TestSequence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch(b"a"), h.ch(b"b"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"ab"), (b"a", b"b"))

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"a"), None)
        self.assertEqual(self.parser.parse(b"b"), None)


class TestSequenceWhitespace(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch(b"a"), h.whitespace(h.ch(b"b")))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"ab"), (b"a", b"b"))
        self.assertEqual(self.parser.parse(b"a b"), (b"a", b"b"))
        self.assertEqual(self.parser.parse(b"a  b"), (b"a", b"b"))

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"a  c"), None)


class TestChoice(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.choice(h.ch(b"a"), h.ch(b"b"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a"), b"a")
        self.assertEqual(self.parser.parse(b"b"), b"b")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"c"), None)


class TestButNot(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.butnot(h.ch(b"a"), h.token(b"ab"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a"), b"a")
        self.assertEqual(self.parser.parse(b"aa"), b"a")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"ab"), None)


class TestButNotRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.butnot(h.ch_range(b"0", b"9"), h.ch(b"6"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"4"), b"4")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"6"), None)


class TestDifference(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.difference(h.token(b"ab"), h.ch(b"a"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"ab"), b"ab")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"a"), None)


class TestXor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.xor(h.ch_range(b"0", b"6"), h.ch_range(b"5", b"9"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"0"), b"0")
        self.assertEqual(self.parser.parse(b"9"), b"9")

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"5"), None)
        self.assertEqual(self.parser.parse(b"a"), None)


class TestMany(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.many(h.choice(h.ch(b"a"), h.ch(b"b")))

    def test_success(self):
        self.assertEqual(self.parser.parse(b""), ())
        self.assertEqual(self.parser.parse(b"a"), (b"a",))
        self.assertEqual(self.parser.parse(b"b"), (b"b",))
        self.assertEqual(
            self.parser.parse(b"aabbaba"), (b"a", b"a", b"b", b"b", b"a", b"b", b"a")
        )

    def test_failure(self):
        pass


class TestMany1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.many1(h.choice(h.ch(b"a"), h.ch(b"b")))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a"), (b"a",))
        self.assertEqual(self.parser.parse(b"b"), (b"b",))
        self.assertEqual(
            self.parser.parse(b"aabbaba"), (b"a", b"a", b"b", b"b", b"a", b"b", b"a")
        )

    def test_failure(self):
        self.assertEqual(self.parser.parse(b""), None)
        self.assertEqual(self.parser.parse(b"daabbabadef"), None)


class TestRepeatN(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.repeat_n(h.choice(h.ch(b"a"), h.ch(b"b")), 2)

    def test_success(self):
        self.assertEqual(self.parser.parse(b"abdef"), (b"a", b"b"))

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"adef"), None)
        self.assertEqual(self.parser.parse(b"dabdef"), None)


class TestOptional(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(
            h.ch(b"a"),
            h.optional(h.choice(h.ch(b"b"), h.ch(b"c"))),
            h.ch(b"d"),
        )

    def test_success(self):
        self.assertEqual(self.parser.parse(b"abd"), (b"a", b"b", b"d"))
        self.assertEqual(self.parser.parse(b"acd"), (b"a", b"c", b"d"))
        self.assertEqual(self.parser.parse(b"ad"), (b"a", h.Placeholder(), b"d"))

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"aed"), None)
        self.assertEqual(self.parser.parse(b"ab"), None)
        self.assertEqual(self.parser.parse(b"ac"), None)


class TestIgnore(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch(b"a"), h.ignore(h.ch(b"b")), h.ch(b"c"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"abc"), (b"a", b"c"))

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"ac"), None)


class TestSepBy(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sepBy(
            h.choice(h.ch(b"1"), h.ch(b"2"), h.ch(b"3")), h.ch(b",")
        )

    def test_success(self):
        self.assertEqual(self.parser.parse(b"1,2,3"), (b"1", b"2", b"3"))
        self.assertEqual(self.parser.parse(b"1,3,2"), (b"1", b"3", b"2"))
        self.assertEqual(self.parser.parse(b"1,3"), (b"1", b"3"))
        self.assertEqual(self.parser.parse(b"3"), (b"3",))
        self.assertEqual(self.parser.parse(b""), ())

    def test_failure(self):
        pass


class TestSepBy1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sepBy1(
            h.choice(h.ch(b"1"), h.ch(b"2"), h.ch(b"3")), h.ch(b",")
        )

    def test_success(self):
        self.assertEqual(self.parser.parse(b"1,2,3"), (b"1", b"2", b"3"))
        self.assertEqual(self.parser.parse(b"1,3,2"), (b"1", b"3", b"2"))
        self.assertEqual(self.parser.parse(b"1,3"), (b"1", b"3"))
        self.assertEqual(self.parser.parse(b"3"), (b"3",))

    def test_failure(self):
        self.assertEqual(self.parser.parse(b""), None)


class TestEpsilonP1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch(b"a"), h.epsilon_p(), h.ch(b"b"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"ab"), (b"a", b"b"))

    def test_failure(self):
        pass


class TestEpsilonP2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.epsilon_p(), h.ch(b"a"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a"), (b"a",))

    def test_failure(self):
        pass


class TestEpsilonP3(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch(b"a"), h.epsilon_p())

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a"), (b"a",))

    def test_failure(self):
        pass


class TestAttrBool(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.attr_bool(
            h.many1(h.choice(h.ch(b"a"), h.ch(b"b"))),
            lambda x: x[0] == x[1],
        )

    def test_success(self):
        self.assertEqual(self.parser.parse(b"aa"), (b"a", b"a"))
        self.assertEqual(self.parser.parse(b"bb"), (b"b", b"b"))

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"ab"), None)


class TestAnd1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.and_(h.ch(b"0")), h.ch(b"0"))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"0"), (b"0",))

    def test_failure(self):
        pass


class TestAnd2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.and_(h.ch(b"0")), h.ch(b"1"))

    def test_success(self):
        pass

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"0"), None)


class TestAnd3(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch(b"1"), h.and_(h.ch(b"2")))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"12"), (b"1",))

    def test_failure(self):
        pass


class TestNot1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(
            h.ch(b"a"),
            h.choice(h.ch(b"+"), h.token(b"++")),
            h.ch(b"b"),
        )

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a+b"), (b"a", b"+", b"b"))

    def test_failure(self):
        self.assertEqual(self.parser.parse(b"a++b"), None)


class TestNot2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(
            h.ch(b"a"),
            h.choice(
                h.sequence(h.ch(b"+"), h.not_(h.ch(b"+"))),
                h.token(b"++"),
            ),
            h.ch(b"b"),
        )

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a+b"), (b"a", (b"+",), b"b"))
        self.assertEqual(self.parser.parse(b"a++b"), (b"a", b"++", b"b"))

    def test_failure(self):
        pass


class TestRightrec(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.indirect()
        a = h.ch(b"a")
        cls.parser.bind(h.choice(h.sequence(a, cls.parser), h.epsilon_p()))

    def test_success(self):
        self.assertEqual(self.parser.parse(b"a"), (b"a",))
        self.assertEqual(self.parser.parse(b"aa"), (b"a", (b"a",)))
        self.assertEqual(self.parser.parse(b"aaa"), (b"a", (b"a", (b"a",))))

    def test_failure(self):
        pass


class TestFloatParsers(unittest.TestCase):
    CASES = (
        ("float16", b"\x3e\x00", b"\x38\x00", b"\x3e", 1.5),
        ("float32", b"\x3f\xc0\x00\x00", b"\x3f\x00\x00\x00", b"\x3f\xc0\x00", 1.5),
        (
            "float64",
            b"\x3f\xf8\x00\x00\x00\x00\x00\x00",
            b"\x3f\xe0\x00\x00\x00\x00\x00\x00",
            b"\x3f\xf8\x00\x00\x00\x00\x00",
            1.5,
        ),
    )

    def test_parse_all_widths(self):
        for constructor, encoded, _, _, expected in self.CASES:
            with self.subTest(constructor=constructor):
                self.assertEqual(getattr(h, constructor)().parse(encoded), expected)

    def test_truncated_input_fails(self):
        for constructor, _, _, truncated, _ in self.CASES:
            with self.subTest(constructor=constructor):
                self.assertIsNone(getattr(h, constructor)().parse(truncated))

    def test_ranges_are_inclusive(self):
        for constructor, encoded, below_range, _, expected in self.CASES:
            with self.subTest(constructor=constructor):
                parser = h.float_range(getattr(h, constructor)(), 1.5, 1.5)
                self.assertEqual(parser.parse(encoded), expected)
                self.assertIsNone(parser.parse(below_range))


class TestNewCombinators(unittest.TestCase):
    def test_bit_helpers(self):
        self.assertEqual(h.bit0().parse(b"\x00"), 0)
        self.assertEqual(h.bit1().parse(b"\x80"), 1)
        self.assertIsNone(h.bit0().parse(b"\x80"))
        self.assertIsNone(h.bit1().parse(b"\x00"))

    def test_capped_repetition(self):
        self.assertEqual(h.many_cap(h.ch(b"a"), 2).parse(b"aaa"), (b"a", b"a"))
        self.assertEqual(h.many1_cap(h.ch(b"a"), 2).parse(b"aaa"), (b"a", b"a"))
        self.assertIsNone(h.many1_cap(h.ch(b"a"), 2).parse(b""))

    def test_ch_rejects_out_of_range_integers(self):
        for value in (-1, 256):
            with self.subTest(value=value), self.assertRaises(ValueError):
                    h.ch(value)

    def test_dispatch_mapping_and_default(self):
        parser = h.dispatch(
            h.uint8(),
            [(1, h.ch(b"a")), (2, h.ch(b"b"))],
            h.ch(b"z"),
        )
        self.assertEqual(parser.parse(b"\x01a"), (1, b"a"))
        self.assertEqual(parser.parse(b"\x02b"), (2, b"b"))
        self.assertEqual(parser.parse(b"\x03z"), (3, b"z"))
        self.assertIsNone(parser.parse(b"\x01b"))

    def test_dispatch_rejects_empty_entries(self):
        with self.assertRaisesRegex(ValueError, "at least one opcode/parser entry"):
            h.dispatch(h.uint8(), [])

    def test_dispatch_rejects_malformed_entry(self):
        with self.assertRaisesRegex(TypeError, "each dispatch entry must be an"):
            h.dispatch(h.uint8(), [object()])

    def test_dispatch_rejects_wrong_entry_length(self):
        with self.assertRaisesRegex(ValueError, "exactly two values"):
            h.dispatch(h.uint8(), [(1,)])

    def test_dispatch_rejects_invalid_opcode(self):
        with self.assertRaises(TypeError):
            h.dispatch(h.uint8(), [("not an opcode", h.ch(b"a"))])

    def test_dispatch_rejects_out_of_range_opcode(self):
        with self.assertRaisesRegex(ValueError, "unsigned 32-bit integer"):
            h.dispatch(h.uint8(), [(1 << 32, h.ch(b"a"))])

    def test_dispatch_rejects_invalid_parser(self):
        with self.assertRaisesRegex(TypeError, "each dispatch parser must be an HParser"):
            h.dispatch(h.uint8(), [(1, object())])


class TestDeferredActions(unittest.TestCase):
    def test_stashed_action_commits_only_on_success(self):
        calls = []
        collection = h.action_collection()
        stashed = h.action_stash(
            h.ch(b"a"),
            lambda value: calls.append(value) or value.upper(),
            collection,
        )
        parser = h.action_apply(stashed, collection)

        self.assertEqual(parser.parse(b"a"), b"A")
        self.assertEqual(calls, [b"a"])

    def test_stashed_action_does_not_run_for_a_failed_parse(self):
        calls = []
        collection = h.action_collection()
        stashed = h.action_stash(
            h.ch(b"a"),
            lambda value: calls.append(value) or value,
            collection,
        )
        parser = h.action_apply(h.sequence(stashed, h.nothing_p()), collection)

        self.assertIsNone(parser.parse(b"a"))
        self.assertEqual(calls, [])


class TestParseDiagnostics(unittest.TestCase):
    def test_diagnostic_snapshot_for_failure(self):
        child = h.set_label(h.ch(b"a"), "letter-a")
        h.set_error_message(child, "expected the letter a")
        parser = h.context(
            child,
            "letter-a-context",
            file_name="grammar.py",
            function_name="make_parser",
            line=42,
            column=3,
        )

        result, diagnostic = parser.parse_debug(b"b")

        self.assertIsNone(result)
        self.assertIsInstance(diagnostic, h.ParseDiagnostic)
        self.assertEqual(diagnostic.error.index, 0)
        self.assertEqual(diagnostic.error.actual, ord("b"))
        self.assertEqual(diagnostic.error.parser, "letter-a-context")
        self.assertEqual(diagnostic.error.message, "expected the letter a")
        self.assertEqual(diagnostic.error.source, h.SourceLocation("grammar.py", "make_parser", 42, 3))
        self.assertIn(
            h.ParseExpectation(0, ord("a"), ord("a")),
            diagnostic.expected,
        )
        self.assertIsInstance(diagnostic.execution_trace, (str, type(None)))

    def test_diagnostic_snapshot_for_success(self):
        with tempfile.TemporaryFile() as stderr:
            saved_stderr = os.dup(2)
            try:
                os.dup2(stderr.fileno(), 2)
                result, diagnostic = h.ch(b"a").parse_debug(b"a", True)
            finally:
                os.dup2(saved_stderr, 2)
                os.close(saved_stderr)
            stderr.seek(0)
            report = stderr.read()

        self.assertEqual(result, b"a")
        self.assertIsInstance(diagnostic, (h.ParseDiagnostic, type(None)))
        if diagnostic is not None:
            self.assertIn(b"=== h_parse_error ===", report)
            self.assertIn(b"parse succeeded", report)

    def test_end_of_input_expectation(self):
        result, diagnostic = h.end_p().parse_debug(b"a")

        self.assertIsNone(result)
        self.assertIn(h.ParseExpectation(1, 0, 0), diagnostic.expected)


if __name__ == "__main__":
    unittest.main()
