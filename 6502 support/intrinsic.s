//
//  intrinsic.s
//  c_compiler
//
//  Created by David Allison on 7/8/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "vars.s"
.text

.global __builtin_isalnum
.global __builtin_isalpha
.global __builtin_isblank
.global __builtin_iscntrl
.global __builtin_isdigit
.global __builtin_isgraph
.global __builtin_islower
.global __builtin_isprint
.global __builtin_ispunct
.global __builtin_isspace
.global __builtin_isupper
.global __builtin_isxdigit
.global __builtin_tolower
.global __builtin_toupper
.global __builtin_memcpy
.global __builtin_memset
.global __builtin_memcmp
.global __builtin_va_arg2
.global __builtin_va_arg4
.global __builtin_va_arg8
.global __builtin_va_arg

// All the ctype is* functions take the integer to check in X,Y and
// return 0 or 1 in A.
__builtin_isdigit:
  CPY #0
  BNE is_false
  CPX #'0'
  BCC is_false
  CPX #'9'+1
  BCC is_true
is_false:
  LDA #0
  CLC
  RTS
is_true:
  LDA #1
  SEC
  RTS

__builtin_isalpha:
  JSR __builtin_isupper
  BCS is_true

__builtin_islower:
  CPY #0
  BNE is_false
  CPX #'a'
  BCC is_false
  CPX #'z'+1
  BCC is_true
  BRA is_false

__builtin_isupper:
  CPY #0
  BNE is_false
  CPX #'A'
  BCC is_false
  CPX #'Z'+1
  BCC is_true
  BRA is_false\
  
__builtin_toupper:
  JSR __builtin_islower
  BCC is_false
  TXA
  SBC #'a'-'A'      // Carry is set.
  RTS
  
__builtin_tolower:
  JSR __builtin_isupper
  BCC is_false
  TXA
  ADC #'a'-'A'-1    // Carry is set.
  RTS
  
// Is A alphanumeric.  Carry set = yes.
__builtin_isalnum:
  JSR __builtin_isdigit
  BCS is_false
  JMP __builtin_isalpha

__builtin_isspace:
  CPY #0
  BNE is_false
  CPX #' '
  BEQ is_true
  CPX #9
  BEQ is_true
  CPX #10
  BEQ is_true
  CPX #13
  BNE is_false
  LDA #1
  RTS             // Carry is set.
  
__builtin_isxdigit:
  JSR __builtin_isdigit
  BCS is_true
  CPX #'A'
  BCC is_false
  CPX #'G'
  BCC is_true
  CPX #'a'
  BCC is_false
  CPX #'g'
  BCC is_true
  LDA #0
  CLC
  RTS

__builtin_isblank:
  CPY #0
  BNE is_false
  CPX #9
  BEQ is_true
  CPX #' '
  BEQ is_true1
is_false1:
  LDA #0
  CLC
  RTS
is_true1:
  LDA #1
  SEC
  RTS


__builtin_iscntrl:
  CPY #0
  BNE is_false1
  CPX #0x20
  BCC is_true1
  BCS is_false1

__builtin_isgraph:
  CPY #0
  BNE is_false1
  CPX #' '
  BNE is_true1
  BEQ is_false1

__builtin_isprint:
  CPY #0
  BNE is_false1
  CPX #127
  BCS is_false1
  CPX #' '
  BCS is_true1
  BCC is_false1

// Punctuation chars:
// 0x21...0x2f
// 0x40
// 0x5b...0x60
// 0x7b...0x7e
__builtin_ispunct:
  CPY #0
  BNE is_false1
  CPX #0x21
  BCC is_false1
  CPX #0x30
  BCC is_true1
  CPX #0x40
  BEQ is_true1
  CPX #0x5b
  BCC is_false1
  CPX #0x61
  BCC is_true1
  CPX #0x7b
  BCC is_true1
  CPX #0x7f
  BCC is_true1
  BCS is_false1

// __mem_dest
// __mem_src
// __mem_size
// Result in __mem_dest
__builtin_memcpy:
  LDA __mem_dest
  STA __t0
  LDA __mem_dest+1
  STA __t1
memcpy_large_page_loop:
  LDX __mem_size+1
  BEQ memcpy_small

  // X is the high byte of size and will be >0
  // Copy one page from __mem_src to __mem_dest.
  LDY #0
memcpy_large_loop:
  LDA (__mem_src),Y
  STA (__t0),Y
  INY
  BNE memcpy_large_loop

  // Page copied, decrement high byte and do another.
  DEC __mem_size+1
  INC __t1
  INC __mem_src+1
  BRA memcpy_large_page_loop

// Less than one page, use indirect Y for fast copy.
memcpy_small:
  LDY #0
memcpy_small_loop:
  CPY __mem_size
  BEQ end_memcpy
  LDA (__mem_src),Y
  STA (__t0),Y
  INY
  BRA memcpy_small_loop
end_memcpy:
  RTS


