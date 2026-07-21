# Hammer v3 source migration tool

This is a local, optional migration aid for testing existing C and C++ projects against Hammer v3.
It is not installed or included in Hammer packages.

## Requirements

- Python 3.8 or newer
- GNU Make, only if you want to use the build target

The tool is a standalone Python script, so compilation is not required. You can run it directly
from this directory.

## Build a local runnable copy

```sh
cd tools/hammer-v3-migration
make build
```

This creates `build/hammer-migrate-v3` inside this directory. It does not install anything.

Run its tests with:

```sh
make test
```

## Test it on a project

Always start in preview mode. The first command prints a unified diff without changing the project:

```sh
./build/hammer-migrate-v3 /path/to/existing/project
```

After reviewing the diff, apply it explicitly:

```sh
./build/hammer-migrate-v3 --write /path/to/existing/project
```

Check whether a project still needs changes without printing a diff:

```sh
./build/hammer-migrate-v3 --check /path/to/existing/project
```

`--check` exits with status 1 when migrations are needed and 0 when no changes are found.

The tool scans `.c`, `.cc`, `.cpp`, `.cxx`, `.h`, `.hh`, `.hpp`, and `.hxx` files. It skips common
generated or third-party directories, comments, and string literals. It only rewrites recognized
Hammer public-structure accesses, but you should still review the result and run the target
project's tests before committing it.

Remove the local build copy with:

```sh
make clean
```
