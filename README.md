# DaveCC

DaveCC (`davecc`) is a from-scratch C and C++ compiler with its own assembler,
linker, loader, guest libc, and interpreters. It compiles hosted programs for
Linux and freestanding programs that run in the in-tree interpreters (or, on
matching hardware, as native code those interpreters can jump to).

Build and install instructions live in [docs/building.md](docs/building.md).

## Features

`davecc` is a complete toolchain, not a frontend bolted onto someone else's
assembler and libc. One driver compiles, assembles, and links for every
supported architecture; one guest library and one interpreter go with each
target. Cross compilation is the normal path: any host can produce a binary
for any listed target.

Every architecture also has an in-tree interpreter, so you can run the
result on the machine that built it. Interpretation is slower than native
code, but you do not need the matching CPU, a board, or a VM to test an
AArch64, RISC-V, ARM, 65C02, or wasm32 program.

**65C02 is a fully supported target.** It has a code generator, assembler,
linker ABI, guest libc, C++ standard library, and a support ROM. You can
compile real C++ (default C++20) and run it on a 6502-class computer — or
under the `6502` interpreter while you develop. That combination is rare:
most C++ toolchains stopped caring about the 6502 decades ago. The ABI,
zero-page registers, heap and stack, and syscalls are in
[docs/6502.md](docs/6502.md).

Other things the tree includes:

- C89 through C23, and C++98 through the C++29 draft, with a C++20 guest
  standard library
- Hosted Linux images (static, and dynamic on x86_64 / ARM / RV64) plus
  freestanding interpreter-profile ELF (and wasm32 / WASI)
- In-tree assemblers, linker, loader, archiver, disassemblers, and
  `elfdump`
- Native execution on a matching x86_64 or AArch64 host (`-n`), still
  using DaveCC's loader and guest libc rather than the Linux ABI

## What the toolchain is

`davecc` is a GCC/Clang-style driver. Suffix decides the stage:

| Input | What happens |
| --- | --- |
| `.c`, `.cc`, `.cpp`, `.cxx`, `.cppm`, `.ixx`, `.h`, `.hpp`, `.hxx` | Compile |
| `.s` | Assemble to a relocatable object |
| `.o`, `.a`, and anything else | Pass to the linker |

Without `-c`, `-S`, or `-fsyntax-only`, the driver compiles, assembles, and
links an executable. `-target` is required. Bare names such as `x86_64` mean
the interpreter/freestanding profile (`arch-unknown-none-davecc`). A full
triple `arch-unknown-linux-davecc` selects the hosted Linux ABI, CRT, and
syscalls.

The driver finds `libc/include` and the matching guest archive automatically
when it can see a Bazel or install tree (`DAVECC_INCLUDE_DIR` /
`DAVECC_LIB_DIR` override that). Use `-nostdinc` / `-nostdlib` to skip them.

## Language standards

### C

Default for `.c` is **C99**. `-std` accepts:

| Flag | Also accepted | `__STDC_VERSION__` |
| --- | --- | --- |
| `c89` | `c90`, `iso9899:1990`, `gnu89`, `gnu90` | unset (C90) |
| `c99` | `iso9899:1999`, `gnu99` | `199901L` |
| `c11` | `c1x`, `iso9899:2011`, `gnu11` | `201112L` |
| `c17` | `c18`, `iso9899:2017`, `gnu17`, `gnu18` | `201710L` |
| `c23` | `c2x`, `iso9899:2024`, `gnu23`, `gnu2x` | `202311L` |

C11 `<stdatomic.h>` and `<threads.h>` are available only on targets that
advertise them. Others define `__STDC_NO_ATOMICS__` and/or
`__STDC_NO_THREADS__`. Complete C11 atomics (including `atomic_llong`) are
implemented on **x86_64** and **RISC-V 64**. Threads exist on the hosted
x86_64, AArch64, ARM, and RISC-V profiles; **pcode**, **6502**, **65C02**,
**wasm32**, **eBPF**, and **ESP32** do not.

