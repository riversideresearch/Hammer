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
