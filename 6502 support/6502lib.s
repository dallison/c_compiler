
// Stack, argument and frame pointers.
.set sp 0x70
.set ap 0x72
.set fp 0x74

.text

.global main
main:
  // Install brk handler.
  LDA #%byte0(brk_handler)
  STA 0xfffe
  LDA #%byte1(brk_handler)
  STA 0xffff

  JMP main2
  NOP
main2:
  BRK #4
  LDA 0x55,X
  STA 0
  LDA #0xe
  STA 1
  LDA #5
  LDY #0
loop:
  JSR sub
  CLC
  ADC #10
  INY
  CPY #5
  BNE loop
  BRK #5

sub:
  STA (0),Y
  RTS

.global brk_handler
brk_handler:
// 6502 stack contains:
// s[1]: P register with B flags set
// s[2]: low byte of PC
// s[3]: high byte of PC
// The PC is the address of the next instruction, not the BRK.  The
// BRK is 2 bytes long.  RTI will return to the address on the stack.
  RTI

// Procedure entry.
// This is followed by two 16 bit words:
// 0: mask of "registers" to save
// 1: size of stack frame.
// On entry the 6502 stack contains 1 less than the
// address of the descriptor.  The next address up the
// stack is the actual return address for the procedure
// which needs to be pushed onto the runtime stack so that
// a call to __rts can return to the correct address.
.global __enter
__enter:
    // Pop the return address of the stack and add 1 to it
    // to get the address of the procedure descriptor
    CLC
    PLA
    ADC #1
    STA descriptor
    PLA
    ADC #0
    STA descriptor+1

    // Load the register save mask.
    LDY #0
    LDA (descriptor),Y
    STA mask
    INY
    LDA (descriptor),Y
    STA mask+1
    INY

    // Load the stack frame size.
    LDA (descriptor),Y
    STA stack_frame_size
    INY
    LDA (descriptor),Y
    STA stack_frame_size+1

    // Load the return address and store on the runtime stack.
    // First decrement sp by 2
    JSR __decsp_2

    // Now pull the actual return address of the 6502 stack
    // and push onto the runtime stack.  The return address
    // is pushed high byte first so the first byte popped
    // is the low byte.
    PLA
    LDY #0
    STA (sp),Y      // Low byte.
    INY
    PLA
    STA (sp),Y      // High byte.

    // Push ap onto the runtime stack.
    LDX #ap
    JSR __push_2

    // ap = sp + 4 (the stack contains the return address and saved ap)
    CLC
    LDA sp
    ADC #4
    STA ap
    LDA sp+1
    ADC #0
    STA ap+1

    // Push the first return address onto the 6502 stack and return
    // from here. The return address is the descriptor address + 3
    // It is pushed high byte first.
    CLC
    LDA descriptor
    ADC #3
    TAX
    LDA descriptor+1
    ADC #0
    PHA
    TXA
    PHA

    // TODO: save the runtime register on the runtime stack.

    // Decrement the runtime stack pointer by the stack frame size.
    SEC
    LDA sp
    SBC stack_frame_size
    STA sp
    LDA sp+1
    SBC stack_frame_size+1
    STA sp+1

    // Now return to the caller, after the descriptor
    RTS



.global __rts
__rts:

// Push a 2-byte zero-page register onto the runtime stack.
// Entry:
// X: index into zero page of first byte of register to push
.global __push_2
__push_2:
    // Decrement sp by 2
    JSR __decsp_2

    LDY #0
    LDA 0,X
    STA (sp),Y
    INY
    INX
    LDA 0,X
    STA (sp),Y
    RTS

// Like __push_2, pulls a 2-byte zero-page register off the runtime stack
.global __pull_2
__pull_2:
    LDY #0
    LDA (sp),Y
    STA 0,X
    INY
    INX
    LDA (sp),Y
    STA 0,X

    JSR __incsp_2
    RTS

.global __decsp_2
__decsp_2:
    SEC
    LDA sp
    SBC #2
    STA sp
    LDA sp+1
    SBC #0
    STA sp+1
    LDA sp
    RTS

.global __incsp_2
__incsp_2:
    CLC
    LDA sp
    ADC #2
    STA sp
    LDA sp+1
    ADC #0
    STA sp+1
    LDA sp
    RTS


  .data

.global xxx
xxx:
  .word 1234

