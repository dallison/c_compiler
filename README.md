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
TLS, and C11 threads.

### x86_64 testing with Colima

On an Apple Silicon macOS host, start a QEMU-backed x86_64 Linux profile and
verify the guest architecture:

```sh
brew install colima lima-additional-guestagents
colima start x86 --arch x86_64 --vm-type qemu
colima -p x86 ssh -- uname -m
```

The last command must print `x86_64`. The static and native-loader smoke tests
use the `x86` profile by default:

```sh
bazel test //:native_linux_x86_64_smoke_test
bazel test //:native_linux_x86_64_dynamic_minimal_test
bazel test //:native_linux_x86_64_dynamic_smoke_test
```

Set `COLIMA_PROFILE` to use a different profile name.

### x86_64 native dynamic runtime

Build the compiler, PIC shared libc, and executable-side dynamic CRT:

```sh
bazel build //:davecc //:x86_64_linux_dynamic_runtime
```

Static mode remains the default and can also be requested explicitly:

```sh
bazel-bin/davecc -target x86_64-unknown-linux-davecc -static \
  program.cpp -o program
```

Use `-dynamic` to produce an executable loaded by the native x86_64 Linux
interpreter:

```sh
DAVECC_INCLUDE_DIR="$PWD/libc/include" \
DAVECC_LIB_DIR="$PWD/bazel-bin/libc" \
  bazel-bin/davecc -target x86_64-unknown-linux-davecc -dynamic \
  program.cpp -o program

cp bazel-bin/libc/libdavecc.so.1 .
```

Deploy `program` and `libdavecc.so.1` in the same directory. The executable
contains `/lib64/ld-linux-x86-64.so.2` as `PT_INTERP`, records
`libdavecc.so.1` as `DT_NEEDED`, and uses a `$ORIGIN` runpath. Relocations are
bound eagerly before DaveCC installs its own `%fs`-based TLS runtime. DaveCC
then owns application initialization, finalization, errno, TLS destructors, and
clone/futex threads.

This first dynamic mode uses the native loader only for startup mapping and
eager relocation. Lazy PLT binding, `dlopen`/`dlclose`, and shared objects with
independent TLS are not supported after DaveCC takes ownership of `%fs`.
