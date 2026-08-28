# wasm32 Backend Implementation Plan

Branch: `feature/wasm32-backend`
Worktree: `/Users/FZDSZZ/c_compiler-wasm32`

## Goal

A `-target wasm32` backend that emits genuine `.wasm` modules runnable by
unmodified WebAssembly engines (Wasmtime, browsers, Node), with the C test
suite passing at a rate comparable to the RISC-V target.

Emitting real WebAssembly rather than an ELF-hosted wasm-like VM is the whole
point of the exercise. A stack VM inside an ELF container would duplicate what
`c_compiler/p_code/` already provides; the value here is portability and, just
as importantly, having a mature external engine act as the reference
implementation during bring-up.

## Non-goals for the first pass

- C++ exceptions. `wasm32` starts with exceptions disabled, the way 6502/65c02
  do today (`c_compiler/driver/compiler.c:3229`).
- `setjmp`/`longjmp`. Nothing in `libc/*.c` calls them, so the target can ship
  without them; `<setjmp.h>` support waits for the stack-switching or EH work.
- Threads, atomics, SIMD, memory64, garbage collection.
- Dynamic linking. Static linking only.
- Debug info. No DWARF, no name section beyond function names.

## Design decisions

### Output container: real `.wasm`, with a purpose-built linker

The existing `Linker/` cannot be reused. It patches a fixed-width field at a
fixed offset in an already-laid-out section (`Linker/linker_reloc.c:171-186`),
has no relaxation framework, and resolves symbols to byte addresses. Wasm needs
index-space renumbering and has no byte addresses for code.

But most of the mismatch dissolves if we follow the standard wasm linking
convention:

- Every relocatable LEB128 is emitted **padded to a fixed 5 bytes**, so patching
  never resizes anything.
- A wasm code section is a vector: a count followed by consecutive
  `(size, body)` entries. Bodies from separate objects therefore concatenate
  directly; only the leading count needs rewriting. The same holds for data and
  element segments.

So `wasm32_link` is a genuinely small program: read objects, resolve symbols,
assign final indices, concatenate section payloads, patch the padded slots,
write the module. Archives come free — `c_compiler/AR/ar.h` is format-agnostic,
and the lazy member pull-in in `Linker/linker.c:1021-1090` is the algorithm to
copy (it gives translation-unit-granularity dead code elimination, which is what
the libc archive layout already assumes).

### Runtime: WASI preview 1, no custom embedder

The test harness invokes the interpreter as
`$INTERPRETER [-rom ROM] ${INTERP_ARGS[@]} $bin` with the guest binary always
last (`c_testsuite/run_single_exec.sh:206-217`). A two-line wrapper around the
stock `wasmtime` binary drops straight into that slot.

That argues for targeting WASI rather than inventing a `__davecc_syscall`
import backed by a custom Wasmtime embedder. WASI costs a new
`libc/wasi_syscall.c` mapping the existing `DAVE_SYS_*` guest ABI onto
preview 1 calls — a fourth syscall profile alongside the guest-interpreter,
native-Linux, and host profiles already in `libc/syscall.c`. In exchange there
is no host-side code to build or maintain, and the output runs anywhere.

Fallback if WASI turns out to be a poor fit for some syscall: add a single
`env.__davecc_syscall` import and a small embedder. Keep the syscall layer
behind the existing `DAVE_SYS_*` seam so this stays a one-file swap.

### Register model: wasm locals are the register file

Wasm locals are an unbounded, typed, function-scoped array — exactly the
infinite register file `p_code` already assumes with its 256 registers per
class. There is no allocation, no spilling, no coalescing, no caller/callee
split. `wasm32_reg_alloc.c` is a numbering pass: assign each value-producing
instruction a local index within its type class (i32/i64/f32/f64) and emit the
local declaration vector. Live-range reuse to shrink the local count is a
size optimization to defer.

### Frame model: everything on the shadow stack first, promote later

The wasm call stack is not addressable, so anything address-taken lives in
linear memory behind a mutable global stack pointer. `TargetGenerator` already
models this with its `frame_pointer` / `stack_pointer` pseudo-instructions and
`stack_frame_size` field.

Start by giving **every** local a shadow-stack slot, mirroring the
`variable_pool` loop at the top of `PCodeLower`
(`c_compiler/p_code/p_code_codegen.c:2678-2748`). That is simple and correct.
A later milestone promotes locals that are never the operand of an
`IR_OP(addressof)` into real wasm locals, which is where most of the
performance is.

### Control flow: a stackifier, in two passes

This is the only part with no counterpart anywhere in the tree, and it is where
the schedule risk lives.

