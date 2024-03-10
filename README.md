# JSONST - A streaming JSON library

A minimal JSON streaming parsing library with the following features and design goals:

- Fully JSON compliant (whatever that means, see [#standards-compliance](#standards-compliance))
- Suited for embedded usage
- C11, easy to use from C++
- Low memory usage
- No dynamic allocation (malloc)
- No dependencies, not even stdlib, but users have to do their own number parsing (e.g. `strtod()` from `<stdlib.h>`)

Fully tested (GTest, C++17), passes https://github.com/nst/JSONTestSuite.

The real reason I wrote this was that I read https://nullprogram.com/blog/2023/09/27/
and wanted to play around with arena allocation in C.

## Documentation & Usage

- Documentation: [src/jsonst.h](src/jsonst.h) (and other header files)
- Examples: [examples](examples)

No binaries are provided.
To include the library in your project, simply copy the relevant files from `src/`:

- `jsonst.h`, `jsonst.c`: Core library, this is the only thing you really need
- `jsonst_cstd.h`: Thin wrapper to allow usage of C `FILE *` streams
- `jsonst_util.h`, `jsonst_util.c`: Helpers to convert library ENUMs to strings

## Standards compliance

To the best of my knowledge, this library conforms to the following specifications:

- https://www.json.org/json-en.html
- https://datatracker.ietf.org/doc/html/rfc8259
  - Only UTF-8 is supported
  - No support for BOM
  - Max length of keys and strings is fixed (runtime configurable)

## Development

Requirements:

1. Bazel (https://bazel.build/install/ubuntu#install-on-ubuntu)
2. `apt-get install clang-format clang-tidy clangd gcc g++`

Commands:

```bash
# Run all tests
bazel test //...
# Build all binaries
bazel build //...
# Run clang-format
./format.sh
# Refresh compile commands database for clangd
bazel run //:refresh_compile_commands
# Run clang-tidy
bazel build //... --config clang-tidy
```
