# Host tools

DaveCC is a compiler plus a set of host programs that assemble, link, dump,
archive, disassemble, and run the result. The same assemblers and linker that
`davecc` invokes are also standalone binaries.

```sh
bazelisk build //:davecc //:elfdump //:ltodump //:moduledump
davecc -target x86_64 -c program.c -o program.o
elfdump -a program.o
```

Install (`bazelisk run //:install` or `cmake --install`) puts drivers and
dumpers on `PREFIX/bin` and the compiler / interpreters in
`PREFIX/libexec/davecc`. `ltodump` and `moduledump` are Bazel targets (`//:ltodump`, `//:moduledump`);
they are **not** in the CMake `davecc_tools` set and are not copied by the
install script.

| Binary | Role |
| --- | --- |
| `davecc` | Compiler, integrated assembler, linker |
| `daveld` | Standalone linker ([linker.md](linker.md)) |
| `6502asm`, `rvasm`, `aarch64asm`, `armasm`, `x86asm`, `xtensaasm` | Standalone assemblers |
| `6502dasm`, `riscvdasm`, `aarch64dasm`, `armdasm`, `x86_64dasm`, `xtensadasm`, `bpfdasm`, `pcodedasm` | ELF disassemblers |
| `wasmdasm` | wasm32 module / object disassembler |
| `elfdump` | ELF header, sections, symbols, relocs, disassembly |
| `archivist` | System V `ar` for ELF and wasm32 objects |
| `ltodump` | Pretty-print `-flto` DCCLTO03 objects / archives |
| `moduledump` | Pretty-print C++20 `.dcm` modules |
| `run` | Pick an interpreter from an ELF `e_machine` |
| `6502`, `riscv`, `aarch64`, `arm`, `x86_64`, `esp32`, `pcode`, `bpf` | Interpreters |

There is no standalone assembler for wasm32, eBPF, p-code, or i386.
Use `davecc -target … -c file.s`. There is no dedicated `i386dasm`;
`elfdump -c` covers i386.

## `davecc`

The driver compiles, assembles, and links. Suffix decides the stage; `-S`
stops at assembly, `-c` at an object (ELF `ET_REL`, or a wasm object on
wasm32), and a link is the default.

```sh
davecc -target x86_64 -S program.c -o program.s
davecc -target x86_64 -c program.c -o program.o
davecc -target x86_64 program.c -o program
davecc -target x86_64 foo.s bar.o lib.a -o prog
```

`-flto` writes DCCLTO03 IR objects instead of machine code; inspect those
with `ltodump`. Linker flags and scripts are in [linker.md](linker.md).
On a Linux host (or via Colima), `-fuse-ld=ld` / `-fuse-ld=lld` asks the
driver to exec the native ELF linker instead of daveld; that is Linux
targets only. On macOS, omitting `-target` (or passing
`-target aarch64-apple-darwin-davecc`, or `-fnative` with `-target aarch64`)
writes AArch64 Mach-O objects, links `libcaarch64_darwin.a`
(`bazelisk build //:libc_aarch64_darwin`), and execs host `cc` so the
result can run without the interpreter. `-flto` is not available on that
path: DCCLTO03 is neither Mach-O nor LLVM bitcode. `-fno-native` cannot
turn a Darwin triple back into ELF.
Builtins are in [builtins.md](builtins.md).

After install, `davecc` itself lives in `libexec/davecc`. The `davecc-*`
wrappers on `PATH` add `-target` and the guest include / libc paths.

## `daveld`

Standalone linker. Same engine as `davecc`. `-r` / `--relocatable` writes
one `ET_REL`. See [linker.md](linker.md) for flags, layouts, and the script
language.

```sh
daveld -static -e main a.o b.o -o program
daveld -r a.o b.o -o combined.o
```

## Assemblers

Each standalone assembler is the same code `davecc` runs after `-S`. They
read GNU-style `.s` (C preprocessor included) and write ELF objects. On
macOS, `aarch64asm -fnative` writes a Mach-O `MH_OBJECT` instead so host
`cc` / `ld` can consume it. Other assemblers stay ELF-only.

| Binary | Target forced | Output |
| --- | --- | --- |
| `6502asm` | `6502` | ELF32 6502 / 65C02 |
| `rvasm` | `risc-v` | ELF RISC-V (64-bit triple) |
| `aarch64asm` | `aarch64` | ELF64 AArch64, or Mach-O with `-fnative` |
| `armasm` | `arm` | ELF32 ARM |
| `x86asm` | (internal) | ELF64 x86-64 |

```sh
6502asm -o foo.o foo.s
rvasm -DFOO=1 -I include bar.s          # writes bar.o
x86asm a.s b.s                          # one .o per input
```

`-o` is allowed with exactly one input. Without `-o`, a trailing `.s` is
rewritten to `.o`; any other name gets `.o` appended. `-D`, `-I`, and the
other preprocessor flags go through the same option parser as `davecc`.

`x86asm` is x86-64 only. 65C02 sources assemble with `6502asm` or
`davecc -target 65c02 -c file.s`. Xtensa / ESP32 sources assemble with
`xtensaasm` or `davecc -target esp32 -c file.s`.