**Pass A — structuring.** Input is the `TargetBasicBlock` CFG built by the
shared `TargetBuildBasicBlocks`. Order blocks so each loop's blocks are
contiguous, place a `loop` scope around each loop header's region, place `block`
scopes so every forward branch has a properly nested target, then rewrite
branches as `br`/`br_if`/`br_table` with relative scope depths. Irreducible
loops are detected and rejected with a clear diagnostic until M11 adds the
dispatch-loop fixup.

**Pass B — operand stackification.** Per block, walk backward and fold a
definition into its use's operand position when the def is single-use, in the
same block, adjacent in schedule order, and has no intervening side effects.
Everything not folded becomes `local.set` / `local.get`.

Both live in `wasm32_stackify.c` and get their own tests. Every milestone gate
runs `wasm-validate`, which type-checks the whole module and catches the
overwhelming majority of stackifier bugs structurally rather than as wrong
answers at runtime.

### Target configuration

| Field | Value | Note |
|---|---|---|
| `name` | `"wasm32"` | |
| `pointer_size` | 4 | |
| `short_size` / `int_size` / `long_size` | 2 / 4 / 4 | ILP32 |
| `long_long_size` | 8 | |
| `float_size` / `double_size` | 4 / 8 | no `long double`; alias to `double` |
| `bool_size` / `wchar_size` | 1 / 4 | |
| `stack_alignment` | 16 | matches the clang wasm ABI |
| `alignment` | 8 | |
| `plain_char_is_signed` | true | matches clang wasm |
| `keep_ssa` | false | phis must become local writes before lowering |
| `prepend_underscore` | false | |
| `call_return_fixed_reg` | start `true` | copies pcode; revisit if lowering fights it |

IR optimizations: enable `gvn`, `sccp`, `const_prop`, `dce`, `copy_prop`.
Disable `loop_preheaders`, `induction_vars`, `derived_induction_vars` for the
same reason pcode does (`p_code_target.c:85-89`) — the stackifier is far more
sensitive to loop shape than pcode's lowering is. Treat `code_motion` and
`tail_call` as suspects to disable at the first sign of a malformed CFG.

## File layout

New directory `c_compiler/wasm32/`, following the pattern every backend uses:

| File | Est. lines | Purpose |
|---|---|---|
| `wasm32_target.c` | 130 | `CompilerTarget` vtable; copy `p_code_target.c` |
| `wasm32_machine.h` | 250 | Opcode enum and wasm binary opcode constants |
| `wasm32_codegen.c/h` | 3500-5000 | IR lowering, ABI, shadow stack, varargs |
| `wasm32_stackify.c/h` | 1500-2500 | Control-flow structuring and operand stackification |
| `wasm32_reg_alloc.c/h` | 300 | Local numbering |
| `wasm32_emitter.c/h` | 600 | Textual `.wat`-style output for `-S` and debugging |
| `wasm32_module.c/h` | 1200 | Binary module/object writer, LEB128, relocations |
| `wasm32_optimize.c/h` | 200 | Peephole; can stay near-empty for a long time |

Outside that directory:

- `wasm32_link/` — the wasm linker, ~1500 lines, plus a `cc_binary`.
- `wasm32 support/` — startup and runtime, target-specific.
- `libc/wasi_syscall.c` — WASI preview 1 mapping, ~500 lines.
- `tools/wasm32run.sh` — wrapper that execs `wasmtime run --dir=. "$@"`.

Registration touchpoints, all one-liners:

- `compiler_targets[]` at `c_compiler/driver/compiler.c:243`, which already
  carries the comment `// Add new targets here.`
- `PreprocessorDefineArchitectureMacros` at
  `c_compiler/frontend/lexical/preprocessor.c:537-588` — define `__wasm__`,
  `__wasm32__`, `__WORDSIZE=32`, `__ILP32__`.
- The `:compiler` `cc_library` srcs in `BUILD.bazel` (pcode's block is at
  lines 698-722; add the wasm32 files alongside).
- The `Link()` call at `davecc/main.c:1692` needs a wasm branch dispatching to
  `wasm32_link`. It takes a string argv, so the wasm linker should accept the
  same argument vector rather than a bespoke interface.

The `CompilerTarget` opcode contract: `wasm32_machine.h`'s opcode enum must
mirror the first **32** entries of `TargetOpcode`
(`c_compiler/backend/target_generator.h:21-74`), `save` through `fvarreg`, in
that exact order. Names may differ, positions may not.

The `common_emitter.c` data hooks (`EmitStaticVariable`, `EmitBSSVariable`,
`EmitDataStart`, `EmitStringLiteralSection`, `EmitLiteral`) all emit GNU
assembler text — `.data`, `.comm`, `.section ".rodata"`. None of it is reusable.
wasm32 supplies its own versions that accumulate data segments.

