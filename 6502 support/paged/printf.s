#include "vars.s"
#include "layout.h"

// 6502 printf/scanf for the BBC sideways image.
//
// The caller has pushed arguments right to left, so the first argument is at
// __sp. X/Y is the address of the int result. This file does not use
// __enter: the software stack and __fp are restored around every C call,
// and the callee-saved zero-page registers are copied into the top of the
// frame. NMOS opcodes only.
//
// Conversions: c s d i u x X o p n f e g a (and the uppercase forms),
// flags -+ #0 and space, width and precision including *, lengths
// hh h l ll z j t. scanf adds those conversions plus a scanset.
// %f/%e/%g/%a call the C float printers. The digit buffer is sideways
// RAM, so a printf that calls printf while a conversion is in progress
// would share it; fputc does not do that.

// First 24 bytes are the scan/print state. The next 48 hold the
// callee-saved registers (b2-b5, i4-i7, l2-l3, x1-x2, f1-f3), which a
// C caller may still be using. Arguments begin just above that.
#define STATE 24
#define ABI_SAVE 48
#define FRAME (STATE + ABI_SAVE)
#define F_RES 0
#define F_FMT 2
#define F_AP 4
#define F_CNT 6
#define F_KIND 8
#define F_FL 9
#define F_PTR 10
#define F_LIM 12
#define F_WID 14
#define F_PRE 16
#define F_LEN 18
#define F_ERR 19
#define F_TMP 20
#define F_NUL 23

#define PF_LEFT 0x01
#define PF_PLUS 0x02
#define PF_SPACE 0x04
#define PF_ZERO 0x08
#define PF_ALT 0x10
#define PF_NEG 0x20
#define PF_UPPER 0x40

#define S_ASN 6
#define S_NRD 8
#define S_KIND 10
#define S_STAT 11
#define S_SUP 12
#define S_LEN 13
#define S_PTR 14
#define S_WID 16

#define LEN_DEF 0
#define LEN_HH 1
#define LEN_H 2
#define LEN_L 3
#define LEN_LL 4

#define KIND_FILE 0
#define KIND_STR 1
#define STAT_MATCH 1
#define STAT_INPUT 2

#define FD_OFF 0
#define BUF_OFF 2
#define BUFSIZE_OFF 4
#define RINDEX_OFF 6
#define RLIMIT_OFF 8
#define MODE_OFF 12
#define UNGET_OFF 14
#define EOF_OFF 15
#define ERR_OFF 16
#define ORIENT_OFF 17
#define UNGET_BUF_OFF 18

.comm __pf_val, 8
.comm __pf_digs, 24
.comm __pf_ndig, 1
.comm __pf_fbuf, 48
.comm __pf_repn, 2
.comm __pf_ptr, 2
.comm __pf_len, 2
.comm __pf_ch, 1
.comm __pf_zero, 1
.comm __pf_i, 1
.comm __pf_set, 32
.comm __pf_ftmp, 4

.section ".text.__pf_engine", "ax", @progbits

.global __pf_alloc
.global __pf_free
.global __pf_run
.global __pf_scan
.global __pf_args
.global __pf_copyarg
.global __pf_ldarg
.global getc
.type getc, @function

// X = frame size. __t0/__t0+1 = result address. Zeros the frame and
// stores the result at offset 0. A printf/scanf frame also keeps the
// callee-saved registers in the bytes above the 24-byte state.
__pf_alloc:
  STX __t2
  SEC
  LDA __sp
  SBC __t2
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  LDY __t2
  DEY
  LDA #0
__pf_zloop:
  STA (__sp),Y
  DEY
  BPL __pf_zloop
  LDY #0
  LDA __t0
  STA (__sp),Y
  INY
  LDA __t0+1
  STA (__sp),Y
  CPX #FRAME
  BCC __pf_alloc_out
  JSR __pf_abi_store
__pf_alloc_out:
  RTS

// Copy b2-b5, i4-i7, l2-l3, x1-x2, f1-f3 to frame offset STATE.
__pf_abi_store:
  LDX #0
__pf_abi_b:
  TXA
  CLC
  ADC #STATE
  TAY
  LDA __b2,X
  STA (__sp),Y
  INX
  CPX #4
  BNE __pf_abi_b
  LDX #0
__pf_abi_i:
  TXA
  CLC
  ADC #STATE+4
  TAY
  LDA __i4,X
  STA (__sp),Y
  INX
  CPX #8
  BNE __pf_abi_i
  LDX #0
__pf_abi_l:
  TXA
  CLC
  ADC #STATE+12
  TAY
  LDA __l2,X
  STA (__sp),Y
  INX
  CPX #8
  BNE __pf_abi_l
  LDX #0
__pf_abi_x:
  TXA
  CLC
  ADC #STATE+20
  TAY
  LDA __x1,X
  STA (__sp),Y
  INX
  CPX #16
  BNE __pf_abi_x
  LDX #0
__pf_abi_f:
  TXA
  CLC
  ADC #STATE+36
  TAY
  LDA __f1,X
  STA (__sp),Y
  INX
  CPX #12
  BNE __pf_abi_f
  RTS

__pf_abi_load:
  LDX #0
__pf_abi_lb:
  TXA
  CLC
  ADC #STATE
  TAY
  LDA (__sp),Y
  STA __b2,X
  INX
  CPX #4
  BNE __pf_abi_lb
  LDX #0
__pf_abi_li:
  TXA
  CLC
  ADC #STATE+4
  TAY
  LDA (__sp),Y
  STA __i4,X
  INX
  CPX #8
  BNE __pf_abi_li
  LDX #0
__pf_abi_ll:
  TXA
  CLC
  ADC #STATE+12
  TAY
  LDA (__sp),Y
  STA __l2,X
  INX
  CPX #8
  BNE __pf_abi_ll
  LDX #0
__pf_abi_lx:
  TXA
  CLC
  ADC #STATE+20
  TAY
  LDA (__sp),Y
  STA __x1,X
  INX
  CPX #16
  BNE __pf_abi_lx
  LDX #0
__pf_abi_lf:
  TXA
  CLC
  ADC #STATE+36
  TAY
  LDA (__sp),Y
  STA __f1,X
  INX
  CPX #12
  BNE __pf_abi_lf
  RTS

// X = frame size.
__pf_free:
  CPX #FRAME
  BCC __pf_free_add
  TXA
  PHA
  JSR __pf_abi_load
  PLA
  TAX
__pf_free_add:
  TXA
  CLC
  ADC __sp
  STA __sp
  LDA #0
  ADC __sp+1
  STA __sp+1
  RTS

// A C callee's epilogue does not hand __sp and __fp back to a caller that
// has no __enter frame. Snapshot them around the call. The snapshot lives
// in __pf_digs, which the callee does not use.
__pf_c_save:
  LDA __sp
  STA __pf_digs
  LDA __sp+1
  STA __pf_digs+1
  LDA __fp
  STA __pf_digs+2
  LDA __fp+1
  STA __pf_digs+3
  RTS

__pf_c_restore:
  LDA __pf_digs
  STA __sp
  LDA __pf_digs+1
  STA __sp+1
  LDA __pf_digs+2
  STA __fp
  LDA __pf_digs+3
  STA __fp+1
  RTS

// X = argument offset, word returned in __t0.
__pf_ldarg:
  TXA
  CLC
  ADC #FRAME
  TAY
  LDA (__sp),Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t0+1
  RTS

// X = argument offset, Y = frame offset.
__pf_copyarg:
  STY __t2
  TXA
  CLC
  ADC #FRAME
  TAY
  LDA (__sp),Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t0+1
  LDY __t2
  LDA __t0
  STA (__sp),Y
  INY
  LDA __t0+1
  STA (__sp),Y
  RTS

// X = fmt offset. A = 1 to load a va_list from the following word.
__pf_args:
  STA __pf_ch
  TXA
  CLC
  ADC #FRAME
  TAY
  LDA (__sp),Y
  TAX
  INY
  LDA (__sp),Y
  LDY #F_FMT+1
  STA (__sp),Y
  DEY
  TXA
  STA (__sp),Y
  // ap = __sp + FRAME + fmt_offset + 2. fmt_offset is still needed.
  // Recompute from the fmt pointer's argument slot via __pf_ch and X.
  // The offset was consumed. Recover it from the address we built:
  // We still have the original offset only if we saved it. Save path:
  RTS

// The offset is passed in X and also held by the caller in __pf_i.
// A is the indirect flag. This entry is the one the wrappers use.
__pf_bind:
  STA __pf_ch
  STX __pf_i
  TXA
  CLC
  ADC #FRAME
  TAY
  LDA (__sp),Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t0+1
  LDY #F_FMT
  LDA __t0
  STA (__sp),Y
  INY
  LDA __t0+1
  STA (__sp),Y
  CLC
  LDA __pf_i
  ADC #FRAME+2
  ADC __sp
  STA __t0
  LDA __sp+1
  ADC #0
  STA __t0+1
  LDA __pf_ch
  BEQ __pf_bind_store
  LDY #0
  LDA (__t0),Y
  TAX
  INY
  LDA (__t0),Y
  STA __t0+1
  STX __t0
__pf_bind_store:
  LDY #F_AP
  LDA __t0
  STA (__sp),Y
  INY
  LDA __t0+1
  STA (__sp),Y
  RTS

__pf_ldw:
  LDA (__sp),Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t0+1
  RTS

__pf_stw:
  LDA __t0
  STA (__sp),Y
  INY
  LDA __t0+1
  STA (__sp),Y
  RTS

__pf_incw:
  LDA (__sp),Y
  CLC
  ADC #1
  STA (__sp),Y
  INY
  LDA (__sp),Y
  ADC #0
  STA (__sp),Y
  RTS

__pf_next:
  LDY #F_FMT
  JSR __pf_ldw
  LDY #0
  LDA (__t0),Y
  BEQ __pf_next_done
  INC __t0
  BNE __pf_next_store
  INC __t0+1
__pf_next_store:
  LDY #F_FMT
  JSR __pf_stw
  // The character was overwritten. Reload it from the previous address.
  LDA __t0
  BNE __pf_next_back
  DEC __t0+1
__pf_next_back:
  DEC __t0
  LDY #0
  LDA (__t0),Y
__pf_next_done:
  RTS

__pf_arg:
  // X = byte count. Copies to __pf_val and advances ap. Upper bytes cleared.
  STX __pf_i
  LDX #7
  LDA #0
__pf_arg_clr:
  STA __pf_val,X
  DEX
  BPL __pf_arg_clr
  LDY #F_AP
  JSR __pf_ldw
  LDX #0
__pf_arg_cp:
  CPX __pf_i
  BCS __pf_arg_adv
  TXA
  TAY
  LDA (__t0),Y
  STA __pf_val,X
  INX
  JMP __pf_arg_cp
__pf_arg_adv:
  CLC
  LDA __t0
  ADC __pf_i
  STA __t0
  LDA __t0+1
  ADC #0
  STA __t0+1
  LDY #F_AP
  JMP __pf_stw

__pf_iszero:
  LDX #8
__pf_iszero_loop:
  DEX
  LDA __pf_val,X
  BNE __pf_iszero_no
  CPX #0
  BNE __pf_iszero_loop
  LDA #0
  RTS
__pf_iszero_no:
  LDA #1
  RTS

__pf_neg:
  LDX #0
  SEC
__pf_neg_loop:
  LDA #0
  SBC __pf_val,X
  STA __pf_val,X
  INX
  CPX #8
  BNE __pf_neg_loop
  RTS

// Divide the 8-byte value by the byte in __pf_ch. Remainder in A.
__pf_div:
  LDA #0
  STA __pf_zero
  LDX #8
__pf_div_byte:
  DEX
  LDA __pf_val,X
  STA __t0
  LDA __pf_zero
  STA __t0+1
  LDA #0
  LDY #16
__pf_div_bit:
  ASL __t0
  ROL __t0+1
  ROL A
  CMP __pf_ch
  BCC __pf_div_nosub
  SBC __pf_ch
  INC __t0
