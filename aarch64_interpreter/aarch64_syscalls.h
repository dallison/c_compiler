//
//  aarch64_syscalls.h
//  aarch64_interpreter
//

#ifndef aarch64_syscalls_h
#define aarch64_syscalls_h

#include <stdint.h>

#define AARCH64_SYSCALL_REG 16

#define AARCH64_SYSCALL_HALT 1
#define AARCH64_SYSCALL_OPEN 2
#define AARCH64_SYSCALL_CLOSE 3
#define AARCH64_SYSCALL_WRITE 4
#define AARCH64_SYSCALL_READ 5
#define AARCH64_SYSCALL_RESOLVE 6
#define AARCH64_SYSCALL_LSEEK 7
#define AARCH64_SYSCALL_MALLOC 8
#define AARCH64_SYSCALL_FREE 9
#define AARCH64_SYSCALL_REALLOC 10
#define AARCH64_SYSCALL_ABORT 11
#define AARCH64_SYSCALL_EXIT 12
#define AARCH64_SYSCALL_EXIT_CLEAN 22
#define AARCH64_SYSCALL_TIME 13
#define AARCH64_SYSCALL_CLOCK 14
#define AARCH64_SYSCALL_THREAD_CREATE 15
#define AARCH64_SYSCALL_THREAD_JOIN 16
#define AARCH64_SYSCALL_THREAD_SELF 17
#define AARCH64_SYSCALL_GET_TP 18
#define AARCH64_SYSCALL_THREAD_EXIT 19
#define AARCH64_SYSCALL_HEAP_LOCK 20
#define AARCH64_SYSCALL_HEAP_UNLOCK 21
#define AARCH64_SYSCALL_THREAD_YIELD 23
#define AARCH64_SYSCALL_MONOTONIC_TIME 24

struct AARCH64Interpreter;

int64_t AARCH64HandleSyscall(struct AARCH64Interpreter* interpreter,
                             int64_t number, int64_t a0, int64_t a1,
                             int64_t a2, int64_t a3, int64_t a4, int64_t a5);

#endif /* aarch64_syscalls_h */
