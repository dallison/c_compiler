# DaveCC C++ Testsuite

This suite parallels `c_testsuite`, but starts as a compile-only harness while
C++ frontend support is built out.

## Layout

- `tests/lexical/pass/*.cpp`: source files expected to compile without frontend
  errors.
- `tests/lexical/fail/*.cpp`: source files expected to produce diagnostics.
- `include/`: headers used by tests.

Each test can add compile options with comments at the top of the file:

```cpp
// RUN: -std=c++20
```

Failing tests can list required diagnostic substrings:

```cpp
// EXPECT: Expected semicolon
```

The runner defaults to `-target pcode -S -std=c++20` and writes only assembly,
so tests do not require a C++ runtime or executable support.
