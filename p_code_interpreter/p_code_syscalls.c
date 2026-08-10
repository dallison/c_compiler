#include "p_code_syscalls.h"

#include <fcntl.h>
#include <sched.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "filesystem_host.h"
#include "loader_lifecycle.h"
#include "p_code_interpreter.h"

static int TranslateGuestOpenFlags(int guest_flags) {
  enum {
    kGuestOAccmode = 00000003,
    kGuestOCreat = 00000100,
    kGuestOExcl = 00000200,
    kGuestONoctty = 00000400,
    kGuestOTrunc = 00001000,
    kGuestOAppend = 00002000,
    kGuestONonblock = 00004000,
    kGuestOSync = 00010000,
    kGuestODirectory = 00200000,
    kGuestONofollow = 00400000,
    kGuestOCloexec = 02000000,
  };
  int host_flags = guest_flags & kGuestOAccmode;
  if (guest_flags & kGuestOCreat) host_flags |= O_CREAT;
  if (guest_flags & kGuestOExcl) host_flags |= O_EXCL;
  if (guest_flags & kGuestONoctty) host_flags |= O_NOCTTY;
  if (guest_flags & kGuestOTrunc) host_flags |= O_TRUNC;
  if (guest_flags & kGuestOAppend) host_flags |= O_APPEND;
  if (guest_flags & kGuestONonblock) host_flags |= O_NONBLOCK;
  if (guest_flags & kGuestOSync) host_flags |= O_SYNC;
  if (guest_flags & kGuestODirectory) host_flags |= O_DIRECTORY;
  if (guest_flags & kGuestONofollow) host_flags |= O_NOFOLLOW;
#ifdef O_CLOEXEC
  if (guest_flags & kGuestOCloexec) host_flags |= O_CLOEXEC;
#endif
  return host_flags;
}

