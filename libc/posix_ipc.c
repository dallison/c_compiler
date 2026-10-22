#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sched.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/auxv.h>
#include <link.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#if !defined(__DAVECC_NATIVE_LINUX__) && !defined(__DAVECC_NATIVE_DARWIN__)

#include <arpa/inet.h>
#include <netinet/in.h>
#include <syscall.h>

static int Fail(long result) {
  if (result >= 0) return (int)result;
  errno = (int)-result;
  return -1;
}

static long FailLong(long result) {
  if (result >= 0) return result;
  errno = (int)-result;
  return -1;
}

int shm_open(const char* name, int oflag, ...) {
  va_list ap;
  va_start(ap, oflag);
  int mode = 0;
  if ((oflag & O_CREAT) != 0) mode = va_arg(ap, int);
  va_end(ap);
  return Fail(syscall(SYS_SHM_OPEN, name, oflag, mode));
}

int shm_unlink(const char* name) {
  return Fail(syscall(SYS_SHM_UNLINK, name));
}

int ftruncate(int fd, off_t length) {
  int64_t wide = (int64_t)length;
  return Fail(syscall(SYS_FTRUNCATE, fd, &wide));
}

int socket(int domain, int type, int protocol) {
  return Fail(syscall(SYS_SOCKET, domain, type, protocol));
}

int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
  return Fail(syscall(SYS_BIND, sockfd, addr, addrlen));
}

int listen(int sockfd, int backlog) {
  return Fail(syscall(SYS_LISTEN, sockfd, backlog));
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
  return Fail(syscall(SYS_ACCEPT, sockfd, addr, addrlen));
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
  return Fail(syscall(SYS_CONNECT, sockfd, addr, addrlen));
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
  return (ssize_t)FailLong(syscall(SYS_SEND, sockfd, buf, len, flags));
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
  return (ssize_t)FailLong(syscall(SYS_RECV, sockfd, buf, len, flags));
}

ssize_t sendmsg(int sockfd, const struct msghdr* msg, int flags) {
  return (ssize_t)FailLong(syscall(SYS_SENDMSG, sockfd, msg, flags));
}

ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags) {
  return (ssize_t)FailLong(syscall(SYS_RECVMSG, sockfd, msg, flags));
}

int shutdown(int sockfd, int how) {
  return Fail(syscall(SYS_SHUTDOWN, sockfd, how));
}

struct __davecc_sockopt {
  int32_t fd;
  int32_t level;
  int32_t name;
  int32_t pad;
  uint64_t value;
  uint64_t length;
};

int setsockopt(int sockfd, int level, int optname, const void* optval,
               socklen_t optlen) {
  struct __davecc_sockopt args;
  args.fd = sockfd;
  args.level = level;
  args.name = optname;
  args.pad = 0;
  args.value = (uint64_t)(uintptr_t)optval;
  args.length = optlen;
  return Fail(syscall(SYS_SETSOCKOPT, &args));
}

int getsockopt(int sockfd, int level, int optname, void* optval,
               socklen_t* optlen) {
  struct __davecc_sockopt args;
  args.fd = sockfd;
  args.level = level;
  args.name = optname;
  args.pad = 0;
  args.value = (uint64_t)(uintptr_t)optval;
  args.length = (uint64_t)(uintptr_t)optlen;
  return Fail(syscall(SYS_GETSOCKOPT, &args));
}

int getsockname(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
  return Fail(syscall(SYS_GETSOCKNAME, sockfd, addr, addrlen));
}

int getpeername(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
  return Fail(syscall(SYS_GETPEERNAME, sockfd, addr, addrlen));
}

int pipe(int* fildes) { return Fail(syscall(SYS_PIPE, fildes)); }

int pipe2(int* fildes, int flags) {
  if (pipe(fildes) != 0) return -1;
  if ((flags & O_NONBLOCK) != 0) {
    if (fcntl(fildes[0], F_SETFL, O_NONBLOCK) != 0 ||
        fcntl(fildes[1], F_SETFL, O_NONBLOCK) != 0) {
      int saved = errno;
      close(fildes[0]);
      close(fildes[1]);
      errno = saved;
      return -1;
    }
  }
  if ((flags & O_CLOEXEC) != 0) {
    if (fcntl(fildes[0], F_SETFD, FD_CLOEXEC) != 0 ||
        fcntl(fildes[1], F_SETFD, FD_CLOEXEC) != 0) {
      int saved = errno;
      close(fildes[0]);
      close(fildes[1]);
      errno = saved;
      return -1;
    }
  }
  return 0;
}

