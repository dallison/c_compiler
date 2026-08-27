# GCC frontend sweep findings

This is a handoff for fixing davecc crashes and hangs found by the external GCC
test runner. GCC sources are intentionally outside this repository at:

```text
/Users/FZDSZZ/gcc-test/gcc/testsuite
```

The runner was added in commit `3944b58` and lives at
`gcc_testsuite/run_dg_tests.py`.

## Required workflow

- Take one candidate at a time.
- Keep GCC source files outside this repository.
- Add only a small minimized regression to `c_testsuite` or `cxx_testsuite`.
- Run davecc through the GCC runner or through `subprocess.Popen` with
  `start_new_session=True`; on timeout, call `os.killpg(pid, SIGKILL)`.
- After a fix passes its focused regression and the relevant existing suite,
  commit it before starting the next candidate.
- Do not solve deep parser nesting by merely increasing the process stack.

Build before reproducing:

```sh
bazel build //:davecc
```

### Validating a codegen fix against a baseline

A suite-wide pass count hides which tests moved, so run the four exec suites and
the five single-exec suites twice -- once with the change stashed -- and compare
the two lists of failures rather than the two totals.

```sh
bazel test //cxx_testsuite:exec_x86_64 //cxx_testsuite:exec_aarch64 \
  //cxx_testsuite:exec_arm //cxx_testsuite:exec_riscv \
  //c_testsuite:single_exec_aarch64 //c_testsuite:single_exec_arm \
  //c_testsuite:single_exec_riscv //c_testsuite:single_exec_x86_64 \
  //c_testsuite:single_exec_65c02 --test_output=summary --keep_going
```

Then compare `rg -n "^FAIL|pass=" <testlog>` for each.  Remember to rebuild
`//:davecc` after stashing or unstashing; the interpreters read
`bazel-bin/davecc`, so a stale binary silently measures the wrong compiler.

The failing names below have not changed since `28c946e`.  The pass counts are
current as of the spilled-variable-register fix; each C++ exec suite gained three
tests since `28c946e` (`0444`, `0445` and `0446`), so an older reading of this
table is low by that many there and otherwise identical:

```text
exec_aarch64        pass=409 fail=0
exec_arm            pass=406 fail=3   0158_standard_variant_constexpr (compile),
                                      0305_standard_expected (compile),
                                      0401_standard_stacktrace
exec_riscv          pass=407 fail=2   0276_standard_string_conversions,
                                      0327_standard_filesystem
exec_x86_64         pass=409 fail=0
single_exec_aarch64 pass=248 fail=3   00200, 00216, 00219
single_exec_arm     pass=247 fail=3   00200, 00216, 00219
single_exec_riscv   pass=243 fail=8   00078, 00186, 00187, 00189, 00200,
                                      00216, 00219, 00244
single_exec_x86_64  pass=242 fail=1   00200
single_exec_65c02   pass=234 fail=3   00200, 00219, 00246
```

A full `bazel test //... --keep_going` fails 13 more targets than the nine suites
above, all of them long-standing: they were verified failing at `4e13f01`, before
any of the work recorded here.  The list is worth having in full, because a run
that only knows about the first four names below looks alarming:

```text
//c_testsuite:warning_diagnostics
//cxx_testsuite:modules_{x86_64,aarch64,arm}
//cxx_testsuite:exec_stacktrace_65c02
//:6502_codegen_cleanup_test
//:6502_single_byte_copy_test
//:6502_tail_call_test
//:65c02_cxx_test
//:c23_numeric_headers_test
//:libc_x86_64_test
//:multiarch_inline_asm_test
//:mutex_65c02_test
```

`//:libc_arm_test` fails intermittently and belongs to neither list; see the note
with the ARM fixes below.  Several genrules (`//:hello_{aarch64,arm,x86_64}_exe`,
`//:hello_{aarch64,arm}_libfunc_so`) and `//cxx_testsuite:bazel_hello_module`
fail to *build* because they do not depend on `:libc_headers`, so `--keep_going`
is needed to get a complete run.

Note that a thread test cannot be used as the `-O2` regression for the three
register allocator and liveness fixes below: every `std::thread` program still
returned 137 under the ARM interpreter until the `.bss` fix landed, and a
thread-free reproducer for the AArch64 one was not found.

## Next in the queue

- Triage the remaining non-extension outcome failures from the runner into real
  defects.

- Chase the two remaining dynamic-linking defects the loader relocation fix made
  reachable, listed with that fix below.  The other two, a p-code fault on the
  first `malloc` of any dynamic program and an ARM `-fpic` fault on any reference
  to the program's own globals, are fixed.

- The two defects found while fixing the ARM one, a static `-fpic` link with no
  `.got` and `elfdump -r` misreading ELF32 relocations, are both fixed; see the
  entry with them below.

## Status

Fixed so far, each with a regression in this repository:

- Priority 1, deeply nested `else if`: commit `23a4558`.
- Priority 2, `_BitInt` static initializers: commit `ca447b9`.
- Priority 3, C++ crash after overload diagnostics: commit `c6cfaed`.  The crash
  was a redeclaration of a name introduced by a using-declaration, which has no
  type of its own.  That commit also makes `cxx_testsuite/run_compile_tests.sh`
  fail a `fail/` test whose compiler run does not terminate normally; it used to
  accept a crash as a pass once the expected diagnostics had been printed.
- The first post-diagnostic C hang cluster: commit `d350762`.  All five examples
  listed below now terminate.  Two loops lacked forward progress: the
  translation-unit loop re-parsed a declaration specifier that the parser
  rejected without consuming (`_Complex`), and the old-style argument
  declaration list only stopped at the `{` of the function body, so it spun at
  end of input.  The regression is `tests/c_error_recovery_test.sh`.
- `gcc.dg/c23-constexpr-1.c`, a use-after-free.  Reading a member of an
  aggregate constexpr object materializes the subobject for that member and
  stores it in the containing object's slot; the subobject was owned by the
  evaluation context even when the containing object was the long-lived value of
  a symbol, so the next read of that member followed a freed pointer.  The
  regression is in `tests/c_error_recovery_test.sh`.

- `gcc.dg/c99-vla-2.c`, a released type record still in use.  A conditional
  between two object pointers builds a composite pointer type in C; the pointee
  it had just chained onto that pointer was released a second time, so a later
  use of the type, such as `__typeof__` of the conditional, read a recycled type
  record.  The regression is `c_testsuite/tests/single-exec/00249.c`.

- `gcc.dg/gimplefe-28.c`, an aborted static initializer.  A static initializer
  was encoded from the type of the initializing expression rather than from the
  type of the object it initializes; after a diagnostic the two need not be in
  the same family, and a floating-point object initialized from an expression
  that had failed to parse reached an `assert(false)`.  The regression is in
  `tests/c_error_recovery_test.sh`.

- `gcc.dg/large-size-array-2.c` and `-4.c`, an unbounded array bound.  An array
  designator index is held in an `int`, and `[0x80000000]` was truncated into a
  negative one and then used as an unsigned element count, so deducing the
  bound of the array from it built elements until the machine gave up.  The
  index is now range-checked where it is parsed, and an index that would deduce
  an array too large to lay out is diagnosed before its elements are built.  The
  regressions are in `tests/c_error_recovery_test.sh`.

- `gcc.dg/cpp/tr-paste.c`, `gcc.dg/cpp/trad/paste.c` and
  `gcc.dg/cpp/trad/funlike-4.c`, an empty comment.  A comment closes with the
  two characters right after `/*`, but the preprocessor's comment scan advanced
  before looking at the first of them, so `/**/` sent it looking for a close
  that had already gone by; it then consumed the rest of the input and waited
  for an end of file that the lexer could not report while the line being
  tokenized was still unconsumed.  An unterminated comment hung in the same
  wait.  The regressions are `c_testsuite/tests/single-exec/00250.c` and a case
  in `tests/c_error_recovery_test.sh`.

