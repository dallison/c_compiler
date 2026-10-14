#ifndef _UCONTEXT_H
#define _UCONTEXT_H

#if defined(__DAVECC_NATIVE_DARWIN__)
#include_next <ucontext.h>
#else
#include <signal.h>

#if defined(__x86_64__)
typedef long greg_t;
typedef greg_t gregset_t[23];

enum {
  REG_R8 = 0,
  REG_R9,
  REG_R10,
  REG_R11,
  REG_R12,
  REG_R13,
  REG_R14,
  REG_R15,
  REG_RDI,
  REG_RSI,
  REG_RBP,
  REG_RBX,
  REG_RDX,
  REG_RAX,
  REG_RCX,
  REG_RSP,
  REG_RIP,
};

typedef struct {
  gregset_t gregs;
  void* fpregs;
  unsigned long __reserved[8];
} mcontext_t;

typedef struct ucontext_t {
  unsigned long uc_flags;
  struct ucontext_t* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  sigset_t uc_sigmask;
} ucontext_t;
#else
typedef struct {
  unsigned long __opaque[64];
} mcontext_t;

typedef struct ucontext_t {
  unsigned long uc_flags;
  struct ucontext_t* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  sigset_t uc_sigmask;
} ucontext_t;
#endif
#endif  // __DAVECC_NATIVE_DARWIN__

#endif
