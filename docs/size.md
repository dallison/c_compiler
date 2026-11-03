# DaveCC size snapshots

Physical line counts (`wc -l`) and binary sizes for DaveCC, compared with
Clang/LLVM and GCC. Newest snapshot first.

These are scope indicators, not quality metrics. Clang and GCC implement more
languages, object formats, and targets. DaveCC's compiler binary also includes
its linker and its backends.

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

## 2026-10-30

Host: macOS arm64 (darwin 25.6.0). DaveCC binary from `bazel-bin/davecc`,
Mach-O arm64, mtime 2026-10-30 00:12 UTC. No compiler source is newer than
that binary.

### DaveCC compiler

Trees: `c_compiler/`, `Linker/`, `davecc/`.

| | Files | Lines |
|---|---:|---:|
| `.c` | 291 | 436,218 |
| `.h` | 259 | 27,743 |
| **Total** | **550** | **463,961** |

| Tree | `.c` files | `.c` lines | `.h` files | `.h` lines |
|---|---:|---:|---:|---:|
| `c_compiler/` | 270 | 420,639 | 239 | 26,712 |
| `Linker/` | 20 | 12,631 | 20 | 1,031 |
| `davecc/main.c` | 1 | 2,948 | 0 | 0 |

**Executable:** 6,906,376 bytes (6.6 MB). Depends on `libSystem`, `libc++`,
`Foundation`, and `libobjc`.

Since 2026-09-09: source 330,653 → 463,961 lines (+40%), binary 4,838,136 →
6,906,376 bytes (+43%).

### Vs Clang / LLVM

Apple clang on this machine is unchanged from the September snapshot:
`clang-1700.4.4.1`, dated 2025-11-07. `/usr/bin/clang` is still a 116 KB
stub. LLVM is statically linked; the clang binary depends on `libSystem`,
`libc++`, `libz`, and `libresolv`.

Whole-tree source counts below are [OctoCounts](https://octocounts.com/github/llvm/llvm-project)
tokei **code** lines (comments, blanks, tests, docs, and generated files
excluded), not `wc -l`. DaveCC's 464k is physical lines, so these ratios
understate Clang/LLVM.

| | Source | Binary |
|---|---|---|
| DaveCC | 436k `.c` (464k with headers); linker + 6502, ARM, AArch64, x86, RISC-V, BPF, Xtensa, p-code, wasm32 | **6.6 MB** arm64 |
| llvm-project `main` | **8.37M** code lines (commit `2e800186812e`, 2026-10-30). Open Hub's older ~12.9M figure counted a wider tree | Apple clang 17.0.0: **125 MB** arm64 slice (130,634,640 bytes; universal 269,128,592) |
| Clang frontend, LLVM core, lld | September component split (~1.89M / ~873k / ~110k, ~2.9M together) was not re-counted file by file | linked into the clang binary above |

The arm64 clang binary is about **19×** DaveCC (was **27×** on 2026-09-09;
clang did not change, DaveCC grew). llvm-project's 8.37M code lines are
about **18×** DaveCC's 464k physical lines.

### Vs GCC

No real GCC is installed (`/usr/bin/gcc` is Apple Clang). Source figures are
published counts of the GCC tree, same tokei rules as LLVM where noted.

| | Source | Binary |
|---|---|---|
| DaveCC | 436k `.c` (464k with headers) | **6.6 MB**, linker + the backends above |
| GCC tree | **11.35M** tokei code lines (`gcc-mirror/gcc` commit `b76d14f77337`, 2026-10-30). Open Hub, analyzed the same day, reports **12.06M** code lines. Fossies cloc of the gcc-16.1.0 tarball (2026-04-30, tests included) was **13.2M** code lines | `gcc`/`g++` are tiny drivers; the compilers are `cc1` / `cc1plus` |
| GCC C/C++ proper | September's ~2.2M estimate for `gcc/` + `libcpp/` was not re-counted | one target only |

A March 2025 GCC patch measured **x86_64 `cc1plus` text at 26,205,707 bytes**
after a size tweak. That text section alone is about **4×** the entire
DaveCC binary, for one ISA and no linker. A stripped `cc1plus` file is
larger than its text (the old 20–40 MB installed range still fits).
Unstripped `cc1` / `cc1plus` / `lto1` in Debian gcc-15 were large enough
to triple a minimal buildd chroot.

GCC's whole tree is about **24×** DaveCC on tokei code lines versus DaveCC
physical lines. The old ~2.2M C/C++-proper estimate would be about **5×**
DaveCC's `.c` lines; that slice was not recounted.

### Linker

The linker is built into `davecc` and also shipped as standalone `daveld`.

| Piece | `.c` | `.h` | Total |
|---|---:|---:|---:|
| `Linker/` | 12,631 | 1,031 | 13,662 |
| `c_compiler/ELF/` | 1,425 | 1,125 | 2,550 |
| `c_compiler/AR/` | 792 | 118 | 910 |
| `wasm32_link.c` + `.h` | 1,144 | 23 | 1,167 |
| **Linker + ELF + ar + wasm** | **15,992** | **2,297** | **18,289** |

`Linker/` `.c` files:

| File | Lines |
|---|---:|
| `linker.c` | 2,905 |
| `linker_script.c` | 2,211 |
| `linker_dynamic.c` | 1,669 |
| `Architecture/linker_arch_aarch64.c` | 856 |
| `Architecture/linker_arch_riscv.c` | 786 |
| `linker_stacktrace.c` | 750 |
| `Architecture/linker_arch_arm.c` | 499 |
| `Architecture/linker_arch_pcode.c` | 401 |
| `linker_symbols.c` | 400 |
| `Architecture/linker_arch_x86_64.c` | 377 |
| `linker_main.c` | 316 |
| `Architecture/linker_arch_x86.c` | 302 |
| `linker_config.c` | 259 |
| `linker_reloc.c` | 203 |
| `Architecture/linker_arch_bpf.c` | 173 |
| `Architecture/linker_arch_6502.c` | 170 |
| `linker_gc.c` | 167 |
| `Architecture/linker_arch_xtensa.c` | 98 |
| `linker_file.c` | 71 |
| `main.c` | 18 |

| Linker | Source | Binary |
|---|---|---|
| DaveCC | ~14k core, ~18k with ELF/ar/wasm | inside **6.6 MB** `davecc` |
| lld | ~110k non-test as of the September note; not re-counted | static builds are tens of MB |
| Apple ld (what `clang` uses here) | new linker is closed source | **2,192,304** bytes arm64 (`ld`, same 2025-11-07 binary); **3,302,464** bytes arm64 (`ld-classic`) |

Linker source is 18,289 lines, up from 14,262 in September. Apple `ld`'s
arm64 slice (2.1 MB) is about a third of the whole DaveCC binary. A
standalone GNU `ld` was ~4.9 MB in the September note (AOSC binutils 2.47,
arm64), still the same order as all of DaveCC.

### Notes for later diffs

- DaveCC counts include blanks and comments; they match `wc -l`, not cloc's
  "code" column. The OctoCounts figures above are the opposite: tokei code
  lines, tests excluded.
- Interpreters (`*_interpreter/`), libc, and test suites are excluded.
- Apple clang/ld sizes are fat Mach-O; always compare the arm64 slice.
- Clang/GCC comparison numbers will drift; re-check sources when adding a
  snapshot rather than only copying the previous comparison block.

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

The linker is built into `davecc` and also shipped as standalone `daveld`.
`daveld -r` writes a relocatable object (GNU `ld -r`).

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
