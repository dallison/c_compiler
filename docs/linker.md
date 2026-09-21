# Linker

DaveCC links ELF objects itself. The same engine runs inside `davecc` and as
the standalone program `daveld`. wasm32 is the exception: those objects are
modules, not ELF, and only the driver (`davecc`) links them.

```sh
davecc -target x86_64 program.c -o program          # compile, assemble, link
daveld -static -e main a.o b.o -o program           # link objects you already have
daveld -r a.o b.o -o combined.o                     # relocatable combine
davecc -target x86_64 -r a.c b.c -o combined.o      # same, from sources
```

`davecc -r` forwards `-r` to the linker and does not add CRT or libc.

To use the system ELF linker instead of daveld, pass `-fuse-ld=` on a
Linux target:

```sh
davecc -target aarch64-unknown-linux-davecc -fuse-ld=ld program.c -o program
davecc -target x86_64-unknown-linux-davecc -fuse-ld=/usr/bin/ld.lld foo.c
```

`-fuse-ld` accepts `ld`, `native`, `bfd`, `gold`, `lld`, or a path. The
driver still compiles and assembles, still adds the Linux CRT and
`libc*_linux.a`, then translates its daveld flags (`-I` becomes
`--dynamic-linker`, `-bind-now` becomes `-z now`, and so on) and execs
that linker. Apple `ld` is rejected: it links Mach-O, not ELF. This is
meant for Linux hosts; on macOS, point `-fuse-ld=` at a GNU/`lld` binary
or a wrapper that runs `ld` inside Colima. `-flto` objects stay with
daveld.

On Linux, omitting `-target` or passing `-fnative` (with or without a
bare host architecture such as `-target x86_64`) selects
`ARCH-unknown-linux-davecc`, links that profile's CRT and `libc*_linux.a`,
and execs host `ld`. An explicit Linux triple still uses daveld unless
you pass `-fuse-ld`. `-fnative` and `-fuse-ld` cannot be combined.

On macOS, `-target aarch64-apple-darwin-davecc` (OS `darwin` or `macos`;
vendor `apple` or `unknown`) is the Mach-O counterpart. That is also the
default on Apple Silicon when `-target` is omitted. `-fnative` with
`-target aarch64` selects the same profile. The assemblers write
`MH_OBJECT` files, daveld is not used, and the driver execs host `cc`
with `libcaarch64_darwin.a` (build `//:libc_aarch64_darwin`). C symbols
get the Darwin underscore. PIC is forced. There is no LTO on this path:
DCCLTO03 is not Mach-O or LLVM bitcode. `-fuse-ld` stays Linux/ELF-only
and cannot be combined with `-fnative`.

```sh
bazelisk build //:davecc //:libc_aarch64_darwin
davecc program.cc -o program
./program
```

Inspect the result with `elfdump` (`-H` header, `-S` sections, `-s` symbols,
`-l` segments, `-r` relocs). Flags for `elfdump` and the other host tools
are in [tools.md](tools.md).

## What it produces

| Mode | Flag | ELF type | Notes |
| --- | --- | --- | --- |
| Executable | default | `ET_EXEC` | Program headers, entry point, `chmod +x` |
| Shared object | `-shared` | `ET_DYN` | PIC image; no `PT_PHDR` |
| Dynamic executable | `-dynamic` | `ET_EXEC` | `PT_INTERP`, `PT_DYNAMIC`, GOT/PLT |
| Relocatable | `-r` / `--relocatable` | `ET_REL` | Concatenated objects; relocs kept |

Without `-r` the linker groups input sections by name, assigns virtual
addresses, applies relocations, and writes loadable segments. With `-r` it
still merges same-named sections, but values stay section-relative,
undefined and `COMMON` symbols stay unresolved, and there are no program
headers or entry point. Default output is `a.out`, or `a.o` with `-r`.

Relocations are `SHT_RELA` on 64-bit targets and Xtensa, `SHT_REL` on ARM
and i386.

## Architectures

Every ELF target the compiler emits:

