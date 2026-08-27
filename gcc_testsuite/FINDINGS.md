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
- A hosted C++ program no longer starts on aarch64: the interpreter rejects an
  init-array entry with "Function array entry 0x800005250 is not executable".
  Bisected to `4f4fcb0 Add x86_64 native dynamic runtime`, which added the
  `__davecc_shared_init` hook and `libc/dynamic_libc_lifecycle.c`.  This is what
  makes `davecc_driver_defaults_test` fail, and it fails at that test's very first
  hosted link, so everything after it in that script is currently unexercised.
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
    left alone rather than changed on the strength of an argument.

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

- `//:c23_bitint_test` fails: an `unsigned _BitInt(5)` bitfield read gives the
  wrong value at `-O2` but not at `-O0`.
- `//c_testsuite:single_exec_x86_64` fails `00200.c`.
- `//c_testsuite:warning_diagnostics` fails: the script runs davecc with
  `-Werror=unused-value` under `set -e` and expects it to succeed, but promoting
  a warning to an error makes davecc exit nonzero.
- `//:libc_x86_64_test`, `//:c23_numeric_headers_test` and
  `//:davecc_driver_defaults_test` fail.
- `//cxx_testsuite:exec_x86_64` fails three tests whose guest programs exceed
  the harness's 30 second run timeout on this machine (35 s, 37 s and 75 s);
  all three exit 0 when run without it.

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