## Status

Everything through code generation is done: the backend compiles a large C
subset to `.wasm` modules that `wasm-validate` accepts and Wasmtime runs.
`wasm32_testsuite/run_exec_tests.sh` builds each of its 68 programs twice,
once with the host clang and once with davecc, and requires the two to agree
on the exit status; all 68 pass.

Working: integer and floating-point arithmetic in all four wasm types,
comparisons, conversions, sub-word loads and stores, arbitrary control flow
including irreducible graphs, the shadow stack, pointers, arrays, unions,
bit fields, string literals and static data with pointer initializers,
direct and indirect calls, function pointers through the table, structs
passed and returned by value, varargs, and variable-length arrays.

Not done, in the order they matter: linking (a module is one translation
unit today, and calls or data references that leave it are a clean error),
libc and WASI, C++ exceptions, `setjmp`/`longjmp`, threads and atomics.
Two known inefficiencies: every local lives on the shadow stack rather than
in a wasm local, and control flow always goes through a dispatch loop instead
of recovering the natural block structure. Both are correctness-preserving
and worth revisiting only with a benchmark in hand.

## Milestones

Each milestone has a gate that must pass before moving on. Every gate includes
`wasm-validate` on the produced module.

**M0 — Toolchain and skeleton.**
`brew install wasmtime wabt binaryen`. Register the target, add the
preprocessor defines, stub every `CompilerTarget` hook. Gate: `-target wasm32`
is accepted and `davecc -target wasm32 -S hello.c` runs to completion without
crashing.

**M1 — The spine.**
`int main(void) { return 42; }` through codegen, module writer, and a
single-object link, to a `.wasm` that Wasmtime runs. Gate:
`wasm-validate` clean and `wasmtime run a.wasm` exits 42. This is the milestone
that flushes out container and driver-integration surprises, so do not defer it.

**M2 — Integer arithmetic and locals.**
Straight-line `i32`/`i64` arithmetic, comparisons, shifts, conversions,
sub-word loads and stores via the narrow load/store variants. Sub-word handling
mirrors RISC-V, which has the same no-`i8`/`i16`-registers property. Gate: an
arithmetic test set returning computed exit codes.

**M3 — Control flow.**
Stackifier passes A and B. `if`/`else`, `while`, `for`, `do`, `switch` via
`br_table`, `&&`/`||`, `goto` within reducible bounds. Irreducible CFGs produce
a clear "not yet supported" diagnostic. Gate: control-flow test set plus
`wasm-validate` on every case.

**M4 — Memory.**
Shadow stack prologue/epilogue, address-taken locals, pointers, arrays,
structs, `memcpy`/`memzero` lowering. Gate: pointer and aggregate tests.

**M5 — Calls, ABI, and floats.**
Direct calls, parameter passing, return values, struct parameters passed
indirectly, `sret` for struct returns, recursion, multi-function modules.
Float and double ops land here too since they map one-to-one onto wasm
instructions. Gate: recursive fib, struct pass/return, float arithmetic.

**M6 — Data.**
Data segments, globals, string literals, BSS, static initializers. Gate:
string and global tests.

**M7 — Function pointers.**
Element segment construction, `call_indirect` with type checking, one table
slot per address-taken function. Gate: function-pointer and callback tests,
including a qsort comparator.

**M8 — Linking.**
Wasm object format with `linking` and `reloc.*` custom sections, 5-byte padded
LEB128 for every relocatable index, `wasm32_link` with symbol resolution and
lazy archive member pull-in. Gate: a two-translation-unit program plus a static
archive links and runs.

**M9 — libc.**
The `__wasm32__` ILP32 branch in the headers (the existing `#else` path that
arm already exercises should mostly just work), `libc/wasi_syscall.c`,
`malloc` backed by `memory.grow` in place of the `ExpandHeap` stub at
`libc/malloc.c:469-472`, and `wasm32 support/` startup. Gate: a
`libc_wasm32_compile_test` mirroring `libc_pcode_compile_test`, then a
printf smoke test mirroring `pcode_runtime_smoke_test`.

**M10 — c_testsuite.**
Wire `single_exec_wasm32` via `ctestsuite_sh_test` with `interpreter_label`
pointing at the `wasmtime` wrapper, add `skip/davecc-wasm32.skip`, and grind
the failures down. Gate: pass rate comparable to riscv, which currently skips
nothing.

**M11 — The long tail.**
Irreducible control flow via a dispatch loop; promoting non-address-taken
locals out of the shadow stack; C++ without exceptions against
`//cxx_testsuite:syntax`; then exception handling with tags and `try_table`,
which needs none of `libc/eh_frame.c`, `libc/eh_frame_pointer.c`, or
`eh_transfer.s` because the engine owns unwinding.