__pf_div_nosub:
  DEY
  BNE __pf_div_bit
  STA __pf_zero
  LDA __t0
  STA __pf_val,X
  CPX #0
  BNE __pf_div_byte
  LDA __pf_zero
  RTS

__pf_emit:
  STA __t2
  LDY #F_ERR
  LDA (__sp),Y
  BEQ __pf_emit_go
  RTS
__pf_emit_go:
  LDY #F_KIND
  LDA (__sp),Y
  BNE __pf_emit_str
  LDY #F_PTR
  JSR __pf_ldw
  LDX __t0
  LDY __t0+1
  JSR __pushxy
  LDX __t2
  LDY #0
  JSR __pushxy
  CLC
  LDA __sp
  ADC #4+F_TMP
  TAX
  LDA __sp+1
  ADC #0
  TAY
  JSR fputc
  LDY #F_TMP+1
  LDA (__sp),Y
  BPL __pf_emit_count
  LDA #1
  LDY #F_ERR
  STA (__sp),Y
  LDA #$ff
  LDY #F_CNT
  STA (__sp),Y
  INY
  STA (__sp),Y
  RTS
__pf_emit_str:
  LDY #F_LIM
  LDA (__sp),Y
  INY
  ORA (__sp),Y
  BEQ __pf_emit_count
  LDY #F_PTR
  JSR __pf_ldw
  LDY #0
  LDA __t2
  STA (__t0),Y
  INC __t0
  BNE __pf_emit_ptr
  INC __t0+1
__pf_emit_ptr:
  LDY #F_PTR
  JSR __pf_stw
  LDY #F_LIM
  LDA (__sp),Y
  SEC
  SBC #1
  STA (__sp),Y
  INY
  LDA (__sp),Y
  SBC #0
  STA (__sp),Y
__pf_emit_count:
  LDY #F_CNT
  JMP __pf_incw

__pf_rep:
  STA __pf_ch
__pf_rep_loop:
  LDA __pf_repn
  ORA __pf_repn+1
  BEQ __pf_rep_done
  LDA __pf_repn
  BNE __pf_rep_dec
  DEC __pf_repn+1
__pf_rep_dec:
  DEC __pf_repn
  LDA __pf_ch
  JSR __pf_emit
  LDY #F_ERR
  LDA (__sp),Y
  BEQ __pf_rep_loop
__pf_rep_done:
  RTS

// __t0 = content length. Leaves pad count in __pf_repn.
__pf_pad:
  LDY #F_WID
  LDA (__sp),Y
  SEC
  SBC __t0
  STA __pf_repn
  INY
  LDA (__sp),Y
  SBC __t0+1
  STA __pf_repn+1
  BCS __pf_pad_ok
  LDA #0
  STA __pf_repn
  STA __pf_repn+1
__pf_pad_ok:
  RTS

__pf_emit_n:
__pf_emit_n_loop:
  LDA __pf_len
  ORA __pf_len+1
  BEQ __pf_emit_n_done
  LDA __pf_ptr
  STA __t0
  LDA __pf_ptr+1
  STA __t0+1
  LDY #0
  LDA (__t0),Y
  PHA
  INC __pf_ptr
  BNE __pf_emit_n_adv
  INC __pf_ptr+1
__pf_emit_n_adv:
  LDA __pf_len
  BNE __pf_emit_n_dec
  DEC __pf_len+1
__pf_emit_n_dec:
  DEC __pf_len
  PLA
  JSR __pf_emit
  LDY #F_ERR
  LDA (__sp),Y
  BEQ __pf_emit_n_loop
__pf_emit_n_done:
  RTS

__pf_spaces:
  LDA #' '
  JMP __pf_rep

// Y holds F_FL on entry to the tests below.
__pf_sign_len:
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_NEG+PF_PLUS+PF_SPACE
  BEQ __pf_sign_none
  LDA #1
  RTS
__pf_sign_none:
  LDA #0
  RTS

__pf_emit_sign:
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_NEG
  BEQ __pf_sign_plus
  LDA #'-'
  JMP __pf_emit
__pf_sign_plus:
  LDA (__sp),Y
  AND #PF_PLUS
  BEQ __pf_sign_sp
  LDA #'+'
  JMP __pf_emit
__pf_sign_sp:
  LDA (__sp),Y
  AND #PF_SPACE
  BEQ __pf_sign_skip
  LDA #' '
  JMP __pf_emit
__pf_sign_skip:
  RTS

__pf_acc10:
  // Y = word offset, A = digit 0-9. word = word*10 + digit.
  STA __t2
  STY __t3
  LDA (__sp),Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t0+1
  ASL __t0
  ROL __t0+1
  LDA __t0
  STA __pf_repn
  LDA __t0+1
  STA __pf_repn+1
  ASL __t0
  ROL __t0+1
  ASL __t0
  ROL __t0+1
  CLC
  LDA __t0
  ADC __pf_repn
  STA __t0
  LDA __t0+1
  ADC __pf_repn+1
  STA __t0+1
  CLC
  LDA __t0
  ADC __t2
  STA __t0
  LDA __t0+1
  ADC #0
  STA __t0+1
  LDY __t3
  JMP __pf_stw

__pf_loadint:
  // A = 1 if signed. Uses F_LEN. Leaves magnitude in __pf_val and PF_NEG.
  STA __pf_ch
  LDY #F_LEN
  LDA (__sp),Y
  CMP #LEN_LL
  BNE __pf_load_l
  LDX #8
  JMP __pf_load_copy
__pf_load_l:
  CMP #LEN_L
  BNE __pf_load_small
  LDX #4
  JMP __pf_load_copy
__pf_load_small:
  LDX #2
__pf_load_copy:
  JSR __pf_arg
  LDY #F_LEN
  LDA (__sp),Y
  CMP #LEN_HH
  BNE __pf_load_ext
  LDA #0
  STA __pf_val+1
  LDA __pf_val
  BPL __pf_load_hh_pos
  LDA __pf_ch
  BEQ __pf_load_hh_pos
  LDX #1
  JMP __pf_load_fill
__pf_load_hh_pos:
  RTS
__pf_load_ext:
  LDA __pf_ch
  BEQ __pf_load_done
  LDY #F_LEN
  LDA (__sp),Y
  CMP #LEN_L
  BEQ __pf_load_m4
  CMP #LEN_LL
  BEQ __pf_load_m8
  LDX #1
  JMP __pf_load_sign
__pf_load_m4:
  LDX #3
  JMP __pf_load_sign
__pf_load_m8:
  LDX #7
__pf_load_sign:
  LDA __pf_val,X
  BPL __pf_load_done
  INX
__pf_load_fill:
  CPX #8
  BCS __pf_load_neg
  LDA #$ff
  STA __pf_val,X
  INX
  JMP __pf_load_fill
__pf_load_neg:
  JSR __pf_neg
  LDY #F_FL
  LDA (__sp),Y
  ORA #PF_NEG
  STA (__sp),Y
__pf_load_done:
  RTS

__pf_reverse:
  LDX #0
  LDY __pf_ndig
  BEQ __pf_reverse_done
  DEY
__pf_reverse_loop:
  STY __t0
  CPX __t0
  BCS __pf_reverse_done
  LDA __pf_digs,X
  PHA
  LDA __pf_digs,Y
  STA __pf_digs,X
  PLA
  STA __pf_digs,Y
  INX
  DEY
  JMP __pf_reverse_loop
__pf_reverse_done:
  RTS

__pf_lower_digits:
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_UPPER
  BNE __pf_lower_done
  LDX __pf_ndig
__pf_lower_loop:
  DEX
  BMI __pf_lower_done
  LDA __pf_digs,X
  CMP #'A'
  BCC __pf_lower_loop
  CMP #'F'+1
  BCS __pf_lower_loop
  ORA #$20
  STA __pf_digs,X
  JMP __pf_lower_loop
__pf_lower_done:
  RTS

// A = base. __pf_i = 1 for a signed conversion. __pf_loadint reuses
// __pf_ch, so the base is kept in __pf_ftmp and the zero flag in
// __pf_ftmp+1 (0 when the value is zero).
__pf_number:
  STA __pf_ftmp
  LDA #0
  STA __pf_ndig
  LDA __pf_i
  JSR __pf_loadint
  JSR __pf_iszero
  STA __pf_ftmp+1
  LDA __pf_ftmp
  STA __pf_ch
  LDA __pf_ftmp+1
  BNE __pf_number_div
  LDY #F_PRE
  LDA (__sp),Y
  INY
  ORA (__sp),Y
  BEQ __pf_number_emit
  LDA #'0'
  STA __pf_digs
  LDA #1
  STA __pf_ndig
  JMP __pf_number_emit
__pf_number_div:
  LDA __pf_ch
  STA __pf_i
  JSR __pf_div
  CMP #10
  BCC __pf_number_dec
  CLC
  ADC #'A'-10
  JMP __pf_number_store
__pf_number_dec:
  CLC
  ADC #'0'
__pf_number_store:
  LDY __pf_ndig
  CPY #22
  BCS __pf_number_emit
  STA __pf_digs,Y
  INC __pf_ndig
  JSR __pf_iszero
  BEQ __pf_number_rev
  LDA __pf_i
  STA __pf_ch
  JMP __pf_number_div
__pf_number_rev:
  JSR __pf_reverse
  JSR __pf_lower_digits
__pf_number_emit:
  // Prefix length in __t2. Precision zeros in __pf_len.
  LDA #0
  STA __t2
  STA __pf_len
  STA __pf_len+1
  LDA __pf_ch
  CMP #16
  BNE __pf_number_oct
  LDA __pf_ftmp+1
  BEQ __pf_number_hex_no
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_ALT
  BEQ __pf_number_hex_no
  LDA #2
  STA __t2
__pf_number_hex_no:
  JMP __pf_number_zpad
__pf_number_oct:
  CMP #8
  BNE __pf_number_zpad
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_ALT
  BEQ __pf_number_zpad
  LDA __pf_ndig
  BEQ __pf_number_oct_one
  LDX #0
  LDA __pf_digs
  CMP #'0'
  BEQ __pf_number_zpad
__pf_number_oct_one:
  LDA #1
  STA __t2
__pf_number_zpad:
  LDY #F_PRE
  LDA (__sp),Y
  CMP #$ff
  BNE __pf_number_prec
  INY
  LDA (__sp),Y
  CMP #$ff
  BEQ __pf_number_widths
__pf_number_prec:
  LDY #F_PRE
  LDA (__sp),Y
  SEC
  SBC __pf_ndig
  STA __pf_len
  INY
  LDA (__sp),Y
  SBC #0
  STA __pf_len+1
  BCS __pf_number_widths
  LDA #0
  STA __pf_len
  STA __pf_len+1
__pf_number_widths:
  JSR __pf_sign_len
  CLC
  ADC __t2
  ADC __pf_ndig
  ADC __pf_len
  STA __t0
  LDA #0
  ADC __pf_len+1
  STA __t0+1
  JSR __pf_pad
  // Space pad if not left, and not (zero flag with default precision).
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_LEFT
  BNE __pf_number_body
  LDA (__sp),Y
  AND #PF_ZERO
  BEQ __pf_number_sp
  LDY #F_PRE
  LDA (__sp),Y
  CMP #$ff
  BNE __pf_number_sp
  INY
  LDA (__sp),Y
  CMP #$ff
  BEQ __pf_number_body
__pf_number_sp:
  JSR __pf_spaces
__pf_number_body:
  JSR __pf_emit_sign
  LDA __t2
  CMP #2
  BNE __pf_number_octpfx
  LDA #'0'
  JSR __pf_emit
  LDA #'x'
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_UPPER
  BEQ __pf_number_x
  LDA #'X'
__pf_number_x:
  JSR __pf_emit
  JMP __pf_number_zflag