- `gcc.c-torture/compile/limits-exprparen.c`, 10,000 nested parentheses.  Every
  operand that is itself an expression is parsed by recursive descent, and the
  parser cannot see how much stack is left, so nesting was only ever limited by
  the process running out of it.  Two changes: the eleven binary-precedence
  functions, each of which called the next tighter one and so cost eleven frames
  per level before an operand was even looked at, became one precedence-climbing
  loop; and the nesting depth is now counted and capped at 512, above both what
  the standards require (63 levels in C, 256 in C++) and what real programs
  contain.  Past the cap the expression is skipped in one bracket-balanced pass
  rather than left for each enclosing level to report and rescan, which is what
  the deep input costs today: one diagnostic, and time linear in its length.
  The regressions are in `tests/c_error_recovery_test.sh`.

  This does not make the test compile, since GCC accepts the nesting; it turns a
  crash into a diagnostic.  Accepting it needs the expression parser to stop
  holding a C frame per level, which is a much larger change.

- `gcc.c-torture/compile/limits-structnest.c`, 10,000 nested struct definitions.
  A member declaration can define another class, so member lists nest, and the
  depth was bounded only by the stack; the recursion cycle is five frames and
  about 5.5KB per level, so it faulted somewhere past 1,000.  Capped at 512 like
  the expression nesting above, with the over-deep member list skipped up to the
  brace that closes it.

## Remaining crashes and hangs

The crash sweep is exhausted: every C and C++ test in the GCC testsuite has been
run and one fails.

Two things were left out of the earlier sweeps and have since been added.  The
runner skipped fifteen directories whose feature davecc does not implement
(OpenMP, the sanitizers, LTO, vectorization, the analyzer, C++ modules, atomics,
decimal float, precompiled headers).  Their expectations say nothing here, but
the sources are still valid input that must not crash the compiler, so
`--include-unsupported-dirs` now runs them: 8,980 more tests, no crashes.  The
sparse checkout also only had `gcc.c-torture/compile`; adding `g++.old-deja`,
`gcc.c-torture/execute`, `gcc.misc-tests` and the remaining torture directories
contributed 5,198 more tests, also with no crashes.

```text
suite                    tests   crashes/hangs
g++.dg                  19,175              0
gcc.dg                  16,546              0
c-c++-common             6,520              0
g++.old-deja             3,195              0
gcc.c-torture/compile    1,999              1
gcc.c-torture/execute    1,914              0
gcc.misc-tests              81              0
gcc.c-torture/{compat,unsorted}  8           0
```

The one failure is `gcc.c-torture/compile/limits-externdecl.c`, which still times
out at 30s.  `limits-caselabels.c` is the same: it does not finish in 120s
either.  `limits-externalid.c`, `limits-fndefn.c` and `limits-blockid.c` do
finish, but take long enough to time out at 10s under a parallel sweep.

## What the outcome mode says

`--mode crashes` only asks whether the compiler survives.  `--mode outcome`
compares accept/reject against what GCC expects, and on the three suites where
every test is expected to compile (`gcc.c-torture/compile`, `.../execute`,
`gcc.misc-tests`) it reports 674 failures out of 3,994 -- each one a valid C
program davecc rejects.  Two harness defects accounted for 116 of them and are
fixed: `-fpermissive` was passed through to a compiler that does not accept it,
and a test that includes a sibling header or a helper below it had neither its
own directory nor its suite root on the include path.

What is left is dominated by unimplemented extensions rather than latent bugs.
The largest groups, with the count of tests each accounts for:

```text
112  type attributes, mostly `mode`
 75  a warning promoted to an error by the test's own options
 74  _Complex / _Imaginary
 72  label values: `&&label`, `goto *p`, `__label__`
 39  empty initializer `{}` before C23, which GCC accepts as an extension
 32  nested function definitions
 24  "Expression is not a compile-time constant"
 18  a VLA outside a function
 17  P-CODE asm constraints (an artifact of sweeping with -target pcode)
 10  "Illegal static initializer: need address of variable"
```

The `_Complex`, label-value, nested-function and empty-initializer groups are
each one missing feature behind many tests.  The 24 constant-expression failures
and the 10 static-initializer ones are the likeliest place to find real defects,
since neither names a feature davecc is missing.

### Which of those extensions Clang implements

Worth knowing before deciding what to support, because Clang's choices are a
better guide to what a C compiler is expected to accept than GCC's are.  Checked
against the host Clang, and the answers do not change between `-std=c17` and
`-std=gnu17`:

```text
accepted by Clang          &&label, goto *p, __label__, mode attribute,
                           _Complex, empty initializer {} before C23
rejected by Clang, as here nested function definitions, _Imaginary
```

Two of the groups are therefore not extensions at all from davecc's point of
view.  `_Complex` is required by C99, so those tests are a conformance gap rather
than a missing extension, and Clang's message for `_Imaginary` ("imaginary types
are not supported") says davecc's present stance already matches the state of the
art.  Nested functions are the one group worth leaving alone: Clang has never
implemented them, so rejecting them keeps company with Clang rather than
diverging from it.

### The `-I dir` spelling (fixed)

Both this harness and the clang one pass the value of `-I`, `-D` and `-U` as a
separate argument when a test's own options ask for it, which is the spelling
davecc did not understand: the driver forwarded the bare flag and let the value
fall through to the linker as an input file, and the compiler read the flag as
carrying an empty value.  The path was dropped with no diagnostic.  Both layers
now consume the following argument, and a flag left with no value is reported;
`tests/davecc_option_forms_test.sh` covers both spellings of each flag.

### Real defects found among the non-extension failures

Confirmed by reducing each to a few lines and comparing against Clang.

- **Fixed.** A variable array bound was compared and printed as a number even
  though the pointer to its bound expression shares storage with the fixed size.
  Declaring the same VLA parameter twice (`int f(int n, char m[1][n])`, which
  Clang accepts) was rejected as a redeclaration with a different type, and the
  diagnostic reported bounds like `char[1245987464]` that changed from run to
  run.  Since the template key string is built by the same printer, this was also
  a source of the nondeterministic output recorded above.
- A compound literal is a modifiable lvalue in C99, but `((struct A){0}).i += 1`
  is rejected with "Cannot assign to this expression"
  (`gcc.c-torture/compile/compound-literal-1.c`).
- **Fixed.** Address constants were not folded far enough.  `(char *)&x[18] - 8`
  and `&((&(v.p))->y)` were rejected as non-constant (`20001116-1.c`,
  `20010113-1.c`), which was most of the constant-expression and
  static-initializer groups above.  A symbol initializer now carries a byte
  addend, and an address expression is folded to (symbol, offset) through `&`,
  subscript, member selection, `*`, a pointer cast and pointer `+`/`-`.  Two
  further bugs surfaced while doing it, both fixed here:
  - The addend was applied twice on arm.  `R_ARM_ABS32` added the value already
    in the bytes being relocated on top of the addend in the relocation entry,
    which is what a `SHT_REL` object needs but wrong for the `SHT_RELA` objects
    davecc emits, so `&x[1]` came out as `x+8`.  It was latent only because
    nothing produced a nonzero addend before.  The two conventions are now
    distinguished by a flag on the relocation rather than guessed at, so a
    foreign `SHT_REL` object (`clang --target=armv7-*`, which is what
    `foreign_eh_object_arm_test` links) still gets its in-place addend.
  - `static int *p = &local[2];` in a function silently emitted a relocation
    against the *stack* object, and even defined a bogus symbol for it; Clang
    rejects all such initializers.  Only a symbol that has an address at link
    time is accepted now, so these are diagnosed.  The plain `static int *r =
    local;` form had the same silent wrong code and is fixed with it.
