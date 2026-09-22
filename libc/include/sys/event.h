#ifndef davecc_sys_event_h
#define davecc_sys_event_h

#include <stdint.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVFILT_READ   (-1)
#define EVFILT_WRITE  (-2)
#define EVFILT_AIO    (-3)
#define EVFILT_VNODE  (-4)
#define EVFILT_PROC   (-5)
#define EVFILT_SIGNAL (-6)
#define EVFILT_TIMER  (-7)
#define EVFILT_USER   (-10)

#define EV_ADD     0x0001
#define EV_DELETE  0x0002
#define EV_ENABLE  0x0004
#define EV_DISABLE 0x0008
#define EV_ONESHOT 0x0010
#define EV_CLEAR   0x0020
#define EV_RECEIPT 0x0040
#define EV_DISPATCH 0x0080
#define EV_EOF     0x8000
#define EV_ERROR   0x4000

#define NOTE_TRIGGER  0x01000000
#define NOTE_FFNOP    0x00000000
#define NOTE_FFAND    0x40000000
#define NOTE_FFOR     0x80000000
#define NOTE_FFCOPY   0xc0000000
#define NOTE_LOWAT    0x00000001
#define NOTE_SECONDS  0x00000001
#define NOTE_USECONDS 0x00000002
#define NOTE_NSECONDS 0x00000004
#define NOTE_ABSOLUTE 0x00000008

#ifndef __UINTPTR_T
typedef unsigned long uintptr_t;
#define __UINTPTR_T
#endif
#ifndef __INTPTR_T
typedef long intptr_t;
#define __INTPTR_T
#endif

struct kevent {
  uintptr_t ident;
  int16_t filter;
  uint16_t flags;
  uint32_t fflags;
  intptr_t data;
  void* udata;
};

#define EV_SET(kevp, a, b, c, d, e, f) \
  do {                                 \
    struct kevent* __kevp__ = (kevp);  \
    __kevp__->ident = (a);             \
    __kevp__->filter = (b);            \
    __kevp__->flags = (c);             \
    __kevp__->fflags = (d);            \
    __kevp__->data = (e);              \
    __kevp__->udata = (void*)(f);      \
  } while (0)

int kqueue(void);
int kevent(int kq, const struct kevent* changelist, int nchanges,
           struct kevent* eventlist, int nevents,
           const struct timespec* timeout);

#ifdef __cplusplus
}
#endif

#endif
