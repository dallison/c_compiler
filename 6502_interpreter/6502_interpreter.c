//
//  6502_interpreter.c
//  6502_interpreter
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_interpreter.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "6502_disassembler.h"
#include "6502_devices.h"
#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/mach_time.h> /* mach_absolute_time */
#endif

#define _6502_instruction_decl(x) static void Interpret_##x(_6502Interpreter*);
#define _6502_instruction_def(x) static void Interpret_##x(_6502Interpreter* interpreter)
#define _6502_instruction(x) Interpret_##x

typedef struct {
  void (*func)(_6502Interpreter*);
  int bytes;
  int cycles;
} Instruction;

// Instruction function declarations, one per opcode.
_6502_instruction_decl(brk)
_6502_instruction_decl(ora_zp_x_indirect)
_6502_instruction_decl(cop_zp)
_6502_instruction_decl(ora_zp)
_6502_instruction_decl(tsb_zp)
_6502_instruction_decl(ora_zp)
_6502_instruction_decl(asl_zp)
_6502_instruction_decl(ora_zp)
_6502_instruction_decl(php)
_6502_instruction_decl(ora_immed)
_6502_instruction_decl(asl_accum)
_6502_instruction_decl(phd)
_6502_instruction_decl(tsb_zp)
_6502_instruction_decl(ora_abs)
_6502_instruction_decl(asl_abs)
_6502_instruction_decl(ora_abl)

_6502_instruction_decl(bpl)
_6502_instruction_decl(ora_zp_indirect_y)
_6502_instruction_decl(ora_zp_indirect)
_6502_instruction_decl(ora_zp_s_indirect_y)
_6502_instruction_decl(trb_zp)
_6502_instruction_decl(ora_abs_x)
_6502_instruction_decl(asl_zp_x)
_6502_instruction_decl(ora_zp_y)
_6502_instruction_decl(clc)
_6502_instruction_decl(ora_abs_y)
_6502_instruction_decl(inc_a)
_6502_instruction_decl(tcs)
_6502_instruction_decl(trb_abs)
_6502_instruction_decl(ora_abs_x)
_6502_instruction_decl(asl_abs_x)
_6502_instruction_decl(ora_abs_x)

_6502_instruction_decl(jsr_abs)
_6502_instruction_decl(and_zp_x_indirect)
_6502_instruction_decl(lsr_accum)
_6502_instruction_decl(and_zp_s)
_6502_instruction_decl(bit_zp)
_6502_instruction_decl(and_zp)
_6502_instruction_decl(rol_zp)
_6502_instruction_decl(and_zp)
_6502_instruction_decl(plp)
_6502_instruction_decl(and_immed)
_6502_instruction_decl(rol_accum)
_6502_instruction_decl(pld)
_6502_instruction_decl(bit_abs)
_6502_instruction_decl(and_abs)
_6502_instruction_decl(rol_abs)
_6502_instruction_decl(and_al)

_6502_instruction_decl(bmi)
_6502_instruction_decl(and_zp_indirect_y)
_6502_instruction_decl(and_zp_indirect)
_6502_instruction_decl(and_zp_s_indirect_y)
_6502_instruction_decl(bit_zp_x)
_6502_instruction_decl(and_zp_x)
_6502_instruction_decl(rol_zp_x)
_6502_instruction_decl(and_zp_y)
_6502_instruction_decl(sec)
_6502_instruction_decl(and_abs_y)
_6502_instruction_decl(dec_accum)
_6502_instruction_decl(tsc)
_6502_instruction_decl(bit_abs_x)
_6502_instruction_decl(and_abs_x)
_6502_instruction_decl(rol_abs_x)
_6502_instruction_decl(and_abs_x)

_6502_instruction_decl(rti)
_6502_instruction_decl(eor_zp_x_indirect)
_6502_instruction_decl(wdm)
_6502_instruction_decl(eor_zp_s)
_6502_instruction_decl(mvp_s_zp)
_6502_instruction_decl(eor_zp)
_6502_instruction_decl(lsr_zp)
_6502_instruction_decl(eor_zp)
_6502_instruction_decl(pha)
_6502_instruction_decl(eor_immed)
_6502_instruction_decl(lsr_accum)
_6502_instruction_decl(phk)
_6502_instruction_decl(jmp_abs)
_6502_instruction_decl(eor_abs)
_6502_instruction_decl(lsr_abs)
_6502_instruction_decl(eor_abs)

_6502_instruction_decl(bvc)
_6502_instruction_decl(eor_zp_indirect_y)
_6502_instruction_decl(eor_zp_indirect)
_6502_instruction_decl(eor_zp_s_indirect_y)
_6502_instruction_decl(mvn_s_zp)
_6502_instruction_decl(eor_zp_x)
_6502_instruction_decl(lsr_zp_x)
_6502_instruction_decl(eor_zp_x)
_6502_instruction_decl(cli)
_6502_instruction_decl(eor_zp_y)
_6502_instruction_decl(phy)
_6502_instruction_decl(tcd)
_6502_instruction_decl(jmp_abs)
_6502_instruction_decl(eor_zp_x)
_6502_instruction_decl(lsr_zp_x)
_6502_instruction_decl(eor_zp_x)

_6502_instruction_decl(rts)
_6502_instruction_decl(adc_zp_x_indirect)
_6502_instruction_decl(per)
_6502_instruction_decl(adc_zp_s)
_6502_instruction_decl(stz_zp)
_6502_instruction_decl(adc_zp)
_6502_instruction_decl(ror_zp)
_6502_instruction_decl(adc_zp_y)
_6502_instruction_decl(pla)
_6502_instruction_decl(adc_immed)
_6502_instruction_decl(ror_accum)
_6502_instruction_decl(rtl)
_6502_instruction_decl(jmp_abs_indirect)
_6502_instruction_decl(adc_abs)
_6502_instruction_decl(ror_abs)
_6502_instruction_decl(adc_abs)

_6502_instruction_decl(bvs)
_6502_instruction_decl(adc_zp_indirect_y)
_6502_instruction_decl(adc_zp_indirect)
_6502_instruction_decl(adc_zp_s_indirect_y)
_6502_instruction_decl(stz_zp_x)
_6502_instruction_decl(adc_zp_x)
_6502_instruction_decl(ror_zp_x)
_6502_instruction_decl(adc_zp_x)
_6502_instruction_decl(sei)
_6502_instruction_decl(adc_abs_y)
_6502_instruction_decl(ply)
_6502_instruction_decl(tdc)
_6502_instruction_decl(jmp_abs_x_indirect)
_6502_instruction_decl(adc_abs_x)
_6502_instruction_decl(ror_abs_x)
_6502_instruction_decl(adc_abs_x)

_6502_instruction_decl(bra)
_6502_instruction_decl(sta_zp_x_indirect)
_6502_instruction_decl(brl)
_6502_instruction_decl(sta_zp_s)
_6502_instruction_decl(sty_zp)
_6502_instruction_decl(sta_zp)
_6502_instruction_decl(stx_zp)
_6502_instruction_decl(sta_zp)
_6502_instruction_decl(dey)
_6502_instruction_decl(bit_immed)
_6502_instruction_decl(txa)
_6502_instruction_decl(phb)
_6502_instruction_decl(sty_abs)
_6502_instruction_decl(sta_abs)
_6502_instruction_decl(stx_abs)
_6502_instruction_decl(sta_abs)

_6502_instruction_decl(bcc)
_6502_instruction_decl(sta_zp_indirect_y)
_6502_instruction_decl(sta_zp_indirect)
_6502_instruction_decl(sta_zp_s_indirect_y)
_6502_instruction_decl(sty_zp_x)
_6502_instruction_decl(sta_zp_x)
_6502_instruction_decl(stx_zp_y)
_6502_instruction_decl(sta_abs_y)
_6502_instruction_decl(tya)
_6502_instruction_decl(sta_zp_y)
_6502_instruction_decl(txs)
_6502_instruction_decl(txy)
_6502_instruction_decl(stz_abs)
_6502_instruction_decl(sta_abs_x)
_6502_instruction_decl(stz_abs_x)
_6502_instruction_decl(sta_abs_x)

_6502_instruction_decl(ldy_immed)
_6502_instruction_decl(lda_zp_x_indirect)
_6502_instruction_decl(ldx_immed)
_6502_instruction_decl(lda_zp_s)
_6502_instruction_decl(ldy_zp)
_6502_instruction_decl(lda_zp)
_6502_instruction_decl(ldx_zp)
_6502_instruction_decl(lda_zp_indirect)
_6502_instruction_decl(tay)
_6502_instruction_decl(lda_immed)
_6502_instruction_decl(tax)
_6502_instruction_decl(plb)
_6502_instruction_decl(ldy_abs)
_6502_instruction_decl(lda_abs)
_6502_instruction_decl(ldx_abs)
_6502_instruction_decl(lda_abs)

_6502_instruction_decl(bcs)
_6502_instruction_decl(lda_zp_indirect_y)
_6502_instruction_decl(lda_zp_indirect)
_6502_instruction_decl(lda_zp_s_indirect_y)
_6502_instruction_decl(ldy_zp_x)
_6502_instruction_decl(lda_zp_x)
_6502_instruction_decl(ldx_zp_y)
_6502_instruction_decl(lda_zp_indirect_y)
_6502_instruction_decl(clv)
_6502_instruction_decl(lda_abs_y)
_6502_instruction_decl(tsx)
_6502_instruction_decl(tyx)
_6502_instruction_decl(ldy_abs_x)
_6502_instruction_decl(lda_abs_x)
_6502_instruction_decl(ldx_abs_y)
_6502_instruction_decl(lda_abs_x)

_6502_instruction_decl(cpy_immed)
_6502_instruction_decl(cmp_zp_x_indirect)
_6502_instruction_decl(rep_immed)
_6502_instruction_decl(cmp_zp_s)
_6502_instruction_decl(cpy_zp)
_6502_instruction_decl(cmp_zp)
_6502_instruction_decl(dec_zp)
_6502_instruction_decl(cmp_zp_indirect)
_6502_instruction_decl(iny)
_6502_instruction_decl(cmp_immed)
_6502_instruction_decl(dex)
_6502_instruction_decl(wal)
_6502_instruction_decl(cpy_abs)
_6502_instruction_decl(cmp_abs)
_6502_instruction_decl(dec_abs)
_6502_instruction_decl(cmp_abs)

_6502_instruction_decl(bne)
_6502_instruction_decl(cmp_zp_indirect_y)
_6502_instruction_decl(cmp_zp_indirect)
_6502_instruction_decl(cmp_zp_s_indirect_y)
_6502_instruction_decl(pei_zp)
_6502_instruction_decl(cmp_zp_x)
_6502_instruction_decl(dec_zp_x)
_6502_instruction_decl(cmp_zp_indirect_y)
_6502_instruction_decl(cld)
_6502_instruction_decl(cmp_abs_y)
_6502_instruction_decl(phx)
_6502_instruction_decl(stp)
_6502_instruction_decl(jml_abs_indirect)
_6502_instruction_decl(cmp_abs_x)
_6502_instruction_decl(dec_abs_x)
_6502_instruction_decl(cmp_abs_x)

_6502_instruction_decl(cpx_immed)
_6502_instruction_decl(sbc_zp_x_indirect)
_6502_instruction_decl(sep_immed)
_6502_instruction_decl(sbc_zp_s)
_6502_instruction_decl(cpx_zp)
_6502_instruction_decl(sbc_zp)
_6502_instruction_decl(inc_zp)
_6502_instruction_decl(sbc_zp_indirect)
_6502_instruction_decl(inx)
_6502_instruction_decl(sbc_immed)
_6502_instruction_decl(nop)
_6502_instruction_decl(xba)
_6502_instruction_decl(cpx_abs)
_6502_instruction_decl(sbc_abs)
_6502_instruction_decl(inc_abs)
_6502_instruction_decl(sbrk)

_6502_instruction_decl(beq)
_6502_instruction_decl(sbc_zp_indirect_y)
_6502_instruction_decl(sbc_zp_indirect)
_6502_instruction_decl(sbc_zp_s_indirect_y)
_6502_instruction_decl(pea_abs)
_6502_instruction_decl(sbc_zp_x)
_6502_instruction_decl(inc_zp_x)
_6502_instruction_decl(sbc_zp_indirect_y)
_6502_instruction_decl(sed)
_6502_instruction_decl(sbc_abs_y)
_6502_instruction_decl(plx)
_6502_instruction_decl(xce)
_6502_instruction_decl(jsr_abs_x_indirect)
_6502_instruction_decl(sbc_abs_x)
_6502_instruction_decl(inc_abs_x)
_6502_instruction_decl(bpt)