__pf_number_octpfx:
  CMP #1
  BNE __pf_number_zflag
  LDA #'0'
  JSR __pf_emit
__pf_number_zflag:
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_LEFT+PF_ZERO
  CMP #PF_ZERO
  BNE __pf_number_precz
  LDY #F_PRE
  LDA (__sp),Y
  CMP #$ff
  BNE __pf_number_precz
  INY
  LDA (__sp),Y
  CMP #$ff
  BNE __pf_number_precz
  LDA #'0'
  JSR __pf_rep
  LDA #0
  STA __pf_repn
  STA __pf_repn+1
__pf_number_precz:
  LDA __pf_len
  STA __pf_repn
  LDA __pf_len+1
  STA __pf_repn+1
  LDA #'0'
  JSR __pf_rep
  LDA #0
  STA __pf_i
__pf_number_digits:
  LDA __pf_i
  CMP __pf_ndig
  BCS __pf_number_left
  LDX __pf_i
  LDA __pf_digs,X
  INC __pf_i
  JSR __pf_emit
  JMP __pf_number_digits
__pf_number_left:
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_LEFT
  BEQ __pf_number_done
  JSR __pf_spaces
__pf_number_done:
  RTS

__pf_string:
  // __pf_ptr set, precision caps the length. Width pads with spaces.
  LDA #0
  STA __pf_len
  STA __pf_len+1
  LDA __pf_ptr
  STA __t0
  LDA __pf_ptr+1
  STA __t0+1
__pf_string_len:
  LDY #F_PRE
  LDA (__sp),Y
  CMP #$ff
  BNE __pf_string_cap
  INY
  LDA (__sp),Y
  CMP #$ff
  BEQ __pf_string_ch
__pf_string_cap:
  LDY #F_PRE
  LDA __pf_len
  CMP (__sp),Y
  BNE __pf_string_ch
  INY
  LDA __pf_len+1
  CMP (__sp),Y
  BEQ __pf_string_pad
__pf_string_ch:
  LDY #0
  LDA (__t0),Y
  BEQ __pf_string_pad
  INC __t0
  BNE __pf_string_inc
  INC __t0+1
__pf_string_inc:
  INC __pf_len
  BNE __pf_string_len
  INC __pf_len+1
  JMP __pf_string_len
__pf_string_pad:
  LDA __pf_len
  STA __t0
  LDA __pf_len+1
  STA __t0+1
  JSR __pf_pad
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_LEFT
  BNE __pf_string_out
  JSR __pf_spaces
__pf_string_out:
  JSR __pf_emit_n
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_LEFT
  BEQ __pf_string_done
  JSR __pf_spaces
__pf_string_done:
  RTS

__pf_finish:
  LDY #F_NUL
  LDA (__sp),Y
  BEQ __pf_finish_ret
  LDY #F_PTR
  JSR __pf_ldw
  LDY #0
  LDA #0
  STA (__t0),Y
__pf_finish_ret:
  LDY #F_CNT
  JSR __pf_ldw
  LDY #F_RES
  LDA (__sp),Y
  STA __t2
  INY
  LDA (__sp),Y
  STA __t2+1
  LDY #0
  LDA __t0
  STA (__t2),Y
  INY
  LDA __t0+1
  STA (__t2),Y
  LDX #FRAME
  JMP __pf_free

__pf_null:
  .byte '(', 'n', 'u', 'l', 'l', ')', 0

__pf_run:
__pf_run_loop:
  LDY #F_ERR
  LDA (__sp),Y
  BNE __pf_run_done
  JSR __pf_next
  BEQ __pf_run_done
  CMP #'%'
  BEQ __pf_spec
  JSR __pf_emit
  JMP __pf_run_loop
__pf_run_done:
  JMP __pf_finish
__pf_run_far:
  JMP __pf_run_done
__pf_spec:
  LDA #0
  LDY #F_FL
  STA (__sp),Y
  LDY #F_WID
  STA (__sp),Y
  INY
  STA (__sp),Y
  LDY #F_LEN
  STA (__sp),Y
  LDA #$ff
  LDY #F_PRE
  STA (__sp),Y
  INY
  STA (__sp),Y
  JSR __pf_next
  BEQ __pf_run_done
__pf_flag:
  CMP #'-'
  BNE __pf_flag_plus
  LDA #PF_LEFT
  JMP __pf_flag_set
__pf_flag_plus:
  CMP #'+'
  BNE __pf_flag_sp
  LDA #PF_PLUS
  JMP __pf_flag_set
__pf_flag_sp:
  CMP #' '
  BNE __pf_flag_alt
  LDA #PF_SPACE
  JMP __pf_flag_set
__pf_flag_alt:
  CMP #'#'
  BNE __pf_flag_zero
  LDA #PF_ALT
  JMP __pf_flag_set
__pf_flag_zero:
  CMP #'0'
  BNE __pf_width
  LDA #PF_ZERO
__pf_flag_set:
  LDY #F_FL
  ORA (__sp),Y
  STA (__sp),Y
  JSR __pf_next
  BEQ __pf_run_done
  JMP __pf_flag
__pf_width:
  CMP #'*'
  BNE __pf_width_dig
  JSR __pf_star
  LDY #F_WID
  JSR __pf_stw
  JSR __pf_next
  BEQ __pf_run_done
  JMP __pf_prec
__pf_width_dig:
  CMP #'0'
  BCC __pf_prec
  CMP #'9'+1
  BCS __pf_prec
  SEC
  SBC #'0'
  LDY #F_WID
  JSR __pf_acc10
  JSR __pf_next
  BNE __pf_width_more
  JMP __pf_run_done
__pf_width_more:
  JMP __pf_width_dig
__pf_prec:
  CMP #'.'
  BNE __pf_lmod
  LDA #0
  LDY #F_PRE
  STA (__sp),Y
  INY
  STA (__sp),Y
  JSR __pf_next
  BNE __far_20
    JMP __pf_run_done
__far_20:
  CMP #'*'
  BNE __pf_prec_dig
  JSR __pf_star
  LDA __t0+1
  BMI __pf_prec_neg
  LDY #F_PRE
  JSR __pf_stw
  JMP __pf_prec_next
__pf_prec_neg:
  LDA #$ff
  LDY #F_PRE
  STA (__sp),Y
  INY
  STA (__sp),Y
__pf_prec_next:
  JSR __pf_next
  BNE __far_19
    JMP __pf_run_done
__far_19:
  JMP __pf_lmod
__pf_prec_dig:
  CMP #'0'
  BCC __pf_lmod_far
  CMP #'9'+1
  BCS __pf_lmod_far
  SEC
  SBC #'0'
  LDY #F_PRE
  JSR __pf_acc10
  JSR __pf_next
  BNE __pf_prec_more
  JMP __pf_run_done
__pf_prec_more:
  JMP __pf_prec_dig
__pf_lmod_far:
  JMP __pf_lmod
__pf_lmod:
  CMP #'h'
  BNE __pf_len_l
  JSR __pf_next
  CMP #'h'
  BEQ __pf_len_hh
  PHA
  LDA #LEN_H
  LDY #F_LEN
  STA (__sp),Y
  PLA
  JMP __pf_conv
__pf_len_hh:
  LDA #LEN_HH
  LDY #F_LEN
  STA (__sp),Y
  JSR __pf_next
  JMP __pf_conv
__pf_len_l:
  CMP #'l'
  BNE __pf_len_z
  JSR __pf_next
  CMP #'l'
  BEQ __pf_len_ll
  PHA
  LDA #LEN_L
  LDY #F_LEN
  STA (__sp),Y
  PLA
  JMP __pf_conv
__pf_len_ll:
  LDA #LEN_LL
  LDY #F_LEN
  STA (__sp),Y
  JSR __pf_next
  JMP __pf_conv
__pf_len_z:
  CMP #'z'
  BNE __pf_len_t
  LDA #LEN_H
  JMP __pf_len_one
__pf_len_t:
  CMP #'t'
  BNE __pf_len_j
  LDA #LEN_H
  JMP __pf_len_one
__pf_len_j:
  CMP #'j'
  BNE __pf_len_L
  LDA #LEN_LL
  JMP __pf_len_one
__pf_len_L:
  CMP #'L'
  BNE __pf_conv
  JSR __pf_next
  JMP __pf_conv
__pf_len_one:
  LDY #F_LEN
  STA (__sp),Y
  JSR __pf_next
__pf_conv:
  CMP #'d'
  BNE __pf_not_d
  JMP __pf_conv_sdec
__pf_not_d:
  CMP #'i'
  BNE __pf_not_i
  JMP __pf_conv_sdec
__pf_not_i:
  CMP #'u'
  BNE __pf_not_u
  JMP __pf_conv_udec
__pf_not_u:
  CMP #'x'
  BNE __pf_not_x
  JMP __pf_conv_hex
__pf_not_x:
  CMP #'X'
  BNE __pf_not_X
  JMP __pf_conv_hexu
__pf_not_X:
  CMP #'o'
  BNE __pf_not_o
  JMP __pf_conv_oct
__pf_not_o:
  CMP #'p'
  BNE __pf_not_p
  JMP __pf_conv_ptr
__pf_not_p:
  CMP #'c'
  BNE __pf_not_c
  JMP __pf_conv_c
__pf_not_c:
  CMP #'s'
  BNE __pf_not_s
  JMP __pf_conv_s
__pf_not_s:
  CMP #'n'
  BNE __pf_not_n
  JMP __pf_conv_n
__pf_not_n:
  CMP #'%'
  BNE __pf_not_pct
  JMP __pf_conv_pct
__pf_not_pct:
  CMP #'f'
  BNE __pf_not_f
  JMP __pf_conv_f
__pf_not_f:
  CMP #'F'
  BNE __pf_not_F
  JMP __pf_conv_F
__pf_not_F:
  CMP #'e'
  BNE __pf_not_e
  JMP __pf_conv_e
__pf_not_e:
  CMP #'E'
  BNE __pf_not_E
  JMP __pf_conv_E
__pf_not_E:
  CMP #'g'
  BNE __pf_not_g
  JMP __pf_conv_g
__pf_not_g:
  CMP #'G'
  BNE __pf_not_G
  JMP __pf_conv_G
__pf_not_G:
  CMP #'a'
  BNE __pf_not_a
  JMP __pf_conv_a
__pf_not_a:
  CMP #'A'
  BNE __pf_not_A
  JMP __pf_conv_A
__pf_not_A:
  JSR __pf_emit
  JMP __pf_run_loop
__pf_conv_sdec:
  LDA #1
  STA __pf_i
  LDA #10
  JSR __pf_number
  JMP __pf_run_loop
__pf_conv_udec:
  LDA #0
  STA __pf_i
  LDA #10
  JSR __pf_number
  JMP __pf_run_loop
__pf_conv_hexu:
  LDY #F_FL
  LDA (__sp),Y
  ORA #PF_UPPER
  STA (__sp),Y
__pf_conv_hex:
  LDA #0
  STA __pf_i
  LDA #16
  JSR __pf_number
  JMP __pf_run_loop
__pf_conv_oct:
  LDA #0
  STA __pf_i
  LDA #8
  JSR __pf_number
  JMP __pf_run_loop
__pf_conv_ptr:
  LDA #0
  LDY #F_LEN
  STA (__sp),Y
  LDY #F_FL
  LDA (__sp),Y
  ORA #PF_ALT
  STA (__sp),Y
  LDA #0
  STA __pf_i
  LDA #16
  JSR __pf_number
  JMP __pf_run_loop
__pf_conv_c:
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  STA __pf_digs
  LDA #0
  STA __pf_digs+1
  LDA #%lo(__pf_digs)
  STA __pf_ptr
  LDA #%hi(__pf_digs)
  STA __pf_ptr+1
  LDA #$ff
  LDY #F_PRE
  STA (__sp),Y
  INY
  STA (__sp),Y
  LDA #1
  STA __pf_len
  // Measure is skipped: plant a single-char string with precision 1
  // by calling the emitter with len forced after a prec of 1.
  LDA #1
  LDY #F_PRE
  STA (__sp),Y
  LDA #0
  INY
  STA (__sp),Y
  JSR __pf_string
  JMP __pf_run_loop
