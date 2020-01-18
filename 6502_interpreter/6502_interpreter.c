//
//  6502_interpreter.c
//  6502_interpreter
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "6502_disassembler.h"

#define _6502_instruction_decl(x) static void Interpret_##x(_6502Interpreter*);
#define _6502_instruction_def(x) static void Interpret_##x(_6502Interpreter* interpreter)
#define _6502_instruction(x) Interpret_##x,

typedef void (*Instruction)(_6502Interpreter*);

bool disassemble = true;

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
_6502_instruction_decl(sta_zp_y)
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
_6502_instruction_decl(sbc_abs)

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
_6502_instruction_decl(sbc_abs_x)


// Instructions array.  Ordered by opcode.
static Instruction instructions[256] = {
  _6502_instruction(brk)
  _6502_instruction(ora_zp_x_indirect)
  _6502_instruction(cop_zp)
  _6502_instruction(ora_zp)
  _6502_instruction(tsb_zp)
  _6502_instruction(ora_zp)
  _6502_instruction(asl_zp)
  _6502_instruction(ora_zp)
  _6502_instruction(php)
  _6502_instruction(ora_immed)
  _6502_instruction(asl_accum)
  _6502_instruction(phd)
  _6502_instruction(tsb_zp)
  _6502_instruction(ora_abs)
  _6502_instruction(asl_abs)
  _6502_instruction(ora_abl)
  
  _6502_instruction(bpl)
  _6502_instruction(ora_zp_indirect_y)
  _6502_instruction(ora_zp_indirect)
  _6502_instruction(ora_zp_s_indirect_y)
  _6502_instruction(trb_zp)
  _6502_instruction(ora_abs_x)
  _6502_instruction(asl_zp_x)
  _6502_instruction(ora_zp_y)
  _6502_instruction(clc)
  _6502_instruction(ora_abs_y)
  _6502_instruction(inc_a)
  _6502_instruction(tcs)
  _6502_instruction(trb_abs)
  _6502_instruction(ora_abs_x)
  _6502_instruction(asl_abs_x)
  _6502_instruction(ora_abs_x)
  
  _6502_instruction(jsr_abs)
  _6502_instruction(and_zp_x_indirect)
  _6502_instruction(lsr_accum)
  _6502_instruction(and_zp_s)
  _6502_instruction(bit_zp)
  _6502_instruction(and_zp)
  _6502_instruction(rol_zp)
  _6502_instruction(and_zp)
  _6502_instruction(plp)
  _6502_instruction(and_immed)
  _6502_instruction(rol_accum)
  _6502_instruction(pld)
  _6502_instruction(bit_abs)
  _6502_instruction(and_abs)
  _6502_instruction(rol_abs)
  _6502_instruction(and_al)
  
  _6502_instruction(bmi)
  _6502_instruction(and_zp_indirect_y)
  _6502_instruction(and_zp_indirect)
  _6502_instruction(and_zp_s_indirect_y)
  _6502_instruction(bit_zp_x)
  _6502_instruction(and_zp_x)
  _6502_instruction(rol_zp_x)
  _6502_instruction(and_zp_y)
  _6502_instruction(sec)
  _6502_instruction(and_abs_y)
  _6502_instruction(dec_accum)
  _6502_instruction(tsc)
  _6502_instruction(bit_abs_x)
  _6502_instruction(and_abs_x)
  _6502_instruction(rol_abs_x)
  _6502_instruction(and_abs_x)
  
  _6502_instruction(rti)
  _6502_instruction(eor_zp_x_indirect)
  _6502_instruction(wdm)
  _6502_instruction(eor_zp_s)
  _6502_instruction(mvp_s_zp)
  _6502_instruction(eor_zp)
  _6502_instruction(lsr_zp)
  _6502_instruction(eor_zp)
  _6502_instruction(pha)
  _6502_instruction(eor_immed)
  _6502_instruction(lsr_accum)
  _6502_instruction(phk)
  _6502_instruction(jmp_abs)
  _6502_instruction(eor_abs)
  _6502_instruction(lsr_abs)
  _6502_instruction(eor_abs)
  
  _6502_instruction(bvc)
  _6502_instruction(eor_zp_indirect_y)
  _6502_instruction(eor_zp_indirect)
  _6502_instruction(eor_zp_s_indirect_y)
  _6502_instruction(mvn_s_zp)
  _6502_instruction(eor_zp_x)
  _6502_instruction(lsr_zp_x)
  _6502_instruction(eor_zp_x)
  _6502_instruction(cli)
  _6502_instruction(eor_zp_y)
  _6502_instruction(phy)
  _6502_instruction(tcd)
  _6502_instruction(jmp_abs)
  _6502_instruction(eor_zp_x)
  _6502_instruction(lsr_zp_x)
  _6502_instruction(eor_zp_x)
  
  _6502_instruction(rts)
  _6502_instruction(adc_zp_x_indirect)
  _6502_instruction(per)
  _6502_instruction(adc_zp_s)
  _6502_instruction(stz_zp)
  _6502_instruction(adc_zp)
  _6502_instruction(ror_zp)
  _6502_instruction(adc_zp_y)
  _6502_instruction(pla)
  _6502_instruction(adc_immed)
  _6502_instruction(ror_accum)
  _6502_instruction(rtl)
  _6502_instruction(jmp_abs_indirect)
  _6502_instruction(adc_abs)
  _6502_instruction(ror_abs)
  _6502_instruction(adc_abs)
  
  _6502_instruction(bvs)
  _6502_instruction(adc_zp_indirect_y)
  _6502_instruction(adc_zp_indirect)
  _6502_instruction(adc_zp_s_indirect_y)
  _6502_instruction(stz_zp_x)
  _6502_instruction(adc_zp_x)
  _6502_instruction(ror_zp_x)
  _6502_instruction(adc_zp_x)
  _6502_instruction(sei)
  _6502_instruction(adc_abs_y)
  _6502_instruction(ply)
  _6502_instruction(tdc)
  _6502_instruction(jmp_abs_x_indirect)
  _6502_instruction(adc_abs_x)
  _6502_instruction(ror_abs_x)
  _6502_instruction(adc_abs_x)
  
  _6502_instruction(bra)
  _6502_instruction(sta_zp_x_indirect)
  _6502_instruction(brl)
  _6502_instruction(sta_zp_s)
  _6502_instruction(sty_zp)
  _6502_instruction(sta_zp)
  _6502_instruction(stx_zp)
  _6502_instruction(sta_zp)
  _6502_instruction(dey)
  _6502_instruction(bit_immed)
  _6502_instruction(txa)
  _6502_instruction(phb)
  _6502_instruction(sty_abs)
  _6502_instruction(sta_abs)
  _6502_instruction(stx_abs)
  _6502_instruction(sta_abs)
  
  _6502_instruction(bcc)
  _6502_instruction(sta_zp_indirect_y)
  _6502_instruction(sta_zp_indirect)
  _6502_instruction(sta_zp_s_indirect_y)
  _6502_instruction(sty_zp_x)
  _6502_instruction(sta_zp_x)
  _6502_instruction(stx_zp_y)
  _6502_instruction(sta_zp_y)
  _6502_instruction(tya)
  _6502_instruction(sta_zp_y)
  _6502_instruction(txs)
  _6502_instruction(txy)
  _6502_instruction(stz_abs)
  _6502_instruction(sta_abs_x)
  _6502_instruction(stz_abs_x)
  _6502_instruction(sta_abs_x)
  
  _6502_instruction(ldy_immed)
  _6502_instruction(lda_zp_x_indirect)
  _6502_instruction(ldx_immed)
  _6502_instruction(lda_zp_s)
  _6502_instruction(ldy_zp)
  _6502_instruction(lda_zp)
  _6502_instruction(ldx_zp)
  _6502_instruction(lda_zp_indirect)
  _6502_instruction(tay)
  _6502_instruction(lda_immed)
  _6502_instruction(tax)
  _6502_instruction(plb)
  _6502_instruction(ldy_abs)
  _6502_instruction(lda_abs)
  _6502_instruction(ldx_abs)
  _6502_instruction(lda_abs)
  
  _6502_instruction(bcs)
  _6502_instruction(lda_zp_indirect_y)
  _6502_instruction(lda_zp_indirect)
  _6502_instruction(lda_zp_s_indirect_y)
  _6502_instruction(ldy_zp_x)
  _6502_instruction(lda_zp_x)
  _6502_instruction(ldx_zp_y)
  _6502_instruction(lda_zp_indirect_y)
  _6502_instruction(clv)
  _6502_instruction(lda_abs_y)
  _6502_instruction(tsx)
  _6502_instruction(tyx)
  _6502_instruction(ldy_abs_x)
  _6502_instruction(lda_abs_x)
  _6502_instruction(ldx_abs_y)
  _6502_instruction(lda_abs_x)
  
  _6502_instruction(cpy_immed)
  _6502_instruction(cmp_zp_x_indirect)
  _6502_instruction(rep_immed)
  _6502_instruction(cmp_zp_s)
  _6502_instruction(cpy_zp)
  _6502_instruction(cmp_zp)
  _6502_instruction(dec_zp)
  _6502_instruction(cmp_zp_indirect)
  _6502_instruction(iny)
  _6502_instruction(cmp_immed)
  _6502_instruction(dex)
  _6502_instruction(wal)
  _6502_instruction(cpy_abs)
  _6502_instruction(cmp_abs)
  _6502_instruction(dec_abs)
  _6502_instruction(cmp_abs)
  
  _6502_instruction(bne)
  _6502_instruction(cmp_zp_indirect_y)
  _6502_instruction(cmp_zp_indirect)
  _6502_instruction(cmp_zp_s_indirect_y)
  _6502_instruction(pei_zp)
  _6502_instruction(cmp_zp_x)
  _6502_instruction(dec_zp_x)
  _6502_instruction(cmp_zp_indirect_y)
  _6502_instruction(cld)
  _6502_instruction(cmp_abs_y)
  _6502_instruction(phx)
  _6502_instruction(stp)
  _6502_instruction(jml_abs_indirect)
  _6502_instruction(cmp_abs_x)
  _6502_instruction(dec_abs_x)
  _6502_instruction(cmp_abs_x)
  
  _6502_instruction(cpx_immed)
  _6502_instruction(sbc_zp_x_indirect)
  _6502_instruction(sep_immed)
  _6502_instruction(sbc_zp_s)
  _6502_instruction(cpx_zp)
  _6502_instruction(sbc_zp)
  _6502_instruction(inc_zp)
  _6502_instruction(sbc_zp_indirect)
  _6502_instruction(inx)
  _6502_instruction(sbc_immed)
  _6502_instruction(nop)
  _6502_instruction(xba)
  _6502_instruction(cpx_abs)
  _6502_instruction(sbc_abs)
  _6502_instruction(inc_abs)
  _6502_instruction(sbc_abs)
  
  _6502_instruction(beq)
  _6502_instruction(sbc_zp_indirect_y)
  _6502_instruction(sbc_zp_indirect)
  _6502_instruction(sbc_zp_s_indirect_y)
  _6502_instruction(pea_abs)
  _6502_instruction(sbc_zp_x)
  _6502_instruction(inc_zp_x)
  _6502_instruction(sbc_zp_indirect_y)
  _6502_instruction(sed)
  _6502_instruction(sbc_abs_y)
  _6502_instruction(plx)
  _6502_instruction(xce)
  _6502_instruction(jsr_abs_x_indirect)
  _6502_instruction(sbc_abs_x)
  _6502_instruction(inc_abs_x)
  _6502_instruction(sbc_abs_x)
};