// Instructions array.  Ordered by opcode.
// @@@
static Instruction instructions[256] = {
  // 00 -> 0f
  {_6502_instruction(brk), 1, 7},
  {_6502_instruction(ora_zp_x_indirect), 2, 6},
  {_6502_instruction(cop_zp), 2, 3},
  {_6502_instruction(ora_zp), 2, 3},
  {_6502_instruction(tsb_zp), 2, 3},
  {_6502_instruction(ora_zp), 2, 3},
  {_6502_instruction(asl_zp), 2, 5},
  {_6502_instruction(ora_zp), 2, 3},
  {_6502_instruction(php), 1, 3},
  {_6502_instruction(ora_immed), 1, 2},
  {_6502_instruction(asl_accum), 1, 2},
  {_6502_instruction(phd), 1, 2},
  {_6502_instruction(tsb_zp), 2, 3},
  {_6502_instruction(ora_abs), 3, 4},
  {_6502_instruction(asl_abs), 3, 6},
  {_6502_instruction(ora_abl), 1, 4},
  
  // 10 -> if
  {_6502_instruction(bpl), 1, 3},
  {_6502_instruction(ora_zp_indirect_y), 2, 5},
  {_6502_instruction(ora_zp_indirect), 2, 2},
  {_6502_instruction(ora_zp_s_indirect_y), 2, 5},
  {_6502_instruction(trb_zp), 2, 2},
  {_6502_instruction(ora_abs_x), 3, 4},
  {_6502_instruction(asl_zp_x), 2, 6},
  {_6502_instruction(ora_zp_y), 2, 3},
  {_6502_instruction(clc), 1, 2},
  {_6502_instruction(ora_abs_y), 3, 4},
  {_6502_instruction(inc_a), 1, 2},
  {_6502_instruction(tcs), 1, 2},
  {_6502_instruction(trb_abs), 3, 4},
  {_6502_instruction(ora_abs_x), 3, 4},
  {_6502_instruction(asl_abs_x), 3, 7},
  {_6502_instruction(ora_abs_x), 3, 4},
  
  // 20 -> 2f
  {_6502_instruction(jsr_abs), 3, 6},
  {_6502_instruction(and_zp_x_indirect), 2, 6},
  {_6502_instruction(lsr_accum), 1, 2},
  {_6502_instruction(and_zp_s), 2, 2},
  {_6502_instruction(bit_zp), 2,34},
  {_6502_instruction(and_zp), 2, 2},
  {_6502_instruction(rol_zp), 2, 5},
  {_6502_instruction(and_zp), 2, 2},
  {_6502_instruction(plp), 1, 4},
  {_6502_instruction(and_immed), 1, 2},
  {_6502_instruction(rol_accum), 1, 2},
  {_6502_instruction(pld), 1, 2},
  {_6502_instruction(bit_abs), 3, 4},
  {_6502_instruction(and_abs), 3, 4},
  {_6502_instruction(rol_abs), 3, 6},
  {_6502_instruction(and_al), 1, 2},
  
  // 30 -> 3f
  {_6502_instruction(bmi), 1, 2},
  {_6502_instruction(and_zp_indirect_y), 2, 5},
  {_6502_instruction(and_zp_indirect), 2, 5},
  {_6502_instruction(and_zp_s_indirect_y), 2, 5},
  {_6502_instruction(bit_zp_x), 2, 4},
  {_6502_instruction(and_zp_x), 2, 4},
  {_6502_instruction(rol_zp_x), 2, 6},
  {_6502_instruction(and_zp_y), 2, 2},
  {_6502_instruction(sec), 1, 2},
  {_6502_instruction(and_abs_y), 3, 4},
  {_6502_instruction(dec_accum), 1, 2},
  {_6502_instruction(tsc), 1, 2},
  {_6502_instruction(bit_abs_x), 3, 4},
  {_6502_instruction(and_abs_x), 3, 4},
  {_6502_instruction(rol_abs_x), 3, 7},
  {_6502_instruction(and_abs_x), 3, 4},
  
  // 40 -> 4f
  {_6502_instruction(rti), 1, 6},
  {_6502_instruction(eor_zp_x_indirect), 2, 6},
  {_6502_instruction(wdm), 1, 2},
  {_6502_instruction(eor_zp_s), 2, 2},
  {_6502_instruction(mvp_s_zp), 2, 2},
  {_6502_instruction(eor_zp), 2, 3},
  {_6502_instruction(lsr_zp), 2, 5},
  {_6502_instruction(eor_zp), 2, 3},
  {_6502_instruction(pha), 1, 3},
  {_6502_instruction(eor_immed), 1, 2},
  {_6502_instruction(lsr_accum), 1, 2},
  {_6502_instruction(phk), 1, 2},
  {_6502_instruction(jmp_abs), 3, 2},
  {_6502_instruction(eor_abs), 3, 4},
  {_6502_instruction(lsr_abs), 3, 6},
  {_6502_instruction(eor_abs), 3, 4},
  
  // 50 -> 5f
  {_6502_instruction(bvc), 1, 2},
  {_6502_instruction(eor_zp_indirect_y), 2, 5},
  {_6502_instruction(eor_zp_indirect), 2, 5},
  {_6502_instruction(eor_zp_s_indirect_y), 2, 5},
  {_6502_instruction(mvn_s_zp), 2, 2},
  {_6502_instruction(eor_zp_x), 2, 4},
  {_6502_instruction(lsr_zp_x), 2, 6},
  {_6502_instruction(eor_zp_x), 2, 4},
  {_6502_instruction(cli), 1, 2},
  {_6502_instruction(eor_zp_y), 2, 4},
  {_6502_instruction(phy), 1, 2},
  {_6502_instruction(tcd), 1, 2},
  {_6502_instruction(jmp_abs), 3, 3},
  {_6502_instruction(eor_zp_x), 2, 4},
  {_6502_instruction(lsr_zp_x), 2, 6},
  {_6502_instruction(eor_zp_x), 2, 4},
  
  // 60 -> 6f
  {_6502_instruction(rts), 1, 6},
  {_6502_instruction(adc_zp_x_indirect), 2, 6},
  {_6502_instruction(per), 1, 2},
  {_6502_instruction(adc_zp_s), 2, 2},
  {_6502_instruction(stz_zp), 2, 2},
  {_6502_instruction(adc_zp), 2, 2},
  {_6502_instruction(ror_zp), 2, 5},
  {_6502_instruction(adc_zp_y), 2, 2},
  {_6502_instruction(pla), 1, 4},
  {_6502_instruction(adc_immed), 1, 2},
  {_6502_instruction(ror_accum), 1, 2},
  {_6502_instruction(rtl), 1, 2},
  {_6502_instruction(jmp_abs_indirect), 3, 5},
  {_6502_instruction(adc_abs), 3, 2},
  {_6502_instruction(ror_abs), 3, 6},
  {_6502_instruction(adc_abs), 3, 2},
  
  // 70 -> ff
  {_6502_instruction(bvs), 1, 2},
  {_6502_instruction(adc_zp_indirect_y), 2, 5},
  {_6502_instruction(adc_zp_indirect), 2, 2},
  {_6502_instruction(adc_zp_s_indirect_y), 2, 5},
  {_6502_instruction(stz_zp_x), 2, 4},
  {_6502_instruction(adc_zp_x), 2, 4},
  {_6502_instruction(ror_zp_x), 2, 6},
  {_6502_instruction(adc_zp_x), 2, 4},
  {_6502_instruction(sei), 1, 2},
  {_6502_instruction(adc_abs_y), 3, 2},
  {_6502_instruction(ply), 1, 2},
  {_6502_instruction(tdc), 1, 2},
  {_6502_instruction(jmp_abs_x_indirect), 3, 6},
  {_6502_instruction(adc_abs_x), 3, 2},
  {_6502_instruction(ror_abs_x), 3, 7},
  {_6502_instruction(adc_abs_x), 3, 2},
  
  // 80 -> 8f
  {_6502_instruction(bra), 1, 2},
  {_6502_instruction(sta_zp_x_indirect), 2, 6},
  {_6502_instruction(brl), 1, 2},
  {_6502_instruction(sta_zp_s), 2, 3},
  {_6502_instruction(sty_zp), 2, 3},
  {_6502_instruction(sta_zp), 2, 3},
  {_6502_instruction(stx_zp), 2, 3},
  {_6502_instruction(sta_zp), 2, 3},
  {_6502_instruction(dey), 1, 2},
  {_6502_instruction(bit_immed), 1, 2},
  {_6502_instruction(txa), 1, 2},
  {_6502_instruction(phb), 1, 2},
  {_6502_instruction(sty_abs), 3, 4},
  {_6502_instruction(sta_abs), 3, 4},
  {_6502_instruction(stx_abs), 3, 4},
  {_6502_instruction(sta_abs), 3, 4},
  
  // 90 -> 9f
  {_6502_instruction(bcc), 1, 2},
  {_6502_instruction(sta_zp_indirect_y), 2, 5},
  {_6502_instruction(sta_zp_indirect), 2, 2},
  {_6502_instruction(sta_zp_s_indirect_y), 2, 5},
  {_6502_instruction(sty_zp_x), 2, 4},
  {_6502_instruction(sta_zp_x), 2, 4},
  {_6502_instruction(stx_zp_y), 2, 4},
  {_6502_instruction(sta_zp_y), 2, 4},
  {_6502_instruction(tya), 1, 2},
  {_6502_instruction(sta_abs_y), 2, 4},
  {_6502_instruction(txs), 1, 2},
  {_6502_instruction(txy), 1, 2},
  {_6502_instruction(stz_abs), 3, 2},
  {_6502_instruction(sta_abs_x), 3, 5},
  {_6502_instruction(stz_abs_x), 3, 5},
  {_6502_instruction(sta_abs_x), 3, 5},
  
  // a0 -> af
  {_6502_instruction(ldy_immed), 1, 2},
  {_6502_instruction(lda_zp_x_indirect), 2, 6},
  {_6502_instruction(ldx_immed), 1, 2},
  {_6502_instruction(lda_zp_s), 2, 3},
  {_6502_instruction(ldy_zp), 2, 3},
  {_6502_instruction(lda_zp), 2, 3},
  {_6502_instruction(ldx_zp), 2, 3},
  {_6502_instruction(lda_zp_indirect), 2, 5},
  {_6502_instruction(tay), 1, 2},
  {_6502_instruction(lda_immed), 1, 2},
  {_6502_instruction(tax), 1, 2},
  {_6502_instruction(plb), 1, 2},
  {_6502_instruction(ldy_abs), 3, 2},
  {_6502_instruction(lda_abs), 3, 4},
  {_6502_instruction(ldx_abs), 3, 4},
  {_6502_instruction(lda_abs), 3, 4},
  
  // b0 -> bf
  {_6502_instruction(bcs), 1, 2},
  {_6502_instruction(lda_zp_indirect_y), 2, 5},
  {_6502_instruction(lda_zp_indirect), 2, 5},
  {_6502_instruction(lda_zp_s_indirect_y), 2, 5},
  {_6502_instruction(ldy_zp_x), 2, 4},
  {_6502_instruction(lda_zp_x), 2, 4},
  {_6502_instruction(ldx_zp_y), 2, 4},
  {_6502_instruction(lda_zp_indirect_y), 2, 5},
  {_6502_instruction(clv), 1, 2},
  {_6502_instruction(lda_abs_y), 3, 4},
  {_6502_instruction(tsx), 1, 2},
  {_6502_instruction(tyx), 1, 2},
  {_6502_instruction(ldy_abs_x), 3, 4},
  {_6502_instruction(lda_abs_x), 3, 4},
  {_6502_instruction(ldx_abs_y), 3, 4},
  {_6502_instruction(lda_abs_x), 3, 4},
  
  // c0 -> cf
  {_6502_instruction(cpy_immed), 1, 2},
  {_6502_instruction(cmp_zp_x_indirect), 2, 6},
  {_6502_instruction(rep_immed), 1, 2},
  {_6502_instruction(cmp_zp_s), 2, 2},
  {_6502_instruction(cpy_zp), 2, 3},
  {_6502_instruction(cmp_zp), 2, 3},
  {_6502_instruction(dec_zp), 2, 5},
  {_6502_instruction(cmp_zp_indirect), 2, 5},
  {_6502_instruction(iny), 1, 2},
  {_6502_instruction(cmp_immed), 1, 2},
  {_6502_instruction(dex), 1, 2},
  {_6502_instruction(wal), 1, 2},
  {_6502_instruction(cpy_abs), 3, 4},
  {_6502_instruction(cmp_abs), 3, 4},
  {_6502_instruction(dec_abs), 3, 6},
  {_6502_instruction(cmp_abs), 3, 4},
  
  // d0 -> df
  {_6502_instruction(bne), 1, 2},
  {_6502_instruction(cmp_zp_indirect_y), 2, 5},
  {_6502_instruction(cmp_zp_indirect), 2, 5},
  {_6502_instruction(cmp_zp_s_indirect_y), 2, 5},
  {_6502_instruction(pei_zp), 2, 2},
  {_6502_instruction(cmp_zp_x), 2, 4},
  {_6502_instruction(dec_zp_x), 2, 6},
  {_6502_instruction(cmp_zp_indirect_y), 2, 5},
  {_6502_instruction(cld), 1, 2},
  {_6502_instruction(cmp_abs_y), 3, 4},
  {_6502_instruction(phx), 1, 2},
  {_6502_instruction(stp), 1, 2},
  {_6502_instruction(jml_abs_indirect), 3, 2},
  {_6502_instruction(cmp_abs_x), 3, 4},
  {_6502_instruction(dec_abs_x), 3, 7},
  {_6502_instruction(cmp_abs_x), 3, 4},
  
  // e0 -> ef
  {_6502_instruction(cpx_immed), 1, 2},
  {_6502_instruction(sbc_zp_x_indirect), 2, 5},
  {_6502_instruction(sep_immed), 1, 2},
  {_6502_instruction(sbc_zp_s), 2, 3},
  {_6502_instruction(cpx_zp), 2, 3},
  {_6502_instruction(sbc_zp), 2, 3},
  {_6502_instruction(inc_zp), 2, 5},
  {_6502_instruction(sbc_zp_indirect), 2, 5},
  {_6502_instruction(inx), 1, 2},
  {_6502_instruction(sbc_immed), 1, 2},
  {_6502_instruction(nop), 1, 2},
  {_6502_instruction(xba), 1, 2},
  {_6502_instruction(cpx_abs), 3, 4},
  {_6502_instruction(sbc_abs), 3, 4},
  {_6502_instruction(inc_abs), 3, 6},
  {_6502_instruction(sbrk), 1, 2},
  
  // f0 -> ff
  {_6502_instruction(beq), 1, 2},
  {_6502_instruction(sbc_zp_indirect_y), 2, 5},
  {_6502_instruction(sbc_zp_indirect), 2, 5},
  {_6502_instruction(sbc_zp_s_indirect_y), 2, 5},
  {_6502_instruction(pea_abs), 3, 2},
  {_6502_instruction(sbc_zp_x), 2, 4},
  {_6502_instruction(inc_zp_x), 2, 6},
  {_6502_instruction(sbc_zp_indirect_y), 2, 5},
  {_6502_instruction(sed), 1, 2},
  {_6502_instruction(sbc_abs_y), 3, 4},
  {_6502_instruction(plx), 1, 2},
  {_6502_instruction(xce), 1, 2},
  {_6502_instruction(jsr_abs_x_indirect), 3, 2},
  {_6502_instruction(sbc_abs_x), 3, 4},
  {_6502_instruction(inc_abs_x), 3, 7},
  {_6502_instruction(bpt), 1, 2},
};

void _6502InterpreterInit(_6502Interpreter* interpreter, bool debug, bool cycle_accurate) {
  memset(interpreter, 0, sizeof(_6502Interpreter));
  interpreter->debug = debug;
  interpreter->current_bp = NULL;
  interpreter->next_bp_num = 1;
  interpreter->trace = false;
  interpreter->cycle_accurate = cycle_accurate;
  VectorInit(&interpreter->breakpoints);
  
  VectorInit(&interpreter->devices);
  VectorAppend(&interpreter->devices, ConsoleInit());
}

Device* PollForDevice(_6502Interpreter* interpreter, uint16_t addr) {
  for (size_t i = 0; i < interpreter->devices.length; i++) {
    Device* dev = interpreter->devices.value.p[i];
    if (dev->claim(dev, addr)) {
      return dev;
    }
  }
  return NULL;
}

// Reset handler, invoked on 6502 reset.
static char reset_handler[] = {
  // We have the entry address in 0,1. Jump indirect via 0x00.
  0x78,              // SEI
  0xa2, 0xff,        // LDX #0xff
  0x9a,              // TXS
  0x58,              // CLI
  0x6c, 0x00,        // JMP (0x00)
};

// 6502 instructions executed on IRQ or BRK
// Entered with interrupt flag set.  On stack we have
// s+1: P
// s+2: PC lo
// s+3: PC hi
// Use RTI to return.
// Uses 0xfd, 0xfe and 0xff as scratch.
// Jumps to BRK vector at 0xfd00
static char irq_handler[] = {
  0x85, 0xfd,         // STA $fd
  0x68,               // PLA
  0x48,               // PHA
  0x29, 0x10,         // AND #$10
  0xd0, 0x03,         // BNE $060b
  0xa5, 0xfd,         // LDA $fd
  0x40,               // RTI
  0x8a,               // TXA
  0x48,               // PHA
  0xba,               // TSX
  0xbd, 0x03, 0x01,   // LDA $0103,X
  0xd8,               // CLD
  0x38,               // SEC
  0xe9, 0x01,         // SBC #$01
  0x85, 0xfe,         // STA $fe
  0xbd, 0x04, 0x01,   // LDA $0104,X
  0xe9, 0x00,         // SBC #$00
  0x85, 0xff,         // STA $ff
  0xb2, 0xfe,         // LDA ($fe)
  0x20, 0x00, 0xfd,   // JSR 0xfd00
  0x68,               // PLA
  0xaa,               // TAX
  0xa5, 0xfd,         // LDA $fd
  0x40,               // RTI
};