## Disassemblers

These print executable sections. Addresses accept decimal or `0x`.

Shared CLI (`riscvdasm`, `aarch64dasm`, `armdasm`, `x86_64dasm`,
`xtensadasm`, `bpfdasm`, `pcodedasm`):

```sh
riscvdasm [--help] [--all] filename [addr [length]]
```

`--all` prints the section name before each block. `addr` is the first
address to show; `length` is a byte count.

`6502dasm` is older and loader-based:

```sh
6502dasm filename [addr [length]]
```

Default range is the full 64 KiB image. No `--help` / `--all`.

| Binary | ELF machine |
| --- | --- |
| `6502dasm` | 6502 / 65C02 |
| `riscvdasm` | RISC-V 32 and 64 |
| `aarch64dasm` | AArch64 |
| `armdasm` | ARM |
| `x86_64dasm` | x86-64 |
| `xtensadasm` | Xtensa / ESP32 |
| `bpfdasm` | eBPF |
| `pcodedasm` | p-code |

`wasmdasm` uses the same flags on a wasm object or linked module, not ELF.
i386 has no dedicated dumper; use `elfdump -c`.

## `elfdump`

One ELF file. The last dump flag wins, except `-a`. Default is `-H`.

```sh
elfdump [option] filename
```

| Flag | What it prints |
| --- | --- |
| `-h`, `--help` | This list |
| `-H` | ELF header |
| `-S` | Sections |
| `-s` | Symbols |
| `-l` | Program headers / segments |
| `-r` | Relocations |
| `-d` | Dynamic section |
| `-c` | Disassemble `SHF_EXECINSTR` sections |
| `-x N` | Hex dump of section number `N` |
| `-a` | Header, sections, symbols, segments, relocs, dynamic, disassembly (not `-x`) |

`-c` uses the shared decoder for 6502, RISC-V, AArch64, ARM, x86-64,
i386, Xtensa, eBPF, and p-code. The header line names those machines
(x86-64 as `x86-64`, i386 as `i386`, ARM as `ARM`).

wasm32 modules are not ELF. `elfdump` refuses them and tells you to use
`wasmdasm`.

## `archivist`

System V archive tool. Not GNU `ar`: no `q`, `p`, `m`, thin archives, or
MRI scripts. Members are ELF objects, or wasm32 objects (modules with a
linking section). Used to build the guest `libc*.a` files.

```sh
archivist <command> archive [member...]
```

Command letters may be combined. One of `t`, `r`, `d`, `x`, `s` is
required; `c`, `u`, and `v` are modifiers.

| Letter | Meaning |
| --- | --- |
| `t` | List members |
| `r` | Replace or append members |
| `d` | Delete members |
| `x` | Extract members |
| `s` | Print the archive symbol table |
| `c` | Do not warn when creating the archive |
| `u` | With `r`, replace only if the file is newer |
| `v` | Verbose |

```sh
archivist rc libfoo.a a.o b.o
archivist t libfoo.a
archivist s libfoo.a
archivist xv libfoo.a a.o
```

There is no `archivist --help`. A missing command or archive name is an
error.

## `ltodump`

Pretty-prints serialized LTO IR (`DCCLTO03`). Input is a `davecc -c -flto`
object or a System V archive of those. Output is the compiler's
`-Xbe-print` IR dump plus types, linkage, source locations, global
initializers, and the file table.

```sh
ltodump [--target ARCH] FILE [FILE...]
```

`--target` (default `x86_64`) initializes the compiler so types render
correctly. Use the architecture the IR was compiled for.

```sh
davecc -target x86_64 -flto -c foo.c -o foo.o
ltodump foo.o
ltodump --target aarch64 libguest.a
```

Not installed by `//:install`. Build with `bazelisk build //:ltodump`.

## `moduledump`

Dumps a C++20 module interface (`.dcm`). Format details are in
[module_file_format.md](module_file_format.md); the compiler flags are in
[cxx20_modules.md](cxx20_modules.md).

With no section flags it prints the header and a pool summary.

```sh
moduledump [options] <module.dcm>
```

| Flag | Contents |
| --- | --- |
| `--header` | Name, target, roots |
| `--summary` | Object-pool counts |
| `--archive` | Raw `ar` members and symbol table |
| `--strings` | Interned strings |
| `--types` | Type records |
| `--symbols` | Symbols, flags, types, templates |
| `--structs` | Classes / structs / unions |
| `--enums` | Enumerators |
| `--members` | Flat member pool |
| `--namespaces` | Namespace tree |
| `--ast` | Serialized function bodies and initializers |
| `-a`, `--all` | Every section |
| `--target T` | Loader triple (default `x86_64`) |
| `-h`, `--help` | This list |

```sh
moduledump --symbols math.dcm
moduledump --target aarch64 --all math.dcm
```

Not installed by `//:install`. Build with `bazelisk build //:moduledump`.

## Interpreters

