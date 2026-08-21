#ifndef davecc_asm_unistd_h
#define davecc_asm_unistd_h

#if !defined(__DAVECC_NATIVE_LINUX__)
#error "<asm/unistd.h> is only available for Linux targets"
#elif defined(__x86_64__)
#include <davecc/linux/x86_64/syscall_numbers.h>
#elif defined(__aarch64__)
#include <davecc/linux/aarch64/syscall_numbers.h>
#elif defined(__arm__)
#include <davecc/linux/arm/syscall_numbers.h>
#elif defined(__risc_v__)
#include <davecc/linux/riscv/syscall_numbers.h>
#else
#error "Linux syscall numbers are unavailable for this architecture"
#endif

#endif