#define REG_SP 0x98

#define _6502_SYS_EXIT 1
#define _6502_SYS_OPEN 2
#define _6502_SYS_CLOSE 3
#define _6502_SYS_WRITE 4
#define _6502_SYS_READ 5
#define _6502_SYS_LSEEK 7
#define _6502_SYS_ABORT 8

static void BrkHandler(_6502Interpreter* interpreter, int8_t code) {
  uint16_t _6502sp = *(uint16_t*)&interpreter->memory[REG_SP];     // SP as 6502 address.
  uint8_t* sp = (uint8_t*)(interpreter->memory + _6502sp);  // SP in native.
  
  switch (code) {
    case _6502_SYS_EXIT:
      exit(*((uint16_t*)(sp)));
      break;
    case _6502_SYS_ABORT: {
      printf("Abort\n");
      exit(1);
      break;
      }
    default:
      printf("Undefined sbrk\n");
      exit(1);
  }
}


void _6502DisassemblePc(_6502Interpreter* interpreter) {
  interpreter->current_symbol =
      LoaderFindSymbolAndCacheResult(interpreter->loader, interpreter->pc);
  Breakpoint* bp = FindBreakpoint(interpreter, interpreter->pc);
  if (bp != NULL) {
    BreakpointUninstall(interpreter, bp);
  }
  Disassemble6502Instruction(interpreter->current_symbol, interpreter->pc,
                            &interpreter->memory[interpreter->pc], stdout);
  if (bp != NULL) {
    BreakpointInstall(interpreter, bp);
  }
}

// 2Mhz = 500ns cycle time
#define CPU_CYCLE_NS 500

static inline uint64_t TimeNow() {
#ifdef __MACH__
  return mach_absolute_time();
#else
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec * 1000000000 + now.tv_nsec;
#endif
}

  
static void StepOneInstruction(_6502Interpreter* interpreter, bool cycle_accurate) {
  if (interpreter->trace) {
    _6502DisassemblePc(interpreter);
  }
  if (interpreter->stop_at_next_instruction) {
    DebuggerLoop(interpreter);
  }
  uint8_t opcode = interpreter->memory[interpreter->pc];
  Instruction* inst = &instructions[opcode];
  
  
  // Execute instruction and determine time.
  uint64_t start = TimeNow();
  (*inst->func)(interpreter);
  uint64_t end = TimeNow();

  // Calculate time to execute instruction.
  uint64_t diff = end - start;

  if (cycle_accurate && interpreter->cycle_accurate) {
    // Wait for instruction to complete timing.
    int wait_time_ns = (inst->cycles * CPU_CYCLE_NS) -
          (int)diff;
    if (wait_time_ns > 0) {
      const struct timespec t = {.tv_sec = 0, .tv_nsec = wait_time_ns};
      nanosleep(&t, NULL);
    }
  }
}

void _6502Reset(_6502Interpreter* interpreter) {
  // Write entry address into zero page 0,1
  interpreter->memory[0] = interpreter->entry_address & 0xff;
  interpreter->memory[1] = (interpreter->entry_address >> 8) & 0xff;

  // Start execution at the reset handler.
  interpreter->pc = interpreter->memory[0xfffc] | interpreter->memory[0xfffd] << 8;
  
  interpreter->stop_at_next_instruction = false;
}

#define RESET_HANDLER 0xff00
#define IRQ_HANDLER 0xff80

#define VECTOR_RAM 0xfd00

void _6502InterpreterRun(_6502Interpreter* interpreter, Loader* loader, uint64_t entry_address, int argc, char** argv) {
  interpreter->memory = calloc(65536, 1);    // 64K of memory.
  interpreter->s = 0xff;
  interpreter->zero_page = (uint8_t*)interpreter->memory;
  interpreter->stack = (uint8_t*)interpreter->memory + interpreter->s + 0x100;
  interpreter->loader = loader;
  
  // Set up reset and IRQ handlers.
  memcpy(&interpreter->memory[RESET_HANDLER], reset_handler, sizeof(reset_handler));
  memcpy(&interpreter->memory[IRQ_HANDLER], irq_handler, sizeof(irq_handler));

  // Reset Handler.
  interpreter->memory[0xfffc] = RESET_HANDLER & 0xff;
  interpreter->memory[0xfffd] = RESET_HANDLER >> 8;

  // Point IRQ vector to handler.
  interpreter->memory[0xfffe] = IRQ_HANDLER & 0xff;
  interpreter->memory[0xffff] = IRQ_HANDLER >> 8;


  // Copy the memory mapped in from the file into the interpreter's
  // memory.
  for (size_t i = 1; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    for (size_t section_index = 0; section_index < region->sections.length; section_index++) {
      ELFReaderSection* section = region->sections.value.p[section_index];
      void* section_addr = (char*)region->address + section->header->offset;
      memcpy(interpreter->memory+section->header->addr, section_addr, section->header->size);
    }
  }
  
  // A real 6502 loads the start address from 0xfffc:0xfffd and jumps there.
  // We will just set the PC to the entry address.  The stack pointer is
  // already initialized.
  interpreter->entry_address = (int)entry_address;
  
  // Write entry address into zero page 0,1
  interpreter->memory[0] = entry_address & 0xff;
  interpreter->memory[1] = (entry_address >> 8) & 0xff;

  // Start execution at the reset handler.
  interpreter->pc = interpreter->memory[0xfffc] | interpreter->memory[0xfffd] << 8;

  if (interpreter->debug) {
    interpreter->stop_at_next_instruction = true;
  }
  
  // Run processor by fetching instruction at PC and jumping to the handler
  // function for that opcode.  Each handler function will set the PC to
  // the next instruction address.
  for (;;) {
    StepOneInstruction(interpreter, true);
  }
}

void _6502InterpreterDisassemble(_6502Interpreter* interpreter, Loader* loader) {
  for (size_t i = 1; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    for (size_t section_index = 0; section_index < region->sections.length; section_index++) {
      ELFReaderSection* section = region->sections.value.p[section_index];
      if ((section->header->flags & SHF(execinstr)) != 0) {
        void* addr = (void*)((char*)region->address + section->header->offset);
        void* end_addr = addr + section->header->size;
        uint16_t pc = section->header->addr;
        SymbolScope* symbol = NULL;
        while (addr <= end_addr) {
          symbol =
              LoaderFindSymbolAndCacheResult(interpreter->loader, pc);
          void* new_addr = Disassemble6502Instruction(symbol, pc, addr, stdout);
          pc += new_addr - addr;
          addr = new_addr;
        }
      }
    }
  }
  
}

// Extract the contents of the EXE file into a binary file containing
// only the sections with data.  This will be .text and .data sections
// containing code and static data.  This can be burned into a ROM.
void _6502InterpreterExtract(_6502Interpreter* interpreter, Loader* loader, FILE* fp) {
  char* memory = calloc(65536, 1);
  char* start_memory = memory + 65536;
  char* end_memory = memory;
  
  // Copy all section contents to the temporary memory at the correct address.
  for (size_t i = 1; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    for (size_t section_index = 0; section_index < region->sections.length; section_index++) {
      ELFReaderSection* section = region->sections.value.p[section_index];
      if (section->header->size == 0) {
        continue;
      }
      void* section_addr = (char*)region->address + section->header->offset;
      memcpy(memory+section->header->addr, section_addr, section->header->size);
      
      // Calculate limits of data.
      char* start_addr = memory+section->header->addr;
      if (start_addr < start_memory) {
        start_memory = start_addr;
      }
      char* end_addr = memory+section->header->addr + section->header->size;
      if (end_addr > end_memory) {
        end_memory = end_addr;
      }
    }
  }
  // Write the memory to the file.
  fwrite(start_memory, end_memory - start_memory, 1, fp);
  free(memory);
}

void _6502InterpreterDestruct(_6502Interpreter* interpreter) {
  free(interpreter->memory);
  VectorDestruct(&interpreter->breakpoints);
}

// Stack is empty/descending.  SP starts at 0xff and always points to the next
// location to use.
// Push PC plus an increment, onto stack.  Pushed high byte first.
#define PUSH_PC(inc) {\
  *(interpreter->memory + 0x100 + interpreter->s--) = (interpreter->pc + inc) >> 8;\
  *(interpreter->memory + 0x100 + interpreter->s--) = (interpreter->pc + inc)& 0xff; \
  }

// Pop PC from the stack
#define POP_PC(inc) {\
  uint16_t v = *(interpreter->memory + 0x100 + ++interpreter->s); \
  v |= *(interpreter->memory + 0x100 + ++interpreter->s) << 8; \
  interpreter->pc = v + inc; \
}

// Puah an 8-bit value onto the stack.
#define PUSH(value) *(uint8_t*)(interpreter->memory + 0x100 + interpreter->s--) = value;

// Pop stack into register.
#define POP(reg) interpreter->reg = interpreter->memory[0x100 + ++interpreter->s]

// Jump to an address.
#define JMP(addr) interpreter->pc = addr

// 8-git operand in instruction.
#define OP8() interpreter->memory[interpreter->pc+1]

// 16-bit operand in instruction.
#define OP16() *(uint16_t*)(interpreter->memory + interpreter->pc+1)

#define SET_ZERO(_v) interpreter->flags.bits.z = (_v) == 0
#define SET_SIGN(_v) interpreter->flags.bits.s = ((_v) & 0x80) != 0
#define SET_OVERFLOW(_v) interpreter->flags.bits.v = (_v) != 0

// Set the z and s flags.
#define SET_FLAGS(value) {\
  uint8_t _v = value;\
  SET_ZERO(_v);\
  SET_SIGN(_v);\
}

