#include "posix_host.h"

#include "../../libc/include/davecc_guest_syscalls.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <unistd.h>

// Guest errno values from the non-native branch of libc/include/errno.h.
enum {
  kGuestOk = 0,
  kGuestENOENT = 1,
  kGuestENOMEM = 2,
  kGuestEACCES = 3,
  kGuestENODEV = 4,
  kGuestEMFILE = 5,
  kGuestEBUSY = 6,
  kGuestEINVAL = 7,
  kGuestENOSPC = 8,
  kGuestEEXIST = 9,
  kGuestEAGAIN = 10,
  kGuestEIO = 11,
  kGuestEINTR = 12,
  kGuestENOSYS = 13,
  kGuestEBADF = 16,
  kGuestEPERM = 19,
  kGuestENAMETOOLONG = 24,
  kGuestEPIPE = 40,
  kGuestENOTSOCK = 41,
  kGuestEAFNOSUPPORT = 46,
  kGuestEADDRINUSE = 47,
  kGuestEADDRNOTAVAIL = 48,
  kGuestENOBUFS = 49,
  kGuestEISCONN = 50,
  kGuestENOTCONN = 51,
  kGuestETIMEDOUT = 52,
  kGuestECONNREFUSED = 53,
  kGuestEALREADY = 54,
  kGuestECONNABORTED = 55,
  kGuestECONNRESET = 56,
  kGuestEHOSTUNREACH = 57,
  kGuestENETDOWN = 58,
  kGuestENETUNREACH = 60,
  kGuestEFAULT = 37,
};

