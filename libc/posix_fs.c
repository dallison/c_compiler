#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syscall.h>
#include <unistd.h>
#include <time.h>

#include "posix_fs.h"

enum {
  kFsStatus = 1,
  kFsRemove = 6,
  kFsCurrentPath = 8,
  kFsSetCurrentPath = 9,
  kFsSetPermissions = 13,
  kFsCanonical = 18,
  kFsDescriptorStatus = 19,
};

static long FsCall(int operation, intptr_t first, intptr_t second,
                   intptr_t third) {
#if defined(__DAVECC_NATIVE_LINUX__)
  return (long)__davecc_linux_fs_service(operation, first, second, third);
#else
  switch (operation) {
    case kFsStatus:
      return syscall(SYS_FS_STATUS, first, second, third);
    case kFsRemove:
      return syscall(SYS_FS_REMOVE, first);
    case kFsCurrentPath:
      return syscall(SYS_FS_CURRENT_PATH, first, second);
    case kFsSetCurrentPath:
      return syscall(SYS_FS_SET_CURRENT_PATH, first);
    case kFsSetPermissions:
      return syscall(SYS_FS_SET_PERMISSIONS, first, second, third);
    case kFsCanonical:
      return syscall(SYS_FS_CANONICAL, first, second, third);
    case kFsDescriptorStatus:
      return syscall(SYS_FS_DESCRIPTOR_STATUS, first, second);
    default:
      return -ENOSYS;
  }
#endif
}

static int Failed(long result) {
  if (result >= 0) return 0;
  errno = (int)-result;
  return 1;
}

static void CopyStatus(struct stat* output, const DaveWireStatus* input) {
  memset(output, 0, sizeof(*output));
  output->st_dev = input->device;
  output->__st_ino = input->inode;
  output->st_ino = input->inode;
  output->st_mode = input->mode;
  output->st_nlink = (uint32_t)input->hard_link_count;
  output->st_size = (int64_t)input->size;
  output->st_blksize = 4096;
  output->st_blocks = (input->size + 511) / 512;
  output->st_atime = (uint64_t)(input->access_time_ns / 1000000000LL);
  output->st_atime_nsec = (uint64_t)(input->access_time_ns % 1000000000LL);
  output->st_mtime = (uint64_t)(input->modification_time_ns / 1000000000LL);
  output->st_mtime_nsec =
      (uint64_t)(input->modification_time_ns % 1000000000LL);
  output->st_ctime = (uint64_t)(input->status_change_time_ns / 1000000000LL);
  output->st_ctime_nsec =
      (uint64_t)(input->status_change_time_ns % 1000000000LL);
}

static int Status(const char* path, struct stat* output, int follow) {
  if (path == NULL || output == NULL) {
    errno = EINVAL;
    return -1;
  }
  DaveWireStatus wire;
  long result = FsCall(kFsStatus, (intptr_t)path, follow, (intptr_t)&wire);
  if (Failed(result)) return -1;
  CopyStatus(output, &wire);
  return 0;
}

int stat(const char* path, struct stat* output) {
  return Status(path, output, 1);
}

int lstat(const char* path, struct stat* output) {
  return Status(path, output, 0);
}

int fstat(int fd, struct stat* output) {
  if (output == NULL) {
    errno = EINVAL;
    return -1;
  }
  DaveWireStatus wire;
  long result =
      FsCall(kFsDescriptorStatus, (intptr_t)fd, (intptr_t)&wire, 0);
  if (Failed(result)) return -1;
  CopyStatus(output, &wire);
  return 0;
}

char* getcwd(char* buffer, size_t size) {
  if (buffer == NULL || size == 0) {
    errno = EINVAL;
    return NULL;
  }
  long result = FsCall(kFsCurrentPath, (intptr_t)buffer, (intptr_t)size, 0);
  return Failed(result) ? NULL : buffer;
}

int chdir(const char* path) {
  long result = FsCall(kFsSetCurrentPath, (intptr_t)path, 0, 0);
  return Failed(result) ? -1 : 0;
}

int access(const char* path, int mode) {
  if ((mode & ~(R_OK | W_OK | X_OK)) != 0) {
    errno = EINVAL;
    return -1;
  }
#if defined(__DAVECC_NATIVE_LINUX__)
  return (int)syscall(SYS_faccessat, -100, path, mode, 0, 0, 0);
#else
  struct stat value;
  if (stat(path, &value) != 0) return -1;
  if (mode == F_OK) return 0;
  uint32_t allowed =
      ((value.st_mode >> 6) | (value.st_mode >> 3) | value.st_mode) & 7;
  if ((mode & R_OK) && !(allowed & 4)) goto denied;
  if ((mode & W_OK) && !(allowed & 2)) goto denied;
  if ((mode & X_OK) && !(allowed & 1)) goto denied;
  return 0;
denied:
  errno = EACCES;
  return -1;
#endif
}

char* realpath(const char* restrict path, char* restrict resolved) {
  int allocated = 0;
  if (resolved == NULL) {
    resolved = malloc(PATH_MAX);
    if (resolved == NULL) return NULL;
    allocated = 1;
  }
  long result =
      FsCall(kFsCanonical, (intptr_t)path, (intptr_t)resolved, PATH_MAX);
  if (Failed(result)) {
    if (allocated) free(resolved);
    return NULL;
  }
  return resolved;
}

int remove(const char* path) {
  long result = FsCall(kFsRemove, (intptr_t)path, 0, 0);
  return Failed(result) ? -1 : 0;
}

int chmod(const char* path, uint32_t mode) {
  long result =
      FsCall(kFsSetPermissions, (intptr_t)path, (intptr_t)mode, 0);
  return Failed(result) ? -1 : 0;
}

int mkstemp(char* template_name) {
  if (template_name == NULL) {
    errno = EINVAL;
    return -1;
  }
  size_t length = strlen(template_name);
  size_t start = length;
  while (start != 0 && template_name[start - 1] == 'X') --start;
  if (length - start < 6) {
    errno = EINVAL;
    return -1;
  }

  unsigned long value =
      (unsigned long)time(NULL) ^ (unsigned long)(uintptr_t)template_name;
  static const char alphabet[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (int attempt = 0; attempt != 100; ++attempt) {
    unsigned long current = value + (unsigned long)attempt * 1103515245UL;
    for (size_t index = start; index != length; ++index) {
      current = current * 1664525UL + 1013904223UL;
      template_name[index] = alphabet[current % (sizeof(alphabet) - 1)];
    }
    int fd = open(template_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) return fd;
    if (errno != EEXIST) return -1;
  }
  errno = EEXIST;
  return -1;
}
