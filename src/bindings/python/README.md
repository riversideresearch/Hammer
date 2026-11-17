# Hammer Python Bindings

Python bindings for the Hammer parser combinator library, generated with [SWIG](https://www.swig.org/).

## Prerequisites

- [SWIG](https://www.swig.org/) 4.x
- Python 3.8+
- `setuptools` (`pip install setuptools`)
- Hammer shared library built (see top-level README)

```bash
sudo apt install swig
pip install setuptools
```

## Building

From the repository root, pass `bindings=python` to SCons:

```bash
scons bindings=python
```

This copies the SWIG interface into the build tree, runs `setup.py build_ext --inplace`, and produces `hammer.py` and `_hammer.so` under `build/opt/src/bindings/python/`.

The default interpreter is `python3`. To use a specific version:

```bash
scons bindings=python python=python3.12
```

## Running the Tests

```bash
scons bindings=python testpython
```

Or run the test suite directly after building:

```bash
cd build/opt/src/bindings/python
LD_LIBRARY_PATH=../.. python -m unittest hammer_tests
```

## Usage

Add the build directory to your Python path, then import `hammer`:

```python
import sys
sys.path.insert(0, "build/opt/src/bindings/python")
import hammer as h
```

If you ran `scons install` (or `scons bindings=python installpython`), the package is available system-wide without any path manipulation.

#### Virtual Environments

If you are using a Python virtual environment, activate it before building and installing so that `setup.py` installs into the venv's site-packages rather than the system Python:

```bash
source /path/to/venv/bin/activate
scons bindings=python installpython
```

Alternatively, pass the venv interpreter explicitly so SCons uses it throughout:

```bash
scons bindings=python python=/path/to/venv/bin/python installpython
```

### Basic Example

```python
import hammer as h

# Parse the literal bytes "GET "
method = h.token(b"GET ")

# Parse one or more printable ASCII characters
printable = h.many1(h.ch_range(b"\x21", b"\x7e"))

# Sequence : method followed by the path
request_line = h.sequence(method, printable)

result = request_line.parse(b"GET /index.html")
print(result)  # (b'GET ', (b'/', b'i', b'n', ...))
```

### Parser Combinators

| Python function      | Description                                              |
|----------------------|----------------------------------------------------------|
| `h.token(b"...")`    | Match a literal byte string                              |
| `h.ch(byte)`         | Match a single byte (integer or `bytes` of length 1)    |
| `h.ch_range(lo, hi)` | Match any byte in `[lo, hi]`                             |
| `h.in_(charset)`     | Match any byte in the given `bytes` charset              |
| `h.not_in(charset)`  | Match any byte not in the given `bytes` charset          |
| `h.take_rest()`      | Capture all remaining input as `bytes`                   |
| `h.sequence(*ps)`    | Match each parser in order; return tuple of results      |
| `h.choice(*ps)`      | Try each parser in order; return first success           |
| `h.many(p)`          | Match `p` zero or more times; return tuple               |
| `h.many1(p)`         | Match `p` one or more times; return tuple                |
| `h.many_cap(p, n)`   | Match `p` up to `n` times; return tuple                  |
| `h.many1_cap(p, n)`  | Match `p` one to `n` times; return tuple                 |
| `h.repeat_n(p, n)`   | Match `p` exactly `n` times; return tuple                |
| `h.optional(p)`      | Match `p` or produce a `Placeholder` on failure          |
| `h.ignore(p)`        | Match `p` but suppress its result from sequences         |
| `h.sepBy(p, sep)`    | Match `p` separated by `sep`, zero or more times         |
| `h.sepBy1(p, sep)`   | Match `p` separated by `sep`, one or more times          |
| `h.left(p1, p2)`     | Match both; return result of `p1`                        |
| `h.right(p1, p2)`    | Match both; return result of `p2`                        |
| `h.middle(p1,p2,p3)` | Match all three; return result of `p2`                   |
| `h.butnot(p1, p2)`   | Match `p1` only if `p2` does not also match              |
| `h.difference(p1,p2)`| Match `p1` only when `p2` matches less input             |
| `h.xor(p1, p2)`      | Match exactly one of `p1` or `p2`, not both              |
| `h.and_(p)`          | Succeed if `p` would match, but consume no input         |
| `h.not_(p)`          | Succeed if `p` would not match, consuming no input       |
| `h.whitespace(p)`    | Skip leading whitespace, then match `p`                  |
| `h.action(p, fn)`    | Apply `fn` to the result of `p`                          |
| `h.attr_bool(p, fn)` | Match `p` only if predicate `fn` returns `True`          |
| `h.int_range(p,lo,hi)`| Match `p` only if the integer result is in `[lo, hi]`   |
| `h.float_range(p,lo,hi)`| Match `p` only if the float result is in `[lo, hi]`    |
| `h.bit0()` / `h.bit1()` | Match one bit with the specified value                 |
| `h.dispatch(d, entries, default=None)` | Select a parser from `(opcode, parser)` entries |
| `h.indirect()`       | Create a forward-declared parser for recursive grammars  |
| `h.epsilon_p()`      | Always succeed, consuming no input                       |
| `h.end_p()`          | Succeed only at end of input                             |
| `h.nothing_p()`      | Always fail                                              |
| `h.put_value(p, name)` | Parse `p` and store the result under `name`            |
| `h.get_value(name)`  | Retrieve a previously stored value by `name`             |
| `h.free_value(name)` | Retrieve and free a previously stored value by `name`    |

### Integer Parsers

```python
h.uint8()   # unsigned 8-bit
h.uint16()  # unsigned 16-bit, big-endian
h.uint32()  # unsigned 32-bit, big-endian
h.uint64()  # unsigned 64-bit, big-endian
h.int8()    # signed 8-bit
h.int16()   # signed 16-bit, big-endian
h.int32()   # signed 32-bit, big-endian
h.int64()   # signed 64-bit, big-endian
```

### Floating-Point Parsers

```python
h.float16()  # IEEE 754 binary16, returned as Python float
h.float32()  # IEEE 754 binary32, returned as Python float
h.float64()  # IEEE 754 binary64, returned as Python float
```

Use `h.float_range(parser, lower, upper)` to require an inclusive range.

### Actions and Predicates

```python
# Transform a parse result
digits = h.action(h.many1(h.ch_range(b"0", b"9")),
                  lambda bs: int(b"".join(bs)))

# Reject a result based on a condition
even_byte = h.attr_bool(h.uint8(), lambda n: n % 2 == 0)
```

### Deferred Actions

Use a collection to defer transformations until the enclosing parse path succeeds:

```python
actions = h.action_collection()
stashed = h.action_stash(h.ch(b"a"), lambda value: value.upper(), actions)
parser = h.action_apply(stashed, actions)

assert parser.parse(b"a") == b"A"
```

Keep the returned parser alive while using its action collection; the binding retains the
collection for parsers created by `action_stash()` and `action_apply()`.

### Dispatch

`dispatch()` accepts an iterable of `(opcode, parser)` pairs or a mapping. The discriminator must
produce an integer token; the matching parser is selected by that value.

```python
message = h.dispatch(h.uint8(), {1: h.ch(b"a"), 2: h.ch(b"b")}, h.ch(b"z"))
assert message.parse(b"\x01a") == (1, b"a")
```

### Recursive Grammars

Use `h.indirect()` and `.bind()` to define recursive parsers:

```python
expr = h.indirect()
atom = h.ch_range(b"a", b"z")
expr.bind(h.choice(h.sequence(atom, expr), h.epsilon_p()))

result = expr.parse(b"abc")  # (b'a', (b'b', (b'c',)))
```

### Parse Results

- A successful parse returns the parsed value (bytes, int, tuple, or a value returned by an action).
- A failed parse returns `None`.
- `h.optional()` uses `h.Placeholder()` to represent a missing optional element.

### Diagnostics

`parser.parse_debug(data, False)` returns `(result, diagnostic)`. `result` has the same
shape as `parse()`. When tracing is enabled, `diagnostic` is an immutable `ParseDiagnostic` with
an `error` (`ParseFailure`), normalized byte-range/EOF `expected` values, and an execution trace.
It is `None` when Hammer was built without tracing support. `True` also writes Hammer's
native diagnostic report to stderr.

Use `parser.set_label(text)`, `parser.set_error_message(text)`, or
`h.context(parser, label)` to add diagnostic metadata. `context()` captures the Python callsite
unless explicit source fields are supplied.

## Notes

- All byte-oriented parsers expect and return `bytes` objects. Pass input as `b"..."`.
- `ch()` accepts either an integer byte value or a single-byte `bytes` object.
- `ch_range()`, `in_()`, and `not_in()` accept `bytes` arguments only.
- The `sequence()` and `many()` family return Python `tuple` objects.
- Parser memory is managed by the existing binding; `h_parser_free()` is intentionally not
  exposed because Hammer combinators borrow their child parsers.