- `m[i] = m[i - 1] + b` on a `double **` draws "Array dimension required after
  first dimension", a message that belongs to declaration parsing, in a file that
  declares no multidimensional array at all (`20011219-1.c`).
- The check on `main`'s second parameter rejects an old-style definition whose
  parameters have no declared type, which `-std=gnu89` allows (`call.c`).

### Bugs found outside the sweep while working on the above

- `unsigned short << n` has the type of the unpromoted left operand.  C99 6.5.7
  makes the result the type of the *promoted* left operand, so it must be `int`.
  `c_testsuite` test 00200 fails on this and has been failing for some time; it is
  a wrong-code bug, not a diagnostic one.
- **Fixed.** A hosted C++ program did not start on any dynamic target.  The
  reported symptom was aarch64 rejecting an init-array entry with "Function array
  entry 0x800005250 is not executable", and 0x800005250 is exactly twice the
  correct 0x400002928: every address the loader relocated came out doubled.
  x86_64 and RISC-V failed identically, and ARM failed for a separate reason
  described below, so all four were broken.

  A `.rela.dyn` `R_*_RELATIVE` entry carries the whole link-time value in its
  addend, and the place contributes nothing.  The linker writes that value into
  the place as well, so that this loader and a native `ld.so`, which assigns
  `base + addend`, agree on the result; a check over the hosted binaries confirms
  addend and place are equal for all 77 entries on each of aarch64, x86_64 and
  RISC-V.  The loader was computing `base + place + addend` and so summing two
  copies of the same address.  Only `.init_array` failed loudly, because the
  lifecycle validates those entries before calling them; the 75 relocated `.data`
  pointers were equally wrong and would have failed later.

  Bisected to `4f4fcb0 Add x86_64 native dynamic runtime`, which is what
  populated the addend -- it previously wrote a literal `0`, which made the
  loader's `place + addend` correct by accident.  Since populating it is what a
  native `ld.so` needs, the fix is on the loader side: aarch64, x86_64, RISC-V and
  p-code now assign `base + addend` and ignore the place.

  ARM is the one target whose dynamic relocations are `SHT_REL`, where the linker
  drops the addend and the place is authoritative, so the doubling could not
  happen there.  Its bug was the mirror image: the handler translated the linked
  value to a runtime address, but the interpreter maps the image at a host
  address above 4GB and the slot is 32 bits wide, so the result was truncated.
  The interpreter already resolves a linked address on every guest access
  precisely because it maps wherever it likes, so the handler now leaves the
  linked value in place.  The `.init_array` entry then translates the way it
  always did for a static image, and no ARM guest slot has to hold a host
  address it cannot represent.

  This is what made `davecc_driver_defaults_test` fail, at that test's very first
  hosted link, so everything after it in the script had gone unexercised since
  `4f4fcb0`; it all passes now.  It was also the real cause of the
  `//:ir_optimizer_regression_test` failure, whose dynamically linked coroutine
  cases hit the same error on RISC-V.  The test's hosted coverage now spans
  RISC-V and x86_64 as well as aarch64 and ARM, since those two loaders were
  equally broken and nothing exercised them.

  Getting past the doubling exposed four further dynamic-linking defects, each
  previously masked and none of them a regression from this fix.  The p-code and
  ARM `-fpic` ones are the two entries after this; these two are open:
  - A data slot initialized to the address of a function imported from a DSO is
    left as 0 on aarch64, RISC-V and x86_64, so calling through it faults.
  - In a freestanding `-nostdlib` dynamic program on ARM and p-code the init
    arrays never run.
- **Fixed.** Any dynamically linked p-code program faulted on its first `malloc`,
  which made every p-code C++ program that allocates unusable.  This is what
  `//:ir_optimizer_regression_test` failed on once the relocation doubling above
  was out of the way, and it reproduces with a five-line C program that mallocs
  64 bytes.  It is not a regression: the dynamic path had never reserved the
  heap.

  p-code is excluded from the guest thread syscalls, so it does not get
  `__DAVECC_HAS_HEAP_LOCK__` and is the only target whose `malloc` still takes
  its heap from the linker-defined `_end` rather than from a 1MB `.bss` array.
  Nothing ever calls brk or mmap for that heap, so the loader has to map it, and
  only `LoadStaticSegments` did -- by enlarging the highest writable segment's
  memsz before mapping it.  The dynamic path maps sections one at a time, so
  there is no segment mapping to extend and the reserve was simply absent: the
  4MB above `_end` was unmapped, and the very first `RegisterRegion` writes the
  region's end sentinel one megabyte up, straight past the last mapped page.

  `LoadFixedAddressSections` now maps the same `LOADER_HEAP_RESERVE` itself,
  above the highest writable allocated section and only for the program, since
  a DSO's `_end` is not the one `malloc` uses.  The reserve is mapped after the
  sections, so it stops short of anything already mapped: a MAP_FIXED mapping
  over a section would replace it with zeroed pages.  Advancing the next free
  address past the reserve keeps the libraries loaded afterwards out of the heap.
- **Fixed.** Any `-fpic` ARM program faulted as soon as it named one of its own
  globals, and eager PLT binding (`LD_BIND_NOW`) never worked on ARM at all.
  Neither is a regression; both were reachable before any of this work.

  The interpreter was handing the guest a *host* PC.  Reading PC is how ARM code
  names an address -- it adds a link-time displacement to it, as `ldr rX,
  [pc, #n]` for a literal and `add rX, pc, rX` for the GOT in
  position-independent code -- and for an ignore_vaddr target that sum is wrong
  twice over.  The loader maps each segment with its own `mmap`, so the segments
  do not sit their link-time distance apart and a displacement that leaves
  `.text` lands nowhere; and the host addresses are above 4GB, so the sum does
  not survive being narrowed to a 32-bit register.  Both showed up in one
  three-instruction sequence, the first as an unmapped GOT address and the
  second, once that was resolved, as a `.rodata` address with its top byte gone.

  `ReadReg` now hands out the linked PC.  Every link-time displacement is then
  exact, no 32-bit register has to hold a host address, and the memory and
  branch paths already translate a linked address when it is used -- the branch
  helpers accept either, and the link register was *already* being stored as a
  linked address, so PC was the one register that disagreed.  Three things
  computed in the wrong space fell out of that and are fixed with it: the PLT's
  own displacements (`FixupResolverPLTEntry`, `FixupPLTTrampoline`) were runtime
  differences and are now link-time ones, which is the same value on a target
  that does keep its layout; the lazy resolver identified the invoked GOT slot by
  comparing against a translated relocation offset and now compares the offset
  itself; and `R_ARM_GLOB_DAT` stored a truncated runtime address where, like
  `R_ARM_RELATIVE`, it has to leave the linked one.

  `R_ARM_JUMP_SLOT` had the same truncation on the eager path, which is why
  `LD_BIND_NOW` died in `Fetch32` on a host address with its top byte cut off
  while lazy binding was fine.  It now binds to the linked address, the value the
  lazy resolver was already storing.  `//:dynamic_interpreters_test` runs each
  target both ways so the eager path stops being untested, and
  `//:davecc_driver_defaults_test` gained an ARM `-fpic` program that reads its
  own global through the GOT.

  Two related defects were found with it, both verified failing at `4ce2088~1`
  and now fixed; they are the entries immediately below.