__pf_conv_s:
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  ORA __pf_val+1
  BNE __pf_conv_s_ptr
  LDA #%lo(__pf_null)
  STA __pf_ptr
  LDA #%hi(__pf_null)
  STA __pf_ptr+1
  JMP __pf_conv_s_go
__pf_conv_s_ptr:
  LDA __pf_val
  STA __pf_ptr
  LDA __pf_val+1
  STA __pf_ptr+1
__pf_conv_s_go:
  JSR __pf_string
  JMP __pf_run_loop
__pf_conv_n:
  LDY #F_CNT
  JSR __pf_ldw
  LDA __t0
  STA __pf_fbuf
  LDA __t0+1
  STA __pf_fbuf+1
  BPL __pf_conv_n_ext
  LDA #$ff
  JMP __pf_conv_n_fill
__pf_conv_n_ext:
  LDA #0
__pf_conv_n_fill:
  STA __pf_fbuf+2
  STA __pf_fbuf+3
  STA __pf_fbuf+4
  STA __pf_fbuf+5
  STA __pf_fbuf+6
  STA __pf_fbuf+7
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  STA __t0
  LDA __pf_val+1
  STA __t0+1
  JSR __pf_store
  JMP __pf_run_loop
__pf_conv_pct:
  LDA #'%'
  JSR __pf_emit
  JMP __pf_run_loop
__pf_conv_F:
  LDY #F_FL
  LDA (__sp),Y
  ORA #PF_UPPER
  STA (__sp),Y
__pf_conv_f:
  LDA #0
  JMP __pf_float
__pf_conv_E:
  LDY #F_FL
  LDA (__sp),Y
  ORA #PF_UPPER
  STA (__sp),Y
__pf_conv_e:
  LDA #1
  JMP __pf_float
__pf_conv_G:
  LDY #F_FL
  LDA (__sp),Y
  ORA #PF_UPPER
  STA (__sp),Y
__pf_conv_g:
  LDA #2
  JMP __pf_float
__pf_conv_A:
  LDY #F_FL
  LDA (__sp),Y
  ORA #PF_UPPER
  STA (__sp),Y
__pf_conv_a:
  LDA #3
  JMP __pf_float

// A = 0 fixed, 1 scientific, 2 general, 3 hex.
__pf_float:
  ; __pf_arg reuses __pf_i as its byte count, so the format lives in __pf_ch.
  STA __pf_ch
  LDX #4
  JSR __pf_arg
  LDA __pf_ch
  STA __pf_i
  LDY #F_PRE
  LDA (__sp),Y
  STA __pf_repn
  INY
  LDA (__sp),Y
  STA __pf_repn+1
  CMP #$ff
  BNE __pf_float_push
  LDA __pf_repn
  CMP #$ff
  BNE __pf_float_push
  LDA #6
  STA __pf_repn
  LDA #0
  STA __pf_repn+1
__pf_float_push:
  LDA #0
  STA __pf_fbuf
  LDX #48
  LDY #0
  JSR __pushxy
  LDX #%lo(__pf_fbuf)
  LDY #%hi(__pf_fbuf)
  JSR __pushxy
  LDA __pf_i
  CMP #3
  BNE __pf_float_prec
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_ALT
  BEQ __pf_float_alt0
  LDX #1
  JMP __pf_float_alt
__pf_float_alt0:
  LDX #0
__pf_float_alt:
  LDY #0
  JSR __pushxy
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_UPPER
  BEQ __pf_float_up0
  LDX #1
  JMP __pf_float_up
__pf_float_up0:
  LDX #0
__pf_float_up:
  LDY #0
  JSR __pushxy
__pf_float_prec:
  LDX __pf_repn
  LDY __pf_repn+1
  JSR __pushxy
  JSR __decsp4
  LDY #0
__pf_float_cp:
  LDA __pf_val,Y
  STA (__sp),Y
  INY
  CPY #4
  BNE __pf_float_cp
  LDA __sp
  LDX __pf_i
  CPX #3
  BNE __pf_float_off10
  CLC
  ADC #14+F_TMP
  TAX
  LDA __sp+1
  ADC #0
  TAY
  JSR pf_hex
  JMP __pf_float_out
__pf_float_off10:
  CLC
  ADC #10+F_TMP
  TAX
  LDA __sp+1
  ADC #0
  TAY
  LDA __pf_i
  BNE __pf_float_sci
  JSR pf_fixed
  JMP __pf_float_pop
__pf_float_sci:
  CMP #1
  BNE __pf_float_gen
  JSR pf_scientific
  JMP __pf_float_pop
__pf_float_gen:
  JSR pf_general
__pf_float_pop:
__pf_float_out:
  // The printer right-justifies into the buffer and returns the first
  // significant character, which is not necessarily the first byte.
  LDY #F_TMP
  JSR __pf_ldw
  LDA __t0
  STA __pf_ptr
  LDA __t0+1
  STA __pf_ptr+1
  ORA __t0
  BNE __pf_float_case
  JMP __pf_run_loop
__pf_float_case:
  LDY #F_FL
  LDA (__sp),Y
  AND #PF_UPPER
  BEQ __pf_float_as_str
  LDY #0
__pf_float_uploop:
  LDA (__t0),Y
  BEQ __pf_float_as_str
  CMP #'a'
  BCC __pf_float_upnext
  CMP #'z'+1
  BCS __pf_float_upnext
  AND #$df
  STA (__t0),Y
__pf_float_upnext:
  INY
  JMP __pf_float_uploop
__pf_float_as_str:
  LDA #$ff
  LDY #F_PRE
  STA (__sp),Y
  INY
  STA (__sp),Y
  JSR __pf_string
  JMP __pf_run_loop

// * width or precision. Negative width sets the left flag and is negated.
__pf_star:
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  STA __t0
  LDA __pf_val+1
  STA __t0+1
  BPL __pf_star_done
  SEC
  LDA #0
  SBC __t0
  STA __t0
  LDA #0
  SBC __t0+1
  STA __t0+1
  LDY #F_FL
  LDA (__sp),Y
  ORA #PF_LEFT
  STA (__sp),Y
__pf_star_done:
  RTS

// Store __pf_val through the next pointer argument. Width from F_LEN.
__pf_store_ap:
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  // arg copied the pointer over __pf_val. Save the pointer first.
  RTS

// Real store: pointer is in __t0 on entry, bytes already in a side buffer.
// The conversion puts the value in __pf_digs for %n before taking the pointer.
// See __pf_store below.

__pf_store:
  // __t0 = destination. Byte count derived from F_LEN.
  LDY #F_LEN
  LDA (__sp),Y
  CMP #LEN_HH
  BNE __pf_store_h
  LDX #1
  JMP __pf_store_cp
__pf_store_h:
  CMP #LEN_L
  BNE __pf_store_ll
  LDX #4
  JMP __pf_store_cp
__pf_store_ll:
  CMP #LEN_LL
  BNE __pf_store_2
  LDX #8
  JMP __pf_store_cp
__pf_store_2:
  LDX #2
__pf_store_cp:
  LDY #0
__pf_store_loop:
  LDA __pf_fbuf,Y
  STA (__t0),Y
  INY
  DEX
  BNE __pf_store_loop
  RTS

// ---- scanf -----------------------------------------------------------

__sc_fail:
  STA __pf_ch
  LDY #S_STAT
  LDA (__sp),Y
  BNE __sc_fail_done
  LDA __pf_ch
  STA (__sp),Y
__sc_fail_done:
  RTS

__sc_ninc:
  PHA
  LDY #S_NRD
  JSR __pf_incw
  PLA
  RTS

__sc_ndec:
  LDY #S_NRD
  LDA (__sp),Y
  SEC
  SBC #1
  STA (__sp),Y
  INY
  LDA (__sp),Y
  SBC #0
  STA (__sp),Y
  RTS

__sc_getc:
  LDY #S_KIND
  LDA (__sp),Y
  BNE __sc_getc_str
  LDY #S_PTR
  JSR __pf_ldw
  LDX __t0
  LDY __t0+1
  JSR __pushxy
  CLC
  LDA __sp
  ADC #2+F_TMP
  TAX
  LDA __sp+1
  ADC #0
  TAY
  JSR fgetc
  LDY #F_TMP+1
  LDA (__sp),Y
  BMI __sc_getc_eof
  LDY #F_TMP
  LDA (__sp),Y
  JSR __sc_ninc
  CLC
  RTS
__sc_getc_str:
  LDY #S_PTR
  JSR __pf_ldw
  LDY #0
  LDA (__t0),Y
  BEQ __sc_getc_eof
  PHA
  INC __t0
  BNE __sc_getc_st
  INC __t0+1
__sc_getc_st:
  LDY #S_PTR
  JSR __pf_stw
  PLA
  JSR __sc_ninc
  CLC
  RTS
__sc_getc_eof:
  LDA #$ff
  SEC
  RTS

__sc_unget:
  CMP #$ff
  BEQ __sc_unget_done
  PHA
  JSR __sc_ndec
  LDY #S_KIND
  LDA (__sp),Y
  BNE __sc_unget_str
  PLA
  TAX
  LDY #0
  JSR __pushxy
  LDY #S_PTR
  JSR __pf_ldw
  LDX __t0
  LDY __t0+1
  JSR __pushxy
  CLC
  LDA __sp
  ADC #4+F_TMP
  TAX
  LDA __sp+1
  ADC #0
  TAY
  JSR ungetc
  RTS
__sc_unget_str:
  LDY #S_PTR
  JSR __pf_ldw
  LDA __t0
  BNE __sc_unget_dec
  DEC __t0+1
__sc_unget_dec:
  DEC __t0
  LDY #S_PTR
  JSR __pf_stw
  PLA
__sc_unget_done:
  RTS

__sc_space:
  CMP #' '
  BEQ __sc_space_yes
  CMP #9
  BEQ __sc_space_yes
  CMP #10
  BEQ __sc_space_yes
  CMP #13
  BEQ __sc_space_yes
  CMP #11
  BEQ __sc_space_yes
  CMP #12
  BEQ __sc_space_yes
  CLC
  RTS
__sc_space_yes:
  SEC
  RTS

__sc_skip:
__sc_skip_loop:
  JSR __sc_getc
  BCS __sc_skip_eof
  JSR __sc_space
  BCS __sc_skip_loop
  JMP __sc_unget
__sc_skip_eof:
  RTS

__sc_width_init:
  LDA #$ff
  LDY #S_WID
  STA (__sp),Y
  INY
  STA (__sp),Y
  RTS

// Carry set when the width is exhausted.
__sc_width:
  LDY #S_WID
  LDA (__sp),Y
  CMP #$ff
  BNE __sc_width_lim
  INY
  LDA (__sp),Y
  CMP #$ff
  BNE __sc_width_lim
  CLC
  RTS
__sc_width_lim:
  LDY #S_WID
  LDA (__sp),Y
  INY
  ORA (__sp),Y
  BNE __sc_width_dec
  SEC
  RTS
__sc_width_dec:
  LDY #S_WID
  LDA (__sp),Y
  SEC
  SBC #1
  STA (__sp),Y
  INY
  LDA (__sp),Y
  SBC #0
  STA (__sp),Y
  CLC
  RTS

__sc_digit:
  // A = char, __pf_ch = base. Returns digit in A, carry clear if ok.
  CMP #'0'
  BCC __sc_digit_no
  CMP #'9'+1
  BCS __sc_digit_hex
  SEC
  SBC #'0'
  CMP __pf_ch
  BCS __sc_digit_no
  CLC
  RTS
__sc_digit_hex:
  CMP #'A'
  BCC __sc_digit_no
  CMP #'F'+1
  BCS __sc_digit_lower
  SEC
  SBC #'A'-10
  CMP __pf_ch
  BCS __sc_digit_no
  CLC
  RTS
