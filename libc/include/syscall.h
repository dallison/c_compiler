//
//  syscall.h
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef syscall_h
#define syscall_h

#include <davecc_guest_syscalls.h>

#if defined(__DAVECC_NATIVE_DARWIN__)
// C library entry points come from libSystem.  <filesystem>, <random>, and
// chrono TZDB still use the guest numbers below; darwin_fs.c / darwin_syscall.c
// implement them with POSIX and the host TZif reader.
#define SYS_MONOTONIC_TIME DAVE_SYS_MONOTONIC_TIME
#define SYS_REALTIME_TIME DAVE_SYS_REALTIME_TIME
#define SYS_TZDB_VERSION DAVE_SYS_TZDB_VERSION
#define SYS_TZDB_GENERATION DAVE_SYS_TZDB_GENERATION
#define SYS_TZDB_RELOAD DAVE_SYS_TZDB_RELOAD
#define SYS_TZDB_CURRENT_ZONE DAVE_SYS_TZDB_CURRENT_ZONE
#define SYS_TZDB_ZONE_COUNT DAVE_SYS_TZDB_ZONE_COUNT
#define SYS_TZDB_ZONE_NAME DAVE_SYS_TZDB_ZONE_NAME
#define SYS_TZDB_LOCATE_ZONE DAVE_SYS_TZDB_LOCATE_ZONE
#define SYS_TZDB_SYS_INFO DAVE_SYS_TZDB_SYS_INFO
#define SYS_TZDB_LOCAL_INFO DAVE_SYS_TZDB_LOCAL_INFO
#define SYS_TZDB_LEAP_COUNT DAVE_SYS_TZDB_LEAP_COUNT
#define SYS_TZDB_LEAP_INFO DAVE_SYS_TZDB_LEAP_INFO
#define SYS_RANDOM_BYTES DAVE_SYS_RANDOM_BYTES
#elif defined(__DAVECC_NATIVE_LINUX__)
#include <sys/syscall.h>
#if defined(__risc_v__) && defined(__ILP32__)
// RV32 was introduced after the Linux time64 transition.  Its kernel ABI does
// not implement the legacy futex syscall number.
#undef SYS_futex
#define SYS_futex SYS_futex_time64
#endif
#define __DAVECC_HAS_NATIVE_THREADS__ 1
#define __DAVECC_HAS_TLS_THREAD_ERRNO__ 1
#define __DAVECC_HAS_HEAP_LOCK__ 1
#define __DAVECC_HAS_ATEXIT_LOCK__ 1
#define __DAVECC_HAS_THREAD_SAFE_GUARDS__ 1
#define SYS_CLOSE SYS_close
#define SYS_WRITE SYS_write
#define SYS_READ SYS_read
#define SYS_POLL SYS_poll
#define SYS_LSEEK SYS_lseek
#define SYS_ABORT SYS_exit_group
#define SYS_EXIT SYS_exit_group
#define SYS_EXIT_CLEAN SYS_exit_group
#define SYS_MONOTONIC_TIME SYS_clock_gettime
#define SYS_REALTIME_TIME SYS_clock_gettime
#define SYS_RANDOM_BYTES SYS_getrandom
#else
#define SYS_OPEN DAVE_SYS_OPEN
#define SYS_CLOSE DAVE_SYS_CLOSE
#define SYS_WRITE DAVE_SYS_WRITE
#define SYS_READ DAVE_SYS_READ
#define SYS_LSEEK DAVE_SYS_LSEEK
#define SYS_ABORT DAVE_SYS_ABORT
#define SYS_EXIT DAVE_SYS_EXIT
#define SYS_EXIT_CLEAN DAVE_SYS_EXIT_CLEAN
#define SYS_TIME DAVE_SYS_TIME
#define SYS_CLOCK DAVE_SYS_CLOCK
#define SYS_MONOTONIC_TIME DAVE_SYS_MONOTONIC_TIME
#define SYS_REALTIME_TIME DAVE_SYS_REALTIME_TIME