int fcntl(int fd, int cmd, ...) {
  if (cmd == F_GETPIPE_SZ) return 65536;
  if (cmd == F_SETPIPE_SZ) return 0;
  int arg = 0;
  if (cmd != F_GETFL && cmd != F_GETFD) {
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, int);
    va_end(ap);
  }
  return Fail(syscall(SYS_FCNTL, fd, cmd, arg));
}

pid_t getpid(void) { return (pid_t)syscall(SYS_GETPID); }

int gethostname(char* name, size_t size) {
  return Fail(syscall(SYS_GETHOSTNAME, name, size));
}

int unlink(const char* path) { return Fail(syscall(SYS_UNLINK, path)); }

int dup(int oldfd) { return Fail(syscall(SYS_DUP, oldfd)); }

int dup2(int oldfd, int newfd) { return Fail(syscall(SYS_DUP2, oldfd, newfd)); }

mode_t umask(mode_t mask) {
  static mode_t current = 022;
  mode_t previous = current;
  current = mask & 0777;
  return previous;
}

void* dlopen(const char* path, int mode) {
  (void)path;
  (void)mode;
  errno = ENOSYS;
  return NULL;
}

void* dlsym(void* handle, const char* symbol) {
  (void)handle;
  (void)symbol;
  errno = ENOSYS;
  return NULL;
}

int dlclose(void* handle) {
  (void)handle;
  return 0;
}

char* dlerror(void) { return (char*)"dynamic loading is not available"; }

int getifaddrs(struct ifaddrs** list) {
  if (list != NULL) *list = NULL;
  errno = ENOSYS;
  return -1;
}

void freeifaddrs(struct ifaddrs* list) { (void)list; }

unsigned int if_nametoindex(const char* name) {
  (void)name;
  return 0;
}

char* if_indextoname(unsigned int index, char* name) {
  (void)index;
  (void)name;
  return NULL;
}

int ioctl(int fd, unsigned long request, ...) {
  (void)fd;
  if (request == TIOCGWINSZ) {
    va_list ap;
    va_start(ap, request);
    struct winsize* window = va_arg(ap, struct winsize*);
    va_end(ap);
    if (window == NULL) {
      errno = EFAULT;
      return -1;
    }
    window->ws_row = 24;
    window->ws_col = 80;
    window->ws_xpixel = 0;
    window->ws_ypixel = 0;
    return 0;
  }
  errno = ENOTTY;
  return -1;
}

static uint16_t Swap16(uint16_t value) {
  return (uint16_t)((value << 8) | (value >> 8));
}

static uint32_t Swap32(uint32_t value) {
  return (value << 24) | ((value << 8) & 0x00ff0000u) |
         ((value >> 8) & 0x0000ff00u) | (value >> 24);
}

uint16_t htons(uint16_t value) { return Swap16(value); }
uint16_t ntohs(uint16_t value) { return Swap16(value); }
uint32_t htonl(uint32_t value) { return Swap32(value); }
uint32_t ntohl(uint32_t value) { return Swap32(value); }

int inet_pton(int family, const char* text, void* destination) {
  if (family != AF_INET || text == NULL || destination == NULL) {
    errno = EAFNOSUPPORT;
    return -1;
  }
  unsigned parts[4];
  const char* cursor = text;
  for (int i = 0; i < 4; i++) {
    if (*cursor < '0' || *cursor > '9') return 0;
    unsigned value = 0;
    int digits = 0;
    while (*cursor >= '0' && *cursor <= '9') {
      value = value * 10u + (unsigned)(*cursor - '0');
      if (value > 255) return 0;
      cursor++;
      digits++;
      if (digits > 3) return 0;
    }
    if (digits == 0) return 0;
    parts[i] = value;
    if (i < 3) {
      if (*cursor != '.') return 0;
      cursor++;
    }
  }
  if (*cursor != '\0') return 0;
  unsigned char* out = destination;
  out[0] = (unsigned char)parts[0];
  out[1] = (unsigned char)parts[1];
  out[2] = (unsigned char)parts[2];
  out[3] = (unsigned char)parts[3];
  return 1;
}