| Target | Notes |
| --- | --- |
| x86_64, i386 / x86 | System V; i386 uses `SHT_REL` |
| AArch64, ARM | ARM also merges `.ARM.exidx` / `.ARM.extab` |
| RISC-V 64 and 32 | Same machine id; class picks the built-in map |
| 6502 / 65C02 | Static only; 64 KiB image limit |
| p-code, eBPF | Interpreter-profile maps |
| Xtensa / ESP32 | Static only; keeps `.xtensa.info` |

wasm32 is not ELF. Use `davecc` for those links.

## Inputs and libraries

Objects (`.o`), static archives (`.a`), and shared objects (`.so`) are
recognized by suffix. `-lname` searches `libname.so` then `libname.a` on
`-L` directories, then `/usr/lib` and `/lib`, then `LD_LIBRARY_PATH`.
`-whole-archive` / `-no-whole-archive` pull every member of the archives
that follow. Archives given without `-l` are linked the same way.

Undefined symbols are resolved from those archives. A static executable
fails the link if any remain (except undefined weaks). A DSO may leave
them for the loader.

## Command-line flags

These are the flags `daveld` and the driver both pass to the linker.
`-fuse-ld=` is a `davecc` driver flag only.

| Flag | Meaning |
| --- | --- |
| `-o FILE` | Output path |
| `-r`, `--relocatable` | Write one `ET_REL` object |
| `-shared` | Shared object |
| `-static` / `-dynamic` | Fully static vs dynamic |
| `-e SYMBOL` | Entry (`_start` by default; overrides `ENTRY()`) |
| `-lNAME`, `-LDIR` | Library search |
| `-IINTERP` | `PT_INTERP` path (joined: `-I/lib/ld.so`) |
| `-T FILE`, `--script`, `--script=` | GNU ld / LLVM lld script |
| `-tLAYOUT`, `--layout=` | Built-in layout when no `-T` is given |
| `-rpath PATH` | Runtime search path (`$ORIGIN` is accepted) |
| `-origin ADDR` | Load-address origin |
| `-bind-now` | Resolve dynamic symbols at load |
| `-defer-init` | Let the program run init arrays itself |
| `-whole-archive` / `-no-whole-archive` | Archive member policy |
| `--gc-sections` / `--no-gc-sections` | Drop unreferenced allocatable sections |
| `--print-gc-sections` | Report what GC discarded |
| `-chdir DIR` | `chdir` before opening inputs |
| `-Xsymbol-tables`, `-Xrelocations`, `-Xsections` | Debug dumps |

