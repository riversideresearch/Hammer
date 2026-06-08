# Hammer Parsing Backends

Hammer exposes several parser backends. Packrat remains the default and is the most general backend for parser-combinator grammars. LL(k), LALR(k), and GLR are restored context-free grammar backends that compile a parser through Hammer's CFG/desugar layer.

## Available Backends

| Enum | Name | Parameters | Typical Use |
| --- | --- | --- | --- |
| `PB_PACKRAT` | `packrat` | none | General Hammer parser-combinator grammars, including non-CFG features supported by the packrat parser. |
| `PB_LL` | `llk` | optional `k` | Predictive context-free grammars where bounded lookahead resolves choices. |
| `PB_LALR` | `lalr` | optional `k` | Deterministic bottom-up context-free grammars. |
| `PB_GLR` | `glr` | optional `k` | Context-free grammars with LALR table conflicts, including ambiguous grammars where one parse is acceptable. |

The default `k` for the context-free backends is currently 1. Pass a numeric parameter by casting it through `void *`, or use the backend-with-params API.

## Combinator Compatibility

The context-free backends (`PB_LL`, `PB_LALR`, and `PB_GLR`) all compile through the same CFG/desugar layer. At the combinator level they therefore accept the same set of Hammer parsers. They still differ in grammar acceptance:

- `PB_LL` can reject CFG-capable grammars that need more lookahead than the supplied `k`, or that are not predictive.
- `PB_LALR` can reject CFG-capable grammars with unresolved LR table conflicts.
- `PB_GLR` accepts LALR table conflicts, but can use more runtime work for ambiguous grammars.

Legend:

- `Yes` means the backend has direct support for that combinator.
- `If children` means the combinator is supported only if all nested parsers are also supported by that backend.
- `Limited` means support exists, but with an important backend-specific restriction.
- `No` means the combinator is not currently representable by that backend.

| Combinator or Parser | Packrat | LL(k) | LALR(k) | GLR | Notes |
| --- | --- | --- | --- | --- | --- |
| `h_ch` | Yes | Yes | Yes | Yes | Single byte terminal. |
| `h_ch_range` | Yes | Yes | Yes | Yes | Desugars to a charset. |
| `h_in` | Yes | Yes | Yes | Yes | Charset terminal. |
| `h_not_in` | Yes | Yes | Yes | Yes | Complement charset terminal. |
| `h_token`, `h_literal` | Yes | Yes | Yes | Yes | Fixed byte string. |
| `h_bits` | Yes | Limited | Limited | Limited | CFG desugaring only handles whole-byte bit lengths. |
| `h_int8`, `h_uint8` | Yes | Yes | Yes | Yes | Whole-byte integer parser. |
| `h_int16`, `h_uint16` | Yes | Yes | Yes | Yes | Whole-byte integer parser. |
| `h_int32`, `h_uint32` | Yes | Yes | Yes | Yes | Whole-byte integer parser. |
| `h_int64`, `h_uint64` | Yes | Yes | Yes | Yes | Whole-byte integer parser. |
| `h_int_range` | Yes | Limited | Limited | Limited | CFG support assumes a whole-byte integer parser. |
| `h_bytes` | Yes | No | No | No | Current implementation has no CFG desugar path. |
| `h_sequence` | Yes | If children | If children | If children | Grammar shape can still affect LL/LALR compilation. |
| `h_choice` | Yes | If children | If children | If children | LL may require larger `k`; LALR may report conflicts. |
| `h_many` | Yes | If children | If children | If children | Repetition desugars to CFG recursion. |
| `h_many1` | Yes | If children | If children | If children | Built from `h_many` plus the repeated parser. |
| `h_repeat_n` | Yes | If children | If children | If children | Fixed repetition. |
| `h_optional` | Yes | If children | If children | If children | Desugars to choice with epsilon. |
| `h_sepBy`, `h_sepBy1` | Yes | If children | If children | If children | Supported when item and separator are supported. |
| `h_left`, `h_right`, `h_middle` | Yes | If children | If children | If children | Convenience wrappers over ignored sequences. |
| `h_ignoreseq` | Yes | If children | If children | If children | Used by the left/right/middle helpers. |
| `h_drop_from` | Yes | If children | If children | If children | Sequence helper that drops selected AST fields. |
| `h_ignore` | Yes | If child | If child | If child | Suppresses the child result. |
| `h_whitespace` | Yes | If child | If child | If child | Wraps another parser with whitespace handling. |
| `h_action` | Yes | If child | If child | If child | Semantic action is preserved through CFG desugaring. |
| `h_attr_bool` | Yes | If child | If child | If child | Predicate is preserved through CFG desugaring. |
| `h_epsilon_p` | Yes | Yes | Yes | Yes | Empty success. |
| `h_end_p` | Yes | Yes | Yes | Yes | End-of-input marker. |
| `h_nothing_p` | Yes | Yes | Yes | Yes | Always fails. |
| `h_indirect` | Yes | Limited | Limited | Limited | Recursive CFGs are supported, but LL rejects left recursion and LALR may reject conflicts. |
| `h_permutation` | Yes | No | No | No | Not CFG-desugared. |
| `h_butnot` | Yes | No | No | No | Not currently marked context-free. |
| `h_difference` | Yes | No | No | No | Not currently marked context-free. |
| `h_xor` | Yes | No | No | No | Not currently marked context-free. |
| `h_length_value` | Yes | No | No | No | Depends on parsed values. |
| `h_and` | Yes | No | No | No | Lookahead predicate is not CFG-desugared. |
| `h_not` | Yes | No | No | No | Negative lookahead is not CFG-desugared. |
| `h_with_endianness` | Yes | No | No | No | Runtime input-state change, not CFG-desugared. |
| `h_put_value` | Yes | No | No | No | Symbol-table state. |
| `h_get_value` | Yes | No | No | No | Symbol-table state. |
| `h_free_value` | Yes | No | No | No | Symbol-table state. |
| `h_bind` | Yes | No | No | No | Continuation depends on a previous parse result. |
| `h_skip` | Yes | No | No | No | Input-position operation. |
| `h_seek` | Yes | No | No | No | Input-position operation. |
| `h_tell` | Yes | No | No | No | Input-position operation. |

