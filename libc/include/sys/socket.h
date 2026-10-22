#ifndef davecc_sys_socket_h
#define davecc_sys_socket_h

#if defined(__DAVECC_NATIVE_DARWIN__)
#include <sys/types.h>
#include_next <sys/socket.h>
#else

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define AF_UNSPEC 0
#define AF_UNIX 1
#define AF_INET 2

#define SOCK_STREAM 1
#define SOCK_DGRAM 2

#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define SO_REUSEPORT 15
#define SO_BROADCAST 6
#define SO_NOSIGPIPE 7
#define SOMAXCONN 128

#define SCM_RIGHTS 1

#define MSG_CTRUNC 8
#define MSG_TRUNC 0x20
#define MSG_NOSIGNAL 0x4000

#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

typedef unsigned int socklen_t;
typedef unsigned short sa_family_t;

struct sockaddr {
  sa_family_t sa_family;
  char sa_data[14];
};

struct sockaddr_storage {
  sa_family_t ss_family;
  char ss_pad[126];
};

struct iovec {
  void* iov_base;
  size_t iov_len;
};

// Layout is part of the guest/host ABI.  64-bit records are 52 bytes and
// 32-bit records are 28 bytes; the host translator reads those offsets.
struct msghdr {
  void* msg_name;
  socklen_t msg_namelen;
#if defined(__LP64__) || defined(__p_code__)
  int msg_pad;
#endif
  struct iovec* msg_iov;
  size_t msg_iovlen;
  void* msg_control;
  size_t msg_controllen;
  int msg_flags;
};

struct cmsghdr {
  size_t cmsg_len;
  int cmsg_level;
  int cmsg_type;
};

#define CMSG_ALIGN(length) \
  (((length) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1))
#define CMSG_LEN(length) (sizeof(struct cmsghdr) + (length))
#define CMSG_SPACE(length) \
  (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(length))
#define CMSG_DATA(cmsg) ((unsigned char*)(cmsg) + sizeof(struct cmsghdr))
#define CMSG_FIRSTHDR(msg) \
  ((msg)->msg_controllen < sizeof(struct cmsghdr) \
       ? (struct cmsghdr*)0 \
       : (struct cmsghdr*)(msg)->msg_control)
#define CMSG_NXTHDR(msg, cmsg) DaveccCmsgNext(msg, cmsg)

static inline struct cmsghdr* DaveccCmsgNext(struct msghdr* msg,
                                            struct cmsghdr* cmsg) {
  if (msg == 0 || cmsg == 0 || msg->msg_control == 0) return 0;
  size_t step = CMSG_ALIGN(cmsg->cmsg_len);
  unsigned char* next = (unsigned char*)cmsg + step;
  unsigned char* end =
      (unsigned char*)msg->msg_control + msg->msg_controllen;
  if (next + sizeof(struct cmsghdr) > end) return 0;
  return (struct cmsghdr*)next;
}

#ifdef __cplusplus
extern "C" {
#endif

int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
ssize_t send(int sockfd, const void* buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* dest, socklen_t destlen);
ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags,
                 struct sockaddr* src, socklen_t* srclen);
ssize_t recv(int sockfd, void* buf, size_t len, int flags);
ssize_t sendmsg(int sockfd, const struct msghdr* msg, int flags);
ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags);
int shutdown(int sockfd, int how);
int setsockopt(int sockfd, int level, int optname, const void* optval,
               socklen_t optlen);
int getsockopt(int sockfd, int level, int optname, void* optval,
               socklen_t* optlen);
int getsockname(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int getpeername(int sockfd, struct sockaddr* addr, socklen_t* addrlen);

#ifdef __cplusplus
}
#endif

#endif /* __DAVECC_NATIVE_DARWIN__ */
#endif
