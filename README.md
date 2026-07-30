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