// __mem_dest
// __mem_src (contains value to set in lower byte)
// __mem_size
// Result in __mem_dest
__builtin_memset:
  LDA __mem_dest
  STA __t0
  LDA __mem_dest+1
  STA __t1
  LDA __mem_src
memset_large_page_loop:
  LDX __mem_size+1
  BEQ memset_small

  LDY #0
memset_large_loop:
  STA (__t0), Y
  INY
  BNE memset_large_loop

  // Page set, decrement high byte and do another.
  DEC __mem_size+1
  INC __t1
  BRA memset_large_page_loop

  // Less than one page, use indirect Y for fast set.
memset_small:
  LDY #0
memset_small_loop:
  CPY __mem_size
  BEQ end_memset
  STA (__t0),Y
  INY
  BRA memset_small_loop
end_memset:
  RTS

// __mem_dest (arg 0)
// __mem_src (arg 1)
// __mem_size bytes
// Result in X,Y
__builtin_memcmp:
  LDA __mem_size+1
  BEQ memcmp_small

  LDY #0
memcmp_large_loop:
  LDA __mem_size
  ORA __mem_size+1
  BEQ memcmp_end1
  SEC
  LDA (__mem_dest),Y
  SBC (__mem_src),Y
  BNE memcmp_end
  INC __mem_dest
  BNE mcmp1
  INC __mem_dest+1
mcmp1:
  INC __mem_src
  BNE mcmp2
  INC __mem_src+1
mcmp2:
  DEC __mem_size
  BPL memcmp_large_loop
  DEC __mem_size+1
  BRA memcmp_large_loop

memcmp_small:
  LDY #0
memcmp_loop:
  CPY __mem_size
  BEQ memcmp_end1
  SEC
  LDA (__mem_dest), Y
  SBC (__mem_src), Y
  BNE memcmp_end
  INY
  BRA memcmp_loop
memcmp_end1:
  LDA #0
memcmp_end:
  TAX               // Low byte of result in X
  BPL memcmp_end2
  LDY #255
  RTS
memcmp_end2:
  LDY #0
  RTS



// Input:
// __mem_src,+1: address of va_list
// __mem_dest,+1: destination address
// X,Y: number of bytes.
__builtin_va_arg:
  STX __mem_size
  STY __mem_size+1

  // Load the contents of va_list and push onto stack.
  LDA (__mem_src)
  PHA
  LDY #1
  LDA (__mem_src), Y
  PHA

  // Increment contents of va_list by mem_size.
  CLC
  LDA (__mem_src)
  ADC __mem_size
  STA (__mem_src)
  LDA (__mem_src),Y
  ADC __mem_size+1
  STA (__mem_src),Y

  // Put original contents of va_list into mem_src for the memcpy.
  PLA
  STA __mem_src+1
  PLA
  STA __mem_src
  
  // __mem_src contains the address held in the va_list: an address on the stack.
  // __mem_size contains the number of bytes to read into (__mem_dest)
  JMP __builtin_memcpy

__builtin_va_arg2:
  // Load the contents of va_list and push onto stack.
  LDA (__mem_src)
  PHA
  LDY #1
  LDA (__mem_src), Y
  PHA

  // Increment contents of va_list by 2.
  CLC
  LDA (__mem_src)
  ADC #2
  STA (__mem_src)
  LDA (__mem_src),Y
  ADC #0
  STA (__mem_src),Y

  // Put original contents of va_list into mem_src for the memcpy.
  PLA
  STA __mem_src+1
  PLA
  STA __mem_src
  LDA (__mem_src)
  STA (__mem_dest)
  LDA (__mem_src), Y
  STA (__mem_dest), Y
  RTS


__builtin_va_arg4:
  // Load the contents of va_list and push onto stack.
  LDA (__mem_src)
  PHA
  LDY #1
  LDA (__mem_src), Y
  PHA

  // Increment contents of va_list by 4.
  CLC
  LDA (__mem_src)
  ADC #4
  STA (__mem_src)
  LDA (__mem_src),Y
  ADC #0
  STA (__mem_src),Y

  // Put original contents of va_list into mem_src for the memcpy.
  PLA
  STA __mem_src+1
  PLA
  STA __mem_src
  LDA #4
  STA __mem_size
  STZ __mem_size+1
  JMP __builtin_memcpy

__builtin_va_arg8:
  // Load the contents of va_list and push onto stack.
  LDA (__mem_src)
  PHA
  LDY #1
  LDA (__mem_src), Y
  PHA

  // Increment contents of va_list by 4.
  CLC
  LDA (__mem_src)
  ADC #8
  STA (__mem_src)
  LDA (__mem_src),Y
  ADC #0
  STA (__mem_src),Y

  // Put original contents of va_list into mem_src for the memcpy.
  PLA
  STA __mem_src+1
  PLA
  STA __mem_src
  LDA #8
  STA __mem_size
  STZ __mem_size+1
  JMP __builtin_memcpy
