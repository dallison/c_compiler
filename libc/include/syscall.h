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

#if defined(__DAVECC_NATIVE_LINUX__)
#include <sys/syscall.h>
#define __DAVECC_HAS_NATIVE_THREADS__ 1
#define __DAVECC_HAS_TLS_THREAD_ERRNO__ 1
#define __DAVECC_HAS_HEAP_LOCK__ 1
#define SYS_CLOSE SYS_close
#define SYS_WRITE SYS_write
#define SYS_READ SYS_read
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

#if !defined(__p_code__) && !defined(__6502__)
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
#elif defined(__x86_64__)
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
#elif defined(__6502__)
#ifdef __cplusplus
extern "C" long syscall(int n, ...);
#else
extern long syscall(int n, ...);
#endif
#else
#error "Unknown architecture"
#endif


#endif /* syscall_h */
