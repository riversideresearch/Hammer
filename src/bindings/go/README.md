# Hammer Go Bindings

Go bindings for the Hammer parser combinator library, generated with [SWIG](https://www.swig.org/) and CGO.

## Prerequisites

* [Go](https://go.dev/) 1.22+
* [SWIG](https://www.swig.org/) 4.x
* Hammer shared library built (see top-level README)

```bash
sudo apt install golang swig
```

Verify your Go version:

```bash
go version
```

## Building

From the repository root, pass `bindings=go` to SCons:

```bash
scons bindings=go
```

This:

1. Copies the SWIG interface into the build tree
2. Generates Go wrapper sources with SWIG
3. Builds the Go package using CGO
4. Links against the Hammer shared library

Generated files are placed under:

```text
build/opt/src/bindings/go/
```

Typical generated artifacts include:

```text
hammer.go
hammer_wrap.c
go.mod
```

These are build artifacts and do not need to be committed.

## Running the Tests

```bash
scons bindings=go testgo
```

Or run the tests manually after building:

```bash
cd build/opt/src/bindings/go

LD_LIBRARY_PATH=../.. \
CGO_LDFLAGS="-L../.. -lhammer" \
go test ./...
```

## Installing

Install the generated package into your Go environment:

```bash
scons bindings=go installgo
```

Or manually:

```bash
cd build/opt/src/bindings/go
go install
```

## Usage

After building, import the generated package:

```go
package main

import (
    "fmt"
    "hammer"
)

func main() {
    parser := hammer.Token([]byte("GET "))
    result := parser.Parse([]byte("GET /index.html"))

    fmt.Println(result)
}
```

If the package was not installed system-wide, you may need to use a local module replacement or run from inside the generated build directory.

## CGO Notes

The bindings use CGO to call the Hammer shared library.

At runtime, the Hammer shared library must be discoverable by the dynamic linker.

Typical Linux setup:

```bash
export LD_LIBRARY_PATH=/path/to/libhammer:$LD_LIBRARY_PATH
```

Or pass it inline:

```bash
LD_LIBRARY_PATH=../.. go test
```

## Module Support

The build generates a temporary `go.mod` file compatible with Go 1.22 module mode.

No GOPATH setup is required.

## Basic Example

```go
package main

import (
    "fmt"
    "hammer"
)

func main() {
    // Parse the literal bytes "GET "
    method := hammer.Token([]byte("GET "))

    // Parse one or more printable ASCII characters
    printable := hammer.Many1(
        hammer.ChRange(byte(0x21), byte(0x7e)),
    )

    // Sequence: method followed by the path
    requestLine := hammer.Sequence(method, printable)

    result := requestLine.Parse([]byte("GET /index.html"))

    fmt.Println(result)
}
```

## Parser Combinators

| Go function                    | Description                                             |
| ------------------------------ | ------------------------------------------------------- |
| `hammer.Token([]byte("..."))`  | Match a literal byte string                             |
| `hammer.Ch(b)`                 | Match a single byte                                     |
| `hammer.ChRange(lo, hi)`       | Match any byte in `[lo, hi]`                            |
| `hammer.In([]byte(...))`       | Match any byte in the given charset                     |
| `hammer.NotIn([]byte(...))`    | Match any byte not in the given charset                 |
| `hammer.Sequence(p1, p2, ...)` | Match each parser in order                              |
| `hammer.Choice(p1, p2, ...)`   | Try each parser in order and return the first success   |
| `hammer.Many(p)`               | Match `p` zero or more times                            |
| `hammer.Many1(p)`              | Match `p` one or more times                             |
| `hammer.RepeatN(p, n)`         | Match `p` exactly `n` times                             |
| `hammer.Optional(p)`           | Match `p` or produce a placeholder result               |
| `hammer.Ignore(p)`             | Match `p` but suppress its result                       |
| `hammer.SepBy(p, sep)`         | Match `p` separated by `sep`, zero or more times        |
| `hammer.SepBy1(p, sep)`        | Match `p` separated by `sep`, one or more times         |
| `hammer.Left(p1, p2)`          | Match both parsers and return the result of `p1`        |
| `hammer.Right(p1, p2)`         | Match both parsers and return the result of `p2`        |
| `hammer.Middle(p1, p2, p3)`    | Match all three parsers and return the result of `p2`   |
| `hammer.ButNot(p1, p2)`        | Match `p1` only if `p2` does not also match             |
| `hammer.Difference(p1, p2)`    | Match `p1` only when `p2` matches less input            |
| `hammer.Xor(p1, p2)`           | Match exactly one of `p1` or `p2`                       |
| `hammer.And(p)`                | Positive lookahead; consume no input                    |
| `hammer.Not(p)`                | Negative lookahead; consume no input                    |
| `hammer.Whitespace(p)`         | Skip leading whitespace before matching `p`             |
| `hammer.Action(p, fn)`         | Apply `fn` to the result of `p`                         |
| `hammer.AttrBool(p, fn)`       | Match `p` only if predicate `fn` returns `true`         |
| `hammer.IntRange(p, lo, hi)`   | Match `p` only if integer result is in `[lo, hi]`       |
| `hammer.Indirect()`            | Create a forward-declared parser for recursive grammars |
| `hammer.EpsilonP()`            | Always succeed without consuming input                  |
| `hammer.EndP()`                | Succeed only at end of input                            |
| `hammer.NothingP()`            | Always fail                                             |
| `hammer.PutValue(p, name)`     | Parse `p` and store the result under `name`             |
| `hammer.GetValue(name)`        | Retrieve a previously stored value                      |
| `hammer.FreeValue(name)`       | Retrieve and free a previously stored value             |

## Integer Parsers

```go
hammer.Uint8()
hammer.Uint16()
hammer.Uint32()
hammer.Uint64()

hammer.Int8()
hammer.Int16()
hammer.Int32()
hammer.Int64()
```

## Actions and Predicates

```go
package main

import (
    "fmt"
    "hammer"
)

func main() {
    digits := hammer.Action(
        hammer.Many1(
            hammer.ChRange(byte('0'), byte('9')),
        ),
        func(v interface{}) interface{} {
            return v
        },
    )

    evenByte := hammer.AttrBool(
        hammer.Uint8(),
        func(v interface{}) bool {
            n := v.(uint8)
            return n%2 == 0
        },
    )

    fmt.Println(digits)
    fmt.Println(evenByte)
}
```

## Recursive Grammars

Recursive grammars use indirect parsers:

```go
package main

import (
    "fmt"
    "hammer"
)

func main() {
    expr := hammer.Indirect()

    atom := hammer.ChRange(byte('a'), byte('z'))

    expr.Bind(
        hammer.Choice(
            hammer.Sequence(atom, expr),
            hammer.EpsilonP(),
        ),
    )

    result := expr.Parse([]byte("abc"))

    fmt.Println(result)
}
```

## Parse Results

* A successful parse returns the parsed value.
* A failed parse returns `nil`.
* `hammer.Optional()` may return a placeholder value when the parser does not match.
* Sequence and repetition combinators typically return slices or tuples of parsed values depending on the generated SWIG bindings.

## Notes

* Hammer parsers are byte-oriented.
* Use `[]byte` for parser input.
* `hammer.Ch()` accepts either a byte value or a single-byte character.
* `hammer.ChRange()`, `hammer.In()`, and `hammer.NotIn()` operate on byte ranges and byte slices.
* SWIG-generated APIs may not feel fully idiomatic compared to handwritten Go wrappers.
* The bindings require CGO and therefore a working C toolchain.
* The generated wrapper files are temporary build outputs and should not normally be committed.