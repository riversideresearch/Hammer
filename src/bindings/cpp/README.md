# Hammer C++ Bindings

A header-only C++ wrapper around the Hammer parser combinator library.

## Prerequisites

- `g++` (C++14 or later)
- Hammer shared library built (see top-level README)
- `libgtest-dev` for running tests

```bash
sudo apt install libgtest-dev
```

## Building

```bash
scons bindings=cpp
```

## Running the Tests

```bash
scons bindings=cpp testcpp
```

## Usage

Add `src/bindings/cpp` (or the install prefix) to your include path, then include the header and link against `libhammer`:

```bash
g++ -std=c++14 -I/usr/local/include/hammer -o myparser myparser.cpp -lhammer
```

### Basic Example

```cpp
#include <hammer/hammer.hpp>
#include <iostream>

static {
    System.loadLibrary("hammer_jni");
}

int main() {
    using namespace hammer;

    // Match the literal bytes "GET "
    Parser method = Token("GET ");

    // Match one or more printable ASCII bytes
    Parser printable = Many1(ChRange(0x21, 0x7e));

    // Sequence: method then path
    Parser request = Sequence(method, printable, NULL);

    ParseResult result = request.parse("GET /index.html");
    if (!result) {
        std::cout << "Parse failed\n";
    } else {
        std::cout << result.asUnambiguous() << "\n";
    }
    return 0;
}
```

### Parser Combinators

All combinators live in the `hammer` namespace.

| C++ call                          | Description                                            |
|-----------------------------------|--------------------------------------------------------|
| `Token(str)`                      | Match a literal string                                 |
| `Ch(byte)`                        | Match a single byte value                              |
| `ChRange(lo, hi)`                 | Match any byte in `[lo, hi]`                           |
| `In(str)`                         | Match any byte in the given charset                    |
| `NotIn(str)`                      | Match any byte not in the given charset                |
| `Sequence(p, ..., NULL)`          | Match each parser in order; result is `TT_SEQUENCE`    |
| `Choice(p, ..., NULL)`            | Try each parser in order; return first success         |
| `Many(p)`                         | Match `p` zero or more times; result is `TT_SEQUENCE`  |
| `Many1(p)`                        | Match `p` one or more times; result is `TT_SEQUENCE`   |
| `RepeatN(p, n)`                   | Match `p` exactly `n` times; result is `TT_SEQUENCE`   |
| `Optional(p)`                     | Match `p` or produce a `TT_NONE` token on failure      |
| `Ignore(p)`                       | Match `p` but suppress its result from sequences       |
| `SepBy(p, sep)`                   | Match `p` separated by `sep`, zero or more times       |
| `SepBy1(p, sep)`                  | Match `p` separated by `sep`, one or more times        |
| `Left(p, q)`                      | Match both; return result of `p`                       |
| `Right(p, q)`                     | Match both; return result of `q`                       |
| `Middle(p, q, r)`                 | Match all three; return result of `q`                  |
| `ButNot(p, q)`                    | Match `p` only if `q` does not also match              |
| `Difference(p, q)`                | Match `p` only when `q` matches less input             |
| `Xor(p, q)`                       | Match exactly one of `p` or `q`, not both              |
| `And(p)`                          | Succeed if `p` would match, consuming no input         |
| `Not(p)`                          | Succeed if `p` would not match, consuming no input     |
| `Whitespace(p)`                   | Skip leading whitespace, then match `p`                |
| `IntRange(p, lo, hi)`             | Match `p` only if the integer result is in `[lo, hi]`  |
| `Epsilon()`                       | Always succeed, consuming no input                     |
| `End()`                           | Succeed only at end of input                           |
| `Nothing()`                       | Always fail                                            |
| `Action(p, fn)`                   | Apply action function `fn` to parse result of `p`      |
| `AttrBool(p, pred)`               | Accept result of `p` only if predicate `pred` is true  |

### Integer Parsers

```cpp
hammer::Uint8()    // unsigned 8-bit
hammer::Uint16()   // unsigned 16-bit, big-endian
hammer::Uint32()   // unsigned 32-bit, big-endian
hammer::Uint64()   // unsigned 64-bit, big-endian
hammer::Int8()     // signed 8-bit
hammer::Int16()    // signed 16-bit, big-endian
hammer::Int32()    // signed 32-bit, big-endian
hammer::Int64()    // signed 64-bit, big-endian
```

### Inspecting Parse Results

`ParseResult` is truthy on success and falsy on failure. Use `asUnambiguous()` for a human-readable representation, or `getAST()` to get the raw `ParsedToken` wrapper.

`ParseResult` owns the parse tree and frees it in its destructor.

### Recursive Grammars

Use `Indirect` and `bind()` to define recursive parsers:

```cpp
using namespace hammer;

Indirect expr;
Parser atom = ChRange('a', 'z');
expr.bind(Choice(Sequence(atom, expr, NULL), Epsilon(), NULL));

ParseResult result = expr.parse("abc");
```

### Testing Helpers

Include `<hammer/hammer_test.hpp>` to get gtest assertion helpers:

```cpp
#include <hammer/hammer_test.hpp>

TEST(MyTest, Example) {
    hammer::Parser p = hammer::Ch('a');
    EXPECT_TRUE(ParsesTo(p, "a", "u0x61"));
    EXPECT_TRUE(ParseFails(p, "b"));
    EXPECT_TRUE(ParsesOK(p, "a"));
}
```
