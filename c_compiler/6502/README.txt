Zero page regs:
sp   stack pointer
fp   frame pointer


+--------------------+
|                    |
|    Previous        |
|      frame         |
|                    |
+--------------------+    <- previous sp, new fp
|                    |
|                    |    <- variables (accessed via fp-X)
|                    |
|                    |
|                    |
+--------------------+
|   result addr      |    <- not present for leaf procs
+--------------------+
|   saved fp         |    <-- not present if no local vars
+--------------------+    <- current sp

A procedure is passed the address of where to put its result
in A.  This is in zero page.
This is held in a zero page location and saved onto
the stack frame for non-leaf procedures.

General procedure:

proc:           <- entry point for result in zero page
  LDY #0
proc_1:         <- entry point for result in memory.
  STX __result
  STY __result+1

Procedure entry (leaf): (24 bytes)
  LDA #__i2
  LDX #(frame_size + 2) & 0xff
  LDY #(frame_size + 2) >> 24
  JSR __enter_leaf or __enter_nonleaf
  
Enter function (leaf, with local vars):
  A = offset into zero page for result
  X = frame_size_lo
  Y = frame_size hi
__enter_leaf:
  SEC
  LDA sp
  TXA
  SBC #((frame_size) & 0xff)
  STA sp
  LDA sp+1
  TYA
  SBC #((frame_size ) >> 24)
  STA sp+1
  RTS

If no variables on stack, nothing to do:

Enter function (non-leaf):
  X = frame_size_lo
  Y = frame_size hi
__enter_nonleaf:
  SEC
  LDA fp
  PHA
  TXA
  SBC #((frame_size + 4) & 0xff)
  STA sp
  LDA fp+1
  PHA
  TYA
  SBC #((frame_size + 4) >> 24)
  STA sp+1
  LDY #3
  LDA __result+1
  STA (sp),Y
  DEY
  LDA __result
  STA (sp),Y
  PLA
  DEY
  STA (sp),Y
  PLA
  STA (sp)
  RTS

Returning value from procedure:
Leaf, with value in __regX:
  LDA __regX
  STA (__result)
  LDY #1
  LDA __regX+1
  STA (__result),Y
  // And more bytes
  
Non-leaf:
  LDY #3
  LDA (sp),Y
  STA __result+1
  DEY
  LDA (sp),Y
  STA __result
  DEY
  LDA __regX+1
  STA (__result),Y
  DEY                   // For last byte, remove.
  LDA __regX
  STA (__result),Y      // For last byte STA (__result)
  // More bytes.

Procedure exit:
  NOTE: Y and X are reversed from entry.
  LDY #(frame_size + 2) & 0xff
  LDX #(frame_size + 2) >> 24
  JMP __leave_leaf or __leave_nonleaf

Leaf:
  Y = frame_size_lo
   X = frame_size hi
__laave_leaf:
  CLC
  TYA
  ADC sp
  STA sp
  TXA
  ADC sp+1
  STA sp+1
  RTS
  
  
Non-leaf:
  Y = frame_size_lo
  X = frame_size hi
__leave_nonleaf:
  PHY
  LDA (sp)
  STA fp
  LDY #1
  LDA (sp),Y
  STA fp+1
  CLC
  PLA
  ADC sp
  STA sp
  TXA
  ADC sp+1
  STA sp+1
  RTS
  
Address of local variable (in tmp) (13 bytes)

  A = dest offset (in zero page)
  X = var_offset lo
  Uses t0, t1 as temps
__var_addr1:
  LDY #0
  
  A = dest offset into zero page.
  X = var_offset lo
  Y = var_offset hi
__var_addr2:
  SEC
  STX t0
  STY t1
  TAY
  LDA fp
  SBC t0
  STA 0, Y
  LDA fp+1
  SBC t1
  STA 1, Y
  RTS


Var at offset -8, result at offset 30
  LDA #30
  LDX #8
  JSR __var_addr
  
  
Var at offset -258, result at offset 84
  LDA #84
  LDX #2
  LDY #1
  JSR __var_addrb
  

Store value into 2-byte local variable (ofset -8):
  LDA #tmp
  LDX #8
  JSR __var_addr     // tmp contains address.
  
  LDA value
  STA (tmp)
  LDY #1
  LDA value+1
  STA (tmp), Y

