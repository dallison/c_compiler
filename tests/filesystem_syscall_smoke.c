#include <stdint.h>
#include <stddef.h>
#include <syscall.h>

typedef struct {
  uint64_t device;
  uint64_t inode;
  uint64_t size;
  uint64_t hard_link_count;
  int64_t access_time_ns;
  int64_t modification_time_ns;
  int64_t status_change_time_ns;
  uint32_t mode;
  uint32_t reserved;
} FilesystemStatus;

typedef struct {
  uint64_t capacity;
  uint64_t free;
  uint64_t available;
} FilesystemSpace;

typedef struct {
  uint32_t type;
  uint32_t reserved;
  char name[1024];
} FilesystemDirectoryEntry;

int main(void) {
  char current_path[1024];
  char canonical_path[1024];
  char first_byte = 0;
  FilesystemStatus status;
  FilesystemSpace space;
  FilesystemDirectoryEntry entry;
  int64_t monotonic = 0;
  int64_t realtime = 0;

  if (syscall(SYS_FS_CURRENT_PATH, current_path, sizeof(current_path)) != 0)
    return 1;
  if (current_path[0] == '\0') return 2;
  if (syscall(SYS_FS_STATUS, current_path, 1, &status) != 0) return 3;
  if ((status.mode & 0170000u) != 0040000u) return 4;
  if (syscall(SYS_FS_SPACE, current_path, &space) != 0) return 5;
  if (space.capacity == 0 || space.free > space.capacity) return 6;
  long directory = syscall(SYS_FS_OPEN_DIRECTORY, current_path);
  if (directory <= 0) return 7;
  long directory_result =
      syscall(SYS_FS_READ_DIRECTORY, (int)directory, &entry);
  if (directory_result < 0) return 8;
  if (directory_result > 0 && entry.name[0] == '\0') return 8;
  if (syscall(SYS_FS_CLOSE_DIRECTORY, (int)directory) != 0) return 9;
  if (syscall(SYS_FS_CANONICAL, current_path, canonical_path,
              sizeof(canonical_path)) != 0)
    return 10;
  if (canonical_path[0] == '\0') return 11;

  long fd =
      syscall(SYS_OPEN, "tests/filesystem_syscall_smoke.c", 0, 0);
  if (fd < 0) return 12;
  if (syscall(SYS_READ, (int)fd, &first_byte, (size_t)1) != 1) return 13;
  if (first_byte != '#') return 14;
  if (syscall(SYS_LSEEK, (int)fd, (long)0, 0) != 0) return 15;
  if (syscall(SYS_CLOSE, (int)fd) != 0) return 16;

  if (syscall(SYS_MONOTONIC_TIME, &monotonic) != 0) return 17;
  if (syscall(SYS_REALTIME_TIME, &realtime) != 0) return 18;
  if (monotonic <= 0 || realtime <= 0) return 19;
  return 0;
}
