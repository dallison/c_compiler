#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#if !defined(__DAVECC_NATIVE_LINUX__) && !defined(__DAVECC_NATIVE_DARWIN__)

#include <netinet/in.h>
#include <poll.h>

enum { kMaxEpoll = 32, kMaxInterest = 64, kMaxTimer = 32 };

struct Interest {
  int fd;
  uint32_t events;
  epoll_data_t data;
  int used;
};

struct EpollSet {
  int fd;
  struct Interest items[kMaxInterest];
  int used;
};

struct TimerFd {
  int read_fd;
  int write_fd;
  time_t deadline_sec;
  long deadline_nsec;
  int armed;
  int fired;
  int used;
};

static struct EpollSet g_epoll[kMaxEpoll];
static struct TimerFd g_timers[kMaxTimer];

static int TimeReached(time_t sec, long nsec) {
  struct timespec now;
  if (clock_gettime(CLOCK_REALTIME, &now) != 0) return 0;
  if (now.tv_sec > sec) return 1;
  if (now.tv_sec < sec) return 0;
  return now.tv_nsec >= nsec;
}

static int DeltaMs(time_t sec, long nsec) {
  struct timespec now;
  if (clock_gettime(CLOCK_REALTIME, &now) != 0) return 0;
  if (TimeReached(sec, nsec)) return 0;
  time_t dsec = sec - now.tv_sec;
  long dnsec = nsec - now.tv_nsec;
  if (dnsec < 0) {
    dnsec += 1000000000L;
    dsec--;
  }
  if (dsec > 2000000) return 2000000000;
  return (int)dsec * 1000 + (int)(dnsec / 1000000L);
}

static struct EpollSet* FindEpoll(int fd) {
  for (int i = 0; i < kMaxEpoll; i++) {
    if (g_epoll[i].used && g_epoll[i].fd == fd) return &g_epoll[i];
  }
  return NULL;
}

static struct TimerFd* FindTimer(int fd) {
  for (int i = 0; i < kMaxTimer; i++) {
    if (g_timers[i].used && g_timers[i].read_fd == fd) return &g_timers[i];
  }
  return NULL;
}

static void SetFlag(int fd, int get_cmd, int set_cmd, int flag) {
  int current = fcntl(fd, get_cmd);
  if (current >= 0) fcntl(fd, set_cmd, current | flag);
}

int eventfd(unsigned int initval, int flags) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) return -1;
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(0x7f000001);
  address.sin_port = 0;
  socklen_t length = sizeof(address);
  if (bind(fd, (struct sockaddr*)&address, length) != 0 ||
      getsockname(fd, (struct sockaddr*)&address, &length) != 0 ||
      connect(fd, (struct sockaddr*)&address, length) != 0) {
    int saved = errno;
    close(fd);
    errno = saved;
    return -1;
  }
  if ((flags & EFD_NONBLOCK) != 0) SetFlag(fd, F_GETFL, F_SETFL, O_NONBLOCK);
  if ((flags & EFD_CLOEXEC) != 0) SetFlag(fd, F_GETFD, F_SETFD, FD_CLOEXEC);
  if (initval != 0) {
    uint64_t value = initval;
    if (write(fd, &value, sizeof(value)) != (ssize_t)sizeof(value)) {
      int saved = errno;
      close(fd);
      errno = saved;
      return -1;
    }
  }
  return fd;
}

static int MakePipeEnd(int* read_fd, int* write_fd, int flags) {
  int ends[2];
  if (pipe(ends) != 0) return -1;
  if ((flags & TFD_NONBLOCK) != 0 || (flags & EFD_NONBLOCK) != 0) {
    SetFlag(ends[0], F_GETFL, F_SETFL, O_NONBLOCK);
  }
  if ((flags & TFD_CLOEXEC) != 0) SetFlag(ends[0], F_GETFD, F_SETFD, FD_CLOEXEC);
  *read_fd = ends[0];
  *write_fd = ends[1];
  return 0;
}

int epoll_create(int size) {
  (void)size;
  return epoll_create1(0);
}

int epoll_create1(int flags) {
  int read_fd = -1;
  int write_fd = -1;
  if (MakePipeEnd(&read_fd, &write_fd, flags) != 0) return -1;
  close(write_fd);
  for (int i = 0; i < kMaxEpoll; i++) {
    if (!g_epoll[i].used) {
      memset(&g_epoll[i], 0, sizeof(g_epoll[i]));
      g_epoll[i].used = 1;
      g_epoll[i].fd = read_fd;
      return read_fd;
    }
  }
  close(read_fd);
  errno = ENOMEM;
  return -1;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event) {
  struct EpollSet* set = FindEpoll(epfd);
  if (set == NULL) {
    errno = EBADF;
    return -1;
  }
  struct Interest* slot = NULL;
  for (int i = 0; i < kMaxInterest; i++) {
    if (set->items[i].used && set->items[i].fd == fd) {
      slot = &set->items[i];
      break;
    }
  }
  if (op == EPOLL_CTL_ADD) {
    if (slot != NULL || event == NULL) {
      errno = EEXIST;
      return -1;
    }
    for (int i = 0; i < kMaxInterest; i++) {
      if (!set->items[i].used) {
        set->items[i].used = 1;
        set->items[i].fd = fd;
        set->items[i].events = event->events;
        set->items[i].data = event->data;
        return 0;
      }
    }
    errno = ENOMEM;
    return -1;
  }
  if (slot == NULL || event == NULL) {
    errno = ENOENT;
    return -1;
  }
  if (op == EPOLL_CTL_DEL) {
    slot->used = 0;
    return 0;
  }
  if (op == EPOLL_CTL_MOD) {
    slot->events = event->events;
    slot->data = event->data;
    return 0;
  }
  errno = EINVAL;
  return -1;
}