Store value of one local (offset -8) into local at -24
  LDA #tmp1
  LDX #8
  JSR __var_addr
  
  LDA #tmp2
  LDX #24
  JSR __var_addr
  
  LDA (tmp2)
  STA (tmp1)
  LDY #1
  LDA (tmp2),Y
  STA (tmp1),Y
  
Pushing value onto stack

1 byte
  A = value
  JSR pusha
  
2 byte (X,Y = lo,hi)
  JSR pushxy
  
4 byte in reg
   LDX #reg index
   JSR push4
   
8 byte in reg
  LDX #reg index
  JSR push8

4 and 8 byte literals:
  LDX %lo(lit)
  LDY %hi(lit)
  JSR push4xy (push8xy)

1 byte variable at offset 8
  LDA #tmp1
  LDX #8
  JSR __push_var1


2 byte variable at offset 8
  LDA #tmp1
  LDX #8
  JSR __push_var2

etc. for 4 and 8 byte

  A = value
__pusha:
  DEC sp
  BNE xx
  DEC sp+1
xx:
  STA (sp)
  RTS

  
  X = value lo
  Y = value hi
__pushxy:
  SEC
  LDA sp
  SBC #2
  STA sp
  LDA sp+1
  SBC #0
  STA sp+1
  PHY
  TXA
  STA (sp)
  PLA
  LDY #1
  STA (sp),Y
  RTS

  X = index of reg in zero page
__push4:
  SEC
  LDA sp
  SBC #4
  STA sp
  LDA sp+1
  SBC #0
  STA sp+1
  LDY #0
xx:
  LDA 0,X
  STA (sp),Y
  INX
  INY
  CPY #4
  BNE xx
  RTS
  
  X = index of reg in zero page
__push8:
  SEC
  LDA sp
  SBC #8
  STA sp
  LDA sp+1
  SBC #0
  STA sp+1
  LDY #0
xx:
  LDA 0,X
  STA (sp),Y
  INX
  INY
  CPY #8
  BNE xx
  RTS
  
  
  X = amount lo
  Y = amount hi
__incsp1:        // For Y == 0
  LDY #0
__incsp2:       // For Y != 0
  CLC
  TXA
  ADC sp
  STA sp
  TYA
  ADC sp+1
  STA sp+1
  RTS
  
  Entry:
  A = ?
  X = src offset lo
  Y = srx offset hi
  
  Exit:
  t0, t0+1 = fp - (x,y)
__load_var_fp:
  SEC
  STX t0
  STY t1
  LDA fp
  SBC t0
  STA t0
  LDA fp+1
  SBC t1
  STA t0+1
  RTS
  
  X = var_offset lo
  Uses t0, t1 as temps
__push_var1:
  LDY #0
  
   Y = var_offset hi
__push_var1b:
  JSR __load_var_fp
  LDA (t0)
  JMP __pusha

  A = src offset (in zero page)
  X = var_offset lo
  Uses t0, t1 as temps
__push_var2:
  LDY #0
  
   Y = var_offset hi
__push_var2b:
  JSR __load_var_fp
  LDA (t0)
  TAX
  LDY #1
  LDA (t0),Y
  TAY
  JMP __pushxy

__push_var4a:
  LDY #0
__push_var4b:
  JSR __load_var_fp
  


Call a function with a 2 byte arg
  LDX #arg_lo
  LDY #arg_hi
  JSR __pushxy
  JSR func
  LDX #2
  JSR __incsp1
  
Call func with 2-byte variable at offset -8
  LDA #tmp
  LDX #8
  JSR __var_addr1
  LDA (tmp)
  TAX
  LDY #1
  LDA (tmp),Y
  TAY
  JSR __push2
  JSR func
  LDX #2
  JSR __incsp1
  
Jump table:
__jump_table:
// X = byte offset into table (take lower 2 bytes) In zero page.
// ASL X
// ROL X+1
// ASL X
// ROL X+1
// LDA %lo(table)
// STA __t0
// LDA %i(table)
// STA __t1
// CLC
// LDA __t0
// ADC X
// STA __t0
// LDA __t1
// ADC X+1
// STA __t1
// LDA (__t0)
// STA X
// LDY #1
// LDA (__t0),Y
// STA X+1
// JMP (X)
// .table:
//
// OR:
