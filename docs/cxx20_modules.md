# C++20 Modules in DaveCC

DaveCC supports named modules, implementation units, interface and internal
partitions, global and private module fragments, re-exports, and header units.
Its compiled module format is DaveCC's own `.dcm` format; it is not compatible
with Clang `.pcm`, GCC CMI, or MSVC `.ifc` files.

## Build a named module

Compile the interface to a `.dcm` and object in one invocation:

```sh
davecc -target x86_64 -std=c++20 -c \
  -fmodule-output math.dcm math.cppm -o math.o
```

Compile an importer by searching a directory:

```sh
davecc -target x86_64 -std=c++20 -c \
  -fprebuilt-module-path . main.cpp -o main.o
```

An explicit mapping avoids relying on artifact filenames:

```sh
davecc -target x86_64 -std=c++20 -c \
  -fmodule-file math=/build/modules/math.dcm main.cpp -o main.o
```

For partitions, use the full logical name, for example
`-fmodule-file math:detail=/build/modules/math-detail.dcm`. Build every imported
partition before the unit that imports it.

## Header units

A header unit needs an explicit logical header name. Preserve the quote or
angle spelling used by its import declaration:

```sh
davecc -target x86_64 -std=c++20 -c -fmodule-header \
  -fmodule-name '"config.hpp"' \
  -fmodule-output config.hpp.dcm config.hpp -o config.hpp.o
```

It can then be consumed with `import "config.hpp";`. Declarations and macros
that remain defined at the end of header processing are made available to the
importer. Macros from ordinary named modules are not exported.

## Standard library module

`import std;` is available in C++23 mode. Bazel builds a target-specific
`std.dcm` and `std.o`; importers must compile for the same target and link the
matching object.

The verified profile exports:

- Core utilities: `any`, `array`, `compare`, `concepts`, `exception`,
  `initializer_list`, `iterator`, `memory`, `new`, `optional`, `ratio`,
  `source_location`, `span`, `stdexcept`, `string`, `string_view`,
  `system_error`, `tuple`, `type_traits`, `utility`, and `variant`.
- Containers and adaptors: `deque`, `list`, `map`, `queue`, `set`, `stack`,
  `unordered_map`, `unordered_set`, and `vector`.
- Concurrency and time: `atomic`, `barrier`, `chrono`, `condition_variable`,
  `coroutine`, `latch`, `mutex`, `semaphore`, `stop_token`, and `thread`.
- C++23 facilities: `algorithm`, `expected`, `format`, `functional`,
  `generator`, `mdspan`, `print`, and `ranges`.

This is the library's currently implemented narrow profile. The exported
`format` and `print` surfaces include variadic formatting/printing, the
implemented chrono duration and system-time-point formatters, and
`formatter<thread::id, char>`. They do not claim locale, Unicode,
wide-character, filesystem, or full calendar/time-zone formatting. Headers
absent from this list are intentionally not exported yet.

## Dependency scanning

DaveCC emits P1689R5 dependency information from the preprocessed token stream:

```sh
davecc -target x86_64 -std=c++20 -c \
  -fdeps-file math.json -fdeps-format=p1689r5 -fdeps-scan-only \
  math.cppm -o math.o
```

Remove `-fdeps-scan-only` to write the dependency file and continue compiling.
The output records the provided logical module and all direct `import` and
`export import` requirements.

## Bazel

Load the repository rules and declare module dependencies explicitly:

```starlark
load("//:davecc/modules.bzl", "davecc_binary", "davecc_module")

davecc_module(
    name = "math_module",
    src = "math.cppm",
    module_name = "math",
)

davecc_binary(
    name = "calculator",
    src = "main.cpp",
    modules = [":math_module"],
    link_inputs = ["//:libc_x86_64"],
    linkopts = ["-Wl,-e", "-Wl,main"],
)
```

`davecc_module` creates coordinated `.dcm` and `.o` outputs and propagates
transitive module mappings and link objects. `davecc_binary` compiles one
importing source and links it with those transitive objects.

## Diagnostics and inspection

`moduledump` prints module metadata and can include serialized symbols:

```sh
moduledump --symbols math.dcm
```

The low-level `-Xemit-module` and `-Xload-module` driver hooks remain useful for
format diagnostics, but normal builds should use `-fmodule-output`.