// Set overflow flag based on accumulator value, the src value being
// used and the result of a calculation.
#define SET_OVERFLOW_A(src, result) \
SET_OVERFLOW(((~(interpreter->a ^ src)) & (interpreter->a ^ result)) & 0x80);
//  SET_OVERFLOW(!(((interpreter->a ^ src) & 0x80) &\
//    ((interpreter->a ^ result) & 0x80)));

// Set the carry flag from a bit in a value.
#define SET_CARRY_BIT(value, bit) interpreter->flags.bits.c = (value & (1 << bit)) != 0

// Set the carry to a value.
#define SET_CARRY_VALUE(value) interpreter->flags.bits.c = (value) != 0

// Set the borrow flag (inverse of carry).
#define SET_BORROW_BIT(value, bit) interpreter->flags.bits.c = (value & (1 << bit)) == 0

#define SET_BORROW_VALUE(value) interpreter->flags.bits.c = (value) == 0

// 8-bit zero page value from an address.
#define ZP(addr) interpreter->zero_page[addr]

// 16-bit zero page address from an address.
#define ZP_ADDR(addr) &interpreter->zero_page[addr]

// 8-bit value from memory at address.
#define MEM(addr) interpreter->memory[addr]

// 16-bit address from memory at address.
#define MEM_ADDR(addr) (uint8_t*)(interpreter->memory + addr)

// Increment the PC by the given number of bytes.
#define INC_PC(inc) interpreter->pc += inc

// A 16-bit address formed by adding an absolute address to an index register.
#define INDEXED(addr, reg) (interpreter->memory + addr + interpreter->reg)

// An 8-bit value read from memory at the given address
#define INDIRECT(addr) *(uint8_t*)(interpreter->memory + addr)

// 16-bit value at address.  Used to set PC.
#define INDIRECT2(addr) *(uint16_t*)(interpreter->memory + addr)

// A 16-bit address read from memory at the given address
#define INDIRECT_ADDR(addr) (uint8_t*)(interpreter->memory + *(uint16_t*)(interpreter->memory + addr))

// An address  read from the address formed by adding an index register to a
// zero page address, then taking the contents of that 16-bit address.
#define INDEXED_INDIRECT(addr, reg) (interpreter->memory + ((*(uint16_t*)(interpreter->memory + addr + interpreter->reg))))

// An address read from the address formed by taking the contents of 2 bytes
// in zero page and then adding an index register.
#define INDIRECT_INDEXED(addr, reg) (interpreter->memory + (*(uint16_t*)(interpreter->memory + addr) + interpreter->reg))

// 16-bit address formed from the address formed by adding an index register to a
// zero page address, then taking the contents of that 16-bit address.
#define INDEXED_INDIRECT_ADDR(addr, reg) (interpreter->memory + (*(uint16_t*)(interpreter->memory + addr + interpreter->reg)))

// A 16-bit address formed from the address formed by taking the contents of 2 bytes
// in zero page and then adding an index register.
#define INDIRECT_INDEXED_ADDR(addr, reg) (interpreter->memory + *((uint16_t*)(interpreter->memory + addr) + interpreter->reg))

// Load an 8-bit immediate into a register.
#define SET_IMMED(reg, value) interpreter->reg = value

// Load a register with a value.
#define LOAD(reg, addr) { \
  if ((uint16_t)addr < _6502_IO_START || (uint16_t)addr > _6502_IO_END) {\
    SET_FLAGS(interpreter->reg = *(addr));\
  } else {\
    Device* dev = PollForDevice(interpreter, (uint16_t)addr); \
    if (dev != NULL) { \
      SET_FLAGS(interpreter->reg = dev->read(dev, (uint16_t)addr)); \
    } else { \
      SET_FLAGS(interpreter->reg = *(addr));\
    }\
  }\
}

#define LOAD_ZP(reg, addr) SET_FLAGS(interpreter->reg = *(addr));

// Store a register into an address.
#define STORE(reg, addr) { \
  if ((uint16_t)addr < _6502_IO_START || (uint16_t)addr > _6502_IO_END) {\
    *(addr) = interpreter->reg;\
  } else {\
    Device* dev = PollForDevice(interpreter, (uint16_t)addr);\
    if (dev != NULL) {\
      dev->write(dev, (uint16_t)addr, interpreter->reg); \
    } else { \
      *(addr) = interpreter->reg;\
    }\
  }\
}

#define STORE_ZP(reg, addr) *(addr) = interpreter->reg;

#define STORE_ZERO(addr) *(addr) = 0

#define ADD_WITH_CARRY(value) {\
  int tmp = value;\
  int16_t v = interpreter->a + tmp + interpreter->flags.bits.c; \
  SET_CARRY_VALUE(v > 255);\
  SET_FLAGS(interpreter->a = v); \
  SET_OVERFLOW_A(tmp, v);\
}

#define SUB_WITH_BORROW(value) {\
  int tmp = value;\
  int borrow = interpreter->flags.bits.c ^ 1;\
  int16_t v = interpreter->a - tmp - borrow; \
  SET_BORROW_VALUE(v > 255);\
  SET_FLAGS(interpreter->a = v); \
  SET_OVERFLOW_A(tmp, v);\
}

#define ALU_OP_INDEXED_INDIRECT_ZP(index, op) { \
  uint16_t addr = OP8();\
  uint16_t value = *INDEXED_INDIRECT(addr, index);\
  SET_FLAGS(interpreter->a op##= value); \
  INC_PC(2); \
}

#define ALU_OP_INDIRECT_INDEXED_ZP(index, op) { \
  uint16_t addr = OP8();\
  uint16_t value = *INDIRECT_INDEXED(addr, index);\
  SET_FLAGS(interpreter->a op##= value); \
  INC_PC(2); \
}

#define ALU_IMMED(op) {\
  SET_FLAGS(interpreter->a op##= OP8());\
  INC_PC(2);\
}

#define ALU_OP_ZP(op) {\
  SET_FLAGS(interpreter->a op##= interpreter->zero_page[OP8()]);\
  INC_PC(2);\
}

#define ALU_OP_ABS(op) {\
  SET_FLAGS(interpreter->a op##= interpreter->memory[OP16()]);\
  INC_PC(3);\
}

#define ALU_OP_ZP_INDIRECT(op) {\
  uint16_t addr = OP8();\
  uint8_t value = INDIRECT(addr);\
  SET_FLAGS(interpreter->a op##= value);\
  INC_PC(2);\
}

#define ALU_OP_ZP_INDEXED(index, op) {\
  uint16_t addr = OP8();\
  uint8_t value = INDIRECT(addr + interpreter->index);\
  SET_FLAGS(interpreter->a op##= value);\
  INC_PC(2);\
}

#define ALU_OP_ABS_INDEXED(index, op) {\
  uint16_t addr = OP16();\
  uint8_t value = INDIRECT(addr + interpreter->index);\
  SET_FLAGS(interpreter->a op##= value);\
  INC_PC(3);\
}

#define BRANCH(flag, value) {\
  if (interpreter->flags.bits.flag == value) {\
    int8_t offset = OP8();\
    interpreter->pc += offset + 2;\
    return;\
  }\
  INC_PC(2);\
}

#define UNIMPLEMENTED(inst) fprintf(stderr, "Unimplemented instruction %s\n", #inst)

// Instruction definitions.


// *************************************************************************
// Miscellaneous
// *************************************************************************

_6502_instruction_def(brk) {
  PUSH_PC(2);
  PUSH(interpreter->flags.value | (3 << 4));    // Push P with bits 4 and 5 fiag set.
  uint16_t vector = interpreter->memory[0xfffe] + (interpreter->memory[0xffff] << 8);
  JMP(vector);
}

_6502_instruction_def(nop) {
  INC_PC(1);
}

// *************************************************************************
// ORA
// *************************************************************************
_6502_instruction_def(ora_zp_x_indirect) {
  ALU_OP_INDEXED_INDIRECT_ZP(x, |);
}

_6502_instruction_def(ora_zp) {
  ALU_OP_ZP(|);
}

_6502_instruction_def(ora_immed) {
  ALU_IMMED(|);
}


_6502_instruction_def(ora_abs) {
  ALU_OP_ABS(|);
}

_6502_instruction_def(ora_abl) {
  UNIMPLEMENTED(ora_abl);
}

_6502_instruction_def(ora_zp_indirect_y) {
  ALU_OP_INDIRECT_INDEXED_ZP(y, |);
}

_6502_instruction_def(ora_zp_indirect) {
  ALU_OP_ZP_INDIRECT(|);
}

_6502_instruction_def(ora_zp_s_indirect_y) {
  UNIMPLEMENTED(ora_zp_s_indirect_y);
}

_6502_instruction_def(ora_abs_x) {
  ALU_OP_ABS_INDEXED(x, |);
}

_6502_instruction_def(ora_zp_y) {
  ALU_OP_ZP_INDEXED(y, |);
}

_6502_instruction_def(ora_abs_y) {
  ALU_OP_ABS_INDEXED(y, |);
}


// *************************************************************************
// AND
// *************************************************************************

_6502_instruction_def(and_zp_x_indirect) {
  ALU_OP_INDEXED_INDIRECT_ZP(x, &);
}

_6502_instruction_def(and_zp_s) {
  UNIMPLEMENTED(and_zp_s);
}

_6502_instruction_def(and_zp) {
  ALU_OP_ZP(&);
}

_6502_instruction_def(and_immed) {
  ALU_IMMED(&);
}

_6502_instruction_def(and_abs) {
  ALU_OP_ABS(&);
}

_6502_instruction_def(and_al) {
  UNIMPLEMENTED(and_al);
}

_6502_instruction_def(and_zp_indirect_y) {
  ALU_OP_INDEXED_INDIRECT_ZP(x, &);
  
}
_6502_instruction_def(and_zp_indirect) {
  ALU_OP_ZP_INDIRECT(&);
  
}
_6502_instruction_def(and_zp_s_indirect_y) {
  ALU_OP_INDIRECT_INDEXED_ZP(y, &);
}

_6502_instruction_def(and_zp_x) {
  ALU_OP_ZP_INDEXED(x, &);
}

_6502_instruction_def(and_zp_y) {
  ALU_OP_ZP_INDEXED(y, &);
}

_6502_instruction_def(and_abs_y) {
  ALU_OP_ABS_INDEXED(y, &);
}

