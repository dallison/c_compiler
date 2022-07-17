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
#define SYS_EXIT 12

#elif defined(__p_code__)

#elif defined(__W65C02__)
#define SYS_EXIT 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_WRITE 4
#define SYS_READ 5
#define SYS_LSEEK 7
#define SYS_ABORT 8
#else
#error "Unknown architecture for syscall"
#endif

#if defined(__risc_v__)
#define SYSCALL \
   return asm( \
              "mv t6, a0\n" \
              "ecall" \
              )
extern int syscall(int n, ...);
#elif defined(__p_code__)
extern int syscall(int n, ...);
#elif defined(__W65C02__)
extern int syscall(int n, ...);
#else
#error "Unknown architecture"
#endif


#endif /* syscall_h */
