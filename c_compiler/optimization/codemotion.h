//
//  codemotion.h
//  c_compiler_library
//
//  Created by David Allison on 7/6/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef codemotion_h
#define codemotion_h

#include "codegen.h"

// This optimization moves loop-invariant code to the loop's dedicated
// preheader.  Nested loops are processed inside-out so an invariant can
// climb from an inner preheader into the enclosing loop.
//
// It does, however, increase register pressure and might lead to more
// register spilling.
//
// On average, code motion does increase the speed of the program though.
//
// For example, this function has a candidate for code motion:
// void foo(int x, int y) {
//   int sum = 0;
//   while (y < 100) {
//     sum += x << 4;  // <-- x << 4 is loop invariant.
//     y += 2;
//   }
//   dave(sum);
// }
//
// In this case, the expression x << 4 doesn't have any dependency
// in things inside the loop and can be hoisted to a dominator
// block.
//
// Here's the (pretty optimal) RISC-V code generated for it:
//
//foo:
//
//  // *** Basic block 0
//
//  .global dave
//  // Leaf procedure, no stack frame generated
//  // Local vars at offset -16(s0)
//  // End of stack frame
//  mv          t0, a1
//  mv          t1, x0
//  slli        t2, a0, 4
//  li          t3, 100    // 0x64 ASCII 'd'
//  bge         t0, t3, .foo_label_26
//
//  // *** Basic block 1
//
//.foo_label_20:
//  add         t1, t1, t2
//  addi        t0, t0, 2
//  blt         t0, t3, .foo_label_20
//
//  // *** Basic block 2
//
//.foo_label_26:
//  mv          a0, t1
//  // Restored registers.
//  j           dave
//.func_end_foo:
//  .size foo, .func_end_foo-foo

void CodeMotionOptimization(Generator* gen);

#endif /* codemotion_h */