_6502_instruction_def(and_abs_x) {
  ALU_OP_ABS_INDEXED(x, &);
}

// *************************************************************************
// ASL
// *************************************************************************

_6502_instruction_def(asl_zp) {
  uint16_t addr = OP8();
  SET_CARRY_BIT(addr, 7);
  SET_FLAGS(ZP(addr) <<= 1);
  INC_PC(2);
}

_6502_instruction_def(asl_accum) {
  SET_CARRY_BIT(interpreter->a, 7);
  SET_FLAGS(interpreter->a <<= 1);
  INC_PC(1);
}

_6502_instruction_def(asl_abs_x) {
  uint16_t addr = OP16() + interpreter->x;
  SET_CARRY_BIT(MEM(addr), 7);
  SET_FLAGS(MEM(addr) <<= 1);
  INC_PC(3);
}

_6502_instruction_def(asl_zp_x) {
  uint16_t addr = OP8() + interpreter->x;
  SET_CARRY_BIT(MEM(addr), 7);
  SET_FLAGS(MEM(addr) <<= 1);
  INC_PC(2);
}

_6502_instruction_def(asl_abs) {
  uint16_t addr = OP16();
  SET_CARRY_BIT(MEM(addr), 7);
  SET_FLAGS(MEM(addr) <<= 1);
  INC_PC(3);
}


// *************************************************************************
// LSR
// *************************************************************************

_6502_instruction_def(lsr_accum) {
  SET_CARRY_BIT(interpreter->a, 0);
  SET_FLAGS(interpreter->a >>= 1);
  INC_PC(1);
}

_6502_instruction_def(lsr_zp) {
  uint16_t addr = OP8();
  SET_CARRY_BIT(addr, 0);
  SET_FLAGS(ZP(addr) >>= 1);
  INC_PC(2);

}

_6502_instruction_def(lsr_abs) {
  uint16_t addr = OP16();
  SET_CARRY_BIT(addr, 0);
  SET_FLAGS(ZP(addr) >>= 1);
  INC_PC(3);
}

_6502_instruction_def(lsr_zp_x) {
  uint16_t addr = OP8();
  SET_CARRY_BIT(addr, 0);
  SET_FLAGS(*INDEXED(addr,x) >>= 1);
  INC_PC(2);
}

// *************************************************************************
// ROL
// *************************************************************************

_6502_instruction_def(rol_zp) {
  uint16_t addr = OP8();
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(addr, 7);
  int v = ZP(addr) << 1 | carry;
  SET_FLAGS(ZP(addr) = v);
  INC_PC(2);
}

_6502_instruction_def(rol_accum) {
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(interpreter->a, 7);
  int v = interpreter->a << 1 | carry;
  SET_FLAGS(interpreter->a = v);
  INC_PC(1);
}

_6502_instruction_def(rol_abs) {
  uint16_t addr = OP16();
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(addr, 7);
  int v = MEM(addr) << 1 | carry;
  SET_FLAGS(MEM(addr) = v);
  INC_PC(3);
}

_6502_instruction_def(rol_zp_x) {
  uint16_t addr = OP8();
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(addr, 7);
  uint8_t* p = INDEXED(addr,x);
  int v = *p << 1 | carry;
  SET_FLAGS(*p = v);
  INC_PC(2);
}

_6502_instruction_def(rol_abs_x) {
  uint16_t addr = OP16();
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(addr, 7);
  uint8_t* p = INDEXED(addr,x);
  int v = *p << 1 | carry;
  SET_FLAGS(*p = v);
  INC_PC(3);
}

// *************************************************************************
// ROR
// *************************************************************************

_6502_instruction_def(ror_zp) {
  uint16_t addr = OP8();
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(addr, 0);
  int v = ZP(addr) >> 1 | carry << 7;
  SET_FLAGS(ZP(addr) = v);
  INC_PC(2);
}

_6502_instruction_def(ror_accum) {
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(interpreter->a, 0);
  int v = interpreter->a >> 1 | carry << 7;
  SET_FLAGS(interpreter->a = v);
  INC_PC(1);
}

_6502_instruction_def(ror_abs) {
  uint16_t addr = OP16();
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(addr, 0);
  int v = MEM(addr) >> 1 | carry << 7;
  SET_FLAGS(MEM(addr) = v);
  INC_PC(3);
}

_6502_instruction_def(ror_zp_x) {
  uint16_t addr = OP8();
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(addr, 0);
  uint8_t* p = INDEXED(addr, x);
  int v = *p >> 1 | carry << 7;
  SET_FLAGS(*p = v);
  INC_PC(2);
}

_6502_instruction_def(ror_abs_x) {
  uint16_t addr = OP16();
  int carry = interpreter->flags.bits.c;
  SET_CARRY_BIT(addr, 0);
  uint8_t* p = INDEXED(addr, x);
  int v = *p >> 1 | carry << 7;
  SET_FLAGS(*p = v);
  INC_PC(3);
}


// *************************************************************************
// EOR
// *************************************************************************

_6502_instruction_def(eor_zp_x_indirect) {
  ALU_OP_INDEXED_INDIRECT_ZP(x, ^);
}

_6502_instruction_def(eor_zp) {
  ALU_OP_ZP(^);
}

_6502_instruction_def(eor_zp_s) {
  UNIMPLEMENTED(eor_zp_s);
}

_6502_instruction_def(eor_immed) {
  ALU_IMMED(^);
}

_6502_instruction_def(eor_abs) {
  ALU_OP_ABS(^);
}

_6502_instruction_def(eor_zp_indirect_y) {
  ALU_OP_INDIRECT_INDEXED_ZP(y, ^);
}
_6502_instruction_def(eor_zp_indirect) {
  ALU_OP_ZP_INDIRECT(^);
}
_6502_instruction_def(eor_zp_s_indirect_y) {
  UNIMPLEMENTED(eor_zp_s_indirect_y);
}


_6502_instruction_def(eor_zp_x) {
  ALU_OP_ZP_INDEXED(x, ^);
}

_6502_instruction_def(eor_zp_y) {
  ALU_OP_ZP_INDEXED(y, ^);
}


// *************************************************************************
// LDA
// *************************************************************************

_6502_instruction_def(lda_zp_x_indirect) {
  LOAD(a, INDEXED_INDIRECT(OP8(), x));
  INC_PC(2);
}

_6502_instruction_def(lda_zp_s) {
  UNIMPLEMENTED(lda_zp_s);
}

_6502_instruction_def(lda_zp) {
  LOAD_ZP(a, ZP_ADDR(OP8()));
  INC_PC(2);
}

_6502_instruction_def(lda_zp_indirect) {
  uint16_t addr = OP8();
  LOAD(a, INDIRECT_ADDR(addr));
  INC_PC(2);
}

_6502_instruction_def(lda_immed) {
  uint8_t v = OP8();
  SET_FLAGS(interpreter->a = v);
  INC_PC(2);
}

_6502_instruction_def(lda_abs) {
  LOAD(a, MEM_ADDR(OP16()));
  INC_PC(3);
}


_6502_instruction_def(lda_zp_indirect_y) {
  LOAD(a, INDIRECT_INDEXED(OP8(), y));
  INC_PC(2);
}

_6502_instruction_def(lda_zp_s_indirect_y) {
  UNIMPLEMENTED(lda_zp_s_indirect_y);
}

_6502_instruction_def(lda_zp_x) {
  LOAD_ZP(a, INDEXED(OP8(), x));
  INC_PC(2);
}

_6502_instruction_def(lda_abs_y) {
  LOAD(a, INDEXED(OP16(), y));
  INC_PC(3);
}

_6502_instruction_def(lda_abs_x) {
  LOAD(a, INDEXED(OP16(), x));
  INC_PC(3);
}

// *************************************************************************
// LDX
// *************************************************************************
_6502_instruction_def(ldx_zp) {
  LOAD_ZP(x, ZP_ADDR(OP8()));
  INC_PC(2);
}


_6502_instruction_def(ldx_abs) {
  LOAD(x, MEM_ADDR(OP16()));
  INC_PC(3);
}

_6502_instruction_def(ldx_zp_y) {
  LOAD_ZP(x, INDEXED(OP8(), y));
  INC_PC(2);
}

_6502_instruction_def(ldx_abs_y) {
  LOAD(x, INDEXED(OP16(), y));
  INC_PC(3);
}

_6502_instruction_def(ldx_immed) {
  uint8_t v = OP8();
  SET_FLAGS(interpreter->x = v);
  INC_PC(2);
}


// *************************************************************************
// LDY
// *************************************************************************

_6502_instruction_def(ldy_zp) {
  LOAD_ZP(y, ZP_ADDR(OP8()));
  INC_PC(2);
}

_6502_instruction_def(ldy_zp_x) {
  LOAD_ZP(y, INDEXED(OP8(), x));
  INC_PC(2);
}

_6502_instruction_def(ldy_immed) {
  uint8_t v = OP8();
  SET_FLAGS(interpreter->y = v);
  INC_PC(2);
}

_6502_instruction_def(ldy_abs) {
  LOAD(y, MEM_ADDR(OP16()));
  INC_PC(3);
}

_6502_instruction_def(ldy_abs_x) {
  LOAD(y, INDEXED(OP16(), x));
  INC_PC(3);
}

// *************************************************************************
// ADC
// *************************************************************************

_6502_instruction_def(adc_zp_x_indirect) {
  ADD_WITH_CARRY(*INDEXED_INDIRECT(OP8(),x));
  INC_PC(2);
}

_6502_instruction_def(adc_zp_s) {
  UNIMPLEMENTED(adc_zp_s);
}

_6502_instruction_def(adc_zp) {
  ADD_WITH_CARRY(ZP(OP8()));
  INC_PC(2);
}

_6502_instruction_def(adc_zp_y) {
  ADD_WITH_CARRY(*INDEXED(OP8(), y));
  INC_PC(2);
}

_6502_instruction_def(adc_immed) {
  ADD_WITH_CARRY(OP8());
  INC_PC(2);
}

_6502_instruction_def(adc_abs) {
  ADD_WITH_CARRY(MEM(OP16()));
  INC_PC(3);
}

