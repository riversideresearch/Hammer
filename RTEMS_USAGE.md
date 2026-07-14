# Porting Hammer to RTEMS 5

This document describes the port of the Hammer parser combinator library
to RTEMS 5.0.0 SPARC (GR712RC LEON3, tested under TSIM eval).

It covers two things:
1. Building and running Hammer on RTEMS/SPARC.
2. The optional **static (no-heap) memory mode**, in which Hammer allocates
   entirely from fixed static buffers instead of the heap. Sections that only
   apply to that mode are marked **[STATIC]**.

## Build requirements

- RTEMS 5.0.0 toolchain at a known prefix
  - SPARC: rcc-1.3.1-gcc (Gaisler RTEMS 5 toolchain)
- A target BSP installed alongside the toolchain
  - SPARC: `gr712rc`
- GNU Make
- A simulator or hardware to run the resulting executable
  - SPARC: TSIM used to test

## Build

The Makefile accepts a target via two variables:

    make RTEMS_PREFIX=<prefix> RTEMS_BSP=<arch>/<bsp>

Examples:

    # SPARC
    make RTEMS_PREFIX=/opt/rcc-1.3.1-gcc \
         RTEMS_BSP=sparc-gaisler-rtems5/gr712rc

Run `make install` to stage `libhammer.a` and the public headers into
`$INSTALL_PREFIX` (default `$HOME/install`). Run `make install-tests`
to additionally stage `libhammer_tests.a`.

## Linking your application

    <gcc> <target-flags> \
      -B<RTEMS_PREFIX>/<arch>/<bsp>/lib \
      -specs bsp_specs -qrtems \
      -I<RTEMS_PREFIX>/<arch>/<bsp>/lib/include \
      -I$HOME/install/include \
      your_app.c \
      -L$HOME/install/lib -lhammer \
      -o your_app.exe

ex. (for code below)

    sparc-gaisler-rtems5-gcc -mcpu=leon3 -mfix-gr712rc \
      -B/opt/rcc-1.3.1-gcc/sparc-gaisler-rtems5/gr712rc/lib \
      -specs bsp_specs -qrtems \
      -I/opt/rcc-1.3.1-gcc/sparc-gaisler-rtems5/gr712rc/lib/include \
      -I$HOME/install/include \
      -I$HOME/install/tests \
      init.c \
      -L$HOME/install/tests \
      -L$HOME/install/lib -lhammer_tests -lhammer \
      -o hammer_smoke.exe

For test apps, add `-I$HOME/install/tests -L$HOME/install/tests -lhammer_tests`,
placing `-lhammer_tests` BEFORE `-lhammer`.

### NOTES

* When doing `make tests` you will see many warnings of functions being
  redefined in the different test files. This is intended, as glib is not
  supported on rtems, so any functions provided by it were redefined for the
  rtems build.
* There is a CFlag named `SIMULATOR_LIMITED`. This was added because some tests
  take too long in the simulator, preventing the suite from fully running. The
  flag can be safely removed if not running on a simulator.
* Running `make install` or `make install-tests` installs everything needed to
  link, into `~/install/` by default, which the linking instructions above
  assume.
* Some desugar tests were removed, as they did not verify any functionality —
  they only verified desugar output formatting.

## Static (no-heap) memory mode  **[STATIC]**

By default Hammer allocates from the heap (`malloc`), exactly as the upstream
non-RTEMS build does. For targets where the heap is not permitted, Hammer can
instead allocate entirely from two fixed-size static buffers. This mode is
**opt-in** — nothing below changes unless the application enables it, so the
default and host builds are unaffected.

### What it is

A custom allocator (`static_alloc.c` / `static_alloc.h`) provides two bump
allocators over statically-declared buffers (in `.bss`, no heap, no flash
cost):

- **Grammar pool**: serves grammar construction (the parser nodes created by
  `h_ch`, `h_sequence`, etc.) and compilation (backend vtables). Written once
  at startup, never reclaimed; the grammar is fixed for the program lifetime.
- **Parse pool**: serves everything a single parse allocates (arena, packrat
  memo table, parse state, input stream, output tokens). It is **reset, not
  freed, between parses**; the same region is reused each parse.

Both allocators return 8-byte-aligned pointers (SPARC traps on misaligned
access). On overflow they halt with a diagnostic rather than corrupting, since
there is no heap to fall back on.

### Difference from the normal (heap) build

| | Normal build (default / host) | Static build **[STATIC]** |
|---|---|---|
| Allocation source | heap via `malloc` | two static `.bss` buffers |
| Combinator calls | `h_ch('a')` etc. | same source, if default allocators are repointed (below) |
| Per-parse cleanup | automatic (arena freed) | app calls `h_parse_pool_reset()` after each parse |
| realloc | supported | **not supported** — halts if called (see `STATIC_MEM`) |
| Input model | single-shot or chunked | single-shot only (`h_parse`); chunked streaming not supported |
| Enabled by | nothing (default) | calling `h_static_alloc_init()` + repointing defaults |

### How the application uses it

Hammer routes all allocation through its `HAllocator` interface, and every
plain combinator (`h_ch`, `h_sequence`, `h_parse`, ...) is a thin wrapper over
an allocator-taking `__m` variant (`h_ch__m`, `h_parse__m`, ...). The plain
wrappers consult two global defaults:

    h_default_allocator        // used by construction / compile wrappers
    h_default_parse_allocator  // used by the h_parse wrapper

Both default to the heap allocator, so behavior is unchanged until the
application repoints them. To enable static mode, at startup:

    #include "static_alloc.h"

    h_static_alloc_init();                          // set up the pools (call FIRST)
    h_default_allocator       = h_grammar_allocator; // construction -> grammar pool
    h_default_parse_allocator = h_parse_allocator;   // parsing      -> parse pool