__sc_digit_lower:
  CMP #'a'
  BCC __sc_digit_no
  CMP #'f'+1
  BCS __sc_digit_no
  SEC
  SBC #'a'-10
  CMP __pf_ch
  BCS __sc_digit_no
  CLC
  RTS
__sc_digit_no:
  SEC
  RTS

__sc_muladd:
  // A = digit, __pf_ch = base. val = val*base + digit.
  STA __pf_i
  LDA __pf_ch
  CMP #10
  BEQ __sc_mul10
  CMP #16
  BEQ __sc_mul16
  LDX #3
  JSR __sc_shift
  JMP __sc_add_digit
__sc_mul16:
  LDX #4
  JSR __sc_shift
  JMP __sc_add_digit
__sc_mul10:
  LDX #0
__sc_mul10_cp:
  LDA __pf_val,X
  STA __pf_digs,X
  INX
  CPX #8
  BNE __sc_mul10_cp
  LDX #1
  JSR __sc_shift_digs
  LDX #3
  JSR __sc_shift
  LDX #0
  CLC
__sc_mul10_add:
  LDA __pf_val,X
  ADC __pf_digs,X
  STA __pf_val,X
  INX
  CPX #8
  BNE __sc_mul10_add
  JMP __sc_add_digit

__sc_shift_digs:
  STX __t3
__sc_shift_digs_n:
  LDX #0
  CLC
__sc_shift_digs_b:
  ROL __pf_digs,X
  INX
  CPX #8
  BNE __sc_shift_digs_b
  DEC __t3
  BNE __sc_shift_digs_n
  RTS
__sc_shift:
  // X = shift count. Shifts __pf_val left. ROL has no absolute,Y form.
  STX __t3
__sc_shift_n:
  LDX #0
  CLC
__sc_shift_b:
  ROL __pf_val,X
  INX
  CPX #8
  BNE __sc_shift_b
  DEC __t3
  BNE __sc_shift_n
  RTS
__sc_add_digit:
  CLC
  LDA __pf_val
  ADC __pf_i
  STA __pf_val
  LDX #1
__sc_add_loop:
  LDA __pf_val,X
  ADC #0
  STA __pf_val,X
  INX
  CPX #8
  BNE __sc_add_loop
  RTS

__sc_store_val:
  LDY #S_LEN
  LDA (__sp),Y
  LDY #F_LEN
  STA (__sp),Y
  LDY #S_SUP
  LDA (__sp),Y
  BNE __sc_store_skip
  // Move the value aside, then take the pointer.
  LDX #0
__sc_store_save:
  LDA __pf_val,X
  STA __pf_fbuf,X
  INX
  CPX #8
  BNE __sc_store_save
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  STA __t0
  LDA __pf_val+1
  STA __t0+1
  JSR __pf_store
  LDY #S_ASN
  JSR __pf_incw
__sc_store_skip:
  RTS

__sc_int:
  // A = base. __pf_i = 1 signed, 2 = %i auto base.
  STA __pf_ch
  JSR __sc_skip
  JSR __sc_width
  BCC __far_18
    JMP __sc_int_input
__far_18:
  JSR __sc_getc
  BCC __far_17
    JMP __sc_int_input
__far_17:
  LDX #0
  STX __pf_zero
  CMP #'+'
  BEQ __sc_int_sign
  CMP #'-'
  BNE __sc_int_first
__sc_int_sign:
  STA __pf_zero
  JSR __sc_width
  BCC __far_16
    JMP __sc_int_nodig
__far_16:
  JSR __sc_getc
  BCC __far_15
    JMP __sc_int_nodig
__far_15:
__sc_int_first:
  STA __t2
  LDA __pf_i
  CMP #2
  BNE __sc_int_digit
  LDA __t2
  CMP #'0'
  BNE __sc_int_base10
  LDA #8
  STA __pf_ch
  JSR __sc_width
  BCS __sc_int_zero_only
  JSR __sc_getc
  BCS __sc_int_zero_only
  CMP #'x'
  BEQ __sc_int_hex
  CMP #'X'
  BEQ __sc_int_hex
  JSR __sc_unget
  LDA #'0'
  STA __t2
  JMP __sc_int_digit
__sc_int_hex:
  LDA #16
  STA __pf_ch
  JSR __sc_width
  BCS __sc_int_nodig
  JSR __sc_getc
  BCS __sc_int_nodig
  STA __t2
  JMP __sc_int_digit
__sc_int_base10:
  LDA #10
  STA __pf_ch
__sc_int_digit:
  LDX #8
  LDA #0
__sc_int_clr:
  DEX
  STA __pf_val,X
  CPX #0
  BNE __sc_int_clr
  LDA #0
  STA __pf_ndig
  LDA __t2
__sc_int_loop:
  JSR __sc_digit
  BCS __sc_int_end
  JSR __sc_muladd
  INC __pf_ndig
  JSR __sc_width
  BCS __sc_int_endok
  JSR __sc_getc
  BCS __sc_int_endok
  JMP __sc_int_loop
__sc_int_end:
  JSR __sc_unget
__sc_int_endok:
  LDA __pf_ndig
  BEQ __sc_int_nodig
  LDA __pf_zero
  CMP #'-'
  BNE __sc_int_store
  JSR __pf_neg
__sc_int_store:
  JMP __sc_store_val
__sc_int_zero_only:
  // A lone 0.
  LDA #0
  STA __pf_val
  STA __pf_val+1
  STA __pf_val+2
  STA __pf_val+3
  STA __pf_val+4
  STA __pf_val+5
  STA __pf_val+6
  STA __pf_val+7
  JMP __sc_int_store
__sc_int_nodig:
  LDA #STAT_MATCH
  JMP __sc_fail
__sc_int_input:
  LDA #STAT_INPUT
  JMP __sc_fail

__sc_str:
  JSR __sc_skip
  LDA #0
  STA __pf_ndig
  JSR __sc_getc
  BCC __sc_str_first
  LDA #STAT_INPUT
  JMP __sc_fail
__sc_str_first:
  JSR __sc_space
  BCC __sc_str_take
  JSR __sc_unget
  LDA #STAT_MATCH
  JMP __sc_fail
__sc_str_loop:
  JSR __sc_width
  BCS __sc_str_done
  JSR __sc_getc
  BCS __sc_str_done
  JSR __sc_space
  BCC __sc_str_take
  JSR __sc_unget
  JMP __sc_str_done
__sc_str_take:
  LDY #S_SUP
  LDA (__sp),Y
  BNE __sc_str_more
  LDY __pf_ndig
  CPY #46
  BCS __sc_str_more
  STA __pf_fbuf,Y
  // A was the suppress flag, not the char. The char was lost.
  JMP __sc_str_more
__sc_str_done:
  LDA __pf_ndig
  BNE __sc_str_term
  LDA #STAT_MATCH
  JMP __sc_fail
__sc_str_term:
  LDY #S_SUP
  LDA (__sp),Y
  BNE __sc_str_skip
  LDY __pf_ndig
  LDA #0
  STA __pf_fbuf,Y
  JSR __sc_store_buf
__sc_str_skip:
  RTS
__sc_str_more:
  INC __pf_ndig
  JMP __sc_str_loop

// The string scanner above loses the character. Replaced by __sc_text.
__sc_text:
  // A = 0 for %s (stop on space), 1 for scanset (bitmap in __pf_set, __pf_i invert).
  STA __pf_zero
  BNE __sc_text_start
  JSR __sc_skip
__sc_text_start:
  LDA #0
  STA __pf_ndig
  JSR __sc_getc
  BCC __sc_text_go
  LDA #STAT_INPUT
  JMP __sc_fail
__sc_text_go:
  JMP __sc_text_have
__sc_text_loop:
  JSR __sc_width
  BCS __sc_text_finish
  JSR __sc_getc
  BCS __sc_text_finish
__sc_text_have:
  STA __pf_ch
  LDA __pf_zero
  BNE __sc_text_set
  LDA __pf_ch
  JSR __sc_space
  BCC __sc_text_keep
  LDA __pf_ch
  JSR __sc_unget
  JMP __sc_text_finish
__sc_text_set:
  LDA __pf_ch
  JSR __sc_inset
  EOR __pf_i
  BNE __sc_text_keep
  LDA __pf_ch
  JSR __sc_unget
  JMP __sc_text_finish
__sc_text_keep:
  LDY #S_SUP
  LDA (__sp),Y
  BNE __sc_text_next
  LDY __pf_ndig
  CPY #46
  BCS __sc_text_next
  LDA __pf_ch
  STA __pf_fbuf,Y
__sc_text_next:
  INC __pf_ndig
  JMP __sc_text_loop
__sc_text_finish:
  LDA __pf_ndig
  BNE __sc_text_ok
  LDA #STAT_MATCH
  JMP __sc_fail
__sc_text_ok:
  LDY #S_SUP
  LDA (__sp),Y
  BNE __sc_text_out
  LDY __pf_ndig
  LDA #0
  STA __pf_fbuf,Y
  JSR __sc_store_buf
__sc_text_out:
  RTS

__sc_inset:
  PHA
  LSR A
  LSR A
  LSR A
  TAX
  PLA
  AND #7
  TAY
  LDA __pf_set,X
  AND __pf_bits,Y
  BEQ __sc_inset_no
  LDA #1
  RTS
__sc_inset_no:
  LDA #0
  RTS

__pf_bits:
  .byte 1, 2, 4, 8, 16, 32, 64, 128

__sc_store_buf:
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  STA __t0
  LDA __pf_val+1
  STA __t0+1
  LDY #0
__sc_store_buf_loop:
  LDA __pf_fbuf,Y
  STA (__t0),Y
  BEQ __sc_store_buf_count
  INY
  JMP __sc_store_buf_loop
__sc_store_buf_count:
  LDY #S_ASN
  JMP __pf_incw

__sc_char:
  LDY #S_WID
  LDA (__sp),Y
  INY
  AND (__sp),Y
  CMP #$ff
  BNE __sc_char_go
  LDA #1
  LDY #S_WID
  STA (__sp),Y
  LDA #0
  INY
  STA (__sp),Y
__sc_char_go:
  LDA #0
  STA __pf_ndig
__sc_char_loop:
  JSR __sc_width
  BCS __sc_char_done
  JSR __sc_getc
  BCS __sc_char_eof
  STA __pf_ch
  LDY #S_SUP
  LDA (__sp),Y
  BNE __sc_char_next
  LDY __pf_ndig
  CPY #46
  BCS __sc_char_next
  LDA __pf_ch
  STA __pf_fbuf,Y
__sc_char_next:
  INC __pf_ndig
  JMP __sc_char_loop
__sc_char_eof:
  LDA __pf_ndig
  BNE __sc_char_done
  LDA #STAT_INPUT
  JMP __sc_fail
__sc_char_done:
  LDY #S_SUP
  LDA (__sp),Y
  BNE __sc_char_out
  // Store exactly ndig bytes, no NUL. Reuse fbuf.
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  STA __t0
  LDA __pf_val+1
  STA __t0+1
  LDY #0
__sc_char_cp:
  CPY __pf_ndig
  BCS __sc_char_asn
  LDA __pf_fbuf,Y
  STA (__t0),Y
  INY
  JMP __sc_char_cp
__sc_char_asn:
  LDY #S_ASN
  JSR __pf_incw
__sc_char_out:
  RTS

__sc_float:
  JSR __sc_skip
  LDA #0
  STA __pf_ndig
  JSR __sc_width
  BCS __sc_float_in
  JSR __sc_getc
  BCS __sc_float_in
  CMP #'+'
  BEQ __sc_float_sign
  CMP #'-'
  BNE __sc_float_body
__sc_float_sign:
  JSR __sc_float_keep
  JSR __sc_width
  BCS __sc_float_bad
  JSR __sc_getc
  BCS __sc_float_bad