enum {
  kGuestAFUnix = 1,
  kGuestAFInet = 2,
  kGuestSockStream = 1,
  kGuestSockDgram = 2,
  kGuestSolSocket = 1,
  kGuestSoReuseAddr = 2,
  kGuestSoReusePort = 15,
  kGuestSoBroadcast = 6,
  kGuestSoNoSigPipe = 7,
  kGuestScmRights = 1,
  kGuestMsgNoSignal = 0x4000,
  kGuestMsgTrunc = 0x20,
  kGuestMsgCtrunc = 8,
  kMapShared = 0x01,
  kMapPrivate = 0x02,
  kMapFixed = 0x10,
  kMapAnonymous = 0x20,
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

typedef struct {
  uint64_t guest;
  void* host;
  size_t size;
} Mapping;

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static Mapping g_maps[512];
static int g_nmap;
static uint64_t g_next_guest = 0x60000000ull;

static int GuestErrno(int host) {
  switch (host) {
    case 0: return kGuestOk;
    case ENOENT: return kGuestENOENT;
    case ENOMEM: return kGuestENOMEM;
    case EACCES: return kGuestEACCES;
    case ENODEV: return kGuestENODEV;
    case EMFILE: return kGuestEMFILE;
    case ENFILE: return kGuestEMFILE;
    case EBUSY: return kGuestEBUSY;
    case EINVAL: return kGuestEINVAL;
    case ENOSPC: return kGuestENOSPC;
    case EEXIST: return kGuestEEXIST;
    case EAGAIN: return kGuestEAGAIN;
    case EIO: return kGuestEIO;
    case EINTR: return kGuestEINTR;
    case ENOSYS: return kGuestENOSYS;
    case EBADF: return kGuestEBADF;
    case EPERM: return kGuestEPERM;
    case ENAMETOOLONG: return kGuestENAMETOOLONG;
    case EPIPE: return kGuestEPIPE;
    case ENOTSOCK: return kGuestENOTSOCK;
    case EAFNOSUPPORT: return kGuestEAFNOSUPPORT;
    case EADDRINUSE: return kGuestEADDRINUSE;
    case EADDRNOTAVAIL: return kGuestEADDRNOTAVAIL;
    case ENOBUFS: return kGuestENOBUFS;
    case EISCONN: return kGuestEISCONN;
    case ENOTCONN: return kGuestENOTCONN;
    case ETIMEDOUT: return kGuestETIMEDOUT;
    case ECONNREFUSED: return kGuestECONNREFUSED;
    case EALREADY: return kGuestEALREADY;
    case ECONNABORTED: return kGuestECONNABORTED;
    case ECONNRESET: return kGuestECONNRESET;
    case EHOSTUNREACH: return kGuestEHOSTUNREACH;
    case ENETDOWN: return kGuestENETDOWN;
    case ENETUNREACH: return kGuestENETUNREACH;
    case EFAULT: return kGuestEFAULT;
    default: return kGuestEINVAL;
  }
}

static int64_t Fail(void) { return -(int64_t)GuestErrno(errno); }

void* DaveHostMappingFind(uint64_t guest, size_t size) {
  if (size == 0 || guest + (uint64_t)size < guest) return NULL;
  pthread_mutex_lock(&g_mu);
  void* host = NULL;
  for (int i = 0; i < g_nmap; i++) {
    Mapping* map = &g_maps[i];
    if (guest >= map->guest &&
        guest + (uint64_t)size <= map->guest + (uint64_t)map->size) {
      host = (char*)map->host + (guest - map->guest);
      break;
    }
  }
  pthread_mutex_unlock(&g_mu);
  return host;
}

static int Remember(uint64_t guest, void* host, size_t size) {
  pthread_mutex_lock(&g_mu);
  if (g_nmap == (int)(sizeof(g_maps) / sizeof(g_maps[0]))) {
    pthread_mutex_unlock(&g_mu);
    return -1;
  }
  g_maps[g_nmap].guest = guest;
  g_maps[g_nmap].host = host;
  g_maps[g_nmap].size = size;
  g_nmap++;
  pthread_mutex_unlock(&g_mu);
  return 0;
}

static Mapping Take(uint64_t guest) {
  Mapping found = {0};
  pthread_mutex_lock(&g_mu);
  for (int i = 0; i < g_nmap; i++) {
    if (g_maps[i].guest == guest) {
      found = g_maps[i];
      g_maps[i] = g_maps[g_nmap - 1];
      g_nmap--;
      break;
    }
  }
  pthread_mutex_unlock(&g_mu);
  return found;
}

static int TranslateOpen(int guest) {
  int host = guest & kGuestOAccmode;
  if (guest & kGuestOCreat) host |= O_CREAT;
  if (guest & kGuestOExcl) host |= O_EXCL;
  if (guest & kGuestONoctty) host |= O_NOCTTY;
  if (guest & kGuestOTrunc) host |= O_TRUNC;
  if (guest & kGuestOAppend) host |= O_APPEND;
  if (guest & kGuestONonblock) host |= O_NONBLOCK;
  if (guest & kGuestOSync) host |= O_SYNC;
  if (guest & kGuestODirectory) host |= O_DIRECTORY;
  if (guest & kGuestONofollow) host |= O_NOFOLLOW;
#ifdef O_CLOEXEC
  if (guest & kGuestOCloexec) host |= O_CLOEXEC;
#endif
  return host;
}

static int TranslateProt(int guest) {
  int host = 0;
  if (guest & 1) host |= PROT_READ;
  if (guest & 2) host |= PROT_WRITE;
  if (guest & 4) host |= PROT_EXEC;
  return host;
}

static int TranslateMap(int guest) {
  int host = 0;
  if (guest & kMapShared) host |= MAP_SHARED;
  if (guest & kMapPrivate) host |= MAP_PRIVATE;
  if (guest & kMapFixed) host |= MAP_FIXED;
  if (guest & kMapAnonymous) host |= MAP_ANON;
  return host;
}

static int TranslateDomain(int guest) {
  if (guest == kGuestAFUnix) return AF_UNIX;
  if (guest == kGuestAFInet) return AF_INET;
  errno = EAFNOSUPPORT;
  return -1;
}

static int TranslateType(int guest) {
  int base = guest & 0xff;
  if (base == kGuestSockStream) return SOCK_STREAM;
  if (base == kGuestSockDgram) return SOCK_DGRAM;
  errno = EPROTONOSUPPORT;
  return -1;
}

static int TranslateLevel(int guest) {
  if (guest == kGuestSolSocket) return SOL_SOCKET;
  return guest;
}

static int TranslateOption(int level, int guest) {
  if (level != SOL_SOCKET) {
    if (guest == 35) return IP_ADD_MEMBERSHIP;
    if (guest == 36) return IP_DROP_MEMBERSHIP;
    return guest;
  }
  if (guest == kGuestSoReuseAddr) return SO_REUSEADDR;
#ifdef SO_REUSEPORT
  if (guest == kGuestSoReusePort) return SO_REUSEPORT;
#endif
  if (guest == kGuestSoBroadcast) return SO_BROADCAST;
#ifdef SO_NOSIGPIPE
  if (guest == kGuestSoNoSigPipe) return SO_NOSIGPIPE;
#endif
  return guest;
}

static int TranslateMsg(int guest) {
  int host = 0;
  if (guest & kGuestMsgTrunc) host |= MSG_TRUNC;
#ifdef MSG_NOSIGNAL
  if (guest & kGuestMsgNoSignal) host |= MSG_NOSIGNAL;
#endif
  if (guest & kGuestMsgCtrunc) host |= MSG_CTRUNC;
  return host;
}

static int GuestMsg(int host) {
  int guest = 0;
  if (host & MSG_TRUNC) guest |= kGuestMsgTrunc;
  if (host & MSG_CTRUNC) guest |= kGuestMsgCtrunc;
  return guest;
}

static uint64_t ReadU64(const void* p) {
  uint64_t value;
  memcpy(&value, p, sizeof(value));
  return value;
}

static uint32_t ReadU32(const void* p) {
  uint32_t value;
  memcpy(&value, p, sizeof(value));
  return value;
}

static uint16_t ReadU16(const void* p) {
  uint16_t value;
  memcpy(&value, p, sizeof(value));
  return value;
}

static void WriteU32(void* p, uint32_t value) { memcpy(p, &value, sizeof(value)); }

static void WriteU16(void* p, uint16_t value) { memcpy(p, &value, sizeof(value)); }

static int64_t DoMmap(DaveHostGuestPtr guest_ptr, void* context,
                      uint64_t request_guest, int address_bits) {
  struct {
    uint64_t address;
    uint64_t length;
    int32_t protection;
    int32_t flags;
    int32_t fd;
    int32_t pad;
    int64_t offset;
  } request;
  void* raw = guest_ptr(context, request_guest, sizeof(request));
  if (raw == NULL) return -kGuestEFAULT;
  memcpy(&request, raw, sizeof(request));
  if (request.length == 0) return -kGuestEINVAL;
  int flags = TranslateMap(request.flags);
  void* hint = NULL;
  if ((request.flags & kMapFixed) != 0 && address_bits == 64) {
    hint = (void*)(uintptr_t)request.address;
  }
  int fd = (request.flags & kMapAnonymous) != 0 ? -1 : request.fd;
  void* host = mmap(hint, (size_t)request.length, TranslateProt(request.protection),
                    flags, fd, (off_t)request.offset);
  if (host == MAP_FAILED) return Fail();
  uint64_t guest;
  if (address_bits == 64) {
    guest = (uint64_t)(uintptr_t)host;
  } else {
    pthread_mutex_lock(&g_mu);
    guest = g_next_guest;
    g_next_guest += (request.length + 0xfffu) & ~0xfffull;
    pthread_mutex_unlock(&g_mu);
  }
  if (Remember(guest, host, (size_t)request.length) != 0) {
    munmap(host, (size_t)request.length);
    return -kGuestENOMEM;
  }
  return (int64_t)guest;
}

static int64_t DoMunmap(uint64_t guest, uint64_t length) {
  Mapping map = Take(guest);
  if (map.host == NULL) return -kGuestEINVAL;
  if (munmap(map.host, map.size < (size_t)length ? map.size : (size_t)length) !=
      0) {
    return Fail();
  }
  return 0;
}

static int64_t DoMprotect(uint64_t guest, uint64_t length, int protection) {
  void* host = DaveHostMappingFind(guest, (size_t)length);
  if (host == NULL) return -kGuestEINVAL;
  if (mprotect(host, (size_t)length, TranslateProt(protection)) != 0) return Fail();
  return 0;
}

static int BuildSockaddr(const unsigned char* guest, uint32_t length,
                         struct sockaddr_storage* host, socklen_t* host_len) {
  if (guest == NULL || length < 2) return -kGuestEINVAL;
  uint16_t family = ReadU16(guest);
  if (family == kGuestAFUnix) {
    struct sockaddr_un* un = (struct sockaddr_un*)host;
    memset(un, 0, sizeof(*un));
    un->sun_family = AF_UNIX;
    size_t cap = sizeof(un->sun_path) - 1;
    size_t n = length > 2 ? (size_t)length - 2 : 0;
    if (n > cap) n = cap;
    memcpy(un->sun_path, guest + 2, n);
    *host_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + n + 1);
#ifdef __APPLE__
    un->sun_len = (unsigned char)*host_len;
#endif
    return 0;
  }
  if (family == kGuestAFInet) {
    if (length < 8) return -kGuestEINVAL;
    struct sockaddr_in* in = (struct sockaddr_in*)host;
    memset(in, 0, sizeof(*in));
    in->sin_family = AF_INET;
    in->sin_port = ReadU16(guest + 2);
    memcpy(&in->sin_addr, guest + 4, 4);
    *host_len = sizeof(*in);
    return 0;
  }
  return -kGuestEAFNOSUPPORT;
}