_6502_instruction_def(adc_zp_indirect_y) {
  ADD_WITH_CARRY(*INDIRECT_INDEXED(OP8(), y));
  INC_PC(2);
}

_6502_instruction_def(adc_zp_indirect) {
  ADD_WITH_CARRY(*INDIRECT_ADDR(OP8()));
  INC_PC(2);
}

_6502_instruction_def(adc_zp_s_indirect_y) {
  UNIMPLEMENTED(adc_zp_s_indirect_y);
}

_6502_instruction_def(adc_zp_x) {
  ADD_WITH_CARRY(*INDEXED(OP8(), x));
  INC_PC(2);
}

_6502_instruction_def(adc_abs_y) {
  ADD_WITH_CARRY(*INDEXED(OP16(), y));
  INC_PC(3);
}

_6502_instruction_def(adc_abs_x) {
  ADD_WITH_CARRY(*INDEXED(OP16(), x));
  INC_PC(3);
}


// *************************************************************************
//  SBC
// *************************************************************************

_6502_instruction_def(sbc_zp_x_indirect) {
  SUB_WITH_BORROW(*INDEXED_INDIRECT(OP8(),x));
  INC_PC(2);
}

_6502_instruction_def(sbc_zp_s) {
  UNIMPLEMENTED(sbc_zp_s);
}

_6502_instruction_def(sbc_zp) {
  SUB_WITH_BORROW(ZP(OP8()));
  INC_PC(2);
}

_6502_instruction_def(sbc_zp_indirect) {
  SUB_WITH_BORROW(*INDIRECT_ADDR(OP8()));
  INC_PC(2);
}

_6502_instruction_def(sbc_immed) {
  SUB_WITH_BORROW(OP8());
  INC_PC(2);
}

_6502_instruction_def(sbc_abs) {
  SUB_WITH_BORROW(MEM(OP16()));
  INC_PC(3);
}

_6502_instruction_def(sbc_zp_indirect_y) {
  SUB_WITH_BORROW(*INDIRECT_INDEXED(OP8(), y));
  INC_PC(2);
}

_6502_instruction_def(sbc_zp_s_indirect_y) {
  UNIMPLEMENTED(sbc_zp_s_indirect_y);
}

_6502_instruction_def(sbc_zp_x) {
  SUB_WITH_BORROW(*INDEXED(OP8(), x));
  INC_PC(2);
}


_6502_instruction_def(sbc_abs_y) {
  SUB_WITH_BORROW(*INDEXED(OP16(), y));
  INC_PC(3);
}

_6502_instruction_def(sbc_abs_x) {
  SUB_WITH_BORROW(*INDEXED(OP16(), x));
  INC_PC(3);
}

// *************************************************************************
// INC
// *************************************************************************

_6502_instruction_def(inc_a) {
  SET_FLAGS(++interpreter->a);
  INC_PC(1);
}

_6502_instruction_def(inc_zp) {
  uint16_t addr = OP8();
  SET_FLAGS(++(*ZP_ADDR(addr)));
  INC_PC(2);
}

_6502_instruction_def(inc_abs) {
  uint16_t addr = OP16();
  SET_FLAGS(++(*MEM_ADDR(addr)));
  INC_PC(3);
}

_6502_instruction_def(inc_zp_x) {
  uint8_t* p = INDEXED(OP8(),x);
  SET_FLAGS(++(*p));
  INC_PC(2);
}

_6502_instruction_def(inc_abs_x) {
  uint8_t* p = INDEXED(OP16(),x);
  SET_FLAGS(++(*p));
  INC_PC(3);
}

_6502_instruction_def(inx) {
  SET_FLAGS(++interpreter->x);
  INC_PC(1);
}

_6502_instruction_def(iny) {
  SET_FLAGS(++interpreter->y);
  INC_PC(1);
}


// *************************************************************************
// DEC
// *************************************************************************

_6502_instruction_def(dec_accum) {
  SET_FLAGS(--interpreter->a);
  INC_PC(1);
}

_6502_instruction_def(dec_zp) {
  uint8_t* p = ZP_ADDR(OP8());
  SET_FLAGS(--(*p));
  INC_PC(2);
}

_6502_instruction_def(dec_abs) {
  uint8_t* p = (uint8_t*)MEM_ADDR(OP16());
  SET_FLAGS(--(*p));
  INC_PC(3);
}

_6502_instruction_def(dec_zp_x) {
  uint8_t* p = INDEXED(OP8(),x);
  SET_FLAGS(--(*p));
  INC_PC(2);
}

_6502_instruction_def(dec_abs_x) {
  uint8_t* p = INDEXED(OP16(),x);
  SET_FLAGS(--(*p));
  INC_PC(3);
}

_6502_instruction_def(dex) {
  SET_FLAGS(--interpreter->x);
  INC_PC(1);
}

_6502_instruction_def(dey) {
  SET_FLAGS(--interpreter->y);
  INC_PC(1);
}

// *************************************************************************
// BIT
// *************************************************************************

_6502_instruction_def(bit_zp) {
  uint16_t addr = OP8();
  int result = interpreter->a & ZP(addr);
  SET_SIGN(ZP(addr));
  SET_ZERO(result);
  SET_OVERFLOW(ZP(addr) & 0x40);
  INC_PC(2);
}

_6502_instruction_def(bit_abs) {
  uint16_t addr = OP16();
  int result = interpreter->a & MEM(addr);
  SET_SIGN(MEM(addr));
  SET_ZERO(result);
  SET_OVERFLOW(MEM(addr) & 0x40);
  INC_PC(3);
}

_6502_instruction_def(bit_zp_x) {
  uint16_t addr = OP8();
  uint8_t* ptr = INDEXED(addr, x);
  int result = interpreter->a & *ptr;
  SET_SIGN(*ptr);
  SET_ZERO(result);
  SET_OVERFLOW(*ptr & 0x40);
  INC_PC(2);
}


_6502_instruction_def(bit_abs_x) {
  uint16_t addr = OP16();
  uint8_t* ptr = INDEXED(addr, x);
  int result = interpreter->a & *ptr;
  SET_SIGN(*ptr);
  SET_ZERO(result);
  SET_OVERFLOW(*ptr & 0x40);
  INC_PC(3);
}

_6502_instruction_def(bit_immed) {
  uint16_t value = OP8();
  int result = interpreter->a & value;
  SET_SIGN(value);
  SET_ZERO(result);
  SET_OVERFLOW(value & 0x40);
  INC_PC(2);
}


// *************************************************************************
// CMP
// *************************************************************************

_6502_instruction_def(cmp_zp_x_indirect) {
  uint16_t addr = OP8();
  int8_t v = *INDEXED_INDIRECT(addr,x);
  int result = interpreter->a - v;
  SET_FLAGS(result);
  SET_BORROW_VALUE(result > 255);
  INC_PC(2);
}

_6502_instruction_def(cmp_zp_s) {
  
}

_6502_instruction_def(cmp_zp) {
  uint16_t addr = OP8();
  int8_t v = ZP(addr);
  int result = interpreter->a - v;
  SET_FLAGS(result);
  SET_BORROW_VALUE(result > 255);
  INC_PC(2);
}

_6502_instruction_def(cmp_zp_indirect) {
  uint16_t addr = OP8();
  int8_t v = *INDIRECT_ADDR(addr);
  int result = interpreter->a - v;
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(2);
}

_6502_instruction_def(cmp_immed) {
  uint16_t value = OP8();
  int result = interpreter->a - value;
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(2);
}

_6502_instruction_def(cmp_abs) {
  uint16_t addr = OP16();
  int8_t v = MEM(addr);
  int result = interpreter->a - v;
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(3);
}

_6502_instruction_def(cmp_zp_indirect_y) {
  uint16_t addr = OP8();
  int8_t v = *INDIRECT_INDEXED(addr,y);
  int result = interpreter->a - v;
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(2);
}

_6502_instruction_def(cmp_zp_s_indirect_y) {
  
}

_6502_instruction_def(cmp_zp_x) {
  uint16_t addr = OP8();
  uint8_t* ptr = INDEXED(addr, x);
  int result = interpreter->a - *ptr;
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(2);
}

_6502_instruction_def(cmp_abs_y) {
  uint16_t addr = OP16();
  uint8_t* ptr = INDEXED(addr, y);
  int result = interpreter->a - *ptr;
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(3);
}

_6502_instruction_def(cmp_abs_x) {
  uint16_t addr = OP16();
  uint8_t* ptr = INDEXED(addr, x);
  int result = interpreter->a - *ptr;
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(3);
}



// *************************************************************************
// CPX
// *************************************************************************

_6502_instruction_def(cpx_immed) {
  uint16_t value = OP8();
  int result = interpreter->x - value;
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(2);
}

_6502_instruction_def(cpx_zp) {
  uint16_t addr = OP8();
  int result = interpreter->x - ZP(addr);
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(2);
}

_6502_instruction_def(cpx_abs) {
  uint16_t addr = OP16();
  int result = interpreter->x - MEM(addr);
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(3);
}



// *************************************************************************
// CPY
// *************************************************************************

_6502_instruction_def(cpy_immed) {
  uint16_t value = OP8();
  int result = interpreter->y - value;
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(2);
}

_6502_instruction_def(cpy_zp) {
  uint16_t addr = OP8();
  int result = interpreter->y - ZP(addr);
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(2);
}

_6502_instruction_def(cpy_abs) {
  uint16_t addr = OP16();
  int result = interpreter->y - MEM(addr);
  SET_FLAGS(result);
  SET_CARRY_VALUE(result >= 0);
  INC_PC(3);
}


// *************************************************************************
// Branches
// *************************************************************************

_6502_instruction_def(bpl) {
  BRANCH(s, 0);
}

_6502_instruction_def(bmi) {
  BRANCH(s, 1);
}

_6502_instruction_def(bvc) {
  BRANCH(v, 0);
}

_6502_instruction_def(bvs) {
  BRANCH(v, 1);
}

_6502_instruction_def(bcc) {
  BRANCH(c, 0);
}

_6502_instruction_def(bcs) {
  BRANCH(c, 1);
}

_6502_instruction_def(bne) {
  BRANCH(z, 0);
}