__sc_float_body:
  CMP #'.'
  BEQ __sc_float_dot
  JSR __sc_isdig
  BCS __sc_float_inf
  JSR __sc_float_digits
  JSR __sc_getc
  BCS __sc_float_call
  CMP #'.'
  BNE __sc_float_exp
__sc_float_dot:
  JSR __sc_float_keep
  // Fraction parsing finishes the conversion itself. A JSR here would return
  // into a second strtod call when the number has an exponent.
  JMP __sc_float_frac
__sc_float_exp:
  CMP #'e'
  BEQ __sc_float_e
  CMP #'E'
  BNE __sc_float_unget
  // fall through
__sc_float_e:
  JSR __sc_float_keep
  JSR __sc_width
  BCS __sc_float_pop_e
  JSR __sc_getc
  BCS __sc_float_pop_e
  CMP #'+'
  BEQ __sc_float_esign
  CMP #'-'
  BNE __sc_float_edig
__sc_float_esign:
  JSR __sc_float_keep
  JSR __sc_width
  BCS __sc_float_pop_e
  JSR __sc_getc
  BCS __sc_float_pop_e
__sc_float_edig:
  JSR __sc_isdig
  BCS __sc_float_pop_e
  JSR __sc_float_digits
  JMP __sc_float_call
__sc_float_unget:
  JSR __sc_unget
  JMP __sc_float_call
__sc_float_inf:
  // Not a digit and not a dot: try inf/nan, otherwise fail.
  JMP __sc_float_word
__sc_float_bad:
  LDA #STAT_MATCH
  JMP __sc_fail
__sc_float_in:
  LDA #STAT_INPUT
  JMP __sc_fail
__sc_float_pop_e:
  // Drop a trailing e/sign that had no digits, then convert what remains.
  DEC __pf_ndig
  LDY __pf_ndig
  LDA __pf_fbuf,Y
  JSR __sc_unget
  LDA __pf_ndig
  BEQ __sc_float_call
  DEY
  LDA __pf_fbuf,Y
  CMP #'e'
  BEQ __sc_float_pop2
  CMP #'E'
  BNE __sc_float_call
__sc_float_pop2:
  DEC __pf_ndig
  LDY __pf_ndig
  LDA __pf_fbuf,Y
  JSR __sc_unget
__sc_float_call:
  LDA __pf_ndig
  BNE __sc_float_go
  LDA #STAT_MATCH
  JMP __sc_fail
__sc_float_go:
  LDY __pf_ndig
  LDA #0
  STA __pf_fbuf,Y
  LDY #S_SUP
  LDA (__sp),Y
  BNE __sc_float_tmp
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  STA __pf_ptr
  LDA __pf_val+1
  STA __pf_ptr+1
  JMP __sc_float_call2
__sc_float_tmp:
  LDA #%lo(__pf_ftmp)
  STA __pf_ptr
  LDA #%hi(__pf_ftmp)
  STA __pf_ptr+1
__sc_float_call2:
  JSR __pf_c_save
  ; The conversion overwrites the frame, including the suppress flag.
  LDY #S_SUP
  LDA (__sp),Y
  STA __pf_ch
  LDX #0
  LDY #0
  JSR __pushxy
  LDX #%lo(__pf_fbuf)
  LDY #%hi(__pf_fbuf)
  JSR __pushxy
  LDX __pf_ptr
  LDY __pf_ptr+1
  JSR pf_strtod
  JSR __pf_c_restore
  LDA __pf_ch
  BNE __sc_float_out
  LDY #S_ASN
  JSR __pf_incw
__sc_float_out:
  RTS

__sc_float_keep:
  LDY __pf_ndig
  CPY #46
  BCS __sc_float_keep_done
  STA __pf_fbuf,Y
  INC __pf_ndig
__sc_float_keep_done:
  RTS

__sc_isdig:
  CMP #'0'
  BCC __sc_isdig_no
  CMP #'9'+1
  BCS __sc_isdig_no
  CLC
  RTS
__sc_isdig_no:
  SEC
  RTS

// A is a digit. Consume the run, keeping characters.
__sc_float_digits:
  JSR __sc_float_keep
__sc_float_digits_loop:
  JSR __sc_width
  BCS __sc_float_digits_done
  JSR __sc_getc
  BCS __sc_float_digits_done
  JSR __sc_isdig
  BCC __sc_float_digits
  JMP __sc_unget
__sc_float_digits_done:
  RTS

__sc_float_frac:
__sc_float_frac_loop:
  JSR __sc_width
  BCS __sc_float_frac_exp
  JSR __sc_getc
  BCS __sc_float_frac_exp
  JSR __sc_isdig
  BCS __sc_float_frac_maybe_e
  JSR __sc_float_keep
  JMP __sc_float_frac_loop
__sc_float_frac_maybe_e:
  CMP #'e'
  BNE __far_14
    JMP __sc_float_e
__far_14:
  CMP #'E'
  BNE __far_13
    JMP __sc_float_e
__far_13:
  JSR __sc_unget
  JMP __sc_float_call
__sc_float_frac_exp:
  JMP __sc_float_call

__sc_float_word:
  // A is the first letter. Accept inf/infinity/nan, case insensitive.
  JSR __sc_float_keep
  JMP __sc_float_word_more
__sc_float_word_loop:
  JSR __sc_width
  BCC __far_12
    JMP __sc_float_call
__far_12:
  JSR __sc_getc
  BCC __far_11
    JMP __sc_float_call
__far_11:
  CMP #'A'
  BCC __sc_float_word_no
  CMP #'Z'+1
  BCS __sc_float_word_low
  ORA #$20
__sc_float_word_low:
  CMP #'a'
  BCC __sc_float_word_no
  CMP #'z'+1
  BCS __sc_float_word_no
  JSR __sc_float_keep
__sc_float_word_more:
  JMP __sc_float_word_loop
__sc_float_word_no:
  JSR __sc_unget
  JMP __sc_float_call

__sc_set_clear:
  LDX #31
  LDA #0
__sc_set_clear_loop:
  STA __pf_set,X
  DEX
  BPL __sc_set_clear_loop
  RTS

__sc_set_bit:
  PHA
  LSR A
  LSR A
  LSR A
  TAX
  PLA
  AND #7
  TAY
  LDA __pf_set,X
  ORA __pf_bits,Y
  STA __pf_set,X
  RTS

__sc_scan_done:
  LDY #S_STAT
  LDA (__sp),Y
  CMP #STAT_INPUT
  BNE __sc_scan_asn
  LDY #S_ASN
  LDA (__sp),Y
  INY
  ORA (__sp),Y
  BNE __sc_scan_asn
  LDA #$ff
  STA __t0
  STA __t0+1
  JMP __sc_scan_ret
__sc_scan_asn:
  LDY #S_ASN
  JSR __pf_ldw
__sc_scan_ret:
  LDY #F_RES
  LDA (__sp),Y
  STA __t2
  INY
  LDA (__sp),Y
  STA __t2+1
  LDY #0
  LDA __t0
  STA (__t2),Y
  INY
  LDA __t0+1
  STA (__t2),Y
  LDX #FRAME
  JMP __pf_free

__pf_scan:
__pf_scan_loop:
  LDY #S_STAT
  LDA (__sp),Y
  BNE __sc_scan_done
  JSR __pf_next
  BEQ __sc_scan_done
  JSR __sc_space
  BCC __pf_scan_lit
  JSR __sc_skip
  JMP __pf_scan_loop
__pf_scan_lit:
  CMP #'%'
  BEQ __pf_scan_spec
  STA __pf_ch
  JSR __sc_getc
  BCS __pf_scan_in
  CMP __pf_ch
  BEQ __pf_scan_loop
  JSR __sc_unget
  LDA #STAT_MATCH
  JSR __sc_fail
  JMP __sc_scan_done
__pf_scan_in:
  LDA #STAT_INPUT
  JSR __sc_fail
  JMP __sc_scan_done
__pf_scan_spec:
  LDA #0
  LDY #S_SUP
  STA (__sp),Y
  LDY #S_LEN
  STA (__sp),Y
  JSR __sc_width_init
  JSR __pf_next
  BNE __far_10
    JMP __sc_scan_done
__far_10:
  CMP #'*'
  BNE __pf_scan_w
  LDA #1
  LDY #S_SUP
  STA (__sp),Y
  JSR __pf_next
  BNE __far_9
    JMP __sc_scan_done
__far_9:
__pf_scan_w:
  CMP #'0'
  BCC __pf_scan_len
  CMP #'9'+1
  BCS __pf_scan_len
  // First digit: start width at 0 then accumulate.
  PHA
  LDY #S_WID
  LDA (__sp),Y
  CMP #$ff
  BNE __pf_scan_wdig
  LDA #0
  STA (__sp),Y
  INY
  STA (__sp),Y
__pf_scan_wdig:
  PLA
  SEC
  SBC #'0'
  LDY #S_WID
  JSR __pf_acc10
  JSR __pf_next
  BNE __far_8
    JMP __sc_scan_done
__far_8:
  CMP #'0'
  BCC __pf_scan_len
  CMP #'9'+1
  BCS __pf_scan_len
  PHA
  JMP __pf_scan_wdig
__pf_scan_len:
  CMP #'h'
  BNE __pf_scan_l
  JSR __pf_next
  CMP #'h'
  BEQ __pf_scan_hh
  PHA
  LDA #LEN_H
  LDY #S_LEN
  STA (__sp),Y
  PLA
  JMP __pf_scan_conv
__pf_scan_hh:
  LDA #LEN_HH
  LDY #S_LEN
  STA (__sp),Y
  JSR __pf_next
  JMP __pf_scan_conv
__pf_scan_l:
  CMP #'l'
  BNE __pf_scan_j
  JSR __pf_next
  CMP #'l'
  BEQ __pf_scan_ll
  PHA
  LDA #LEN_L
  LDY #S_LEN
  STA (__sp),Y
  PLA
  JMP __pf_scan_conv
__pf_scan_ll:
  LDA #LEN_LL
  LDY #S_LEN
  STA (__sp),Y
  JSR __pf_next
  JMP __pf_scan_conv
__pf_scan_j:
  CMP #'j'
  BNE __pf_scan_z
  LDA #LEN_LL
  JMP __pf_scan_one
__pf_scan_z:
  CMP #'z'
  BNE __pf_scan_conv
  LDA #LEN_H
__pf_scan_one:
  LDY #S_LEN
  STA (__sp),Y
  JSR __pf_next
__pf_scan_conv:
  // scanf lengths live in S_LEN. The store helper reads F_LEN, same offset.
  CMP #'d'
  BNE __far_39
    JMP __pf_scan_d
__far_39:
  CMP #'u'
  BNE __far_38
    JMP __pf_scan_u
__far_38:
  CMP #'i'
  BNE __far_37
    JMP __pf_scan_i
__far_37:
  CMP #'x'
  BNE __far_7
    JMP __pf_scan_x
__far_7:
  CMP #'X'
  BNE __far_36
    JMP __pf_scan_x
__far_36:
  CMP #'o'
  BNE __far_6
    JMP __pf_scan_o
__far_6:
  CMP #'p'
  BNE __far_5
    JMP __pf_scan_p
__far_5:
  CMP #'s'
  BNE __far_4
    JMP __pf_scan_s
__far_4:
  CMP #'c'
  BNE __far_3
    JMP __pf_scan_c
__far_3:
  CMP #'n'
  BNE __far_2
    JMP __pf_scan_n
__far_2:
  CMP #'['
  BNE __far_1
    JMP __pf_scan_set
__far_1:
  CMP #'f'
  BNE __far_35
    JMP __pf_scan_f
__far_35:
  CMP #'e'
  BNE __far_34
    JMP __pf_scan_f
__far_34:
  CMP #'g'
  BNE __far_33
    JMP __pf_scan_f
__far_33:
  CMP #'F'
  BNE __far_32
    JMP __pf_scan_f