void _6502InterpreterInit(_6502Interpreter* interpreter) {
  memset(interpreter, 0, sizeof(_6502Interpreter));
}

void _6502InterpreterRun(_6502Interpreter* interpreter, Loader* loader, uint64_t entry_address, int argc, char** argv) {
  interpreter->memory = calloc(65536, 1);    // 64K of memory.
  interpreter->s = 0xff;
  interpreter->zero_page = (uint8_t*)interpreter->memory;
  interpreter->stack = (uint8_t*)interpreter->memory + interpreter->s + 0x100;
      
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
  interpreter->pc = entry_address;

  // Run processor by fetching instruction at PC and jumping to the handler
  // function for that opcode.  Each handler function will set the PC to
  // the next instruction address.
  for (;;) {
    if (disassemble) {
      Disassemble6502Instruction(interpreter, interpreter->pc,
                                &interpreter->memory[interpreter->pc], stdout);
    }
    uint8_t opcode = interpreter->memory[interpreter->pc];
    (*instructions[opcode])(interpreter);
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
        while (addr <= end_addr) {
          void* new_addr = Disassemble6502Instruction(interpreter, pc, addr, stdout);
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
#define PUSH(value) *(uint8_t*)(interpreter->memory + interpreter->s--) = value;

// Pop stack into register.
#define POP(reg) interpreter->reg = interpreter->memory[++interpreter->s]

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
  SET_OVERFLOW(!(((interpreter->a ^ src) & 0x80) &\
    ((interpreter->a ^ result) & 0x80)));

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
#define MEM_ADDR(addr) (interpreter->memory + addr)

// Increment the PC by the given number of bytes.
#define INC_PC(inc) interpreter->pc += inc

// A 16-bit address formed by adding an absolute address to an index register.
#define INDEXED(addr, reg) (interpreter->memory + addr + interpreter->reg)

// An 8-bit value read from memory at the given address
#define INDIRECT(addr) *(uint8_t*)(interpreter->memory + addr)

// A 16-bit addres read from memory at the given address
#define INDIRECT_ADDR(addr) (interpreter->memory + addr)

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
#define LOAD(reg, addr) SET_FLAGS(interpreter->reg = *(addr))

// Store a register into an address.
#define STORE(reg, addr) *(addr) = interpreter->reg

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
    interpreter->pc += offset;\
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
  PUSH(interpreter->flags.value | (3 << 4));    // Push P with bits 4 and 4 fiag set.
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
  INC_PC(2);
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
  INC_PC(2);
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
  LOAD(a, ZP_ADDR(OP8()));
  INC_PC(2);
}

_6502_instruction_def(lda_zp_indirect) {
  LOAD(a, INDIRECT_ADDR(OP8()));
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
  LOAD(a, INDEXED(OP8(), x));
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
  LOAD(x, ZP_ADDR(OP8()));
  INC_PC(2);
}


_6502_instruction_def(ldx_abs) {
  LOAD(x, MEM_ADDR(OP16()));
  INC_PC(3);
}

_6502_instruction_def(ldx_zp_y) {
  LOAD(x, INDEXED(OP8(), y));
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
  LOAD(y, ZP_ADDR(OP8()));
  INC_PC(2);
}

_6502_instruction_def(ldy_zp_x) {
  LOAD(y, INDEXED(OP8(), x));
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
  ADD_WITH_CARRY(INDIRECT(OP8()));
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
  SUB_WITH_BORROW(INDIRECT(OP8()));
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
  uint8_t* p = MEM_ADDR(OP16());
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
  int result = interpreter->a - *INDEXED_INDIRECT(addr,x);
  SET_FLAGS(result);
  SET_OVERFLOW_A(ZP(addr), result);
  INC_PC(2);
}

_6502_instruction_def(cmp_zp_s) {
  
}

_6502_instruction_def(cmp_zp) {
  uint16_t addr = OP8();
  int result = interpreter->a - ZP(addr);
  SET_FLAGS(result);
  SET_OVERFLOW_A(ZP(addr), result);
  INC_PC(2);
}

_6502_instruction_def(cmp_zp_indirect) {
  
}

_6502_instruction_def(cmp_immed) {
  uint16_t value = OP8();
  int result = interpreter->a - value;
  SET_FLAGS(result);
  SET_OVERFLOW_A(value, result);
  INC_PC(2);
}

_6502_instruction_def(cmp_abs) {
  uint16_t addr = OP16();
  int result = interpreter->a - MEM(addr);
  SET_FLAGS(result);
  SET_OVERFLOW_A(MEM(addr), result);
  INC_PC(3);
}

_6502_instruction_def(cmp_zp_indirect_y) {
  uint16_t addr = OP8();
  int result = interpreter->a - *INDIRECT_INDEXED(addr,y);
  SET_FLAGS(result);
  SET_OVERFLOW_A(ZP(addr), result);
  INC_PC(2);
}

_6502_instruction_def(cmp_zp_s_indirect_y) {
  
}

_6502_instruction_def(cmp_zp_x) {
  uint16_t addr = OP8();
  uint8_t* ptr = INDEXED(addr, x);
  int result = interpreter->a - *ptr;
  SET_FLAGS(result);
  SET_OVERFLOW_A(*ptr, result);
  INC_PC(2);
}

_6502_instruction_def(cmp_abs_y) {
  uint16_t addr = OP16();
  uint8_t* ptr = INDEXED(addr, y);
  int result = interpreter->a - *ptr;
  SET_FLAGS(result);
  SET_OVERFLOW_A(*ptr, result);
  INC_PC(3);
}

_6502_instruction_def(cmp_abs_x) {
  uint16_t addr = OP16();
  uint8_t* ptr = INDEXED(addr, x);
  int result = interpreter->a - *ptr;
  SET_FLAGS(result);
  SET_OVERFLOW_A(*ptr, result);
  INC_PC(3);
}



// *************************************************************************
// CPX
// *************************************************************************

_6502_instruction_def(cpx_immed) {
  uint16_t value = OP8();
  int result = interpreter->x - value;
  SET_FLAGS(result);
  SET_OVERFLOW_A(value, result);
  INC_PC(2);
}

_6502_instruction_def(cpx_zp) {
  uint16_t addr = OP8();
  int result = interpreter->x - ZP(addr);
  SET_FLAGS(result);
  SET_OVERFLOW_A(ZP(addr), result);
  INC_PC(2);
}

_6502_instruction_def(cpx_abs) {
  uint16_t addr = OP16();
  int result = interpreter->x - MEM(addr);
  SET_FLAGS(result);
  SET_OVERFLOW_A(MEM(addr), result);
  INC_PC(3);
}



// *************************************************************************
// CPY
// *************************************************************************

_6502_instruction_def(cpy_immed) {
  uint16_t value = OP8();
  int result = interpreter->y - value;
  SET_FLAGS(result);
  SET_OVERFLOW_A(value, result);
  INC_PC(2);
}

_6502_instruction_def(cpy_zp) {
  uint16_t addr = OP8();
  int result = interpreter->y - ZP(addr);
  SET_FLAGS(result);
  SET_OVERFLOW_A(ZP(addr), result);
  INC_PC(2);
}

_6502_instruction_def(cpy_abs) {
  uint16_t addr = OP16();
  int result = interpreter->y - MEM(addr);
  SET_FLAGS(result);
  SET_OVERFLOW_A(MEM(addr), result);
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
  uint8_t offset = OP8();
  interpreter->pc += offset;
}

// *************************************************************************
// Jumps
// *************************************************************************

_6502_instruction_def(jmp_abs) {
  uint16_t addr = OP16();
  JMP(addr);
}

_6502_instruction_def(jmp_abs_indirect) {
  JMP(INDIRECT(OP16()));
}

_6502_instruction_def(jmp_abs_x_indirect) {
  JMP(INDIRECT(OP16()));
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
  STORE(a, ZP_ADDR(OP8()));
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
  STORE(a, INDEXED(addr, x));
  INC_PC(2);
}

_6502_instruction_def(sta_zp_y) {
  uint16_t addr = OP8();
  STORE(a, INDEXED(addr, y));
  INC_PC(2);
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
  STORE(x, ZP_ADDR(OP8()));
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
  STORE(y, INDEXED(addr, x));
  INC_PC(2);
}

_6502_instruction_def(sty_abs) {
  STORE(y, MEM_ADDR(OP16()));
  INC_PC(3);
}

_6502_instruction_def(sty_zp) {
  STORE(y, ZP_ADDR(OP8()));
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
  SET_FLAGS(interpreter->s = interpreter->x);
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

_6502_instruction_def(tya) {
  
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
