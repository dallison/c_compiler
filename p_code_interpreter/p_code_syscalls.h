#ifndef p_code_syscalls_h
#define p_code_syscalls_h

#include <stdint.h>
#include "../libc/include/davecc_guest_syscalls.h"

struct PCodeInterpreter;

enum {
  P_CODE_SYSCALL_OPEN = 2,
  P_CODE_SYSCALL_CLOSE = 3,
  P_CODE_SYSCALL_WRITE = 4,
  P_CODE_SYSCALL_READ = 5,
  P_CODE_SYSCALL_LSEEK = 7,
  P_CODE_SYSCALL_ABORT = 11,
  P_CODE_SYSCALL_EXIT = 12,
  P_CODE_SYSCALL_TIME = 13,
  P_CODE_SYSCALL_CLOCK = 14,
  P_CODE_SYSCALL_EXIT_CLEAN = 22,
  P_CODE_SYSCALL_MONOTONIC_TIME = 24,
  P_CODE_SYSCALL_REALTIME_TIME = 30,
  P_CODE_SYSCALL_FS_STATUS = 31,
  P_CODE_SYSCALL_FS_OPEN_DIRECTORY = 32,
  P_CODE_SYSCALL_FS_READ_DIRECTORY = 33,
  P_CODE_SYSCALL_FS_CLOSE_DIRECTORY = 34,
  P_CODE_SYSCALL_FS_CREATE_DIRECTORY = 35,
  P_CODE_SYSCALL_FS_REMOVE = 36,
  P_CODE_SYSCALL_FS_RENAME = 37,
  P_CODE_SYSCALL_FS_CURRENT_PATH = 38,
  P_CODE_SYSCALL_FS_SET_CURRENT_PATH = 39,
  P_CODE_SYSCALL_FS_READ_SYMLINK = 40,
  P_CODE_SYSCALL_FS_CREATE_SYMLINK = 41,
  P_CODE_SYSCALL_FS_CREATE_HARD_LINK = 42,
  P_CODE_SYSCALL_FS_SET_PERMISSIONS = 43,
  P_CODE_SYSCALL_FS_RESIZE = 44,
  P_CODE_SYSCALL_FS_SET_MODIFICATION_TIME = 45,
  P_CODE_SYSCALL_FS_SPACE = 46,
  P_CODE_SYSCALL_FS_COPY_FILE = 47,
  P_CODE_SYSCALL_FS_CANONICAL = 48,
  P_CODE_SYSCALL_TZDB_VERSION = 49,
  P_CODE_SYSCALL_TZDB_GENERATION = 50,
  P_CODE_SYSCALL_TZDB_RELOAD = 51,
  P_CODE_SYSCALL_TZDB_CURRENT_ZONE = 52,
  P_CODE_SYSCALL_TZDB_ZONE_COUNT = 53,
  P_CODE_SYSCALL_TZDB_ZONE_NAME = 54,
  P_CODE_SYSCALL_TZDB_LOCATE_ZONE = 55,
  P_CODE_SYSCALL_TZDB_SYS_INFO = 56,
  P_CODE_SYSCALL_TZDB_LOCAL_INFO = 57,
  P_CODE_SYSCALL_TZDB_LEAP_COUNT = 58,
  P_CODE_SYSCALL_TZDB_LEAP_INFO = 59,
  P_CODE_SYSCALL_RANDOM_BYTES = 60,
  P_CODE_SYSCALL_FS_DESCRIPTOR_STATUS = 61,
  P_CODE_SYSCALL_ENVIRONMENT_VALUE = 62,
};

#define P_CODE_VALIDATE_DAVE_SYSCALL(name)                              \
  typedef char p_code_dave_syscall_##name[                             \
      P_CODE_SYSCALL_##name == DAVE_SYS_##name ? 1 : -1];
#define P_CODE_VALIDATE_DAVE_BASE_SYSCALLS(X) \
  X(OPEN)                                     \
  X(CLOSE)                                    \
  X(WRITE)                                    \
  X(READ)                                     \
  X(LSEEK)                                    \
  X(ABORT)                                    \
  X(EXIT)                                     \
  X(TIME)                                     \
  X(CLOCK)                                    \
  X(EXIT_CLEAN)                               \
  X(MONOTONIC_TIME)                           \
  X(REALTIME_TIME)
P_CODE_VALIDATE_DAVE_BASE_SYSCALLS(P_CODE_VALIDATE_DAVE_SYSCALL)
#undef P_CODE_VALIDATE_DAVE_BASE_SYSCALLS
#undef P_CODE_VALIDATE_DAVE_SYSCALL

int64_t PCodeHandleSyscall(struct PCodeInterpreter* interpreter,
                           int64_t number, int64_t a0, int64_t a1, int64_t a2,
                           int64_t a3, int64_t a4, int64_t a5);
int64_t PCodeHandlePackedSyscall(struct PCodeInterpreter* interpreter,
                                 int64_t number, const void* arguments);

#endif
