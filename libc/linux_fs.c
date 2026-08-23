#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/syscall.h>
#include <syscall.h>

#include "posix_fs.h"

#define AT_FDCWD (-100)
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR 0x200
#define AT_EMPTY_PATH 0x1000
#define STATX_BASIC_STATS 0x7ff
#define O_PATH 0x200000
#define O_CLOEXEC 0x80000

typedef struct {
  int64_t seconds;
  uint32_t nanoseconds;
  int32_t reserved;
} LinuxStatxTimestamp;

typedef struct {
  uint32_t mask;
  uint32_t block_size;
  uint64_t attributes;
  uint32_t link_count;
  uint32_t uid;
  uint32_t gid;
  uint16_t mode;
  uint16_t reserved0;
  uint64_t inode;
  uint64_t size;
  uint64_t blocks;
  uint64_t attributes_mask;
  LinuxStatxTimestamp access_time;
  LinuxStatxTimestamp creation_time;
  LinuxStatxTimestamp status_change_time;
  LinuxStatxTimestamp modification_time;
  uint32_t device_major;
  uint32_t device_minor;
  uint32_t rdev_major;
  uint32_t rdev_minor;
  uint64_t mount_id;
  uint32_t dio_memory_alignment;
  uint32_t dio_offset_alignment;
  uint64_t spare[12];
} LinuxStatx;

typedef struct {
  uint32_t type;
  uint32_t reserved;
  char name[1024];
} DaveWireDirectoryEntry;

typedef struct {
  uint64_t capacity;
  uint64_t free;
  uint64_t available;
} DaveWireSpace;

typedef struct {
  uint64_t inode;
  int64_t offset;
  uint16_t record_length;
  uint8_t type;
  char name[256];
} LinuxDirectoryEntry;

typedef struct {
  long type;
  long block_size;
  uint64_t blocks;
  uint64_t blocks_free;
  uint64_t blocks_available;
  uint64_t files;
  uint64_t files_free;
  int filesystem_id[2];
  long name_length;
  long fragment_size;
  long flags;
  long spare[4];
} LinuxStatfs;

static int64_t ServiceResult(long result) {
  return result == -1 ? -(int64_t)errno : (int64_t)result;
}

static int64_t CopyFile(const char* source, const char* destination,
                        int mode) {
  long input = syscall(SYS_openat, AT_FDCWD, source, O_RDONLY | O_CLOEXEC, 0);
  if (input == -1) {
    return -(int64_t)errno;
  }
  long output = syscall(SYS_openat, AT_FDCWD, destination,
                        O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
                        mode == 0 ? 0666 : mode);
  if (output == -1) {
    int error = errno;
    syscall(SYS_close, input);
    return -(int64_t)error;
  }
  char buffer[4096];
  int64_t result = 0;
  for (;;) {
    long count = syscall(SYS_read, input, buffer, sizeof(buffer));
    if (count == 0) {
      break;
    }
    if (count == -1) {
      result = -(int64_t)errno;
      break;
    }
    long written = 0;
    while (written < count) {
      long part = syscall(SYS_write, output, buffer + written, count - written);
      if (part == -1) {
        result = -(int64_t)errno;
        break;
      }
      written += part;
    }
    if (result != 0) {
      break;
    }
  }
  syscall(SYS_close, input);
  syscall(SYS_close, output);
  return result;
}

static int64_t CanonicalPath(const char* path, char* output, size_t capacity) {
  long fd = syscall(SYS_openat, AT_FDCWD, path, O_PATH | O_CLOEXEC, 0);
  if (fd == -1) {
    return -(int64_t)errno;
  }
  char descriptor_path[64] = "/proc/self/fd/";
  char digits[24];
  int digit_count = 0;
  long value = fd;
  do {
    digits[digit_count++] = (char)('0' + value % 10);
    value /= 10;
  } while (value != 0);
  size_t prefix = strlen(descriptor_path);
  for (int i = digit_count - 1; i >= 0; --i) {
    descriptor_path[prefix++] = digits[i];
  }
  descriptor_path[prefix] = '\0';
  long length = syscall(SYS_readlinkat, AT_FDCWD, descriptor_path, output,
                        capacity == 0 ? 0 : capacity - 1);
  int error = errno;
  syscall(SYS_close, fd);
  if (length == -1) {
    return -(int64_t)error;
  }
  if (capacity == 0 || (size_t)length >= capacity) {
    return -ERANGE;
  }
  output[length] = '\0';
  return length;
}

static void CopyStatx(DaveWireStatus* output, const LinuxStatx* value) {
  output->device = ((uint64_t)value->device_major << 32) | value->device_minor;
  output->inode = value->inode;
  output->size = value->size;
  output->hard_link_count = value->link_count;
  output->access_time_ns =
      value->access_time.seconds * 1000000000LL +
      value->access_time.nanoseconds;
  output->modification_time_ns =
      value->modification_time.seconds * 1000000000LL +
      value->modification_time.nanoseconds;
  output->status_change_time_ns =
      value->status_change_time.seconds * 1000000000LL +
      value->status_change_time.nanoseconds;
  output->mode = value->mode;
  output->reserved = 0;
}