static void FireTimers(void) {
  for (int i = 0; i < kMaxTimer; i++) {
    struct TimerFd* timer = &g_timers[i];
    if (!timer->used || !timer->armed || timer->fired ||
        !TimeReached(timer->deadline_sec, timer->deadline_nsec)) {
      continue;
    }
    uint64_t value = 1;
    if (write(timer->write_fd, &value, sizeof(value)) == (ssize_t)sizeof(value)) {
      timer->fired = 1;
    }
  }
}

static int NextTimerWait(int timeout_ms) {
  int wait_ms = timeout_ms;
  for (int i = 0; i < kMaxTimer; i++) {
    struct TimerFd* timer = &g_timers[i];
    if (!timer->used || !timer->armed || timer->fired) continue;
    int delta = DeltaMs(timer->deadline_sec, timer->deadline_nsec);
    if (wait_ms < 0 || delta < wait_ms) wait_ms = delta;
  }
  return wait_ms;
}

int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout) {
  struct EpollSet* set = FindEpoll(epfd);
  if (set == NULL || events == NULL || maxevents <= 0) {
    errno = EINVAL;
    return -1;
  }
  struct pollfd polls[kMaxInterest];
  int index[kMaxInterest];
  int count = 0;
  for (int i = 0; i < kMaxInterest; i++) {
    if (!set->items[i].used) continue;
    short bits = 0;
    if ((set->items[i].events & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) != 0) {
      bits = (short)(bits | POLLIN);
    }
    if ((set->items[i].events & EPOLLOUT) != 0) bits = (short)(bits | POLLOUT);
    polls[count].fd = set->items[i].fd;
    polls[count].events = bits;
    polls[count].revents = 0;
    index[count] = i;
    count++;
  }
  FireTimers();
  int wait_ms = NextTimerWait(timeout);
  int ready = count == 0 ? 0 : poll(polls, (nfds_t)count, wait_ms);
  FireTimers();
  if (ready == 0 && count != 0) ready = poll(polls, (nfds_t)count, 0);
  if (ready < 0) return -1;
  int out = 0;
  for (int i = 0; i < count && out < maxevents; i++) {
    if (polls[i].revents == 0) continue;
    struct Interest* item = &set->items[index[i]];
    uint32_t bits = 0;
    if ((polls[i].revents & (POLLIN | POLLPRI)) != 0) bits |= EPOLLIN;
    if ((polls[i].revents & POLLOUT) != 0) bits |= EPOLLOUT;
    if ((polls[i].revents & POLLERR) != 0) bits |= EPOLLERR;
    if ((polls[i].revents & POLLHUP) != 0) bits |= EPOLLHUP;
    events[out].events = bits;
    events[out].data = item->data;
    out++;
  }
  return out;
}

int timerfd_create(int clockid, int flags) {
  (void)clockid;
  int read_fd = -1;
  int write_fd = -1;
  if (MakePipeEnd(&read_fd, &write_fd, flags) != 0) return -1;
  for (int i = 0; i < kMaxTimer; i++) {
    if (!g_timers[i].used) {
      memset(&g_timers[i], 0, sizeof(g_timers[i]));
      g_timers[i].used = 1;
      g_timers[i].read_fd = read_fd;
      g_timers[i].write_fd = write_fd;
      return read_fd;
    }
  }
  close(read_fd);
  close(write_fd);
  errno = ENOMEM;
  return -1;
}

int timerfd_settime(int fd, int flags, const struct itimerspec* new_value,
                    struct itimerspec* old_value) {
  struct TimerFd* timer = FindTimer(fd);
  if (timer == NULL || new_value == NULL) {
    errno = EINVAL;
    return -1;
  }
  if (old_value != NULL) memset(old_value, 0, sizeof(*old_value));
  if (new_value->it_value.tv_sec == 0 && new_value->it_value.tv_nsec == 0) {
    timer->armed = 0;
    timer->fired = 0;
    return 0;
  }
  time_t sec = new_value->it_value.tv_sec;
  long nsec = new_value->it_value.tv_nsec;
  if ((flags & TFD_TIMER_ABSTIME) == 0) {
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) return -1;
    sec += now.tv_sec;
    nsec += now.tv_nsec;
  }
  while (nsec >= 1000000000L) {
    nsec -= 1000000000L;
    sec++;
  }
  timer->deadline_sec = sec;
  timer->deadline_nsec = nsec;
  timer->armed = 1;
  timer->fired = 0;
  return 0;
}

int timerfd_gettime(int fd, struct itimerspec* curr_value) {
  struct TimerFd* timer = FindTimer(fd);
  if (timer == NULL || curr_value == NULL) {
    errno = EINVAL;
    return -1;
  }
  memset(curr_value, 0, sizeof(*curr_value));
  if (!timer->armed || TimeReached(timer->deadline_sec, timer->deadline_nsec)) {
    return 0;
  }
  struct timespec now;
  if (clock_gettime(CLOCK_REALTIME, &now) != 0) return -1;
  time_t sec = timer->deadline_sec - now.tv_sec;
  long nsec = timer->deadline_nsec - now.tv_nsec;
  if (nsec < 0) {
    nsec += 1000000000L;
    sec--;
  }
  curr_value->it_value.tv_sec = sec;
  curr_value->it_value.tv_nsec = nsec;
  return 0;
}

#endif