const char* inet_ntop(int family, const void* source, char* destination,
                      unsigned int size) {
  if (family != AF_INET || source == NULL || destination == NULL) {
    errno = EAFNOSUPPORT;
    return NULL;
  }
  const unsigned char* in = source;
  char text[16];
  int n = 0;
  for (int i = 0; i < 4; i++) {
    unsigned value = in[i];
    char digits[3];
    int count = 0;
    do {
      digits[count++] = (char)('0' + value % 10);
      value /= 10;
    } while (value != 0);
    while (count > 0) text[n++] = digits[--count];
    if (i < 3) text[n++] = '.';
  }
  text[n] = '\0';
  if ((unsigned)n + 1 > size) {
    errno = ENOSPC;
    return NULL;
  }
  memcpy(destination, text, (size_t)n + 1);
  return destination;
}

ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* dest, socklen_t destlen) {
  (void)dest;
  (void)destlen;
  return send(sockfd, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags,
                 struct sockaddr* src, socklen_t* srclen) {
  (void)src;
  (void)srclen;
  return recv(sockfd, buf, len, flags);
}

int kill(pid_t pid, int signal_number) {
  if (signal_number != 0) {
    errno = ENOSYS;
    return -1;
  }
  if (pid == getpid()) return 0;
  errno = ESRCH;
  return -1;
}

int clone(int (*function)(void*), void* child_stack, int flags, void* argument,
          ...) {
  (void)function;
  (void)child_stack;
  (void)flags;
  (void)argument;
  errno = ENOSYS;
  return -1;
}

struct hostent* gethostbyname(const char* name) {
  (void)name;
  return NULL;
}

DIR* opendir(const char* name) {
  (void)name;
  errno = ENOSYS;
  return NULL;
}

struct dirent* readdir(DIR* directory) {
  (void)directory;
  return NULL;
}

int closedir(DIR* directory) {
  (void)directory;
  return 0;
}

int dl_iterate_phdr(int (*callback)(struct dl_phdr_info* info, size_t size,
                                    void* data),
                    void* data) {
  (void)callback;
  (void)data;
  return 0;
}

int sigaltstack(const stack_t* stack, stack_t* old_stack) {
  (void)stack;
  (void)old_stack;
  return 0;
}

int sched_getcpu(void) { return 0; }

unsigned long getauxval(unsigned long type) {
  (void)type;
  return 0;
}

int getpagesize(void) { return 4096; }

unsigned sleep(unsigned seconds) {
  (void)seconds;
  return 0;
}

int setenv(const char* name, const char* value, int overwrite) {
  (void)name;
  (void)value;
  (void)overwrite;
  return 0;
}

int unsetenv(const char* name) {
  (void)name;
  return 0;
}

uid_t getuid(void) { return 0; }

gid_t getgid(void) { return 0; }

int chown(const char* path, uid_t owner, gid_t group) {
  (void)path;
  (void)owner;
  (void)group;
  errno = ENOSYS;
  return -1;
}

int rename(const char* from, const char* to) {
  (void)from;
  (void)to;
  errno = ENOSYS;
  return -1;
}

int execv(const char* path, char** argv) {
  (void)path;
  (void)argv;
  errno = ENOSYS;
  return -1;
}

int execve(const char* path, char** argv, char** envp) {
  (void)path;
  (void)argv;
  (void)envp;
  errno = ENOSYS;
  return -1;
}

unsigned alarm(unsigned seconds) {
  (void)seconds;
  return 0;
}

int sigaction(int signal_number, const struct sigaction* action,
              struct sigaction* old_action) {
  (void)signal_number;
  (void)action;
  if (old_action != NULL) {
    old_action->sa_handler = SIG_DFL;
    old_action->sa_flags = 0;
  }
  return 0;
}

int getrlimit(int resource, struct rlimit* limit) {
  (void)resource;
  if (limit == NULL) {
    errno = EFAULT;
    return -1;
  }
  limit->rlim_cur = 1024;
  limit->rlim_max = 1024;
  return 0;
}

int setrlimit(int resource, const struct rlimit* limit) {
  (void)resource;
  (void)limit;
  return 0;
}

int getaddrinfo(const char* node, const char* service,
                const struct addrinfo* hints, struct addrinfo** result) {
  (void)node;
  (void)service;
  (void)hints;
  if (result != NULL) *result = NULL;
  return EAI_NONAME;
}

void freeaddrinfo(struct addrinfo* result) { (void)result; }

const char* gai_strerror(int error) {
  (void)error;
  return "name resolution is not available";
}

#endif
