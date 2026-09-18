# c_compiler
Full C compiler, assembler, linker and loader

This is my full C compiler.  It includes an assembler, linker, loader and interpreter for 6502 and RISC-V.

How to build the compiler, guest libc, and tools with Bazelisk/Bazel or CMake,
and how to install them, is in [docs/building.md](docs/building.md).

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

Linux eBPF is `bpf-unknown-linux-davecc` (aliases `bpf`, `bpfel`, `ebpf`). It is
a freestanding ELF backend with no libc; compile with `-nostdlib` and run the
result with the `bpf` interpreter:

```sh
bazel-bin/davecc -target bpf-unknown-linux-davecc -nostdlib -static \
  -Wl,-e -Wl,main program.c -o program
bazel-bin/bpf program
```

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

### ARM32 testing with Colima

ARM32 binaries run through qemu-arm binfmt in an AArch64 Colima guest. Start or
reuse an AArch64 profile, then install the armhf loader and emulator:

```sh
colima start --arch aarch64
colima ssh -- uname -m
colima ssh -- sudo dpkg --add-architecture armhf
colima ssh -- sudo apt-get update
colima ssh -- sudo apt-get install -y libc6:armhf qemu-user-static binfmt-support
colima ssh -- sudo update-binfmts --enable qemu-arm
```

`uname -m` must print `aarch64`; the emulated process, not the guest kernel, is
32-bit ARM. Verify both required capabilities:

```sh
colima ssh -- test -r /proc/sys/fs/binfmt_misc/qemu-arm
colima ssh -- test -x /lib/ld-linux-armhf.so.3
```

The static and native-loader ARM32 tests use the `default` profile unless
`COLIMA_PROFILE` is set:

```sh
bazel test //:native_linux_arm_minimal_smoke_test
bazel test //:native_linux_arm_smoke_test
bazel test //:native_linux_arm_dynamic_minimal_test
bazel test //:native_linux_arm_dynamic_smoke_test
```

### RV64 testing with Colima

RV64 LP64D binaries run through qemu-riscv64 binfmt in the same AArch64
Colima guest. Install the emulator and cross-architecture glibc loader, then
expose the loader at its ABI path:

```sh
colima start --arch aarch64
colima ssh -- sudo apt-get update
colima ssh -- sudo apt-get install -y \
  libc6-riscv64-cross qemu-user-static binfmt-support
colima ssh -- sudo update-binfmts --enable qemu-riscv64
colima ssh -- sudo ln -sf \
  /usr/riscv64-linux-gnu/lib/ld-linux-riscv64-lp64d.so.1 \
  /lib/ld-linux-riscv64-lp64d.so.1
```

Verify the binfmt registration and loader:

```sh
colima ssh -- test -r /proc/sys/fs/binfmt_misc/qemu-riscv64
colima ssh -- test -x /lib/ld-linux-riscv64-lp64d.so.1
```

The RV64 static and native-loader tests use the `default` profile unless
`COLIMA_PROFILE` is set:

```sh
bazel test //:native_linux_riscv_minimal_smoke_test
bazel test //:native_linux_riscv_smoke_test
bazel test //:native_linux_riscv_dynamic_minimal_test
bazel test //:native_linux_riscv_dynamic_smoke_test
```

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

### ARM32 native dynamic runtime

Build the compiler and architecture-isolated ARM32 dynamic artifacts:

```sh
bazel build //:davecc //:arm_linux_dynamic_runtime
```

Compile with the ARM runtime directory and deploy the executable beside its
shared libc:

```sh
DAVECC_INCLUDE_DIR="$PWD/libc/include" \
DAVECC_LIB_DIR="$PWD/bazel-bin/libc/arm" \
  bazel-bin/davecc -target arm-unknown-linux-davecc -dynamic \
  program.cpp -o program

cp bazel-bin/libc/arm/libdavecc.so.1 .
```

The executable requests `/lib/ld-linux-armhf.so.3`, needs
`libdavecc.so.1`, and finds it through `$ORIGIN`. The native armhf loader maps
both files and applies ARM `REL`, `GLOB_DAT`, and eagerly bound `JUMP_SLOT`
relocations. DaveCC startup then installs its own ARM thread pointer with
`__ARM_NR_set_tls` and owns TLS, errno, lifecycle arrays, exceptions, and
clone/futex threads.

As on x86_64, native-loader participation is startup-only. Lazy PLT binding,
`dlopen`/`dlclose`, and DSOs with independent TLS are unsupported after DaveCC
takes ownership of the thread pointer.

### RV64 native dynamic runtime

Build the compiler and architecture-isolated RV64 dynamic artifacts:

```sh
bazel build //:davecc //:riscv_linux_dynamic_runtime
```

Compile with the RISC-V runtime directory and deploy the executable beside its
shared libc:

```sh
DAVECC_INCLUDE_DIR="$PWD/libc/include" \
DAVECC_LIB_DIR="$PWD/bazel-bin/libc/riscv" \
  bazel-bin/davecc -target riscv-unknown-linux-davecc -dynamic \
  program.cpp -o program

cp bazel-bin/libc/riscv/libdavecc.so.1 .
```

The executable requests `/lib/ld-linux-riscv64-lp64d.so.1`, needs
`libdavecc.so.1`, and finds it through `$ORIGIN`. The native loader applies
RISC-V `RELA`, `R_RISCV_64`, `R_RISCV_RELATIVE`, and eagerly bound
`R_RISCV_JUMP_SLOT` relocations before DaveCC startup takes ownership of the
`tp` register, TLS, errno, lifecycle arrays, exceptions, and clone/futex
threads.

Native-loader participation remains startup-only. Lazy PLT binding,
`dlopen`/`dlclose`, and DSOs with independent TLS are unsupported after DaveCC
takes ownership of `tp`.
