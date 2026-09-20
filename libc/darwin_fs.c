// libSystem-backed filesystem service for AArch64 Darwin.  Same operation
// numbers and DaveWireStatus layout as linux_fs.c so filesystem.cc can share
// the host-service path.  Darwin errno and struct layouts are translated;
// directory streams use a small handle table because DIR* does not fit in
// the int handle filesystem.cc stores.

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "posix_fs.h"

enum {
  kDarwinEPERM = 1,
  kDarwinENOENT = 2,
  kDarwinESRCH = 3,
  kDarwinEINTR = 4,
  kDarwinEIO = 5,
  kDarwinENOEXEC = 8,
  kDarwinEBADF = 9,
  kDarwinEDEADLK = 11,
  kDarwinENOMEM = 12,
  kDarwinEACCES = 13,
  kDarwinEBUSY = 16,
  kDarwinEEXIST = 17,
  kDarwinEXDEV = 18,
  kDarwinENODEV = 19,
  kDarwinENOTDIR = 20,
  kDarwinEISDIR = 21,
  kDarwinEINVAL = 22,
  kDarwinENFILE = 23,
  kDarwinEMFILE = 24,
  kDarwinENOTTY = 25,
  kDarwinEFBIG = 27,
  kDarwinENOSPC = 28,
  kDarwinESPIPE = 29,
  kDarwinEROFS = 30,
  kDarwinERANGE = 34,
  kDarwinEAGAIN = 35,
  kDarwinENOTSUP = 45,
  kDarwinELOOP = 62,
  kDarwinENAMETOOLONG = 63,
  kDarwinENOTEMPTY = 66,
  kDarwinENOSYS = 78,
  kDarwinEOVERFLOW = 84,
};

enum {
  kORdonly = 0,
  kOWronly = 1,
  kOCreat = 0x200,
  kOTrunc = 0x400,
  kOCloexec = 0x1000000,
};

enum { kDirHandleCap = 64 };
enum { kDarwinPathMax = 1024 };
enum { kDarwinStatfsSize = 2168 };

typedef struct {
  int64_t seconds;
  long nanoseconds;
} DarwinTimespec;

typedef struct {
  int32_t device;
  uint16_t mode;
  uint16_t hard_link_count;
  uint64_t inode;
  uint32_t uid;
  uint32_t gid;
  int32_t rdev;
  int32_t pad0;
  DarwinTimespec access_time;
  DarwinTimespec modification_time;
  DarwinTimespec status_change_time;
  DarwinTimespec birth_time;
  int64_t size;
  int64_t blocks;
  int32_t block_size;
  uint32_t flags;
  uint32_t gen;
  int32_t lspare;
  int64_t qspare[2];
} DarwinStat;

typedef struct {
  uint64_t inode;
  uint64_t seek_offset;
  uint16_t record_length;
  uint16_t name_length;
  uint8_t type;
  char name[1024];
  uint8_t pad[3];
} DarwinDirent;

typedef struct {
  uint32_t block_size;
  int32_t io_size;
  uint64_t blocks;
  uint64_t blocks_free;
  uint64_t blocks_available;
  uint64_t files;
  uint64_t files_free;
  uint8_t rest[kDarwinStatfsSize - 48];
} DarwinStatfs;

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

extern int* __error(void);
int stat(const char* path, DarwinStat* value);
int lstat(const char* path, DarwinStat* value);
int fstat(int fd, DarwinStat* value);
int mkdir(const char* path, unsigned short mode);
int unlink(const char* path);
int rmdir(const char* path);
int rename(const char* from, const char* to);
char* getcwd(char* buffer, unsigned long capacity);
int chdir(const char* path);
long readlink(const char* path, char* buffer, unsigned long capacity);
int symlink(const char* target, const char* link_path);
int link(const char* target, const char* link_path);
int chmod(const char* path, unsigned short mode);
int lchmod(const char* path, unsigned short mode);
int truncate(const char* path, long long length);
int utimes(const char* path, const void* times);
int statfs(const char* path, DarwinStatfs* value);
char* realpath(const char* path, char* resolved);
int open(const char* path, int flags, ...);
int close(int fd);
long read(int fd, void* buffer, unsigned long size);
long write(int fd, const void* buffer, unsigned long size);
void* opendir(const char* path);
DarwinDirent* readdir(void* directory);
int closedir(void* directory);

static void* g_directories[kDirHandleCap];