### C++

Default for `.cc` / `.cpp` is **C++20**. `-std` accepts C++98 through the
C++29 draft, including the usual `gnu++` and `c++2a` / `c++2b` / `c++2c` /
`c++2d` aliases.

| Flag | `__cplusplus` | Notes |
| --- | --- | --- |
| `c++98` / `c++03` | `199711L` | Language mode only |
| `c++11` | `201103L` | Language mode only |
| `c++14` | `201402L` | Language mode only |
| `c++17` | `201703L` | Language mode only |
| `c++20` | `202002L` | Primary supported library dialect |
| `c++23` | `202302L` | Language + library features in use |
| `c++26` | `202603L` | Language features; still evolving |
| `c++29` | `202700L` | Experimental draft work |

The guest C++ standard library is written for **C++20 and later**. Most
headers will not compile at `-std=c++11`, `c++14`, or `c++17`. Exceptions
are on by default (`-fexceptions` / `-fno-exceptions`). C++20 named modules
use DaveCC's own `.dcm` format; see [docs/cxx20_modules.md](docs/cxx20_modules.md).
C++29 experimental work is sketched in [docs/cxx29_roadmap.md](docs/cxx29_roadmap.md).

Threading headers (`<thread>`, `<stop_token>`, `<condition_variable>`,
`<semaphore>`, `<latch>`, `<barrier>`) compile only on the hosted x86-64,
AArch64, ARM, and RISC-V profiles. On p-code, 6502, and 65C02 they produce a
compile-time diagnostic.

## Targets

`-target` takes an architecture name or `arch-vendor-os-env`. The only
supported OS names are `none` (interpreter / freestanding) and `linux`. The
only supported environment is `davecc`.

| Canonical | Aliases | Pointer / `long` | ELF | Interpreter libc | Linux triple |
| --- | --- | --- | --- | --- | --- |
| `x86_64` | `x86-64` | 64 / 64 | ELF64, `EM_X86_64` (62) | `libcx86_64.a` | `x86_64-unknown-linux-davecc` |
| `aarch64` | `armv8` | 64 / 64 | ELF64, `EM_AARCH64` (183) | `libcaarch64.a` | `aarch64-unknown-linux-davecc` |
| `arm` | `armv7`, `armv7-a`, `arm32` | 32 / 32 | ELF32, `EM_ARM` (40), armhf | `libcarm.a` | `arm-unknown-linux-davecc` |
| `riscv` | `risc-v` | 64 / 64 | ELF64, `EM_RISCV` (243), LP64D | `libcriscv.a` | `riscv-unknown-linux-davecc` |
| `riscv32` | `risc-v32` | 32 / 32 | ELF32, `EM_RISCV` (243) | `libcriscv32.a` | `riscv32-unknown-linux-davecc` |
| `x86` | `i386`, `i486`, `i586`, `i686`, `x86-32` | 32 / 32 | ELF32, `EM_386` (3) | `libcx86.a` | no hosted Linux profile |
| `pcode` | `p-code` | 64 / 64 | ELF64, machine **6500** | `libcpcode.a` | not Linux |
| `6502` | | 16 / 32 | ELF, machine **6502** | `libc65c02.a` | not Linux |
| `65c02` | `65C02` | 16 / 32 | same as 6502, extra opcodes | `libc65c02.a` | not Linux |
| `esp32` | `xtensa-esp32` | 32 / 32 | ELF32, `EM_XTENSA` (94) | `libcxtensa.a` + `esp32_start.o` | no hosted profile |
| `wasm32` | `wasm` | 32 / 32 | **not ELF** (WebAssembly) | `libcwasm32.a` | WASI via wasmtime |
| `bpf` | `bpfel`, `ebpf` | 64 / 64 | ELF64, `EM_BPF` (247) | none (`-nostdlib`) | `bpf-unknown-linux-davecc` |