- **Fixed.** `elfdump -r` printed every relocation in an ELF32 file as
  `R_ARM_NONE` with offset 0 and no symbol name, in objects as well as
  executables.  This is worth recording ahead of the linker defect below because
  it is what made that one hard to see: while diagnosing an ARM relocation bug
  the tool reports that the file has no relocations in it.

  Two things were wrong, both from `PrintRelocations` never being updated when
  ELF32 support landed.  It took its file base from `elf->header`, which is only
  the start of the mapping for ELF64 -- for ELF32 the header is decoded into an
  owned wide structure, so every file offset was applied to an unrelated heap
  allocation.  And it cast the bytes at that offset straight to the canonical
  (ELF64) `ELFRelocation` and `ELFSymbol`, which are wider than their on-disk
  ELF32 forms.  The linker's own `ReadRelocations` is the same loop written
  correctly, so this now does what that does: offsets from `elf->base`, and
  `elf->ops->ReadRelocation` / `ReadSymbol` to decode when the file is ELF32.
  The ARM relocation names were also filled in, since the table stopped at the
  seven types an object file uses and left every dynamic one as "unknown".

  `//:elfdump_relocations_test` dumps an ARM object and an ARM executable, which
  cover `SHT_RELA` and the narrower addend-less `SHT_REL`, and an aarch64 object
  to hold the shared ELF64 path.
- **Fixed.** A `-fpic -static` link was broken on every target, not just ARM:
  aarch64, RISC-V and p-code crashed the linker outright, while ARM and x86_64
  silently produced an executable that faulted on its first global.

  Position-independent code reads a variable's address out of the GOT.  Nothing
  built a GOT for a static link -- `DynamicLinkerGatherDynamicRelocations` was
  skipped, so no slot was ever allocated, no `.got` section was created, and the
  displacement the code adds to the PC to reach its slot resolved to 0.  The
  crashes are the same absence reached from the other direction: the relocation
  code for a call through the PLT dereferenced `plt_group` without checking it,
  and a static link has no PLT.

  A static link now collects GOT entries like any other, creates just the GOT
  (none of `.dynsym`, `.dynamic`, `.rel.dyn`, `.plt` or `.interp` mean anything
  without a loader), and writes each slot's final value itself, since there is no
  loader to relocate them.  It is limited to the variable entries, which are the
  only ones position-independent code asks for here: the PLT appenders are
  replaced by ones that allocate nothing, so a call resolves straight to its
  target, which is what `R_ARM_PLT32` and `R_X86_64_PLT32` already did and what
  `R_AARCH64_CALL_PLT`, `R_RISCV_CALL_PLT` and `R_PCODE_CALL_PLT` now do instead
  of faulting.  `_GLOBAL_OFFSET_TABLE_` points at `.got` rather than the empty
  `.got.plt`, and is invented at that point rather than up front, so a static
  link that asked for no GOT entries is left exactly as it was -- no table, no
  symbol.  `//:static_pic_test` links and runs a C and a hosted C++ program
  `-fpic -static` on all five targets, and the same C program `-static` alone to
  hold that containment.
- **Fixed.** Throwing an exception cost time proportional to the size of the whole
  `.eh_frame`, so exceptions were around a thousand times more expensive than the
  work they interrupt.  `FindFDEInRange` in `libc/eh_frame.c` walks and fully
  parses every entry in the table on every frame lookup, and does not stop at the
  first match: it keeps scanning so that it can pick a "best" candidate among all
  the FDEs that contain the pc.  A lookup was therefore O(entries) and a throw
  O(frames x entries), with no index or cache in front of it.

  Measured on x86_64, where the interpreter runs about 5M guest instructions a
  second, one throw/catch takes 1.8s -- roughly 9M instructions to unwind two
  frames.  Both factors are visible directly.  Adding 800 functions that are
  never called grows `.eh_frame` from 21KB to 79KB and takes a single throw from
  1.93s to 6.53s, about 80us per byte of table per throw; in the 21KB binary
  about 88% of a throw is spent on FDEs for functions unrelated to it.
  Independently, throwing through 1, 4 and 16 frames costs 2.59s, 4.51s and
  12.69s.

  This is what made `cxx_testsuite:exec_x86_64` fail.  Its two failing tests,
  `0115_standard_vector_exception_safety.cpp` and
  `0124_standard_vector_range_exception_safety.cpp`, do four throws each from
  inside `std::vector`, which was 36s against the harness's 30s per-test limit in
  `cxx_testsuite/run_exec_tests.sh`.  They were never stuck: truncating `main` to
  run 0 to 4 of the four subtests gave 0.1s, 10.8s, 17.0s, 25.7s and 34.9s.
  Raising bazel's `--test_timeout` does not help either, because the limit that
  kills them is the harness script's own `TIMEOUT=30`.

  The fix indexes the table instead of scanning it.  The program's own
  `.eh_frame` is delimited by linker symbols and so never changes, which lets an
  array of (pc_begin, pc_end, entry) sorted by pc_begin be built once, on the
  first lookup, and never invalidated; module and foreign ranges come and go
  through `__register_frame` and hold few entries each, so those keep walking.
  Tracking the widest span in the index bounds how far below the pc a covering
  entry can start, so a search over the usual non-overlapping table costs one or
  two probes, and the existing "best candidate" tie-break still runs over
  whatever entries do overlap.  If the allocation fails the old scan is used, so
  the table is never left unsearchable.

  One throw now costs 0.34s rather than 1.97s, and eight cost 0.76s rather than
  14.30s -- a marginal cost of about 50ms a throw instead of 1.8s.  Unwinding 16
  frames went from 12.69s to 0.77s, and the 800 never-called functions that used
  to add 4.6s to a single throw now add 0.47s, paid once when the index is built.
  `exec_x86_64` passes, and the suite as a whole runs in 160s rather than 475s.
- **Fixed.** Catching an exception corrupted the *caller's* callee-saved
  registers, on x86_64 and on riscv, for two unrelated reasons.  A caller that
  keeps a value in such a register across a call sees it change, so this is a
  wrong-code bug in any program that catches, not just in the coroutine tests
  that exposed it.  `cxx_testsuite/tests/exec/0436_exceptions_preserve_callee_saved.cpp`
  covers both.
  - On x86_64 the landing pad ran with the stack pointer the throwing call site
    left it at.  The unwinder hands a frame back at the stack pointer its CFI
    describes, which is the value at the call instruction -- and on x86_64 the
    outgoing arguments have been pushed by then, so it is below the bottom of the
    frame.  The rest of the function assumes the stack pointer is at the bottom:
    the exit sequence reads the callee-saved registers at fixed offsets from it,
    so it handed the caller the outgoing arguments of the call that threw.
    `throw 8` inside a coroutine left the caller's `%rbx` holding `&_ZTIi`, the
    second argument of `__cxa_throw`.  The landing pad now puts the stack pointer
    back from the frame pointer, which is the one register the unwinder does
    restore.  aarch64, arm and riscv already did exactly this, so x86_64 was the
    only target missing it; the new code follows theirs.

    This is what made `cxx_testsuite:exec_coroutines_x86_64` fail
    (`0007_cxx23_range_for_lifetime.cpp`).  It is not coroutine-specific: a
    coroutine's resume function is simply big enough that the optimizer parks a
    constant in a callee-saved register across the resume, and the test compares
    against that constant on both sides.
  - On riscv the CFI did not say where the callee-saved registers had been
    spilled.  The prologue stores them, but the FDE only described `s0` and `ra`,
    so unwinding through a frame could not recover them and the unwinder passed
    the throwing code's values through to the handler, whose transfer stub then
    installed them over the caller's.  It was masked for any register the
    handler's own function also saves, because its exit sequence reloads those
    from its frame; a register only it uses -- `s4` here, holding the caller's
    third live value -- came out of `__cxa_throw` as 40 instead of 33.  The
    backend now emits the `DW_CFA_offset` rules, as the x86_64 backend already
    did.

    aarch64 and arm have the same missing rules.  No case where it is observable
    was found there (up to twelve live values across a catching call), because
    their landing pads reload every saved register from the frame, so they are
    left alone rather than changed on the strength of an argument.  That reload
    turned out to be a bug of its own; see the next entry, which is what makes
    the aarch64 rules observable and adds them.
