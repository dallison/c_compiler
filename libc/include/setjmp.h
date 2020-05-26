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
#elif defined(__6502__)
#else
#error "Unknown architecture"
#endif

typedef struct jmp_buf {
  struct __jmp_buf buf;
} jmp_buf[1];

extern int setjmp(jmp_buf buf);
extern void longjmp(jmp_buf buf, int value);

#endif /* setjmp_h */