// Wasm has no thread of its own to lend and no host heap to borrow: its
// memory is the module's own, so it is left out along with the targets that
// never had either.
#if !defined(__p_code__) && !defined(__6502__) && !defined(__wasm32__) && \
    !defined(__xtensa__) && !defined(__bpf__)
#define SYS_MALLOC DAVE_SYS_MALLOC
#define SYS_FREE DAVE_SYS_FREE
#define SYS_REALLOC DAVE_SYS_REALLOC
#define SYS_THREAD_CREATE DAVE_SYS_THREAD_CREATE
#define SYS_THREAD_JOIN DAVE_SYS_THREAD_JOIN
#define SYS_THREAD_SELF DAVE_SYS_THREAD_SELF
#define SYS_GET_TP DAVE_SYS_GET_TP
#define SYS_THREAD_EXIT DAVE_SYS_THREAD_EXIT
#define SYS_HEAP_LOCK DAVE_SYS_HEAP_LOCK
#define SYS_HEAP_UNLOCK DAVE_SYS_HEAP_UNLOCK
#define SYS_THREAD_YIELD DAVE_SYS_THREAD_YIELD
#define SYS_THREAD_DETACH DAVE_SYS_THREAD_DETACH
#define SYS_ADDR_WAIT DAVE_SYS_ADDR_WAIT
#define SYS_ADDR_WAKE DAVE_SYS_ADDR_WAKE
#define SYS_THREAD_SLEEP DAVE_SYS_THREAD_SLEEP
#define SYS_HARDWARE_CONCURRENCY DAVE_SYS_HARDWARE_CONCURRENCY
#endif

#if defined(__6502__)
#undef SYS_EXIT
#undef SYS_ABORT
#define SYS_EXIT DAVE_SYS_6502_EXIT
#define SYS_ABORT DAVE_SYS_6502_ABORT
#endif

#define SYS_FS_STATUS DAVE_SYS_FS_STATUS
#define SYS_FS_OPEN_DIRECTORY DAVE_SYS_FS_OPEN_DIRECTORY
#define SYS_FS_READ_DIRECTORY DAVE_SYS_FS_READ_DIRECTORY
#define SYS_FS_CLOSE_DIRECTORY DAVE_SYS_FS_CLOSE_DIRECTORY
#define SYS_FS_CREATE_DIRECTORY DAVE_SYS_FS_CREATE_DIRECTORY
#define SYS_FS_REMOVE DAVE_SYS_FS_REMOVE
#define SYS_FS_RENAME DAVE_SYS_FS_RENAME
#define SYS_FS_CURRENT_PATH DAVE_SYS_FS_CURRENT_PATH
#define SYS_FS_SET_CURRENT_PATH DAVE_SYS_FS_SET_CURRENT_PATH
#define SYS_FS_READ_SYMLINK DAVE_SYS_FS_READ_SYMLINK
#define SYS_FS_CREATE_SYMLINK DAVE_SYS_FS_CREATE_SYMLINK
#define SYS_FS_CREATE_HARD_LINK DAVE_SYS_FS_CREATE_HARD_LINK
#define SYS_FS_SET_PERMISSIONS DAVE_SYS_FS_SET_PERMISSIONS
#define SYS_FS_RESIZE DAVE_SYS_FS_RESIZE
#define SYS_FS_SET_MODIFICATION_TIME DAVE_SYS_FS_SET_MODIFICATION_TIME
#define SYS_FS_SPACE DAVE_SYS_FS_SPACE
#define SYS_FS_COPY_FILE DAVE_SYS_FS_COPY_FILE
#define SYS_FS_CANONICAL DAVE_SYS_FS_CANONICAL
#define SYS_FS_DESCRIPTOR_STATUS DAVE_SYS_FS_DESCRIPTOR_STATUS
#define SYS_ENVIRONMENT_VALUE DAVE_SYS_ENVIRONMENT_VALUE
#define SYS_POLL DAVE_SYS_POLL
#define SYS_MMAP DAVE_SYS_MMAP
#define SYS_MUNMAP DAVE_SYS_MUNMAP
#define SYS_MPROTECT DAVE_SYS_MPROTECT
#define SYS_SHM_OPEN DAVE_SYS_SHM_OPEN
#define SYS_SHM_UNLINK DAVE_SYS_SHM_UNLINK
#define SYS_FTRUNCATE DAVE_SYS_FTRUNCATE
#define SYS_SOCKET DAVE_SYS_SOCKET
#define SYS_BIND DAVE_SYS_BIND
#define SYS_LISTEN DAVE_SYS_LISTEN
#define SYS_ACCEPT DAVE_SYS_ACCEPT
#define SYS_CONNECT DAVE_SYS_CONNECT
#define SYS_SEND DAVE_SYS_SEND
#define SYS_RECV DAVE_SYS_RECV
#define SYS_SENDMSG DAVE_SYS_SENDMSG
#define SYS_RECVMSG DAVE_SYS_RECVMSG
#define SYS_SHUTDOWN DAVE_SYS_SHUTDOWN
#define SYS_SETSOCKOPT DAVE_SYS_SETSOCKOPT
#define SYS_GETSOCKOPT DAVE_SYS_GETSOCKOPT
#define SYS_GETSOCKNAME DAVE_SYS_GETSOCKNAME
#define SYS_GETPEERNAME DAVE_SYS_GETPEERNAME
#define SYS_PIPE DAVE_SYS_PIPE
#define SYS_FCNTL DAVE_SYS_FCNTL
#define SYS_GETPID DAVE_SYS_GETPID
#define SYS_GETHOSTNAME DAVE_SYS_GETHOSTNAME
#define SYS_UNLINK DAVE_SYS_UNLINK
#define SYS_DUP DAVE_SYS_DUP
#define SYS_DUP2 DAVE_SYS_DUP2