All objects are little-endian. 6502/65C02 default to `-O2` and optimize for
size. eBPF, ESP32, wasm32, and 6502/65C02 are static-only.

6502 and 65C02 share one ABI, guest archive, and interpreter. Zero-page
registers, the software stack and heap, the support ROM, IEEE-754 single
precision (there is no binary64), and the syscall surface are documented in
[docs/6502.md](docs/6502.md).

The `x86` interpreter profile is loaded by the `x86_64` interpreter, which
accepts ELF32 i386 and decodes IA-32 (cdecl, 32-bit stack slots, `int $0x80`).


## Object and executable files

### ELF (every target except wasm32)

`davecc -c` writes a relocatable **`ET_REL`** object. The linker writes an
**`ET_EXEC`** executable, or **`ET_DYN`** for `-shared` / `-dynamic`.

Typical contents:

- `.text`, `.data`, `.rodata`, `.bss`, and TLS (`.tdata` / `.tbss`) when used
- `.symtab` / `.strtab` and `.rela*` (or `.rel*` on ARM and i386)
- Program headers (`PT_LOAD`, and on Linux dynamic links `PT_INTERP`,
  `PT_DYNAMIC`, `PT_TLS`)
- DWARF when debug info is enabled

Machine numbers are the System V values except **p-code (6500)** and
**6502/65C02 (6502)**, which are DaveCC-specific. Inspect any of these files
with `elfdump` (`-H` header, `-S` sections, `-s` symbols, `-l` segments,
`-r` relocs, `-c` disassemble).

Linux static executables use DaveCC's CRT (`*_linux_start.o`) and enter at
`_start`. Interpreter-profile x86 / x86_64 / AArch64 / ARM / p-code images
enter at `main` unless you pass `-e`. Dynamic Linux images request the native
interpreter:

| Triple | `PT_INTERP` | Shared libc |
| --- | --- | --- |
| `x86_64-unknown-linux-davecc` | `/lib64/ld-linux-x86-64.so.2` | `libdavecc.so.1` |
| `arm-unknown-linux-davecc` | `/lib/ld-linux-armhf.so.3` | `libdavecc.so.1` |
| `riscv-unknown-linux-davecc` | `/lib/ld-linux-riscv64-lp64d.so.1` | `libdavecc.so.1` |

Those dynamic binaries use `$ORIGIN` as the runpath. The native loader maps
and relocates; DaveCC then owns TLS, errno, init/fini, and clone/futex
threads. Lazy PLT binding, `dlopen`/`dlclose`, and DSOs with their own TLS
are not supported after that hand-off.

### WebAssembly (wasm32)

`-c` writes a relocatable wasm **object** (a module with a linking section).
Linking merges those objects plus `libcwasm32.a` into a WASI command module
(`wasi_snapshot_preview1`). That module is not ELF; `archivist` understands
wasm members inside `.a` files. Run the result with
[tools/wasm32run.sh](tools/wasm32run.sh) (wasmtime).

## Assemblers, linker, disassemblers, and tools

### Built into `davecc`

The compiler emits assembly, then the in-tree assembler for that `-target`
produces the object, then the in-tree linker produces the executable. You can
stop at any stage:

```sh
davecc -target x86_64 -S program.c -o program.s   # assembly listing
davecc -target x86_64 -c program.c -o program.o   # ET_REL
davecc -target x86_64 program.c -o program        # link
davecc -target x86_64 foo.s bar.o lib.a -o prog   # assemble + link
```

Useful linker flags: `-static`, `-dynamic`, `-shared`, `-e symbol`,
`--gc-sections`, `-L`, `-l`, `-rpath`, `-Wl,…`. `-flto` writes DCCLTO03 IR
objects instead of machine code.

### Standalone assemblers

Same code as the integrated assemblers; they read `.s` and write ELF objects.

