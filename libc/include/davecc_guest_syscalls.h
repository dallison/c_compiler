#ifndef davecc_guest_syscalls_h
#define davecc_guest_syscalls_h

// Stable OS-neutral ABI used between DaveCC programs and the software
// interpreters. These are not kernel syscall numbers.
#define DAVE_SYS_OPEN 2
#define DAVE_SYS_CLOSE 3
#define DAVE_SYS_WRITE 4
#define DAVE_SYS_READ 5
#define DAVE_SYS_RESOLVE 6
#define DAVE_SYS_LSEEK 7
#define DAVE_SYS_MALLOC 8
#define DAVE_SYS_FREE 9
#define DAVE_SYS_REALLOC 10
#define DAVE_SYS_ABORT 11
#define DAVE_SYS_EXIT 12
#define DAVE_SYS_TIME 13
#define DAVE_SYS_CLOCK 14
#define DAVE_SYS_THREAD_CREATE 15
#define DAVE_SYS_THREAD_JOIN 16
#define DAVE_SYS_THREAD_SELF 17
#define DAVE_SYS_GET_TP 18
#define DAVE_SYS_THREAD_EXIT 19
#define DAVE_SYS_HEAP_LOCK 20
#define DAVE_SYS_HEAP_UNLOCK 21
#define DAVE_SYS_EXIT_CLEAN 22
#define DAVE_SYS_THREAD_YIELD 23
#define DAVE_SYS_MONOTONIC_TIME 24
#define DAVE_SYS_THREAD_DETACH 25
#define DAVE_SYS_ADDR_WAIT 26
#define DAVE_SYS_ADDR_WAKE 27
#define DAVE_SYS_THREAD_SLEEP 28
#define DAVE_SYS_HARDWARE_CONCURRENCY 29
#define DAVE_SYS_REALTIME_TIME 30

#define DAVE_SYS_FS_STATUS 31
#define DAVE_SYS_FS_OPEN_DIRECTORY 32
#define DAVE_SYS_FS_READ_DIRECTORY 33
#define DAVE_SYS_FS_CLOSE_DIRECTORY 34
#define DAVE_SYS_FS_CREATE_DIRECTORY 35
#define DAVE_SYS_FS_REMOVE 36
#define DAVE_SYS_FS_RENAME 37
#define DAVE_SYS_FS_CURRENT_PATH 38
#define DAVE_SYS_FS_SET_CURRENT_PATH 39
#define DAVE_SYS_FS_READ_SYMLINK 40
#define DAVE_SYS_FS_CREATE_SYMLINK 41
#define DAVE_SYS_FS_CREATE_HARD_LINK 42
#define DAVE_SYS_FS_SET_PERMISSIONS 43
#define DAVE_SYS_FS_RESIZE 44
#define DAVE_SYS_FS_SET_MODIFICATION_TIME 45
#define DAVE_SYS_FS_SPACE 46
#define DAVE_SYS_FS_COPY_FILE 47
#define DAVE_SYS_FS_CANONICAL 48

#define DAVE_SYS_TZDB_VERSION 49
#define DAVE_SYS_TZDB_GENERATION 50
#define DAVE_SYS_TZDB_RELOAD 51
#define DAVE_SYS_TZDB_CURRENT_ZONE 52
#define DAVE_SYS_TZDB_ZONE_COUNT 53
#define DAVE_SYS_TZDB_ZONE_NAME 54
#define DAVE_SYS_TZDB_LOCATE_ZONE 55
#define DAVE_SYS_TZDB_SYS_INFO 56
#define DAVE_SYS_TZDB_LOCAL_INFO 57
#define DAVE_SYS_TZDB_LEAP_COUNT 58
#define DAVE_SYS_TZDB_LEAP_INFO 59
#define DAVE_SYS_RANDOM_BYTES 60

#define DAVE_SYS_6502_EXIT 1
#define DAVE_SYS_6502_ABORT 8

#define DAVE_GUEST_SYSCALL_LIST(X) \
  X(OPEN)                           \
  X(CLOSE)                          \
  X(WRITE)                          \
  X(READ)                           \
  X(RESOLVE)                        \
  X(LSEEK)                          \
  X(MALLOC)                         \
  X(FREE)                           \
  X(REALLOC)                        \
  X(ABORT)                          \
  X(EXIT)                           \
  X(TIME)                           \
  X(CLOCK)                          \
  X(THREAD_CREATE)                  \
  X(THREAD_JOIN)                    \
  X(THREAD_SELF)                    \
  X(GET_TP)                         \
  X(THREAD_EXIT)                    \
  X(HEAP_LOCK)                      \
  X(HEAP_UNLOCK)                    \
  X(EXIT_CLEAN)                     \
  X(THREAD_YIELD)                   \
  X(MONOTONIC_TIME)                 \
  X(THREAD_DETACH)                  \
  X(ADDR_WAIT)                      \
  X(ADDR_WAKE)                      \
  X(THREAD_SLEEP)                   \
  X(HARDWARE_CONCURRENCY)           \
  X(REALTIME_TIME)                  \
  X(FS_STATUS)                      \
  X(FS_OPEN_DIRECTORY)              \
  X(FS_READ_DIRECTORY)              \
  X(FS_CLOSE_DIRECTORY)             \
  X(FS_CREATE_DIRECTORY)            \
  X(FS_REMOVE)                      \
  X(FS_RENAME)                      \
  X(FS_CURRENT_PATH)                \
  X(FS_SET_CURRENT_PATH)            \
  X(FS_READ_SYMLINK)                \
  X(FS_CREATE_SYMLINK)              \
  X(FS_CREATE_HARD_LINK)            \
  X(FS_SET_PERMISSIONS)             \
  X(FS_RESIZE)                      \
  X(FS_SET_MODIFICATION_TIME)       \
  X(FS_SPACE)                       \
  X(FS_COPY_FILE)                   \
  X(FS_CANONICAL)                   \
  X(TZDB_VERSION)                   \
  X(TZDB_GENERATION)                \
  X(TZDB_RELOAD)                    \
  X(TZDB_CURRENT_ZONE)              \
  X(TZDB_ZONE_COUNT)                \
  X(TZDB_ZONE_NAME)                 \
  X(TZDB_LOCATE_ZONE)               \
  X(TZDB_SYS_INFO)                  \
  X(TZDB_LOCAL_INFO)                \
  X(TZDB_LEAP_COUNT)                \
  X(TZDB_LEAP_INFO)                 \
  X(RANDOM_BYTES)

#endif
