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

## Remaining crashes and hangs

`gcc.dg` and `c-c++-common` are clean (`--jobs 8 --timeout 10`): 14,053 and
2,842 pass with no crashes or hangs.  `gcc.c-torture` has 1,996 passing and 3
left, all in the `limits-*` family:

```text
gcc.c-torture/compile/limits-structnest.c  rc=-11 (deep struct nesting)
gcc.c-torture/compile/limits-externdecl.c  timeout
gcc.c-torture/compile/limits-fndefn.c      timeout
```

The signal is recursive descent running out of stack, like the `else if` chain
in `23a4558`; it wants the same treatment, not a larger stack.  The `g++.dg`
sweep has not been re-run since these fixes.

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
