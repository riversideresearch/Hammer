# Hammer Java Bindings

Java bindings for the Hammer parser combinator library, generated with [SWIG](https://www.swig.org/).

## Prerequisites

- [SWIG](https://www.swig.org/) 4.x
- JDK 11+ (`javac`, `jar`)
- Hammer shared library built (see top-level README)

```bash
sudo apt install swig default-jdk
```

## Building

From the repository root, pass `bindings=java` to SCons:

```bash
scons bindings=java
```

This generates `hammer_wrap.c` and a set of Java source files via SWIG, compiles them into the JNI shared library `libhammer_jni.so`, and packages the Java classes into `hammer.jar`. Both artifacts are placed under `build/opt/src/bindings/java/`.

To build both Python and Java bindings at once:

```bash
scons bindings=python,java
```

## Running the Tests

```bash
scons bindings=java testjava
```

Or run the full test suite including all language bindings:

```bash
scons bindings=all test
```

## Usage

Add `hammer.jar` to your classpath and ensure `libhammer_jni.so` (and `libhammer.so`) are on the library path at runtime.

```bash
javac -cp build/opt/src/bindings/java/hammer.jar MyParser.java
LD_LIBRARY_PATH=build/opt/src:build/opt/src/bindings/java \
  java -Djava.library.path=build/opt/src/bindings/java \
       -cp .:build/opt/src/bindings/java/hammer.jar \
       MyParser
```

If you ran `scons install` (or `scons bindings=java installjava`), `hammer.jar` is installed to `/usr/local/share/java/` and `libhammer_jni.so` to `/usr/local/lib`.

### Loading the Native Library

Your application must load the JNI library once at startup, typically in a static initializer:

```java
static {
    System.loadLibrary("hammer_jni");
}
```

### Basic Example

```java
import com.riversideresearch.hammer.*;

public class Example {
    static {
        System.loadLibrary("hammer_jni");
    }

    public static void main(String[] args) {
        // Parse the literal bytes "GET "
        HParser method = hammer.h_token(new byte[]{'G','E','T',' '});

        // Parse one or more printable ASCII characters
        HParser printable = hammer.h_many1(
            hammer.h_ch_range((short)0x21, (short)0x7e)
        );

        // Sequence: method followed by the path
        HParser requestLine = hammer.h_sequence__a(new HParser[]{method, printable});

        HParseResult result = requestLine.parse("GET /index.html".getBytes());
        if (result == null) {
            System.out.println("Parse failed");
        } else {
            HParsedToken ast = result.getAst();
            System.out.println("Parsed sequence of length " + ast.seqLength());
        }
    }
}
```

### Parser Combinators

All Hammer functions are exposed as static methods of the `hammer` class in the `com.riversideresearch.hammer` package.

| Java call                                 | Description                                           |
| ----------------------------------------- | ----------------------------------------------------- |
| `hammer.h_token(byte[])`                  | Match a literal byte array                            |
| `hammer.h_ch((short)b)`                   | Match a single byte value (0–255)                     |
| `hammer.h_ch_range((short)lo, (short)hi)` | Match any byte in `[lo, hi]`                          |
| `hammer.h_in(byte[])`                     | Match any byte in the given charset                   |
| `hammer.h_not_in(byte[])`                 | Match any byte not in the given charset               |
| `hammer.h_sequence__a(HParser[])`         | Match each parser in order; result is `TT_SEQUENCE`   |
| `hammer.h_choice__a(HParser[])`           | Try each parser in order; return first success        |
| `hammer.h_many(p)`                        | Match `p` zero or more times; result is `TT_SEQUENCE` |
| `hammer.h_many1(p)`                       | Match `p` one or more times; result is `TT_SEQUENCE`  |
| `hammer.h_repeat_n(p, n)`                 | Match `p` exactly `n` times; result is `TT_SEQUENCE`  |
| `hammer.h_optional(p)`                    | Match `p` or produce a `TT_NONE` token on failure     |
| `hammer.h_ignore(p)`                      | Match `p` but suppress its result from sequences      |
| `hammer.h_sepBy(p, sep)`                  | Match `p` separated by `sep`, zero or more times      |
| `hammer.h_sepBy1(p, sep)`                 | Match `p` separated by `sep`, one or more times       |
| `hammer.h_left(p1, p2)`                   | Match both; return result of `p1`                     |
| `hammer.h_right(p1, p2)`                  | Match both; return result of `p2`                     |
| `hammer.h_middle(p1, p2, p3)`             | Match all three; return result of `p2`                |
| `hammer.h_butnot(p1, p2)`                 | Match `p1` only if `p2` does not also match           |
| `hammer.h_difference(p1, p2)`             | Match `p1` only when `p2` matches less input          |
| `hammer.h_xor(p1, p2)`                    | Match exactly one of `p1` or `p2`, not both           |
| `hammer.h_and(p)`                         | Succeed if `p` would match, but consume no input      |
| `hammer.h_not(p)`                         | Succeed if `p` would not match, consuming no input    |
| `hammer.h_whitespace(p)`                  | Skip leading whitespace, then match `p`               |
| `hammer.h_int_range(p, lo, hi)`           | Match `p` only if the integer result is in `[lo, hi]` |
| `hammer.h_epsilon_p()`                    | Always succeed, consuming no input                    |
| `hammer.h_end_p()`                        | Succeed only at end of input                          |
| `hammer.h_nothing_p()`                    | Always fail                                           |
| `hammer.h_put_value(p, name)`             | Parse `p` and store the result under `name`           |
| `hammer.h_get_value(name)`                | Retrieve a previously stored value by `name`          |
| `hammer.h_free_value(name)`               | Retrieve and free a previously stored value by `name` |

### Integer Parsers

```java
hammer.h_uint8()    // unsigned 8-bit
hammer.h_uint16()   // unsigned 16-bit, big-endian
hammer.h_uint32()   // unsigned 32-bit, big-endian
hammer.h_uint64()   // unsigned 64-bit, big-endian
hammer.h_int8()     // signed 8-bit
hammer.h_int16()    // signed 16-bit, big-endian
hammer.h_int32()    // signed 32-bit, big-endian
hammer.h_int64()    // signed 64-bit, big-endian
```

### Inspecting Parse Results

A successful parse returns an `HParseResult`; a failed parse returns `null`. Call `result.getAst()` to retrieve the `HParsedToken` and inspect it using the methods below.

`HParseResult` owns the memory for the entire parse tree. Do not hold references to tokens returned by `getAst()` or `seqElement()` after the `HParseResult` has been garbage-collected or explicitly deleted.

| Method                | Returns        | Description                                                                                 |
| --------------------- | -------------- | ------------------------------------------------------------------------------------------- |
| `token.tokenType()`   | `int`          | One of the `TT_*` constants below                                                           |
| `token.sintValue()`   | `long`         | Signed integer value (`TT_SINT` tokens)                                                     |
| `token.uintValue()`   | `long`         | Unsigned integer value (`TT_UINT` tokens); treat as unsigned with `Long.toUnsignedString()` |
| `token.seqLength()`   | `long`         | Number of elements (`TT_SEQUENCE` tokens)                                                   |
| `token.seqElement(i)` | `HParsedToken` | The `i`-th sequence element                                                                 |
| `token.bytesLength()` | `long`         | Byte count (`TT_BYTES` tokens)                                                              |
| `token.byteAt(i)`     | `short`        | Byte value at index `i` (0–255), or -1 if out of range                                      |

### Token Type Constants

Compare `token.tokenType()` against these values:

| Constant      | Value | Produced by                                       |
| ------------- | ----- | ------------------------------------------------- |
| `TT_NONE`     | 1     | `h_optional()` on failure, `h_end_p()`, `h_and()` |
| `TT_BYTES`    | 2     | `h_token()`, `h_in()`, `h_not_in()`               |
| `TT_SINT`     | 4     | `h_int8/16/32/64()`                               |
| `TT_UINT`     | 8     | `h_uint8/16/32/64()`, `h_ch()`, `h_ch_range()`    |
| `TT_SEQUENCE` | 16    | `h_sequence__a()`, `h_many()`, `h_sepBy()`, etc.  |

### Recursive Grammars

Use `h_indirect()` and `h_bind_indirect()` to define recursive parsers:

```java
HParser expr = hammer.h_indirect();
HParser atom = hammer.h_ch_range((short)'a', (short)'z');
hammer.h_bind_indirect(expr,
    hammer.h_choice__a(new HParser[]{
        hammer.h_sequence__a(new HParser[]{atom, expr}),
        hammer.h_epsilon_p(),
    })
);

HParseResult result = expr.parse("abc".getBytes());
```

## Notes

- Java's `byte` type is signed (-128 to 127). Single byte values passed to `h_ch()` and `h_ch_range()` use `short` (0–255) to avoid sign confusion.
- `h_sequence__a()` and `h_choice__a()` accept `HParser[]` arrays and are the recommended way to build combinators in Java. The sentinel-terminated varargs versions (`h_sequence`, `h_choice`) are not exposed.
- `uintValue()` returns a Java `long`. For values above `Long.MAX_VALUE` use `Long.toUnsignedString(token.uintValue())`.
- `HParseResult` implements a finalizer that calls `h_parse_result_free`, so explicit cleanup is not required, but calling `result.delete()` eagerly is good practice in tight loops.
