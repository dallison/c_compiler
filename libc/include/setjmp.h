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
