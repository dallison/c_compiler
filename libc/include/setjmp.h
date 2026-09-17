//
//  setjmp.h
//  c_compiler
//
//  Created by David Allison on 5/8/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef setjmp_h
#define setjmp_h

#if defined(__risc_v__) && defined(__ILP32__)
struct __jmp_buf {
  // Integer saves occupy 0..52; callee-saved FP registers start at 112.
  unsigned long long regs[26];
};
#elif defined(__risc_v__)
struct __jmp_buf {
  long regs[32];
};
#elif defined(__p_code__)
struct __jmp_buf {
  long iregs[256];
  float fregs[256];
  double dregs[256];
  long retaddr;
};
#elif defined(__6502__)
struct __jmp_buf {
  char b[6];
  int i[12];
  long l[6];
  long long x[3];
  float f[3];
  int sp;
  int fp;
  int result;
  char machine_sp;
  int retaddr;
};
#elif defined(__aarch64__)
struct __jmp_buf {
  long regs[32];
};
#elif defined(__arm__)
struct __jmp_buf {
  long regs[32];
};
#elif defined(__x86_64__)
struct __jmp_buf {
  long regs[32];
  long retaddr;
};
#elif defined(__i386__)
struct __jmp_buf {
  // ebx, esi, edi, ebp, caller esp, return address.
  long regs[8];
};
#elif defined(__xtensa__)
struct __jmp_buf {
  // Interpreter snapshot: call depth, return PC, window base, and the 64
  // physical address registers of the setjmp call frame.
  unsigned long regs[68];
};
#elif defined(__wasm32__)
struct __jmp_buf {
  // Shadow SP, continuation block, owning frame, longjmp value.
  unsigned long regs[4];
};
#elif defined(__bpf__)
struct __jmp_buf {
  long dummy[4];
};
#else
#error "Unknown architecture"
#endif

typedef struct __jmp_buf_tag {
  struct __jmp_buf buf;
} jmp_buf[1];

#ifdef __cplusplus
extern "C" {
#endif

extern int setjmp(jmp_buf buf);
extern void longjmp(jmp_buf buf, int value);

#ifdef __cplusplus
}
#endif

#endif /* setjmp_h */
