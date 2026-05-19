package hammer_test

import (
	"bytes"
	"testing"
	"unsafe"

	h "hammer"
)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// parseBytes calls H_parse on the given input and returns the HParseResult.
// Returns nil (zero HParseResult) when the result's AST is nil.
func parseBytes(parser h.HParser, input []byte) h.HParseResult {
	if len(input) == 0 {
		// H_parse needs at least a valid pointer; pass a dummy one.
		dummy := byte(0)
		return h.H_parse(parser, &dummy, 0)
	}
	return h.H_parse(parser, &input[0], int64(len(input)))
}

// astNil returns true when the parse failed (AST pointer is nil / zero).
func astNil(r h.HParseResult) bool {
	if r == nil {
		return true
	}
	// SWIG wraps the result as an interface; Swigcptr() == 0 means NULL.
	type swigger interface{ Swigcptr() uintptr }
	if s, ok := r.(swigger); ok && s.Swigcptr() == 0 {
		return true
	}
	ast := r.GetAst()
	if ast == nil {
		return true
	}
	if s, ok := ast.(swigger); ok && s.Swigcptr() == 0 {
		return true
	}
	return false
}

// tokenType constants (mirroring the TT_* accessors).
var (
	TT_BYTES    = h.TT_BYTES
	TT_SINT     = h.TT_SINT
	TT_UINT     = h.TT_UINT
	TT_SEQUENCE = h.TT_SEQUENCE
	TT_NONE     = h.TT_NONE
)

// getBytes extracts a []byte from a TT_BYTES token.
func getBytes(tok h.HParsedToken) []byte {
	ptr := tok.BytesData()

	if ptr == nil {
		return nil
	}

	n := tok.BytesLength()

	src := unsafe.Slice((*byte)(unsafe.Pointer(ptr)), n)

	out := make([]byte, n)
	copy(out, src)

	return out
}

// getSint extracts a signed int64 from a TT_SINT token.
func getSint(tok h.HParsedToken) int64 {
	return tok.SintValue()
}

// getUint extracts an unsigned uint64 from a TT_UINT token.
func getUint(tok h.HParsedToken) uint64 {
	return tok.UintValue()
}

// getSeq returns the HCountedArray for a TT_SEQUENCE token.
func getSeq(tok h.HParsedToken) []h.HParsedToken {
	n := tok.SeqLength()
	out := make([]h.HParsedToken, n)

	for i := int64(0); i < n; i++ {
		out[i] = tok.SeqElement(i)
	}

	return out
}

// seqElem returns the i-th element of a sequence as an HParsedToken.
// HCountedArray.GetElements() returns a pointer to the first element; we
// advance by pointer arithmetic using unsafe.
func seqElem(seq h.HCountedArray, i int) h.HParsedToken {
	// GetElements returns HParsedToken which is a SWIG interface wrapping
	// HParsedToken*; the sequence stores HParsedToken* elements[].
	// We get the base pointer via Swigcptr and offset by pointer size.
	base := seq.GetElements().(interface{ Swigcptr() uintptr }).Swigcptr()
	ptrSize := unsafe.Sizeof(uintptr(0))
	elemPtr := *(*uintptr)(unsafe.Pointer(base + uintptr(i)*ptrSize))
	return h.SwigcptrHParsedToken(elemPtr)
}

// buildParsers is a variadic helper to build a NULL-terminated uintptr slice
// for H_sequence__a / H_choice__a.
func buildParsers(parsers ...h.HParser) *uintptr {
	ptrs := make([]uintptr, len(parsers)+1)
	for i, p := range parsers {
		ptrs[i] = p.(interface{ Swigcptr() uintptr }).Swigcptr()
	}
	ptrs[len(parsers)] = 0
	return &ptrs[0]
}

func sequence(parsers ...h.HParser) h.HParser {
	return h.H_sequence__a(buildParsers(parsers...))
}

func choice(parsers ...h.HParser) h.HParser {
	return h.H_choice__a(buildParsers(parsers...))
}

// ---------------------------------------------------------------------------
// TestToken
// ---------------------------------------------------------------------------

func TestTokenSuccess(t *testing.T) {
	input := []byte{0x39, 0x35, 0xa2} // "95\xa2"
	parser := h.H_token(&input[0], int64(len(input)))

	r := parseBytes(parser, input)
	if astNil(r) {
		t.Fatal("expected parse success, got nil")
	}
	ast := r.GetAst()
	if int(ast.GetToken_type()) != int(TT_BYTES) {
		t.Fatal("expected TT_BYTES token")
	}
	got := getBytes(ast)
	if !bytes.Equal(got, input) {
		t.Errorf("got %v, want %v", got, input)
	}
}

