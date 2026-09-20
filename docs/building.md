# Building DaveCC

DaveCC is the compiler (`davecc`) plus assemblers, disassemblers, a linker,
interpreters for each target, and a guest libc for each hosted profile.
Compiler builtins and per-architecture SIMD / runtime intrinsics are listed
in [builtins.md](builtins.md). Assemblers, dumpers, interpreters, and the
other host programs are in [tools.md](tools.md).

Two build systems produce the same host tools and guest libraries:

| | Bazel / Bazelisk | CMake 3.20+ |
| --- | --- | --- |
| Host compiler and tools | `//:davecc`, `//:davecc_tools` data | `davecc_tools` |
| Guest libc archives | `//:libc_*` | `davecc_libc` |
| Install | `bazel run //:install` | `cmake --install` |
| Test suites | `bazel test //...` | not wired |

Use Bazel for day-to-day compiler work and the test suites. Use CMake when you
want a conventional out-of-tree build or `make install`.

`bazel` and `bazelisk` are interchangeable below. If both are installed, prefer
`bazelisk` so the repo's Bazel version is selected automatically.

## Prerequisites

- A C99 host compiler (Clang or GCC). Apple Clang is fine on macOS.
- **Bazel** 8+ with Bzlmod, or **Bazelisk**.
- **CMake** 3.20 or newer, and Ninja or Make, if you use the CMake path.
- Python 3 (used by some test harnesses, not required to compile).

No extra SDK is required for the in-tree interpreters. Native Linux profiles
and Colima-based smoke tests are documented in the top-level [README](../README.md).

## What gets built

### Host tools

These land in `bazel-bin/` (Bazel) or the CMake build directory:

| Binary | Role |
| --- | --- |
| `davecc` | Compiler, assembler driver, and linker |
| `daveld` | Standalone linker (`-r` writes a relocatable object); see [linker.md](linker.md) |
| `archivist` | Archive (`ar`) tool used to pack guest libc |
| `run` | Multi-target program launcher |
| `6502`, `riscv`, `aarch64`, `arm`, `x86_64`, `esp32`, `pcode`, `bpf` | Target interpreters |
| `6502asm`, `rvasm`, `aarch64asm`, `armasm`, `x86asm`, `xtensaasm` | Standalone assemblers |
| `6502dasm`, `riscvdasm`, `aarch64dasm`, `armdasm`, `x86_64dasm`, `xtensadasm`, `bpfdasm`, `pcodedasm`, `wasmdasm`, `elfdump` | Disassemblers / ELF and wasm dump |
| `ltodump` | Pretty-print `-flto` DCCLTO03 objects (not installed) |
| `moduledump` | Pretty-print C++20 `.dcm` files (not installed) |

Wrapper scripts in `bin/` (`davecc-x86_64`, `run-riscv`, …) point at those
binaries after install.

### Guest libraries

Each hosted target has a libc compiled **by davecc**, not by the host
compiler. Every target gets a static archive. Targets that support
`-fPIC` / `-shared` also get a shared object (`davecc -target … -dynamic`
links against it):

| Archive | Shared object | CRT (`-dynamic`) | Target |
| --- | --- | --- | --- |
| `libc/libcx86_64.a` | `libc/libcx86_64.so` | `libc/libcx86_64_crt.a` | `x86_64` |
| `libc/libcx86.a` | `libc/libcx86.so` | `libc/libcx86_crt.a` | `x86` / `i386` |
| `libc/libcaarch64.a` | `libc/libcaarch64.so` | `libc/libcaarch64_crt.a` | `aarch64` |
| `libc/libcarm.a` | `libc/libcarm.so` | `libc/libcarm_crt.a` | `arm` |
| `libc/libcriscv.a` | `libc/libcriscv.so` | `libc/libcriscv_crt.a` | `riscv` |
| `libc/libcriscv32.a` | `libc/libcriscv32.so` | `libc/libcriscv32_crt.a` | `riscv32` |
| `libc/libcpcode.a` | `libc/libcpcode.so` | `libc/libcpcode_crt.a` | `pcode` |
| `libc/libc65c02.a` | — | `65c02` |
| `libc/libcxtensa.a` | — | `esp32` |

Also built with the guest libraries:

- `6502_support/6502rom.exe` — 65C02 interpreter ROM
- `libc/esp32_start.o` — ESP32/`xtensa` CRT start object

Bazel additionally builds `//:libc_wasm32` and the native Linux libc/startup
profiles (`//:libc_x86_64_linux`, `//:libc_aarch64_linux`, …). Those are not
part of the CMake `davecc_libc` target.

