# c_compiler
Full C compiler, assembler, linker and loader

This is my full C compiler.  It includes an assembler, linker, loader and interpreter for 6502 and RISC-V.

## C++ threading target profiles

The hosted x86-64, AArch64, ARM, and RISC-V profiles support the C++20
threading library, including threads, stop tokens, condition variables,
semaphores, latches, and barriers.

P-code, 6502, and 65C02 are intentionally single-threaded profiles. Including
`<thread>`, `<stop_token>`, `<condition_variable>`, `<semaphore>`, `<latch>`, or
`<barrier>` for those targets produces a compile-time diagnostic.

## Native Linux

Build the compiler and the native libc/startup profile for the target:

```sh
bazel build //:davecc //:libc_aarch64_linux //:aarch64_linux_start
```

The canonical Linux target automatically selects the static native startup and
libc, so compiling a program does not require manual object ordering:

```sh
bazel-bin/davecc -target aarch64-unknown-linux-davecc program.c -o program
./program
```

Equivalent static cross-target profiles are available for
`x86_64-unknown-linux-davecc`, `arm-unknown-linux-davecc`, and
`riscv-unknown-linux-davecc`. Native libc uses Linux UAPI syscall numbers and
provides file I/O, filesystem operations, clocks, random data, heap allocation,
TLS, and C11 threads. Dynamic ELF loading is not yet supported.