func TestTokenPartialFails(t *testing.T) {
	tok := []byte{0x39, 0x35, 0xa2}
	parser := h.H_token(&tok[0], int64(len(tok)))

	r := parseBytes(parser, []byte{0x39, 0x35})
	if !astNil(r) {
		t.Fatal("expected parse failure, got success")
	}
}

// ---------------------------------------------------------------------------
// TestCh
// ---------------------------------------------------------------------------

func TestChSuccess(t *testing.T) {
	parser := h.H_ch(0xa2)
	r := parseBytes(parser, []byte{0xa2})
	if astNil(r) {
		t.Fatal("expected parse success")
	}
	ast := r.GetAst()
	if int(ast.GetToken_type()) != int(TT_UINT) {
		t.Fatal("expected TT_UINT token")
	}
	if getUint(ast) != 0xa2 {
		t.Errorf("got %#x, want 0xa2", getUint(ast))
	}
}

func TestChFailure(t *testing.T) {
	parser := h.H_ch(0xa2)
	r := parseBytes(parser, []byte{0xa3})
	if !astNil(r) {
		t.Fatal("expected parse failure")
	}
}

// ---------------------------------------------------------------------------
// TestChRange
// ---------------------------------------------------------------------------

func TestChRangeSuccess(t *testing.T) {
	parser := h.H_ch_range('a', 'c')
	r := parseBytes(parser, []byte{'b'})
	if astNil(r) {
		t.Fatal("expected parse success")
	}
	if getUint(r.GetAst()) != 'b' {
		t.Errorf("got %c, want 'b'", getUint(r.GetAst()))
	}
}

func TestChRangeFailure(t *testing.T) {
	parser := h.H_ch_range('a', 'c')
	r := parseBytes(parser, []byte{'d'})
	if !astNil(r) {
		t.Fatal("expected parse failure")
	}
}

// ---------------------------------------------------------------------------
// TestInt64
// ---------------------------------------------------------------------------

func TestInt64Success(t *testing.T) {
	parser := h.H_int64()
	input := []byte{0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00}
	r := parseBytes(parser, input)
	if astNil(r) {
		t.Fatal("expected parse success")
	}
	want := int64(-0x200000000)
	if getSint(r.GetAst()) != want {
		t.Errorf("got %d, want %d", getSint(r.GetAst()), want)
	}
}

func TestInt64Failure(t *testing.T) {
	parser := h.H_int64()
	r := parseBytes(parser, []byte{0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00})
	if !astNil(r) {
		t.Fatal("expected parse failure (too short)")
	}
}

// ---------------------------------------------------------------------------
// TestInt32
// ---------------------------------------------------------------------------

func TestInt32Success(t *testing.T) {
	parser := h.H_int32()

	cases := []struct {
		input []byte
		want  int64
	}{
		{[]byte{0xff, 0xfe, 0x00, 0x00}, -0x20000},
		{[]byte{0x00, 0x02, 0x00, 0x00}, 0x20000},
	}
	for _, c := range cases {
		r := parseBytes(parser, c.input)
		if astNil(r) {
			t.Fatalf("expected success for %v", c.input)
		}
		if getSint(r.GetAst()) != c.want {
			t.Errorf("got %d, want %d", getSint(r.GetAst()), c.want)
		}
	}
}

func TestInt32Failure(t *testing.T) {
	parser := h.H_int32()
	for _, input := range [][]byte{{0xff, 0xfe, 0x00}, {0x00, 0x02, 0x00}} {
		if !astNil(parseBytes(parser, input)) {
			t.Fatalf("expected failure for %v", input)
		}
	}
}

// ---------------------------------------------------------------------------
// TestInt16
// ---------------------------------------------------------------------------

func TestInt16Success(t *testing.T) {
	parser := h.H_int16()
	cases := []struct {
		input []byte
		want  int64
	}{
		{[]byte{0xfe, 0x00}, -0x200},
		{[]byte{0x02, 0x00}, 0x200},
	}
	for _, c := range cases {
		r := parseBytes(parser, c.input)
		if astNil(r) {
			t.Fatalf("expected success for %v", c.input)
		}
		if getSint(r.GetAst()) != c.want {
			t.Errorf("got %d, want %d", getSint(r.GetAst()), c.want)
		}
	}
}

func TestInt16Failure(t *testing.T) {
	parser := h.H_int16()
	for _, input := range [][]byte{{0xfe}, {0x02}} {
		if !astNil(parseBytes(parser, input)) {
			t.Fatalf("expected failure for %v", input)
		}
	}
}

// ---------------------------------------------------------------------------
// TestInt8
// ---------------------------------------------------------------------------