After that, ordinary Hammer code is heap-free with no source changes at the
call site:

    HParser *p = h_sequence(h_ch('a'), h_many(h_ch('b')), NULL);
    h_compile(p, PB_PACKRAT, NULL);

    HParseResult *r = h_parse(p, input, len);
    /* ... use r ... */
    h_parse_pool_reset();       // REQUIRED after each parse (see below)

You may also call the `__m` forms explicitly and pass the allocators directly,
if you prefer not to rely on the globals:

    HAllocator *g = h_grammar_allocator;
    HParser *p = h_sequence__m(g, h_ch__m(g, 'a'), h_many__m(g, h_ch__m(g, 'b')), NULL);
    h_compile__m(g, p, PB_PACKRAT, NULL);
    HParseResult *r = h_parse__m(h_parse_allocator, p, input, len);

### The one rule you must follow: reset after each parse **[STATIC]**

The parse pool does not free individual allocations; it reclaims memory only
when reset. Call `h_parse_pool_reset()` after you are done reading each
`HParseResult`. **The reset invalidates everything the parse produced,
including the result**, so read/copy anything you need out of it first. Only
the application knows when it is done with the result, so this step cannot be
automatic.

Failing to reset causes the parse pool to fill monotonically across parses and
eventually halt with "parse pool exhausted".

### Pool sizes **[STATIC]**

Sizes are compile-time constants in `static_alloc.c`:

    GRAMMAR_POOL_SIZE   // e.g. 256 KB
    PARSE_POOL_SIZE     // e.g. sized to worst-case parse + margin

The grammar pool size can be justified by measuring `h_grammar_pool_used()`
after construction. The parse pool size depends on the worst-case input and
grammar; the packrat memo table dominates it and grows roughly with input
length. If a pool is undersized, the allocator halts loudly on overflow rather
than corrupting, so undersizing is caught immediately on the first run that
exceeds it. `h_parse_pool_high_water()` reports the largest parse footprint
seen so far, which is useful for tuning `PARSE_POOL_SIZE`.


### Build flags related to static mode **[STATIC]**

* `STATIC_BUILD`: excludes tests and code paths that exercise `realloc`, which
  the static allocator does not support. Find this flag in the CFLAGS in the Makefile. The static allocator's own
  `realloc` deliberately halts if called; this is a tripwire, not a bug; it
  signals that an unexpected code path tried to realloc.

Flag relationship: `RTEMS_BUILD` means "building for RTEMS at all" (may still
use the heap). `STATIC_BUILD` is a narrowing of that: "on RTEMS *and* using the
static pools, so no heap and no realloc." A build can be `RTEMS_BUILD` without
`STATIC_BUILD` (the heap-based RTEMS build); `STATIC_BUILD` without `RTEMS_BUILD`
is not meaningful.

## Example rtems test

```c
#include <rtems.h>
#include <stdio.h>
#include <stdlib.h>
#include "hammer.h"
#include "rtems_test_suite.h"
#include <sys/stat.h>

extern void register_all_hammer_tests(void);

rtems_task Init(rtems_task_argument arg) {
    (void) arg;
    int r = mkdir("/tmp", 0777);
    g_test_init(NULL, NULL, NULL);
    register_all_hammer_tests();
    g_run_test();
    exit(0);
}

/* ---- RTEMS configuration ---- */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_MAXIMUM_TASKS            4
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT_TASK_STACK_SIZE     (128 * 1024)
#define CONFIGURE_INIT
#define CONFIGURE_FILESYSTEM_IMFS
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 16
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT
#include <rtems/confdefs.h>
```

## Example rtems test — static memory **[STATIC]**

A minimal program that builds a `ch` + `sequence` parser and parses one input,
using the static pools explicitly. 

```c
#include <rtems.h>
#include <stdio.h>
#include <stdlib.h>
#include "hammer.h"
#include "static_alloc.h"

rtems_task Init(rtems_task_argument arg) {
    (void) arg;

    /* Set up the static pools before any allocation. */
    h_static_alloc_init();
    h_default_allocator = h_grammar_allocator;
    h_default_parse_allocator = h_parse_allocator;

    /* Build "ab": a sequence of h_ch('a') then h_ch('b'), on the grammar pool. */
    HParser *p = h_sequence(h_ch('a'),h_ch('b'),NULL);
    h_compile(p, PB_PACKRAT, NULL);

    /* Parse "ab" on the parse pool. */
    const uint8_t input[] = { 'a', 'b' };
    HParseResult *r = h_parse(p, input, 2);

    if (r && r->ast)
        printf("PARSE OK  (parse pool used: %lu bytes)\n",
               (unsigned long)h_parse_pool_used());
    else
        printf("PARSE FAILED\n");

    /* Done reading r — reclaim the parse pool. This invalidates r. */
    h_parse_pool_reset();

    exit(0);
}

/* ---- RTEMS configuration ---- */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_MAXIMUM_TASKS            4
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT_TASK_STACK_SIZE     (128 * 1024)
#define CONFIGURE_INIT
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT
#include <rtems/confdefs.h>
```

Build command (no test library needed — this links only against `libhammer`):

    sparc-gaisler-rtems5-gcc -mcpu=leon3 -mfix-gr712rc -B/opt/rcc-1.3.1-gcc/sparc-gaisler-rtems5/gr712rc/lib -specs bsp_specs -qrtems -I/opt/rcc-1.3.1-gcc/sparc-gaisler-rtems5/gr712rc/lib/include -I$HOME/install/include static_demo.c -L$HOME/install/lib -lhammer -o static_demo.exe


Expected output:

    PARSE OK  (parse pool used: <N> bytes)