#define SYS_TZDB_VERSION DAVE_SYS_TZDB_VERSION
#define SYS_TZDB_GENERATION DAVE_SYS_TZDB_GENERATION
#define SYS_TZDB_RELOAD DAVE_SYS_TZDB_RELOAD
#define SYS_TZDB_CURRENT_ZONE DAVE_SYS_TZDB_CURRENT_ZONE
#define SYS_TZDB_ZONE_COUNT DAVE_SYS_TZDB_ZONE_COUNT
#define SYS_TZDB_ZONE_NAME DAVE_SYS_TZDB_ZONE_NAME
#define SYS_TZDB_LOCATE_ZONE DAVE_SYS_TZDB_LOCATE_ZONE
#define SYS_TZDB_SYS_INFO DAVE_SYS_TZDB_SYS_INFO
#define SYS_TZDB_LOCAL_INFO DAVE_SYS_TZDB_LOCAL_INFO
#define SYS_TZDB_LEAP_COUNT DAVE_SYS_TZDB_LEAP_COUNT
#define SYS_TZDB_LEAP_INFO DAVE_SYS_TZDB_LEAP_INFO
#define SYS_RANDOM_BYTES DAVE_SYS_RANDOM_BYTES

// Linux programs call syscall(SYS_mmap, ...) with kernel numbers and the
// kernel argument list. Those numbers collide with the DaveCC guest ABI, so
// interpreter builds publish them in a private range and __davecc_linux_syscall
// turns them back into guest calls.
#if defined(__linux__)
#define __NR_read 0x40000001
#define SYS_read __NR_read
#define __NR_write 0x40000002
#define SYS_write __NR_write
#define __NR_close 0x40000003
#define SYS_close __NR_close
#define __NR_mmap 0x40000004
#define SYS_mmap __NR_mmap
#define __NR_mmap2 0x40000005
#define SYS_mmap2 __NR_mmap2
#define __NR_munmap 0x40000006
#define SYS_munmap __NR_munmap
#define __NR_mprotect 0x40000007
#define SYS_mprotect __NR_mprotect
#define __NR_getpid 0x40000008
#define SYS_getpid __NR_getpid
#define __NR_futex 0x40000009
#define SYS_futex __NR_futex
#define __NR_gettid 0x4000000a
#define SYS_gettid __NR_gettid
#define __NR_clone 0x4000000b
#define SYS_clone __NR_clone
#define __NR_exit 0x4000000c
#define SYS_exit __NR_exit
#define __NR_exit_group 0x4000000d
#define SYS_exit_group __NR_exit_group
#define __NR_rt_sigprocmask 0x4000000e
#define SYS_rt_sigprocmask __NR_rt_sigprocmask
#define __NR_getcpu 0x4000000f
#define SYS_getcpu __NR_getcpu
#endif
#endif