static void WriteSockaddr(void* guest, uint32_t capacity, socklen_t* guest_len,
                          const struct sockaddr* host, socklen_t host_len) {
  if (guest == NULL || guest_len == NULL) return;
  unsigned char* out = guest;
  if (host != NULL && host->sa_family == AF_UNIX && capacity >= 2) {
    WriteU16(out, kGuestAFUnix);
    size_t n = host_len > 2 ? (size_t)host_len - 2 : 0;
    if (n > capacity - 2) n = capacity - 2;
    const struct sockaddr_un* un = (const struct sockaddr_un*)host;
    memcpy(out + 2, un->sun_path, n);
    *guest_len = (socklen_t)(2 + n);
    return;
  }
  if (host != NULL && host->sa_family == AF_INET && capacity >= 8) {
    const struct sockaddr_in* in = (const struct sockaddr_in*)host;
    WriteU16(out, kGuestAFInet);
    WriteU16(out + 2, in->sin_port);
    memcpy(out + 4, &in->sin_addr, 4);
    *guest_len = 16;
    return;
  }
  *guest_len = 0;
}

static uint64_t GuestPtrAt(const unsigned char* base, int pointer_size,
                           int offset) {
  if (pointer_size == 4) return ReadU32(base + offset);
  return ReadU64(base + offset);
}

