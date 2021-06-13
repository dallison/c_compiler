//
//  utils.s
//  c_compiler
//
//  Created by David Allison on 9/29/20.
//  Copyright © 2020 David Allison. All rights reserved.
//
#include "addresses.h"
#include "device.h"

.text

set_rega_bit:
  ORA rega
  STA rega
  STA REGA
  RTS

clear_rega_bit:
  EOR #0xff
  AND rega
  STA rega
  STA REGA
  RTS
  
.global isdigit
isdigit:
  CMP #'0'
  BCC is_false
  CMP #'9'+1
  BCC is_true
is_false:
  CLC
  RTS
is_true:
  SEC
is_true1:
  RTS

.global isalpha
isalpha:
  JSR isupper
  BCS is_true1

.global islower
islower:
  CMP #'a'
  BCC is_false
  CMP #'z'+1
  BCC is_true
  SEC
  RTS

.global isupper
isupper:
  CMP #'A'
  BCC is_false
  CMP #'Z'+1
  BCC is_true
  SEC
  RTS
  
.global toupper
toupper:
  JSR islower
  BCC is_false
  SBC #'a'-'A'      // Carry is set.
  RTS
  
.global tolower
tolower:
  JSR isupper
  BCC is_true1
  ADC #'a'-'A'-1    // Carry is set.
  RTS
  
// Is A alphanumeric.  Carry set = yes.
.global isalnum
isalnum:
  JSR isdigit
  BCS is_true1
  JMP isalpha

.global isspace
isspace:
  CMP #' '
  BEQ is_true
  CMP #9
  BEQ is_true
  CMP #10
  BEQ is_true
  CMP #13
  BNE is_false
  RTS             // Carry is set.
  
.global isxdigit
isxdigit:
  JSR isdigit
  BCS is_true1
  CMP #'A'
  BCC is_false
  CMP #'G'
  BCC is_true
  CMP #'a'
  BCC is_false
  CMP #'g'
  BCC is_true
  CLC
  RTS
  