`davecc` also accepts `-Wl,arg` to pass a token through. `-t` on `davecc`
is the compiler's own flag (not layout); use `-Wl,-tprogram` or `daveld
-tprogram` for a built-in layout, or `-T` / `-Wl,-T` for a script.

## Layouts

With no `-T`, the linker loads a built-in GNU-style script for the object's
machine and ELF class. The layout name is `program` unless you pass
`-t` / `--layout=`.

| Layout | Who uses it | What it describes |
| --- | --- | --- |
| `program` | Every ELF target | RAM image: code then data |
| `rom` | 6502 / 65C02 only | Support ROM at `0xc000`, boot at `0xff00`, vectors at `0xfffa` |
| `introm` | 6502 / 65C02 only | Internal ROM through `0xfff9`, vectors at `0xfffa` |

Typical `program` maps (LENGTH `0` means unbounded):

| Target | Text `ORIGIN` | Data `ORIGIN` |
| --- | --- | --- |
| x86_64, AArch64, RV64, p-code, BPF | `0x400000000` | `0x410000000` |
| RV32, ARM | `0x40000000` | `0x41000000` |
| i386 | `0x08048000` | `0x09000000` |
| 6502 / 65C02 | `0x800` | immediately after text |
| ESP32 | IRAM `0x40080000` | DRAM `0x3ffb0000` |

Those scripts also place `.rodata`, `.eh_frame`, `.gcc_except_table`,
`.davecc_stacktrace`, GOT/PLT, init arrays, and `.bss` / `COMMON`. They
discard `.comment` and `.note*`. ARM adds `.ARM.exidx*` / `.ARM.extab*`;
p-code adds `.davecc_except_table`; ESP32 adds `.literal*` and
`.xtensa.info`.

A `-T` script replaces the built-in map entirely. `-e` still wins over
`ENTRY()`.

## How a script is applied

DaveCC is not a full BFD linker-script virtual machine. It reads the GNU
subset below and turns it into four segment kinds:

| Segment | How the script selects it |
| --- | --- |
| Text | Memory attrs include `x`, or a `PT_LOAD` phdr with the execute flag |
| Data | Memory attrs include `w` (and not `x`), or a writable `PT_LOAD` |
| Dynamic | A `PT_DYNAMIC` phdr |
| Interp | A `PT_INTERP` phdr |

Output sections listed after `> region` land in that memory region. Input
globs (`*(.text*)`) are the section names the linker will merge into that
region. `/DISCARD/` globs are dropped. Assignments become linker-defined
symbols (`_etext = .`, `PROVIDE(_end = .)`, or an absolute value).

Things the parser accepts for compatibility but does **not** implement:
`KEEP` (GC is `--gc-sections` only), `SORT*`, `EXCLUDE_FILE`,
`BYTE`/`SHORT`/`LONG`/`QUAD`/`FILL`, `OVERLAY`, `ASSERT`, `INPUT`/`GROUP`,
`OUTPUT_FORMAT`, `ADDR()`/`SIZEOF()`/`ALIGNOF()`/`LOADADDR()` (those
functions evaluate to 0). `VERSION`, `INSERT`, and
`INPUT_SECTION_FLAGS` are not part of the language.

The location counter `.` is not a live address during parse. In an
expression it is 0. `_name = .` and `PROVIDE(_name = .)` inside an output
section mean “address after the sections collected here”; at the top of
`SECTIONS`, `PROVIDE(_end = .)` means “address after the whole image”.

## Linker script format

Scripts are the usual GNU ld / LLVM lld text. Comments are `/* … */`, `//`
to end of line, or `#` to end of line. Identifiers and `"strings"` are
accepted in the same places. `INCLUDE` is limited to 8 nested files;
a relative name is first tried next to the including script, then as given.

### Top-level commands

```
ENTRY(symbol)
MEMORY { … }
PHDRS { … }
SECTIONS { … }
INCLUDE filename
REGION_ALIAS("alias", region)
OUTPUT_ARCH(…) / OUTPUT_FORMAT(…) / SEARCH_DIR(…) / STARTUP(…) / TARGET(…) / OUTPUT(…)
INPUT(…) / GROUP(…) / AS_NEEDED(…)
EXTERN(…)
name = expression;
PROVIDE(name = expression);
PROVIDE_HIDDEN(name = expression);
HIDDEN(name = expression);
```

`ENTRY` sets the default entry if you do not pass `-e`. The `OUTPUT_*` /
`INPUT` / `GROUP` / `EXTERN` forms are parsed and ignored.

### MEMORY

```
MEMORY
{
  name (attrs) : ORIGIN = expr, LENGTH = expr
  name (attrs) : org = expr, len = expr
}
```

`ORIGIN`/`org`/`o` and `LENGTH`/`len`/`l` are interchangeable. `LENGTH` may
be omitted (unbounded). Attrs are the GNU letters `r`, `w`, `x`, optionally
with `!` to invert (`rw!x`). They decide text vs data when no `PHDRS` type
says otherwise.

`REGION_ALIAS("flash", rom)` makes `> flash` mean the same region as `rom`.

### PHDRS

```
PHDRS
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
  dynamic PT_DYNAMIC;
  interp PT_INTERP;
}
```

Types may be names (`PT_LOAD`, `PT_DYNAMIC`, `PT_INTERP`) or the numeric
values 1, 2, 3. `FLAGS(n)` bit 0 is execute and selects text vs data for a
`PT_LOAD`. `FILEHDR`, `PHDRS`, and `AT(expr)` are accepted and ignored.
DaveCC still emits the program headers its image needs; this block classifies
regions, it does not author an arbitrary phdr table.

### SECTIONS