static int64_t GuestFromDarwinErrno(int darwin_errno) {
  switch (darwin_errno) {
    case kDarwinENOENT:
      return -ENOENT;
    case kDarwinENOMEM:
      return -ENOMEM;
    case kDarwinEACCES:
      return -EACCES;
    case kDarwinENODEV:
      return -ENODEV;
    case kDarwinEMFILE:
      return -EMFILE;
    case kDarwinEBUSY:
      return -EBUSY;
    case kDarwinEINVAL:
      return -EINVAL;
    case kDarwinENOSPC:
      return -ENOSPC;
    case kDarwinEEXIST:
      return -EEXIST;
    case kDarwinEAGAIN:
      return -EAGAIN;
    case kDarwinEIO:
      return -EIO;
    case kDarwinEINTR:
      return -EINTR;
    case kDarwinENOSYS:
      return -ENOSYS;
    case kDarwinESPIPE:
      return -ESPIPE;
    case kDarwinERANGE:
      return -ERANGE;
    case kDarwinEBADF:
      return -EBADF;
    case kDarwinENOEXEC:
      return -ENOEXEC;
    case kDarwinEPERM:
      return -EPERM;
    case kDarwinESRCH:
      return -ESRCH;
    case kDarwinEDEADLK:
      return -EDEADLK;
    case kDarwinENOTDIR:
      return -ENOTDIR;
    case kDarwinEISDIR:
      return -EISDIR;
    case kDarwinENAMETOOLONG:
      return -ENAMETOOLONG;
    case kDarwinENOTEMPTY:
      return -ENOTEMPTY;
    case kDarwinELOOP:
      return -ELOOP;
    case kDarwinEROFS:
      return -EROFS;
    case kDarwinEXDEV:
      return -EXDEV;
    case kDarwinENOTSUP:
      return -ENOTSUP;
    case kDarwinEFBIG:
      return -EFBIG;
    case kDarwinENFILE:
      return -ENFILE;
    case kDarwinEOVERFLOW:
      return -EOVERFLOW;
    case kDarwinENOTTY:
      return -ENOTTY;
    default:
      return -EUNKNOWN;
  }
}

static int64_t Fail(void) { return GuestFromDarwinErrno(*__error()); }

static int64_t ServiceResult(int result) { return result == -1 ? Fail() : 0; }

static int64_t AllocDirectory(void* directory) {
  for (int index = 0; index < kDirHandleCap; ++index) {
    if (g_directories[index] == NULL) {
      g_directories[index] = directory;
      return index + 1;
    }
  }
  closedir(directory);
  return -EMFILE;
}

static void* DirectoryFromHandle(intptr_t handle) {
  if (handle < 1 || handle > kDirHandleCap) {
    return NULL;
  }
  return g_directories[handle - 1];
}

static void CopyStat(DaveWireStatus* output, const DarwinStat* value) {
  output->device = (uint64_t)(uint32_t)value->device;
  output->inode = value->inode;
  output->size = (uint64_t)value->size;
  output->hard_link_count = value->hard_link_count;
  output->access_time_ns =
      value->access_time.seconds * 1000000000LL + value->access_time.nanoseconds;
  output->modification_time_ns = value->modification_time.seconds * 1000000000LL +
                                 value->modification_time.nanoseconds;
  output->status_change_time_ns =
      value->status_change_time.seconds * 1000000000LL +
      value->status_change_time.nanoseconds;
  output->mode = value->mode;
  output->reserved = 0;
}

static int64_t CopyFile(const char* source, const char* destination, int mode) {
  int input = open(source, kORdonly | kOCloexec, 0);
  if (input < 0) {
    return Fail();
  }
  int output = open(destination, kOWronly | kOCreat | kOTrunc | kOCloexec,
                    mode == 0 ? 0666 : mode);
  if (output < 0) {
    int64_t error = Fail();
    close(input);
    return error;
  }
  char buffer[4096];
  int64_t result = 0;
  for (;;) {
    long count = read(input, buffer, sizeof(buffer));
    if (count == 0) {
      break;
    }
    if (count < 0) {
      result = Fail();
      break;
    }
    long written = 0;
    while (written < count) {
      long part = write(output, buffer + written, (unsigned long)(count - written));
      if (part < 0) {
        result = Fail();
        break;
      }
      written += part;
    }
    if (result != 0) {
      break;
    }
  }
  close(input);
  close(output);
  return result;
}

static int64_t CanonicalPath(const char* path, char* output, size_t capacity) {
  char resolved[kDarwinPathMax];
  if (realpath(path, resolved) == NULL) {
    return Fail();
  }
  size_t length = strlen(resolved);
  if (capacity == 0 || length + 1 > capacity) {
    return -ERANGE;
  }
  memcpy(output, resolved, length + 1);
  return (int64_t)length;
}