#if defined(SYS_TZDB_VERSION)
#define __DAVECC_HAS_HOST_TZDB__ 1
#endif

#if defined(SYS_RANDOM_BYTES)
#define __DAVECC_HAS_HOST_RANDOM__ 1
#endif

#if defined(SYS_THREAD_CREATE) && defined(SYS_HEAP_LOCK) && \
    !defined(__p_code__)
#define __DAVECC_HAS_GUEST_THREADS__ 1
#define __DAVECC_HAS_THREAD_SAFE_GUARDS__ 1
#define __DAVECC_HAS_ATEXIT_LOCK__ 1
#define __DAVECC_HAS_HEAP_LOCK__ 1
#define __DAVECC_HAS_TLS_THREAD_ERRNO__ 1
#endif

// libc/threads.c implements <threads.h> (and the address waits behind
// atomic wait/notify) for guest threads, native Linux, and Darwin.
#if defined(__DAVECC_HAS_GUEST_THREADS__) || \
    defined(__DAVECC_HAS_NATIVE_THREADS__) || defined(__DAVECC_NATIVE_DARWIN__)
#define __DAVECC_HAS_THREADS__ 1
#endif

#if defined(SYS_MONOTONIC_TIME) && defined(SYS_REALTIME_TIME)
#define __DAVECC_HAS_HOST_CLOCK__ 1
#endif

#if defined(__risc_v__)
#define SYSCALL \
   return asm( \
              "mv t6, a0\n" \
              "ecall" \
              )
#ifdef __cplusplus
extern "C" {
#endif
extern long syscall(int n, ...);
#ifdef __cplusplus
}
#endif
#elif defined(__x86_64__) || defined(__i386__)
#ifdef __cplusplus
extern "C" long syscall(int n, ...);
#else
extern long syscall(int n, ...);
#endif
#elif defined(__p_code__)
#ifdef __cplusplus
extern "C" long syscall(int n, ...);
#else
extern long syscall(int n, ...);
#endif
#elif defined(__aarch64__)
#ifdef __cplusplus
extern "C" long syscall(int n, ...);
#else
extern long syscall(int n, ...);
#endif
#elif defined(__arm__)
#ifdef __cplusplus
extern "C" long syscall(int n, ...);
#else
extern long syscall(int n, ...);
#endif
#elif defined(__xtensa__)
#ifdef __cplusplus
extern "C" long syscall(int n, ...);
#else
extern long syscall(int n, ...);
#endif
#elif defined(__6502__)
#ifdef __cplusplus
extern "C" long syscall(int n, ...);
#else
extern long syscall(int n, ...);
#endif
#elif defined(__wasm32__)
#ifdef __cplusplus
extern "C" long syscall(int n, ...);
#else
extern long syscall(int n, ...);
#endif
#elif defined(__bpf__)
#ifdef __cplusplus
extern "C" long syscall(int n, ...);
#else
extern long syscall(int n, ...);
#endif
#else
#error "Unknown architecture"
#endif

#if defined(__DAVECC_INTERPRETER_ABI__) && defined(__linux__) && \
    !defined(__DAVECC_LIBC_BUILD__)
#ifdef __cplusplus
extern "C" {
#endif
long __davecc_linux_syscall(long number, ...);
#ifdef __cplusplus
}
#endif
#define syscall __davecc_linux_syscall
#endif

#endif /* syscall_h */