```
SECTIONS
{
  .text : { *(.text .text.*) *(.rodata*) } > text :text
  .data : { KEEP(*(.data)) *(.got) } > data :data
  .bss : { *(.bss* COMMON) } > data
  /DISCARD/ : { *(.comment .note*) }
  _etext = .;
  PROVIDE(_end = .);
}
```

An output section is:

```
name [addr] [ALIGN(n)] [SUBALIGN(n)] [ONLY_IF_RO|ONLY_IF_RW] [(NOLOAD)] :
  [AT(expr)]
{
  contents
} [> memory] [:phdr] [AT> load_memory]
```

`ALIGN` / `SUBALIGN` / `ONLY_IF_*` / `(NOLOAD)` / `AT()` / `AT>` are parsed.
The assignment that matters for layout is `> memory` (and `:phdr` for
segment class). Inside the braces the useful contents are:

| Construct | Effect |
| --- | --- |
| `*(.text .rodata)` | Input sections matching those globs |
| `*(.text*)` | Same, GNU glob (`*`, `?`, `[abc]`, `[a-z]`) |
| `KEEP(…)`, `SORT(…)`, `SORT_BY_NAME(…)` | Parsed; patterns are kept, order is input order |
| `. = ALIGN(n);` | Trailing alignment of that output section |
| `sym = .;` / `sym = ALIGN(n);` | Symbol at the end of the collected sections |
| `sym = expr;` | Absolute symbol |
| `PROVIDE(sym = .)` | Define `sym` only if no object already did |
| `/DISCARD/` as the output name | Drop matching input sections |

`COMMON` is the usual name for common-symbol storage. File-name prefixes on
input sections (`foo.o(.text)`) are not a separate matching dimension: write
the section glob only.

### Expressions

Used for `ORIGIN`, `LENGTH`, `FLAGS`, and absolute assignments:

| Syntax | Meaning |
| --- | --- |
| number | Decimal, `0x` hex; suffixes `K` / `M` / `G` (1024) |
| `+ - * / << >> & \| ~` | Integer operators |
| `a ? b : c` | Ternary |
| `ALIGN(n)` / `ALIGN(v, n)` / `BLOCK(n)` | Align 0 or `v` up to `n` |
| `MAX(a, b)`, `MIN(a, b)` | |
| `ORIGIN(name)`, `LENGTH(name)` | Memory region |
| `CONSTANT(MAXPAGESIZE)` | Target page size (any argument is accepted) |
| `SIZEOF_HEADERS` | ELF + program-header span for the machine |
| `ABSOLUTE(expr)` | Same as `expr` |
| `DEFINED(name)` | 1 if that script symbol already exists |
| a previously assigned script symbol | Its absolute value |

### Example

A small 65C02 ROM map:

```
ENTRY(_start)

MEMORY
{
  rom (rx) : ORIGIN = 0xc000, LENGTH = 0x3f00
  boot (rx) : ORIGIN = 0xff00, LENGTH = 0xfa
  vec (r)  : ORIGIN = 0xfffa, LENGTH = 6
}

SECTIONS
{
  .text : { *(.text* .rodata*) } > rom
  .boot : { *(.boot) } > boot
  .hwvectors : { *(.hwvectors) } > vec
  /DISCARD/ : { *(.comment .note*) }
  PROVIDE(_end = .);
}
```

```sh
daveld -T rom.ld -static -e _start start.o main.o -o rom.elf
# or
davecc -target 65c02 -nostdlib -Wl,-T -Wl,rom.ld start.s main.c -o rom.elf
```

A hosted-style RAM image (same shape as the built-in `program` layout):

```
ENTRY(_start)
PHDRS
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
  dynamic PT_DYNAMIC;
  interp PT_INTERP;
}
MEMORY
{
  text (rx) : ORIGIN = 0x400000000, LENGTH = 0
  data (rw) : ORIGIN = 0x410000000, LENGTH = 0
}
SECTIONS
{
  .text : { *(.text* .rodata* .eh_frame*) } > text :text
  .data : { *(.data* .got .got.plt) } > data :data
  .bss  : { *(.bss* COMMON) } > data
  _etext = .;
  PROVIDE(_end = .);
}
```