## Where the implementation departed from this plan

**The dispatch loop came first, not last.** M3 planned to recover natural
loop and block structure and to reject irreducible graphs until M11. Building
the dispatch loop instead turned out to be both simpler and complete: one
`loop` wrapping a nest of `block`s, a selector local naming the next basic
block, and a `br_table` dispatching on it. Any CFG at all works, including
irreducible ones, and there is no diagnostic to write and later delete.
Recovering natural structure is now purely a code-quality change.

**Pass B of the stackifier does not exist.** Every value round-trips through
a local. Wasm engines fold those away in their own register allocation, so
this costs module size rather than speed, and it removed the pass most likely
to produce subtly wrong operand ordering.

**Two IR-shaped surprises worth recording.** `memzero` carries only its
destination — the length comes from the destination's own type, and reading a
second input crashes. And the instruction that selects a wasm opcode has to be
chosen from the wasm type of the operands, never from the C type's byte size:
a pointer whose C type is a large array or struct is still an `i32`, and
sizing off `type->size` silently produces `i64` arithmetic on `i32` values.

**Bit fields, variable-length arrays, and varargs were not in the plan** but
fell out cheaply once the shadow stack existed. Variadic arguments go in a
frame buffer whose address is an extra trailing parameter, so a wasm signature
stays fixed however many arguments a call passes.

## Risks

**The stackifier is the schedule.** Everything else is mechanical work with an
existing template. If a milestone slips, it will be M3 or M11. Mitigate by
building the `wasm-validate` gate into the very first test script, so
structural bugs surface as validation errors rather than wrong answers.

**Silent LEB128 padding bugs.** A missed pad produces a module that validates
cleanly but calls the wrong function. Assert in the writer that every
relocatable value is emitted through the padded path.

**Loop-shape sensitivity of IR passes.** pcode already had to disable three
optimizations to protect its lowering. Expect the stackifier to be pickier and
to find at least one more pass that produces CFGs it handles badly.

**Entry point convention.** The `-Wl,-e -Wl,main` flags that x86_64 and pcode
pass do not apply; wasm uses an exported `_start` or the `start` section.
Decide this at M1, not later.

## Runbook

```bash
cd /Users/FZDSZZ/c_compiler-wasm32
brew install wasmtime wabt binaryen
bazel build //:davecc

# The differential exec tests: every program is compiled by both clang and
# davecc, and the two exit statuses have to match.
./wasm32_testsuite/run_exec_tests.sh
./wasm32_testsuite/run_exec_tests.sh bazel-bin/davecc wasm32_testsuite/tests/c06_switch.c

# Compile and run one file today, with no libc
bazel-bin/davecc -target wasm32 -c program.c -o program.wasm
wasm-validate program.wasm
wasmtime run --invoke main program.wasm

# Compile and run a single file (target state, from M9 onward)
bazel-bin/davecc -target wasm32 -static -isystem libc/include \
  program.c bazel-bin/libc/libcwasm32.a -o program.wasm
wasm-validate program.wasm
wasmtime run program.wasm

# Inspect
wasm2wat program.wasm | less
wasm-objdump -x program.wasm

# One c_testsuite case (from M10 onward)
bash c_testsuite/run_single_exec.sh \
  --davecc bazel-bin/davecc --target wasm32 \
  --libc bazel-bin/libc/libcwasm32.a \
  --interpreter tools/wasm32run.sh \
  --skip c_testsuite/skip/davecc-wasm32.skip \
  --compile-arg -target --compile-arg wasm32 --compile-arg -static \
  --suite-root c_testsuite --start 1 --end 1
```

## Reference: templates to copy

| Need | Copy from |
|---|---|
| `CompilerTarget` wiring | `c_compiler/p_code/p_code_target.c` (116 lines, complete) |
| Lowering skeleton, `LowerIRNode` dispatch | `c_compiler/p_code/p_code_codegen.c:2296-2748` |
| `TargetVirtuals` instance | `c_compiler/p_code/p_code_codegen.c:418-449` |
| Emitter loop | `c_compiler/p_code/p_code_emitter.c:530-553` |
| Sub-word integer handling | `c_compiler/risc_v/risc_v_codegen.c` |
| Varargs on a shadow stack | `c_compiler/p_code/p_code_codegen.c:2224-2279` |
| Archive lazy pull-in | `Linker/linker.c:1021-1090` |
| libc archive genrule | `libc_pcode` in `BUILD.bazel:2370-2419` |
| Test wiring | `ctestsuite_sh_test` in `c_testsuite/ctestsuite.bzl` |
