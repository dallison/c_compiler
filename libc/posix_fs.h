#ifndef davecc_posix_fs_h
#define davecc_posix_fs_h

#include <stdint.h>

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
} DaveWireStatus;

int64_t __davecc_linux_fs_service(int operation, intptr_t first,
                                  intptr_t second, intptr_t third);

#endif
