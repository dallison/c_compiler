# GCC frontend tests

This runner applies davecc to frontend tests from an external GCC source
checkout. GCC sources are deliberately not stored in this repository.

Create a sparse checkout:

```sh
git clone --depth=1 --filter=blob:none --sparse \
  https://github.com/gcc-mirror/gcc.git ~/gcc-test
git -C ~/gcc-test sparse-checkout set \
  gcc/testsuite/g++.dg \
  gcc/testsuite/c-c++-common \
  gcc/testsuite/gcc.dg \
  gcc/testsuite/gcc.c-torture/compile
```

Build davecc and run the crash/timeout sweep:

```sh
bazel build //:davecc
python3 gcc_testsuite/run_dg_tests.py \
  --davecc bazel-bin/davecc \
  --root ~/gcc-test/gcc/testsuite
```

The default mode treats ordinary diagnostics as successful test execution and
fails only for crashes, internal compiler errors, or timeouts. Use
`--mode outcome` to compare compile success with `dg-error` expectations, or
`--mode diagnostics` to additionally match supported `dg-error`, `dg-warning`,
and `dg-message` regular expressions.

The default suites are `g++.dg`, `c-c++-common`, `gcc.dg`, and
`gcc.c-torture/compile`. Use `--suites`, `--match`, and `--limit` for focused
runs. Every davecc invocation runs in a separate process group and the entire
group is killed when its per-test timeout expires.