static int64_t ClockMicroseconds(clockid_t clock_id, int64_t* result) {
  struct timespec now;
  if (result == NULL || clock_gettime(clock_id, &now) != 0) {
    return -1;
  }
  *result = (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
  return 0;
}

static int64_t Terminate(PCodeInterpreter* interpreter, int64_t status,
                         int clean) {
  if (clean && interpreter->loader != NULL) {
    LoaderLifecycleMarkExecutableFiniComplete(
        interpreter->loader, interpreter->loader->lifecycle);
  }
  interpreter->exit_code = (int)status;
  interpreter->running = false;
  return 0;
}

int64_t PCodeHandleSyscall(PCodeInterpreter* interpreter, int64_t number,
                           int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                           int64_t a4, int64_t a5) {
  (void)a3;
  (void)a4;
  (void)a5;
  switch (number) {
    case P_CODE_SYSCALL_OPEN:
      return open((const char*)(uintptr_t)a0,
                  TranslateGuestOpenFlags((int)a1), (mode_t)a2);
    case P_CODE_SYSCALL_CLOSE:
      return close((int)a0);
    case P_CODE_SYSCALL_WRITE:
      return write((int)a0, (const void*)(uintptr_t)a1, (size_t)a2);
    case P_CODE_SYSCALL_READ:
      return read((int)a0, (void*)(uintptr_t)a1, (size_t)a2);
    case P_CODE_SYSCALL_LSEEK:
      return lseek((int)a0, (off_t)a1, (int)a2);
    case P_CODE_SYSCALL_ABORT:
      abort();
    case P_CODE_SYSCALL_EXIT:
      return Terminate(interpreter, a0, 0);
    case P_CODE_SYSCALL_EXIT_CLEAN:
      return Terminate(interpreter, a0, 1);
    case P_CODE_SYSCALL_TIME:
      return (int64_t)time(NULL);
    case P_CODE_SYSCALL_CLOCK: {
      struct timespec now;
      if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return -1;
      return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
    }
    case P_CODE_SYSCALL_MONOTONIC_TIME:
      return ClockMicroseconds(CLOCK_MONOTONIC, (int64_t*)(uintptr_t)a0);
    case P_CODE_SYSCALL_REALTIME_TIME:
      return ClockMicroseconds(CLOCK_REALTIME, (int64_t*)(uintptr_t)a0);
    case P_CODE_SYSCALL_FS_STATUS:
      return DaveHostFilesystemGetStatus(
          (const char*)(uintptr_t)a0, (int)a1,
          (DaveHostFilesystemStat*)(uintptr_t)a2);
    case P_CODE_SYSCALL_FS_OPEN_DIRECTORY:
      return DaveHostFilesystemOpenDirectory((const char*)(uintptr_t)a0);
    case P_CODE_SYSCALL_FS_READ_DIRECTORY:
      return DaveHostFilesystemReadDirectory(
          (int)a0, (DaveHostFilesystemDirectoryEntry*)(uintptr_t)a1);
    case P_CODE_SYSCALL_FS_CLOSE_DIRECTORY:
      return DaveHostFilesystemCloseDirectory((int)a0);
    case P_CODE_SYSCALL_FS_CREATE_DIRECTORY:
      return DaveHostFilesystemCreateDirectory((const char*)(uintptr_t)a0,
                                               (uint32_t)a1);
    case P_CODE_SYSCALL_FS_REMOVE:
      return DaveHostFilesystemRemove((const char*)(uintptr_t)a0);
    case P_CODE_SYSCALL_FS_RENAME:
      return DaveHostFilesystemRename((const char*)(uintptr_t)a0,
                                      (const char*)(uintptr_t)a1);
    case P_CODE_SYSCALL_FS_CURRENT_PATH:
      return DaveHostFilesystemCurrentPath((char*)(uintptr_t)a0, (size_t)a1);
    case P_CODE_SYSCALL_FS_SET_CURRENT_PATH:
      return DaveHostFilesystemSetCurrentPath((const char*)(uintptr_t)a0);
    case P_CODE_SYSCALL_FS_READ_SYMLINK:
      return DaveHostFilesystemReadSymlink(
          (const char*)(uintptr_t)a0, (char*)(uintptr_t)a1, (size_t)a2);
    case P_CODE_SYSCALL_FS_CREATE_SYMLINK:
      return DaveHostFilesystemCreateSymlink((const char*)(uintptr_t)a0,
                                             (const char*)(uintptr_t)a1);
    case P_CODE_SYSCALL_FS_CREATE_HARD_LINK:
      return DaveHostFilesystemCreateHardLink((const char*)(uintptr_t)a0,
                                              (const char*)(uintptr_t)a1);
    case P_CODE_SYSCALL_FS_SET_PERMISSIONS:
      return DaveHostFilesystemSetPermissions(
          (const char*)(uintptr_t)a0, (uint32_t)a1, (int)a2);
    case P_CODE_SYSCALL_FS_RESIZE:
      return a1 == 0
                 ? -DAVE_HOST_EINVAL
                 : DaveHostFilesystemResize(
                       (const char*)(uintptr_t)a0,
                       *(const uint64_t*)(uintptr_t)a1);
    case P_CODE_SYSCALL_FS_SET_MODIFICATION_TIME:
      return a1 == 0
                 ? -DAVE_HOST_EINVAL
                 : DaveHostFilesystemSetModificationTime(
                       (const char*)(uintptr_t)a0,
                       *(const int64_t*)(uintptr_t)a1);
    case P_CODE_SYSCALL_FS_SPACE:
      return DaveHostFilesystemQuerySpace(
          (const char*)(uintptr_t)a0,
          (DaveHostFilesystemSpace*)(uintptr_t)a1);
    case P_CODE_SYSCALL_FS_COPY_FILE:
      return DaveHostFilesystemCopyFile((const char*)(uintptr_t)a0,
                                        (const char*)(uintptr_t)a1, (int)a2);
    case P_CODE_SYSCALL_FS_CANONICAL:
      return DaveHostFilesystemCanonical(
          (const char*)(uintptr_t)a0, (char*)(uintptr_t)a1, (size_t)a2);
    default:
      return -DAVE_HOST_ENOSYS;
  }
}

static int64_t ReadPackedInt(const unsigned char** cursor) {
  int32_t value;
  memcpy(&value, *cursor, sizeof(value));
  *cursor += sizeof(value);
  return value;
}

static int64_t ReadPackedLong(const unsigned char** cursor) {
  int64_t value;
  memcpy(&value, *cursor, sizeof(value));
  *cursor += sizeof(value);
  return value;
}

int64_t PCodeHandlePackedSyscall(PCodeInterpreter* interpreter,
                                 int64_t number, const void* arguments) {
  const unsigned char* cursor = arguments;
  int64_t a0 = 0;
  int64_t a1 = 0;
  int64_t a2 = 0;
  switch (number) {
    case P_CODE_SYSCALL_OPEN:
      a0 = ReadPackedLong(&cursor);
      a1 = ReadPackedInt(&cursor);
      a2 = ReadPackedInt(&cursor);
      break;
    case P_CODE_SYSCALL_CLOSE:
    case P_CODE_SYSCALL_EXIT:
    case P_CODE_SYSCALL_EXIT_CLEAN:
    case P_CODE_SYSCALL_FS_CLOSE_DIRECTORY:
      a0 = ReadPackedInt(&cursor);
      break;
    case P_CODE_SYSCALL_WRITE:
    case P_CODE_SYSCALL_READ:
      a0 = ReadPackedInt(&cursor);
      a1 = ReadPackedLong(&cursor);
      a2 = ReadPackedLong(&cursor);
      break;
    case P_CODE_SYSCALL_LSEEK:
      a0 = ReadPackedInt(&cursor);
      a1 = ReadPackedLong(&cursor);
      a2 = ReadPackedInt(&cursor);
      break;
    case P_CODE_SYSCALL_MONOTONIC_TIME:
    case P_CODE_SYSCALL_REALTIME_TIME:
    case P_CODE_SYSCALL_FS_OPEN_DIRECTORY:
    case P_CODE_SYSCALL_FS_REMOVE:
    case P_CODE_SYSCALL_FS_SET_CURRENT_PATH:
      a0 = ReadPackedLong(&cursor);
      break;
    case P_CODE_SYSCALL_FS_STATUS:
      a0 = ReadPackedLong(&cursor);
      a1 = ReadPackedInt(&cursor);
      a2 = ReadPackedLong(&cursor);
      break;
    case P_CODE_SYSCALL_FS_READ_DIRECTORY:
      a0 = ReadPackedInt(&cursor);
      a1 = ReadPackedLong(&cursor);
      break;
    case P_CODE_SYSCALL_FS_CREATE_DIRECTORY:
      a0 = ReadPackedLong(&cursor);
      a1 = ReadPackedInt(&cursor);
      break;
    case P_CODE_SYSCALL_FS_RENAME:
    case P_CODE_SYSCALL_FS_CREATE_SYMLINK:
    case P_CODE_SYSCALL_FS_CREATE_HARD_LINK:
    case P_CODE_SYSCALL_FS_RESIZE:
    case P_CODE_SYSCALL_FS_SET_MODIFICATION_TIME:
    case P_CODE_SYSCALL_FS_SPACE:
      a0 = ReadPackedLong(&cursor);
      a1 = ReadPackedLong(&cursor);
      break;
    case P_CODE_SYSCALL_FS_CURRENT_PATH:
      a0 = ReadPackedLong(&cursor);
      a1 = ReadPackedLong(&cursor);
      break;
    case P_CODE_SYSCALL_FS_READ_SYMLINK:
    case P_CODE_SYSCALL_FS_CANONICAL:
      a0 = ReadPackedLong(&cursor);
      a1 = ReadPackedLong(&cursor);
      a2 = ReadPackedLong(&cursor);
      break;
    case P_CODE_SYSCALL_FS_SET_PERMISSIONS:
      a0 = ReadPackedLong(&cursor);
      a1 = ReadPackedInt(&cursor);
      a2 = ReadPackedInt(&cursor);
      break;
    case P_CODE_SYSCALL_FS_COPY_FILE:
      a0 = ReadPackedLong(&cursor);
      a1 = ReadPackedLong(&cursor);
      a2 = ReadPackedInt(&cursor);
      break;
    default:
      break;
  }
  return PCodeHandleSyscall(interpreter, number, a0, a1, a2, 0, 0, 0);
}