int64_t __davecc_linux_fs_service(int operation, intptr_t first,
                                  intptr_t second, intptr_t third) {
  switch (operation) {
    case 1: {
      LinuxStatx statx_value;
      int flags = second != 0 ? 0 : AT_SYMLINK_NOFOLLOW;
      long result = syscall(SYS_statx, AT_FDCWD, (const char*)first, flags,
                            STATX_BASIC_STATS, &statx_value);
      if (result == -1) return -(int64_t)errno;
      CopyStatx((DaveWireStatus*)third, &statx_value);
      return 0;
    }
    case 2:
      return ServiceResult(syscall(SYS_openat, AT_FDCWD, (const char*)first,
                                   O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0));
    case 3: {
      LinuxDirectoryEntry entry;
      for (;;) {
        long count = syscall(SYS_getdents64, first, &entry, sizeof(entry));
        if (count <= 0) return ServiceResult(count);
        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) {
          continue;
        }
        DaveWireDirectoryEntry* output = (DaveWireDirectoryEntry*)second;
        output->type = entry.type;
        output->reserved = 0;
        size_t length = strlen(entry.name);
        if (length >= sizeof(output->name)) length = sizeof(output->name) - 1;
        memcpy(output->name, entry.name, length);
        output->name[length] = '\0';
        return 1;
      }
    }
    case 4:
      return ServiceResult(syscall(SYS_close, first));
    case 5:
      return ServiceResult(
          syscall(SYS_mkdirat, AT_FDCWD, (const char*)first, second));
    case 6: {
      long result = syscall(SYS_unlinkat, AT_FDCWD, (const char*)first, 0);
      if (result == -1 && (errno == EISDIR || errno == EPERM)) {
        result =
            syscall(SYS_unlinkat, AT_FDCWD, (const char*)first, AT_REMOVEDIR);
      }
      return ServiceResult(result);
    }
    case 7:
      return ServiceResult(syscall(SYS_renameat, AT_FDCWD, (const char*)first,
                                   AT_FDCWD, (const char*)second));
    case 8:
      return ServiceResult(syscall(SYS_getcwd, (char*)first, second));
    case 9:
      return ServiceResult(syscall(SYS_chdir, (const char*)first));
    case 10:
      return ServiceResult(syscall(SYS_readlinkat, AT_FDCWD,
                                   (const char*)first, (char*)second, third));
    case 11:
      return ServiceResult(syscall(SYS_symlinkat, (const char*)first, AT_FDCWD,
                                   (const char*)second));
    case 12:
      return ServiceResult(syscall(SYS_linkat, AT_FDCWD, (const char*)first,
                                   AT_FDCWD, (const char*)second, 0));
    case 13:
      return ServiceResult(syscall(SYS_fchmodat, AT_FDCWD, (const char*)first,
                                   second, third ? AT_SYMLINK_NOFOLLOW : 0));
    case 14:
      return ServiceResult(
          syscall(SYS_truncate, (const char*)first, *(uint64_t*)second));
    case 15: {
      int64_t nanoseconds = *(int64_t*)second;
      struct {
        int64_t seconds;
        int64_t nanoseconds;
      } times[2] = {{0, 0x3fffffff}, {nanoseconds / 1000000000LL,
                                    nanoseconds % 1000000000LL}};
      return ServiceResult(syscall(SYS_utimensat, AT_FDCWD, (const char*)first,
                                   times, 0));
    }
    case 16: {
#if defined(__arm__)
      return -ENOSYS;
#else
      LinuxStatfs value;
      long result = syscall(SYS_statfs, (const char*)first, &value);
      if (result == -1) return -(int64_t)errno;
      DaveWireSpace* output = (DaveWireSpace*)second;
      uint64_t size = value.fragment_size != 0
                          ? (uint64_t)value.fragment_size
                          : (uint64_t)value.block_size;
      output->capacity = value.blocks * size;
      output->free = value.blocks_free * size;
      output->available = value.blocks_available * size;
      return 0;
#endif
    }
    case 17:
      return CopyFile((const char*)first, (const char*)second, (int)third);
    case 18:
      return CanonicalPath((const char*)first, (char*)second, third);
    case 19: {
      LinuxStatx statx_value;
      long result = syscall(SYS_statx, (int)first, "", AT_EMPTY_PATH,
                            STATX_BASIC_STATS, &statx_value);
      if (result == -1) return -(int64_t)errno;
      CopyStatx((DaveWireStatus*)second, &statx_value);
      return 0;
    }
    default:
      return -ENOSYS;
  }
}
