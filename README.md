![Hammer Logo](docs/assets/Hammer-Logo-Full-Color-Horizontal.png)

Hammer is a parsing library. Like many modern parsing libraries, it provides a parser combinator interface for writing grammars as inline domain-specific languages, but Hammer also provides a variety of parsing backends. It's also bit-oriented rather than character-oriented, making it ideal for parsing binary data such as images, network packets, audio, and executables.

Hammer is written in C and provides a packrat parsing backend.

## MicroHammer

MicroHammer is a slimmed-down version of Hammer with the goal of providing a lightweight, Linux-focused version of Hammer with a minimal, clean codebase. [Link to public release.](https://github.com/riversideresearch/hammer/releases/)

The main feature of MicroHammer is its significantly smaller codebase, allowing for easier maintenance and onboarding. Key differences from the full Hammer library include:

- Linux-focused development and deployment
- More thorough and consistent documentation
- Windows / macOS not supported
- Packrat parsing backend only
- Language bindings for Python, Java, and C++ (see [Python Bindings](src/bindings/python/README.md), [Java Bindings](src/bindings/java/README.md), [C++ Bindings](src/bindings/cpp/README.md))

## Features

- **Bit-oriented** -- grammars can include single-bit flags or multi-bit constructs that span character boundaries, with no hassle
- **Thread-safe, reentrant** (for most purposes; see Known Issues for details)
- **Benchmarking for parsing backends** -- determine empirically which backend will be most time-efficient for your grammar
- **Parsing backends:** -- currently only Packrat parsing is supported

## Installing

### Prerequisites

- [SCons](http://scons.org/)

```bash
sudo apt install scons
```

### Optional Dependencies for Testing

- pkg-config
- glib-2.0-dev

```bash
sudo apt install pkg-config libglib2.0-dev
```

To build, type `scons`.
To run the built-in test suite, type `scons test`.
To avoid the test dependencies, add `--no-tests`.
For a debug build, add `--variant=debug`.

To make Hammer available system-wide, use `scons install`. This places include files in `/usr/local/include/hammer` and library files in `/usr/local/lib` by default; to install elsewhere, add a `prefix=<destination>` argument, e.g. `scons install prefix=$HOME`.

To remove installed files, use `scons uninstall` (with the same `prefix` if non-default).

To check which variant and version of Hammer is currently installed:

```bash
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig pkg-config --variable=variant libhammer
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig pkg-config --modversion libhammer
```

## Usage

Just `#include <hammer/hammer.h>` (also `#include <hammer/glue.h>` if you plan to use any of the convenience macros) and link with `-lhammer`.

If you've installed Hammer system-wide, you can use `pkg-config` in the usual way.

To learn about hammer, check:

- the [user guide](https://github.com/UpstandingHackers/hammer/wiki/User-guide)
- [Hammer Primer](https://github.com/sergeybratus/HammerPrimer) (outdated in terms of code, but good to get the general thinking)
- [Try Hammer](https://github.com/sboesen/TryHammer)

## Language Bindings

Hammer provides bindings for use from languages other than C.

### Python

Requires [SWIG](https://www.swig.org/) 4.x, Python 3.8+, and `setuptools`.

```bash
sudo apt install swig
scons bindings=python
```

See [src/bindings/python/README.md](src/bindings/python/README.md) for the full API reference and usage guide.

### Java

Requires [SWIG](https://www.swig.org/) 4.x and JDK 11+.

```bash
sudo apt install swig default-jdk
scons bindings=java
```

See [src/bindings/java/README.md](src/bindings/java/README.md) for the full API reference and usage guide.

### C++

Requires `g++` and `libgtest-dev` (for tests).

```bash
sudo apt install libgtest-dev
scons bindings=cpp
```

See [src/bindings/cpp/README.md](src/bindings/cpp/README.md) for the full API reference and usage guide.

## Examples

The `examples/` directory contains some simple examples, currently including:

- [base64](https://en.wikipedia.org/wiki/Base64)
- [DNS](https://en.wikipedia.org/wiki/Domain_Name_System)
- [NTP](https://en.wikipedia.org/wiki/Network_Time_Protocol)
- [TFTP](https://en.wikipedia.org/wiki/Trivial_File_Transfer_Protocol)

See the example-specific READMEs and the wiki for build and walkthrough details.

## Contributing

For information on contributing to Hammer, including development setup, code formatting guidelines, and documentation generation, please see [DEVELOPMENT.md](DEVELOPMENT.md).

## Contact

Send an email to <parsing@riversideresearch.org>

## Acknowledgment

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Prime Contract No. HR001119C0077. Any opinions, findings, and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA).

This work is a fork of the repository found at: <https://gitlab.special-circumstanc.es/hammer/hammer>

Distribution A: Approved for Public Release
