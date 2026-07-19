//
//  syscall.h
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef syscall_h
#define syscall_h

#if defined(__risc_v__)
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_WRITE 4
#define SYS_READ 5
#define SYS_LSEEK 7
#define SYS_MALLOC 8
#define SYS_FREE 9
#define SYS_REALLOC 10
#define SYS_ABORT 11
#define SYS_EXIT 12
#define SYS_EXIT_CLEAN 22
#define SYS_TIME 13
#define SYS_CLOCK 14

#elif defined(__x86_64__)
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_WRITE 4
#define SYS_READ 5
#define SYS_LSEEK 7
#define SYS_MALLOC 8
#define SYS_FREE 9
#define SYS_REALLOC 10
#define SYS_ABORT 11
#define SYS_EXIT 12
#define SYS_EXIT_CLEAN 22
#define SYS_TIME 13
#define SYS_CLOCK 14
#define SYS_THREAD_CREATE 15
#define SYS_THREAD_JOIN 16
#define SYS_THREAD_SELF 17
#define SYS_GET_TP 18
#define SYS_THREAD_EXIT 19
#define SYS_HEAP_LOCK 20
#define SYS_HEAP_UNLOCK 21

#elif defined(__p_code__)
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_WRITE 4
#define SYS_READ 5
#define SYS_LSEEK 7
#define SYS_ABORT 11
#define SYS_EXIT 12
#define SYS_EXIT_CLEAN 22
#define SYS_TIME 13
#define SYS_CLOCK 14

#elif defined(__aarch64__)
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_WRITE 4
#define SYS_READ 5
#define SYS_LSEEK 7
#define SYS_MALLOC 8
#define SYS_FREE 9
#define SYS_REALLOC 10
#define SYS_ABORT 11
#define SYS_EXIT 12
#define SYS_EXIT_CLEAN 22
#define SYS_TIME 13
#define SYS_CLOCK 14
#define SYS_THREAD_CREATE 15
#define SYS_THREAD_JOIN 16
#define SYS_THREAD_SELF 17
#define SYS_GET_TP 18
#define SYS_THREAD_EXIT 19
#define SYS_HEAP_LOCK 20
#define SYS_HEAP_UNLOCK 21

#elif defined(__arm__)
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_WRITE 4
#define SYS_READ 5
#define SYS_LSEEK 7
#define SYS_MALLOC 8
#define SYS_FREE 9
#define SYS_REALLOC 10
#define SYS_ABORT 11
#define SYS_EXIT 12
#define SYS_EXIT_CLEAN 22
#define SYS_TIME 13
#define SYS_CLOCK 14
#define SYS_THREAD_CREATE 15
#define SYS_THREAD_JOIN 16
#define SYS_THREAD_SELF 17
#define SYS_GET_TP 18
#define SYS_THREAD_EXIT 19
#define SYS_HEAP_LOCK 20
#define SYS_HEAP_UNLOCK 21

#elif defined(__W65C02__)
#define SYS_EXIT 1
#define SYS_EXIT_CLEAN 22
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_WRITE 4
#define SYS_READ 5
#define SYS_LSEEK 7
#define SYS_ABORT 8
#define SYS_TIME 13
#define SYS_CLOCK 14
#else
#error "Unknown architecture for syscall"
#endif

#if defined(SYS_THREAD_CREATE) && defined(SYS_HEAP_LOCK) && \
    !defined(__p_code__)
#define __DAVECC_HAS_GUEST_THREADS__ 1
#define __DAVECC_HAS_THREAD_SAFE_GUARDS__ 1
#define __DAVECC_HAS_ATEXIT_LOCK__ 1
#define __DAVECC_HAS_HEAP_LOCK__ 1
#define __DAVECC_HAS_TLS_THREAD_ERRNO__ 1
#endif

#if defined(__risc_v__)
#define SYSCALL \
   return asm( \
              "mv t6, a0\n" \
              "ecall" \
              )
extern long syscall(int n, ...);
#elif defined(__x86_64__)
extern long syscall(int n, ...);
#elif defined(__p_code__)
extern long syscall(int n, ...);
#elif defined(__aarch64__)
extern long syscall(int n, ...);
#elif defined(__arm__)
extern long syscall(int n, ...);
#elif defined(__W65C02__)
extern long syscall(int n, ...);
#else
#error "Unknown architecture"
#endif


#endif /* syscall_h */
