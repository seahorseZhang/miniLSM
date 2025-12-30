# minilsm

A minimal C++ lsm library and tests.

## Quick build & test (system gtest)

1. Create build directory and configure:

```bash
mkdir -p build
cd build
cmake ..
```

2. Build:

```bash
cmake --build . -j
```

3. Run tests:

```bash
ctest --output-on-failure
# or run the test binary directly
./tests/test_skiplist
```

## If you don't have GoogleTest installed (manual fetch)

The project supports a `third_party` fallback. You can fetch and build googletest locally:

```bash
./scripts/fetch_gtest.sh
```

This will build and install googletest into `third_party/googletest/install`. After that re-run `cmake ..` in the `build` directory.

If you prefer, set the `GTEST_ROOT` CMake cache variable to point to an existing gtest install:

```bash
cmake -S . -B build -DGTEST_ROOT=/path/to/googletest/install
```

## AddressSanitizer (ASAN) and Valgrind

- To build with ASAN enabled (helpful for memory errors), configure with:

```bash
cmake -S . -B build -DENABLE_ASAN=ON
cmake --build build -j
ctest --output-on-failure
```

- If `valgrind` is installed, CMake adds a `skiplist_memcheck` test which runs the unit test under valgrind. Run it with:

```bash
ctest -R skiplist_memcheck --output-on-failure
```

## Install

To install the library, headers and test executable to the system prefix (or a custom `CMAKE_INSTALL_PREFIX`):

```bash
cmake --install build --prefix /desired/install/prefix
```

## Debug vs Release targets in `src`

The `src` folder provides two library targets built from the same sources:

- `minilsm` — compiled with release-style flags
- `minilsm_debug` — compiled with debug-style flags (debug symbols, no optimizations)

You can build either target individually:

```bash
# build the release-like library
cmake --build build --target minilsm -j

# build the debug library
cmake --build build --target minilsm_debug -j
```

Or build both with the default `cmake --build build -j`.

When you change source files and only want to verify that the library compiles, build `minilsm` or `minilsm_debug` directly as above — this is faster than building the entire tree including tests.

## Convenience build script

You can also use the provided `build.sh` to build the library quickly. Run it directly or with `bash`:

```bash
# build debug-style library
./build.sh -f debug
# or
sh build.sh -f debug

# build release-style library
./build.sh -f release
```

This script will create the `build` directory, run `cmake` configuration if needed, and build only the requested library target.

You can also build and run tests with a single command:

```bash
# build tests and run the suite
./build.sh -t
```

This will place the library under `lib` and headers under `include` relative to the prefix; the test executable will be installed to `bin`.

## Notes

- The skiplist template is defined in `include/minilsm/skiplist.h` and implemented in `src/skiplist.cpp`.
- Tests live under `tests/` and use GoogleTest.
- The build defaults to using a system `gtest` if available, otherwise falls back to `third_party/googletest/install` (see above).

If you want, I can:
- Run `cmake --install` here to install into a local prefix (e.g. `/usr/local` or `build/install`).
- Add a small CPack config for packaging.
- Add CI workflow to run tests with ASAN and valgrind.

Tell me which you prefer.
