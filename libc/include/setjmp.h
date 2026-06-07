//
//  setjmp.h
//  c_compiler
//
//  Created by David Allison on 5/8/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef setjmp_h
#define setjmp_h

#if defined(__risc_v__)
struct __jmp_buf {
  long regs[32];
};
#elif defined(__pcode__)
#elif defined(__W65C02__)
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

typedef struct jmp_buf {
  struct __jmp_buf buf;
} jmp_buf[1];

extern int setjmp(jmp_buf buf);
extern void longjmp(jmp_buf buf, int value);

#endif /* setjmp_h */