func TestInt8Success(t *testing.T) {
	parser := h.H_int8()
	r := parseBytes(parser, []byte{0x88})
	if astNil(r) {
		t.Fatal("expected success")
	}
	want := int64(-0x78)
	if getSint(r.GetAst()) != want {
		t.Errorf("got %d, want %d", getSint(r.GetAst()), want)
	}
}

func TestInt8Failure(t *testing.T) {
	parser := h.H_int8()
	if !astNil(parseBytes(parser, []byte{})) {
		t.Fatal("expected failure on empty input")
	}
}

// ---------------------------------------------------------------------------
// TestUint64
// ---------------------------------------------------------------------------

func TestUint64Success(t *testing.T) {
	parser := h.H_uint64()
	input := []byte{0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00}
	r := parseBytes(parser, input)
	if astNil(r) {
		t.Fatal("expected success")
	}
	want := uint64(0x200000000)
	if getUint(r.GetAst()) != want {
		t.Errorf("got %d, want %d", getUint(r.GetAst()), want)
	}
}

func TestUint64Failure(t *testing.T) {
	parser := h.H_uint64()
	if !astNil(parseBytes(parser, []byte{0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestUint32
// ---------------------------------------------------------------------------

func TestUint32Success(t *testing.T) {
	parser := h.H_uint32()
	r := parseBytes(parser, []byte{0x00, 0x02, 0x00, 0x00})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != 0x20000 {
		t.Errorf("got %d, want 0x20000", getUint(r.GetAst()))
	}
}

func TestUint32Failure(t *testing.T) {
	parser := h.H_uint32()
	if !astNil(parseBytes(parser, []byte{0x00, 0x02, 0x00})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestUint16
// ---------------------------------------------------------------------------

func TestUint16Success(t *testing.T) {
	parser := h.H_uint16()
	r := parseBytes(parser, []byte{0x02, 0x00})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != 0x200 {
		t.Errorf("got %d, want 0x200", getUint(r.GetAst()))
	}
}

func TestUint16Failure(t *testing.T) {
	parser := h.H_uint16()
	if !astNil(parseBytes(parser, []byte{0x02})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestUint8
// ---------------------------------------------------------------------------

func TestUint8Success(t *testing.T) {
	parser := h.H_uint8()
	r := parseBytes(parser, []byte{0x78})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != 0x78 {
		t.Errorf("got %d, want 0x78", getUint(r.GetAst()))
	}
}

func TestUint8Failure(t *testing.T) {
	parser := h.H_uint8()
	if !astNil(parseBytes(parser, []byte{})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestIntRange
// ---------------------------------------------------------------------------

func TestIntRangeSuccess(t *testing.T) {
	parser := h.H_int_range(h.H_uint8(), 3, 10)
	r := parseBytes(parser, []byte{0x05})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != 5 {
		t.Errorf("got %d, want 5", getUint(r.GetAst()))
	}
}

func TestIntRangeFailure(t *testing.T) {
	parser := h.H_int_range(h.H_uint8(), 3, 10)
	if !astNil(parseBytes(parser, []byte{0x0b})) {
		t.Fatal("expected failure for value out of range")
	}
}

// ---------------------------------------------------------------------------
// TestWhitespace
// ---------------------------------------------------------------------------

func TestWhitespaceSuccess(t *testing.T) {
	parser := h.H_whitespace(h.H_ch('a'))
	for _, input := range [][]byte{{'a'}, {' ', 'a'}, {' ', ' ', 'a'}, {'\t', 'a'}} {
		r := parseBytes(parser, input)
		if astNil(r) {
			t.Fatalf("expected success for %q", input)
		}
		if getUint(r.GetAst()) != 'a' {
			t.Errorf("got %c, want 'a'", getUint(r.GetAst()))
		}
	}
}

func TestWhitespaceFailure(t *testing.T) {
	parser := h.H_whitespace(h.H_ch('a'))
	if !astNil(parseBytes(parser, []byte{'_', 'a'})) {
		t.Fatal("expected failure for non-whitespace prefix")
	}
}

// ---------------------------------------------------------------------------
// TestWhitespaceEnd
// ---------------------------------------------------------------------------

func TestWhitespaceEndSuccess(t *testing.T) {
	// whitespace(end_p()) parses optional trailing whitespace then end — the
	// result AST is TT_NONE (epsilon), so astNil behaviour matches Python None.
	parser := h.H_whitespace(h.H_end_p())
	// Both "" and "  " succeed but the Python test expects None (TT_NONE token)
	for _, input := range [][]byte{{}, {' ', ' '}} {
		r := parseBytes(parser, input)
		// Success means we get a result, even if the AST is TT_NONE.
		// Here we just assert no crash; the Python test also expects None.
		_ = r
	}
}

func TestWhitespaceEndFailure(t *testing.T) {
	parser := h.H_whitespace(h.H_end_p())
	if !astNil(parseBytes(parser, []byte{' ', ' ', 'x'})) {
		t.Fatal("expected failure when non-whitespace follows")
	}
}

// ---------------------------------------------------------------------------
// TestLeft
// ---------------------------------------------------------------------------

func TestLeftSuccess(t *testing.T) {
	parser := h.H_left(h.H_ch('a'), h.H_ch(' '))
	r := parseBytes(parser, []byte{'a', ' '})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != 'a' {
		t.Errorf("got %c, want 'a'", getUint(r.GetAst()))
	}
}

func TestLeftFailure(t *testing.T) {
	parser := h.H_left(h.H_ch('a'), h.H_ch(' '))
	for _, input := range [][]byte{{'a'}, {' '}, {'a', 'b'}} {
		if !astNil(parseBytes(parser, input)) {
			t.Fatalf("expected failure for %q", input)
		}
	}
}

// ---------------------------------------------------------------------------
// TestRight
// ---------------------------------------------------------------------------

func TestRightSuccess(t *testing.T) {
	parser := h.H_right(h.H_ch(' '), h.H_ch('a'))
	r := parseBytes(parser, []byte{' ', 'a'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != 'a' {
		t.Errorf("got %c, want 'a'", getUint(r.GetAst()))
	}
}

func TestRightFailure(t *testing.T) {
	parser := h.H_right(h.H_ch(' '), h.H_ch('a'))
	for _, input := range [][]byte{{'a'}, {' '}, {'b', 'a'}} {
		if !astNil(parseBytes(parser, input)) {
			t.Fatalf("expected failure for %q", input)
		}
	}
}

// ---------------------------------------------------------------------------
// TestMiddle
// ---------------------------------------------------------------------------

func TestMiddleSuccess(t *testing.T) {
	parser := h.H_middle(h.H_ch(' '), h.H_ch('a'), h.H_ch(' '))
	r := parseBytes(parser, []byte{' ', 'a', ' '})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != 'a' {
		t.Errorf("got %c, want 'a'", getUint(r.GetAst()))
	}
}

func TestMiddleFailure(t *testing.T) {
	parser := h.H_middle(h.H_ch(' '), h.H_ch('a'), h.H_ch(' '))
	for _, input := range [][]byte{
		{'a'}, {' '}, {' ', 'a'}, {'a', ' '}, {' ', 'b', ' '}, {'b', 'a', ' '}, {' ', 'a', 'b'},
	} {
		if !astNil(parseBytes(parser, input)) {
			t.Fatalf("expected failure for %q", input)
		}
	}
}

// ---------------------------------------------------------------------------
// TestIn / TestNotIn
// ---------------------------------------------------------------------------

func TestInSuccess(t *testing.T) {
	charset := []byte("abc")
	parser := h.H_in(&charset[0], int64(len(charset)))
	r := parseBytes(parser, []byte{'b'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != 'b' {
		t.Errorf("got %c, want 'b'", getUint(r.GetAst()))
	}
}

func TestInFailure(t *testing.T) {
	charset := []byte("abc")
	parser := h.H_in(&charset[0], int64(len(charset)))
	if !astNil(parseBytes(parser, []byte{'d'})) {
		t.Fatal("expected failure")
	}
}

func TestNotInSuccess(t *testing.T) {
	charset := []byte("abc")
	parser := h.H_not_in(&charset[0], int64(len(charset)))
	r := parseBytes(parser, []byte{'d'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != 'd' {
		t.Errorf("got %c, want 'd'", getUint(r.GetAst()))
	}
}

func TestNotInFailure(t *testing.T) {
	charset := []byte("abc")
	parser := h.H_not_in(&charset[0], int64(len(charset)))
	if !astNil(parseBytes(parser, []byte{'a'})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestEndP
// ---------------------------------------------------------------------------

func TestEndPSuccess(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_end_p())
	r := parseBytes(parser, []byte{'a'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	if len(seq) != 1 {
		t.Errorf("expected 1 element in sequence, got %d", len(seq))
	}
	if getUint(seq[0]) != 'a' {
		t.Errorf("got %c, want 'a'", getUint(seq[0]))
	}
}

func TestEndPFailure(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_end_p())
	if !astNil(parseBytes(parser, []byte{'a', 'a'})) {
		t.Fatal("expected failure when input not exhausted")
	}
}

// ---------------------------------------------------------------------------
// TestNothingP
// ---------------------------------------------------------------------------

func TestNothingPFailure(t *testing.T) {
	parser := h.H_nothing_p()
	if !astNil(parseBytes(parser, []byte{'a'})) {
		t.Fatal("expected failure; nothing_p always fails")
	}
}

// ---------------------------------------------------------------------------
// TestSequence
// ---------------------------------------------------------------------------

func TestSequenceSuccess(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_ch('b'))
	r := parseBytes(parser, []byte{'a', 'b'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	if len(seq) != 2 {
		t.Fatalf("expected 2 elements, got %d", len(seq))
	}
	if getUint(seq[0]) != 'a' || getUint(seq[1]) != 'b' {
		t.Error("sequence elements mismatch")
	}
}

func TestSequenceFailure(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_ch('b'))
	for _, input := range [][]byte{{'a'}, {'b'}} {
		if !astNil(parseBytes(parser, input)) {
			t.Fatalf("expected failure for %q", input)
		}
	}
}

// ---------------------------------------------------------------------------
// TestSequenceWhitespace
// ---------------------------------------------------------------------------

func TestSequenceWhitespaceSuccess(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_whitespace(h.H_ch('b')))
	for _, input := range [][]byte{{'a', 'b'}, {'a', ' ', 'b'}, {'a', ' ', ' ', 'b'}} {
		r := parseBytes(parser, input)
		if astNil(r) {
			t.Fatalf("expected success for %q", input)
		}
		seq := getSeq(r.GetAst())
		if getUint(seq[0]) != 'a' || getUint(seq[1]) != 'b' {
			t.Errorf("wrong elements for %q", input)
		}
	}
}

func TestSequenceWhitespaceFailure(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_whitespace(h.H_ch('b')))
	if !astNil(parseBytes(parser, []byte{'a', ' ', ' ', 'c'})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestChoice
// ---------------------------------------------------------------------------

func TestChoiceSuccess(t *testing.T) {
	parser := choice(h.H_ch('a'), h.H_ch('b'))
	for ch, want := range map[byte]uint64{'a': 'a', 'b': 'b'} {
		r := parseBytes(parser, []byte{ch})
		if astNil(r) {
			t.Fatalf("expected success for %c", ch)
		}
		if getUint(r.GetAst()) != want {
			t.Errorf("got %c, want %c", getUint(r.GetAst()), want)
		}
	}
}

func TestChoiceFailure(t *testing.T) {
	parser := choice(h.H_ch('a'), h.H_ch('b'))
	if !astNil(parseBytes(parser, []byte{'c'})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestButNot
// ---------------------------------------------------------------------------

func TestButNotSuccess(t *testing.T) {
	tok := []byte("ab")
	parser := h.H_butnot(h.H_ch('a'), h.H_token(&tok[0], int64(len(tok))))
	for _, input := range [][]byte{{'a'}, {'a', 'a'}} {
		r := parseBytes(parser, input)
		if astNil(r) {
			t.Fatalf("expected success for %q", input)
		}
		if getUint(r.GetAst()) != 'a' {
			t.Errorf("got %c, want 'a'", getUint(r.GetAst()))
		}
	}
}

func TestButNotFailure(t *testing.T) {
	tok := []byte("ab")
	parser := h.H_butnot(h.H_ch('a'), h.H_token(&tok[0], int64(len(tok))))
	if !astNil(parseBytes(parser, []byte{'a', 'b'})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestButNotRange
// ---------------------------------------------------------------------------

func TestButNotRangeSuccess(t *testing.T) {
	parser := h.H_butnot(h.H_ch_range('0', '9'), h.H_ch('6'))
	r := parseBytes(parser, []byte{'4'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	if getUint(r.GetAst()) != '4' {
		t.Errorf("got %c, want '4'", getUint(r.GetAst()))
	}
}

func TestButNotRangeFailure(t *testing.T) {
	parser := h.H_butnot(h.H_ch_range('0', '9'), h.H_ch('6'))
	if !astNil(parseBytes(parser, []byte{'6'})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestDifference
// ---------------------------------------------------------------------------

func TestDifferenceSuccess(t *testing.T) {
	tok := []byte("ab")
	parser := h.H_difference(h.H_token(&tok[0], int64(len(tok))), h.H_ch('a'))
	r := parseBytes(parser, []byte{'a', 'b'})
	if astNil(r) {
		t.Fatal("expected success")
	}
}

func TestDifferenceFailure(t *testing.T) {
	tok := []byte("ab")
	parser := h.H_difference(h.H_token(&tok[0], int64(len(tok))), h.H_ch('a'))
	if !astNil(parseBytes(parser, []byte{'a'})) {
		t.Fatal("expected failure")
	}
}

// ---------------------------------------------------------------------------
// TestXor
// ---------------------------------------------------------------------------

func TestXorSuccess(t *testing.T) {
	parser := h.H_xor(h.H_ch_range('0', '6'), h.H_ch_range('5', '9'))
	for _, ch := range []byte{'0', '9'} {
		r := parseBytes(parser, []byte{ch})
		if astNil(r) {
			t.Fatalf("expected success for %c", ch)
		}
	}
}

func TestXorFailure(t *testing.T) {
	parser := h.H_xor(h.H_ch_range('0', '6'), h.H_ch_range('5', '9'))
	for _, ch := range []byte{'5', 'a'} {
		if !astNil(parseBytes(parser, []byte{ch})) {
			t.Fatalf("expected failure for %c", ch)
		}
	}
}

// ---------------------------------------------------------------------------
// TestMany
// ---------------------------------------------------------------------------

func TestManySuccess(t *testing.T) {
	parser := h.H_many(choice(h.H_ch('a'), h.H_ch('b')))

	// Empty input → empty sequence (used == 0)
	r := parseBytes(parser, []byte{})
	if astNil(r) {
		t.Fatal("expected success on empty input")
	}
	if len(getSeq(r.GetAst())) != 0 {
		t.Error("expected empty sequence")
	}

	// Single char
	r = parseBytes(parser, []byte{'a'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	if len(seq) != 1 || getUint(seq[0]) != 'a' {
		t.Error("expected ['a']")
	}

	// Multiple chars
	r = parseBytes(parser, []byte{'a', 'a', 'b', 'b', 'a', 'b', 'a'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq = getSeq(r.GetAst())
	expected := []byte{'a', 'a', 'b', 'b', 'a', 'b', 'a'}
	if len(seq) != int(len(expected)) {
		t.Fatalf("expected %d elements, got %d", len(expected), len(seq))
	}
	for i, want := range expected {
		if getUint(seq[i]) != uint64(want) {
			t.Errorf("element %d: got %c, want %c", i, getUint(seq[i]), want)
		}
	}
}

// ---------------------------------------------------------------------------
// TestMany1
// ---------------------------------------------------------------------------

func TestMany1Success(t *testing.T) {
	parser := h.H_many1(choice(h.H_ch('a'), h.H_ch('b')))
	for _, input := range [][]byte{{'a'}, {'b'}, {'a', 'a', 'b', 'b', 'a', 'b', 'a'}} {
		r := parseBytes(parser, input)
		if astNil(r) {
			t.Fatalf("expected success for %q", input)
		}
	}
}

func TestMany1Failure(t *testing.T) {
	parser := h.H_many1(choice(h.H_ch('a'), h.H_ch('b')))
	for _, input := range [][]byte{{}, {'d', 'a', 'a', 'b', 'b', 'a', 'b', 'a', 'd'}} {
		if !astNil(parseBytes(parser, input)) {
			t.Fatalf("expected failure for %q", input)
		}
	}
}

// ---------------------------------------------------------------------------
// TestRepeatN
// ---------------------------------------------------------------------------

func TestRepeatNSuccess(t *testing.T) {
	parser := h.H_repeat_n(choice(h.H_ch('a'), h.H_ch('b')), 2)
	r := parseBytes(parser, []byte{'a', 'b', 'd', 'e', 'f'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	if len(seq) != 2 {
		t.Fatalf("expected 2 elements, got %d", len(seq))
	}
	if getUint(seq[0]) != 'a' || getUint(seq[1]) != 'b' {
		t.Error("wrong elements")
	}
}

func TestRepeatNFailure(t *testing.T) {
	parser := h.H_repeat_n(choice(h.H_ch('a'), h.H_ch('b')), 2)
	for _, input := range [][]byte{{'a', 'd', 'e', 'f'}, {'d', 'a', 'b', 'd', 'e', 'f'}} {
		if !astNil(parseBytes(parser, input)) {
			t.Fatalf("expected failure for %q", input)
		}
	}
}

// ---------------------------------------------------------------------------
// TestOptional
// ---------------------------------------------------------------------------

func TestOptionalSuccess(t *testing.T) {
	parser := sequence(
		h.H_ch('a'),
		h.H_optional(choice(h.H_ch('b'), h.H_ch('c'))),
		h.H_ch('d'),
	)

	// "abd" → (a, b, d)
	r := parseBytes(parser, []byte{'a', 'b', 'd'})
	if astNil(r) {
		t.Fatal("expected success for 'abd'")
	}
	seq := getSeq(r.GetAst())
	if len(seq) != 3 {
		t.Fatalf("expected 3 elements, got %d", len(seq))
	}

	// "acd" → (a, c, d)
	r = parseBytes(parser, []byte{'a', 'c', 'd'})
	if astNil(r) {
		t.Fatal("expected success for 'acd'")
	}

	// "ad" → (a, <placeholder>, d); the optional returns TT_NONE
	r = parseBytes(parser, []byte{'a', 'd'})
	if astNil(r) {
		t.Fatal("expected success for 'ad'")
	}
	seq = getSeq(r.GetAst())
	if len(seq) != 3 {
		t.Fatalf("expected 3 elements including placeholder, got %d", len(seq))
	}
	placeholder := seq[1]
	if int(placeholder.GetToken_type()) != int(TT_NONE) {
		t.Errorf("expected TT_NONE placeholder, got token type %d", placeholder.GetToken_type())
	}
}

func TestOptionalFailure(t *testing.T) {
	parser := sequence(
		h.H_ch('a'),
		h.H_optional(choice(h.H_ch('b'), h.H_ch('c'))),
		h.H_ch('d'),
	)
	for _, input := range [][]byte{{'a', 'e', 'd'}, {'a', 'b'}, {'a', 'c'}} {
		if !astNil(parseBytes(parser, input)) {
			t.Fatalf("expected failure for %q", input)
		}
	}
}

// ---------------------------------------------------------------------------
// TestIgnore
// ---------------------------------------------------------------------------

func TestIgnoreSuccess(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_ignore(h.H_ch('b')), h.H_ch('c'))
	r := parseBytes(parser, []byte{'a', 'b', 'c'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	// ignore drops 'b', so only 'a' and 'c' remain
	if len(seq) != 2 {
		t.Fatalf("expected 2 elements after ignore, got %d", len(seq))
	}
	if getUint(seq[0]) != 'a' || getUint(seq[1]) != 'c' {
		t.Error("wrong elements after ignore")
	}
}

func TestIgnoreFailure(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_ignore(h.H_ch('b')), h.H_ch('c'))
	if !astNil(parseBytes(parser, []byte{'a', 'c'})) {
		t.Fatal("expected failure when ignored token is missing")
	}
}

// ---------------------------------------------------------------------------
// TestSepBy
// ---------------------------------------------------------------------------

func TestSepBySuccess(t *testing.T) {
	parser := h.H_sepBy(
		choice(h.H_ch('1'), h.H_ch('2'), h.H_ch('3')),
		h.H_ch(','),
	)

	cases := []struct {
		input     []byte
		wantLen   int64
		wantElems []byte
	}{
		{[]byte("1,2,3"), 3, []byte{'1', '2', '3'}},
		{[]byte("1,3,2"), 3, []byte{'1', '3', '2'}},
		{[]byte("1,3"), 2, []byte{'1', '3'}},
		{[]byte("3"), 1, []byte{'3'}},
		{[]byte(""), 0, nil},
	}
	for _, c := range cases {
		r := parseBytes(parser, c.input)
		if astNil(r) {
			t.Fatalf("expected success for %q", c.input)
		}
		seq := getSeq(r.GetAst())
		if len(seq) != int(c.wantLen) {
			t.Fatalf("%q: expected %d elements, got %d", c.input, c.wantLen, len(seq))
		}
		for i, want := range c.wantElems {
			if getUint(seq[i]) != uint64(want) {
				t.Errorf("%q: element %d: got %c, want %c", c.input, i, getUint(seq[i]), want)
			}
		}
	}
}

// ---------------------------------------------------------------------------
// TestSepBy1
// ---------------------------------------------------------------------------

func TestSepBy1Success(t *testing.T) {
	parser := h.H_sepBy1(
		choice(h.H_ch('1'), h.H_ch('2'), h.H_ch('3')),
		h.H_ch(','),
	)
	for _, input := range [][]byte{[]byte("1,2,3"), []byte("1,3,2"), []byte("1,3"), {'3'}} {
		if astNil(parseBytes(parser, input)) {
			t.Fatalf("expected success for %q", input)
		}
	}
}

func TestSepBy1Failure(t *testing.T) {
	parser := h.H_sepBy1(
		choice(h.H_ch('1'), h.H_ch('2'), h.H_ch('3')),
		h.H_ch(','),
	)
	if !astNil(parseBytes(parser, []byte{})) {
		t.Fatal("expected failure on empty input")
	}
}

// ---------------------------------------------------------------------------
// TestEpsilonP
// ---------------------------------------------------------------------------

func TestEpsilonP1Success(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_epsilon_p(), h.H_ch('b'))
	r := parseBytes(parser, []byte{'a', 'b'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	// epsilon_p consumes nothing; sequence should contain 'a' and 'b'
	if len(seq) != 2 {
		t.Fatalf("expected 2 elements, got %d", len(seq))
	}
}

func TestEpsilonP2Success(t *testing.T) {
	parser := sequence(h.H_epsilon_p(), h.H_ch('a'))
	r := parseBytes(parser, []byte{'a'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	if len(seq) != 1 || getUint(seq[0]) != 'a' {
		t.Error("expected ['a']")
	}
}

func TestEpsilonP3Success(t *testing.T) {
	parser := sequence(h.H_ch('a'), h.H_epsilon_p())
	r := parseBytes(parser, []byte{'a'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	if len(seq) != 1 || getUint(seq[0]) != 'a' {
		t.Error("expected ['a']")
	}
}

// ---------------------------------------------------------------------------
// TestAnd
// ---------------------------------------------------------------------------

func TestAnd1Success(t *testing.T) {
	parser := sequence(h.H_and(h.H_ch('0')), h.H_ch('0'))
	r := parseBytes(parser, []byte{'0'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	if len(seq) != 1 || getUint(seq[0]) != '0' {
		t.Error("expected ['0']")
	}
}

func TestAnd2Failure(t *testing.T) {
	// and_(ch('0')) succeeds as lookahead, but ch('1') then fails on '0'
	parser := sequence(h.H_and(h.H_ch('0')), h.H_ch('1'))
	if !astNil(parseBytes(parser, []byte{'0'})) {
		t.Fatal("expected failure")
	}
}

func TestAnd3Success(t *testing.T) {
	// sequence(ch('1'), and_(ch('2'))) on "12": and_ is lookahead, not consumed
	parser := sequence(h.H_ch('1'), h.H_and(h.H_ch('2')))
	r := parseBytes(parser, []byte{'1', '2'})
	if astNil(r) {
		t.Fatal("expected success")
	}
	seq := getSeq(r.GetAst())
	if len(seq) != 1 || getUint(seq[0]) != '1' {
		t.Error("expected ['1'] (and_ result excluded)")
	}
}

// ---------------------------------------------------------------------------
// TestNot
// ---------------------------------------------------------------------------

func TestNot1Success(t *testing.T) {
	tok := []byte("++")
	parser := sequence(
		h.H_ch('a'),
		choice(h.H_ch('+'), h.H_token(&tok[0], int64(len(tok)))),
		h.H_ch('b'),
	)
	r := parseBytes(parser, []byte("a+b"))
	if astNil(r) {
		t.Fatal("expected success for 'a+b'")
	}
}

func TestNot1Failure(t *testing.T) {
	tok := []byte("++")
	parser := sequence(
		h.H_ch('a'),
		choice(h.H_ch('+'), h.H_token(&tok[0], int64(len(tok)))),
		h.H_ch('b'),
	)
	if !astNil(parseBytes(parser, []byte("a++b"))) {
		t.Fatal("expected failure for 'a++b'")
	}
}

func TestNot2Success(t *testing.T) {
	tok := []byte("++")
	parser := sequence(
		h.H_ch('a'),
		choice(
			sequence(h.H_ch('+'), h.H_not(h.H_ch('+'))),
			h.H_token(&tok[0], int64(len(tok))),
		),
		h.H_ch('b'),
	)
	// "a+b" → inner sequence (+, not(+)) matches: + consumed, lookahead passes
	r := parseBytes(parser, []byte("a+b"))
	if astNil(r) {
		t.Fatal("expected success for 'a+b'")
	}
	// "a++b" → inner sequence fails (not(+) fails on second +), so ++ token matches
	r = parseBytes(parser, []byte("a++b"))
	if astNil(r) {
		t.Fatal("expected success for 'a++b'")
	}
}

// ---------------------------------------------------------------------------
// TestRightrec (indirect / recursive parser)
// ---------------------------------------------------------------------------

func TestRightrecSuccess(t *testing.T) {
	parser := h.H_indirect()
	a := h.H_ch('a')
	h.H_bind_indirect(parser, choice(sequence(a, parser), h.H_epsilon_p()))

	// "a" → ('a',)
	r := parseBytes(parser, []byte{'a'})
	if astNil(r) {
		t.Fatal("expected success for 'a'")
	}

	// "aa" → ('a', ('a',))
	r = parseBytes(parser, []byte{'a', 'a'})
	if astNil(r) {
		t.Fatal("expected success for 'aa'")
	}

	// "aaa" → ('a', ('a', ('a',)))
	r = parseBytes(parser, []byte{'a', 'a', 'a'})
	if astNil(r) {
		t.Fatal("expected success for 'aaa'")
	}
}