## Basic Usage

Packrat is selected automatically for new parsers:

```c
HParser *p = h_sequence(h_ch('a'), h_ch('b'), NULL);
HParseResult *result = h_parse(p, (const uint8_t *)"ab", 2);
```

Select a context-free backend with `h_compile` before parsing:

```c
HParser *ab = h_sequence(h_ch('a'), h_ch('b'), NULL);
HParser *ac = h_sequence(h_ch('a'), h_ch('c'), NULL);
HParser *p = h_choice(ab, ac, NULL);

if (h_compile(p, PB_LL, (void *)2) == 0) {
    HParseResult *result = h_parse(p, (const uint8_t *)"ac", 2);
    h_parse_result_free(result);
}
```

Backend names can be resolved from strings:

```c
HParserBackend be = h_query_backend_by_name("lalr");
```

Backends with parameters can be parsed from strings:

```c
HParserBackendWithParams *be = h_get_backend_with_params_by_name("llk(2)");
h_compile_for_backend_with_params(p, be);
h_free_backend_with_params(be);
```

## Backend Differences

### Packrat

Packrat parses by recursively evaluating the parser combinator graph with memoization and left-recursion support. It is the default because it handles the broadest set of Hammer parser features.

Use it when:

- the grammar uses non-context-free combinators such as binding or symbol-table state
- left recursion is useful
- the grammar is being developed and backend constraints are not yet clear

Limitations:

- memoization can use more memory than table-driven context-free backends
- it is not intended as a streaming table parser

### LL(k)

LL(k) builds a predictive parse table from the context-free form of the parser. It chooses productions by looking ahead up to `k` bytes.

Use it when:

- the grammar is naturally top-down and deterministic
- a small `k` separates alternatives
- incremental parsing with chunks is needed

Limitations:

- left-recursive grammars do not fit LL(k)
- ambiguous alternatives fail compilation unless a larger `k` resolves them
- only parsers that desugar to Hammer's CFG representation are supported

### LALR(k)

LALR(k) builds an LR table and rejects unresolved conflicts. It is suitable for deterministic bottom-up context-free grammars.

Use it when:

- the grammar is context-free and deterministic
- bottom-up parsing is a better fit than predictive LL parsing
- conflicts should be treated as grammar errors

Limitations:

- unresolved shift/reduce or reduce/reduce conflicts fail compilation
- only parsers that desugar to Hammer's CFG representation are supported

### GLR

GLR reuses the LALR table machinery but accepts tables with conflicts. At runtime it forks parser engines for conflicting actions and merges compatible engines.

Use it when:

- the grammar is context-free but not LALR-deterministic
- ambiguity is expected or acceptable
- recovering one successful parse is enough for the caller

Limitations:

- conflicting grammars can use more CPU and memory because the parser explores multiple engines
- ambiguous result reporting is still limited; the restored backend returns one successful result
- iterative chunk parsing is not currently exposed for GLR

## Not Restored

The regex/RVM backend was removed in commit `7b75556a` along with the context-free backends, but it has not been restored here. Its old implementation depends on parser-vtable hooks such as `compile_to_rvm` that are no longer present in the current parser/combinator API. Restoring it cleanly would require rebuilding the RVM compiler interface, not just re-adding the old files.

The old `contextfree.h` compatibility header was also not restored. The current codebase has `cfgrammar.h`, and the restored LR-family backends use that current API through `lr.h`.