int64_t __davecc_linux_fs_service(int operation, intptr_t first,
                                  intptr_t second, intptr_t third) {
  switch (operation) {
    case 1: {
      DarwinStat value;
      int result = second != 0 ? stat((const char*)first, &value)
                               : lstat((const char*)first, &value);
      if (result == -1) {
        return Fail();
      }
      CopyStat((DaveWireStatus*)third, &value);
      return 0;
    }
    case 2: {
      void* directory = opendir((const char*)first);
      if (directory == NULL) {
        return Fail();
      }
      return AllocDirectory(directory);
    }
    case 3: {
      void* directory = DirectoryFromHandle(first);
      if (directory == NULL) {
        return -EBADF;
      }
      for (;;) {
        DarwinDirent* entry = readdir(directory);
        if (entry == NULL) {
          return 0;
        }
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
          continue;
        }
        DaveWireDirectoryEntry* output = (DaveWireDirectoryEntry*)second;
        output->type = entry->type;
        output->reserved = 0;
        size_t length = strlen(entry->name);
        if (length >= sizeof(output->name)) {
          length = sizeof(output->name) - 1;
        }
        memcpy(output->name, entry->name, length);
        output->name[length] = '\0';
        return 1;
      }
    }
    case 4: {
      void* directory = DirectoryFromHandle(first);
      if (directory == NULL) {
        return -EBADF;
      }
      g_directories[first - 1] = NULL;
      return ServiceResult(closedir(directory));
    }
    case 5:
      return ServiceResult(mkdir((const char*)first, (unsigned short)second));
    case 6: {
      if (unlink((const char*)first) == 0) {
        return 0;
      }
      int first_error = *__error();
      if (first_error == kDarwinEPERM || first_error == kDarwinEISDIR) {
        if (rmdir((const char*)first) == 0) {
          return 0;
        }
      }
      return GuestFromDarwinErrno(*__error());
    }
    case 7:
      return ServiceResult(rename((const char*)first, (const char*)second));
    case 8:
      return getcwd((char*)first, (unsigned long)second) == NULL ? Fail() : 0;
    case 9:
      return ServiceResult(chdir((const char*)first));
    case 10: {
      long length =
          readlink((const char*)first, (char*)second, (unsigned long)third);
      return length < 0 ? Fail() : length;
    }
    case 11:
      return ServiceResult(symlink((const char*)first, (const char*)second));
    case 12:
      return ServiceResult(link((const char*)first, (const char*)second));
    case 13:
      return ServiceResult(
          third ? lchmod((const char*)first, (unsigned short)second)
                : chmod((const char*)first, (unsigned short)second));
    case 14:
      return ServiceResult(
          truncate((const char*)first, (long long)*(uint64_t*)second));
    case 15: {
      DarwinStat current;
      if (stat((const char*)first, &current) == -1) {
        return Fail();
      }
      int64_t nanoseconds = *(int64_t*)second;
      struct {
        int64_t seconds;
        int32_t microseconds;
        int32_t pad;
      } timevals[2];
      timevals[0].seconds = current.access_time.seconds;
      timevals[0].microseconds =
          (int32_t)(current.access_time.nanoseconds / 1000);
      timevals[0].pad = 0;
      timevals[1].seconds = nanoseconds / 1000000000LL;
      timevals[1].microseconds = (int32_t)((nanoseconds % 1000000000LL) / 1000);
      timevals[1].pad = 0;
      return ServiceResult(utimes((const char*)first, timevals));
    }
    case 16: {
      DarwinStatfs value;
      if (statfs((const char*)first, &value) == -1) {
        return Fail();
      }
      DaveWireSpace* output = (DaveWireSpace*)second;
      uint64_t size = value.block_size != 0 ? value.block_size : 512;
      output->capacity = value.blocks * size;
      output->free = value.blocks_free * size;
      output->available = value.blocks_available * size;
      return 0;
    }
    case 17:
      return CopyFile((const char*)first, (const char*)second, (int)third);
    case 18:
      return CanonicalPath((const char*)first, (char*)second, (size_t)third);
    case 19: {
      DarwinStat value;
      if (fstat((int)first, &value) == -1) {
        return Fail();
      }
      CopyStat((DaveWireStatus*)second, &value);
      return 0;
    }
    default:
      return -ENOSYS;
  }
}
