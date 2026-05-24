//
//  syscall.c
//  c_compiler
//
//  Created by David Allison on 2/15/20.
//  Copyright © 2020 David Allison. All rights reserved.
//


#if defined(__risc_v__)
// Args are:
// a0: syscall number
// a1...: args to syscall
int syscall(int n, ...) {
   return asm(
              "mv t6, a0\n"
              "ecall"
              );
}
#elif defined(__p_code__)
#elif defined(__x86_64__)
// Implemented in x86_64 support/syscall.s
#elif defined(__W65C02__)
// Implemented in 6502runtime.s
#else
#error "Unknown architecture"
#endif