- **Fixed.** A landing pad on aarch64, arm and riscv began by reloading the
  callee-saved registers from its own frame.  Those slots hold the values the
  *caller* passed in, stored by the prologue, whereas a handler needs the values
  the function itself had at the call that threw -- so the reload destroyed
  exactly the registers it was meant to recover, and it destroyed them at every
  catch, not just an unusual one.  The unwinder already delivers the right
  values, recovered from the CFI of the frames it pops (aarch64, riscv) or from
  its virtual register set (arm), so the landing pads now only put the stack
  pointer back and reload the hidden result pointer, which is genuinely their
  own.  aarch64 also needed the `DW_CFA_offset` rules from the previous entry,
  without which the unwinder had nothing to recover the registers from.

  The visible damage depended on what was in the register.  A coroutine's resume
  function keeps the coroutine frame pointer in one across the whole body, so
  after the frame's `catch` ran `unhandled_exception`, the following
  `frame->state = 0` store went through the *caller's* value instead: the
  coroutine handle in `main` was overwritten, and `handle.destroy()` then called
  through a function pointer read out of `main`'s stack.  This is what made
  `0006_lifetime_matrix.cpp` and `0007_cxx23_range_for_lifetime.cpp` fail on all
  three targets -- riscv died on the wild call, aarch64 and arm on the load
  through the wrecked handle.  `cxx_testsuite/tests/exec/0437_catch_keeps_own_registers.cpp`
  covers it without coroutines: eight values and a pointer live across a
  `try`/`catch`, with a store through the pointer afterwards.  Before the fix
  aarch64 read back the wrong sum, arm the wrong caught value, and riscv
  segfaulted.

  Floating-point registers are left alone now too, though nothing recovers them:
  the unwind context has room for 64 integer registers only, so the aarch64/arm
  capture and install stubs do not carry `d8`-`d15` at all.  Leaving them is
  still an improvement, since a value untouched by the unwind path now survives
  where before it was always replaced by the caller's.
- Fixed: a local written in a `catch` block read back as its value before the
  `try`, on every target, at `-O2` but not at `-O0`.  Found while writing the
  test for the entry above:

  ```c++
  int caught = 0;
  try { Thrower(9); } catch (int value) { caught = value; }
  return caught;    // 0
  ```

  A landing pad is entered from the unwinder rather than from a branch, so no
  block listed it as a successor and it had no predecessor in the CFG.  With no
  predecessor it never got an immediate dominator, which kept it out of the
  dominator tree; SSA construction walks that tree, so the store to `caught` was
  never given a name, contributed nothing to a dominance frontier, and no phi
  appeared where the handler rejoins the code after the `try`.  The read there
  then resolved to the only definition that reached it, the pre-`try` one, and
  constant propagation folded it.

  `AddExceptionHandlerEdges` in `c_compiler/backend/codegen.c` now links the
  block holding each protected region's start label to that region's pad.
  Starting the edge at the region's start, rather than at each call inside it,
  keeps definitions made before the region dominating the pad -- handlers and
  cleanups only name objects declared outside the region, so they need those --
  while definitions inside it correctly do not, since the throw may precede
  them.  Any call that can throw ends its block, so a definition sharing the
  start block always precedes the region's first throw point and dominating the
  pad is accurate for it.  The edges are added after `StraightenGraph`, whose
  coalescing assumes a block that does not end in a branch has exactly one
  successor.  `cxx_testsuite/tests/exec/0438_catch_local_reaches_join.cpp`
  covers the store in the handler, a value defined before the `try` and left
  alone by the handler, both paths defining the local, and a handler reading the
  local it overwrites.
- Fixed: inlining a call that relied on a default argument wrote outside the
  call's argument array, corrupting whatever heap block happened to follow.  It
  showed up as the compiler segfaulting on
  `cxx_testsuite/tests/exec/0297_standard_thread.cpp` at `-O2`, in a pass over
  scope-exit destructors that found a NULL statement in a compound belonging to
  a completely unrelated function.

  A call node keeps the callee in `left` and the arguments in `children`, and an
  argument's stored `child_id` is its index in `children`.  Visitors number the
  same children one higher so that the callee can be 0, and
  `AppendDefaultCallArguments` was storing the visitor's number.  Inlining moves
  each actual out of the call by that stored id, so moving a default argument
  cleared the argument *after* it, and moving the last one -- the usual case,
  since defaults are trailing -- wrote a NULL one slot past the end of the
  array.  Which allocation that landed in decided whether anything went wrong,
  which is why a large translation unit failed while the same code in isolation
  did not.

  `VectorASTNodeReplaceChild` in `c_compiler/frontend/syntax/ast.c` now asserts
  the index is in range, so a future mismatch fails there instead of silently
  scribbling.  `cxx_testsuite/tests/exec/0439_inlined_default_arguments.cpp`
  covers inlined calls with one and two trailing defaults, supplied and omitted,
  as a free function and as a member.
- Fixed: a function returning a pointer to member function by value returned
  garbage on every target.  A pointer to member function is a pair of words, and
  the backend already knew to pass one by address, but on the way out it was
  classified as a scalar: the callee left the caller's destination untouched and
  put the address of its own copy in the result register, so the caller called
  through uninitialized stack.  It surfaced as
  `cxx_testsuite/tests/exec/0297_standard_thread.cpp` faulting at `-O2`, where
  `std::thread` passes a member function through `__decay_copy`.

  `TypeReturnedThroughHiddenPointer` in `c_compiler/backend/codegen.h` now names
  the classification -- a class, a union, or a member-pointer pair -- and the
  places that decide whether a result travels through `IR_OP(structreturn)` ask
  it instead of `TypeIsStructOrUnion`: allocating the hidden pointer, the return
  statement, ordinary and inlined calls, and each target's argument-slot
  assignment.

  On RISC-V that exposed a second bug in the frame layout of a leaf function.
  Local variables and the argument-home area are placed below a fixed 16-byte
  header, but a leaf has no return address to save, and the prologue reclaimed
  the freed word by moving the callee-saved register area up eight bytes.  The
  freed word is at the top of the header, not the bottom, so the first saved
  register landed on the lowest local -- harmless while a leaf had no homed
  arguments, but a leaf returning a pair through a hidden pointer homes two of
  them, and the saved register overwrote the first word of the result.

  `cxx_testsuite/tests/exec/0440_member_function_pointer_return.cpp` covers a
  pointer to member function returned from a reference parameter, from a value
  parameter, from an inlined function, bound to a reference, from a member
  function, const-qualified, and a pointer to data member alongside, which is
  one word and still comes back in a register.