Headers live in `libc/include/` and are used with `-isystem libc/include`.

---

## Bazel / Bazelisk

From the repository root:

```sh
# Compiler only (fastest incremental target)
bazelisk build //:davecc

# Interpreters you are about to run
bazelisk build //:x86_64 //:aarch64 //:arm //:riscv //:6502 //:esp32 //:pcode //:bpf

# Guest libc for one target
bazelisk build //:libc_x86_64

# Everything the installer needs: tools, all guest archives, ROM, start object
bazelisk build //:install
```

`//:install` is an alias for `//:install_davecc_tools`. Building it compiles
every host tool and every guest archive the install step copies.

Useful one-shots:

```sh
# All default-package tests (long)
bazelisk test //...

# C++ execution suite on one interpreter
bazelisk test //cxx_testsuite:exec_x86_64 --test_output=errors

# C execution suite
bazelisk test //c_testsuite:exec_x86_64
```

Outputs are under `bazel-bin/` (`bazel-bin/davecc`, `bazel-bin/libc/libcx86_64.a`,
`bazel-bin/x86_64`, …).

### Compile a program with the Bazel tree

```sh
bazelisk build //:davecc //:libc_x86_64 //:x86_64

bazel-bin/davecc -target x86_64 -static -isystem libc/include \
  program.c bazel-bin/libc/libcx86_64.a -o program
bazel-bin/x86_64 -i program
```

C++ execution tests in this repo also pass `-std=c++20` (or `c++23`) and
`-Wl,-e -Wl,main`. RISC-V interpreter runs omit `-i`.

### Install (Bazel)

```sh
bazelisk run //:install -- --prefix /usr/local
```

Stage into a DESTDIR without writing `/usr/local` directly:

```sh
bazelisk run //:install -- --prefix /usr/local --destdir "$PWD/stage"
```

Other flags: `--bindir`, `--includedir`, `--libdir`, `--libexecdir`.
Defaults match CMake: `PREFIX/bin`, `PREFIX/include/davecc`, `PREFIX/lib/davecc`,
`PREFIX/libexec/davecc`.

The installer writes `davecc-env.sh` next to the wrapper scripts. Source it, or
keep those `DAVECC_*` variables in the environment, so `davecc-*` / `run-*`
find the libexec tools and guest archives.

---

## CMake

CMake 3.20+ is required. Ninja is recommended.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target davecc_tools      # host compiler and tools
cmake --build build --target davecc_libc       # guest archives + ROM + esp32_start
```

`davecc_tools` does **not** build guest libc. That is intentional: compiling
eight target archives with davecc is much slower than the host link. Build
`davecc_libc` when you need archives or are about to install.

Individual archives:

```sh
cmake --build build --target davecc_libcx86_64.a
cmake --build build --target davecc_libcaarch64.a
```

Host binaries are named as on the Bazel side (`davecc`, `x86_64`, `archivist`,
…) inside the build directory.

### Compile a program with the CMake tree

```sh
cmake --build build --target davecc_bin davecc_libcx86_64.a

build/davecc -target x86_64 -static -isystem libc/include \
  program.c build/libc/libcx86_64.a -o program
build/x86_64 -i program
```

### Install (CMake)

```sh
cmake --install build --prefix /usr/local
```

`cmake --install` builds `davecc_libc` first so the guest archives exist, then
installs the same layout as `bazel run //:install`:

- `bin/` — wrapper scripts, `run`, assemblers/disassemblers, `davecc-env.sh`
- `libexec/davecc/` — `davecc`, interpreters, assemblers
- `include/davecc/` — libc headers
- `lib/davecc/` — `libc*.a`, `6502rom.exe`, `esp32_start.o`

DESTDIR works the usual way:

```sh
DESTDIR="$PWD/stage" cmake --install build --prefix /usr/local
```

---

## After install

```sh
# If the wrappers are on PATH:
source /usr/local/bin/davecc-env.sh   # or PREFIX/bin/davecc-env.sh

davecc-x86_64 program.c -o program
run-x86_64 program
```

The `davecc-*` scripts invoke the installed compiler with the matching
`-target` and the installed include/lib directories.

## Cleaning

```sh
bazelisk clean                 # Bazel outputs (keeps the analysis cache)
bazelisk clean --expunge       # full wipe, including the disk cache
cmake --build build --target clean
rm -rf build                   # CMake build tree
```

The two build systems do not share object files. Cleaning one does not affect
the other.