Each ELF target has an in-tree interpreter. `run` execs the matching one
after reading `e_machine`. On a matching x86_64 or AArch64 host, those two
interpreters can jump to the mapped image (`-n`); DaveCC's loader and
guest libc still own the process.

| Binary | Target | Flags |
| --- | --- | --- |
| `x86_64` | x86-64 (also used for i386 by `run`) | `-i` / `--interpret`, `-n` / `--native`, `-d` instructions, `-r` registers |
| `aarch64` | AArch64 | same as `x86_64` |
| `arm` | ARM | `-d`, `-r`, `-g` (debugger), `-i` accepted and ignored |
| `riscv` | RISC-V 64 and 32 | `-d`, `-r`, `-g` |
| `6502` | 6502 / 65C02 | `-d` disassemble only, `-x FILE` extract image, `-rom FILE`, `-debug`, `-trace`, `-cycle` |
| `esp32` | Xtensa / ESP32 | `-d` |
| `pcode` | p-code | `-d` |
| `bpf` | eBPF | `-d` |

Guest arguments start at the program name. `LD_TRACE_LOADED_OBJECTS` (if
set) lists loaded objects and does not run.

The 6502 interpreter needs the support ROM: `-rom`, `DAVECC_6502_ROM`,
`DAVECC_ROOT`, or the usual Bazel / install path
(`6502_support/6502rom.exe` or `lib/davecc/6502rom.exe`). Do not override
the 6502 entry point to `main`; link at `_start`. See [6502.md](6502.md).

wasm32 is not ELF. Use [tools/wasm32run.sh](../tools/wasm32run.sh)
(requires host `wasmtime`):

```sh
davecc -target wasm32 program.c -o program.wasm
tools/wasm32run.sh program.wasm
```

The script runs `wasmtime run -W exceptions`, forwards `PATH` / `HOME` /
`USER` / `TMPDIR` / `LANG`, and preopens the host cwd as guest `/` plus
`/tmp`.

## `run` and wrappers

```sh
run executable [argument ...]
```

Reads the ELF type and machine, finds the interpreter next to itself (or
on `PATH` / `DAVECC_LIBEXEC_DIR`), and execs it. `ET_EXEC` and `ET_DYN`
only. 6502 gets `-rom` when the ROM is found. x86-64, AArch64, ARM, and
i386 get `-i` so a matching host does not silently go native.

| `e_machine` | Interpreter |
| --- | --- |
| p-code | `pcode` |
| eBPF | `bpf` |
| RISC-V | `riscv` |
| 6502 | `6502` |
| AArch64 | `aarch64` |
| ARM | `arm` |
| x86-64, i386 | `x86_64` |
| Xtensa / ESP32 | `esp32` |

No wasm32.

Installed wrappers (they source `davecc-env.sh` when present):

| Script | Invokes |
| --- | --- |
| `davecc-x86_64`, `davecc-x86`, `davecc-aarch64`, `davecc-arm`, `davecc-riscv`, `davecc-riscv32`, `davecc-65c02`, `davecc-esp32` | `davecc -target …` plus guest `-isystem` and `libc*.a` |
| `run-x86_64`, `run-x86`, `run-aarch64`, `run-arm`, `run-riscv`, `run-riscv32`, `run-esp32`, `run-65c02` | Matching interpreter (`-i` on the Linux-profile ones; `run-65c02` adds `-rom`) |
| `davecc-env.sh` | Sets `DAVECC_LIBEXEC_DIR`, `DAVECC_INCLUDE_DIR`, `DAVECC_LIB_DIR`, `DAVECC_6502_ROM` |

There is no `davecc-pcode`, `davecc-bpf`, or `davecc-wasm32`. Call
`davecc -target …` yourself.

`davecc-*` skips adding libc when you pass `-c`, `-S`, `-shared`,
`-nostdlib`, or an archive on the command line. Otherwise it adds
`-static` (unless you already passed it) and, for targets that start at
`main`, `-Wl,-e -Wl,main`.

## Install layout

```
PREFIX/bin/davecc-x86_64          # wrappers + daveld + run + asm/dasm + elfdump
PREFIX/bin/davecc-env.sh
PREFIX/libexec/davecc/davecc      # real compiler and interpreters
PREFIX/libexec/davecc/archivist
PREFIX/include/davecc/            # guest headers
PREFIX/lib/davecc/libc*.a         # guest archives, 6502rom.exe, esp32_start.o
```

Source `davecc-env.sh` (or set the same variables) so the wrappers find
`libexec` and `lib`.

```sh
source /usr/local/bin/davecc-env.sh
davecc-x86_64 program.c -o program
run-x86_64 program
```

## In-tree extras (not installed)

| Program | What it is |
| --- | --- |
| `6502map` | Writes a chip-select map ROM image for the hardware decoder (`6502map filename`) |
| `6502server` | Serial filesystem server on `/dev/ttyUSB1` for a real board |
| `tools/wasm32run.sh` | wasmtime launcher, documented above |
| `tools/build_guest_libc.sh` | Rebuild guest archives outside Bazel |

`6502map` and `6502server` are hardware helpers, not part of the
compiler toolchain.
