//
//  x86_64_syscalls.h
//  x86_64_interpreter
//

#ifndef x86_64_syscalls_h
#define x86_64_syscalls_h

#include <stdint.h>

#define X86_64_SYSCALL_HALT 1
#define X86_64_SYSCALL_OPEN 2
#define X86_64_SYSCALL_CLOSE 3
#define X86_64_SYSCALL_WRITE 4
#define X86_64_SYSCALL_READ 5
#define X86_64_SYSCALL_RESOLVE 6
#define X86_64_SYSCALL_LSEEK 7
#define X86_64_SYSCALL_MALLOC 8
#define X86_64_SYSCALL_FREE 9
#define X86_64_SYSCALL_REALLOC 10
#define X86_64_SYSCALL_ABORT 11
#define X86_64_SYSCALL_EXIT 12
#define X86_64_SYSCALL_TIME 13
#define X86_64_SYSCALL_CLOCK 14
#define X86_64_SYSCALL_THREAD_CREATE 15
#define X86_64_SYSCALL_THREAD_JOIN 16
#define X86_64_SYSCALL_THREAD_SELF 17
#define X86_64_SYSCALL_GET_TP 18
#define X86_64_SYSCALL_THREAD_EXIT 19
#define X86_64_SYSCALL_HEAP_LOCK 20
#define X86_64_SYSCALL_HEAP_UNLOCK 21

struct X86_64Interpreter;

int64_t X86_64HandleSyscall(struct X86_64Interpreter* interpreter,
                            int64_t number, int64_t a0, int64_t a1,
                            int64_t a2, int64_t a3, int64_t a4, int64_t a5);

#endif /* x86_64_syscalls_h */
