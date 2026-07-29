# 65C02 coroutine parity plan

## Scope

- Treat `65c02` as the runnable embedded target; `//:6502` is its interpreter.
- Match the six-test coroutine suite except for the throw/unwind branch.
- Keep full 6502 exception handling out of scope: metadata emission, frame
  walking, transfer assembly, and runtime support are currently absent.

## Steps

### 1. Add ROM-aware execution harness support

- Extend `cxx_testsuite/run_exec_tests.sh` with an optional `--rom` argument.
- Resolve the ROM through Bazel runfiles.
- Pass `-rom <path>` to the interpreter without affecting existing targets.
- Stop for review after implementation and focused harness validation.

### 2. Wire first-class 65C02 coroutine coverage

- Add `//cxx_testsuite:exec_coroutines_65c02`.
- Use `-target 65c02`, `//:libc_65c02`, `//:6502`, and
  `//:support_rom_65c02`.
- Do not override the 65C02 `_start` entry point with `main`.
- Add the target to `//cxx_testsuite:all`.

### 3. Make the exception-only test profile-aware

- Guard only the throw/unhandled-exception case in
  `cxx_testsuite/tests/exec_coroutines/0006_lifetime_matrix.cpp` with
  `__cpp_exceptions`.
- Preserve all exception-independent lifetime coverage on 65C02.
- Preserve the exception checks on primary backends.

### 4. Document the tested embedded profile

- Update `c_compiler/6502/README.txt` with the 16-bit data model, static/ROM
  execution, coroutine support, and intentional `-fno-exceptions` boundary.

### 5. Verify parity

- Run `//cxx_testsuite:exec_coroutines_65c02` and
  `//:ir_optimizer_regression_test`.
- Re-run coroutine suites on x86-64, AArch64, ARM, and RISC-V.
- Run the syntax and forced x86-64 execution regressions.
- Fix genuine non-exception 65C02 backend/runtime defects rather than skipping
  affected coroutine cases.
