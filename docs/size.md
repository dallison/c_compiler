# DaveCC size snapshots

Physical line counts (`wc -l`) and binary sizes for DaveCC, compared with
Clang/LLVM and GCC. Newest snapshot first.

These are scope indicators, not quality metrics. Clang and GCC implement more
languages, object formats, and targets. DaveCC's compiler binary also includes
its linker and six backends.

## How to take a new snapshot

Count the compiler that links into `davecc` (not libc, tests, or interpreters):

```sh
find c_compiler Linker davecc -name '*.c' | xargs wc -l | tail -1
find c_compiler Linker davecc -name '*.h' | xargs wc -l | tail -1
ls -l bazel-bin/davecc
file bazel-bin/davecc
```

On macOS, report the arm64 slice of fat binaries:

```sh
lipo -thin arm64 PATH -output /tmp/slice && ls -l /tmp/slice
```

Add a new `## YYYY-MM-DD` section at the top of the snapshot list, using the
same trees and the same `wc -l` definition (every newline, including blanks
and comments).

---

## 2026-09-09

Host: macOS arm64 (darwin 25.6.0). DaveCC binary from `bazel-bin/davecc`,
Mach-O arm64, mtime 2026-09-09 08:09 BST.

### DaveCC compiler

Trees: `c_compiler/`, `Linker/`, `davecc/`.

| | Files | Lines |
|---|---:|---:|
| `.c` | 195 | 309,059 |
| `.h` | 200 | 21,594 |
| **Total** | **395** | **330,653** |

| Tree | `.c` files | `.c` lines | `.h` files | `.h` lines |
|---|---:|---:|---:|---:|
| `c_compiler/` | 179 | 298,406 | 185 | 20,723 |
| `Linker/` | 15 | 8,933 | 15 | 871 |
| `davecc/main.c` | 1 | 1,720 | 0 | 0 |

**Executable:** 4,838,136 bytes (4.6 MB).

### Vs Clang / LLVM

LLVM line counts are non-test source (`.c`/`.cpp`/`.h`/`.inc`) from the
llvm-project `main` snapshot dated 2026-04-30 (commit `2fdb09cf65e6`).
Open Hub reports ~12.9M code lines for the whole monorepo.

| | Source | Binary |
|---|---|---|
| DaveCC | 309k `.c` (331k with headers), 6 backends + linker | **4.6 MB** arm64 |
| Clang frontend | ~1.89M | Apple clang 17.0.0 (`clang-1700.4.4.1`): **125 MB** arm64 slice (257 MB universal, 269,128,592 bytes) |
| LLVM core | ~873k | linked into clang |
| lld | ~110k | see [Linker](#linker) |
| Clang + LLVM + lld | ~2.9M | — |

Clang frontend is about **6×** DaveCC source. Clang+LLVM+lld is about **9×**.
The Apple clang arm64 binary is about **27×** DaveCC.

`/usr/bin/clang` on this machine is a 116 KB stub; the real compiler is the
Xcode toolchain binary above. LLVM is statically linked; the clang binary
depends on `libSystem`, `libc++`, `libz`, and `libresolv`.

### Vs GCC

No real GCC is installed here (`/usr/bin/gcc` is Apple Clang). Source and
binary figures are from published GCC 16 / distro packages.

| | Source | Binary |
|---|---|---|
| DaveCC | 309k `.c` | **4.6 MB**, 6 backends + linker |
| GCC C/C++ compiler proper | ~2.2M in `gcc/` + `libcpp/`, excluding Ada/D/Go/ObjC and tests (MaskRay `tokei` estimate; dated, still the right ballpark) | `gcc`/`g++` are tiny drivers; the compilers are `cc1` / `cc1plus` |
| Whole GCC tree | ~12–13M code lines (Open Hub; Fossies cloc of gcc-16.1.0) | — |

A stripped `cc1plus` for **one** target is typically **20–40 MB** (GCC 5.2 was
21 MB after `install-strip`; it has grown). Ubuntu 24.04 `gcc-14` for x86_64
is ~68 MB installed (`cc1` + `lto1` + plugins). Unstripped debug `cc1plus` is
often **100–500 MB**.

GCC C/C++ proper is about **7×** DaveCC source. One stripped `cc1plus` is
already **5–8×** the DaveCC binary, for one ISA and no linker.

### Linker

DaveCC has no standalone `ld`; the linker is built into `davecc`.

| Piece | `.c` | `.h` | Total |
|---|---:|---:|---:|
| `Linker/` | 8,933 | 871 | 9,804 |
| `c_compiler/ELF/` | 1,371 | 1,060 | 2,431 |
| `c_compiler/AR/` | 792 | 118 | 910 |
| `wasm32_link.c` + `.h` | 1,094 | 23 | 1,117 |
| **Linker + ELF + ar + wasm** | **12,190** | **2,072** | **14,262** |

`Linker/` `.c` files:

| File | Lines |
|---|---:|
| `linker.c` | 2,333 |
| `linker_dynamic.c` | 1,586 |
| `Architecture/linker_arch_aarch64.c` | 796 |
| `Architecture/linker_arch_riscv.c` | 718 |
| `linker_stacktrace.c` | 681 |
| `linker_main.c` | 662 |
| `Architecture/linker_arch_arm.c` | 467 |
| `linker_symbols.c` | 404 |
| `Architecture/linker_arch_pcode.c` | 398 |
| `Architecture/linker_arch_x86_64.c` | 309 |
| `linker_reloc.c` | 199 |
| `Architecture/linker_arch_6502.c` | 161 |
| `linker_config.c` | 130 |
| `linker_file.c` | 71 |
| `main.c` | 18 |

| Linker | Source | Binary |
|---|---|---|
| DaveCC | ~9k core, ~14k with ELF/ar/wasm; ELF + 6 ISAs + wasm | inside **4.6 MB** `davecc` |
| lld (Clang) | ~110k non-test (ELF + COFF + Mach-O + Wasm). lld/ELF was 21k in 2017 | often a small driver plus LLVM/`lld` shared libs; static builds are tens of MB |
| GNU ld (GCC) | `ld/` plus BFD — hundreds of thousands of lines. gold, the smaller ELF rewrite, was **198k** in 2017 (now deprecated) | ~**4.9 MB** for `ld` on a typical Linux (AOSC binutils 2.47, arm64) |
| Apple ld (what `clang` uses here) | new linker is closed source; classic ld64 is open | **2,192,304** bytes arm64 (`ld`, PROJECT:ld-1230.1); **3,302,464** bytes arm64 (`ld-classic`) |

DaveCC's linker is about **8×** smaller than lld and about **14×** smaller
than gold. A standalone GNU `ld` is already as large as all of DaveCC.
DaveCC is smaller in part because it is ELF + wasm + six ISAs, not a Mach-O
or COFF system linker and not a full GNU linker-script engine.

### Notes for later diffs

- DaveCC counts include blanks and comments; they match `wc -l`, not cloc's
  "code" column.
- Interpreters (`*_interpreter/`), libc, and test suites are excluded.
- Apple clang/ld sizes are fat Mach-O; always compare the arm64 slice.
- Clang/GCC comparison numbers will drift; re-check sources when adding a
  snapshot rather than only copying the previous comparison block.