| Binary | Architecture |
| --- | --- |
| `6502asm` | 6502 / 65C02 |
| `rvasm` | RISC-V |
| `aarch64asm` | AArch64 |
| `armasm` | ARM |
| `x86asm` | x86-64 |

### Disassemblers and dumps

| Binary | Role |
| --- | --- |
| `elfdump` | ELF header, sections, symbols, relocs, optional disassembly |
| `6502dasm` | 6502 / 65C02 |
| `riscvdasm` | RISC-V |
| `aarch64dasm` | AArch64 |
| `armdasm` | ARM |
| `x86_64dasm` | x86-64 |
| `xtensadasm` | Xtensa / ESP32 |

### Other tools

| Binary | Role |
| --- | --- |
| `archivist` | `ar`-like archiver for ELF objects and wasm32 objects |
| `run` | Peek at an ELF `e_machine` and exec the matching interpreter |
| `moduledump` | Pretty-print C++20 `.dcm` module files |
| `ltodump` | Pretty-print `-flto` DCCLTO03 IR objects and archives |

After install, `bin/davecc-*` and `bin/run-*` wrap the compiler and
interpreters with the right `-target` and search paths. Source
`davecc-env.sh` (written next to those wrappers) so they find
`libexec/davecc` and `lib/davecc`.

## Building

From the repository root, with Bazelisk or Bazel:

```sh
bazelisk build //:davecc
bazelisk build //:davecc //:libc_x86_64 //:x86_64   # typical interpreter loop
bazelisk build //:install                            # every host tool + guest archive
```

Install:

```sh
bazelisk run //:install -- --prefix /usr/local
```

CMake 3.20+ is the other supported path (`davecc_tools`, `davecc_libc`,
`cmake --install`). Details, DESTDIR, and cleaning:
[docs/building.md](docs/building.md).

Outputs land in `bazel-bin/` (`davecc`, `x86_64`, `libc/libcx86_64.a`, …).

## Compile, link, and run

Two different products:

1. **Interpreter profile** (`-target ARCH`) — DaveCC ELF (or a wasm module)
   that the matching in-tree interpreter loads.
2. **Linux profile** (`-target ARCH-unknown-linux-davecc`) — a Linux ELF the
   kernel (or qemu-user / Colima) can `exec`.

Once `davecc` and the guest archive are built, you do not pass the `.a` by
hand unless you use `-nostdlib`.

### Interpreter profile (any host)

```sh
bazelisk build //:davecc //:libc_x86_64 //:x86_64

bazel-bin/davecc -target x86_64 program.c -o program
bazel-bin/x86_64 -i program
# or: bazel-bin/run program
```

C++:

```sh
bazel-bin/davecc -target x86_64 -std=c++20 program.cpp -o program
bazel-bin/x86_64 -i program
```

Same pattern for the other interpreter targets. Build the libc and
interpreter listed in the table, then:

```sh
# AArch64
bazelisk build //:davecc //:libc_aarch64 //:aarch64
bazel-bin/davecc -target aarch64 program.c -o program
bazel-bin/aarch64 -i program

# 32-bit ARM
bazelisk build //:davecc //:libc_arm //:arm
bazel-bin/davecc -target arm program.c -o program
bazel-bin/arm -i program          # -i is accepted; ARM is interpret-only

# i386 (loaded by the x86_64 interpreter)
bazelisk build //:davecc //:libc_x86 //:x86_64
bazel-bin/davecc -target x86 program.c -o program
bazel-bin/x86_64 -i program


# RISC-V 64 (the `riscv` interpreter also loads RV32 ELF)
bazelisk build //:davecc //:libc_riscv //:riscv
bazel-bin/davecc -target riscv program.c -o program
bazel-bin/riscv program           # no -i flag; always interprets

bazelisk build //:libc_riscv32
bazel-bin/davecc -target riscv32 program.c -o program
bazel-bin/riscv program

# P-code
bazelisk build //:davecc //:libc_pcode //:pcode
bazel-bin/davecc -target pcode program.c -o program
bazel-bin/pcode program

# 65C02 (needs the interpreter ROM)
bazelisk build //:davecc //:libc_65c02 //:6502 //:support_rom_65c02
bazel-bin/davecc -target 65c02 program.c -o program
bazel-bin/6502 -rom bazel-bin/6502_support/6502rom.exe program

# ESP32 / Xtensa
bazelisk build //:davecc //:libc_xtensa //:esp32_start //:esp32
bazel-bin/davecc -target esp32 program.c -o program
bazel-bin/esp32 program

# Linux eBPF (no libc)
bazelisk build //:davecc //:bpf
bazel-bin/davecc -target bpf -nostdlib -static -Wl,-e -Wl,main \
  program.c -o program
bazel-bin/bpf program

# wasm32 (needs wasmtime on PATH)
bazelisk build //:davecc //:libc_wasm32
bazel-bin/davecc -target wasm32 program.c -o program.wasm
tools/wasm32run.sh program.wasm
```

Separate compile and link:

```sh
bazel-bin/davecc -target x86_64 -c a.c -o a.o
bazel-bin/davecc -target x86_64 -c b.c -o b.o
bazel-bin/davecc -target x86_64 a.o b.o -o program
```

### Interpreters

Each interpreter is a small host program: it reads the guest ELF with
DaveCC's loader, applies relocations, maps segments, and either simulates
the ISA or (on x86_64 and AArch64 hosts) jumps into the mapped text.

| Binary | Loads | Flags |
| --- | --- | --- |
| `x86_64` | ELF64 x86-64 and ELF32 i386 | `-i` interpret, `-n` native (64-bit hosts only), `-d` insn trace, `-r` reg trace |
| `aarch64` | ELF64 AArch64 | same as `x86_64` |
| `arm` | ELF32 ARM | `-i` (no-op), `-d`, `-r`, `-g` debugger |
| `riscv` | ELF64 or ELF32 RISC-V | `-d`, `-r`, `-g` debugger |
| `pcode` | DaveCC p-code ELF | `-d` |
| `6502` | 6502/65C02 ELF | `-d`, `-rom file`, `-x outfile` |
| `esp32` | Xtensa ELF | `-d` |
| `bpf` | eBPF ELF | `-d` |
| `run` | any of the above | picks the interpreter from `e_machine` |

**Native mode (`-n` / `--native`)** exists on the x86_64 and AArch64
interpreters. On a matching host CPU it `mprotect`s the loaded text
executable and calls the guest entry; on any other host it is unavailable
and you must pass `-i`. Default is native on a matching CPU, interpret
otherwise. This still uses DaveCC's loader and guest libc — it is **not**
the Linux ABI. ARM, RISC-V, p-code, 6502, ESP32, and eBPF always interpret.

`run-x86_64` and friends force `-i` so installed wrappers behave the same on
every machine. `LD_BIND_NOW` and `LD_TRACE_LOADED_OBJECTS` are honored by
the RISC-V, ARM, and p-code loaders for DaveCC-dynamic images.

### Native Linux

Hosted triples emit a Linux ELF that the kernel can run. Static is the
default. Build the Linux libc and CRT for that arch (or `//:install`), then:

```sh
bazelisk build //:davecc //:libc_aarch64_linux //:aarch64_linux_start

bazel-bin/davecc -target aarch64-unknown-linux-davecc program.c -o program
./program          # on AArch64 Linux; elsewhere use qemu or Colima
```

The same form works for `x86_64-unknown-linux-davecc`,
`arm-unknown-linux-davecc`, `riscv-unknown-linux-davecc`, and
`riscv32-unknown-linux-davecc`. Native libc uses Linux UAPI syscall numbers
and provides file I/O, filesystem operations, clocks, random data, heap
allocation, TLS, and C11 threads.

