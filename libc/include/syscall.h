//
//  syscall.h
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef syscall_h
#define syscall_h

#if defined(__risc_v__)
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_WRITE 4
#define SYS_READ 5
#define SYS_LSEEK 7
#define SYS_MALLOC 8
#define SYS_FREE 9
#define SYS_REALLOC 10
#define SYS_ABORT 11

#elif defined(__p_code__)

#else
#error "Unknown architecture for syscall"
#endif

#if defined(__risc_v__)
#define SYSCALL \
   return asm( \
              "mv t6, a0\n" \
              "ecall" \
              )
#elif defined(__p_code__)
#else
#error "Unknown architecture"
#endif

extern int syscall(int n, ...);

#endif /* syscall_h */