__far_32:
  CMP #'E'
  BNE __far_31
    JMP __pf_scan_f
__far_31:
  CMP #'G'
  BNE __far_30
    JMP __pf_scan_f
__far_30:
  CMP #'a'
  BNE __far_29
    JMP __pf_scan_f
__far_29:
  CMP #'A'
  BNE __far_28
    JMP __pf_scan_f
__far_28:
  CMP #'%'
  BNE __pf_scan_bad
  LDA #'%'
  STA __pf_ch
  JSR __sc_getc
  BCC __far_27
    JMP __pf_scan_in
__far_27:
  CMP #'%'
  BNE __far_26
    JMP __pf_scan_loop
__far_26:
  JSR __sc_unget
__pf_scan_bad:
  LDA #STAT_MATCH
  JSR __sc_fail
  JMP __sc_scan_done
__pf_scan_d:
  LDA #1
  STA __pf_i
  LDA #10
  JSR __sc_int
  JMP __pf_scan_loop
__pf_scan_u:
  LDA #0
  STA __pf_i
  LDA #10
  JSR __sc_int
  JMP __pf_scan_loop
__pf_scan_i:
  LDA #2
  STA __pf_i
  LDA #10
  JSR __sc_int
  JMP __pf_scan_loop
__pf_scan_x:
  LDA #0
  STA __pf_i
  LDA #16
  JSR __sc_int
  JMP __pf_scan_loop
__pf_scan_o:
  LDA #0
  STA __pf_i
  LDA #8
  JSR __sc_int
  JMP __pf_scan_loop
__pf_scan_p:
  LDA #0
  LDY #S_LEN
  STA (__sp),Y
  STA __pf_i
  LDA #16
  JSR __sc_int
  JMP __pf_scan_loop
__pf_scan_s:
  LDA #0
  JSR __sc_text
  JMP __pf_scan_loop
__pf_scan_c:
  JSR __sc_char
  JMP __pf_scan_loop
__pf_scan_n:
  LDY #S_NRD
  JSR __pf_ldw
  LDA __t0
  STA __pf_fbuf
  LDA __t0+1
  STA __pf_fbuf+1
  LDA #0
  STA __pf_fbuf+2
  STA __pf_fbuf+3
  STA __pf_fbuf+4
  STA __pf_fbuf+5
  STA __pf_fbuf+6
  STA __pf_fbuf+7
  LDY #S_SUP
  LDA (__sp),Y
  BEQ __far_25
    JMP __pf_scan_loop
__far_25:
  LDY #S_LEN
  LDA (__sp),Y
  LDY #F_LEN
  STA (__sp),Y
  LDX #2
  JSR __pf_arg
  LDA __pf_val
  STA __t0
  LDA __pf_val+1
  STA __t0+1
  JSR __pf_store
  JMP __pf_scan_loop
__pf_scan_f:
  JSR __sc_float
  JMP __pf_scan_loop
__pf_scan_set:
  JSR __sc_set_clear
  LDA #0
  STA __pf_i
  JSR __pf_next
  BNE __far_24
    JMP __sc_scan_done
__far_24:
  CMP #'^'
  BNE __pf_scan_set_ch
  LDA #1
  STA __pf_i
  JSR __pf_next
  BNE __far_23
    JMP __sc_scan_done
__far_23:
__pf_scan_set_ch:
  CMP #']'
  BEQ __pf_scan_set_go
  STA __pf_ch
  JSR __pf_next
  BNE __far_22
    JMP __sc_scan_done
__far_22:
  CMP #'-'
  BNE __pf_scan_set_one
  JSR __pf_next
  BNE __far_21
    JMP __sc_scan_done
__far_21:
  CMP #']'
  BEQ __pf_scan_set_dash
  STA __t2
  LDA __pf_ch
__pf_scan_set_range:
  JSR __sc_set_bit
  CMP __t2
  BEQ __pf_scan_set_next
  CLC
  ADC #1
  JMP __pf_scan_set_range
__pf_scan_set_dash:
  LDA __pf_ch
  JSR __sc_set_bit
  LDA #'-'
  JSR __sc_set_bit
  JMP __pf_scan_set_go
__pf_scan_set_one:
  PHA
  LDA __pf_ch
  JSR __sc_set_bit
  PLA
  JMP __pf_scan_set_ch
__pf_scan_set_next:
  JSR __pf_next
  JMP __pf_scan_set_ch
__pf_scan_set_go:
  LDA #1
  JSR __sc_text
  JMP __pf_scan_loop

// ---- fgetc / ungetc --------------------------------------------------

__gc_ret:
  // A = lo, X = hi. Result pointer at frame 0. Frees 8 bytes.
  STA __t0
  STX __t0+1
  LDY #0
  LDA (__sp),Y
  STA __t2
  INY
  LDA (__sp),Y
  STA __t2+1
  LDY #0
  LDA __t0
  STA (__t2),Y
  INY
  LDA __t0+1
  STA (__t2),Y
  LDX #10
  JMP __pf_free

__gc_file:
  LDY #2
  LDA (__sp),Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t0+1
  RTS

getc:
  CLD
  STX __t0
  STY __t0+1
  LDX #8
  JSR __pf_alloc
  // Arguments sit just above this 8-byte frame, not FRAME bytes up.
  LDY #8
  LDA (__sp),Y
  LDY #2
  STA (__sp),Y
  LDY #9
  LDA (__sp),Y
  LDY #3
  STA (__sp),Y
  JSR __gc_file
  LDY #ORIENT_OFF
  LDA (__t0),Y
  BNE __gc_buf
  LDA #$ff
  STA (__t0),Y
__gc_buf:
  LDY #BUF_OFF
  LDA (__t0),Y
  INY
  ORA (__t0),Y
  BNE __gc_unget
  JMP __gc_raw
__gc_unget:
  LDY #UNGET_OFF
  LDA (__t0),Y
  BEQ __gc_cached
  SEC
  SBC #1
  STA (__t0),Y
  CLC
  ADC #UNGET_BUF_OFF
  TAY
  LDA (__t0),Y
  LDX #0
  JMP __gc_ret
__gc_cached:
  LDY #RINDEX_OFF
  LDA (__t0),Y
  STA __pf_repn
  INY
  LDA (__t0),Y
  STA __pf_repn+1
  LDY #RLIMIT_OFF
  LDA __pf_repn
  CMP (__t0),Y
  INY
  LDA __pf_repn+1
  SBC (__t0),Y
  BCS __gc_fill
  JMP __gc_from_buf
__gc_fill:
  LDY #EOF_OFF
  LDA (__t0),Y
  LDY #ERR_OFF
  ORA (__t0),Y
  BEQ __gc_fill_go
  LDA #$ff
  TAX
  JMP __gc_ret
__gc_fill_go:
  LDA #0
  LDY #RINDEX_OFF
  STA (__t0),Y
  INY
  STA (__t0),Y
  LDY #RLIMIT_OFF
  STA (__t0),Y
  INY
  STA (__t0),Y
  LDY #MODE_OFF
  LDA (__t0),Y
  CMP #1
  BNE __gc_line
  JMP __gc_full
__gc_line:
  // Read one byte at a time until newline, a full buffer, or EOF.
__gc_line_loop:
  JSR __gc_read1
  BCS __gc_after_fill
  LDY #RLIMIT_OFF
  LDA (__t0),Y
  // The byte is in __pf_ch. Newline or full buffer stops the fill.
  LDA __pf_ch
  CMP #10
  BEQ __gc_after_fill
  LDY #RLIMIT_OFF
  LDA (__t0),Y
  LDY #BUFSIZE_OFF
  CMP (__t0),Y
  BNE __gc_line_loop
  LDY #RLIMIT_OFF+1
  LDA (__t0),Y
  LDY #BUFSIZE_OFF+1
  CMP (__t0),Y
  BNE __gc_line_loop
  JMP __gc_after_fill
__gc_full:
  LDY #BUF_OFF
  LDA (__t0),Y
  STA __pf_ptr
  INY
  LDA (__t0),Y
  STA __pf_ptr+1
  LDY #BUFSIZE_OFF
  LDA (__t0),Y
  STA __pf_len
  INY
  LDA (__t0),Y
  STA __pf_len+1
__gc_full_loop:
  LDA __pf_len
  ORA __pf_len+1
  BEQ __gc_after_fill
  JSR __gc_read_n
  BCS __gc_after_fill
  JMP __gc_full_loop
__gc_after_fill:
  JSR __gc_file
  LDY #EOF_OFF
  LDA (__t0),Y
  LDY #ERR_OFF
  ORA (__t0),Y
  BEQ __gc_from_buf
  LDY #RLIMIT_OFF
  LDA (__t0),Y
  INY
  ORA (__t0),Y
  BNE __gc_from_buf
  LDA #$ff
  TAX
  JMP __gc_ret
__gc_from_buf:
  JSR __gc_file
  LDY #BUF_OFF
  LDA (__t0),Y
  CLC
  LDY #RINDEX_OFF
  ADC (__t0),Y
  STA __pf_ptr
  LDY #BUF_OFF+1
  LDA (__t0),Y
  LDY #RINDEX_OFF+1
  ADC (__t0),Y
  STA __pf_ptr+1
  LDY #RINDEX_OFF
  LDA (__t0),Y
  CLC
  ADC #1
  STA (__t0),Y
  INY
  LDA (__t0),Y
  ADC #0
  STA (__t0),Y
  JMP __gc_ret_fix
__gc_raw:
  JSR __gc_read1
  BCS __gc_raw_eof
  LDA __pf_ch
  LDX #0
  JMP __gc_ret
__gc_raw_eof:
  LDA #$ff
  TAX
  JMP __gc_ret

// Read one byte onto the current buffer end. Carry set on EOF/error.
// Byte returned in __pf_ch. rlimit incremented on success.
__gc_read1:
  JSR __gc_file
  LDY #BUF_OFF
  LDA (__t0),Y
  CLC
  LDY #RLIMIT_OFF
  ADC (__t0),Y
  STA __pf_ptr
  LDY #BUF_OFF+1
  LDA (__t0),Y
  LDY #RLIMIT_OFF+1
  ADC (__t0),Y
  STA __pf_ptr+1
  LDA #1
  STA __pf_len
  LDA #0
  STA __pf_len+1
  JSR __gc_read_n
  BCS __gc_read1_fail
  LDA __pf_ptr
  STA __t2
  LDA __pf_ptr+1
  STA __t2+1
  LDY #0
  LDA (__t2),Y
  STA __pf_ch
  CLC
  RTS
__gc_read1_fail:
  SEC
  RTS

// __pf_ptr = dest, __pf_len = length. Adds a successful count to rlimit
// and subtracts it from __pf_len, advancing __pf_ptr. Carry set if no
// byte was read.
__gc_read_n:
  JSR __gc_file
  LDY #FD_OFF
  LDA (__t0),Y
  TAX
  INY
  LDA (__t0),Y
  TAY
  JSR __pushxy
  LDX __pf_ptr
  LDY __pf_ptr+1
  JSR __pushxy
  LDX __pf_len
  LDY __pf_len+1
  JSR __pushxy
  CLC
  LDA __sp
  ADC #6+4
  TAX
  LDA __sp+1
  ADC #0
  TAY
  JSR read
  // Result at frame offset 4.
  LDY #4
  LDA (__sp),Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t0+1
  BMI __gc_read_err
  ORA __t0
  BEQ __gc_read_eof
  JSR __gc_file
  LDY #RLIMIT_OFF
  LDA (__t0),Y
  // __t0 was just replaced by the FILE pointer. The count was in the
  // previous __t0. Reload the count from the frame.
  LDY #4
  LDA (__sp),Y
  LDY #RLIMIT_OFF
  CLC
  ADC (__t0),Y
  STA (__t0),Y
  LDY #5
  LDA (__sp),Y
  LDY #RLIMIT_OFF+1
  ADC (__t0),Y
  STA (__t0),Y
  LDY #4
  LDA (__sp),Y
  CLC
  ADC __pf_ptr
  STA __pf_ptr
  LDY #5
  LDA (__sp),Y
  ADC __pf_ptr+1
  STA __pf_ptr+1
  LDY #4
  LDA __pf_len
  SEC
  SBC (__sp),Y
  STA __pf_len
  INY
  LDA __pf_len+1
  SBC (__sp),Y
  STA __pf_len+1
  CLC
  RTS