#### Dynamic Linux (x86_64, ARM, RV64)

```sh
bazelisk build //:davecc //:x86_64_linux_dynamic_runtime

bazel-bin/davecc -target x86_64-unknown-linux-davecc -dynamic \
  program.cpp -o program
cp bazel-bin/libc/libdavecc.so.1 .
./program
```

ARM and RV64 isolate their DSOs under `bazel-bin/libc/arm` and
`bazel-bin/libc/riscv`. Point `DAVECC_LIB_DIR` at that directory (or let the
driver find it) and copy `libdavecc.so.1` next to the executable.

```sh
bazelisk build //:davecc //:arm_linux_dynamic_runtime
DAVECC_LIB_DIR="$PWD/bazel-bin/libc/arm" \
  bazel-bin/davecc -target arm-unknown-linux-davecc -dynamic \
  program.cpp -o program
cp bazel-bin/libc/arm/libdavecc.so.1 .

bazelisk build //:davecc //:riscv_linux_dynamic_runtime
DAVECC_LIB_DIR="$PWD/bazel-bin/libc/riscv" \
  bazel-bin/davecc -target riscv-unknown-linux-davecc -dynamic \
  program.cpp -o program
cp bazel-bin/libc/riscv/libdavecc.so.1 .
```

`-dynamic` is rejected for AArch64 Linux and for every non-Linux target.

#### Cross-testing Linux binaries with Colima

On Apple Silicon, Colima plus qemu-user is how the Linux smoke tests run
foreign ELF files.

**x86_64** — QEMU-backed x86_64 VM (`uname -m` must print `x86_64`):

```sh
brew install colima lima-additional-guestagents
colima start x86 --arch x86_64 --vm-type qemu
colima -p x86 ssh -- uname -m

bazel test //:native_linux_x86_64_smoke_test
bazel test //:native_linux_x86_64_dynamic_minimal_test
bazel test //:native_linux_x86_64_dynamic_smoke_test
```

**ARM32** — AArch64 guest, `qemu-arm` binfmt, armhf loader
(`/lib/ld-linux-armhf.so.3`):

```sh
colima start --arch aarch64
colima ssh -- sudo dpkg --add-architecture armhf
colima ssh -- sudo apt-get update
colima ssh -- sudo apt-get install -y libc6:armhf qemu-user-static binfmt-support
colima ssh -- sudo update-binfmts --enable qemu-arm

bazel test //:native_linux_arm_minimal_smoke_test
bazel test //:native_linux_arm_smoke_test
bazel test //:native_linux_arm_dynamic_minimal_test
bazel test //:native_linux_arm_dynamic_smoke_test
```

**RV64** — same AArch64 guest, `qemu-riscv64` and the LP64D loader at
`/lib/ld-linux-riscv64-lp64d.so.1`:

```sh
colima ssh -- sudo apt-get install -y \
  libc6-riscv64-cross qemu-user-static binfmt-support
colima ssh -- sudo update-binfmts --enable qemu-riscv64
colima ssh -- sudo ln -sf \
  /usr/riscv64-linux-gnu/lib/ld-linux-riscv64-lp64d.so.1 \
  /lib/ld-linux-riscv64-lp64d.so.1

bazel test //:native_linux_riscv_minimal_smoke_test
bazel test //:native_linux_riscv_smoke_test
bazel test //:native_linux_riscv_dynamic_minimal_test
bazel test //:native_linux_riscv_dynamic_smoke_test
```

`COLIMA_PROFILE` overrides the profile name (default `x86` for x86_64 tests,
`default` for ARM and RISC-V).

## Tests

```sh
bazelisk test //...                              # everything; long
bazelisk test //c_testsuite:exec_x86_64
bazelisk test //cxx_testsuite:exec_x86_64 --test_output=errors
```

C and C++ execution suites compile with `davecc` and run under the matching
interpreter. Wasm32 C tests need host `wasmtime`.
