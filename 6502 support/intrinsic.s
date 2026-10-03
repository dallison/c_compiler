//
//  intrinsic.s
//  c_compiler
//
//  Created by David Allison on 7/8/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "vars.s"
// Functions are emitted in per-symbol ELF sections.

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
// return 0 or 1 in A.  Carry is set for a true result.  Every branch
// stays inside its own section: the linker splits these sections, so a
// branch to a label in another function lands a few bytes away and loops.
.section ".text.__builtin_isdigit", "ax", @progbits
__builtin_isdigit:
  CPY #0
  BNE isdigit_no
  CPX #'0'
  BCC isdigit_no
  CPX #'9'+1
  BCC isdigit_yes
isdigit_no:
  LDA #0
  CLC
  RTS
isdigit_yes:
  LDA #1
  SEC
  RTS

.section ".text.__builtin_isalpha", "ax", @progbits
__builtin_isalpha:
  JSR __builtin_isupper
  BCC isalpha_lower
  RTS
isalpha_lower:
  JMP __builtin_islower

.section ".text.__builtin_islower", "ax", @progbits
__builtin_islower:
  CPY #0
  BNE islower_no
  CPX #'a'
  BCC islower_no
  CPX #'z'+1
  BCS islower_no
  LDA #1
  SEC
  RTS
islower_no:
  LDA #0
  CLC
  RTS

.section ".text.__builtin_isupper", "ax", @progbits
__builtin_isupper:
  CPY #0
  BNE isupper_no
  CPX #'A'
  BCC isupper_no
  CPX #'Z'+1
  BCS isupper_no
  LDA #1
  SEC
  RTS
isupper_no:
  LDA #0
  CLC
  RTS

.section ".text.__builtin_toupper", "ax", @progbits
__builtin_toupper:
  JSR __builtin_islower
  BCC toupper_keep
  TXA
  SBC #'a'-'A'      // Carry is set.
  RTS
toupper_keep:
  TXA
  RTS

.section ".text.__builtin_tolower", "ax", @progbits
__builtin_tolower:
  JSR __builtin_isupper
  BCC tolower_keep
  TXA
  ADC #'a'-'A'-1    // isupper returned with carry set.
  RTS
tolower_keep:
  TXA
  RTS

// Carry set means the character is alphanumeric.
.section ".text.__builtin_isalnum", "ax", @progbits
__builtin_isalnum:
  JSR __builtin_isdigit
  BCC isalnum_alpha
  RTS
isalnum_alpha:
  JMP __builtin_isalpha

.section ".text.__builtin_isspace", "ax", @progbits
__builtin_isspace:
  CPY #0
  BNE isspace_no
  CPX #' '
  BEQ isspace_yes
  CPX #9
  BCC isspace_no
  CPX #14
  BCS isspace_no
isspace_yes:
  LDA #1
  SEC
  RTS
isspace_no:
  LDA #0
  CLC
  RTS

.section ".text.__builtin_isxdigit", "ax", @progbits
__builtin_isxdigit:
  JSR __builtin_isdigit
  BCC isxdigit_letter
  RTS
isxdigit_letter:
  CPX #'A'
  BCC isxdigit_no
  CPX #'G'
  BCC isxdigit_yes
  CPX #'a'
  BCC isxdigit_no
  CPX #'g'
  BCS isxdigit_no
isxdigit_yes:
  LDA #1
  SEC
  RTS
isxdigit_no:
  LDA #0
  CLC
  RTS

.section ".text.__builtin_isblank", "ax", @progbits
__builtin_isblank:
  CPY #0
  BNE isblank_no
  CPX #9
  BEQ isblank_yes
  CPX #' '
  BNE isblank_no
isblank_yes:
  LDA #1
  SEC
  RTS
isblank_no:
  LDA #0
  CLC
  RTS

.section ".text.__builtin_iscntrl", "ax", @progbits
__builtin_iscntrl:
  CPY #0
  BNE iscntrl_no
  CPX #0x20
  BCC iscntrl_yes
  CPX #0x7f
  BNE iscntrl_no
iscntrl_yes:
  LDA #1
  SEC
  RTS
iscntrl_no:
  LDA #0
  CLC
  RTS

.section ".text.__builtin_isgraph", "ax", @progbits
__builtin_isgraph:
  CPY #0
  BNE isgraph_no
  CPX #0x21
  BCC isgraph_no
  CPX #0x7f
  BCS isgraph_no
  LDA #1
  SEC
  RTS
isgraph_no:
  LDA #0
  CLC
  RTS

.section ".text.__builtin_isprint", "ax", @progbits
__builtin_isprint:
  CPY #0
  BNE isprint_no
  CPX #' '
  BCC isprint_no
  CPX #0x7f
  BCS isprint_no
  LDA #1
  SEC
  RTS
isprint_no:
  LDA #0
  CLC
  RTS

// Punctuation chars:
// 0x21...0x2f
// 0x40
// 0x5b...0x60
// 0x7b...0x7e
.section ".text.__builtin_ispunct", "ax", @progbits
__builtin_ispunct:
  CPY #0
  BNE ispunct_no
  CPX #0x21
  BCC ispunct_no
  CPX #0x30
  BCC ispunct_yes
  CPX #0x3a
  BCC ispunct_no
  CPX #0x41
  BCC ispunct_yes
  CPX #0x5b
  BCC ispunct_no
  CPX #0x61
  BCC ispunct_yes
  CPX #0x7b
  BCC ispunct_no
  CPX #0x7f
  BCS ispunct_no