__gc_read_err:
  JSR __gc_file
  LDA #1
  LDY #ERR_OFF
  STA (__t0),Y
  SEC
  RTS
__gc_read_eof:
  JSR __gc_file
  LDY #RLIMIT_OFF
  LDA (__t0),Y
  INY
  ORA (__t0),Y
  BNE __gc_read_eof_soft
  LDA #1
  LDY #EOF_OFF
  STA (__t0),Y
__gc_read_eof_soft:
  SEC
  RTS

__gc_ret_fix:
  // A holds the byte only if we loaded it. Load through a ZP pointer.
  LDA __pf_ptr
  STA __t0
  LDA __pf_ptr+1
  STA __t0+1
  LDY #0
  LDA (__t0),Y
  LDX #0
  JMP __gc_ret

// ---- ungetc ----------------------------------------------------------

__ug_ret:
  STA __t0
  STX __t0+1
  LDY #0
  LDA (__sp),Y
  STA __t2
  INY
  LDA (__sp),Y
  STA __t2+1
  LDY #0
  LDA __t0
  STA (__t2),Y
  INY
  LDA __t0+1
  STA (__t2),Y
  LDX #FRAME+4
  JMP __pf_free

// Public entries. Each one is its own section so the shim can keep it.

.section ".text.printf", "ax", @progbits
.global printf
.type printf, @function
printf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_FILE
  LDY #F_KIND
  STA (__sp),Y
  LDA PAGED_STDOUT_PTR
  LDY #F_PTR
  STA (__sp),Y
  LDA PAGED_STDOUT_PTR+1
  INY
  STA (__sp),Y
  LDX #0
  LDA #0
  JSR __pf_bind
  JMP __pf_run

.section ".text.fprintf", "ax", @progbits
.global fprintf
.type fprintf, @function
fprintf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDX #0
  LDY #F_PTR
  JSR __pf_copyarg
  LDX #2
  LDA #0
  JSR __pf_bind
  JMP __pf_run

.section ".text.sprintf", "ax", @progbits
.global sprintf
.type sprintf, @function
sprintf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_STR
  LDY #F_KIND
  STA (__sp),Y
  LDA #1
  LDY #F_NUL
  STA (__sp),Y
  LDA #$ff
  LDY #F_LIM
  STA (__sp),Y
  INY
  STA (__sp),Y
  LDX #0
  LDY #F_PTR
  JSR __pf_copyarg
  LDX #2
  LDA #0
  JSR __pf_bind
  JMP __pf_run

.section ".text.snprintf", "ax", @progbits
.global snprintf
.type snprintf, @function
snprintf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_STR
  LDY #F_KIND
  STA (__sp),Y
  LDX #0
  LDY #F_PTR
  JSR __pf_copyarg
  LDX #2
  JSR __pf_ldarg
  LDA __t0
  ORA __t0+1
  BEQ __sn_nospace
  LDA __t0
  BNE __sn_dec
  DEC __t0+1
__sn_dec:
  DEC __t0
  LDA #1
  LDY #F_NUL
  STA (__sp),Y
__sn_store:
  LDY #F_LIM
  JSR __pf_stw
  LDX #4
  LDA #0
  JSR __pf_bind
  JMP __pf_run
__sn_nospace:
  JMP __sn_store

.section ".text.vprintf", "ax", @progbits
.global vprintf
.type vprintf, @function
vprintf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_FILE
  LDY #F_KIND
  STA (__sp),Y
  LDA PAGED_STDOUT_PTR
  LDY #F_PTR
  STA (__sp),Y
  LDA PAGED_STDOUT_PTR+1
  INY
  STA (__sp),Y
  LDX #0
  LDA #1
  JSR __pf_bind
  JMP __pf_run

.section ".text.vfprintf", "ax", @progbits
.global vfprintf
.type vfprintf, @function
vfprintf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDX #0
  LDY #F_PTR
  JSR __pf_copyarg
  LDX #2
  LDA #1
  JSR __pf_bind
  JMP __pf_run

.section ".text.vsprintf", "ax", @progbits
.global vsprintf
.type vsprintf, @function
vsprintf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_STR
  LDY #F_KIND
  STA (__sp),Y
  LDA #1
  LDY #F_NUL
  STA (__sp),Y
  LDA #$ff
  LDY #F_LIM
  STA (__sp),Y
  INY
  STA (__sp),Y
  LDX #0
  LDY #F_PTR
  JSR __pf_copyarg
  LDX #2
  LDA #1
  JSR __pf_bind
  JMP __pf_run

.section ".text.vsnprintf", "ax", @progbits
.global vsnprintf
.type vsnprintf, @function
vsnprintf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_STR
  LDY #F_KIND
  STA (__sp),Y
  LDX #0
  LDY #F_PTR
  JSR __pf_copyarg
  LDX #2
  JSR __pf_ldarg
  LDA __t0
  ORA __t0+1
  BEQ __vsn_nospace
  LDA __t0
  BNE __vsn_dec
  DEC __t0+1
__vsn_dec:
  DEC __t0
  LDA #1
  LDY #F_NUL
  STA (__sp),Y
__vsn_store:
  LDY #F_LIM
  JSR __pf_stw
  LDX #4
  LDA #1
  JSR __pf_bind
  JMP __pf_run
__vsn_nospace:
  JMP __vsn_store

.section ".text.scanf", "ax", @progbits
.global scanf
.type scanf, @function
scanf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_FILE
  LDY #S_KIND
  STA (__sp),Y
  LDA PAGED_STDIN_PTR
  LDY #S_PTR
  STA (__sp),Y
  LDA PAGED_STDIN_PTR+1
  INY
  STA (__sp),Y
  LDX #0
  LDA #0
  JSR __pf_bind
  JMP __pf_scan

.section ".text.fscanf", "ax", @progbits
.global fscanf
.type fscanf, @function
fscanf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDX #0
  LDY #S_PTR
  JSR __pf_copyarg
  LDX #2
  LDA #0
  JSR __pf_bind
  JMP __pf_scan

.section ".text.sscanf", "ax", @progbits
.global sscanf
.type sscanf, @function
sscanf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_STR
  LDY #S_KIND
  STA (__sp),Y
  LDX #0
  LDY #S_PTR
  JSR __pf_copyarg
  LDX #2
  LDA #0
  JSR __pf_bind
  JMP __pf_scan

.section ".text.vscanf", "ax", @progbits
.global vscanf
.type vscanf, @function
vscanf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_FILE
  LDY #S_KIND
  STA (__sp),Y
  LDA PAGED_STDIN_PTR
  LDY #S_PTR
  STA (__sp),Y
  LDA PAGED_STDIN_PTR+1
  INY
  STA (__sp),Y
  LDX #0
  LDA #1
  JSR __pf_bind
  JMP __pf_scan

.section ".text.vfscanf", "ax", @progbits
.global vfscanf
.type vfscanf, @function
vfscanf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDX #0
  LDY #S_PTR
  JSR __pf_copyarg
  LDX #2
  LDA #1
  JSR __pf_bind
  JMP __pf_scan

.section ".text.vsscanf", "ax", @progbits
.global vsscanf
.type vsscanf, @function
vsscanf:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDA #KIND_STR
  LDY #S_KIND
  STA (__sp),Y
  LDX #0
  LDY #S_PTR
  JSR __pf_copyarg
  LDX #2
  LDA #1
  JSR __pf_bind
  JMP __pf_scan

.section ".text.fgetc", "ax", @progbits
.global fgetc
.type fgetc, @function
fgetc:
  JMP getc

.section ".text.getchar", "ax", @progbits
.global getchar
.type getchar, @function
getchar:
  CLD
  STX __t0
  STY __t0+1
  LDA PAGED_STDIN_PTR
  TAX
  LDA PAGED_STDIN_PTR+1
  TAY
  JSR __pushxy
  LDX __t0
  LDY __t0+1
  JSR getc
  RTS

.section ".text.ungetc", "ax", @progbits
.global ungetc
.type ungetc, @function
ungetc:
  CLD
  STX __t0
  STY __t0+1
  LDX #FRAME
  JSR __pf_alloc
  LDX #0
  JSR __pf_ldarg
  LDA __t0
  STA __pf_ch
  LDA __t0+1
  STA __pf_i
  LDX #2
  LDY #2
  JSR __pf_copyarg
  // EOF in either byte.
  LDA __pf_ch
  AND __pf_i
  CMP #$ff
  BEQ __ug_eof
  JMP __ug_room
__ug_eof:
  LDA #$ff
  TAX
  JMP __ug_ret
__ug_room:
  LDY #2
  LDA (__sp),Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t0+1
  LDY #UNGET_OFF
  LDA (__t0),Y
  CMP #10
  BCC __ug_push
  LDA #$ff
  TAX
  JMP __ug_ret
__ug_push:
  TAY
  CLC
  ADC #UNGET_BUF_OFF
  // Y was the index, then we overwrote Y. Use X.
  TAX
  LDY #UNGET_OFF
  LDA (__t0),Y
  TAY
  LDA __pf_ch
  PHA
  TYA
  CLC
  ADC #UNGET_BUF_OFF
  TAY
  PLA
  STA (__t0),Y
  LDY #UNGET_OFF
  LDA (__t0),Y
  CLC
  ADC #1
  STA (__t0),Y
  LDA #0
  LDY #EOF_OFF
  STA (__t0),Y
  LDA __pf_ch
  LDX #0
  JMP __ug_ret

// The compiler rewrites a constant format to one of these. The signature
// is unchanged, so each one is the general routine.
#define ALIAS(name, target) \
  .section ".text." #name, "ax", @progbits \
  .global name \
  name: \
  JMP target

.section ".text.__printf_int", "ax", @progbits
.global __printf_int
.type __printf_int, @function
__printf_int:
  JMP printf
.section ".text.__printf_long", "ax", @progbits
.global __printf_long
.type __printf_long, @function
__printf_long:
  JMP printf
.section ".text.__printf_fp", "ax", @progbits
.global __printf_fp
.type __printf_fp, @function
__printf_fp:
  JMP printf
.section ".text.__fprintf_int", "ax", @progbits
.global __fprintf_int
.type __fprintf_int, @function
__fprintf_int:
  JMP fprintf
.section ".text.__fprintf_long", "ax", @progbits
.global __fprintf_long
.type __fprintf_long, @function
__fprintf_long:
  JMP fprintf
.section ".text.__fprintf_fp", "ax", @progbits
.global __fprintf_fp
.type __fprintf_fp, @function
__fprintf_fp:
  JMP fprintf
.section ".text.__sprintf_int", "ax", @progbits
.global __sprintf_int
.type __sprintf_int, @function
__sprintf_int:
  JMP sprintf
.section ".text.__sprintf_long", "ax", @progbits
.global __sprintf_long
.type __sprintf_long, @function
__sprintf_long:
  JMP sprintf
.section ".text.__sprintf_fp", "ax", @progbits
.global __sprintf_fp
.type __sprintf_fp, @function
__sprintf_fp:
  JMP sprintf
.section ".text.__snprintf_int", "ax", @progbits
.global __snprintf_int
.type __snprintf_int, @function
__snprintf_int:
  JMP snprintf
.section ".text.__snprintf_long", "ax", @progbits
.global __snprintf_long
.type __snprintf_long, @function
__snprintf_long:
  JMP snprintf
.section ".text.__snprintf_fp", "ax", @progbits
.global __snprintf_fp
.type __snprintf_fp, @function
__snprintf_fp:
  JMP snprintf
