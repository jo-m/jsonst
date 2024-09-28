# JSONST - A streaming JSON library

A minimal JSON streaming parsing library with the following features and design goals:

- Fully JSON compliant (whatever that means, see [#standards-compliance](#standards-compliance))
- Suited for embedded usage
- C11, easy to use from C++
- Low memory usage
- No dynamic allocation (malloc)
- No dependencies, not even stdlib (users have to do their own number parsing though, e.g. `strtod()` from `<stdlib.h>`)

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
  - Max length of strings incl. object keys is fixed but runtime configurable

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

### Inside Docker

```fish
docker run \
  -it --rm \
  --user 0 \
  --volume (pwd):/home/ubuntu/src/ \
  --workdir=/home/ubuntu/src/ \
  --entrypoint=/bin/bash \
  gcr.io/bazel-public/bazel:latest
```

## Fuzz

```fish
sudo apt-get install afl++
cd fuzz
make

# Test the target
./fuzztarget ../src/testdata/test.json
# Get some seeds
cp bazel-src/external/com_github_nst_json_test_suite/test_*/*.json fuzz/seeds/
# FUZZ!!
echo core | sudo tee /proc/sys/kernel/core_pattern
set -x AFL_SKIP_CPUFREQ 1
afl-fuzz -i seeds/ -o fuzz_out/ -- ./fuzztarget '@@'
```
