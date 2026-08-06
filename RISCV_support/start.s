//
//  start.s
//  c_compiler
//
//  Created by David Allison on 1/18/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

.text

.global main

.global _start
_start:
  call main
  li t6, 1
  ecall