- Fixed: reading a member back out of a local that was initialized to a constant
  produced a load from the constant at `-O2`.  `receiver zero_initialized{0}; if
  (zero_initialized.value != 0)` loaded from address zero, which asserted in the
  ARM, AArch64 and RISC-V emitters (a load whose base operand never got a
  register) and quietly read the wrong stack slot on x86-64.  It is what
  `cxx_testsuite/tests/exec/0297_standard_thread.cpp` hit at `-O2` once the
  member-pointer return was fixed.

  The SSA name of a variable stands for its storage.  The conditional constant
  propagator gives that name the value a whole-variable store put there, which is
  what a load straight through the name reads, and `EvaluateIntegerExpression`
  then treated the same name as a plain number when it appeared in an expression.
  Reaching a member builds `adda(ssavar, offset)`, which consumes the storage, so
  folding the contents in produced the initializer plus the offset as an address.
  An expression with a variable node as an operand is now overdefined.

  Only locals whose address is never taken are affected, since
  `IsSafeSSASymbol` already declines to track the rest, which is why a zero
  initializer -- address zero -- was the visible case.
  `cxx_testsuite/tests/exec/0441_member_of_initialized_local.cpp` covers a
  single-member class, a member mutated after initialization, a member past the
  first, and a nested class.

- Fixed: three defects in the `-O2` inliner, which `std::invoke` walks straight
  into and which together kept
  `cxx_testsuite/tests/exec/0297_standard_thread.cpp` from reaching its member
  function at `-O2`.  `cxx_testsuite/tests/exec/0442_inlined_forwarding_wrapper.cpp`
  covers all three, and each one alone makes it fail.

  A wrapper forwarding to a void callee is written `return callee(...);`.  The
  inliner replaced a return statement with a jump to the epilogue label and kept
  its operand only when there was a result temporary to store it in -- correct
  for a value, but for a void call the operand was the whole point of the
  statement, and it was discarded.

  A reference parameter bound to a prvalue argument gets a materialized
  temporary, and the reference holds that temporary's address.  Nothing marked
  the temporary address-taken, so a scalar one could be register-allocated;
  reading through the reference then loaded from the value instead of from the
  object.  Materializing it also released the only reference to the copied type
  record, which tore down the declarator spine and left a pointer temporary
  typed `<invalid>*`.

  A pointer-to-member-function call lowers to a dereference of the loaded
  pointer, typed with the member function's own type record -- which carries
  that member's body.  Inlining decided from the callee's type alone, so every
  such call was inlined as a direct call to whichever member the type record
  came from.  A call may now only be inlined when its callee names the function.

- Fixed: two RISC-V register allocator defects that between them left
  `cxx_testsuite/tests/exec/0297_standard_thread.cpp` faulting at `-O2`, and are
  covered by `cxx_testsuite/tests/exec/0443_arguments_live_across_calls.cpp`
  (both fixes are needed to make it pass).

  A value that has to survive a call needs a callee-saved register, and when
  none was free the allocator evicted whichever value was cheapest to spill,
  wherever it sat, then took that register.  A victim holding a caller-saved
  temp handed over a register the call clobbers.  The victim search now honours
  the same constraint the allocation does, and falls back to a register variable
  when nothing else in the callee-saved range can be freed -- which a non-leaf
  function needs, since register variables reserve all of `s2`..`s11` and only
  `s1` is left to hand out.

  A call's arguments are placed by a parallel copy, ordered so that no move
  overwrites a register a later one still reads.  A move whose source had been
  spilled becomes a reload, which is not a register-to-register move at all, and
  it kept the tag marking it part of the copy.  The resolver read it as a
  malformed member and left the entire run in its original order, where
  `mv a1,t1` preceded the `mv a0,a1` that still needed the old `a1`.

- Fixed: a hole in the backend's block live-in sets that left two live values
  sharing one register, which faulted
  `cxx_testsuite/tests/exec/0297_standard_thread.cpp` at `-O2` on AArch64 with
  `Store8 outside mapped memory`.  A test that reproduces it without threads has
  not been found; the shape needs three values interacting, and the two thread
  tests that do reproduce it hang on ARM at every optimization level.

  Live-in sets are built by walking the dominator tree, so a value is recorded
  as live only in the blocks that dominate its uses.  A join block's live-in
  values are also live along its other incoming paths, and those paths need not
  dominate it: the sibling arm of an `if` is the common case.  The register
  allocator reserves a register only where a block records the value as live, so
  the gap in the middle of the range let an unrelated value take the same
  register.  The two then travelled together until one of them was spilled,
  which released the register while the other was still live in it.

- Fixed: an ARM program that reached any `__cxa_atexit` registration spun
  forever in `__davecc_atexit_lock`, which took out every `std::thread` and
  `std::error_code` program on that target -- `std::make_error_code` alone was
  enough.  The lock byte is the first object in `.bss`, and the writable
  `PT_LOAD` claimed four file bytes more than the initialized sections hold, so
  the loader copied the padding that follows `.init_array` over the lock and it
  read as held.  The extra bytes came from stretching the load segment to back
  the `PT_TLS` file image, which a block built only from `.tbss` does not have.
  The ARM cxx suite lost five failures.

  A `PT_TLS` image that is not empty was still mapped at an address inside
  `.bss`; that is the same defect from the other side and is the entry below.

- Fixed: an atomic operation whose returned value nobody reads was deleted at
  `-O2` on ARM, x86-64 and RISC-V, while the memory it was supposed to update
  kept its old value.  The target-instruction dead-code pass decides what to
  keep from an instruction's register result alone, and an atomic has an effect
  that its result does not describe.  AArch64 was the only backend that already
  asked separately, through `AARCH64HasImplicitEffect`.
  `cxx_testsuite/tests/exec/0444_atomics_survive_unused_result.cpp` covers all
  of it, and it fails on each of the three targets before the fix and passes on
  AArch64.

  ARM lost the store.  `ARMIsExpression` did not list `ARM_OP(atomic_store)` or
  `ARM_OP(atomic_fence)` next to the plain stores, so a store -- which has no
  register result at all -- looked like an expression nobody read.
  `value.store(...)` compiled to a prologue and an epilogue with nothing in
  between.  This is what remained of
  `cxx_testsuite/tests/exec/0297_standard_thread.cpp` returning 5 on ARM at
  `-O2`, and it needs no threads to show:

```cpp
#include <atomic>

int main() {
  std::atomic<unsigned long> value{0};
  value.store(0x12abcdefUL);
  return value.load() == 0x12abcdefUL ? 0 : 5;
}
```

  Note `unsigned long` is 32 bits on ARM, so keep any literal inside 32 bits or
  the comparison fails for its own reasons.  RISC-V already listed both opcodes
  and x86-64 lowers an atomic store to a plain store, so neither needed this
  half.

  ARM, x86-64 and RISC-V all lost the read-modify-writes -- `fetch_add`,
  `fetch_sub`, `add_fetch`, `sub_fetch` and the three compare-exchange forms.
  Those cannot be fixed the same way, because they do produce a value and are
  genuinely expressions when someone reads it; the pass has to ask separately
  whether the instruction has an effect beyond its result.  Each of the three
  backends now has a `HasImplicitEffect` predicate listing those opcodes,
  consulted in `RemoveBlockUnusedExpressions` alongside the existing
  `observable_checkpoint` check, which is the shape AArch64 already had.

  Three barriers were being dropped along with the instruction that carried
  them, and the predicate now keeps all three.  x86-64 carries
  `atomic_thread_fence` on a `nop` flagged `X86_64_MFENCE`, so a seq_cst fence
  emitted no `mfence` at all at `-O2`.  On ARM and RISC-V the `atomic_load`
  pseudo emits its own `dmb ish` / `fence` around the load, so discarding the
  loaded value discarded the ordering with it; AArch64 already kept
  `atomic_load`, and needs no separate barrier because it uses `ldar`.  x86-64
  is the one target where an atomic load cannot be protected -- it lowers to an
  ordinary load, indistinguishable from any other -- and is also the one that
  needs no barrier around it.

  None of the three barriers has a check in the regression, because a missing
  fence is not observable under a single-threaded interpreter.  They were
  confirmed by counting `mfence`, `dmb ish` and `fence` in `-S` output for a
  program whose only atomic is one whose result is discarded.