static int64_t DoSendRecvMsg(int send, int pointer_size,
                             DaveHostGuestPtr guest_ptr, void* context,
                             int fd, uint64_t msg_guest, int flags) {
  size_t msg_size = pointer_size == 4 ? 28 : 52;
  unsigned char* msg = guest_ptr(context, msg_guest, msg_size);
  if (msg == NULL) return -kGuestEFAULT;
  uint64_t name_guest = GuestPtrAt(msg, pointer_size, 0);
  uint32_t name_len = ReadU32(msg + pointer_size);
  uint64_t iov_guest = GuestPtrAt(msg, pointer_size, pointer_size == 4 ? 8 : 16);
  uint64_t iov_len = pointer_size == 4
                         ? ReadU32(msg + 12)
                         : ReadU64(msg + 24);
  uint64_t control_guest =
      GuestPtrAt(msg, pointer_size, pointer_size == 4 ? 16 : 32);
  uint64_t control_len = pointer_size == 4 ? ReadU32(msg + 20) : ReadU64(msg + 40);
  if (iov_len > 16) return -kGuestEINVAL;
  size_t iov_bytes = (size_t)iov_len * (pointer_size == 4 ? 8 : 16);
  unsigned char* iov_raw =
      iov_len == 0 ? NULL : guest_ptr(context, iov_guest, iov_bytes);
  if (iov_len != 0 && iov_raw == NULL) return -kGuestEFAULT;
  struct iovec host_iov[16];
  for (uint64_t i = 0; i < iov_len; i++) {
    unsigned char* entry = iov_raw + i * (pointer_size == 4 ? 8 : 16);
    uint64_t base = GuestPtrAt(entry, pointer_size, 0);
    uint64_t len = pointer_size == 4 ? ReadU32(entry + 4) : ReadU64(entry + 8);
    host_iov[i].iov_base = len == 0 ? NULL : guest_ptr(context, base, (size_t)len);
    host_iov[i].iov_len = (size_t)len;
    if (len != 0 && host_iov[i].iov_base == NULL) return -kGuestEFAULT;
  }
  unsigned char* control =
      control_len == 0 ? NULL
                       : guest_ptr(context, control_guest, (size_t)control_len);
  if (control_len != 0 && control == NULL) return -kGuestEFAULT;
  char host_control[1024];
  struct sockaddr_storage name;
  socklen_t name_len_host = 0;
  if (name_guest != 0) {
    void* name_raw = guest_ptr(context, name_guest, name_len == 0 ? 2 : name_len);
    if (name_raw == NULL) return -kGuestEFAULT;
    int built = BuildSockaddr(name_raw, name_len, &name, &name_len_host);
    if (built < 0) return built;
  }
  struct msghdr host_msg;
  memset(&host_msg, 0, sizeof(host_msg));
  host_msg.msg_name = name_guest != 0 ? (void*)&name : NULL;
  host_msg.msg_namelen = name_len_host;
  host_msg.msg_iov = host_iov;
  host_msg.msg_iovlen = (int)iov_len;
  if (send && control != NULL) {
    size_t used = 0;
    size_t offset = 0;
    size_t align = (size_t)pointer_size;
    while (offset + align + 8 <= (size_t)control_len &&
           used + CMSG_SPACE(sizeof(int) * 16) < sizeof(host_control)) {
      uint64_t clen = pointer_size == 4 ? ReadU32(control + offset)
                                       : ReadU64(control + offset);
      int level = (int)ReadU32(control + offset + align);
      int type = (int)ReadU32(control + offset + align + 4);
      if (clen < align + 8) break;
      size_t data = (size_t)clen - (align + 8);
      if (level == kGuestSolSocket && type == kGuestScmRights) {
        struct cmsghdr* cmsg = (struct cmsghdr*)(host_control + used);
        size_t space = CMSG_SPACE(data);
        if (used + space > sizeof(host_control)) break;
        memset(cmsg, 0, space);
        cmsg->cmsg_len = CMSG_LEN(data);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        memcpy(CMSG_DATA(cmsg), control + offset + align + 8, data);
        used += space;
      }
      size_t step = (clen + align - 1) & ~(align - 1);
      if (step == 0) break;
      offset += step;
    }
    host_msg.msg_control = used == 0 ? NULL : host_control;
    host_msg.msg_controllen = (socklen_t)used;
  } else if (!send) {
    host_msg.msg_control = host_control;
    host_msg.msg_controllen = sizeof(host_control);
  }
  ssize_t n = send ? sendmsg(fd, &host_msg, TranslateMsg(flags))
                   : recvmsg(fd, &host_msg, TranslateMsg(flags));
  if (n < 0) return Fail();
  if (!send && control != NULL) {
    memset(control, 0, (size_t)control_len);
    size_t offset = 0;
    size_t align = (size_t)pointer_size;
    for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&host_msg); cmsg != NULL;
         cmsg = CMSG_NXTHDR(&host_msg, cmsg)) {
      if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
        continue;
      }
      size_t data = cmsg->cmsg_len - CMSG_LEN(0);
      size_t need = align + 8 + data;
      if (offset + need > (size_t)control_len) break;
      if (pointer_size == 4) WriteU32(control + offset, (uint32_t)need);
      else {
        uint64_t wide = need;
        memcpy(control + offset, &wide, sizeof(wide));
      }
      WriteU32(control + offset + align, kGuestSolSocket);
      WriteU32(control + offset + align + 4, kGuestScmRights);
      memcpy(control + offset + align + 8, CMSG_DATA(cmsg), data);
      offset += (need + align - 1) & ~(align - 1);
    }
    if (pointer_size == 4) WriteU32(msg + 20, (uint32_t)offset);
    else {
      uint64_t wide = offset;
      memcpy(msg + 40, &wide, sizeof(wide));
    }
    int guest_flags = GuestMsg(host_msg.msg_flags);
    WriteU32(msg + (pointer_size == 4 ? 24 : 48), (uint32_t)guest_flags);
  }
  return (int64_t)n;
}