_6502_instruction_def(beq) {
  BRANCH(z, 1);
}

_6502_instruction_def(bra) {
  int8_t offset = OP8();
  interpreter->pc += offset + 2;
}

// *************************************************************************
// Jumps
// *************************************************************************

_6502_instruction_def(jmp_abs) {
  uint16_t addr = OP16();
  JMP(addr);
}

_6502_instruction_def(jmp_abs_indirect) {
  JMP(INDIRECT2(OP16()));
}

_6502_instruction_def(jmp_abs_x_indirect) {
  JMP(INDIRECT2(OP16()));
}

_6502_instruction_def(jsr_abs) {
  PUSH_PC(2);
  uint16_t addr = OP16();
  JMP(addr);
}

_6502_instruction_def(jsr_abs_x_indirect) {
  PUSH_PC(2);
  uint16_t addr = OP16();
  JMP(*(uint16_t*)(INDEXED_INDIRECT(addr, x)));
}

_6502_instruction_def(rts) {
  POP_PC(1);
}

_6502_instruction_def(rti) {
  POP(flags.value);
  interpreter->flags.value &= ~(3 << 4);  // Clear bits 4 and 5.
  POP_PC(0);
}

// *************************************************************************
// STA
// *************************************************************************

_6502_instruction_def(sta_zp_x_indirect) {
  STORE(a, INDEXED_INDIRECT_ADDR(OP8(), x));
  INC_PC(2);
}

_6502_instruction_def(sta_zp_s) {
  
}

_6502_instruction_def(sta_zp) {
  STORE_ZP(a, ZP_ADDR(OP8()));
  INC_PC(2);
}

_6502_instruction_def(sta_abs) {
  STORE(a, MEM_ADDR(OP16()));
  INC_PC(3);
}

_6502_instruction_def(sta_zp_indirect_y) {
  STORE(a, INDIRECT_INDEXED(OP8(), y));
  INC_PC(2);
}

_6502_instruction_def(sta_zp_indirect) {
  uint16_t addr = OP8();
  STORE(a, INDIRECT_ADDR(addr));
  INC_PC(2);
}

_6502_instruction_def(sta_zp_s_indirect_y) {
  STORE(a, INDIRECT_INDEXED(OP8(), y));
  INC_PC(2);
}

_6502_instruction_def(sta_zp_x) {
  uint16_t addr = OP8();
  STORE_ZP(a, INDEXED(addr, x));
  INC_PC(2);
}

_6502_instruction_def(sta_zp_y) {
  uint16_t addr = OP8();
  STORE_ZP(a, INDEXED(addr, y));
  INC_PC(2);
}

_6502_instruction_def(sta_abs_y) {
  uint16_t addr = OP8();
  STORE(a, INDEXED(addr, y));
  INC_PC(3);
}

_6502_instruction_def(sta_abs_x) {
  uint16_t addr = OP16();
  STORE(a, INDEXED(addr, x));
  INC_PC(3);
}

// *************************************************************************
// STX
// *************************************************************************

_6502_instruction_def(stx_zp) {
  STORE_ZP(x, ZP_ADDR(OP8()));
  INC_PC(2);
}

_6502_instruction_def(stx_abs) {
  STORE(x, MEM_ADDR(OP16()));
  INC_PC(3);
}

_6502_instruction_def(stx_zp_y) {
  uint16_t addr = OP8();
  STORE(x, INDEXED(addr, y));
  INC_PC(2);
}

// *************************************************************************
// STY
// *************************************************************************

_6502_instruction_def(sty_zp_x) {
  uint16_t addr = OP8();
  STORE_ZP(y, INDEXED(addr, x));
  INC_PC(2);
}

_6502_instruction_def(sty_abs) {
  STORE(y, MEM_ADDR(OP16()));
  INC_PC(3);
}

_6502_instruction_def(sty_zp) {
  STORE_ZP(y, ZP_ADDR(OP8()));
  INC_PC(2);
}

// *************************************************************************
// STZ
// *************************************************************************

_6502_instruction_def(stz_zp) {
  STORE_ZERO(ZP_ADDR(OP8()));
  INC_PC(2);
}

_6502_instruction_def(stz_zp_x) {
  uint16_t addr = OP8();
  STORE_ZERO(INDEXED(addr, x));
  INC_PC(2);
}

_6502_instruction_def(stz_abs) {
  STORE_ZERO(MEM_ADDR(OP16()));
  INC_PC(3);
}

_6502_instruction_def(stz_abs_x) {
  uint16_t addr = OP16();
  STORE_ZERO(INDEXED(addr, x));
  INC_PC(3);
}

// *************************************************************************
// Flags
// *************************************************************************

_6502_instruction_def(clc) {
  interpreter->flags.bits.c = 0;
  INC_PC(1);
}

_6502_instruction_def(sec) {
  interpreter->flags.bits.c = 1;
  INC_PC(1);
}

_6502_instruction_def(cli) {
  interpreter->flags.bits.i = 0;
  INC_PC(1);
}

_6502_instruction_def(sei) {
  interpreter->flags.bits.i = 1;
  INC_PC(1);
}

_6502_instruction_def(clv) {
  interpreter->flags.bits.v = 0;
  INC_PC(1);
}

_6502_instruction_def(cld) {
  interpreter->flags.bits.d = 0;
  INC_PC(1);
}

_6502_instruction_def(sed) {
  interpreter->flags.bits.d = 1;
  INC_PC(1);
}

// *************************************************************************
// Stack
// *************************************************************************

_6502_instruction_def(php) {
  PUSH(interpreter->flags.value | (3 << 4));  // Bits 4 and 5 set.
  INC_PC(1);
}

_6502_instruction_def(plp) {
  POP(flags.value);
  interpreter->flags.value &= ~(3 << 4);  // Clear bits 4 and 5.
  INC_PC(1);
}

_6502_instruction_def(pha) {
  PUSH(interpreter->a);
  INC_PC(1);
}

_6502_instruction_def(phy) {
  PUSH(interpreter->y);
  INC_PC(1);
}

_6502_instruction_def(ply) {
  POP(y);
  INC_PC(1);
}

_6502_instruction_def(pla) {
  POP(a);
  INC_PC(1);
}

_6502_instruction_def(phx) {
  PUSH(interpreter->x);
  INC_PC(1);
}

_6502_instruction_def(plx) {
  POP(x);
  INC_PC(1);
}

// *************************************************************************
// Register transfers
// *************************************************************************


_6502_instruction_def(tsx) {
  SET_FLAGS(interpreter->x = interpreter->s);
  INC_PC(1);
}
_6502_instruction_def(tyx) {
  SET_FLAGS(interpreter->y = interpreter->x);
  INC_PC(1);
}

_6502_instruction_def(txa) {
  SET_FLAGS(interpreter->a = interpreter->x);
  INC_PC(1);
}

_6502_instruction_def(tya) {
  SET_FLAGS(interpreter->a = interpreter->y);
  INC_PC(1);
}

_6502_instruction_def(tay) {
  SET_FLAGS(interpreter->y = interpreter->a);
  INC_PC(1);
}


_6502_instruction_def(tax) {
  SET_FLAGS(interpreter->x = interpreter->a);
  INC_PC(1);
}

_6502_instruction_def(txs) {
  SET_FLAGS(interpreter->s = interpreter->x);
  INC_PC(1);
}

_6502_instruction_def(txy) {
  SET_FLAGS(interpreter->y = interpreter->x);
  INC_PC(1);
}


// *************************************************************************
// Additional
// *************************************************************************

// Software break.  Called from hardware BRK handler.  On entry we have
// A = value of byte after BRK instruction.
_6502_instruction_def(sbrk) {
  // Interpreted 6502 BRK.
  INC_PC(1);            // Move PC to byte after BRK.
  BrkHandler(interpreter, interpreter->a);
}

_6502_instruction_def(bpt) {
  // Uninstall the current breakpoint.
  bool temp_bp = false;
  Breakpoint* bp = FindBreakpoint(interpreter, interpreter->pc);
  interpreter->current_bp = bp;
  if (bp->is_temp) {
    // Hit temp breakpoint: remove it, step one instruction and continue
    // with main loop.
    BreakpointUninstall(interpreter, bp);
    RemoveBreakpoint(interpreter, bp);
    temp_bp = true;
  } else {
    printf("Stopped at breakpoint set at address 0x%x\n", interpreter->pc);
    BreakpointUninstall(interpreter, bp);
  }
  
  // Enter debugger command loop.  We will exit on a continue or step
  // instruction.
  DebuggerLoop(interpreter);
  
  // Move on from the current instruction.
  StepOneInstruction(interpreter, false);
  
  // Reinstall the breakpoint.
  if (!temp_bp) {
    BreakpointInstall(interpreter, bp);
  }
  interpreter->current_bp = NULL;
}

_6502_instruction_def(cop_zp) {
  
}

_6502_instruction_def(tsb_zp) {
  
}

_6502_instruction_def(phd) {
  
}

_6502_instruction_def(trb_zp) {
  
}

_6502_instruction_def(tcs) {
  
}
_6502_instruction_def(trb_abs) {
  
}

_6502_instruction_def(pld) {
  
}


_6502_instruction_def(tsc) {
  
}

_6502_instruction_def(wdm) {
  
}

_6502_instruction_def(mvp_s_zp) {
  
}


_6502_instruction_def(phk) {
  
}


_6502_instruction_def(mvn_s_zp) {
  
}



_6502_instruction_def(tcd) {
  
}



_6502_instruction_def(per) {
  
}



_6502_instruction_def(rtl) {
  
}





_6502_instruction_def(tdc) {
  
}

_6502_instruction_def(brl) {
  
}

_6502_instruction_def(phb) {
  
}


_6502_instruction_def(plb) {
  
}

_6502_instruction_def(rep_immed) {
  
}

_6502_instruction_def(wal) {
  
}

_6502_instruction_def(pei_zp) {
  
}

_6502_instruction_def(stp) {
  
}

_6502_instruction_def(jml_abs_indirect) {
  
}

_6502_instruction_def(sep_immed) {
  
}

_6502_instruction_def(xba) {
  
}

_6502_instruction_def(pea_abs) {
  
}

_6502_instruction_def(xce) {
  
}