- Fixed: loading a program wrote its TLS initialization image over the first
  bytes of `.bss`, on all four targets, at `-O0` as much as `-O2`.  A
  `thread_local` with an initializer plus a `.bss` array was enough, and the
  array came up holding the image.
  `cxx_testsuite/tests/exec/0445_tls_image_clear_of_bss.cpp` covers it and
  returns 6 on x86-64, AArch64, ARM and RISC-V before the fix.

  A TLS symbol's value is its offset within the thread's block, which is what a
  local-exec relocation needs, so the TLS sections cannot also carry a load
  address and `AssignSegmentSectionAddresses` skips them when it hands addresses
  out.  Their file content is still written immediately after the initialized
  data, and `WriteProgramHeaders` stretches the writable `PT_LOAD` over it so a
  native loader can reach the image, pointing `PT_TLS` at
  `data vaddr + (tls offset - data offset)`.  That address was never reserved,
  and `.bss` began exactly there, so the loader's copy landed on it.

  The linker now advances the data segment's end past the image before `.bss` is
  placed, in `LinkerTLSImageSpan`.  Reserving the image's size alone is not
  enough: the writer aligns each section's file offset by that section's own
  alignment, so an image needing 8-byte alignment behind an odd-sized `.data`
  gets file padding in front of it, and the reservation has to cover the padding
  too or the image's tail still overhangs `.bss`.  The span is computed by
  walking the TLS sections and aligning as the writer will, which is exact
  because a load segment's address and file offset are given the same residue
  modulo the segment alignment -- so the same walk over addresses reproduces the
  file layout.

  Only file-backed sections count, which is what keeps this off the `.tbss`-only
  case fixed in `28c946e`: that block has no image, `PT_TLS` has `filesz` 0, the
  writer leaves the load segment's file size alone, and reserving room would
  reintroduce exactly the ARM `__davecc_atexit_lock` hang that commit removed.
  A DSO is skipped for the same reason -- only an executable relocates `PT_TLS`
  into the writable segment.

  Worth knowing for the next one: the runtime symptom is a poor detector here.
  The original reproducer passed on ARM purely because of which `.bss` object
  the image happened to land on, and an intermediate version of this fix left a
  4-byte overhang on ARM that no test noticed.  What found both was comparing
  `elfdump -l` against `elfdump -S`: the writable `PT_LOAD`'s
  `vaddr + filesz` must not reach past the start of `.bss`, and `PT_TLS`'s
  `vaddr` must equal what its file offset maps to under that segment.  Checking
  those two directly flagged 12 of 16 target/shape combinations before the fix
  and none after.

- Fixed: a chained `||` read back a value that had just been written as though
  the write had not happened, on ARM at `-O2`.  Every other target passed, `-O0`
  passed, and splitting the condition into separate `if`s passed on ARM too, so
  the comparisons were fine and the chain was where it went wrong.
  `cxx_testsuite/tests/exec/0446_spilled_var_reg_keeps_no_register.cpp` covers
  it and returns 7 on ARM at `-O2` before the fix.

  `InitializeBasicBlockRegisters` in `c_compiler/arm/arm_reg_alloc.c` reclaims a
  promoted variable's register at every block entry, because the approximate
  live-in sets can omit a variable that a loop back edge needs again.  It did so
  for a variable that had since been spilled as well.  A spilled value lives in
  its slot and every use reloads into a fresh register, so it has no claim on the
  register it held before the spill -- `inst->reg` merely still names it, since
  `SpillInstruction` clears the ownership but not the field.

  The squatting is not harmless, and the failure needs all three steps.  The
  live-in loop right below refuses to reserve a register another value already
  owns, so the value genuinely holding it across the block silently lost its
  reservation.  The spill-victim search then found a register owned by a spilled
  instruction and cleared that stale ownership, as it is entitled to.  The
  register now looked free, so the next value needing one took it while the real
  owner was still live in it.  One line fixes it: skip a spilled variable in the
  reclaim loop, which is the check the live-in loop already makes.

  In the reproducer the spilled variable is the pointer walking a `.bss` array
  and the value it displaced is the constant 3, materialized once for `g_zero = 3`
  and reused as the third term's comparison operand two blocks later.  The middle
  term's block took the register, so the chain compared `g_zero` against a
  boolean.

  It resisted reduction, which is worth knowing for the next one of these:
  removing the loop, the reads before the writes, or one term of the chain all
  make it pass, and none of those feeds the chain.  They only change which value
  lands in the squatted register.  Reading the `-O2` assembly is what actually
  found it -- `cmp r8, r5` where the working version had `cmp r7, #3` -- and
  tracing `r5` backwards through the block order.  The other three backends do
  not have this defect: x86-64 and RISC-V mark variable registers `reserved` for
  the whole function so they never enter the dynamic pool, and AArch64 has no
  block-entry reclaim loop at all.

## Deep nesting outside the constructs already capped

The suites cover parenthesized expressions and struct nesting, and both are now
bounded, but the same unbounded recursion is reachable from many other
constructs.  None is covered by a test in the sweep; all were found by probing
with 20,000 levels, and each faults:

```text
{{{ ... }}}                 nested compound statements
while(x)while(x)...         nested loop bodies (same for `for`)
f(f(f( ... )))              nested call arguments
a[a[a[ ... ]]]              nested subscripts
(int)(int) ... 0            nested casts
!!! ... 0                   nested unary operators (same for `*`, `sizeof`)
namespace a { namespace ... nested namespaces (C++)
S<S<S< ... >>>              nested template-ids (C++; hangs rather than faults)
```

The expression cap counts levels at `ParsePrimaryExpression`, which a chain of
prefix operators or casts never reaches until its end, so those slip past it;
statements, namespaces and template-ids are not counted at all.  Bounding these
wants one shared parser-depth counter checked at each recursive entry point
rather than a counter per construct.

Weaknesses noticed while fixing the above and deliberately left alone, since
none is a crash or a hang and no test in the sweep covers them:

- An explicit array bound whose byte size overflows the `int` that holds it is
  accepted silently (`static char *a[0x80000000];`).
- A range designator (`[0 ... 0x7ffffff0]`) builds one initializer per index.
- An unterminated comment is accepted without a diagnostic; GCC and Clang both
  reject it.
- A function-like macro is not expanded when a comment or a newline separates
  its name from the `(`, so `f /**/ (0)` is left as a call to `f`.  This is what
  `funlike-4.c` tests once it no longer hangs.
- A long flat operator chain crashes even though nothing about it is nested:
  `1+1+1...` with 100,000 terms builds a left-leaning tree that
  `BinaryASTNodeVisit` then walks recursively.  Capping expression *nesting* does
  not help here, because the parser builds this tree without recursing; the tree
  walkers are what need bounding.
- Compiling the same source twice does not always produce the same output.  23
  of the 648 sources in `cxx_testsuite/tests/exec` and
  `c_testsuite/tests/single-exec` hash differently from one run to the next,
  which makes byte comparison of the output useless for exactly those files.
- `-I dir` written as two arguments is ignored without a diagnostic; only the
  joined `-Idir` is honored.  GCC and Clang accept both, and silently dropping an
  include path turns into a confusing failure to open a header far away from the
  command line that caused it.

Unrelated failures seen while validating, each reproducible with a compiler
built before this work and so not caused by it:

- `//:c23_bitint_test` used to fail: an `unsigned _BitInt(5)` bitfield read gave
  the wrong value at `-O2` but not at `-O0`.  It passes as of `28c946e`, so one
  of the intervening `-O2` fixes covered it.
- `//c_testsuite:single_exec_x86_64` fails `00200.c`.
- `//c_testsuite:warning_diagnostics` fails: the script runs davecc with
  `-Werror=unused-value` under `set -e` and expects it to succeed, but promoting
  a warning to an error makes davecc exit nonzero.
- `//:libc_x86_64_test` and `//:c23_numeric_headers_test` fail.
- `//cxx_testsuite:exec_x86_64` fails three tests whose guest programs exceed
  the harness's 30 second run timeout on this machine (35 s, 37 s and 75 s);
  all three exit 0 when run without it.
- `//:libc_arm_test` is flaky, and only under the parallel load of a full
  `bazel test //...`; on its own it does not fail.  Measured while validating the
  spilled-variable-register fix: it failed one full run (`thread_condition`),
  passed the next, and passed 6 of 6 runs on its own with that fix and 10 of 10
  with it stashed.  Treat a single failure there as noise, but confirm it the
  same way rather than assuming -- a change to the ARM backend is exactly the
  kind that could make it real.  Its thread cases (mutex, condition, addr_wait)
  are the source.

A full `bazel test //... --keep_going` is worth running for a change that
touches a backend, since the nine suites above do not cover the 6502, the
modules tests or the libc tests.  It runs 122 targets and the two sets barely
overlap: the five `//c_testsuite:single_exec_*` targets are tagged `manual`, so
`//...` does not include them at all, and must be asked for by name.

A full run leaves these 15 failing, and a change that adds no name to this list
has not regressed anything the repository can detect:

```text
//:6502_codegen_cleanup_test      //:multiarch_inline_asm_test
//:6502_single_byte_copy_test     //:mutex_65c02_test
//:6502_tail_call_test            //c_testsuite:warning_diagnostics
//:65c02_cxx_test                 //cxx_testsuite:exec_arm
//:c23_numeric_headers_test       //cxx_testsuite:exec_riscv
//:libc_x86_64_test               //cxx_testsuite:exec_stacktrace_65c02
                                  //cxx_testsuite:modules_aarch64
                                  //cxx_testsuite:modules_arm
                                  //cxx_testsuite:modules_x86_64
```

`//:libc_arm_test` shows up on top of those in some runs; it is the flaky one
described above.  The list was 17 names from `28c946e` through the `PT_TLS` fix:
`//:c23_bitint_test` passes as of `28c946e`, `//:davecc_driver_defaults_test` as
of the loader relocation fix, and `//:ir_optimizer_regression_test` as of the
p-code heap reserve fix.

## Priority 1: deeply nested `else if` stack overflow

Source:

```text
/Users/FZDSZZ/gcc-test/gcc/testsuite/gcc.dg/20020425-1.c
```

The test expands macros into 11,000 consecutive `else if (0) { }` clauses.
davecc exits with `SIGSEGV` (`rc=-11`) without producing a diagnostic. GCC
describes this as a parser-stack-overflow regression.

Reproduce safely:

```sh
python3 gcc_testsuite/run_dg_tests.py \
  --davecc bazel-bin/davecc \
  --root /Users/FZDSZZ/gcc-test/gcc/testsuite \
  --suites gcc.dg \
  --match 20020425-1.c \
  --jobs 1 \
  --timeout 3
```

Likely area:

```text
c_compiler/frontend/syntax/statement_parser.c
```

The probable cause is recursive parsing of the `else if` chain. The preferred
fix is to parse consecutive `else if` clauses iteratively, or otherwise avoid
one C call-stack frame per clause while preserving the AST shape.

Acceptance criteria:

- The GCC source completes without a crash or timeout.
- A minimized/generated C regression exercises enough nesting to catch
  recursion returning.
- Existing C syntax and execution tests pass.

## Priority 2: `_BitInt` static initializer assertion

Source:

```text
/Users/FZDSZZ/gcc-test/gcc/testsuite/gcc.dg/bitint-3.c
```

davecc aborts with:

```text
Assertion failed: (false), function InitInteger, file compiler.c, line 457.
```

Reproduce safely:

```sh
python3 gcc_testsuite/run_dg_tests.py \
  --davecc bazel-bin/davecc \
  --root /Users/FZDSZZ/gcc-test/gcc/testsuite \
  --suites gcc.dg \
  --match bitint-3.c \
  --jobs 1 \
  --timeout 5
```

Confirmed cause:

```text
c_compiler/driver/compiler.c:424-458
```

`InitInteger` handles char, short, int, long, and long long, but an integral
`_BitInt` reaches the final `assert(false)`. A correct fix should encode
supported `_BitInt` initializers according to their storage size and signed
value representation. The test only requires widths up to davecc's supported
64-bit maximum.

Do not use `gcc.dg/bitint-87.c` as the regression: it requires GCC's
`bitint575` effective target and widths above davecc's supported maximum.

Acceptance criteria:

- `bitint-3.c` no longer aborts.
- Static initializers for representative 2-, 6-, 32-, and 64-bit `_BitInt`
  objects are covered.
- Invalid widths still diagnose rather than abort.

## Priority 3: C++ semantic crash after overload diagnostics

Source:

```text
/Users/FZDSZZ/gcc-test/gcc/testsuite/g++.dg/vect/pr116674.cc
```

The original sweep produced `SIGSEGV` after diagnostics beginning with:

```text
No matching overload for operator=
```

This test is under GCC's vectorizer directory and is now excluded from default
runner sweeps, but the crash occurs in frontend processing and is still worth
investigating after the two generic candidates above.

Compile it with `-std=c++17`, `-target pcode`, `-fsyntax-only`, and
`-error-limit=0`, using the required process-group timeout wrapper. Use LLDB to
capture the first crashing backtrace before minimizing it.

## Post-diagnostic C hangs

The raw C/common/torture sweep found many files that emit a diagnostic and then
fail to terminate. Some depend on unsupported GCC extensions, but nontermination
after an error is still a recovery defect. Start with small sources and group
them by the final diagnostic and stack sample before fixing them individually.

Examples (all fixed by `d350762`; the remaining `gcc.dg` cases are listed
above):

```text
c-c++-common/Wunused-value-1.c
c-c++-common/auto-init-5.c
gcc.dg/20020319-1.c
gcc.dg/Waddress-3.c
gcc.dg/attr-assume-3.c
```

Both fixed loops share a shape worth looking for in the remaining hangs: a loop
whose exit condition is a specific token, whose body reports an error, and whose
recovery cannot consume anything at end of input.

Each can be selected with `--match <basename>`, `--jobs 1`, and a 3-5 second
timeout. Avoid starting with tests that require unsupported atomics, decimal
floating point, vector extensions, architecture intrinsics, or `_BitInt`
widths above 64.

## Raw sweep summary

The C++ sweep, before adding target-directory filtering:

- 19,779 files scanned
- 17,031 jobs attempted
- 17,022 completed without crashing
- 1 segmentation fault and 8 timeouts

The C/common/torture sweep, before refining unsupported-target filtering:

- 24,798 files scanned
- 22,396 jobs attempted
- 21,937 completed without crashing
- 459 crashes or timeouts
- Breakdown: 20 in `c-c++-common`, 404 in `gcc.dg`, and 35 in
  `gcc.c-torture/compile`

Most of the 459 are timeout clusters rather than independent root causes.
Prioritize reproducible signals/assertions first, then classify hangs by common
stack location.