static void CopyUtsField(char* dst, const char* src) {
  size_t i = 0;
  if (src != NULL) {
    while (i + 1 < DAVE_UTSNAME_LENGTH && src[i] != '\0') {
      dst[i] = src[i];
      i++;
    }
  }
  while (i < DAVE_UTSNAME_LENGTH) {
    dst[i++] = '\0';
  }
}

int64_t DaveHostPosixSyscall(int number, int pointer_size, int address_bits,
                             DaveHostGuestPtr guest_ptr, void* context,
                             uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                             uint64_t a4, uint64_t a5) {
  (void)a5;
  if (pointer_size != 4 && pointer_size != 8) return -kGuestEINVAL;
  switch (number) {
    case DAVE_SYS_MMAP:
      return DoMmap(guest_ptr, context, a0, address_bits);
    case DAVE_SYS_MUNMAP:
      return DoMunmap(a0, a1);
    case DAVE_SYS_MPROTECT:
      return DoMprotect(a0, a1, (int)a2);
    case DAVE_SYS_SHM_OPEN: {
      const char* name = guest_ptr(context, a0, 1);
      if (name == NULL) return -kGuestEFAULT;
      int fd = shm_open(name, TranslateOpen((int)a1), (mode_t)a2);
      return fd < 0 ? Fail() : fd;
    }
    case DAVE_SYS_SHM_UNLINK: {
      const char* name = guest_ptr(context, a0, 1);
      if (name == NULL) return -kGuestEFAULT;
      return shm_unlink(name) == 0 ? 0 : Fail();
    }
    case DAVE_SYS_FTRUNCATE: {
      int64_t length = 0;
      void* p = guest_ptr(context, a1, sizeof(length));
      if (p == NULL) return -kGuestEFAULT;
      memcpy(&length, p, sizeof(length));
      return ftruncate((int)a0, (off_t)length) == 0 ? 0 : Fail();
    }
    case DAVE_SYS_SOCKET: {
      int domain = TranslateDomain((int)a0);
      int type = TranslateType((int)a1);
      if (domain < 0 || type < 0) return Fail();
      int fd = socket(domain, type, (int)a2);
      if (fd < 0) return Fail();
#ifdef SO_NOSIGPIPE
      int one = 1;
      setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
      return fd;
    }
    case DAVE_SYS_BIND:
    case DAVE_SYS_CONNECT: {
      void* guest = guest_ptr(context, a1, a2 == 0 ? 2 : (size_t)a2);
      if (guest == NULL) return -kGuestEFAULT;
      struct sockaddr_storage host;
      socklen_t host_len = 0;
      int built = BuildSockaddr(guest, (uint32_t)a2, &host, &host_len);
      if (built < 0) return built;
      int rc = number == DAVE_SYS_BIND
                   ? bind((int)a0, (struct sockaddr*)&host, host_len)
                   : connect((int)a0, (struct sockaddr*)&host, host_len);
      return rc == 0 ? 0 : Fail();
    }
    case DAVE_SYS_LISTEN:
      return listen((int)a0, (int)a1) == 0 ? 0 : Fail();
    case DAVE_SYS_ACCEPT: {
      struct sockaddr_storage host;
      socklen_t host_len = sizeof(host);
      int fd = accept((int)a0, (struct sockaddr*)&host, &host_len);
      if (fd < 0) return Fail();
      if (a1 != 0 && a2 != 0) {
        uint32_t capacity = 0;
        void* lenp = guest_ptr(context, a2, sizeof(uint32_t));
        void* addr = NULL;
        if (lenp != NULL) {
          memcpy(&capacity, lenp, sizeof(capacity));
          addr = guest_ptr(context, a1, capacity == 0 ? 1 : capacity);
        }
        if (addr != NULL && lenp != NULL) {
          socklen_t guest_len = 0;
          WriteSockaddr(addr, capacity, &guest_len, (struct sockaddr*)&host,
                        host_len);
          memcpy(lenp, &guest_len, sizeof(guest_len));
        }
      }
      return fd;
    }
    case DAVE_SYS_SEND:
    case DAVE_SYS_RECV: {
      void* buf = a2 == 0 ? NULL : guest_ptr(context, a1, (size_t)a2);
      if (a2 != 0 && buf == NULL) return -kGuestEFAULT;
      ssize_t n = number == DAVE_SYS_SEND
                      ? send((int)a0, buf, (size_t)a2, TranslateMsg((int)a3))
                      : recv((int)a0, buf, (size_t)a2, TranslateMsg((int)a3));
      return n < 0 ? Fail() : (int64_t)n;
    }
    case DAVE_SYS_SENDMSG:
      return DoSendRecvMsg(1, pointer_size, guest_ptr, context, (int)a0, a1,
                           (int)a2);
    case DAVE_SYS_RECVMSG:
      return DoSendRecvMsg(0, pointer_size, guest_ptr, context, (int)a0, a1,
                           (int)a2);
    case DAVE_SYS_SHUTDOWN:
      return shutdown((int)a0, (int)a1) == 0 ? 0 : Fail();
    case DAVE_SYS_SETSOCKOPT:
    case DAVE_SYS_GETSOCKOPT: {
      unsigned char args[32];
      void* raw = guest_ptr(context, a0, sizeof(args));
      if (raw == NULL) return -kGuestEFAULT;
      memcpy(args, raw, sizeof(args));
      int fd = (int)ReadU32(args);
      int level = TranslateLevel((int)ReadU32(args + 4));
      int name = TranslateOption(level, (int)ReadU32(args + 8));
      uint64_t value_guest = ReadU64(args + 16);
      uint64_t length_guest = ReadU64(args + 24);
      if (number == DAVE_SYS_SETSOCKOPT) {
        void* value = guest_ptr(context, value_guest, (size_t)length_guest);
        if (value == NULL) return -kGuestEFAULT;
        return setsockopt(fd, level, name, value, (socklen_t)length_guest) == 0
                   ? 0
                   : Fail();
      }
      uint32_t length = 0;
      void* lenp = guest_ptr(context, length_guest, sizeof(length));
      if (lenp == NULL) return -kGuestEFAULT;
      memcpy(&length, lenp, sizeof(length));
      void* value = guest_ptr(context, value_guest, length == 0 ? 1 : length);
      if (value == NULL) return -kGuestEFAULT;
      socklen_t host_len = length;
      if (getsockopt(fd, level, name, value, &host_len) != 0) return Fail();
      memcpy(lenp, &host_len, sizeof(host_len));
      return 0;
    }
    case DAVE_SYS_GETSOCKNAME:
    case DAVE_SYS_GETPEERNAME: {
      struct sockaddr_storage host;
      socklen_t host_len = sizeof(host);
      int rc = number == DAVE_SYS_GETSOCKNAME
                   ? getsockname((int)a0, (struct sockaddr*)&host, &host_len)
                   : getpeername((int)a0, (struct sockaddr*)&host, &host_len);
      if (rc != 0) return Fail();
      uint32_t capacity = 0;
      void* lenp = guest_ptr(context, a2, sizeof(capacity));
      if (lenp == NULL) return -kGuestEFAULT;
      memcpy(&capacity, lenp, sizeof(capacity));
      void* addr = guest_ptr(context, a1, capacity == 0 ? 1 : capacity);
      if (addr == NULL) return -kGuestEFAULT;
      socklen_t guest_len = 0;
      WriteSockaddr(addr, capacity, &guest_len, (struct sockaddr*)&host,
                    host_len);
      memcpy(lenp, &guest_len, sizeof(guest_len));
      return 0;
    }
    case DAVE_SYS_PIPE: {
      int host_fds[2];
      if (pipe(host_fds) != 0) return Fail();
      void* out = guest_ptr(context, a0, sizeof(int) * 2);
      if (out == NULL) {
        close(host_fds[0]);
        close(host_fds[1]);
        return -kGuestEFAULT;
      }
      memcpy(out, host_fds, sizeof(host_fds));
      return 0;
    }
    case DAVE_SYS_FCNTL: {
      int cmd = (int)a1;
      if (cmd == F_GETFL || cmd == 3) {
        int flags = fcntl((int)a0, F_GETFL);
        if (flags < 0) return Fail();
        int guest = flags & 3;
        if (flags & O_NONBLOCK) guest |= kGuestONonblock;
        if (flags & O_APPEND) guest |= kGuestOAppend;
        return guest;
      }
      if (cmd == F_SETFL || cmd == 4) {
        return fcntl((int)a0, F_SETFL, TranslateOpen((int)a2)) < 0 ? Fail() : 0;
      }
      if (cmd == F_GETFD || cmd == 1) {
        int flags = fcntl((int)a0, F_GETFD);
        return flags < 0 ? Fail() : flags;
      }
      if (cmd == F_SETFD || cmd == 2) {
        return fcntl((int)a0, F_SETFD, (int)a2) < 0 ? Fail() : 0;
      }
      if (cmd == F_DUPFD || cmd == 0) {
        int fd = fcntl((int)a0, F_DUPFD, (int)a2);
        return fd < 0 ? Fail() : fd;
      }
      return -kGuestENOSYS;
    }
    case DAVE_SYS_GETPID:
      return (int64_t)getpid();
    case DAVE_SYS_GETHOSTNAME: {
      void* name = guest_ptr(context, a0, (size_t)a1);
      if (name == NULL) return -kGuestEFAULT;
      return gethostname(name, (size_t)a1) == 0 ? 0 : Fail();
    }
    case DAVE_SYS_UNLINK: {
      const char* path = guest_ptr(context, a0, 1);
      if (path == NULL) return -kGuestEFAULT;
      return unlink(path) == 0 ? 0 : Fail();
    }
    case DAVE_SYS_DUP: {
      int fd = dup((int)a0);
      return fd < 0 ? Fail() : fd;
    }
    case DAVE_SYS_DUP2: {
      int fd = dup2((int)a0, (int)a1);
      return fd < 0 ? Fail() : fd;
    }
    case DAVE_SYS_UNAME: {
      struct utsname host_name;
      char* guest_name = guest_ptr(context, a0, (size_t)DAVE_UTSNAME_LENGTH * 5);
      if (guest_name == NULL) return -kGuestEFAULT;
      if (uname(&host_name) != 0) return Fail();
      CopyUtsField(guest_name, host_name.sysname);
      CopyUtsField(guest_name + DAVE_UTSNAME_LENGTH, host_name.nodename);
      CopyUtsField(guest_name + DAVE_UTSNAME_LENGTH * 2, host_name.release);
      CopyUtsField(guest_name + DAVE_UTSNAME_LENGTH * 3, host_name.version);
      CopyUtsField(guest_name + DAVE_UTSNAME_LENGTH * 4, host_name.machine);
      return 0;
    }
    default:
      return -kGuestENOSYS;
  }
}
