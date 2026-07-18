---
name: run-cxx-exec-tests
description: >-
  Build davecc + target libc + interpreter and run the C++ execution test suite
  (cxx_testsuite/tests/exec). Use when adding or debugging a std library header
  or any C++ exec test, to compile a single .cpp the exact way the harness does
  and check its exit code, or to run the whole suite via bazel.
---

# Running the C++ exec test suite

Exec tests live in `cxx_testsuite/tests/exec/*.cpp`. Each test is **return-code
based**: it declares an expected process exit code and `main` returns it.

```cpp
// RUN: -std=c++20
// EXPECT_EXIT: 0
```

The harness (`cxx_testsuite/run_exec_tests.sh`, bazel target
`//cxx_testsuite:exec_x86_64`) compiles each test with davecc and runs it under
the x86_64 interpreter, comparing the exit status to `// EXPECT_EXIT:`.

## Full suite (final validation)

```bash
bazel test //cxx_testsuite:exec_x86_64 --test_output=errors
```

Runs all ~270 exec tests. Use for final validation; too slow for iteration.

## Fast single-file iteration (preferred while developing)

Compile and run ONE test exactly the way the harness does:

```bash
bazel build //:davecc //:libc_x86_64 //:x86_64 2>/dev/null

bazel-bin/davecc -target x86_64 -static -std=c++20 -isystem libc/include \
  -Wl,-e -Wl,main \
  cxx_testsuite/tests/exec/0271_standard_deque.cpp \
  bazel-bin/libc/libcx86_64.a -o /tmp/t.bin >/tmp/c.log 2>&1
echo "compile=$?"          # nonzero -> read /tmp/c.log (grep -v note:)

bazel-bin/x86_64 -i /tmp/t.bin >/tmp/r.log 2>&1
echo "run=$?"              # compare against the test's // EXPECT_EXIT:
```

Key flags (must match the harness):
- `-target x86_64 -static -std=c++20 -isystem libc/include`
- link flags `-Wl,-e -Wl,main` (entry point is `main`)
- interpreter flag `-i`
- libc archive: `bazel-bin/libc/libcx86_64.a`  (bazel label `//:libc_x86_64`)
- interpreter: `bazel-bin/x86_64`             (bazel label `//:x86_64`)

## Gotchas

- **No C runtime init.** Because entry is `main` (`-Wl,-e -Wl,main`), `_start`/
  crt startup is skipped, so libc globals like `stdout` are never initialized.
  `printf`/iostream will dereference a null `FILE` and the interpreter aborts with
  `Load32 outside mapped memory at 0x0`. This is NOT a bug in your code — write
  tests using **return codes**, not stdout. (Do not add `// EXPECT_EXIT` output
  files that rely on printed output for exec tests.)
- **A nonzero guest exit suppresses later shell output.** When `bazel-bin/x86_64`
  exits nonzero (a test returning a nonzero rc, OR a fault), the surrounding
  Shell tool call swallows any later stdout in the same call. Since exec tests
  communicate via exit codes, this bites constantly. Robust pattern: capture the
  code without letting it abort/clobber, append results to a file, and read that
  file in a separate call:

```bash
: > /tmp/res.txt
run_one() {
  local t=$1 rc=0
  bazel-bin/davecc -target x86_64 -static -std=c++20 -isystem libc/include \
    -Wl,-e -Wl,main "$t" bazel-bin/libc/libcx86_64.a -o /tmp/q.bin \
    >/tmp/q.log 2>&1 || { echo "$t COMPILEFAIL" >>/tmp/res.txt; return 0; }
  bazel-bin/x86_64 -i /tmp/q.bin >/tmp/qr.log 2>&1 || rc=$?   # capture, don't clobber
  echo "$t rc=$rc $(head -1 /tmp/qr.log)" >> /tmp/res.txt
  return 0
}
run_one cxx_testsuite/tests/exec/0271_standard_deque.cpp
true   # keep the shell call's own exit 0
# then read /tmp/res.txt in a SEPARATE call
```

  Note `|| rc=$?` (NOT `|| true`, which would overwrite `$?` with 0). A guest
  runtime fault prints diagnostics like `Unsupported instruction at 0x...` or
  `Load32 outside mapped memory at 0x0` to the log with a nonzero exit.
- New headers must be listed in the top-level `BUILD.bazel` `libc_headers`
  filegroup (and their `.cpp` tests just dropped into `tests/exec/`).
- To debug a native davecc crash while compiling a test, use the
  `lldb-interactive` skill.