ispunct_yes:
  LDA #1
  SEC
  RTS
ispunct_no:
  LDA #0
  CLC
  RTS

// __mem_dest
// __mem_src
// __mem_size
// Result in __mem_dest
.section ".text.__builtin_memcpy", "ax", @progbits
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
#ifdef __65c02__
  BRA memcpy_large_page_loop
#else
  JMP memcpy_large_page_loop
#endif

// Less than one page, use indirect Y for fast copy.
memcpy_small:
  LDY #0
memcpy_small_loop:
  CPY __mem_size
  BEQ end_memcpy
  LDA (__mem_src),Y
  STA (__t0),Y
  INY
#ifdef __65c02__
  BRA memcpy_small_loop
#else
  JMP memcpy_small_loop
#endif
end_memcpy:
  RTS


// __mem_dest
// __mem_src (contains value to set in lower byte)
// __mem_size
// Result in __mem_dest
.section ".text.__builtin_memset", "ax", @progbits
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
#ifdef __65c02__
  BRA memset_large_page_loop
#else
  JMP memset_large_page_loop
#endif

  // Less than one page, use indirect Y for fast set.
memset_small:
  LDY #0
memset_small_loop:
  CPY __mem_size
  BEQ end_memset
  STA (__t0),Y
  INY
#ifdef __65c02__
  BRA memset_small_loop
#else
  JMP memset_small_loop
#endif
end_memset:
  RTS

// __mem_dest (arg 0)
// __mem_src (arg 1)
// __mem_size bytes
// Result in X,Y
.section ".text.__builtin_memcmp", "ax", @progbits
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
#ifdef __65c02__
  BRA memcmp_large_loop
#else
  JMP memcmp_large_loop
#endif

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
#ifdef __65c02__
  BRA memcmp_loop
#else
  JMP memcmp_loop
#endif
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
.section ".text.__builtin_va_arg", "ax", @progbits
__builtin_va_arg:
  STX __mem_size
  STY __mem_size+1

  // Load the contents of va_list and push onto stack.
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  PHA
  LDY #1
  LDA (__mem_src), Y
  PHA

  // Increment contents of va_list by mem_size.
  CLC
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  ADC __mem_size
#ifdef __65c02__
  STA (__mem_src)
#else
  PHP
  STY __nmos_tmp
  LDY #0
  STA (__mem_src),Y
  LDY __nmos_tmp
  PLP
#endif
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

.section ".text.__builtin_va_arg2", "ax", @progbits
__builtin_va_arg2:
  // Load the contents of va_list and push onto stack.
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  PHA
  LDY #1
  LDA (__mem_src), Y
  PHA

  // Increment contents of va_list by 2.
  CLC
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  ADC #2
#ifdef __65c02__
  STA (__mem_src)
#else
  PHP
  STY __nmos_tmp
  LDY #0
  STA (__mem_src),Y
  LDY __nmos_tmp
  PLP
#endif
  LDA (__mem_src),Y
  ADC #0
  STA (__mem_src),Y

  // Put original contents of va_list into mem_src for the memcpy.
  PLA
  STA __mem_src+1
  PLA
  STA __mem_src
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
#ifdef __65c02__
  STA (__mem_dest)
#else
  PHP
  STY __nmos_tmp
  LDY #0
  STA (__mem_dest),Y
  LDY __nmos_tmp
  PLP
#endif
  LDA (__mem_src), Y
  STA (__mem_dest), Y
  RTS


.section ".text.__builtin_va_arg4", "ax", @progbits
__builtin_va_arg4:
  // Load the contents of va_list and push onto stack.
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  PHA
  LDY #1
  LDA (__mem_src), Y
  PHA

  // Increment contents of va_list by 4.
  CLC
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  ADC #4
#ifdef __65c02__
  STA (__mem_src)
#else
  PHP
  STY __nmos_tmp
  LDY #0
  STA (__mem_src),Y
  LDY __nmos_tmp
  PLP
#endif
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
#ifdef __65c02__
  STZ __mem_size+1
#else
  PHA
  LDA #0
  STA __mem_size+1
  STA __nmos_tmp
  PLA
  BIT __nmos_tmp
#endif
  JMP __builtin_memcpy

.section ".text.__builtin_va_arg8", "ax", @progbits
__builtin_va_arg8:
  // Load the contents of va_list and push onto stack.
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  PHA
  LDY #1
  LDA (__mem_src), Y
  PHA

  // Increment contents of va_list by 4.
  CLC
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  ADC #8
#ifdef __65c02__
  STA (__mem_src)
#else
  PHP
  STY __nmos_tmp
  LDY #0
  STA (__mem_src),Y
  LDY __nmos_tmp
  PLP
#endif
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
#ifdef __65c02__
  STZ __mem_size+1
#else
  PHA
  LDA #0
  STA __mem_size+1
  STA __nmos_tmp
  PLA
  BIT __nmos_tmp
#endif
  JMP __builtin_memcpy
