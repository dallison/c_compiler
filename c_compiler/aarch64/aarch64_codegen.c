//
//  g->_codegen.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "aarch64_codegen.h"
#include "aarch64_optimize.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "debug.h"
#include "member_pointer.h"

#include "target_basic_block.h"

static void LowerVariables(AARCH64Generator* g, Generator* gen);

static void Trap() {}
static void TrapLower(String* name) {
  if (StringEqual(name, "")) {
    Trap();
  }
}

const char* AARCH64OpcodeName(int op) {
  switch (op) {
  case AARCH64_OP(save): return "save";
  case AARCH64_OP(restore): return "restore";

  case AARCH64_OP(symbol): return "symbol";   // Static symbol.
  case AARCH64_OP(literal): return "literal";  // String literal.
  case AARCH64_OP(tmp): return "tmp";

  // Constants.
  case AARCH64_OP(const8): return "const8";
  case AARCH64_OP(const16): return "const16";
  case AARCH64_OP(const32): return "const32";
  case AARCH64_OP(const64): return "const64";
  case AARCH64_OP(constf): return "constf";
  case AARCH64_OP(constd): return "constd";

  case AARCH64_OP(mv): return "mv";
  case AARCH64_OP(fmv_s): return "fmv_s";
  case AARCH64_OP(fmv_d): return "fmv_d";

  case AARCH64_OP(movc): return "movc";
  case AARCH64_OP(movfc): return "movfc";
  case AARCH64_OP(movdc): return "movdc";
  case AARCH64_OP(movxc): return "movxc";

  case AARCH64_OP(ret): return "ret";

  case AARCH64_OP(label): return "label";

  case AARCH64_OP(fp): return "fp";  // Frame pointer pseudo operation.
  case AARCH64_OP(sp): return "sp";  // Stack pointer pseudo operation.
  case AARCH64_OP(tp): return "tp";  // Thread pointer pseudo operation.

  // Function result registers.
  case AARCH64_OP(resulti): return "resulti";
  case AARCH64_OP(resultf): return "resultf";
  case AARCH64_OP(resultd): return "resultd";

  case AARCH64_OP(structreturn): return "structreturn";  // Struct return address.

  case AARCH64_OP(asm): return "asm";

  case AARCH64_OP(loc): return "loc";
  case AARCH64_OP(named_label): return "named_label";
  case AARCH64_OP(ivarreg): return "ivarreg";
  case AARCH64_OP(fvarreg): return "fvarreg";
  
  // End of TargetOpcode enumeration.

  // Now follow the actual AARCH64v8 64 instruction directly from the
  // specifications.

  case AARCH64_OP(adc): return "adc";
  case AARCH64_OP(add): return "add";
  case AARCH64_OP(adcs): return "adcs";
  case AARCH64_OP(adds): return "adds";
  case AARCH64_OP(adr): return "adr";
  case AARCH64_OP(adrp): return "adrp";
  case AARCH64_OP(gotaddr): return "gotaddr";
  case AARCH64_OP(cmn): return "cmn";
  case AARCH64_OP(cmp): return "cmp";
  case AARCH64_OP(madd): return "madd";
  case AARCH64_OP(mneg): return "mneg";
  case AARCH64_OP(msub): return "msub";
  case AARCH64_OP(mul): return "mul";
  case AARCH64_OP(neg): return "neg";
  case AARCH64_OP(ngc): return "ngc";
  case AARCH64_OP(sbc): return "sbc";
  case AARCH64_OP(negs): return "negs";
  case AARCH64_OP(ngcs): return "ngcs";
  case AARCH64_OP(sbcs): return "sbcs";
    case AARCH64_OP(sdiv): return "sdiv";
    case AARCH64_OP(smod): return "smod";
  case AARCH64_OP(smaddl): return "smaddl";
  case AARCH64_OP(smnegl): return "smnegl";
  case AARCH64_OP(smsubl): return "smsubl";
  case AARCH64_OP(smulh): return "smulh";
  case AARCH64_OP(smull): return "smull";
  case AARCH64_OP(sub): return "sub";
  case AARCH64_OP(subs): return "subs";
    case AARCH64_OP(udiv): return "udiv";
    case AARCH64_OP(umod): return "umod";
  case AARCH64_OP(umaddl): return "umaddl";
  case AARCH64_OP(umnegl): return "umnegl";
  case AARCH64_OP(umsubl): return "umsubl";
  case AARCH64_OP(umulh): return "umulh";
    case AARCH64_OP(umull): return "umull";
    case AARCH64_OP(eor): return "eor";

  case AARCH64_OP(bfi): return "bfi";
  case AARCH64_OP(bfxil): return "bfxil";
  case AARCH64_OP(cls): return "cls";
  case AARCH64_OP(clz): return "clz";
  case AARCH64_OP(extr): return "extr";
  case AARCH64_OP(rbit): return "rbit";
  case AARCH64_OP(rev): return "rev";
  case AARCH64_OP(rev16): return "rev16";
  case AARCH64_OP(rev32): return "rev32";
  case AARCH64_OP(sbfiz): return "sbfiz";
  case AARCH64_OP(ubfiz): return "ubfiz";
  case AARCH64_OP(sbfx): return "sbfx";
  case AARCH64_OP(ubfx): return "ubfx";
  case AARCH64_OP(sbxt): return "sbxt";
  case AARCH64_OP(sbxtb): return "sbxtb";
  case AARCH64_OP(sbxth): return "sbxth";
  case AARCH64_OP(ubxt): return "ubxt";
  case AARCH64_OP(ubxtb): return "ubxtb";
  case AARCH64_OP(ubxth): return "ubxth";
  case AARCH64_OP(sxtb): return "sxtb";
  case AARCH64_OP(sxth): return "sxth";
  case AARCH64_OP(sxtw): return "sxtw";

  case AARCH64_OP(and): return "and";
  case AARCH64_OP(ands): return "ands";
  case AARCH64_OP(asr): return "asr";
  case AARCH64_OP(asrv): return "asrv";
  case AARCH64_OP(bic): return "bic";
  case AARCH64_OP(bics): return "bics";
  case AARCH64_OP(eon): return "eon";
  case AARCH64_OP(eons): return "eons";
  case AARCH64_OP(lsl): return "lsl";
  case AARCH64_OP(lslv): return "lslv";
  case AARCH64_OP(lsr): return "lsr";
  case AARCH64_OP(lsrv): return "lsrv";
  case AARCH64_OP(mov): return "mov";
  case AARCH64_OP(movk): return "movk";
  case AARCH64_OP(movn): return "movn";
  case AARCH64_OP(movz): return "movz";
  case AARCH64_OP(mvn): return "mvn";
  case AARCH64_OP(orn): return "orn";
  case AARCH64_OP(orr): return "orr";
  case AARCH64_OP(ror): return "ror";
  case AARCH64_OP(rorv): return "rorv";
  case AARCH64_OP(tst): return "tst";

  case AARCH64_OP(b): return "b";
  case AARCH64_OP(bl): return "bl";
  case AARCH64_OP(blr): return "blr";
  case AARCH64_OP(br): return "br";
  case AARCH64_OP(cbnz): return "cbnz";
  case AARCH64_OP(cbz): return "cbz";
  case AARCH64_OP(tbnz): return "tbnz";
  case AARCH64_OP(tbz): return "tbz";
  
    case AARCH64_OP(eq): return "eq";
    case AARCH64_OP(ne): return "ne";
    case AARCH64_OP(cs): return "cs";
    case AARCH64_OP(hs): return "hs";
    case AARCH64_OP(cc): return "cc";
    case AARCH64_OP(lo): return "lo";
    case AARCH64_OP(mi): return "mi";
    case AARCH64_OP(pl): return "pl";
    case AARCH64_OP(vs): return "vs";
    case AARCH64_OP(vc): return "vc";
    case AARCH64_OP(hi): return "hi";
    case AARCH64_OP(ls): return "ls";
    case AARCH64_OP(ge): return "ge";
    case AARCH64_OP(lt): return "lt";
    case AARCH64_OP(gt): return "gt";
    case AARCH64_OP(le): return "le";
    case AARCH64_OP(al): return "al";
      
  case AARCH64_OP(ccmn): return "ccmn";
  case AARCH64_OP(ccmp): return "ccmp";
  case AARCH64_OP(cinc): return "cinc";
  case AARCH64_OP(cinv): return "cinv";
  case AARCH64_OP(cneg): return "cneg";
  case AARCH64_OP(csel): return "csel";
  case AARCH64_OP(cset): return "cset";
  case AARCH64_OP(csetm): return "csetm";
  case AARCH64_OP(csinc): return "csinc";
  case AARCH64_OP(csinv): return "csinv";
  case AARCH64_OP(csneg): return "csneg";
  
  case AARCH64_OP(ldp): return "ldp";
  case AARCH64_OP(ldpsw): return "ldpsw";
  case AARCH64_OP(ldr): return "ldr";
  case AARCH64_OP(ldur): return "ldur";
  case AARCH64_OP(ldrb): return "ldrb";
  case AARCH64_OP(ldrh): return "ldrh";
  case AARCH64_OP(ldurb): return "ldurb";
  case AARCH64_OP(ldurh): return "ldurh";
  case AARCH64_OP(ldrsb): return "ldrsb";
  case AARCH64_OP(ldrsh): return "ldrsh";
  case AARCH64_OP(ldursb): return "ldursb";
  case AARCH64_OP(ldursh): return "ldursh";
  case AARCH64_OP(ldursw): return "ldursw";
  case AARCH64_OP(prfm): return "prfm";
  case AARCH64_OP(stp): return "stp";
  case AARCH64_OP(str): return "str";
  case AARCH64_OP(stur): return "stur";
  case AARCH64_OP(strb): return "strb";
  case AARCH64_OP(strh): return "strh";
  case AARCH64_OP(sturb): return "sturb";
  case AARCH64_OP(sturh): return "sturh";

  case AARCH64_OP(ldxr): return "ldxr";
  case AARCH64_OP(ldaxr): return "ldaxr";
  case AARCH64_OP(stxr): return "stxr";
  case AARCH64_OP(stlxr): return "stlxr";
  case AARCH64_OP(ldar): return "ldar";
  case AARCH64_OP(stlr): return "stlr";
  case AARCH64_OP(dmb): return "dmb";
  case AARCH64_OP(clrex): return "clrex";

  case AARCH64_OP(atomic_load): return "atomic_load";
  case AARCH64_OP(atomic_store): return "atomic_store";
  case AARCH64_OP(atomic_fetch_add): return "atomic_fetch_add";
  case AARCH64_OP(atomic_fetch_sub): return "atomic_fetch_sub";
  case AARCH64_OP(atomic_add_fetch): return "atomic_add_fetch";
  case AARCH64_OP(atomic_sub_fetch): return "atomic_sub_fetch";
  case AARCH64_OP(atomic_compare_exchange_bool):
    return "atomic_compare_exchange_bool";
  case AARCH64_OP(atomic_compare_exchange_val):
    return "atomic_compare_exchange_val";
  case AARCH64_OP(atomic_compare_exchange_n):
    return "atomic_compare_exchange_n";
  case AARCH64_OP(atomic_fence): return "atomic_fence";

  case AARCH64_OP(fldr): return "fldr";
  case AARCH64_OP(fstr): return "fstr";
  case AARCH64_OP(fadd): return "fadd";
  case AARCH64_OP(fsub): return "fsub";
  case AARCH64_OP(fmul): return "fmul";
  case AARCH64_OP(fdiv): return "fdiv";
  case AARCH64_OP(fsqrt): return "fsqrt";
  case AARCH64_OP(fmin): return "fmin";
  case AARCH64_OP(fmax): return "fmax";
  case AARCH64_OP(fcvtns): return "fcvtns";
  case AARCH64_OP(fcvtnu): return "fcvtnu";
  case AARCH64_OP(fcvtzs): return "fcvtzs";
  case AARCH64_OP(fcvtzu): return "fcvtzu";
    case AARCH64_OP(fcvtsd): return "fcvtsd";
    case AARCH64_OP(fcvtds): return "fcvtds";
  case AARCH64_OP(fmov): return "fmov";
  case AARCH64_OP(fcmp): return "fcmp";
  case AARCH64_OP(scvtf): return "scvtf";
  case AARCH64_OP(ucvtf): return "ucvtf";
    case AARCH64_OP(fneg): return "fneg";
    case AARCH64_OP(fcvt): return "fcvt";
    case AARCH64_OP(vadd): return "vadd";
    case AARCH64_OP(vsub): return "vsub";
    case AARCH64_OP(vand): return "vand";
    case AARCH64_OP(vorr): return "vorr";
    case AARCH64_OP(veor): return "veor";
    case AARCH64_OP(vcmeq): return "vcmeq";
    case AARCH64_OP(vcmgt): return "vcmgt";
    case AARCH64_OP(vcmge): return "vcmge";
    case AARCH64_OP(vcmhi): return "vcmhi";
    case AARCH64_OP(vcmhs): return "vcmhs";
    case AARCH64_OP(vfadd): return "vfadd";
    case AARCH64_OP(vfsub): return "vfsub";
    case AARCH64_OP(vfmul): return "vfmul";
    case AARCH64_OP(vfdiv): return "vfdiv";

  case AARCH64_OP(xxx): return "xxx";
    case AARCH64_OP(not): return "not";
    case AARCH64_OP(nop): return "nop";

    case AARCH64_OP(oplsl): return "oplsl";

  case AARCH64_OP(nrvoval): return "nrvoval";
  
  // Argument Registers
  // int
  case AARCH64_OP(r0): return "r0";
  case AARCH64_OP(r1): return "r1";
  case AARCH64_OP(r2): return "r2";
  case AARCH64_OP(r3): return "r3";
  case AARCH64_OP(r4): return "r4";
  case AARCH64_OP(r5): return "r5";
  case AARCH64_OP(r6): return "r6";
    case AARCH64_OP(r7): return "r7";
    case AARCH64_OP(r8): return "r8";
    case AARCH64_OP(r9): return "r9";

  case AARCH64_OP(d0): return "d0";
  case AARCH64_OP(d1): return "d1";
  case AARCH64_OP(d2): return "d2";
  case AARCH64_OP(d3): return "d3";
  case AARCH64_OP(d4): return "d4";
  case AARCH64_OP(d5): return "d5";
  case AARCH64_OP(d6): return "d6";
  case AARCH64_OP(d7): return "d7";

  case AARCH64_OP(zr): return "zr";
  case AARCH64_OP(lr): return "lr";
  case AARCH64_OP(xr): return "xr";

  case AARCH64_OP(regarg): return "regarg";  // Holder for reg args.
  
  // Spill and reload.
  case AARCH64_OP(spill): return "spill";
  case AARCH64_OP(reload): return "reload";
    default: return "unknown AARCH64 instruction";
  }
}

bool AARCH64IsExpression(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(label):
    case AARCH64_OP(asm):
    case AARCH64_OP(b):
    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
    case AARCH64_OP(br):
    case AARCH64_OP(cbnz):
    case AARCH64_OP(cbz):
    case AARCH64_OP(tbnz):
    case AARCH64_OP(tbz):
    case AARCH64_OP(ret):
    case AARCH64_OP(save):
    case AARCH64_OP(restore):
    case AARCH64_OP(stp):
    case AARCH64_OP(str):
    case AARCH64_OP(stur):
    case AARCH64_OP(strb):
    case AARCH64_OP(strh):
    case AARCH64_OP(sturb):
    case AARCH64_OP(sturh):
    case AARCH64_OP(stlr):
    case AARCH64_OP(dmb):
    case AARCH64_OP(clrex):
    case AARCH64_OP(atomic_store):
    case AARCH64_OP(atomic_fence):
    case AARCH64_OP(loc):
    case AARCH64_OP(named_label):
    case AARCH64_OP(regarg):
    case AARCH64_OP(nrvoval):
    case AARCH64_OP(symbol):
    case AARCH64_OP(spill):
      return false;
    default:
      return !AARCH64IsFixedRegister(inst) && !AARCH64IsConst(inst);
  }
}

bool AARCH64GeneratesOutput(TargetInstruction* inst) {
  if (AARCH64IsFixedRegister(inst)) {
    return false;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(ivarreg):
    case AARCH64_OP(fvarreg):
    case AARCH64_OP(resulti):
    case AARCH64_OP(resultf):
    case AARCH64_OP(resultd):
      return false;
    default:
      return AARCH64IsExpression(inst) && !AARCH64IsSymbol(inst) && !AARCH64IsConst(inst);
  }
}

bool AARCH64IsLoad(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(ldp):
    case AARCH64_OP(ldpsw):
    case AARCH64_OP(ldr):
    case AARCH64_OP(ldur):
    case AARCH64_OP(ldrb):
    case AARCH64_OP(ldrh):
    case AARCH64_OP(ldurb):
    case AARCH64_OP(ldurh):
    case AARCH64_OP(ldrsb):
    case AARCH64_OP(ldrsh):
    case AARCH64_OP(ldursb):
    case AARCH64_OP(ldursh):
    case AARCH64_OP(ldursw):
      return true;
    default:
      return false;
  }
}

bool AARCH64IsSignedLoad(TargetInstruction* inst) {
  // Only the sign-extending loads leave a sign-extended value in the
  // destination register.  Plain ldr/ldur/ldrb/ldrh zero-extend (writing a
  // w-register clears the upper 32 bits), so a value loaded by them still needs
  // an explicit sign-extension when widened.
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(ldpsw):
    case AARCH64_OP(ldrsb):
    case AARCH64_OP(ldrsh):
    case AARCH64_OP(ldursb):
    case AARCH64_OP(ldursh):
    case AARCH64_OP(ldursw):
      return true;
    default:
      return false;
  }
}

bool AARCH64IsStore(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(stp):
    case AARCH64_OP(str):
    case AARCH64_OP(stur):
    case AARCH64_OP(strb):
    case AARCH64_OP(strh):
    case AARCH64_OP(sturb):
    case AARCH64_OP(sturh):
      return true;
    default:
      return false;
  }
}

bool AARCH64IsIntConst(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(const32):
    case AARCH64_OP(const8):
    case AARCH64_OP(const16):
    case AARCH64_OP(const64):
      return true;
    default:
      return false;
  }
}

bool AARCH64IsConst(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(const32):
    case AARCH64_OP(const8):
    case AARCH64_OP(const16):
    case AARCH64_OP(const64):
    case AARCH64_OP(constf):
    case AARCH64_OP(constd):
      return true;
    default:
      return false;
  }
}

bool AARCH64IsFloatingPoint(TargetInstruction* inst) {
  if ((AARCH64Opcode)inst->opcode < AARCH64_OP(fldr) ||
      (AARCH64Opcode)inst->opcode > AARCH64_OP(vcmhs)) {
    return false;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(fvarreg):
    case AARCH64_OP(constf):
    case AARCH64_OP(constd):
    case AARCH64_OP(fmv_s):
    case AARCH64_OP(fmv_d):
       return true;
    default:
      return false;
  }
}

bool AARCH64IsSymbol(TargetInstruction* inst) {
  return (AARCH64Opcode)((int)inst->opcode == (int)AARCH64_OP(symbol));
}

bool AARCH64IsCall(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
      return true;
    default:
      return false;
  }
}

bool AARCH64IsArgRegister(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(r0):
    case AARCH64_OP(r1):
    case AARCH64_OP(r2):
    case AARCH64_OP(r3):
    case AARCH64_OP(r4):
    case AARCH64_OP(r5):
    case AARCH64_OP(r6):
    case AARCH64_OP(r7):
      return true;
    default:
      return false;
  }
}
 
bool AARCH64IsVarRegister(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
  case AARCH64_OP(ivarreg):
  case AARCH64_OP(fvarreg):
      return true;
  default:
    return false;
  }
}

bool AARCH64IsFixedRegister(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(r0):
    case AARCH64_OP(r1):
    case AARCH64_OP(r2):
    case AARCH64_OP(r3):
    case AARCH64_OP(r4):
    case AARCH64_OP(r5):
    case AARCH64_OP(r6):
    case AARCH64_OP(r7):
    case AARCH64_OP(fp):
    case AARCH64_OP(sp):
    case AARCH64_OP(lr):
    case AARCH64_OP(xr):
    case AARCH64_OP(zr):
    case AARCH64_OP(r9):

      // Calls always return in z0 (or fa0?).
    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
      return true;
    default:
      return false;
  }
}

bool AARCH64IsResult(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
     case AARCH64_OP(resulti):
      case AARCH64_OP(resultf):
      case AARCH64_OP(resultd):
      return true;
    default:
      return false;
  }
}

bool AARCH64IsBranch(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(b):
    case AARCH64_OP(br):
    case AARCH64_OP(cbz):
    case AARCH64_OP(cbnz):
      return true;
    default:
      return false;
  }
}

bool AARCH64IsConditionalBranch(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(b):
      // AL condition means unconditional.
      return ((int)inst->operand[0]->opcode != (int)AARCH64_OP(al));
 
    case AARCH64_OP(br):
      return false;

    case AARCH64_OP(cbz):
    case AARCH64_OP(cbnz):
      return true;
      
    default:
      return false;
  }
}

bool AARCH64IsSpill(TargetInstruction* inst) {
  return (AARCH64Opcode)((int)inst->opcode == (int)AARCH64_OP(spill));
}

bool AARCH64IsLabel(TargetInstruction* inst) {
  return (AARCH64Opcode)((int)inst->opcode == (int)AARCH64_OP(label));
}

bool AARCH64IsReturn(TargetInstruction* inst) {
  return (AARCH64Opcode)((int)inst->opcode == (int)AARCH64_OP(ret));
}

int64_t AARCH64IntValue(TargetInstruction* inst) {
  if (inst->opcode == (TargetOpcode)AARCH64_OP(r0)) {
    return 0;
  }
  return ((TargetConstant*)inst)->value.ivalue;
}

// Is the value small enough to be encoded in an immediate field?
// TODO: there are many sizes of immediate fields.  This is wrong->base.
bool AARCH64IsPossibleImmediate(int64_t value) {
  // Check for 12 bit signed immediate.
  if (value < 0) {
    return value >= -2048;
  }
  return value < 2048;
}

// True when `offset` can be used directly as the immediate of a load/store.
// The unscaled forms the codegen emits for frame access (ldur/stur and the
// byte/half variants) take a *signed 9-bit* byte offset, i.e. -256..255.  This
// is much narrower than AARCH64IsPossibleImmediate (which models the 12-bit
// add/sub immediate), so it must NOT be used to decide whether a memory access
// offset is encodable: an offset such as -320 fits the 12-bit add range but is
// silently truncated to 9 bits by the assembler (-320 & 0x1ff == +192),
// redirecting the access into the caller's frame.  Anything outside this window
// must have its effective address materialized into a register first.
static bool AARCH64LoadStoreImmInRange(int32_t offset) {
  return offset >= -256 && offset <= 255;
}

// True when `value` can be represented as an AArch64 logical (bitmask)
// immediate for an operation of the given width.  Only such values may be used
// with the AND/ORR/EOR immediate forms; anything else must be materialized in a
// register.  This mirrors AARCH64EncodeLogicalImmediate in the assembler.
static bool AARCH64IsMaskRun(uint64_t v) {
  return v != 0 && ((v + 1) & v) == 0;
}
static bool AARCH64IsShiftedMaskRun(uint64_t v) {
  return v != 0 && AARCH64IsMaskRun((v - 1) | v);
}
bool AARCH64IsLogicalImmediate(int64_t value, bool is64) {
  uint64_t imm = (uint64_t)value;
  unsigned reg_size = is64 ? 64 : 32;
  if (reg_size != 64) {
    imm &= 0xffffffffULL;
  }
  uint64_t all_ones = reg_size == 64 ? ~0ULL : 0xffffffffULL;
  if (imm == 0 || imm == all_ones) {
    return false;
  }
  unsigned size = reg_size;
  do {
    size /= 2;
    uint64_t mask = (1ULL << size) - 1;
    if ((imm & mask) != ((imm >> size) & mask)) {
      size *= 2;
      break;
    }
  } while (size > 2);
  uint64_t mask = (~0ULL) >> (64 - size);
  imm &= mask;
  if (AARCH64IsShiftedMaskRun(imm)) {
    return true;
  }
  imm |= ~mask;
  return AARCH64IsShiftedMaskRun(~imm);
}

// The size of the data an instruction operates on is 32 or 64 bits.
int GetRegisterSize(TargetInstruction* inst) {
  return (inst->flags >> 16) & 3;
}

// Set instrution size with 1 == 64 bit.
TargetInstruction* SetInstructionSize(TargetInstruction* inst, int size) {
  inst->flags = (inst->flags & ~(3<<16)) | (size << 16);
  return inst;
}

static bool TypeUsesAArch64FpArgReg(TypeRecord* type);
static bool TypePassedAsAArch64Aggregate(TypeRecord* type);

// Copy size from operand.
TargetInstruction* CopyInstructionSize(TargetInstruction* inst, int op) {
  if (inst->operand[op] == NULL) {
    return inst;
  }
  inst->flags = (inst->flags & ~(3<<16)) | ((inst->operand[op]->flags & (3 << 16)));
  return inst;
}

TargetInstruction* CopyOrSetInstructionSize(IRNode* node, TargetInstruction* inst) {
  // Size the instruction by the IR node's own result type.  Copying the size
  // from operand 0 (the previous behaviour for non-leaf nodes) is wrong
  // whenever the result is wider than that operand: e.g. assembling a 64-bit
  // value by shifting a byte left by 56, or computing a pointer offset from a
  // 32-bit index.  In those cases the operation would be emitted at the narrow
  // operand width, dropping high bits or using an out-of-range shift amount.
  if (node->type != NULL) {
    int size = kSize32Bit;
    bool address_operation =
        node->opcode == IR_OP(adda) || node->opcode == IR_OP(suba) ||
        node->opcode == IR_OP(mova) || node->opcode == IR_OP(nota);
    // Struct/union-typed nodes denote an aggregate, which in this ABI is
    // referenced by address; size them as 64-bit pointers (e.g. forming
    // &local to copy a struct argument by value).
    if (address_operation || node->type->size == 8 ||
        TypeIsLong(node->type) || TypeIsLongLong(node->type) ||
        TypeIsPointerOrArray(node->type) || TypeIsFunction(node->type) ||
        TypeUsesFloat64Representation(node->type) ||
        TypeIsStructOrUnion(node->type) || TypeIsVector(node->type)) {
      size = kSize64Bit;
    }
    SetInstructionSize(inst, size);
  } else if (node->inputs.length > 0) {
    // No result type to consult; fall back to the first operand's width.
    CopyInstructionSize(inst, 0);
  } else {
    SetInstructionSize(inst, kSize32Bit);
  }
  return inst;
}

TargetInstruction* AARCH64GetBranchTarget(TargetInstruction* inst) {
  if (((int)inst->opcode == (int)AARCH64_OP(br))) {
    return inst->operand[0];
  }
  return inst->operand[1];
}

bool AARCH64IsJumpableEntry(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)AARCH64_OP(b) && ((int)inst->operand[0]->opcode == (int)AARCH64_OP(al));
}

static bool HasLoweredNode(IRNode* node) {
  return node->data.ptr != NULL;
}

static TargetVirtuals virtuals = {
  .opcode_name = AARCH64OpcodeName,
  .is_branch = AARCH64IsBranch,
  .is_call = AARCH64IsCall,
  .is_return = AARCH64IsReturn,
  .is_spill = AARCH64IsSpill,
  .is_label = AARCH64IsLabel,
  .is_floating_point = AARCH64IsFloatingPoint,
  .is_conditional_branch = AARCH64IsConditionalBranch,
  .is_fixed_register = AARCH64IsFixedRegister,
  .is_const = AARCH64IsConst,
  .is_symbol = AARCH64IsSymbol,
  .is_expression = AARCH64IsExpression,
  .is_table_entry = AARCH64IsJumpableEntry,
  .get_branch_target = AARCH64GetBranchTarget,
  .calls_may_stay_in_block = true,
};

void AARCH64GeneratorInit(AARCH64Generator* g, Generator* gen) {
  TargetGeneratorInit(&g->base, gen, &virtuals);

  g->num_int_arg_regs = 0;
  g->num_fp_arg_regs = 0;
  g->num_int_reg_vars = 0;
  g->num_fp_reg_vars = 0;
  g->struct_return_reg = -1;
  g->struct_return_spill_offset = 0;
  g->not_leaf = false;
  g->zero = NULL;
  g->tmp = NULL;
  g->lsl = NULL;
  g->struct_return_argument_register = NULL;

  memset(g->int_argument_registers, 0, sizeof(g->int_argument_registers));
  memset(g->fp_argument_registers, 0, sizeof(g->fp_argument_registers));
  VectorInit(&g->var_regs);
  VectorInit(&g->saved_regs);
  VectorInit(&g->offsets);
  VectorInit(&g->exception_ranges);
  VectorInit(&g->exception_typeinfos);

  MapInitForInt64Keys(&g->conditions);
  AARCH64RegisterAllocatorInit(&g->register_allocator, g);
}

AARCH64Generator* NewAARCH64Generator(Generator* gen) {
  AARCH64Generator* g = malloc(sizeof(AARCH64Generator));
  AARCH64GeneratorInit(g, gen);
  return g;
}

void AARCH64GeneratorDestruct(AARCH64Generator* g) {
  TargetGeneratorDestruct(&g->base);
  VectorDestructWithContents(&g->var_regs, NULL, /*free_element=*/true);
  VectorDestructWithContents(&g->saved_regs, NULL, /*free_element=*/true);
  VectorDestructWithContents(&g->offsets, NULL, /*free_element=*/true);
  VectorDestructWithContents(&g->exception_ranges, NULL,
                             /*free_element=*/true);
  VectorDestruct(&g->exception_typeinfos);
  MapDestruct(&g->conditions);
  AARCH64RegisterAllocatorDestruct(&g->register_allocator);
}

static void ResolveExceptionRanges(AARCH64Generator* g, Generator* gen) {
  for (size_t i = 0; i < gen->exception_typeinfos.length; i++) {
    VectorAppend(&g->exception_typeinfos, gen->exception_typeinfos.value.p[i]);
  }
  for (size_t i = 0; i < gen->exception_ranges.length; i++) {
    ExceptionHandlerRange* ir_range = gen->exception_ranges.value.p[i];
    TargetInstruction* try_start = ir_range->try_start->data.ptr;
    TargetInstruction* try_end = ir_range->try_end->data.ptr;
    TargetInstruction* catch_label = ir_range->catch_label->data.ptr;
    if (try_start == NULL || try_end == NULL || catch_label == NULL) {
      continue;
    }
    try_start->flags |= TARGET_INST_KEEP_UNREACHABLE;
    try_end->flags |= TARGET_INST_KEEP_UNREACHABLE;
    catch_label->flags |=
        TARGET_INST_KEEP_UNREACHABLE | TARGET_INST_EXCEPTION_LANDING;
    TargetRecordExceptionEdge(&g->base, try_start, try_end, catch_label);
    AARCH64ExceptionRange* range = malloc(sizeof(AARCH64ExceptionRange));
    range->try_start = try_start;
    range->try_end = try_end;
    range->catch_label = catch_label;
    range->catch_typeinfo = ir_range->catch_typeinfo;
    range->is_cleanup = ir_range->is_cleanup;
    VectorAppend(&g->exception_ranges, range);
  }
  for (size_t i = 0; i < gen->exception_keep_labels.length; i++) {
    IRNode* label = gen->exception_keep_labels.value.p[i];
    TargetInstruction* target_label = label->data.ptr;
    if (target_label != NULL) {
      target_label->flags |= TARGET_INST_KEEP_UNREACHABLE;
    }
  }
}

void AARCH64GeneratorDelete(AARCH64Generator* g) {
  AARCH64GeneratorDestruct(g);
  free(g);
}

static SavedArgumentRegister* NewSavedArgumentRegister(int reg_num,
                                                       int base_reg_num,
                                                       int offset,
                                                       bool is_fp) {
  SavedArgumentRegister* reg = malloc(sizeof(SavedArgumentRegister));
  reg->base_reg_num = base_reg_num;
  reg->reg_num = reg_num;
  reg->offset = offset;
  reg->is_fp = is_fp;
  return reg;
}

static COMPILER_UNUSED void SavedArgumentRegisterDelete(SavedArgumentRegister* reg) {
  free(reg);
}

// Some static utility functions that map to generic target functions.
static TargetInstruction* NewInstruction1(AARCH64Opcode opcode,
                                          TargetInstruction* op1) {
  return TargetNewInstruction1((TargetOpcode)opcode, op1);
}

static TargetInstruction* NewInstruction2(AARCH64Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  return TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
}

static COMPILER_UNUSED TargetInstruction* NewInstruction3(AARCH64Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3) {
  return TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
}

static TargetInstruction* NewInstruction4(AARCH64Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3,
                                          TargetInstruction* op4) {
  return TargetNewInstruction4((TargetOpcode)opcode, op1, op2, op3, op4);
}

static TargetInstruction* Emit(AARCH64Generator* g, TargetInstruction* inst) {
  return TargetEmit(&g->base, inst);
}

static COMPILER_UNUSED TargetInstruction* EmitBefore(AARCH64Generator* g, TargetInstruction* inst,
                                     TargetInstruction* pos) {
  return TargetEmitBefore(&g->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitAfter(AARCH64Generator* g, TargetInstruction* inst,
                                    TargetInstruction* pos) {
  return TargetEmitAfter(&g->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitConstant(AARCH64Generator* g, TargetInstruction* c) {
  return TargetEmitConstant(&g->base, c);
}

static COMPILER_UNUSED TargetInstruction* EmitSymbol(AARCH64Generator* g, TargetInstruction* c) {
  return TargetEmitSymbol(&g->base, c);
}

static COMPILER_UNUSED TargetInstruction* FramePointer(AARCH64Generator* g) {
  return SetInstructionSize(TargetFramePointer(&g->base), kSize64Bit);
}

static COMPILER_UNUSED TargetInstruction* StackPointer(AARCH64Generator* g) {
  return SetInstructionSize(TargetStackPointer(&g->base), kSize64Bit);
}

static TargetInstruction* GetLoweredNode(IRNode* node) {
  return TargetGetLoweredNode(node);
}

static TargetInstruction* SetLoweredNode(IRNode* node,
                                         TargetInstruction* inst) {
  return TargetSetLoweredNode(node, inst);
}

static TargetInstruction* GetIntConstant(AARCH64Generator* g, IRNode* node,
                                         TargetType type, int64_t value) {
  return SetInstructionSize(TargetGetIntConstant(&g->base, node, type, value),
                            type == kTargetType64Bit ? kSize64Bit : kSize32Bit);
}

static COMPILER_UNUSED TargetInstruction* GetFloatingPointConstant(AARCH64Generator* g,
                                                   IRNode* node,
                                                   TargetType type,
                                                   double value) {
  return TargetGetFloatingPointConstant(&g->base, node, type, value);
}

static TargetInstruction* GetSymbol(AARCH64Generator* g, IRNode* node,
                                    Symbol* symbol) {
  return TargetGetSymbol(&g->base, node, symbol);
}

static TargetInstruction* NewInstruction(AARCH64Opcode opcode) {
  return TargetNewInstruction((TargetOpcode)opcode);
}

static TargetInstruction* Condition(AARCH64Generator* g, AARCH64Opcode cond, int size);
static TargetInstruction* EmitBranch(AARCH64Generator* g, AARCH64Opcode cond,
                                     IRNode* target_node) {
  TargetInstruction* bra = Emit(g, NewInstruction1(AARCH64_OP(b), Condition(g, cond, 0)));
  if (target_node->data.ptr == NULL) {
    VectorAppend(&g->base.fixups, NewBranchFixup(bra, target_node, 1));
  } else {
    bra->operand[1] = target_node->data.ptr;
    TargetAddUser(target_node->data.ptr, bra);
  }
  return bra;
}

static COMPILER_UNUSED TargetInstruction* EmitLabelReference(AARCH64Generator* g,
                                             TargetInstruction* label,
                                             IRNode* target_node) {
  Emit(g, label);
  if (target_node->data.ptr == NULL) {
    VectorAppend(&g->base.fixups, NewBranchFixup(label, target_node, 1));
  } else {
    label->operand[1] = target_node->data.ptr;
    TargetAddUser(label, target_node->data.ptr);
  }
  return label;
}

static TargetInstruction* ZeroReg(AARCH64Generator* g) {
  if (g->zero == NULL) {
    g->zero = Emit(g, NewInstruction(AARCH64_OP(zr)));
  }
  return g->zero;
}

static TargetInstruction* StructReturnArgumentRegister(AARCH64Generator* g) {
  if (g->struct_return_argument_register == NULL) {
    g->struct_return_argument_register =
        Emit(g, SetInstructionSize(NewInstruction(AARCH64_OP(xr)),
                                   kSize64Bit));
  }
  return g->struct_return_argument_register;
}

static TargetInstruction* ZeroImm(AARCH64Generator* g) {
  return GetIntConstant(g, NULL, kTargetType64Bit, 0);
}

static TargetInstruction* Tmp(AARCH64Generator* g) {
  if (g->tmp == NULL) {
    g->tmp = Emit(g, NewInstruction(AARCH64_OP(r9)));
  }
  return g->tmp;
}

static TargetInstruction* lsl(AARCH64Generator* g) {
  if (g->lsl == NULL) {
    g->lsl = Emit(g, NewInstruction(AARCH64_OP(oplsl)));
  }
  return g->lsl;
}


static TargetInstruction* movi(AARCH64Generator* g, int size, TargetInstruction* src) {
  TargetInstruction* inst = Emit(g, NewInstruction1(AARCH64_OP(mov), src));
  SetInstructionSize(inst, size);
  return inst;
}

static TargetInstruction* Condition(AARCH64Generator* g, AARCH64Opcode cond, int size) {
  MapKeyType k = {.w = ((int64_t)cond << 1) | (size & 1)};
  TargetInstruction* inst = MapFind(&g->conditions, k);
  if (inst != NULL) {
    return inst;
  }
  inst =  EmitConstant(g, NewInstruction(cond));
  MapKeyValue kv = {.key = k, .value.p = inst};
  MapInsert(&g->conditions, kv);
  return inst;
}

static TargetInstruction* IntArgumentRegister(AARCH64Generator* g, int argnum) {
  if (g->int_argument_registers[argnum] == NULL) {
    // Allocate instruction for argument register and emit it.  The argument
    // registers have to be contiguous in value from x0..x7.
    g->int_argument_registers[argnum] =
        EmitSymbol(g, NewInstruction(AARCH64_OP(r0) + argnum));
  }
  return g->int_argument_registers[argnum];
}


static TargetInstruction* FloatingPointArgumentRegister(AARCH64Generator* g,
                                                        int argnum) {
  if (g->fp_argument_registers[argnum] == NULL) {
    // Allocate instruction for argument register and emit it.  The argument
    // registers have to be contiguous in value from a0..a7.
    g->fp_argument_registers[argnum] =
        EmitSymbol(g, NewInstruction(AARCH64_OP(d0) + argnum));
  }
  return g->fp_argument_registers[argnum];
}

// Outgoing call arguments must use a *fresh* argument-register pseudo for each
// call rather than the cached one returned by Int/FloatingPointArgumentRegister.
// The cached pseudos persist across basic blocks, but the register allocator
// resets each physical register's owner at every block boundary.  When a cached
// pseudo already has a physical register assigned, the destination-allocation
// path skips re-allocation and never re-establishes ownership, so inside a loop
// body the argument register looks free and a temporary (e.g. a pointer
// post-increment's reload) steals it, clobbering an argument that was already
// set up.  A fresh pseudo per call has its ownership and use-count tracked
// correctly within the block where the call lives.
static TargetInstruction* FreshIntArgumentRegister(AARCH64Generator* g,
                                                   int argnum) {
  return EmitSymbol(g, NewInstruction(AARCH64_OP(r0) + argnum));
}

static TargetInstruction* FreshFpArgumentRegister(AARCH64Generator* g,
                                                  int argnum) {
  return EmitSymbol(g, NewInstruction(AARCH64_OP(d0) + argnum));
}

static TargetInstruction* IntVariableRegister(AARCH64Generator* g, int varnum, Symbol* sym) {
  for (size_t i = 0; i < g->var_regs.length; i++) {
    RegisterVariable* var = g->var_regs.value.p[i];
    if (!var->is_fp && var->varnum == varnum) {
      return var->inst;
    }
  }
  RegisterVariable* var = malloc(sizeof(RegisterVariable));
  var->varnum = varnum;
  TargetSymbol* inst = malloc(sizeof(TargetSymbol));
  TargetInitInstruction(&inst->base, TARGET_OP(ivarreg));
  inst->symbol = sym;
  var->inst = &inst->base;
  var->is_fp = false;
  VectorAppend(&g->var_regs, var);
  return EmitSymbol(g, var->inst);
}


static TargetInstruction* FloatingPointVariableRegister(AARCH64Generator* g, int varnum, Symbol* sym) {
  for (size_t i = 0; i < g->var_regs.length; i++) {
    RegisterVariable* var = g->var_regs.value.p[i];
    if (var->is_fp && var->varnum == varnum) {
      return var->inst;
    }
  }
  RegisterVariable* var = malloc(sizeof(RegisterVariable));
  var->varnum = varnum;
  TargetSymbol* inst = malloc(sizeof(TargetSymbol));
  TargetInitInstruction(&inst->base, TARGET_OP(fvarreg));
  inst->symbol = sym;
  var->inst = &inst->base;
  var->is_fp = true;
  VectorAppend(&g->var_regs, var);
  return EmitSymbol(g, var->inst);
}

// Add immediate to the src.  If it fits in 12 bits we can use an addi
// instruction, otherwise load the immediate and use an add instruction.
static TargetInstruction* AddImmediate(AARCH64Generator* g, TargetInstruction* src,
                                       int64_t immed) {
  int64_t imm = immed;
  if (immed < 0) {
    imm = -immed;
  }
  TargetInstruction* immed_inst =
      GetIntConstant(g, NULL, kTargetType64Bit, immed);
  if (imm <= 0x7ff) {
    return Emit(g, CopyInstructionSize(NewInstruction2(AARCH64_OP(add), src, immed_inst), 0));
  }
  TargetInstruction* movi =
      Emit(g, SetInstructionSize(
                  NewInstruction1(AARCH64_OP(mov), immed_inst), kSize64Bit));
  return Emit(g, SetInstructionSize(
                     NewInstruction2(AARCH64_OP(add), src, movi), kSize64Bit));
}

static TargetInstruction* SetDestOrMove(AARCH64Generator* g,
                                        TargetInstruction* from,
                                        TargetInstruction* to,
                                        AARCH64Opcode mov_opcode) {
  bool can_set_dest = from->dest == NULL && AARCH64GeneratesOutput(from);

  if (can_set_dest) {
    TargetSetDest(from, to);
    return from;
  }
  TargetInstruction* move = Emit(g, NewInstruction1(mov_opcode, from));
  move->dest = to;
  return to;
}

static TargetInstruction* LowerExpression(AARCH64Generator* g, IRNode* node);

// If the IR node writes its result into a destination tmp (the "-> $n"
// annotation, used to merge the results of && / || / ?: into one location),
// return the lowered instruction for that destination so the caller can route
// the result there.  Returns NULL when there is no destination.
static TargetInstruction* GetDestInstruction(AARCH64Generator* g, IRNode* node) {
  if (node->dest == NULL) {
    return NULL;
  }
  if (node->dest->data.ptr == NULL) {
    LowerExpression(g, node->dest);
  }
  return GetLoweredNode(node->dest);
}

static TargetInstruction* SetDestOrMoveToArgReg(AARCH64Generator* g,
                                                IRNode* from_node,
                                                TargetInstruction* from,
                                                TargetInstruction* to,
                                                AARCH64Opcode rmov_opcode) {
  // Look at all uses of from_node and make sure they are all in the
  // same basic block as from_node itself.
  bool candidate = true;
  for (size_t i = 0; i < from_node->outputs.length; i++) {
    IRNode* user = from_node->outputs.value.p[i];
    if (user->block != from_node->block) {
      candidate = false;
      break;
    }
  }
  // Only pin the producer's result directly into the (caller-saved) argument
  // register when that result is consumed solely by this argument.  IR basic
  // blocks are not split by calls, so a value with another use later in the
  // same IR block -- after an intervening call that clobbers the argument
  // registers -- would read the wrong value.  This happens when GVN merges a
  // value feeding a call argument with one feeding a later assignment (e.g. the
  // `(float)'a'` conversion shared by `floatfunc('a')` and `float f = 'a'`).
  // The argument node is usually a `pusharg` wrapper, so look through it to the
  // value actually producing the register.  With more than one user, keep the
  // value in its own register (which the allocator can preserve) and emit an
  // explicit move into the argument register instead.
  IRNode* value_node = from_node;
  if (value_node != NULL && (int)value_node->opcode == (int)IR_OP(pusharg) &&
      value_node->inputs.length > 0) {
    value_node = value_node->inputs.value.p[0];
  }
  if (value_node != NULL && value_node->outputs.length > 1) {
    candidate = false;
  }
  // Only redirect the producer's destination straight into the argument
  // register when the producer is the most recently emitted instruction.
  // Arguments are moved into their registers in reverse order, so a preceding
  // argument's value (e.g. a call result still living in x0) may already have
  // been moved out by an instruction emitted *after* this producer.  Giving an
  // older producer an argument register as its destination would place that
  // (clobbering) write before the move that consumes the register's previous
  // value; emit an explicit move at the current position instead.
  if (candidate && TargetNext(from) == NULL) {
    return SetDestOrMove(g, from, to, rmov_opcode);
  }
  if (rmov_opcode == AARCH64_OP(fmov)) {
    // fmov has no register-move (rmov) form in the emitter, so the two-operand
    // encoding would be mis-assembled (it picks up only the first two
    // registers, reversing the move).  Emit it as a destination-move into the
    // argument register, copying its size from the source value.
    TargetInstruction* move =
        Emit(g, CopyInstructionSize(NewInstruction1(rmov_opcode, from), 0));
    move->dest = to;
    return to;
  }
  Emit(g, NewInstruction2(rmov_opcode, to, from));
  return to;
}

static TargetInstruction* AddValue(AARCH64Generator* g, TargetInstruction* src,
                                   TargetInstruction* value) {
  if (TargetIsConst(value)) {
    return AddImmediate(g, src, TargetIntValue(value));
  }

  if (((int)value->opcode == (int)AARCH64_OP(zr))) {
    return src;
  }
  return Emit(g, NewInstruction2(AARCH64_OP(add), src, value));
}

// Calculate the offset from the frame pointer to a local variable in the stack.
static int LocalVariableOffset(AARCH64Generator* g, int32_t var_offset) {
  return var_offset - g->base.stack_frame_size -
      AARCH64_STACK_FRAME_HEADER_SIZE;
}

static TargetInstruction* PagedOffsetFrom(AARCH64Generator* g, TargetInstruction* src,
                                          int32_t offset, int32_t* page_offset) {
  // Offset is not in range.  Need to calculate an offset in a register.
  //
  // We calculate a page offset.  The addi instruction
  // has a 12 bit signed immediate that can be added to an offset
  // calculated from the src.
  int page;
  if (offset < 0) {
    page = -(-offset & ~0x7ff);
  } else {
    page = offset & ~0x7ff;
  }
  TargetInstruction* page_inst = NULL;
  for (size_t i = 0; i < g->offsets.length; i++) {
    Offset* f = g->offsets.value.p[i];
    if (f->page_offset == page) {
      page_inst = f->inst;
      break;
    }
  }
  if (page_inst == NULL) {
    // No page offset calculated, need to calculate one.
    page_inst =
        AddImmediate(g, src, page);
    Offset* f = malloc(sizeof(Offset));
    f->inst = page_inst;
    f->page_offset = page;
    VectorAppend(&g->offsets, f);
  }
  *page_offset = offset - page;
  return page_inst;
}

// Returns either an integer constant or an instruction to calculate an
// offset from the src.
static TargetInstruction* OffsetFrom(AARCH64Generator* g, TargetInstruction* src,
                                     int32_t offset) {
  bool offset_in_range = AARCH64IsPossibleImmediate(offset);
  if (offset_in_range) {
    return AddImmediate(g, src, offset);
  }
  int page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(g, src, offset, &page_offset);
  if (page_offset == 0) {
    return page_inst;
  }
  return AddImmediate(g, page_inst, page_offset);
}

static TargetInstruction* LoadImmediate(AARCH64Generator* g, AARCH64Opcode opcode,
                                          TargetInstruction* base, int32_t offset) {
  if (AARCH64LoadStoreImmInRange(offset)) {
    return Emit(g, NewInstruction2(opcode, base,
                                    GetIntConstant(g, NULL, kTargetType32Bit, offset)));
  }
  // The offset does not fit the load's 9-bit immediate; fold it into the base
  // address and load at offset 0.
  TargetInstruction* addr = OffsetFrom(g, base, offset);
  return Emit(g, NewInstruction2(opcode, addr,
                                  GetIntConstant(g, NULL, kTargetType32Bit, 0)));
}

static TargetInstruction* StoreImmediate(AARCH64Generator* g, AARCH64Opcode opcode,
                                         TargetInstruction* value, TargetInstruction* base, int32_t offset) {
  int size = opcode == AARCH64_OP(strb) || opcode == AARCH64_OP(strh) ||
                     opcode == AARCH64_OP(sturb) || opcode == AARCH64_OP(sturh)
                 ? kSize32Bit
                 : kSize64Bit;
  if (AARCH64LoadStoreImmInRange(offset)) {
    return Emit(g, SetInstructionSize(
                       NewInstruction3(
                           opcode, value, base,
                           GetIntConstant(g, NULL, kTargetType32Bit, offset)),
                       size));
  }
  // The offset does not fit the store's 9-bit immediate; fold it into the base
  // address and store at offset 0.
  TargetInstruction* addr = OffsetFrom(g, base, offset);
  return Emit(g, SetInstructionSize(
                     NewInstruction3(
                         opcode, value, addr,
                         GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                     size));
}

static TargetInstruction* Memcpy(AARCH64Generator* g, TargetInstruction* dest_addr,
                                 TargetInstruction* src_addr, int length,
                                 int src_offset, int dest_offset, bool count_as_call) {
  if (length <= 40) {
    // Length is short, copy using sequence of ld/sd and lb/sb instructions.
    int num_bytes = length & 7;
    int num_words = length >> 3;
    TargetInstruction* result = NULL;
    for (int i = 0; i < num_words; i++, src_offset += 8, dest_offset += 8) {
      TargetInstruction* load = LoadImmediate(g, AARCH64_OP(ldr), src_addr, src_offset);
      result = StoreImmediate(g, AARCH64_OP(str), load, dest_addr, dest_offset);
    }
    for (int i = 0; i < num_bytes; i++, src_offset += 1, dest_offset += 1) {
      TargetInstruction* load = LoadImmediate(g, AARCH64_OP(ldrb), src_addr, src_offset);
      result = StoreImmediate(g, AARCH64_OP(strb), load, dest_addr, dest_offset);
    }
    if (count_as_call) {
      g->base.num_calls--;
    }
    return result;
  }
  // TODO: generate a loop for intermediate lengths?

  // Length in a2.
  TargetInstruction* size = Emit(
      g, NewInstruction1(AARCH64_OP(mov),
                          GetIntConstant(g, NULL, kTargetType32Bit, length)));
  TargetInstruction* arg2 = SetDestOrMove(g, size, IntArgumentRegister(g, 2),
                                           AARCH64_OP(mov));
  // Source in a1.
  if (src_offset != 0) {
    src_addr = OffsetFrom(g, src_addr, src_offset);
  }
  TargetInstruction* arg1 = SetDestOrMove(g, src_addr, IntArgumentRegister(g, 1),
                                           AARCH64_OP(mov));

  // Dest in a0.
  if (dest_offset != 0) {
    dest_addr = OffsetFrom(g, dest_addr, dest_offset);
  }
  TargetInstruction* arg0 = SetDestOrMove(g, dest_addr, IntArgumentRegister(g, 0),
                                          AARCH64_OP(mov));

  // We need to keep the arguments alive until the point of the call.  This
  // is done using a AARCH64_OP(regarg) instruction sequence.  See BuildArgList for
  // details on the regarg instruction.
  TargetInstruction* regarg =
      Emit(g, NewInstruction2(AARCH64_OP(regarg), NULL, arg2));
  regarg = Emit(g, NewInstruction2(AARCH64_OP(regarg), regarg, arg1));
  regarg = Emit(g, NewInstruction2(AARCH64_OP(regarg), regarg, arg0));

  TargetInstruction* memcpy = GetSymbol(g, NULL, g->base.memcpy);
  return Emit(g, NewInstruction2(AARCH64_OP(bl), memcpy, regarg));
}

static TargetInstruction* Memzero(AARCH64Generator* g, TargetInstruction* dest_addr,
                                  int length, int offset) {
  if (length <= 40) {
    // Length is short, copy using sequence of ld/sd and lb/sb instructions.
    int num_bytes = length & 7;
    int num_words = length >> 3;
    TargetInstruction* result = NULL;
    for (int i = 0; i < num_words; i++, offset += 8) {
      result = StoreImmediate(g, AARCH64_OP(str), ZeroReg(g), dest_addr, offset);
    }
    for (int i = 0; i < num_bytes; i++, offset += 1) {
      result = StoreImmediate(g, AARCH64_OP(strb), ZeroReg(g), dest_addr, offset);
    }
    g->base.num_calls--;
    return result;
  }

  // Third parameter to memset is the length.
  TargetInstruction* size = Emit(
      g, NewInstruction1(AARCH64_OP(mov),
                          GetIntConstant(g, NULL, kTargetType32Bit, length)));
  TargetInstruction* arg2 = SetDestOrMove(g, size, IntArgumentRegister(g, 2), AARCH64_OP(mov));

  // Second arg is zero.
  TargetInstruction* arg1 = Emit(
      g, NewInstruction1(AARCH64_OP(mov), ZeroReg(g)));
  arg1->dest = IntArgumentRegister(g, 1);

  // First arg is the address.  The short (inline-store) path above threads the
  // variable's frame offset into each store; the memset path must likewise add
  // the offset to the base address, otherwise memset clears memory at the bare
  // frame pointer instead of at the variable (corrupting the saved frame
  // record / caller's frame).
  if (offset != 0) {
    dest_addr = OffsetFrom(g, dest_addr, offset);
  }
  TargetInstruction* arg0 = SetDestOrMove(g, dest_addr, IntArgumentRegister(g, 0), AARCH64_OP(mov));

  // We need to keep the arguments alive until the point of the call.  This
  // is done using a AARCH64_OP(regarg) instruction sequence.  See BuildArgList for
  // details on the regarg instruction.
  TargetInstruction* regarg =
      Emit(g, NewInstruction2(AARCH64_OP(regarg), NULL, arg2));
  regarg = Emit(g, NewInstruction2(AARCH64_OP(regarg), regarg, arg1));
  regarg = Emit(g, NewInstruction2(AARCH64_OP(regarg), regarg, arg0));

  TargetInstruction* memset = GetSymbol(g, NULL, g->base.memset);
  return Emit(g, NewInstruction2(AARCH64_OP(bl), memset, regarg));
}

static AARCH64Opcode IR2RV(IROpcode op, bool is_unsigned) {
  switch (op) {
    case IR_OP(addi):
      return AARCH64_OP(add);
    case IR_OP(addf):
      return AARCH64_OP(fadd);
    case IR_OP(addd):
      return AARCH64_OP(fadd);
    case IR_OP(adda):
      return AARCH64_OP(add);

    case IR_OP(subi):
      return AARCH64_OP(sub);
    case IR_OP(subf):
      return AARCH64_OP(fsub);
    case IR_OP(subd):
      return AARCH64_OP(fsub);
    case IR_OP(suba):
      return AARCH64_OP(sub);

    case IR_OP(muli):
      return AARCH64_OP(mul);
    case IR_OP(mulf):
      return AARCH64_OP(fmul);
    case IR_OP(muld):
      return AARCH64_OP(fmul);

    case IR_OP(divi):
      return is_unsigned ? AARCH64_OP(udiv) : AARCH64_OP(sdiv);
    case IR_OP(divf):
      return AARCH64_OP(fdiv);
    case IR_OP(divd):
      return AARCH64_OP(fdiv);

    case IR_OP(modi):
      return is_unsigned ? AARCH64_OP(umod) : AARCH64_OP(smod);

    case IR_OP(lsri):
      return AARCH64_OP(lsr);
    case IR_OP(asri):
      return AARCH64_OP(asr);
    case IR_OP(lsli):
      return AARCH64_OP(lsl);
    case IR_OP(rotli):
    case IR_OP(rotri):
      return AARCH64_OP(ror);
    case IR_OP(clzi):
    case IR_OP(ctzi):
      return AARCH64_OP(clz);

    case IR_OP(ori):
      return AARCH64_OP(orr);
    case IR_OP(andi):
      return AARCH64_OP(and);
    case IR_OP(xori):
      return AARCH64_OP(eor);

    case IR_OP(noti):
      return AARCH64_OP(mvn);
    case IR_OP(nota):
      return AARCH64_OP(mvn);
    case IR_OP(onescomp):
      return AARCH64_OP(mvn);
    case IR_OP(negi):
      return AARCH64_OP(neg);
    case IR_OP(negf):
      return AARCH64_OP(fneg);
    case IR_OP(negd):
      return AARCH64_OP(fneg);

    case IR_OP(i2f):
      return AARCH64_OP(scvtf);
    case IR_OP(i2d):
      return AARCH64_OP(scvtf);
    case IR_OP(f2d):
      return AARCH64_OP(fcvtsd);
    case IR_OP(d2f):
      return AARCH64_OP(fcvtds);
    case IR_OP(f2i):
      return is_unsigned ? AARCH64_OP(fcvtzu) : AARCH64_OP(fcvtzs);
    case IR_OP(d2i):
      return is_unsigned ? AARCH64_OP(fcvtzu) : AARCH64_OP(fcvtzs);

    case IR_OP(movi):
      return AARCH64_OP(mov);
    case IR_OP(movf):
      return AARCH64_OP(fmov);
    case IR_OP(movd):
      return AARCH64_OP(fmov);
    case IR_OP(mova):
      return AARCH64_OP(mov);
    case IR_OP(tmp):
      return AARCH64_OP(tmp);
    default:
      assert(false);
      return 0;
  }
}

static bool UseRegisterForVariable(AARCH64Generator* g, IRNode* var_node) {
  if (OptLevel0()) {
    // When not optimizing, all variables are on the stack.
    return false;
  }
  // A normal object whose address is taken needs stable stack storage.  A
  // reference is already represented by an address, however, so taking the
  // address of its referent only reads that pointer value and does not require
  // a home slot for the reference itself.
  IRVariable* var = (IRVariable*)var_node;
  if (TypeIsVolatile(var->symbol->type)) {
    return false;
  }
  if (var->symbol->flags.address_taken &&
      !TypeIsReference(var->symbol->type)) {
    return false;
  }

  // No references?  No point in putting it in a register.
  if (var->base.outputs.length == 0) {
    return false;
  }
  return true;
}

// Static varaibles have an address calculated by the linker so at this
// point they are unknown.  We need to load their address into a register.  This
// is done using a la or lla pseudo-instruction.
static TargetInstruction* LoadStaticVariableAddress(AARCH64Generator* g,
                                                    IRNode* node) {
  IRVariable* var = (IRVariable*)node;
  // Do not reuse node->data.ptr here. A static variable can also flow through
  // argument lowering, which may redirect that lowered value to an argument
  // register. The relocation must always name the variable's symbol.
  TargetInstruction* sym = GetSymbol(g, NULL, var->symbol);

  // A symbol that another image can define has no address the code can compute:
  // position-independent code has to read it out of the GOT, which is the only
  // place the loader can write it.  Computing it from the PC instead resolves to
  // whatever this image reserved for the name, which for an imported object is
  // nothing at all, and the program reads 0.
  bool externally_visible = !var->symbol->flags.is_local &&
                            !StorageIs(var->symbol->storage, STO(static));
  if (compiler->pic && externally_visible) {
    return Emit(g, SetInstructionSize(
                       NewInstruction1(AARCH64_OP(gotaddr), sym), kSize64Bit));
  }

  // Otherwise the address is materialized with an adrp/add pair.  The linker
  // places the code and data segments far apart (well beyond adr's +/-1MB
  // range), so a single PC-relative adr cannot reach data symbols.  adrp
  // computes the 4KB page (R_AARCH64_ADR_PREL_PG_HI21, +/-4GB range) and the
  // add fills in the low 12 bits (R_AARCH64_ADD_ABS_LO12_NC).
  TargetInstruction* page =
      Emit(g, SetInstructionSize(NewInstruction1(AARCH64_OP(adrp), sym),
                                 kSize64Bit));
  TargetInstruction* addr =
      Emit(g, SetInstructionSize(NewInstruction2(AARCH64_OP(add), page, sym),
                                 kSize64Bit));
  addr->flags |= AARCH64_LO_RELOC;
  return addr;
}

static struct {
  bool (*type_func)(TypeRecord*);
  AARCH64Opcode load;
} load_opcodes[] = {
    {TypeIsInt, AARCH64_OP(ldr)},
    {TypeIsShort, AARCH64_OP(ldrh)},
    {TypeIsChar8, AARCH64_OP(ldurb)},
    {TypeIsChar, AARCH64_OP(ldrb)},
    {TypeIsLong, AARCH64_OP(ldr)},
    {TypeIsLongLong, AARCH64_OP(ldr)},
    {TypeIsUnsignedInt, AARCH64_OP(ldur)},
    {TypeIsUnsignedShort, AARCH64_OP(ldurh)},
    {TypeIsUnsignedChar, AARCH64_OP(ldurb)},
    {TypeUsesFloat32Representation, AARCH64_OP(fldr)},
    {TypeUsesFloat64Representation, AARCH64_OP(fldr)},
    {TypeIsBool, AARCH64_OP(ldrb)},
    {TypeIsPointerOrArray, AARCH64_OP(ldr)},
    {TypeIsFunction, AARCH64_OP(ldr)},
    {NULL, 0},
};

static COMPILER_UNUSED TargetInstruction* LoadVariableValue(AARCH64Generator* g, IRNode* node,
                                            TargetInstruction* addr,
                                            TargetInstruction* offset) {
  AARCH64Opcode opcode = AARCH64_OP(ldr);
  for (size_t i = 0; load_opcodes[i].type_func != NULL; i++) {
    if (load_opcodes[i].type_func(node->type)) {
      opcode = load_opcodes[i].load;
      break;
    }
  }
  assert(opcode != 0);
  return Emit(g, NewInstruction2(opcode, addr, offset));
}

static TargetInstruction* MoveImmediate(AARCH64Generator* g, TargetInstruction* src, int size) {
  uint64_t value = TargetIntValue(src);
  return movi(g, size, GetIntConstant(g, NULL,
                                                     kTargetType64Bit, value));
}

static TargetInstruction* GetTlsVariableAddress(AARCH64Generator* g,
                                                IRNode* node) {
  if (compiler->tls_model != TLS(local_exec)) {
    fprintf(stderr,
            "error: AArch64 only supports the local-exec TLS model for static "
            "executables\n");
    abort();
  }

  IRVariable* variable = (IRVariable*)node;
  TargetInstruction* symbol = GetSymbol(g, node, variable->symbol);
  // TPIDR_EL0 is read into an ordinary allocatable register.  Do not reuse a
  // function-wide cached value here: later code may legally clobber that
  // register before another TLS access (for example, strtoll's second errno
  // assignment on its overflow path).
  TargetInstruction* thread_pointer =
      Emit(g, NewInstruction(AARCH64_OP(tp)));
  TargetInstruction* high =
      Emit(g, NewInstruction2(AARCH64_OP(add), thread_pointer, symbol));
  high->flags |= AARCH64_TPREL_HI_RELOC;
  TargetInstruction* low =
      Emit(g, NewInstruction2(AARCH64_OP(add), high, symbol));
  low->flags |= AARCH64_TPREL_LO_RELOC;
  return low;
}

// Materialize a value into a register.  This loads a constant into a register
// or returns the instruction associated with the node if it's
// already in a register.
static TargetInstruction* Materialize1(AARCH64Generator* g, IRNode* node) {
  if (IRIsThreadVariable(node)) {
    return GetTlsVariableAddress(g, node);
  }

  if (IRIsConst(node)) {
    switch (node->opcode) {
      case IR_OP(const8):
      case IR_OP(const16):
      case IR_OP(const32):
         if (IRIsZero(node)) {
          return ZeroReg(g);
        } else {
          return MoveImmediate(g, GetLoweredNode(node), kSize32Bit);
        }
        
      case IR_OP(const64):
      case IR_OP(consta):
        if (IRIsZero(node)) {
         return ZeroReg(g);
       } else {
         return MoveImmediate(g, GetLoweredNode(node), kSize64Bit);
       }
        
      case IR_OP(constf): {
        // The constant's fvalue field is always in double precision; round it
        // to single precision and materialize the 32-bit single bit pattern in
        // a general register, then bit-cast it into a single (s) FP register.
        double dvalue = ((IRConstant*)node)->value.fvalue;
        float fvalue = (float)dvalue;
        uint32_t bits;
        memcpy(&bits, &fvalue, sizeof(bits));
        TargetInstruction* c;
        if (bits == 0) {
          c = ZeroReg(g);
        } else {
          c = MoveImmediate(g, GetIntConstant(g, NULL, kTargetType32Bit, (int64_t)bits), kSize32Bit);
        }
        // fmov Sd, Wn reinterprets the integer bit pattern as a single; fcvt
        // is an FP-to-FP precision conversion and cannot take a general
        // register source.
        return Emit(g, SetInstructionSize(NewInstruction1(AARCH64_OP(fmov), c),
                                          kSize32Bit));
      }
      case IR_OP(constd): {
        double value = ((IRConstant*)node)->value.fvalue;
        int64_t bits = *(int64_t*)(&value);
        TargetInstruction* c;
        if (bits == 0) {
          c = ZeroReg(g);
        } else {
          c = MoveImmediate(g, GetIntConstant(g, NULL, kTargetType64Bit, bits), kSize64Bit);

        }
        // fmov Dd, Xn reinterprets the integer bit pattern as a double; fcvt
        // is an FP-to-FP precision conversion and cannot take a general
        // register source.
        return Emit(g, SetInstructionSize(NewInstruction1(AARCH64_OP(fmov), c),
                                          kSize64Bit));
      }
      default:
        assert(false);
    }
  }
  if (IRIsAutoVariable(node)) {
    IRVariable* var = (IRVariable*)node;
    if (TypeIsVLA(node->type)) {
      IRNode* addr = var->symbol->value.other;
      return GetLoweredNode(addr);
    } else {
      int32_t var_offset = node->data.ivalue;
      if (AARCH64_IS_REG_VAR(var_offset)) {
        // Variable is in a register.
        int var_num = var_offset & ~AARCH64_REG_VAR;
        if (TypeIsFloatingPoint(node->type)) {
          return FloatingPointVariableRegister(g, var_num, var->symbol);
        } else {
          return IntVariableRegister(g, var_num, var->symbol);
        }
      }

      // Auto variable is in the stack frame.  These are accessed through
      // the frame pointer with a negative offset.
      TargetInstruction* addr = FramePointer(g);
      return OffsetFrom(g, addr, LocalVariableOffset(g, var_offset));
    }
  } else if (IRIsArgument(node)) {
    int32_t var_offset = node->data.ivalue;
    // A struct/union larger than 8 bytes was passed by reference: the register
    // or frame slot holds a pointer to the caller's copy, which *is* the
    // address of the parameter.
    bool by_ref_struct =
        TypePassedAsAArch64Aggregate(node->type) && node->type->size > 8;
    if (AARCH64_IS_REG_VAR(var_offset)) {
      // Argument is in a register.
      IRVariable* var = (IRVariable*)node;
      int var_num = var_offset & ~AARCH64_REG_VAR;
      if (TypeIsFloatingPoint(node->type)) {
        return FloatingPointVariableRegister(g, var_num, var->symbol);
      } else {
        // For a by-reference struct the register already holds the pointer.
        return IntVariableRegister(g, var_num, var->symbol);
      }
    } else if (by_ref_struct) {
      // Load the pointer from the frame slot; that pointer is the address.
      TargetInstruction* slot = OffsetFrom(g, FramePointer(g), var_offset);
      return Emit(g, SetInstructionSize(
                         NewInstruction2(AARCH64_OP(ldr), slot, ZeroImm(g)),
                         kSize64Bit));
    } else {
      // Argument is on the stack.
      TargetInstruction* addr = FramePointer(g);
      return OffsetFrom(g, addr, var_offset);
    }
  } else if (IRIsStaticVariable(node)) {
    // The address of static variables need to be moved into a register.

    return LoadStaticVariableAddress(g, node);
  }

  // Node is not a variable, is must be an already-lowered expression.
  return GetLoweredNode(node);
}

// True when Materialize1 returns the *address* of a memory-resident object
// rather than a value held in a register.  Such an address is always a 64-bit
// pointer, so its width must not be reduced to the (possibly narrower) object
// type by CopyOrSetInstructionSize.
static bool MaterializesToAddress(IRNode* node) {
  if (IRIsThreadVariable(node)) {
    return true;
  }

  if (IRIsStaticVariable(node)) {
    return true;
  }
  if (IRIsAutoVariable(node) || IRIsArgument(node)) {
    if (TypeIsVLA(node->type)) {
      return true;
    }
    int32_t var_offset = node->data.ivalue;
    return !AARCH64_IS_REG_VAR(var_offset);
  }
  return false;
}

static TargetInstruction* Materialize(AARCH64Generator* g, IRNode* node) {
  TargetInstruction* inst = Materialize1(g, node);
  if (MaterializesToAddress(node)) {
    // The instruction already carries the correct 64-bit pointer width.
    return inst;
  }
  if (node->opcode == IR_OP(signextendi)) {
    // LowerSignExtend has already sized its result (sxtw / shift pair) to the
    // width required by the operation, which for a narrowing sign-extend is the
    // *source* width rather than the node's narrower result type.  Resizing it
    // here would emit an out-of-range shift (e.g. asr w, w, #32).
    return inst;
  }
  if (node->opcode == IR_OP(cast)) {
    // A cast lowers to the (already correctly sized) instruction of its input;
    // it shares that instruction, so resizing it here to the cast's own type
    // would corrupt the producer (e.g. shrinking a 64-bit shift used by an
    // enclosing widening sign-extend down to a 32-bit, out-of-range shift).
    return inst;
  }
  if (IRIsLoadOnly(node)) {
    // A load instruction is already sized correctly by LowerLoad according to
    // the loaded type (e.g. load32 -> 32-bit, load64/loada -> 64-bit).  Running
    // CopyOrSetInstructionSize here would instead copy the size of the load's
    // address operand (always a 64-bit pointer), widening a narrow load to 64
    // bits.  That makes the load read 8 bytes from a 4-byte object, so when the
    // value is then used as an address offset (e.g. mul-by-1 index scaling for
    // a char array) the garbage high bits corrupt the computed address.
    return inst;
  }
  return CopyOrSetInstructionSize(node, inst);
}

static void ApplyFixups(AARCH64Generator* g, IRNode* label_node) {
  TargetApplyFixups(&g->base, label_node);
}

static bool IsPowerOf2(int64_t v) {
  return v != 0 && (v & (v - 1)) == 0;
}

// Given a number that is a power of 2, what is the log (base 2) of it.
static int64_t Log2(int64_t v) {
  static const int MultiplyDeBruijnBitPosition2[32] =
  {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };
  return MultiplyDeBruijnBitPosition2[(uint32_t)(v * 0x077CB531U) >> 27];
}

// Count the number of 1 bits in the integer up to maxbits in length.
static int PopulationCount(uint64_t x) {
  int c = 0;
  for (; x != 0; x &= x - 1) {
      c++;
  }
  return c;
}

// It's worth multiplying by a constant using shifts and adds
// if the number of shift and add instructions is less than the number of
// cycles it takes for the mul instruction.
//
// Since this is a 64 bit processor, that's
// 8 cycles to multiply two 64 bit numbers.  Each 1-bit in the constant
// causes the emission of a shift instruction and these need to be added
// together.  Therefore the number of instructions for a n bits is
// n + (n - 1) = 2n-1.  However for bit 0 we don't do the shift but instead
// use the input value directly.
//
// Let's assume that both a slli and an add instruction take 1 cycle.
static TargetInstruction* MultiplyByConstant(AARCH64Generator* g,
                               IRNode* variable,
                               IRConstant* constant) {
  int64_t value = constant->value.ivalue;
  int numbits = PopulationCount(value);
  // The register allocator may coalesce a shifted term with the original
  // input. A second shifted term then has to reload that input and can clobber
  // the first term before the add. Keep only powers of two and the safe
  // input-plus-one-shift form; use mul when both set bits require shifts.
  if (numbits > 2 || (numbits == 2 && (value & 1) == 0)) {
    return NULL;
  }
  int num_cycles = numbits * 2 - 1;
  if ((value & 1) == 1) {
    // If the bottom bit is 1 we can subtract one instruction.
    num_cycles--;
  }
  const int kMaxCycles = 8;
  if (num_cycles > kMaxCycles) {
    return NULL;
  }
 
  TargetInstruction* left = NULL;   // Current left instruction.
  TargetInstruction* right = NULL;  // Current right instruction.
  TargetInstruction* input = GetLoweredNode(variable);
  
  // TODO: use AARCH64's shifted register instructions to make this more efficient.
  int bitpos = 0;
  while (value != 0) {
    if ((value & 1) == 1) {
      // Build a shift by the bitpos.
      TargetInstruction* inst;
      if (bitpos == 0) {
        inst = input;     // Bit 0, use input directly.
      } else {
        // Shift left by the bitpos.
       inst =
           Emit(g, CopyInstructionSize(NewInstruction2(AARCH64_OP(lsl), input,
                           GetIntConstant(g, NULL, kTargetType32Bit, bitpos)), 0));
      }
      if (left == NULL) {
        left = inst;
      } else if (right == NULL) {
        right = inst;
      }
      if (left != NULL && right != NULL) {
        // Add left and right together.
        inst = Emit(g, CopyInstructionSize(NewInstruction2(AARCH64_OP(add), left, right), 0));
        
        // Left is now the result of the add.  Right is empty.
        left = inst;
        right = NULL;
      }
    }
    bitpos++;
    value >>= 1;
  }
  return left;
}

static TargetInstruction* LowerModulo(AARCH64Generator* g, IRNode* node,
                                      AARCH64Opcode div_opcode) {
  IRNode* op1 = node->inputs.value.p[0];
  IRNode* op2 = node->inputs.value.p[1];
  TargetInstruction* lhs = Materialize(g, op1);
  TargetInstruction* rhs = Materialize(g, op2);
  TargetInstruction* quotient_tmp = Emit(g, NewInstruction(AARCH64_OP(tmp)));
  TargetInstruction* divide = Emit(
      g, CopyInstructionSize(NewInstruction2(div_opcode, lhs, rhs), 0));
  divide->dest = quotient_tmp;
  return NewInstruction3(AARCH64_OP(msub), quotient_tmp, rhs, lhs);
}


static TargetInstruction* LowerExpression(AARCH64Generator* g, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }

  // Logical NOT (!x).  This is distinct from one's-complement (~x): the result
  // is 1 when the operand is zero and 0 otherwise.  Materialize it as
  // "cmp x, #0 ; cset rd, eq" rather than the bitwise mvn used by onescomp.
  if (node->opcode == IR_OP(noti) || node->opcode == IR_OP(nota)) {
    IRNode* op = node->inputs.value.p[0];
    int compare_size = (op->type->size > 4) ? kSize64Bit : kSize32Bit;
    int result_size = (node->type->size > 4) ? kSize64Bit : kSize32Bit;
    Emit(g, SetInstructionSize(
                NewInstruction2(AARCH64_OP(cmp), Materialize(g, op),
                                GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                compare_size));
    TargetInstruction* result = SetInstructionSize(
        NewInstruction1(AARCH64_OP(cset),
                        Condition(g, AARCH64_OP(eq), result_size)),
        result_size);
    result->flags |= kAARCH64ComparisonGenerated;
    Emit(g, result);
    TargetInstruction* dest = GetDestInstruction(g, node);
    if (dest != NULL) {
      result = SetDestOrMove(g, result, dest, AARCH64_OP(mov));
    }
    return SetLoweredNode(node, result);
  }

  if (node->opcode == IR_OP(rotli) || node->opcode == IR_OP(rotri) ||
      node->opcode == IR_OP(clzi) || node->opcode == IR_OP(ctzi)) {
    IRNode* input = node->inputs.value.p[0];
    int size = ((node->flags & kIRBitWidth64) != 0 ||
                (node->type != NULL && node->type->size > 4))
                   ? kSize64Bit
                   : kSize32Bit;
    TargetInstruction* source = Materialize(g, input);
    TargetInstruction* result;
    if (node->opcode == IR_OP(clzi)) {
      result = SetInstructionSize(NewInstruction1(AARCH64_OP(clz), source), size);
    } else if (node->opcode == IR_OP(ctzi)) {
      TargetInstruction* reversed = Emit(
          g, SetInstructionSize(NewInstruction1(AARCH64_OP(rbit), source), size));
      result =
          SetInstructionSize(NewInstruction1(AARCH64_OP(clz), reversed), size);
    } else {
      TargetInstruction* amount = Materialize(g, node->inputs.value.p[1]);
      if (node->opcode == IR_OP(rotli)) {
        amount = Emit(g, SetInstructionSize(
                             NewInstruction1(AARCH64_OP(neg), amount), size));
      }
      result = SetInstructionSize(
          NewInstruction2(AARCH64_OP(rorv), source, amount), size);
    }
    Emit(g, result);
    TargetInstruction* dest = GetDestInstruction(g, node);
    if (dest != NULL) {
      result = SetDestOrMove(g, result, dest, AARCH64_OP(mov));
    }
    return SetLoweredNode(node, result);
  }

  AARCH64Opcode opcode = IR2RV(node->opcode, TypeIsUnsigned(node->type));
  assert(node->inputs.length <= 2);
  TargetInstruction* inst = NULL;
  bool ref_counts_ok =
      false;  // True if we don't need to update operand ref counts.
  
  // Do some strength reduction if we can.
  switch (opcode) {
    default:
      // All others are handled below.
      break;
    case AARCH64_OP(add): {
      // We have an add with constant instruction.  Use it if we can.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) && IRIsConst(op2)) {
        uint64_t value =
            (uint64_t)((IRConstant*)op1)->value.ivalue +
            (uint64_t)((IRConstant*)op2)->value.ivalue;
        inst = NewInstruction1(
            AARCH64_OP(mov),
            GetIntConstant(g, NULL,
                           (node->opcode == IR_OP(adda) ||
                            node->opcode == IR_OP(suba) ||
                            (node->type != NULL && node->type->size > 4))
                               ? kTargetType64Bit
                               : kTargetType32Bit,
                           (int64_t)value));
        break;
      }
      // Adds are commutative so we can have a const as first or
      // second operand.
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (AARCH64IsPossibleImmediate(c)) {
          TargetInstruction* lhs = Materialize(g, op1);
          if ((AARCH64Opcode)(int)lhs->opcode == AARCH64_OP(zr)) {
            inst = NewInstruction1(AARCH64_OP(mov), GetLoweredNode(op2));
          } else {
            inst = (TargetInstruction*)NewInstruction(AARCH64_OP(add));
            inst->operand[0] = lhs;
            inst->operand[1] = GetLoweredNode(op2);
          }
        }
      } else if (IRIsConst(op1)) {
        int64_t c = ((IRConstant*)op1)->value.ivalue;
        if (AARCH64IsPossibleImmediate(c)) {
          TargetInstruction* rhs = Materialize(g, op2);
          if ((AARCH64Opcode)(int)rhs->opcode == AARCH64_OP(zr)) {
            inst = NewInstruction1(AARCH64_OP(mov), GetLoweredNode(op1));
          } else {
            inst = (TargetInstruction*)NewInstruction(AARCH64_OP(add));
            inst->operand[0] = rhs;
            inst->operand[1] = GetLoweredNode(op1);
          }
        }
      }
      break;
    }

    case AARCH64_OP(sub): {
      // A subtract immediate can be done using an addi with the negative of the
      // immediate.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) && IRIsConst(op2)) {
        uint64_t value =
            (uint64_t)((IRConstant*)op1)->value.ivalue -
            (uint64_t)((IRConstant*)op2)->value.ivalue;
        inst = NewInstruction1(
            AARCH64_OP(mov),
            GetIntConstant(g, NULL,
                           (node->opcode == IR_OP(adda) ||
                            node->opcode == IR_OP(suba) ||
                            (node->type != NULL && node->type->size > 4))
                               ? kTargetType64Bit
                               : kTargetType32Bit,
                           (int64_t)value));
      } else if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (AARCH64IsPossibleImmediate(c)) {
          TargetInstruction* lhs = Materialize(g, op1);
          if ((AARCH64Opcode)(int)lhs->opcode == AARCH64_OP(zr)) {
            inst = NewInstruction1(
                AARCH64_OP(mov),
                GetIntConstant(g, NULL,
                               (node->opcode == IR_OP(adda) ||
                                node->opcode == IR_OP(suba) ||
                                (node->type != NULL && node->type->size > 4))
                                   ? kTargetType64Bit
                                   : kTargetType32Bit,
                               (int64_t)(0 - (uint64_t)c)));
          } else {
            inst = (TargetInstruction*)NewInstruction(AARCH64_OP(sub));
            inst->operand[0] = lhs;
            inst->operand[1] =
                GetIntConstant(g, NULL, kTargetType32Bit, c);
          }
        }
      }
    }
      break;
    case AARCH64_OP(lsl):
    case AARCH64_OP(lsr):
    case AARCH64_OP(asr): {
      // There are constant shift operations.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: what about a shift out of range?
        if (c == 0) {
          // A shift of 0 is a mv.
          inst = NewInstruction(AARCH64_OP(mov));
          inst->operand[0] = Materialize(g, op1);
        } else {
          switch (opcode) {
            case AARCH64_OP(lsl):
              opcode = AARCH64_OP(lsl);
              break;
            case AARCH64_OP(lsr):
              opcode = AARCH64_OP(lsr);
              break;
            case AARCH64_OP(asr):
              opcode = AARCH64_OP(asr);
              break;
            default:
              break;
          }
          inst = (TargetInstruction*)NewInstruction(opcode);
          inst->operand[0] = Materialize(g, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      } else {
        AARCH64Opcode var_opcode = opcode;
        switch (opcode) {
          case AARCH64_OP(lsl):
            var_opcode = AARCH64_OP(lslv);
            break;
          case AARCH64_OP(lsr):
            var_opcode = AARCH64_OP(lsrv);
            break;
          case AARCH64_OP(asr):
            var_opcode = AARCH64_OP(asrv);
            break;
          default:
            break;
        }
        inst = NewInstruction2(var_opcode, Materialize(g, op1), Materialize(g, op2));
      }
      break;
    }
    case AARCH64_OP(mul): {
      // If we are multiplying by a constant we can use shifts and
      // adds.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) || IRIsConst(op2)) {
        if (IRIsConst(op1) && IRIsConst(op2)) {
          break;
        }
        // One is constant, put it on the right of the slli instruction.
        if (IRIsConst(op1)) {
          int64_t c = ((IRConstant*)op1)->value.ivalue;
          if (c == 0) {
            // Multiply by zero is zero
            inst = ZeroReg(g);
          } else if (c == 1) {
            // Multiply by 1 is a mv.
            inst = NewInstruction(AARCH64_OP(mov));
            inst->operand[0] = Materialize(g, op2);
          } else {
            inst = MultiplyByConstant(g, op2, (IRConstant*)op1);
            if (inst == NULL) {
              break;
            }
            ref_counts_ok = true;
          }
        } else {
          int64_t c = ((IRConstant*)op2)->value.ivalue;
          if (c == 0) {
            // Multiply by zero is zero
            inst = ZeroReg(g);
          } else if (c == 1) {
            // Multiply by 1 is a mv.
            inst = NewInstruction(AARCH64_OP(mov));
            inst->operand[0] = Materialize(g, op1);
          } else {
            inst = MultiplyByConstant(g, op1, (IRConstant*)op2);
            if (inst == NULL) {
              break;
            }
            ref_counts_ok = true;
          }
        }
      }
      break;
    }
    case AARCH64_OP(udiv):
    case AARCH64_OP(sdiv): {
      // If we are dividing by a constant power of 2 we can use a shift.
      // TODO: other constants can be done too.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        AARCH64Opcode opcode =
            TypeIsUnsigned(node->type) ? AARCH64_OP(lsr) : AARCH64_OP(asr);
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: can we give an error on division by zero here?
        if (c == 1) {
          // Division by 1 is a mv.
          inst = NewInstruction(AARCH64_OP(mov));
          inst->operand[0] = Materialize(g, op1);
        } else {
          if (IsPowerOf2(c) && c < 64) {
            c = Log2(c);
            inst =
                NewInstruction2(opcode, Materialize(g, op1),
                                GetIntConstant(g, NULL, kTargetType32Bit, c));
            ref_counts_ok = true;
          }
        }
      }

      break;
    }

    case AARCH64_OP(umod):
    case AARCH64_OP(smod): {
      // If we are moding an unsigned value by a constant power of 2 we can use
      // an AND. Signed modulo needs truncation toward zero, so use div/msub.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: can we give an error on division by zero here?
        if (opcode == AARCH64_OP(umod) && IsPowerOf2(c) &&
            AARCH64IsLogicalImmediate(c - 1, node->type->size > 4)) {
          int64_t mask = c - 1;
          inst =
                 NewInstruction2(AARCH64_OP(and), Materialize(g, op1),
                                 GetIntConstant(g, NULL, kTargetType32Bit, mask));
          ref_counts_ok = true;
        }
      }
      if (inst == NULL) {
        inst = LowerModulo(g, node, opcode == AARCH64_OP(umod) ? AARCH64_OP(udiv) : AARCH64_OP(sdiv));
        ref_counts_ok = true;
      }
      break;
    }
      
    case AARCH64_OP(and): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // Anding with zero is zero.
          inst = ZeroReg(g);
        } else if (AARCH64IsLogicalImmediate(c, node->type->size > 4)) {
          inst = (TargetInstruction*)NewInstruction(AARCH64_OP(and));
          inst->operand[0] = Materialize(g, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      }
      break;
    }

    case AARCH64_OP(orr): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // ORing with zero is nop.
          inst = GetLoweredNode(op1);
        } else if (AARCH64IsLogicalImmediate(c, node->type->size > 4)) {
          inst = (TargetInstruction*)NewInstruction(AARCH64_OP(orr));
          inst->operand[0] = Materialize(g, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      }
      break;
    }
  }

  if (inst == NULL) {
    inst = (TargetInstruction*)NewInstruction(opcode);
    for (size_t i = 0; i < node->inputs.length; i++) {
      IRNode* input = node->inputs.value.p[i];
      inst->operand[i] = Materialize(g, input);
    }
  }

  // AArch64 register-form address arithmetic is performed in X registers.
  // When the offset was computed as a signed 32-bit integer, merely naming its
  // X-register view zero-extends it and turns a negative displacement into a
  // large positive one.  Widen variable signed offsets explicitly before they
  // participate in pointer arithmetic.
  if ((node->opcode == IR_OP(adda) || node->opcode == IR_OP(suba)) &&
      node->inputs.length == 2 && !IRIsConst(node->inputs.value.p[1])) {
    IRNode* offset = node->inputs.value.p[1];
    if (offset->type != NULL && offset->type->size == 4 &&
        !TypeIsUnsigned(offset->type)) {
      inst->operand[1] =
          Emit(g, SetInstructionSize(
                      NewInstruction1(AARCH64_OP(sxtw), inst->operand[1]),
                      kSize64Bit));
    }
  }

  // A `tmp` merge slot (used for ?: / && / ||) carries no register class in its
  // opcode, so mark it floating-point when its value is, ensuring the allocator
  // routes the producing instruction's result through an FP register.
  if (opcode == AARCH64_OP(tmp) && TypeIsFloatingPoint(node->type)) {
    inst->flags |= AARCH64_INST_FP;
  }

  CopyOrSetInstructionSize(node, inst);
  if (!ref_counts_ok) {
    TargetUpdateOperandUsers(inst);
  }
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL && inst != NULL) {
    AARCH64Opcode mov_opcode = TypeIsFloatingPoint(node->type)
                                   ? (node->type->size > 4 ? AARCH64_OP(fmv_d)
                                                           : AARCH64_OP(fmv_s))
                                   : AARCH64_OP(mov);
    inst = SetDestOrMove(g, inst, dest, mov_opcode);
  }
  SetLoweredNode(node, inst);
  return Emit(g, inst);
}

static TargetInstruction* LowerComparison(AARCH64Generator* g, IRNode* node) {
  // Check if any the outputs of the node are not branches.  If all
  // the uses are branches we defer the generation of the comparison
  // to the branch.
  //
  bool is_expression = node->dest != NULL;
  for (size_t i = 0; !is_expression && i < node->outputs.length; i++) {
    IRNode* output = node->outputs.value.p[i];
    if (!IRIsConditionalBranch(output)) {
      is_expression = true;
      break;
    }
  }
  if (!is_expression) {
    return NULL;
  }
  // Size is the size of the inputs.  They will all be the same.
  IRNode* op1 = node->inputs.value.p[0];
  bool is_unsigned = IRComparisonIsUnsigned(node);

  IRNode* lhs = node->inputs.value.p[0];
  IRNode* rhs = node->inputs.value.p[1];
  int compare_size = kSize32Bit;
  if (op1->type->size > 4) {
    compare_size = kSize64Bit;
  }
  int result_size = kSize32Bit;
  if (node->type->size > 4) {
    result_size = kSize64Bit;
  }
  TargetInstruction* result = NULL;
#define CMP_SET(cond)                                                       \
  do {                                                                      \
    Emit(g, SetInstructionSize(                                             \
                NewInstruction2(TypeIsFloatingPoint(op1->type)              \
                                    ? AARCH64_OP(fcmp)                      \
                                    : AARCH64_OP(cmp),                      \
                                Materialize(g, lhs), Materialize(g, rhs)),  \
                compare_size));                                             \
    result = SetInstructionSize(                                            \
        NewInstruction1(AARCH64_OP(cset), Condition(g, cond, result_size)),  \
        result_size);                                                       \
  } while (0)
  switch (node->opcode) {
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
    case IR_OP(cmpeqf):
    case IR_OP(cmpeqd):
      CMP_SET(AARCH64_OP(eq));
      break;
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
    case IR_OP(cmpnef):
    case IR_OP(cmpned):
      CMP_SET(AARCH64_OP(ne));
      break;
    case IR_OP(cmplti):
    case IR_OP(cmplta):
      CMP_SET(is_unsigned ? AARCH64_OP(lo) : AARCH64_OP(lt));
      break;
    case IR_OP(cmpltf):
    case IR_OP(cmpltd):
      CMP_SET(AARCH64_OP(lt));
      break;
    case IR_OP(cmplei):
    case IR_OP(cmplea):
      CMP_SET(is_unsigned ? AARCH64_OP(ls) : AARCH64_OP(le));
      break;
    case IR_OP(cmplef):
    case IR_OP(cmpled):
      CMP_SET(AARCH64_OP(le));
      break;
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
      CMP_SET(is_unsigned ? AARCH64_OP(hi) : AARCH64_OP(gt));
      break;
    case IR_OP(cmpgtf):
    case IR_OP(cmpgtd):
      CMP_SET(AARCH64_OP(gt));
      break;
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
      CMP_SET(is_unsigned ? AARCH64_OP(hs) : AARCH64_OP(ge));
      break;
    case IR_OP(cmpgef):
    case IR_OP(cmpged):
      CMP_SET(AARCH64_OP(ge));
      break;
    default:
      abort();
  }
#undef CMP_SET
  // Mark result as having comparison generated.
  if (result != NULL) {
    result->flags |= kAARCH64ComparisonGenerated;
  }
  Emit(g, result);
  // If the comparison result is consumed via a destination tmp (e.g. it is an
  // operand of a && / || / ?: that is materialized into a value), route the
  // cset there so the merged result lands in the destination's register.
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL && result != NULL) {
    result = SetDestOrMove(g, result, dest, AARCH64_OP(mov));
  }
  return SetLoweredNode(node, result);
}

// Emit one compare (cmp / fcmp) and a cset of `cond`, yielding a 0/1 value.
static TargetInstruction* ThreeWayRelation(AARCH64Generator* g, IRNode* lhs,
                                           IRNode* rhs, TypeRecord* op_type,
                                           AARCH64Opcode cond, bool is_fp,
                                           int compare_size, int result_size) {
  Emit(g, SetInstructionSize(
              NewInstruction2(is_fp ? AARCH64_OP(fcmp) : AARCH64_OP(cmp),
                              Materialize(g, lhs), Materialize(g, rhs)),
              compare_size));
  TargetInstruction* set = SetInstructionSize(
      NewInstruction1(AARCH64_OP(cset), Condition(g, cond, result_size)),
      result_size);
  set->flags |= kAARCH64ComparisonGenerated;
  return Emit(g, set);
}

// Lower a three-way comparison to an integer -1/0/1 (and 2 = unordered for
// floating point).  Compute the "less" and "greater" relations as 0/1 booleans
// and subtract; for floating point also detect the unordered case (V flag,
// cset vs) and add 2.
static TargetInstruction* LowerThreeWay(AARCH64Generator* g, IRNode* node) {
  IRNode* lhs = node->inputs.value.p[0];
  IRNode* rhs = node->inputs.value.p[1];
  TypeRecord* op_type = lhs->type;
  bool is_fp =
      node->opcode == IR_OP(cmp3wayf) || node->opcode == IR_OP(cmp3wayd);
  bool is_unsigned =
      node->opcode == IR_OP(cmp3wayu) || node->opcode == IR_OP(cmp3waya);
  int compare_size = op_type->size > 4 ? kSize64Bit : kSize32Bit;
  int result_size = node->type->size > 4 ? kSize64Bit : kSize32Bit;

  // Floating point uses `mi` (N set) for less and `gt` for greater so an
  // unordered (NaN) result makes neither true; integers use the usual
  // signed/unsigned ordering conditions.
  AARCH64Opcode lt_cond =
      is_fp ? AARCH64_OP(mi) : (is_unsigned ? AARCH64_OP(lo) : AARCH64_OP(lt));
  AARCH64Opcode gt_cond =
      is_fp ? AARCH64_OP(gt) : (is_unsigned ? AARCH64_OP(hi) : AARCH64_OP(gt));

  TargetInstruction* gt = ThreeWayRelation(g, lhs, rhs, op_type, gt_cond, is_fp,
                                           compare_size, result_size);
  TargetInstruction* lt = ThreeWayRelation(g, lhs, rhs, op_type, lt_cond, is_fp,
                                           compare_size, result_size);
  TargetInstruction* result = Emit(
      g, SetInstructionSize(NewInstruction2(AARCH64_OP(sub), gt, lt),
                            result_size));
  if (is_fp) {
    TargetInstruction* unord = ThreeWayRelation(
        g, lhs, rhs, op_type, AARCH64_OP(vs), is_fp, compare_size, result_size);
    TargetInstruction* twice = Emit(
        g, SetInstructionSize(NewInstruction2(AARCH64_OP(add), unord, unord),
                              result_size));
    result = Emit(g, SetInstructionSize(
                         NewInstruction2(AARCH64_OP(add), result, twice),
                         result_size));
  }
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    result = SetDestOrMove(g, result, dest, AARCH64_OP(mov));
  }
  return SetLoweredNode(node, result);
}

static void GetAddressAndOffsetFrom(AARCH64Generator* g,
                                 TargetInstruction* addr,
                                 int offset,
                                 TargetInstruction** addr_inst,
                                 TargetInstruction** offset_inst) {
  if (AARCH64LoadStoreImmInRange(offset)) {
    *addr_inst = addr;
    *offset_inst = GetIntConstant(g, NULL, kTargetType32Bit, offset);
    return;
  }
  // The offset is too large for a load/store immediate (9-bit signed for the
  // unscaled forms used for frame access); materialize the full effective
  // address and use a zero offset.
  *addr_inst = OffsetFrom(g, addr, offset);
  *offset_inst = GetIntConstant(g, NULL, kTargetType32Bit, 0);
}

static bool GetRegAndOffset(AARCH64Generator* g, IRNode* addr_node,
                            TargetInstruction** addr,
                            TargetInstruction** offset,
                            TargetInstruction** scale) {
  *scale = NULL;
  if (IRIsThreadVariable(addr_node)) {
    *addr = GetTlsVariableAddress(g, addr_node);
    *offset = ZeroImm(g);
    return true;
  }

  if (IRIsAutoVariable(addr_node)) {
    IRVariable* var = (IRVariable*)addr_node;
    if (TypeIsVLA(var->symbol->type)) {
      IRNode* vla_addr = var->symbol->value.other;
      *addr = GetLoweredNode(vla_addr);
      *offset = ZeroImm(g);
      return false;
    }
    if ((addr_node->flags & kIRNrvoMarker) != 0) {
      // NRVO variable.
      int reg = addr_node->data.ivalue & ~AARCH64_REG_VAR;
      *addr = IntVariableRegister(g, reg, var->symbol);
      *offset = ZeroImm(g);
     return true;
    }
    int32_t var_offset = addr_node->data.ivalue;
    if (AARCH64_IS_REG_VAR(var_offset)) {
      // Variable is in a register.
      int var_num = var_offset & ~AARCH64_REG_VAR;
      if (TypeIsFloatingPoint(addr_node->type)) {
        *addr = FloatingPointVariableRegister(g, var_num, var->symbol);
      } else {
        *addr = IntVariableRegister(g, var_num, var->symbol);
      }
      *offset = NULL;
      return false;
    }

    // Auto variable is in the stack frame.  These are accessed through
    // the frame pointer with a negative offset.
    GetAddressAndOffsetFrom(g, FramePointer(g), LocalVariableOffset(g, var_offset),
                            addr, offset);
  } else if (IRIsArgument(addr_node)) {
    int32_t var_offset = addr_node->data.ivalue;
    IRVariable* var = (IRVariable*)addr_node;
    // A struct/union larger than 8 bytes was passed by reference: the slot (or
    // register) holds a *pointer* to the caller's copy, which is the address of
    // the parameter.  Dereference it so callers see the struct's address.
    bool by_ref_struct =
        TypePassedAsAArch64Aggregate(addr_node->type) && addr_node->type->size > 8;
    if (AARCH64_IS_REG_VAR(var_offset)) {
      // Argument is in a register.
      int var_num = var_offset & ~AARCH64_REG_VAR;
      if (TypeIsFloatingPoint(addr_node->type)) {
        *addr = FloatingPointVariableRegister(g, var_num, var->symbol);
      } else {
        *addr = IntVariableRegister(g, var_num, var->symbol);
      }
      if (by_ref_struct) {
        // The register holds the pointer; use it directly as the base address.
        *offset = ZeroImm(g);
        return true;
      }
      *offset = NULL;
      return false;
    } else if (by_ref_struct) {
      // Load the pointer from its frame slot and use it as the base address.
      TargetInstruction* slot_addr;
      TargetInstruction* slot_off;
      GetAddressAndOffsetFrom(g, FramePointer(g), var_offset, &slot_addr,
                              &slot_off);
      *addr = Emit(g, SetInstructionSize(
                          NewInstruction2(AARCH64_OP(ldr), slot_addr, slot_off),
                          kSize64Bit));
      *offset = ZeroImm(g);
      return true;
    } else {
      GetAddressAndOffsetFrom(g, FramePointer(g), var_offset,
                              addr, offset);
     }
  } else if (IRIsStaticVariable(addr_node)) {
    // The address of static variables need to be moved into a register.

    *addr = LoadStaticVariableAddress(g, addr_node);
    *offset = ZeroImm(g);
  } else {
    // All others have a calculated address.
    *addr = GetLoweredNode(addr_node);
    *offset = ZeroImm(g);
    assert(addr != NULL);
  }
  return true;
}

static TargetInstruction* Load(AARCH64Generator* g, IRNode* addr_node, AARCH64Opcode opcode, int size) {
  TargetInstruction* addr;
  TargetInstruction* offset;
  TargetInstruction* scale;
  bool on_stack = GetRegAndOffset(g, addr_node, &addr, &offset, &scale);

  if (!on_stack) {
    return addr;
  }

  TargetInstruction* result = Emit(g, SetInstructionSize(NewInstruction2(opcode, addr, offset), size));

#if 0
  TargetInstruction* result = NULL;
  if ((AARCH64Opcode)((int)addr->opcode == (int)AARCH64_OP(add)) && TargetIsZero(offset)) {
    // If the address is calculated using an addi instruction we can
    // combine the immediate from the addi with the load.
    // The addi instruction will no longer be used and will be eliminated
    // during the optimization pass.
    TargetInstruction* src = addr->operand[0];
    TargetInstruction* immed = addr->operand[1];
    assert(src != NULL);
    assert(immed != NULL);
    assert(TargetIsConst(immed));
    result = Emit(g, SetInstructionSize(NewInstruction2(opcode, src, immed), size));
  }
  if (result == NULL) {
    result = Emit(g, SetInstructionSize(NewInstruction2(opcode, addr, offset), size));
  }
#endif
  return result;
}

static TargetInstruction* LowerLoad(AARCH64Generator* g, IRNode* node) {
  AARCH64Opcode opcode;
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];
  
  int size = kSize32Bit;
  switch (node->opcode) {
    case IR_OP(load32):
      opcode = AARCH64_OP(ldr);
      break;
    case IR_OP(load8):
      opcode = AARCH64_OP(ldrsb);
      break;
    case IR_OP(load64):
      opcode = AARCH64_OP(ldr);
      size = kSize64Bit;
      break;
    case IR_OP(load16):
      opcode = AARCH64_OP(ldrsh);
      break;
    case IR_OP(loadu32):
      opcode = AARCH64_OP(ldr);
      break;
    case IR_OP(loadu8):
      opcode = AARCH64_OP(ldrb);
      break;
    case IR_OP(loadu16):
      opcode = AARCH64_OP(ldrh);
      break;
    case IR_OP(loadf):
      opcode = AARCH64_OP(fldr);
      break;
    case IR_OP(loadd):
      opcode = AARCH64_OP(fldr);
      size = kSize64Bit;
      break;
    case IR_OP(loada):
      opcode = AARCH64_OP(ldr);
      size = kSize64Bit;
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }

  TargetInstruction* result = Load(g, addr_node, opcode, size);
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    AARCH64Opcode mov_opcode = TypeIsFloatingPoint(node->type)
                                   ? (node->type->size > 4 ? AARCH64_OP(fmv_d)
                                                           : AARCH64_OP(fmv_s))
                                   : AARCH64_OP(mov);
    result = SetDestOrMove(g, result, dest, mov_opcode);
  }
  return SetLoweredNode(node, result);
}

static TargetInstruction* Store(AARCH64Generator* g, IRNode* addr_node, TargetInstruction* src, AARCH64Opcode opcode, int size) {
  TargetInstruction* addr;
  TargetInstruction* offset;
  TargetInstruction* scale;
  bool on_stack = GetRegAndOffset(g, addr_node, &addr, &offset, &scale);
  // addr is a register.
  // if the variable is in memory offset will be an integer constant
  // containing the offset.  Otherwise it is NULL.

  // If we are not on the stack, move the src to the dest.
  if (!on_stack) {
    AARCH64Opcode opcode = TypeIsFloatingPoint(addr_node->type) ? AARCH64_OP(fmov) : AARCH64_OP(mov);
    TargetInstruction* result = SetDestOrMove(g, src, addr, opcode);
    return result;
  }

  return Emit(g, SetInstructionSize(NewInstruction4(opcode, src, addr, offset, scale), size));
}

static TargetInstruction* Compare(AARCH64Generator* g, TargetInstruction* lhs, TargetInstruction* rhs,
                                  bool is_floating_point, int size) {
  return Emit(g, SetInstructionSize(NewInstruction2(is_floating_point ? AARCH64_OP(fcmp) : AARCH64_OP(cmp), lhs, rhs), size));
}

static TargetInstruction* CompareImmediate(AARCH64Generator* g, TargetInstruction* lhs, TargetInstruction* rhs, int size) {
  return Emit(g, SetInstructionSize(NewInstruction2(AARCH64_OP(cmp), lhs, rhs), size));
}

static TargetInstruction* EmitCompareZeroBranch(
    AARCH64Generator* g, AARCH64Opcode opcode, TargetInstruction* value,
    IRNode* target_node, int size) {
  TargetInstruction* branch =
      Emit(g, SetInstructionSize(NewInstruction1(opcode, value), size));
  if (target_node->data.ptr == NULL) {
    VectorAppend(&g->base.fixups, NewBranchFixup(branch, target_node, 1));
  } else {
    branch->operand[1] = target_node->data.ptr;
    TargetAddUser(target_node->data.ptr, branch);
  }
  return branch;
}

static void CompareEqualZero(AARCH64Generator* g, IRNode* value_node,
                             IRNode* target_node, int size) {
  EmitCompareZeroBranch(g, AARCH64_OP(cbz), Materialize(g, value_node),
                        target_node, size);
}

static void CompareNotEqualZero(AARCH64Generator* g, IRNode* value_node,
                             IRNode* target_node, int size) {
  EmitCompareZeroBranch(g, AARCH64_OP(cbnz), Materialize(g, value_node),
                        target_node, size);
}

enum ComparisonOp {
  kCompEqual,
  kCompNotEqual,
  kCompLess,
  kCompGreater,
  kCompLessEq,
  kCompGreaterEq,
};

struct Comparison {
  enum ComparisonOp comparison;
  AARCH64Opcode signed_forward;
  AARCH64Opcode signed_reverse;
  AARCH64Opcode unsigned_forward;
  AARCH64Opcode unsigned_reverse;
} comparisons[] = {
  {kCompEqual, AARCH64_OP(eq), AARCH64_OP(ne), AARCH64_OP(eq), AARCH64_OP(ne)},
  {kCompNotEqual, AARCH64_OP(ne), AARCH64_OP(eq), AARCH64_OP(ne), AARCH64_OP(eq)},
  {kCompLess, AARCH64_OP(lt), AARCH64_OP(ge), AARCH64_OP(lo), AARCH64_OP(hs)},
  {kCompGreater, AARCH64_OP(gt), AARCH64_OP(le), AARCH64_OP(hi), AARCH64_OP(ls)},
  {kCompLessEq, AARCH64_OP(le), AARCH64_OP(gt), AARCH64_OP(ls), AARCH64_OP(hi)},
  {kCompGreaterEq, AARCH64_OP(ge), AARCH64_OP(lt), AARCH64_OP(hs), AARCH64_OP(lo)},
};

#define NUM_COMPARISONS (sizeof(comparisons) / sizeof(comparisons[0]))

static AARCH64Opcode GetComparison(enum ComparisonOp op, bool is_unsigned, bool reverse) {
  for (int i = 0; i < NUM_COMPARISONS; i++) {
    struct Comparison* c = &comparisons[i];
    if (c->comparison == op) {
      if (is_unsigned) {
        if (reverse) {
          return c->unsigned_reverse;
        }
        return c->unsigned_forward;
      } else if (reverse) {
        return c->signed_reverse;
      }
      return c->signed_forward;
    }
  }
  abort();
}

struct ToComparisonOp {
  IROpcode op;
  enum ComparisonOp forward_comparison;
  enum ComparisonOp inverted_comparison;
} comparison_ops[] = {
  {IR_OP(cmpeqi), kCompEqual, kCompNotEqual},
  {IR_OP(cmpeqa), kCompEqual, kCompNotEqual},
  {IR_OP(cmpeqf), kCompEqual, kCompNotEqual},
  {IR_OP(cmpeqd), kCompEqual, kCompNotEqual},
  
  {IR_OP(cmpnei), kCompNotEqual, kCompEqual},
  {IR_OP(cmpnea), kCompNotEqual, kCompEqual},
  {IR_OP(cmpnef), kCompNotEqual, kCompEqual},
  {IR_OP(cmpned), kCompNotEqual, kCompEqual},

  {IR_OP(cmplti), kCompLess, kCompGreater},
  {IR_OP(cmplta), kCompLess, kCompGreater},
  {IR_OP(cmpltf), kCompLess, kCompGreater},
  {IR_OP(cmpltd), kCompLess, kCompGreater},
  
  {IR_OP(cmpgti), kCompGreater, kCompLess},
  {IR_OP(cmpgta), kCompGreater, kCompLess},
  {IR_OP(cmpgtf), kCompGreater, kCompLess},
  {IR_OP(cmpgtd), kCompGreater, kCompLess},

  {IR_OP(cmplei), kCompLessEq, kCompGreaterEq},
  {IR_OP(cmplea), kCompLessEq, kCompGreaterEq},
  {IR_OP(cmplef), kCompLessEq, kCompGreaterEq},
  {IR_OP(cmpled), kCompLessEq, kCompGreaterEq},
  
  {IR_OP(cmpgei), kCompGreaterEq, kCompLessEq},
  {IR_OP(cmpgea), kCompGreaterEq, kCompLessEq},
  {IR_OP(cmpgef), kCompGreaterEq, kCompLessEq},
  {IR_OP(cmpged), kCompGreaterEq, kCompLessEq},
};

#define NUM_COMPARISONS_OPS (sizeof(comparison_ops) / sizeof(comparison_ops[0]))

static enum ComparisonOp ToComparisonOp(IROpcode op, bool invert) {
  for (int i = 0; i < NUM_COMPARISONS_OPS; i++) {
    struct ToComparisonOp* c = &comparison_ops[i];
    if (c->op == op) {
      return invert ? c->inverted_comparison : c->forward_comparison;
    }
  }
  abort();
}

// Compares 2 nodes.  Branches to target.
static TargetInstruction* CompareAndBranch(AARCH64Generator* g,
                                           IRNode* lhs_node,
                                           IRNode* rhs_node,
                                           IRNode* target_node,
                                           bool is_unsigned,
                                           bool reverse,
                                           IROpcode op) {
  bool is_floating_point = false;
  switch (op) {
    case IR_OP(cmpeqf):
    case IR_OP(cmpeqd):
    case IR_OP(cmpnef):
    case IR_OP(cmpned):
    case IR_OP(cmpltf):
    case IR_OP(cmpltd):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgtd):
    case IR_OP(cmplef):
    case IR_OP(cmpled):
    case IR_OP(cmpgef):
    case IR_OP(cmpged):
      is_floating_point = true;
      break;
    default:
      break;
  }
  
  int size = kSize32Bit;
  if (lhs_node->type->size > 4) {
    size = kSize64Bit;
  }
  TargetInstruction* lhs = NULL;
  TargetInstruction* rhs = NULL;
  bool constant_compare = false;
  bool inverted_comparison = false;

  // Floating point comparison doesn't have an immediate variant.
  if (!is_floating_point) {
    if (IRIsConstant(rhs_node)) {
      int64_t v = IRIntConstValue(rhs_node);
      constant_compare = AARCH64IsPossibleImmediate(v);
    } else if (IRIsConstant(lhs_node)) {
      int64_t v = IRIntConstValue(lhs_node);
      constant_compare = AARCH64IsPossibleImmediate(v);
      if (constant_compare) {
        // Comparison with constant on left. Invert comparison.
        inverted_comparison = true;
      }
    }
  }
  
  if (constant_compare) {
    if (inverted_comparison) {
      rhs = GetLoweredNode(lhs_node);
      lhs = Materialize(g, rhs_node);
    } else {
      lhs = Materialize(g, lhs_node);
      rhs = GetLoweredNode(rhs_node);
    }
    CompareImmediate(g, lhs, rhs, size);
  } else {
    lhs = Materialize(g, lhs_node);
    rhs = Materialize(g, rhs_node);
    Compare(g, lhs, rhs, is_floating_point, size);
  }
  
  enum ComparisonOp comp_op = ToComparisonOp(op, inverted_comparison);
  AARCH64Opcode cond = GetComparison(comp_op, is_unsigned, reverse);
  return EmitBranch(g, cond, target_node);
}

static TargetInstruction* LowerStore(AARCH64Generator* g, IRNode* node) {
  AARCH64Opcode opcode;
  assert(node->inputs.length == 2);

  // Address to store to is the first operand of the store IR node.
  IRNode* addr_node = node->inputs.value.p[0];

  // Value to store is in second input.
  IRNode* src_node = node->inputs.value.p[1];

  int size = kSize32Bit;
  
  // Work out store opcode.
  switch (node->opcode) {
    case IR_OP(store32):
      opcode = AARCH64_OP(str);
      break;
    case IR_OP(store8):
      opcode = AARCH64_OP(strb);
      break;
    case IR_OP(store64):
      opcode = AARCH64_OP(str);
      size = kSize64Bit;
      break;
    case IR_OP(store16):
      opcode = AARCH64_OP(strh);
      break;
    case IR_OP(storef):
      opcode = AARCH64_OP(fstr);
      break;
    case IR_OP(stored):
      opcode = AARCH64_OP(fstr);
      size = kSize64Bit;
      break;
    case IR_OP(storea):
      opcode = AARCH64_OP(str);
      size = kSize64Bit;
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }
  TargetInstruction* src = Materialize(g, src_node);
  TargetInstruction* stored = Store(g, addr_node, src, opcode, size);
  if (node->outputs.length > 0 && !TypeIsFloatingPoint(addr_node->type)) {
    // `return value += amount;` reads the assignment's value, which the IR
    // spells as a use of the store.  A store to memory produces no value of its
    // own, so hand on what was stored.  Left as the store instruction this
    // picks up whatever register the allocator happened to give the store,
    // which holds nothing in particular -- the address, as it turns out.
    return SetLoweredNode(node, src);
  }
  return SetLoweredNode(node, stored);
}

// If the branch comes from a comparison node we combine the comparison
// with the branch.  If it comes from another node we generate an equality
// comparison of that node with zero.
static TargetInstruction* LowerConditionalBranch(AARCH64Generator* g,
                                                 IRNode* node) {
  bool reverse = node->opcode == IR_OP(bfalse);

  assert(node->inputs.length == 2);
  IRNode* expr = node->inputs.value.p[0];
  IRNode* target_node = node->inputs.value.p[1];
  IRNode* input = node->inputs.value.p[0];

  // A boolean conversion is represented as (value == 0) or (value != 0).
  // Branch directly on the original integer/address value so the target can
  // use cbz/cbnz instead of materializing or re-testing the boolean.
  if (!HasLoweredNode(input) &&
      (input->opcode == IR_OP(cmpeqi) || input->opcode == IR_OP(cmpeqa) ||
       input->opcode == IR_OP(cmpnei) || input->opcode == IR_OP(cmpnea))) {
    IRNode* value = NULL;
    if (IRIsConst(input->inputs.value.p[1]) &&
        IRIntConstValue(input->inputs.value.p[1]) == 0) {
      value = input->inputs.value.p[0];
    } else if (IRIsConst(input->inputs.value.p[0]) &&
               IRIntConstValue(input->inputs.value.p[0]) == 0) {
      value = input->inputs.value.p[1];
    }
    if (value != NULL) {
      bool comparison_is_equal =
          input->opcode == IR_OP(cmpeqi) || input->opcode == IR_OP(cmpeqa);
      bool branch_when_zero =
          comparison_is_equal != (node->opcode == IR_OP(bfalse));
      int size = value->type->size > 4 ? kSize64Bit : kSize32Bit;
      EmitCompareZeroBranch(g,
                            branch_when_zero ? AARCH64_OP(cbz)
                                             : AARCH64_OP(cbnz),
                            Materialize(g, value), target_node, size);
      return NULL;
    }
  }

  // Check if the branch comes from a comparison.  If not, we compare with
  // zero.
  bool compare_with_zero = !IRIsComparison(input);
  if (!compare_with_zero && HasLoweredNode(input)) {
    TargetInstruction* comp = GetLoweredNode(input);
    compare_with_zero = (comp->flags & kAARCH64ComparisonGenerated) != 0;
  }
  if (compare_with_zero) {
    int size = kSize32Bit;
    if (expr->type->size > 4) {
      size = kSize64Bit;
    }
    if (IRIsConst(input)) {
      // Compare constant.
      // If constant is zero, BRA is comparing false
      // otherwise, BRA is comparing true.
      int64_t cval = IRIntConstValue(input);
      if (cval == 0) {
        if (node->opcode == IR_OP(bfalse)) {
          EmitBranch(g, AARCH64_OP(al), target_node);
        }
      } else {
        if (node->opcode == IR_OP(btrue)) {
          EmitBranch(g, AARCH64_OP(al), target_node);
        }
      }
   } else {
      // Generate equality comparison with zero.
      if (reverse) {
        CompareEqualZero(g, expr, target_node, size);
      } else {
        CompareNotEqualZero(g, expr, target_node, size);
      }
    }
    return NULL;
  }
  IRNode* lhs = input->inputs.value.p[0];
  IRNode* rhs = input->inputs.value.p[1];
  bool is_unsigned = IRComparisonIsUnsigned(input);
  return CompareAndBranch(g, lhs, rhs, target_node, is_unsigned, reverse, expr->opcode);
}

static TargetInstruction* LowerBranch(AARCH64Generator* g, IRNode* node) {
  if (node->inputs.length == 0) {
    return NULL;
  }
  IRNode* target_node = node->inputs.value.p[0];

  // A return jump must branch to the shared epilogue: even a function that
  // looks like a leaf here (no calls, zero stack-frame size) may still need to
  // restore callee-saved registers and deallocate its frame in the epilogue.
  // Register usage is not known until after register allocation, so emitting a
  // bare `ret` here would skip a restore that turns out to be required and
  // leave the caller's frame pointer / callee-saved registers clobbered.

  // Normal branch or return branch.
  TargetInstruction* inst =
      (TargetInstruction*)Emit(g, NewInstruction1(AARCH64_OP(b), Condition(g, AARCH64_OP(al), 0)));

  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&g->base.fixups, NewBranchFixup(inst, target_node, 1));
  } else {
    inst->operand[1] = target;
  }
  return inst;
}

static TargetInstruction* LowerLabel(AARCH64Generator* g, IRNode* label) {
  TargetInstruction* inst =  Emit(g, NewInstruction(AARCH64_OP(label)));
  label->data.ptr = inst;
  ApplyFixups(g, label);
  return inst;
}

static TargetInstruction* LowerNamedLabel(AARCH64Generator* g, IRNode* label) {
  IRNamedLabel* n = (IRNamedLabel*)label;
  TargetInstruction* inst =  Emit(g, TargetNewNamedLabel(n->name));
  label->data.ptr = inst;
  return inst;
}

static TargetInstruction* LowerResult(AARCH64Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  if (node->opcode == IR_OP(resultv)) {
    TargetInstruction* address = Materialize(g, node->inputs.value.p[0]);
    TargetInstruction* load = Emit(
        g, SetInstructionSize(
               NewInstruction2(
                   AARCH64_OP(fldr), address,
                   GetIntConstant(g, NULL, kTargetType32Bit, 0)),
               kSize64Bit));
    TargetInstruction* result_reg =
        EmitSymbol(g, NewInstruction(AARCH64_OP(resultd)));
    load->dest = result_reg;
    return SetLoweredNode(node, load);
  }
  AARCH64Opcode result_reg_opcode, opcode;
  switch (node->opcode) {
    case IR_OP(resulti):
    case IR_OP(resulta):
      result_reg_opcode = AARCH64_OP(resulti);
      opcode = AARCH64_OP(mov);
      break;
    case IR_OP(resultf):
      result_reg_opcode = AARCH64_OP(resultf);
      opcode = AARCH64_OP(fmov);
      break;
    case IR_OP(resultd):
      result_reg_opcode = AARCH64_OP(resultd);
      opcode = AARCH64_OP(fmov);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }
#if 0
  TargetInstruction* result = Materialize(g, node->inputs.value.p[0]);
  TargetInstruction* result_reg = Emit(g, NewInstruction(result_reg_opcode));
  return Emit(g, NewInstruction2(opcode, result_reg, result));
#else
  TargetInstruction* result = Materialize(g, node->inputs.value.p[0]);
  TargetInstruction* result_reg = EmitSymbol(g, NewInstruction(result_reg_opcode));
  return SetLoweredNode(node, SetDestOrMove(g, result, result_reg, opcode));
#endif
}

static TargetInstruction* LowerCaptureVectorResult(AARCH64Generator* g,
                                                   IRNode* node) {
  assert(node->inputs.length == 2);
  TargetInstruction* destination = Materialize(g, node->inputs.value.p[0]);
  TargetInstruction* call = Materialize(g, node->inputs.value.p[1]);
  call->flags |= AARCH64_INST_FP_RETURN;
  return SetLoweredNode(
      node, Emit(g, SetInstructionSize(
                        NewInstruction3(
                            AARCH64_OP(fstr), call, destination,
                            GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                        kSize64Bit)));
}

static TargetInstruction* SetSimdArrangement(TargetInstruction* inst,
                                             int vector_bytes, int elem_bytes) {
  int elem_log = elem_bytes <= 1 ? 0 : elem_bytes == 2 ? 1 : elem_bytes == 4 ? 2 : 3;
  inst->flags = (inst->flags & ~AARCH64_SIMD_ELEM_MASK) |
                (elem_log << AARCH64_SIMD_ELEM_SHIFT);
  return SetInstructionSize(
      inst, vector_bytes == 16 ? kSize128Bit : kSize64Bit);
}

static TargetInstruction* LowerVectorOperation(AARCH64Generator* g,
                                               IRNode* node) {
  assert(node->inputs.length == 3);
  TypeRecord* vector_type = (TypeRecord*)node->aux;
  if (!TypeIsVector(vector_type)) {
    vector_type = ((IRNode*)node->inputs.value.p[0])->type;
    if (TypeIsPointer(vector_type)) {
      vector_type = vector_type->next;
    }
  }
  assert(TypeIsVector(vector_type));
  TypeRecord* element = TypeVectorElement(vector_type);
  assert(element != NULL);
  bool is_float = TypeUsesFloat32Representation(element) ||
                  TypeUsesFloat64Representation(element);

  AARCH64Opcode opcode;
  bool swap_operands = false;
  switch (node->opcode) {
    case IR_OP(vadd):
      opcode = is_float ? AARCH64_OP(vfadd) : AARCH64_OP(vadd);
      break;
    case IR_OP(vsub):
      opcode = is_float ? AARCH64_OP(vfsub) : AARCH64_OP(vsub);
      break;
    case IR_OP(vmul): opcode = AARCH64_OP(vfmul); break;
    case IR_OP(vdiv): opcode = AARCH64_OP(vfdiv); break;
    case IR_OP(vand): opcode = AARCH64_OP(vand); break;
    case IR_OP(vor): opcode = AARCH64_OP(vorr); break;
    case IR_OP(vxor): opcode = AARCH64_OP(veor); break;
    case IR_OP(vcmpeq): opcode = AARCH64_OP(vcmeq); break;
    case IR_OP(vcmpgt): opcode = AARCH64_OP(vcmgt); break;
    case IR_OP(vcmpge): opcode = AARCH64_OP(vcmge); break;
    case IR_OP(vcmplt):
      opcode = AARCH64_OP(vcmgt);
      swap_operands = true;
      break;
    case IR_OP(vcmple):
      opcode = AARCH64_OP(vcmge);
      swap_operands = true;
      break;
    case IR_OP(vcmpgtu): opcode = AARCH64_OP(vcmhi); break;
    case IR_OP(vcmpgeu): opcode = AARCH64_OP(vcmhs); break;
    case IR_OP(vcmpltu):
      opcode = AARCH64_OP(vcmhi);
      swap_operands = true;
      break;
    case IR_OP(vcmpleu):
      opcode = AARCH64_OP(vcmhs);
      swap_operands = true;
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }

  int vec_size = vector_type->size == 16 ? kSize128Bit : kSize64Bit;
  TargetInstruction* destination = Materialize(g, node->inputs.value.p[0]);
  TargetInstruction* left_address = Materialize(g, node->inputs.value.p[1]);
  TargetInstruction* right_address = Materialize(g, node->inputs.value.p[2]);
  TargetInstruction* left = Emit(
      g, SetInstructionSize(
             NewInstruction2(
                 AARCH64_OP(fldr), left_address,
                 GetIntConstant(g, NULL, kTargetType32Bit, 0)),
             vec_size));
  TargetInstruction* right = Emit(
      g, SetInstructionSize(
             NewInstruction2(
                 AARCH64_OP(fldr), right_address,
                 GetIntConstant(g, NULL, kTargetType32Bit, 0)),
             vec_size));
  if (swap_operands) {
    TargetInstruction* temporary = left;
    left = right;
    right = temporary;
  }
  TargetInstruction* operation = Emit(
      g, SetSimdArrangement(NewInstruction2(opcode, left, right),
                            vector_type->size, element->size));
  TargetInstruction* store = Emit(
      g, SetInstructionSize(
             NewInstruction3(
                 AARCH64_OP(fstr), operation, destination,
                 GetIntConstant(g, NULL, kTargetType32Bit, 0)),
             vec_size));
  return SetLoweredNode(node, store);
}

static TargetInstruction* LowerAsm(AARCH64Generator* g, IRNode* node) {
  // The first argument is a literal containing the assembly language.
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(g, TargetNewLiteral((int)id_node->value.ivalue));

  AsmASTNode* asm_node = node->aux;
  if (asm_node == NULL ||
      (asm_node->outputs.length == 0 && asm_node->inputs.length == 0 &&
       asm_node->clobbers.length == 0 && asm_node->labels.length == 0)) {
    TargetInstruction* result = Emit(g, NewInstruction1(AARCH64_OP(asm), literal));

    SetLoweredNode(node, result);
    return result;
  }

  if (asm_node->outputs.length + asm_node->inputs.length > AARCH64_MAX_ASM_OPERANDS) {
    assert(false);
  }

  AARCH64AsmInstruction* asm_inst = malloc(sizeof(AARCH64AsmInstruction));
  TargetInitInstruction(&asm_inst->base, (TargetOpcode)AARCH64_OP(asm));
  asm_inst->base.flags |= AARCH64_INST_EXTENDED_ASM;
  asm_inst->base.operand[0] = literal;
  asm_inst->asm_node = asm_node;
  asm_inst->num_operands = (int)(asm_node->outputs.length + asm_node->inputs.length);
  memset(asm_inst->immediates, 0, sizeof(asm_inst->immediates));
  memset(asm_inst->immediate_values, 0, sizeof(asm_inst->immediate_values));

  size_t ir_index = 1;
  int operand_index = 0;
  TargetInstruction* output_regs[AARCH64_MAX_ASM_OPERANDS] = {0};
  for (size_t i = 0; i < asm_node->outputs.length; i++, operand_index++) {
    AsmOperand* operand = asm_node->outputs.value.p[i];
    IRNode* addr_node = node->inputs.value.p[ir_index++];
    int reg_num = 9 - (int)i;
    if (reg_num < 0) {
      reg_num = 9;
    }
    TargetInstruction* reg =
        EmitSymbol(g, NewInstruction((AARCH64Opcode)(AARCH64_OP(r0) + reg_num)));
    asm_inst->reg_nums[operand_index] = reg_num;
    asm_inst->is_fp[operand_index] = false;
    asm_inst->sizes[operand_index] = operand->expr->type->size <= 4 ? kSize32Bit : kSize64Bit;
    output_regs[i] = reg;
    if (operand->is_readwrite) {
      TargetInstruction* loaded = Load(g, addr_node, AARCH64_OP(ldr),
                                       asm_inst->sizes[operand_index]);
      SetDestOrMove(g, loaded, reg, AARCH64_OP(mov));
    }
  }

  for (size_t i = 0; i < asm_node->inputs.length; i++, operand_index++) {
    AsmOperand* operand = asm_node->inputs.value.p[i];
    IRNode* input_node = node->inputs.value.p[ir_index++];
    int reg_num = (int)i;
    TargetInstruction* reg =
        EmitSymbol(g, NewInstruction((AARCH64Opcode)(AARCH64_OP(r0) + reg_num)));
    asm_inst->reg_nums[operand_index] = reg_num;
    asm_inst->is_fp[operand_index] = false;
    asm_inst->sizes[operand_index] = operand->expr->type->size <= 4 ? kSize32Bit : kSize64Bit;
    if (strchr(operand->constraint.value, 'i') != NULL && IRIsIntConst(input_node)) {
      asm_inst->reg_nums[operand_index] = INT_MIN;
      asm_inst->immediate_values[operand_index] = IRIntConstValue(input_node);
      continue;
    }
    SetDestOrMove(g, Materialize(g, input_node), reg, AARCH64_OP(mov));
  }

  TargetInstruction* result = Emit(g, &asm_inst->base);
  for (size_t i = 0; i < asm_node->outputs.length; i++) {
    AsmOperand* operand = asm_node->outputs.value.p[i];
    IRNode* addr_node = node->inputs.value.p[1 + i];
    Store(g, addr_node, output_regs[i], AARCH64_OP(str),
          operand->expr->type->size <= 4 ? kSize32Bit : kSize64Bit);
  }

  SetLoweredNode(node, result);
  return result;
}

// A literal reference is an add of the literal offset (the first input
// to the literalref node) to the 'literal' with the given id.  This will
// be assembled as a reference to a symbol with the name .str.%d.
static TargetInstruction* LowerLiteralReference(AARCH64Generator* g, IRNode* node) {
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(g, SetInstructionSize(TargetNewLiteral((int)id_node->value.ivalue), kSize64Bit));

  TargetInstruction* result = Emit(g, SetInstructionSize(NewInstruction1(AARCH64_OP(adr), literal), kSize64Bit));

  // Route the result into a destination tmp when this literalref is a ?: / && /
  // || branch (the "-> $n" annotation); otherwise the merge tmp is never
  // written and the value is read uninitialized.
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    result = SetDestOrMove(g, result, dest, AARCH64_OP(mov));
  }

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerAddressOf(AARCH64Generator* g, IRNode* node) {
  TargetInstruction* src = Materialize(g, node->inputs.value.p[0]);
  return SetLoweredNode(node, src);
}

static TargetInstruction* LowerZeroExtend(AARCH64Generator* g, IRNode* node) {
  IRNode* src = node->inputs.value.p[0];
  TargetInstruction* value = Materialize(g, src);
  // The second IR operand is the bit-difference (diff*8), not a usable mask, so
  // derive the mask from the operand types: a zero extension keeps the low
  // min(src,dest) bytes of value and clears the rest.  Using the constant
  // operand directly (e.g. 24 for a char->int extension) produces a wrong mask.
  int src_size = src->type != NULL ? src->type->size : node->type->size;
  int keep_bytes = src_size < node->type->size ? src_size : node->type->size;
  if (keep_bytes >= 8) {
    // No masking required; the value already occupies the full register.
    SetLoweredNode(node, value);
    return value;
  }
  int size = node->type->size > 4 ? kSize64Bit : kSize32Bit;
  // UBFX with a zero lsb is the architectural zero-extension operation.  It
  // replaces the previous mask-materialization plus AND sequence.
  value = Emit(g, SetInstructionSize(
                      NewInstruction3(
                          AARCH64_OP(ubfx), value,
                          GetIntConstant(g, NULL, kTargetType32Bit, 0),
                          GetIntConstant(g, NULL, kTargetType32Bit,
                                         keep_bytes * 8)),
                      size));
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    value = SetDestOrMove(g, value, dest, AARCH64_OP(mov));
  }
  SetLoweredNode(node, value);
  return value;
}

// A post-increment/decrement node may carry a third input: a separate load of
// the old value whose result the surrounding expression still consumes.  When
// the lvalue lives in a register (a register variable), that load aliases the
// variable's register, which the in-place update is about to overwrite.  Copy
// the old value into a fresh temporary and redirect the load's remaining uses
// to it so they observe the pre-update value.
static void PreserveInPlaceOldValue(AARCH64Generator* g, IRNode* node, int size) {
  if (node->inputs.length <= 2) {
    return;
  }
  IRNode* old_load = node->inputs.value.p[2];
  if (old_load == NULL || !HasLoweredNode(old_load)) {
    return;
  }
  TargetInstruction* old = GetLoweredNode(old_load);
  if (old == NULL || !AARCH64IsVarRegister(old)) {
    return;
  }
  AARCH64Opcode copy_op =
      TypeIsFloatingPoint(node->type) ? AARCH64_OP(fmov) : AARCH64_OP(mov);
  TargetInstruction* saved =
      Emit(g, SetInstructionSize(NewInstruction1(copy_op, old), size));
  // SetLoweredNode is intentionally write-once; the load already has its
  // lowered value (the variable register).  Overwrite it directly so the load's
  // remaining consumers observe the preserved copy instead of the register that
  // the in-place update is about to clobber.
  old_load->data.ptr = saved;
}

static TargetInstruction* LowerInc(AARCH64Generator* g, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  AARCH64Opcode ld_opcode;
  AARCH64Opcode st_opcode;
  int size = kSize32Bit;
  switch (node->opcode) {
    case IR_OP(inc8):
      ld_opcode = AARCH64_OP(ldrb);
      st_opcode = AARCH64_OP(strb);
      break;
    case IR_OP(uinc8):
      ld_opcode = AARCH64_OP(ldurb);
      st_opcode = AARCH64_OP(strb);
      break;
    case IR_OP(inc16):
      ld_opcode = AARCH64_OP(ldrh);
      st_opcode = AARCH64_OP(strh);
      break;
    case IR_OP(uinc16):
      ld_opcode = AARCH64_OP(ldurh);
      st_opcode = AARCH64_OP(strh);
      break;
   case IR_OP(inc32):
      ld_opcode = AARCH64_OP(ldr);
      st_opcode = AARCH64_OP(str);
      break;
    case IR_OP(uinc32):
       ld_opcode = AARCH64_OP(ldur);
       st_opcode = AARCH64_OP(str);
       break;
    case IR_OP(inc64):
    case IR_OP(uinc64):
    case IR_OP(inca):
      ld_opcode = AARCH64_OP(ldr);
      st_opcode = AARCH64_OP(str);
      size = kSize64Bit;
     break;
    case IR_OP(incf):
      ld_opcode = AARCH64_OP(fldr);
      st_opcode = AARCH64_OP(fstr);
     break;
    case IR_OP(incd):
      ld_opcode = AARCH64_OP(fldr);
      st_opcode = AARCH64_OP(fstr);
      size = kSize64Bit;
     break;
    default:
      abort();
  }
  TargetInstruction* load = Load(g, addr_node, ld_opcode, size);
  PreserveInPlaceOldValue(g, node, size);
  TargetInstruction* inc;
  IRNode* amount_node = node->inputs.value.p[1];
  TargetInstruction* amount = GetLoweredNode(amount_node);
  if (TypeIsFloatingPoint(node->type)) {
    amount = Materialize(g, amount_node);
    inc =  Emit(g, SetInstructionSize(NewInstruction2(AARCH64_OP(fadd), load, amount), size));
  } else  {
    inc =  AddImmediate(g, load, AARCH64IntValue(amount));
  }
  Store(g, addr_node, inc, st_opcode, size);
  return SetLoweredNode(node, inc);
}


static TargetInstruction* LowerDec(AARCH64Generator* g, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  AARCH64Opcode ld_opcode;
  AARCH64Opcode st_opcode;
  int size = kSize32Bit;

  switch (node->opcode) {
    case IR_OP(dec8):
      ld_opcode = AARCH64_OP(ldrb);
      st_opcode = AARCH64_OP(strb);
      break;
    case IR_OP(udec8):
      ld_opcode = AARCH64_OP(ldurb);
      st_opcode = AARCH64_OP(strb);
      break;
    case IR_OP(dec16):
      ld_opcode = AARCH64_OP(ldrh);
      st_opcode = AARCH64_OP(strh);
      break;
    case IR_OP(udec16):
      ld_opcode = AARCH64_OP(ldurh);
      st_opcode = AARCH64_OP(strh);
      break;
   case IR_OP(dec32):
      ld_opcode = AARCH64_OP(ldr);
      st_opcode = AARCH64_OP(str);
      break;
    case IR_OP(udec32):
       ld_opcode = AARCH64_OP(ldur);
       st_opcode = AARCH64_OP(str);
       break;
    case IR_OP(dec64):
    case IR_OP(udec64):
    case IR_OP(deca):
      ld_opcode = AARCH64_OP(ldr);
      st_opcode = AARCH64_OP(str);
      size = kSize64Bit;
     break;
    case IR_OP(decf):
      ld_opcode = AARCH64_OP(fldr);
      st_opcode = AARCH64_OP(fstr);
     break;
    case IR_OP(decd):
      ld_opcode = AARCH64_OP(fldr);
      st_opcode = AARCH64_OP(fstr);
      size = kSize64Bit;
     break;
    default:
      abort();
  }
  TargetInstruction* load = Load(g, addr_node, ld_opcode, size);
  PreserveInPlaceOldValue(g, node, size);
  TargetInstruction* inc;
  IRNode* amount_node = node->inputs.value.p[1];
  TargetInstruction* amount = GetLoweredNode(amount_node);
  if (TypeIsFloatingPoint(node->type)) {
    amount = Materialize(g, amount_node);
    inc =  Emit(g, SetInstructionSize(NewInstruction2(AARCH64_OP(fsub), load, amount), size));
  } else  {
    inc =  AddImmediate(g, load, -AARCH64IntValue(amount));
  }
  Store(g, addr_node, inc, st_opcode, size);
  return SetLoweredNode(node, inc);
}

// TODO: AARCH64 has bitfield instructions,
static TargetInstruction* LowerGetBitField(AARCH64Generator* g, IRNode* node) {
  TargetInstruction* value = Materialize(g, node->inputs.value.p[0]);
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[1]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[2]);
  if (TypeIsUnsigned(node->type)) {
    // Shift right by bit_pos
    // Mask with bit_size
    TargetInstruction* lsr = Emit(g, NewInstruction2(AARCH64_OP(asr), value, GetIntConstant(g, NULL, kTargetType32Bit, bit_pos)));
    uint64_t mask = bit_size == 64 ? -1LL : (1 << bit_size) - 1;
    TargetInstruction* m = Emit(g, NewInstruction2(AARCH64_OP(and), lsr, GetIntConstant(g, NULL, kTargetType32Bit, mask)));
    SetLoweredNode(node, m);
    return m;
  }
  // Shift left by 64 - (bit_pos + bit_size).  Top bit in bit 63.
  // Shift right by 64 - bit_size.
  TargetInstruction* lsl = Emit(g, NewInstruction2(AARCH64_OP(lsl), value, GetIntConstant(g, NULL, kTargetType32Bit, 64 - (bit_pos + bit_size))));
  TargetInstruction* asr = Emit(g, NewInstruction2(AARCH64_OP(asr), lsl, GetIntConstant(g, NULL, kTargetType32Bit, 64 - bit_size)));

  SetLoweredNode(node, asr);
  return asr;
}

// Shift input left by bit_pos
// Mask input to bit_size bits (in correct position)
// Mask output by ~mask
// Or input into output.
static TargetInstruction* LowerSetBitField(AARCH64Generator* g, IRNode* node) {
  IRNode* output_node = node->inputs.value.p[0];
  IRNode* input_node = node->inputs.value.p[1];
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[2]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[3]);
  uint64_t mask = bit_size == 64 ? -1LL : (1 << bit_size) - 1;
  mask <<= bit_pos;
  
  TargetInstruction* input = Materialize(g, input_node);
  TargetInstruction* output = Materialize(g, output_node);
  TargetInstruction* lsl = Emit(g, NewInstruction2(AARCH64_OP(lsl), input, GetIntConstant(g, NULL, kTargetType32Bit, bit_pos)));
  TargetInstruction* m1 = Emit(g, NewInstruction2(AARCH64_OP(and), lsl, GetIntConstant(g, NULL, kTargetType32Bit, mask)));

  TargetInstruction* m2 = Emit(g, NewInstruction2(AARCH64_OP(and), output, GetIntConstant(g, NULL, kTargetType32Bit, ~mask)));
  TargetInstruction* result = Emit(g, NewInstruction2(AARCH64_OP(orr), m1, m2));
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerSignExtend(AARCH64Generator* g, IRNode* node) {
  TargetInstruction* value = Materialize(g, node->inputs.value.p[0]);
  TargetInstruction* result;
  // diff is (to_size - from_size) * 8: positive when widening, negative when
  // narrowing, and (in either case) the magnitude is the shift amount needed to
  // sign-extend the value held in a register.  Note that unlike RISC-V (whose
  // word loads sign-extend), an AArch64 `ldr w` zero-extends, so a widening
  // sign-extend must actually emit code here.
  IRConstant* diff_value = node->inputs.value.p[1];
  int64_t diff = diff_value->value.ivalue;
  if (AARCH64IsSignedLoad(value) || diff == 0) {
    // Already sign-extended by the load, or no width change at all.
    result = value;
  } else if (diff == 32) {
    // 32-bit value widened to 64 bits: a plain mov would zero-extend (writing a
    // w-register clears the upper 32 bits), turning negative ints into large
    // positives; sxtw performs the arithmetic widening.
    result = Emit(g, SetInstructionSize(NewInstruction1(AARCH64_OP(sxtw), value),
                                        kSize64Bit));
  } else if (diff == -32) {
    // Truncation to a 32-bit result is represented by the low W view of the
    // source.  A later signed widening emits SXTW when a 64-bit representation
    // is required.
    result = value;
  } else {
    int64_t shift = diff < 0 ? -diff : diff;
    // The shift/extend runs in a register as wide as the *larger* of the source
    // and destination types: widening fills the (wider) destination, narrowing
    // shifts within the (wider) source.  shift == abs(diff) is always strictly
    // less than that width, so it is a legal shift amount.
    int to_bits = (node->type != NULL ? (int)node->type->size : 4) * 8;
    int from_bits = to_bits - (int)diff;
    int wide_bits = to_bits > from_bits ? to_bits : from_bits;
    int size = wide_bits > 32 ? kSize64Bit : kSize32Bit;
    TargetInstruction* immed = GetIntConstant(g, NULL, kTargetType32Bit, shift);
    TargetInstruction* lsl = Emit(
        g, SetInstructionSize(NewInstruction2(AARCH64_OP(lsl), value, immed), size));
    result = Emit(
        g, SetInstructionSize(NewInstruction2(AARCH64_OP(asr), lsl, immed), size));
  }
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    if (diff < 0) {
      // A narrowing conversion may feed a fixed W argument/result register.
      // Keep the extension/truncation instruction in its required wider
      // register width and copy only its low destination-width bits afterward;
      // redirecting the instruction itself to the W register can make a legal
      // 64-bit SXTW/shift sequence impossible to encode.
      int dest_size = node->type != NULL && node->type->size > 4
                          ? kSize64Bit
                          : kSize32Bit;
      TargetInstruction* move =
          Emit(g, SetInstructionSize(
                      NewInstruction1(AARCH64_OP(mov), result), dest_size));
      move->dest = dest;
      result = move;
    } else {
      result = SetDestOrMove(g, result, dest, AARCH64_OP(mov));
    }
  }
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerAlign(AARCH64Generator* g, IRNode* node) {
  TargetInstruction* value = Materialize(g, node->inputs.value.p[0]);
  IRConstant* align = node->inputs.value.p[1];

  TargetInstruction* immed = GetIntConstant(g, NULL, kTargetType32Bit, align->value.ivalue - 1);
  TargetInstruction* inv_immed = GetIntConstant(g, NULL, kTargetType32Bit, ~(align->value.ivalue - 1));
  TargetInstruction* add = Emit(g, CopyInstructionSize(NewInstruction2(AARCH64_OP(add), value, immed), 0));
  TargetInstruction* and = Emit(g, CopyInstructionSize(NewInstruction2(AARCH64_OP(and), add, inv_immed), 0));

  SetLoweredNode(node, and);
  return and;
}

static TargetInstruction* PushArg(AARCH64Generator* g, IRNode* node,
                                  TargetInstruction* inst, size_t offset) {
  if (node->type == NULL) {
    // No type, use str instruction.
    return Emit(g, CopyInstructionSize(NewInstruction3(
                        AARCH64_OP(str), inst, StackPointer(g),
                        GetIntConstant(g, NULL, kTargetType64Bit, offset)), 0));
  }
  AARCH64Opcode opcode = AARCH64_OP(str);
  if (TypeIsFloatingPoint(node->type)) {
    opcode = AARCH64_OP(fstr);
  }
  return Emit(g, CopyInstructionSize(NewInstruction3(
                      opcode, inst, StackPointer(g),
                      GetIntConstant(g, NULL, kTargetType64Bit, offset)), 0));
}

static TargetInstruction* PopArg(AARCH64Generator* g, IRNode* node, size_t offset) {
  // Incoming arguments passed on the stack live just above the saved
  // frame-pointer/link-register pair, i.e. at [x29, #16 + offset].  We must
  // address them relative to the frame pointer (not sp) because by the time
  // these loads execute the prologue has already lowered sp by the frame size.
  // Addressing through x29 requires a real stack frame, so this function can
  // no longer be treated as a frameless leaf.
  g->not_leaf = true;
  int64_t fp_offset = 16 + (int64_t)offset;
  if (node->type == NULL) {
    // No type, use ldr instruction.
    return Emit(g, CopyInstructionSize(NewInstruction2(
                        AARCH64_OP(ldr), FramePointer(g),
                        GetIntConstant(g, NULL, kTargetType64Bit, fp_offset)), 0));
  }
  AARCH64Opcode opcode = AARCH64_OP(ldr);
  if (TypeIsFloatingPoint(node->type)) {
    opcode = AARCH64_OP(fldr);
  }
  return Emit(g, CopyInstructionSize(NewInstruction2(
                      opcode, FramePointer(g),
                      GetIntConstant(g, NULL, kTargetType64Bit, fp_offset)), 0));
}


static TargetInstruction* LowerMemcpy(AARCH64Generator* g, IRNode* node) {
  // The memcpy IR node's inputs are the same as those for the memcpy
  // function.  However, there are no load nodes for the desination
  // or source addresses.  If we can do the copy without using a call
  // to memcpy we will do that.

  assert(node->inputs.length == 3);
  assert(IRIsConst(node->inputs.value.p[2]));

  // Source address.
  IRNode* src_node = node->inputs.value.p[1];
  TargetInstruction* src_addr;
  TargetInstruction* src_offset;
  TargetInstruction* src_scale;
  int src_offset_value = 0;
  GetRegAndOffset(g, src_node, &src_addr, &src_offset, &src_scale);
  if (src_offset != NULL) {
    if (!AARCH64IsIntConst(src_offset)) {
      src_addr = AddValue(g, src_addr, src_offset);
      if (src_scale != NULL) {
        // TODO: reduce this to shifts if possible.
        src_addr = Emit(g, NewInstruction2(AARCH64_OP(mul), src_addr, src_offset));
      }
    } else {
      src_offset_value = (int)((TargetConstant*)src_offset)->value.ivalue;
      if (src_scale != NULL) {
        src_offset_value *= TargetIntValue(src_scale);
      }
    }
  }
  src_node->data.ptr = src_addr;

  // Destination address.
  IRNode* dest_node = node->inputs.value.p[0];
  TargetInstruction* dest_addr;
  TargetInstruction* dest_offset;
  TargetInstruction* dest_scale;
  int dest_offset_value = 0;
  GetRegAndOffset(g, dest_node, &dest_addr, &dest_offset, &dest_scale);

  if (dest_offset != NULL) {
    if (!AARCH64IsIntConst(dest_offset)) {
      // We have an address and register for the address, add them together.
      dest_addr = AddValue(g, dest_addr, dest_offset);
      if (dest_scale != NULL) {
        // TODO: reduce this to shifts if possible.
        dest_addr = Emit(g, NewInstruction2(AARCH64_OP(mul), dest_addr, dest_offset));
      }
   } else {
      dest_offset_value = (int)((TargetConstant*)dest_offset)->value.ivalue;
     if (dest_scale != NULL) {
       dest_offset_value *= TargetIntValue(dest_scale);
     }
   }
  }

  int length = (int)((IRConstant*)node->inputs.value.p[2])->value.ivalue;
  TargetInstruction* result = Memcpy(g, dest_addr, src_addr, length,
                                     src_offset_value, dest_offset_value, true);

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerMemzero(AARCH64Generator* g, IRNode* node) {
  // The memzero IR node has one input: the variable to zero.  We
  // emit this is as a call to memset using the size of the symbol unless
  // we can do it more efficiently.
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];
  IRVariable* var = (IRVariable*)addr_node;

  // Dest ddress.
  IRNode* dest_node = node->inputs.value.p[0];
  TargetInstruction* dest_addr;
  TargetInstruction* dest_offset;
  TargetInstruction* scale;
  int offset_value = 0;
  GetRegAndOffset(g, dest_node, &dest_addr, &dest_offset, &scale);

  if (dest_offset != NULL) {
    if (!AARCH64IsIntConst(dest_offset)) {
      // We have an address and register for the address, add them together.
      dest_addr = AddValue(g, dest_addr, dest_offset);
    } else {
      offset_value = (int)((TargetConstant*)dest_offset)->value.ivalue;
    }
  }
  dest_node->data.ptr = dest_addr;
  // Prefer the backing symbol's size, but fall back to the memzero node's type
  // when the destination is a symbol-less slot (e.g. an sret return location).
  int64_t zero_size = (IRIsVariable(addr_node) && var->symbol != NULL)
                          ? var->symbol->type->size
                          : (node->type != NULL ? node->type->size : 0);
  TargetInstruction* result =
      Memzero(g, dest_addr, zero_size, offset_value);

  SetLoweredNode(node, result);
  return result;
}

typedef enum {
  kArgLocationRegister,
  kArgLocationRegisterPair,
  kArgLocationPushed,
  kArgLocationPushedPair,
  kArgLocationPassedByReferenceInRegister,
  kArgLocationPassedByReferenceOnStack,
} ArgLocationType;

typedef struct {
  ArgLocationType type;
  union {
    TargetInstruction* reg;
    size_t offset;
  } location;
  size_t reference_offset;
  TargetInstruction* second_reg;
  size_t second_offset;
} ArgLocation;

static bool TypeUsesAArch64FpArgReg(TypeRecord* type) {
  return TypeIsFloatingPoint(type) ||
         (TypeIsVector(type) && TypeUsesNativeVectorABI(type));
}

static bool TypePassedAsAArch64Aggregate(TypeRecord* type) {
  return TypeIsStructOrUnion(type) ||
         (TypeIsVector(type) && !TypeUsesNativeVectorABI(type));
}

static ArgLocation* NewArgLocationRegister(TargetInstruction* reg) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationRegister;
  loc->location.reg = reg;
  loc->reference_offset = 0;
  loc->second_reg = NULL;
  loc->second_offset = 0;
  return loc;
}

static ArgLocation* NewArgLocationPushed(ArgLocationType type, size_t offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = type;
  loc->location.offset = offset;
  loc->reference_offset = 0;
  loc->second_reg = NULL;
  loc->second_offset = 0;
  return loc;
}

static ArgLocation* NewArgLocationRegisterPair(TargetInstruction* first,
                                               TargetInstruction* second) {
  ArgLocation* loc = NewArgLocationRegister(first);
  loc->type = kArgLocationRegisterPair;
  loc->second_reg = second;
  return loc;
}

static ArgLocation* NewArgLocationPushedPair(size_t first_offset) {
  ArgLocation* loc =
      NewArgLocationPushed(kArgLocationPushedPair, first_offset);
  loc->second_offset = first_offset + 8;
  return loc;
}

static ArgLocation* NewArgLocationReferenceInRegister(TargetInstruction* reg,
                                                      size_t reference_offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationPassedByReferenceInRegister;
  loc->location.reg = reg;
  loc->reference_offset = reference_offset;
  loc->second_reg = NULL;
  loc->second_offset = 0;
  return loc;
}

static ArgLocation* NewArgLocationReferenceOnStack(size_t offset,
                                                   size_t reference_offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationPassedByReferenceOnStack;
  loc->location.offset = offset;
  loc->reference_offset = reference_offset;
  loc->second_reg = NULL;
  loc->second_offset = 0;
  return loc;
}

// Build a list of AARCH64_OP(regarg) instructions to hold the
// argument registers and allow their liveness to extend
// to the point of call.  Each instruction uses two operands:
// 0: the next regarg instruction.
// 1: the rmov instruction that assigns to the
//    argument register.
// This forms a linked list whose head is the second operand to the
// call instruction.
//
// The purpose of this list is to allow the register allocator to know
// that the registers used as arguments (a0..a7, fa0..fa7) are allocated
// until the call instruction executes, than can be freed.
static TargetInstruction* BuildArgList(AARCH64Generator* g, Vector* arg_locations) {
  TargetInstruction* result = NULL;
  // Find next -based argument and add it to the regargs instruction list.
  for (size_t i = 0; i < arg_locations->length; i++) {
    ArgLocation* loc = arg_locations->value.p[i];
    if (loc->type == kArgLocationRegister) {
      result =
          Emit(g, NewInstruction2(AARCH64_OP(regarg), result, loc->location.reg));
    } else if (loc->type == kArgLocationRegisterPair) {
      result =
          Emit(g, NewInstruction2(AARCH64_OP(regarg), result, loc->location.reg));
      result = Emit(
          g, NewInstruction2(AARCH64_OP(regarg), result, loc->second_reg));
    }
  }
  return result;
}

// AAPCS64 provides eight general-purpose argument registers (x0-x7) and eight
// SIMD/floating-point argument registers (v0-v7); x8 carries the indirect
// result address for an aggregate return.  Aggregate classification also
// depends on size and contents (including HFA/HVA rules):
// https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst
//
// This lowering implements the subset represented by ArgLocation: aggregates
// up to 8 bytes use one general-purpose register, member-pointer aggregates use
// a register pair, and larger aggregates are copied to caller-owned stack
// storage before their address is passed.  Ordinary 9-16-byte aggregate
// register pairs and HFA/HVA classification require extending ArgLocation and
// the corresponding callee argument reconstruction together.
//
// Variadic arguments follow the normal AAPCS64 rules: integer/pointer
// arguments in x0-x7, floating-point arguments in d0-d7, and the remainder on
// the stack.  The callee's prologue saves the unnamed argument registers into
// the GP and VR save areas that va_arg walks (see LowerBuiltinVaStart).
static TargetInstruction* LowerCall(AARCH64Generator* g, IRNode* node) {
  assert(node->inputs.length >= 1);
  size_t struct_area_size = 0;
  int next_int_arg_reg = 0;
  int next_fp_arg_reg = 0;
  size_t next_pushed_arg_offset = 0;
  Vector arg_locations;
  VectorInit(&arg_locations);
  bool has_hidden_struct_result = (node->flags & kIRStructReturnCall) != 0;

  // Phase 1:
  // Work out the locations for all arguments.  The first 8 go in argument
  // registers, split into integer and floating point sets.
  for (size_t i = 1; i < node->inputs.length; i++) {
    IRNode* arg_node = node->inputs.value.p[i];
    bool is_hidden_struct_result = i == 1 && has_hidden_struct_result;
    if (is_hidden_struct_result) {
      // The first synthetic operand of a call returning an aggregate is its
      // indirect-result address.  AAPCS64 assigns it to x8 regardless of the
      // source expression used for that address.
      VectorAppend(&arg_locations,
                   NewArgLocationRegister(StructReturnArgumentRegister(g)));
    } else if (TypeIsMemberPointerAggregate(arg_node->type)) {
      if (next_int_arg_reg + 1 < AARCH64_NUM_INT_ARGS) {
        TargetInstruction* first =
            FreshIntArgumentRegister(g, next_int_arg_reg++);
        TargetInstruction* second =
            FreshIntArgumentRegister(g, next_int_arg_reg++);
        VectorAppend(&arg_locations,
                     NewArgLocationRegisterPair(first, second));
      } else {
        VectorAppend(&arg_locations,
                     NewArgLocationPushedPair(next_pushed_arg_offset));
        next_pushed_arg_offset += 16;
      }
    } else if (TypePassedAsAArch64Aggregate(arg_node->type)) {
      // Struct, union, or non-native vector that fits in a register is passed
      // in a register.  Wider values are copied and passed by reference.
      size_t struct_size = arg_node->type->size;
      if (struct_size <= 8) {
        if (next_int_arg_reg < AARCH64_NUM_INT_ARGS) {
          // Argument goes in an argument register.
          TargetInstruction* arg_reg =
              FreshIntArgumentRegister(g, next_int_arg_reg++);
          VectorAppend(&arg_locations, NewArgLocationRegister(arg_reg));
        } else {
          // Need to push argument on to the stack.  But we do that in reverse
          // order so for now, we record that the arg location is on the stack.
          VectorAppend(
              &arg_locations,
              NewArgLocationPushed(kArgLocationPushed, next_pushed_arg_offset));
          next_pushed_arg_offset += 8;
        }
      } else {
        // The struct needs to be copied onto the stack and then its address
        // passed either in a register or on the stack.
        if (next_int_arg_reg < AARCH64_NUM_INT_ARGS) {
          // Argument goes in an argument register.
          TargetInstruction* arg_reg =
              FreshIntArgumentRegister(g, next_int_arg_reg++);
          VectorAppend(&arg_locations, NewArgLocationReferenceInRegister(
                                           arg_reg, struct_area_size));
        } else {
          VectorAppend(&arg_locations,
                       NewArgLocationReferenceOnStack(next_pushed_arg_offset,
                                                      struct_area_size));
          next_pushed_arg_offset += 8;
        }
        struct_area_size += struct_size;
      }
    } else if (TypeUsesAArch64FpArgReg(arg_node->type)) {
      if (next_fp_arg_reg < AARCH64_NUM_FP_ARGS) {
        // Argument goes in an argument register.
        TargetInstruction* arg_reg =
            FreshFpArgumentRegister(g, next_fp_arg_reg++);
        VectorAppend(&arg_locations, NewArgLocationRegister(arg_reg));
      } else {
        // Need to push argument on to the stack.  But we do that in reverse
        // order so for now, we record that the arg location is on the stack.
        VectorAppend(
            &arg_locations,
            NewArgLocationPushed(kArgLocationPushed, next_pushed_arg_offset));
        next_pushed_arg_offset += 8;
      }
    } else {
      if (next_int_arg_reg < AARCH64_NUM_INT_ARGS) {
        // Argument goes in an argument register.
        TargetInstruction* arg_reg =
            FreshIntArgumentRegister(g, next_int_arg_reg++);
        VectorAppend(&arg_locations, NewArgLocationRegister(arg_reg));
      } else {
        // Need to push argument on to the stack.  But we do that in reverse
        // order so for now, we record that the arg location is on the stack.
        VectorAppend(
            &arg_locations,
            NewArgLocationPushed(kArgLocationPushed, next_pushed_arg_offset));
        next_pushed_arg_offset += 8;
      }
    }
  }

  // Phase 2:
  // Decrement the stack pointer to make space for the stack args
  size_t total_stack_size = struct_area_size + next_pushed_arg_offset;
  if (total_stack_size > 0) {
    TargetInstruction* newsp =
        AddImmediate(g, StackPointer(g), -total_stack_size);
    TargetSetDest(newsp, StackPointer(g));
  }

  // Phase 3:
  // Struct and unions that are bigger than 8 bytes are copied onto the
  // stack and passed by reference.  We need to copy all of these onto the
  // stack before we push all other arguments
  for (size_t i = 1; i < node->inputs.length; i++) {
    ArgLocation* arg_location = arg_locations.value.p[i - 1];
    IRNode* arg_node = node->inputs.value.p[i];
    size_t size = arg_node->type->size;
    switch (arg_location->type) {
      case kArgLocationPassedByReferenceInRegister:
      case kArgLocationPassedByReferenceOnStack: {
        TargetInstruction* arg = Materialize(g, arg_node);
        Memcpy(g, StackPointer(g), arg, (int)size, 0,
               (int)(arg_location->reference_offset + next_pushed_arg_offset), false);
        break;
      }
      default:
        break;
    }
  }

  // Phase 3.5:
  // For an indirect call (the target is computed into a register rather than
  // being a link-time symbol) stage the target into its own register *before*
  // the argument registers are set up below.  The blr below references this
  // staged value, so the register allocator keeps it live across the argument
  // moves and won't reuse its register for an argument (e.g. x0).  Without
  // this, the call target and the first argument can land in the same
  // register and the argument move clobbers the target.
  TargetInstruction* staged_target = NULL;
  {
    IRNode* target_node = node->inputs.value.p[0];
    TargetInstruction* a = GetLoweredNode(target_node);
    if (((int)a->opcode != (int)AARCH64_OP(symbol))) {
      // Force the target into the dedicated temp register (x9), which is not
      // an argument register, so the argument moves below cannot clobber it.
      // Prefer giving the target instruction itself the temp as its
      // destination (no extra move, and no dependence on the target's old
      // register surviving argument setup).  Only do this when every use of
      // the target value is in this block, so we don't redirect a value that
      // is read after the (register-clobbering) call.
      bool single_block = true;
      for (size_t u = 0; u < target_node->outputs.length; u++) {
        if (((IRNode*)target_node->outputs.value.p[u])->block !=
            target_node->block) {
          single_block = false;
          break;
        }
      }
      if (single_block) {
        staged_target = SetDestOrMove(g, a, Tmp(g), AARCH64_OP(mov));
      } else {
        TargetInstruction* mv =
            Emit(g, CopyInstructionSize(NewInstruction1(AARCH64_OP(mov), a), 0));
        mv->dest = Tmp(g);
        staged_target = Tmp(g);
      }
    }
  }

  // Capture register arguments before assigning any physical argument
  // registers. Argument expressions can themselves still reside in x0..x7
  // (especially forwarding calls such as gt(a, b) -> lt(b, a)); moving one
  // argument directly would otherwise destroy a later argument in a register
  // cycle.
  Vector staged_register_args;
  VectorInit(&staged_register_args);
  for (size_t i = 1; i < node->inputs.length; i++) {
    VectorAppend(&staged_register_args, NULL);
  }
  for (size_t i = 1; i < node->inputs.length; i++) {
    ArgLocation* arg_location = arg_locations.value.p[i - 1];
    if (arg_location->type != kArgLocationRegister) {
      continue;
    }
    IRNode* arg_node = node->inputs.value.p[i];
    TargetInstruction* arg = Materialize(g, arg_node);
    if (TypeIsVector(arg_node->type) && TypeUsesNativeVectorABI(arg_node->type)) {
      TargetInstruction* staged = Emit(
          g, SetInstructionSize(
                 NewInstruction2(
                     AARCH64_OP(fldr), arg,
                     GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                 kSize64Bit));
      VectorSet(&staged_register_args, i - 1, staged);
      continue;
    }
    if (TypePassedAsAArch64Aggregate(arg_node->type) && arg_node->type->size <= 8) {
      arg = Emit(g, CopyInstructionSize(
                        NewInstruction2(
                            AARCH64_OP(ldr), arg,
                            GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                        0));
    }
    AARCH64Opcode move_opcode = TypeIsFloatingPoint(arg_node->type)
                                    ? AARCH64_OP(fmov)
                                    : AARCH64_OP(mov);
    TargetInstruction* staged = Emit(g, NewInstruction(AARCH64_OP(tmp)));
    TargetInstruction* move =
        Emit(g, CopyInstructionSize(NewInstruction1(move_opcode, arg), 0));
    move->dest = staged;
    VectorSet(&staged_register_args, i - 1, staged);
  }

  // Phase 4:
  // Pass through all args, in reverse order, pushing those not passed in
  // registers and moving the register arguments into their argument
  // registers.
  //
  // TODO: figure out if we can just set the dest to the reg->base.
  for (size_t i = node->inputs.length - 1; i >= 1; i--) {
    IRNode* arg_node = node->inputs.value.p[i];
    ArgLocation* arg_location = arg_locations.value.p[i - 1];
    switch (arg_location->type) {
      case kArgLocationRegisterPair: {
        TargetInstruction* address = Materialize(g, arg_node);
        SetInstructionSize(address, kSize64Bit);
        TargetInstruction* first = Emit(
            g, SetInstructionSize(
                   NewInstruction2(
                       AARCH64_OP(ldr), address,
                       GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                   kSize64Bit));
        TargetInstruction* second = Emit(
            g, SetInstructionSize(
                   NewInstruction2(
                       AARCH64_OP(ldr), address,
                       GetIntConstant(g, NULL, kTargetType32Bit, 8)),
                   kSize64Bit));
        SetDestOrMoveToArgReg(g, arg_node, first,
                              arg_location->location.reg, AARCH64_OP(mov));
        SetDestOrMoveToArgReg(g, arg_node, second, arg_location->second_reg,
                              AARCH64_OP(mov));
        break;
      }
      case kArgLocationPushedPair: {
        TargetInstruction* address = Materialize(g, arg_node);
        SetInstructionSize(address, kSize64Bit);
        TargetInstruction* first = Emit(
            g, SetInstructionSize(
                   NewInstruction2(
                       AARCH64_OP(ldr), address,
                       GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                   kSize64Bit));
        TargetInstruction* second = Emit(
            g, SetInstructionSize(
                   NewInstruction2(
                       AARCH64_OP(ldr), address,
                       GetIntConstant(g, NULL, kTargetType32Bit, 8)),
                   kSize64Bit));
        StoreImmediate(g, AARCH64_OP(str), first, StackPointer(g),
                       (int32_t)arg_location->location.offset);
        StoreImmediate(g, AARCH64_OP(str), second, StackPointer(g),
                       (int32_t)arg_location->second_offset);
        break;
      }
      case kArgLocationPassedByReferenceInRegister: {
        // Struct passed by reference in a register.  The reference_offset
        // contains the offset from the to of the pushed args to the copied
        // struct.
        TargetInstruction* arg = AddImmediate(
            g, StackPointer(g),
            arg_location->reference_offset + next_pushed_arg_offset);
        SetDestOrMoveToArgReg(g, arg_node, arg, arg_location->location.reg, AARCH64_OP(mov));
        break;
      }
      case kArgLocationPassedByReferenceOnStack: {
        // Struct passed by reference on the stack.
        TargetInstruction* arg = AddImmediate(
            g, StackPointer(g),
            arg_location->reference_offset + next_pushed_arg_offset);
        PushArg(g, arg_node, arg, arg_location->location.offset);
        break;
      }
      case kArgLocationPushed: {
        TargetInstruction* arg = Materialize(g, arg_node);
        if (TypeIsVector(arg_node->type) && TypeUsesNativeVectorABI(arg_node->type)) {
          arg = Emit(g, SetInstructionSize(
                            NewInstruction2(
                                AARCH64_OP(ldr), arg,
                                GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                            kSize64Bit));
        }
        if (TypePassedAsAArch64Aggregate(arg_node->type)) {
          size_t size = arg_node->type->size;
          if (size <= 8) {
            // A struct less than 8 bytes is passed directly on stack.  The
            // Materialize call will result in the address of the struct.  We
            // need to load it.
            arg = Emit(g, CopyInstructionSize(NewInstruction2(
                               AARCH64_OP(ldr), arg,
                               GetIntConstant(g, NULL, kTargetType32Bit, 0)), 0));
          }
        }
        PushArg(g, arg_node, arg, arg_location->location.offset);
        break;
      }
      case kArgLocationRegister: {
        // Argument is in a register.
        TargetInstruction* arg =
            VectorGet(&staged_register_args, i - 1);
        assert(arg != NULL);
        AARCH64Opcode mov_opcode = AARCH64_OP(mov);
        if (TypeUsesAArch64FpArgReg(arg_node->type)) {
          mov_opcode = AARCH64_OP(fmov);
        }
        // Emit(g, NewInstruction2(mov_opcode, arg_location->location.reg, arg));
        SetDestOrMoveToArgReg(g, arg_node, arg, arg_location->location.reg, mov_opcode);
        break;
      }
    }
  }

  // Finally emit the call instruction containing the address
  // to call, as its first operand and a linked list of regarg
  // pseudo-instructions as its second operand.  This list makes
  // the lifetime of the registers allocated for argument passing
  // extend to the call site, thus enabling the register allocator
  // to keep them from being used before the call.
  TargetInstruction* addr = GetLoweredNode(node->inputs.value.p[0]);
  AARCH64Opcode opcode;
  TargetInstruction* call;
  
  // If the node has been identified as a tail call by the IR
  // optimizer we might be able to convert it to a jump.
  //
  // Possible optimization: if the arguments don't contain an address in
  // the current stack frame we can allow tail calls when we have space
  // allocated on the stack.  I don't know how to detect that though.
  bool can_be_tail_call = (node->flags & kIRTailCall) != 0 &&
      total_stack_size == 0 && g->base.stack_frame_size == 0;

  if (can_be_tail_call) {
    // Tail call.
    // Add a restore instruction and replace the call with a jump.  Only a
    // direct call to a known function symbol can use the symbol-relative
    // branch; an indirect target (a function pointer in a register, a computed
    // address, or even a constant) must be materialized into a temp register
    // and reached with a register branch.
    if (((int)addr->opcode != (int)AARCH64_OP(symbol))) {
      // Indirect tail call.  The target was staged into the temp register (x9)
      // in phase 3.5, before the argument registers were set up, so it survives
      // both the argument moves and the restore.  (The address may itself be a
      // caller-saved register such as the x0 result of a preceding call, as in
      // `(*(*p)(...))(...)`; capturing it after the argument moves would read a
      // clobbered register.)
      assert(staged_target != NULL);
      BuildArgList(g, &arg_locations);
      Emit(g, NewInstruction(AARCH64_OP(restore)));
      call = Emit(g, NewInstruction1(AARCH64_OP(br), staged_target));
    } else {
      Emit(g, NewInstruction(AARCH64_OP(restore)));
      BuildArgList(g, &arg_locations);
      call = Emit(g, NewInstruction2(AARCH64_OP(b), Condition(g, AARCH64_OP(al), 0), addr));
    }
    g->base.num_calls--;
  } else {
    TargetInstruction* call_target = addr;
    if (staged_target != NULL) {
      // Indirect call: target was staged into its own register above.
      opcode = AARCH64_OP(blr);
      call_target = staged_target;
    } else if (((int)addr->opcode == (int)AARCH64_OP(symbol))) {
      // Calling a symbol, use a regular 'call' instruction.
      opcode = TypeIsFloatingPoint(node->type) ? AARCH64_OP(bl) : AARCH64_OP(bl);
    } else {
      // Calling through a register, rcall.
      opcode = TypeIsFloatingPoint(node->type) ? AARCH64_OP(blr) : AARCH64_OP(blr);
    }
    call =
        Emit(g, NewInstruction2(opcode, call_target, BuildArgList(g, &arg_locations)));
    if (TypeUsesAArch64FpArgReg(node->type)) {
      // The value comes back in the floating-point return register (d0), not
      // x0; tell the register allocator so the result isn't read from x0.
      call->flags |= AARCH64_INST_FP_RETURN;
    }

    // Increment the stack pointer again to remove pushed args.
    if (total_stack_size > 0) {
      TargetInstruction* newsp =
          AddImmediate(g, StackPointer(g), total_stack_size);
      TargetSetDest(newsp, StackPointer(g));
    }
  }
  // If the call result feeds a merge destination (the "-> $n" annotation used
  // to funnel the two arms of &&/||/?: into one location), route the result
  // there.  Without this the call's return value (in x0) is dropped and the
  // merge slot keeps the other arm's stale value, e.g. `x || f()` yields x
  // instead of f()'s result.  bl/blr are not expressions, so SetDestOrMove
  // emits an explicit move rather than redirecting the call's x0 output.
  if (node->dest != NULL) {
    TargetInstruction* dest = GetDestInstruction(g, node);
    if (dest != NULL) {
      // A floating-point result returns in d0 and must be copied with fmov;
      // using the integer mov here corrupts the value.
      AARCH64Opcode mov_opcode = TypeIsFloatingPoint(node->type)
                                     ? AARCH64_OP(fmov)
                                     : AARCH64_OP(mov);
      call = SetDestOrMove(g, call, dest, mov_opcode);
    }
  }
  SetLoweredNode(node, call);

  VectorDestruct(&staged_register_args);
  VectorDestructWithContents(&arg_locations, NULL, /*free_element=*/true);
  return call;
}

// A computed branch is used to branch to a dense switch table consisting
// of a sequence of 'b' instructions to the case labels.  Each instruction
// is 4 bytes long->base.  The instruction sequence for the computed branch is:
//
// entry: t0 = index into table.
// adr t1, 12                  - pc at start of table
// add t1, t1, t0, lsl #2      - address of jump instruction
// br t1
// // Table is here.
// b l1
// b l2
// ...
// b lx

static TargetInstruction* LowerComputedBranch(AARCH64Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  TargetInstruction* value = GetLoweredNode(node->inputs.value.p[0]);

  TargetInstruction* adr =
      Emit(g, SetInstructionSize(NewInstruction1(AARCH64_OP(adr),
                               GetIntConstant(g, NULL, kTargetType32Bit, 12)), kSize64Bit));
  TargetInstruction* add = Emit(g, CopyInstructionSize(NewInstruction4(AARCH64_OP(add), adr, value, lsl(g),
                                                     GetIntConstant(g, NULL, kTargetType32Bit, 2)), 0));
  TargetInstruction* br =
      Emit(g, NewInstruction1(AARCH64_OP(br), add));
  br->flags |= TARGET_INST_TABLE_JUMP;
  SetLoweredNode(node, br);
  return br;

}

// The first input is the address of the 'ap' variable.  The second is the
// address of the last function argument (ignored in RISC-V).  This
// simply stores the value of the frame pointer in the address passed
// in the first input.
// AAPCS64 va_list field byte offsets (see __builtin_va_list in compiler.c).
#define AARCH64_VA_STACK 0     // void*  next stack argument
#define AARCH64_VA_GR_TOP 8    // void*  one past the GP register save area
#define AARCH64_VA_VR_TOP 16   // void*  one past the VR register save area
#define AARCH64_VA_GR_OFFS 24  // int    negative offset from __gr_top
#define AARCH64_VA_VR_OFFS 28  // int    negative offset from __vr_top

// Return a single register holding the address of the va_list structure
// referenced by addr_node (which is &ap or *ap).  Field accesses then use small
// in-range immediate offsets 0..28 relative to it.
static TargetInstruction* VaListAddress(AARCH64Generator* g, IRNode* addr_node) {
  TargetInstruction* base;
  TargetInstruction* offset;
  TargetInstruction* scale;
  GetRegAndOffset(g, addr_node, &base, &offset, &scale);
  int off = (offset != NULL && TargetIsConst(offset)) ? AARCH64IntValue(offset) : 0;
  return (off == 0) ? base : AddImmediate(g, base, off);
}

static TargetInstruction* VaLoad(AARCH64Generator* g, TargetInstruction* ap,
                                 int field, int size) {
  return Emit(g, SetInstructionSize(
                     NewInstruction2(AARCH64_OP(ldr), ap,
                                     GetIntConstant(g, NULL, kTargetType32Bit, field)),
                     size));
}

static void VaStore(AARCH64Generator* g, TargetInstruction* value,
                    TargetInstruction* ap, int field, int size) {
  Emit(g, SetInstructionSize(
              NewInstruction3(AARCH64_OP(str), value, ap,
                              GetIntConstant(g, NULL, kTargetType32Bit, field)),
              size));
}

// va_start initialises the AAPCS64 va_list.  The prologue (see
// aarch64_emitter.c) lowered x29 so the GP and VR register save areas sit
// immediately above it; the caller's on-stack overflow arguments sit above
// those plus the saved x29/x30 pair (16 bytes).
static TargetInstruction* LowerBuiltinVaStart(AARCH64Generator* g, IRNode* node) {
  TargetInstruction* ap = VaListAddress(g, node->inputs.value.p[0]);

  int gp_size = (AARCH64_NUM_INT_ARGS - g->num_int_arg_regs) * 8;
  if (gp_size < 0) gp_size = 0;
  int vr_size = (AARCH64_NUM_FP_ARGS - g->num_fp_arg_regs) * 16;
  if (vr_size < 0) vr_size = 0;

  TargetInstruction* x29 =
      Emit(g, SetInstructionSize(NewInstruction(AARCH64_OP(fp)), kSize64Bit));

  // __stack = x29 + 16 + gp_size + vr_size  (start of the overflow area).
  VaStore(g, AddImmediate(g, x29, 16 + gp_size + vr_size), ap, AARCH64_VA_STACK,
          kSize64Bit);
  // __gr_top = x29 + gp_size  (one past the GP save area).
  VaStore(g, AddImmediate(g, x29, gp_size), ap, AARCH64_VA_GR_TOP, kSize64Bit);
  // __vr_top = x29 + gp_size + vr_size  (one past the VR save area).
  VaStore(g, AddImmediate(g, x29, gp_size + vr_size), ap, AARCH64_VA_VR_TOP,
          kSize64Bit);
  // __gr_offs = -gp_size, __vr_offs = -vr_size (negative until exhausted).
  VaStore(g,
          MoveImmediate(g, GetIntConstant(g, NULL, kTargetType32Bit, -gp_size),
                        kSize32Bit),
          ap, AARCH64_VA_GR_OFFS, kSize32Bit);
  VaStore(g,
          MoveImmediate(g, GetIntConstant(g, NULL, kTargetType32Bit, -vr_size),
                        kSize32Bit),
          ap, AARCH64_VA_VR_OFFS, kSize32Bit);
  return SetLoweredNode(node, x29);
}

// va_arg implements the AAPCS64 algorithm branchlessly with csel:
//   reg_offs = ap->__{gr,vr}_offs
//   addr     = (reg_offs >= 0) ? ap->__stack : ap->__{gr,vr}_top + reg_offs
//   ap->__{gr,vr}_offs += step  (8 for GP, 16 for VR)
//   ap->__stack        += (reg_offs >= 0) ? 8 : 0
//   result   = *addr
static TargetInstruction* LowerBuiltinVaArg(AARCH64Generator* g, IRNode* node) {
  TargetInstruction* ap = VaListAddress(g, node->inputs.value.p[0]);
  bool is_fp = TypeIsFloatingPoint(node->type);
  int top_field = is_fp ? AARCH64_VA_VR_TOP : AARCH64_VA_GR_TOP;
  int offs_field = is_fp ? AARCH64_VA_VR_OFFS : AARCH64_VA_GR_OFFS;
  int step = is_fp ? 16 : 8;  // VR slots are 16 bytes, GP slots 8.

  TargetInstruction* reg_offs = VaLoad(g, ap, offs_field, kSize32Bit);
  TargetInstruction* reg_top = VaLoad(g, ap, top_field, kSize64Bit);
  TargetInstruction* reg_offs64 = Emit(
      g, SetInstructionSize(NewInstruction1(AARCH64_OP(sxtw), reg_offs), kSize64Bit));
  TargetInstruction* reg_addr = Emit(
      g, SetInstructionSize(NewInstruction2(AARCH64_OP(add), reg_top, reg_offs64),
                            kSize64Bit));
  TargetInstruction* stack = VaLoad(g, ap, AARCH64_VA_STACK, kSize64Bit);

  // Use the on-stack overflow area once the register save area is exhausted
  // (reg_offs has counted up to >= 0).
  Emit(g, SetInstructionSize(
              NewInstruction2(AARCH64_OP(cmp), reg_offs,
                              GetIntConstant(g, NULL, kTargetType32Bit, 0)),
              kSize32Bit));
  TargetInstruction* addr = Emit(
      g, SetInstructionSize(
             NewInstruction3(AARCH64_OP(csel), stack, reg_addr,
                             Condition(g, AARCH64_OP(ge), kSize64Bit)),
             kSize64Bit));

  // Advance the register offset (harmless if we used the stack: it only grows
  // further past zero) and the stack pointer (only when the stack was used).
  VaStore(g, AddImmediate(g, reg_offs, step), ap, offs_field, kSize32Bit);
  TargetInstruction* new_stack = AddImmediate(g, stack, 8);
  TargetInstruction* final_stack = Emit(
      g, SetInstructionSize(
             NewInstruction3(AARCH64_OP(csel), new_stack, stack,
                             Condition(g, AARCH64_OP(ge), kSize64Bit)),
             kSize64Bit));
  VaStore(g, final_stack, ap, AARCH64_VA_STACK, kSize64Bit);

  if (TypeIsStructOrUnion(node->type)) {
    // Aggregates are referenced by address.  A struct/union that fits in a
    // single 8-byte general slot is passed by value, so the save-area slot
    // *is* the struct: its address is the slot address.  A larger aggregate is
    // passed by reference, so the slot holds a pointer to the caller's copy;
    // load that pointer and use it as the address.
    if (node->type->size <= 8) {
      return SetLoweredNode(node, addr);
    }
    TargetInstruction* ptr = Emit(
        g, SetInstructionSize(
               NewInstruction2(AARCH64_OP(ldr), addr,
                               GetIntConstant(g, NULL, kTargetType32Bit, 0)),
               kSize64Bit));
    return SetLoweredNode(node, ptr);
  }

  AARCH64Opcode load_op = is_fp ? AARCH64_OP(fldr) : AARCH64_OP(ldr);
  int load_size = (node->type->size > 4) ? kSize64Bit : kSize32Bit;
  TargetInstruction* result = Emit(
      g, SetInstructionSize(
             NewInstruction2(load_op, addr,
                             GetIntConstant(g, NULL, kTargetType32Bit, 0)),
             load_size));
  return SetLoweredNode(node, result);
}

static TargetInstruction* LowerBuiltinVaEnd(AARCH64Generator* g, IRNode* node) {
  // Nothing to do for va_end.
  return NULL;
}

static TargetInstruction* LowerBuiltinVaCopy(AARCH64Generator* g, IRNode* node) {
  // Copy the 32-byte va_list structure from src (input 1) to dst (input 0).
  TargetInstruction* dst = VaListAddress(g, node->inputs.value.p[0]);
  TargetInstruction* src = VaListAddress(g, node->inputs.value.p[1]);
  for (int o = 0; o < 32; o += 8) {
    VaStore(g, VaLoad(g, src, o, kSize64Bit), dst, o, kSize64Bit);
  }
  return NULL;
}

static TargetInstruction* LowerLocation(AARCH64Generator* g, IRNode* node) {
  IRLocation* loc = (IRLocation*)node;
  return SetLoweredNode(node, Emit(g, TargetNewLocation(loc)));
}

static TargetInstruction* LowerStackPointerOps(AARCH64Generator* g, IRNode* node) {
  switch (node->opcode) {
    case IR_OP(decsp): {
      TargetInstruction* size = Materialize(g, node->inputs.value.p[0]);
      TargetInstruction* new_sp;
      if (TargetIsConst(size)) {
        int64_t s = AARCH64IntValue(size);
        new_sp = AddImmediate(g, StackPointer(g), -s);
      } else {
        // AArch64's shifted-register add/sub encoding treats register 31 as
        // XZR, not SP. Copy SP through a general register for a runtime-sized
        // decrement, then write the result back with the SP-capable move
        // encoding.
        TargetInstruction* current_sp = Emit(
            g, CopyInstructionSize(
                   NewInstruction1(AARCH64_OP(mov), StackPointer(g)), 0));
        new_sp = Emit(
            g, CopyInstructionSize(
                   NewInstruction2(AARCH64_OP(sub), current_sp, size), 0));
        TargetInstruction* set_sp = Emit(
            g, CopyInstructionSize(
                   NewInstruction1(AARCH64_OP(mov), new_sp), 0));
        set_sp->dest = StackPointer(g);
        return set_sp;
      }
      new_sp->dest = StackPointer(g);
      return new_sp;
    }
    case IR_OP(savesp): {
      // One operand, a temp to hold stack pointer.  The move has to reach the
      // instruction stream: without it the temp keeps whatever the register
      // happened to hold, which is where a variable-length array's base address
      // was coming from.
      TargetInstruction* tmp = Materialize(g, node->inputs.value.p[0]);
      TargetInstruction* mv = Emit(
          g, CopyInstructionSize(
                 NewInstruction1(AARCH64_OP(mov), StackPointer(g)), 0));
      mv->dest = tmp;
      return tmp;
    }
    case IR_OP(restoresp): {
      TargetInstruction* tmp = Materialize(g, node->inputs.value.p[0]);
      TargetInstruction* mv = Emit(
          g, CopyInstructionSize(NewInstruction1(AARCH64_OP(mov), tmp), 0));
      mv->dest = StackPointer(g);
      return mv->dest;
    }
    default:
      assert(false);
      return NULL;
  }
}

static int AtomicIRConstant(IRNode* node) {
  assert(node != NULL && IRIsConst(node));
  return (int)((IRConstant*)node)->value.ivalue;
}

static int AtomicAccessLog2(TypeRecord* type) {
  assert(type != NULL);
  switch (type->size) {
    case 1: return 0;
    case 2: return 1;
    case 4: return 2;
    case 8: return 3;
    default:
      assert(false && "AArch64 atomics require a 1/2/4/8-byte object");
      return 2;
  }
}

static TargetInstruction* AtomicAddress(AARCH64Generator* g,
                                        IRNode* addr_node) {
  TargetInstruction* base;
  TargetInstruction* offset;
  TargetInstruction* scale;
  GetRegAndOffset(g, addr_node, &base, &offset, &scale);
  int off =
      offset != NULL && TargetIsConst(offset) ? AARCH64IntValue(offset) : 0;
  return off == 0 ? base : AddImmediate(g, base, off);
}

static void SetAtomicMetadata(TargetInstruction* inst, TypeRecord* value_type,
                              int success_order, int failure_order,
                              bool weak) {
  inst->flags |= AtomicAccessLog2(value_type) << AARCH64_ATOMIC_SIZE_SHIFT;
  inst->flags |= success_order << AARCH64_ATOMIC_ORDER_SHIFT;
  inst->flags |= failure_order << AARCH64_ATOMIC_FAILURE_ORDER_SHIFT;
  if (weak) {
    inst->flags |= AARCH64_ATOMIC_WEAK;
  }
}

static TargetInstruction* FinishAtomicValue(AARCH64Generator* g, IRNode* node,
                                             TargetInstruction* inst) {
  CopyOrSetInstructionSize(node, inst);
  Emit(g, inst);
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    inst = SetDestOrMove(g, inst, dest, AARCH64_OP(mov));
  }
  return SetLoweredNode(node, inst);
}

static TargetInstruction* LowerAtomic(AARCH64Generator* g, IRNode* node) {
  if (node->opcode == IR_OP(atomic_fence)) {
    TargetInstruction* inst = NewInstruction(AARCH64_OP(atomic_fence));
    SetAtomicMetadata(inst, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                      AtomicIRConstant(node->inputs.value.p[0]), 0, false);
    Emit(g, inst);
    return SetLoweredNode(node, inst);
  }

  IRNode* addr_node = node->inputs.value.p[0];
  TypeRecord* value_type = addr_node->type->next;
  assert(value_type != NULL);
  TargetInstruction* addr = AtomicAddress(g, addr_node);
  TargetInstruction* inst = NULL;

  switch (node->opcode) {
    case IR_OP(atomic_load):
      inst = NewInstruction1(AARCH64_OP(atomic_load), addr);
      SetAtomicMetadata(inst, value_type,
                        AtomicIRConstant(node->inputs.value.p[1]), 0, false);
      return FinishAtomicValue(g, node, inst);

    case IR_OP(atomic_store): {
      TargetInstruction* value = Materialize(g, node->inputs.value.p[1]);
      inst = NewInstruction2(AARCH64_OP(atomic_store), value, addr);
      SetAtomicMetadata(inst, value_type,
                        AtomicIRConstant(node->inputs.value.p[2]), 0, false);
      Emit(g, inst);
      return SetLoweredNode(node, inst);
    }

    case IR_OP(atomic_fetch_add):
    case IR_OP(atomic_fetch_sub):
    case IR_OP(atomic_add_fetch):
    case IR_OP(atomic_sub_fetch): {
      AARCH64Opcode opcode =
          node->opcode == IR_OP(atomic_fetch_add)
              ? AARCH64_OP(atomic_fetch_add)
              : node->opcode == IR_OP(atomic_fetch_sub)
                    ? AARCH64_OP(atomic_fetch_sub)
                    : node->opcode == IR_OP(atomic_add_fetch)
                          ? AARCH64_OP(atomic_add_fetch)
                          : AARCH64_OP(atomic_sub_fetch);
      TargetInstruction* value = Materialize(g, node->inputs.value.p[1]);
      inst = NewInstruction2(opcode, addr, value);
      SetAtomicMetadata(inst, value_type,
                        AtomicIRConstant(node->inputs.value.p[2]), 0, false);
      return FinishAtomicValue(g, node, inst);
    }

    case IR_OP(atomic_compare_exchange_bool):
    case IR_OP(atomic_compare_exchange_val):
    case IR_OP(atomic_compare_exchange_n): {
      bool expected_is_pointer =
          node->opcode == IR_OP(atomic_compare_exchange_n);
      AARCH64Opcode opcode =
          expected_is_pointer
              ? AARCH64_OP(atomic_compare_exchange_n)
              : node->opcode == IR_OP(atomic_compare_exchange_bool)
                    ? AARCH64_OP(atomic_compare_exchange_bool)
                    : AARCH64_OP(atomic_compare_exchange_val);
      TargetInstruction* expected =
          Materialize(g, node->inputs.value.p[1]);
      TargetInstruction* desired =
          Materialize(g, node->inputs.value.p[2]);
      inst = NewInstruction3(opcode, addr, expected, desired);
      int order_index = expected_is_pointer ? 4 : 3;
      bool weak =
          expected_is_pointer &&
          AtomicIRConstant(node->inputs.value.p[3]) != 0;
      SetAtomicMetadata(
          inst, value_type,
          AtomicIRConstant(node->inputs.value.p[order_index]),
          AtomicIRConstant(node->inputs.value.p[order_index + 1]), weak);
      return FinishAtomicValue(g, node, inst);
    }

    default:
      assert(false);
      return NULL;
  }
}

static TargetInstruction* LowerIRNode(AARCH64Generator* g, Generator* gen,
                                      IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  switch (node->opcode) {
    case IR_OP(popcounti):
      assert(false && "popcount must be software-expanded before AArch64 lowering");
      return NULL;

    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar):
      // These are handled before we get here.
      return NULL;

    case IR_OP(observable_checkpoint): {
      TargetInstruction* checkpoint =
          Emit(g, NewInstruction(AARCH64_OP(nop)));
      checkpoint->observable_checkpoint = true;
      return checkpoint;
    }
    case IR_OP(nop):
    case last_ir_opcode:
      return NULL;

    case IR_OP(nrvoval):
      return Emit(g, NewInstruction1(AARCH64_OP(nrvoval), Materialize(g, node->inputs.value.p[0])));
      
    case IR_OP(ssavar):
    case IR_OP(phi):
      // We should never see these as we've moved out of SSA form
      // before here.
      break;

    case IR_OP(structreturn): {
      // Always allocate a saved register for the struct return value.
      // g->struct_return_reg = g->num_int_reg_vars++;
      TargetInstruction* result =
          SetLoweredNode(node, EmitSymbol(g, NewInstruction(AARCH64_OP(structreturn))));
      TargetInstruction* mv = Emit(g, NewInstruction1(AARCH64_OP(mov),
                  StructReturnArgumentRegister(g)));
      mv->dest = result;
      return result;
    }

    case IR_OP(literalref):
      return LowerLiteralReference(g, node);

    case IR_OP(addressof):
      return LowerAddressOf(g, node);

    case IR_OP(const32):
      if (TypeIsMemberPointerScalar(node->type) &&
          MemberPointerSize(node->type) == 8) {
        return GetIntConstant(g, node, kTargetType64Bit,
                              ((IRConstant*)node)->value.ivalue);
      }
      return GetIntConstant(g, node, kTargetType32Bit,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(consta):
      return GetIntConstant(g, node, kTargetType64Bit,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(const8):
      return GetIntConstant(g, node, kTargetType8Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(const16):
      return GetIntConstant(g, node, kTargetType16Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(const64):
      return GetIntConstant(g, node, kTargetType64Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(constf):
      return GetFloatingPointConstant(g, node, kTargetTypeFloat,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(constd):
      return GetFloatingPointConstant(g, node, kTargetTypeDouble,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(enter):
      Emit(g, NewInstruction(AARCH64_OP(save)));
      LowerVariables(g, gen);
      return NULL;

    case IR_OP(leave):
      Emit(g, NewInstruction(AARCH64_OP(restore)));
      return NULL;

    case IR_OP(ret):
      return Emit(g, NewInstruction(AARCH64_OP(ret)));
      
    case IR_OP(load32):
    case IR_OP(load8):
    case IR_OP(load64):
    case IR_OP(load16):
    case IR_OP(loadu32):
    case IR_OP(loadu8):
    case IR_OP(loadu16):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
      return LowerLoad(g, node);

      // stores.
    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return LowerStore(g, node);

    case IR_OP(inc8):
    case IR_OP(inc16):
    case IR_OP(inc32):
    case IR_OP(inc64):
    case IR_OP(uinc8):
    case IR_OP(uinc16):
    case IR_OP(uinc32):
    case IR_OP(uinc64):
   case IR_OP(inca):
    case IR_OP(incf):
    case IR_OP(incd):
      return LowerInc(g, node);
    case IR_OP(dec8):
     case IR_OP(dec16):
     case IR_OP(dec32):
     case IR_OP(dec64):
    case IR_OP(udec8):
     case IR_OP(udec16):
     case IR_OP(udec32):
     case IR_OP(udec64):
    case IR_OP(deca):
     case IR_OP(decf):
     case IR_OP(decd):
    return LowerDec(g, node);
      
    case IR_OP(getbit):
      return LowerGetBitField(g, node);
      
    case IR_OP(setbit):
      return LowerSetBitField(g, node);

    case IR_OP(addi):
    case IR_OP(addf):
    case IR_OP(addd):
    case IR_OP(adda):

    case IR_OP(subi):
    case IR_OP(subf):
    case IR_OP(subd):
    case IR_OP(suba):

    case IR_OP(muli):
    case IR_OP(mulf):
    case IR_OP(muld):

    case IR_OP(divi):
    case IR_OP(divf):
    case IR_OP(divd):

    case IR_OP(modi):

    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(lsli):
    case IR_OP(rotli):
    case IR_OP(rotri):
    case IR_OP(clzi):
    case IR_OP(ctzi):

    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):

    case IR_OP(noti):
    case IR_OP(nota):
    case IR_OP(onescomp):
    case IR_OP(negi):
    case IR_OP(negf):
    case IR_OP(negd):

    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2d):
    case IR_OP(d2f):
    case IR_OP(f2i):
    case IR_OP(d2i):

    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova):
    case IR_OP(tmp):
      return LowerExpression(g, node);
      
    case IR_OP(cmpeqi):
    case IR_OP(cmpnei):
    case IR_OP(cmplti):
    case IR_OP(cmplei):
    case IR_OP(cmpgti):
    case IR_OP(cmpgei):

    case IR_OP(cmpeqf):
    case IR_OP(cmpnef):
    case IR_OP(cmpltf):
    case IR_OP(cmplef):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgef):

    case IR_OP(cmpeqd):
    case IR_OP(cmpned):
    case IR_OP(cmpltd):
    case IR_OP(cmpled):
    case IR_OP(cmpgtd):
    case IR_OP(cmpged):

    case IR_OP(cmpeqa):
    case IR_OP(cmpnea):
    case IR_OP(cmplta):
    case IR_OP(cmplea):
    case IR_OP(cmpgta):
    case IR_OP(cmpgea):
      return LowerComparison(g, node);

    case IR_OP(cmp3wayi):
    case IR_OP(cmp3wayu):
    case IR_OP(cmp3waya):
    case IR_OP(cmp3wayf):
    case IR_OP(cmp3wayd):
      return LowerThreeWay(g, node);

    case IR_OP(btrue):
    case IR_OP(bfalse):
      return LowerConditionalBranch(g, node);

    case IR_OP(bra):
      return LowerBranch(g, node);

    case IR_OP(cbra):
      return LowerComputedBranch(g, node);

    case IR_OP(label):
      return LowerLabel(g, node);

    case IR_OP(named_label):
      return LowerNamedLabel(g, node);

    case IR_OP(pusharg):
      return SetLoweredNode(node, Materialize(g, node->inputs.value.p[0]));
      
    case IR_OP(calla):
      return LowerCall(g, node);

    case IR_OP(structarg):
    case IR_OP(vectorarg):
      // Same as its input.
      return SetLoweredNode(node, Materialize(g, node->inputs.value.p[0]));
      
    case IR_OP(resulti):
    case IR_OP(resultf):
    case IR_OP(resultd):
    case IR_OP(resulta):
    case IR_OP(resultv):
      return LowerResult(g, node);
    case IR_OP(capturev):
      return LowerCaptureVectorResult(g, node);

    case IR_OP(vadd):
    case IR_OP(vsub):
    case IR_OP(vmul):
    case IR_OP(vdiv):
    case IR_OP(vand):
    case IR_OP(vor):
    case IR_OP(vxor):
    case IR_OP(vcmpeq):
    case IR_OP(vcmplt):
    case IR_OP(vcmple):
    case IR_OP(vcmpgt):
    case IR_OP(vcmpge):
    case IR_OP(vcmpltu):
    case IR_OP(vcmpleu):
    case IR_OP(vcmpgtu):
    case IR_OP(vcmpgeu):
      return LowerVectorOperation(g, node);

    case IR_OP(vmod):
    case IR_OP(vlsl):
    case IR_OP(vlsr):
    case IR_OP(vasr):
    case IR_OP(vneg):
    case IR_OP(vonescomp):
    case IR_OP(vcmpne):
      assert(false && "vector operation must be software-expanded before AArch64 lowering");
      COMPILER_UNREACHABLE();
      return NULL;

    case IR_OP(memzero):
      return LowerMemzero(g, node);

    case IR_OP(memcpy):
      return LowerMemcpy(g, node);

    case IR_OP(cast): {
      TargetInstruction* result = Materialize(g, node->inputs.value.p[0]);
      // A cast just shares its input's value, but it may still carry a merge
      // destination (the "-> $n" annotation used to funnel the arms of
      // &&/||/?: into one location).  Without routing the value there, the
      // merge slot keeps a stale value, e.g. `*s ? (char*)s : 0` would drop
      // the (char*)s arm and return the wrong pointer.
      if (node->dest != NULL) {
        TargetInstruction* dest = GetDestInstruction(g, node);
        if (dest != NULL) {
          result = SetDestOrMove(g, result, dest, AARCH64_OP(mov));
        }
      }
      return SetLoweredNode(node, result);
    }

    case IR_OP(zeroextendi):
      return LowerZeroExtend(g, node);

    case IR_OP(signextendi):
      return LowerSignExtend(g, node);

    case IR_OP(aligni):
       return LowerAlign(g, node);

    case IR_OP(asm):;
      return LowerAsm(g, node);

    case IR_OP(loc):
      return LowerLocation(g, node);

    case IR_OP(builtin_va_start):
      return LowerBuiltinVaStart(g, node);

    case IR_OP(builtin_va_arg):
      return LowerBuiltinVaArg(g, node);

    case IR_OP(builtin_va_end):
      return LowerBuiltinVaEnd(g, node);

    case IR_OP(builtin_va_copy):
      return LowerBuiltinVaCopy(g, node);

    case IR_OP(atomic_load):
    case IR_OP(atomic_store):
    case IR_OP(atomic_fetch_add):
    case IR_OP(atomic_fetch_sub):
    case IR_OP(atomic_add_fetch):
    case IR_OP(atomic_sub_fetch):
    case IR_OP(atomic_compare_exchange_bool):
    case IR_OP(atomic_compare_exchange_val):
    case IR_OP(atomic_compare_exchange_n):
    case IR_OP(atomic_fence):
      return LowerAtomic(g, node);
      
    case IR_OP(decsp):
    case IR_OP(savesp):
    case IR_OP(restoresp):
      return LowerStackPointerOps(g, node);
  }

  // If we get here we've failed to handle the IR node.
  printf("Unhandled IR OP %s\n", IROpcodeName(node->opcode));
  assert(false);
  return NULL;
}

// Calculate the size of an argument based on its type.
static int64_t CalculateArgumentSize(IRNode* arg) {
  TypeRecord* type = arg->type;
  if (arg->opcode == IR_OP(argument) &&
      ((IRVariable*)arg)->symbol != NULL) {
    type = ((IRVariable*)arg)->symbol->type;
  }
  if (TypeUsesAArch64FpArgReg(type)) {
    return TypeUsesNativeVectorABI(type) && type->size == 16 ? 16 : 8;
  }
  if (TypeIsPointerOrArray(type)) {
    return 8;
  }
  if (TypeIsStructOrUnion(type)) {
    return type->info.struct_info->size;
  }
  return type->size < 4 ? 4 : type->size;
}

static COMPILER_UNUSED int CompareRegisterVar(const void* a, const void* b) {
  const PoolEntry* var1 = *(const PoolEntry**)a;
  const PoolEntry* var2 = *(const PoolEntry**)b;

  Symbol* sym1 = var1->value.symbol;
  Symbol* sym2 = var2->value.symbol;
  int weight1 = sym1->usage_info.reads * (sym1->usage_info.used_in_loop + 1);
  int weight2 = sym2->usage_info.reads * (sym2->usage_info.used_in_loop + 1);

  return weight2 - weight1;
  
  // Sorted in reverse order, highest first.
  //return (int)(var2->pooled->outputs.length - var1->pooled->outputs.length);
}

// Work out where an argument is located.  It will either be in a register
// or on the stack.  If it's in a register, floating points arguments are in the
// fp regs.  All other types are in integer registers.
// Returns the argument location in an ArgLocation struct.  Type type field
// says where it is (in reg or stack) and the location.offset field is either
// the register number or stack offset (from s0 - the frame pointer).
static ArgLocation ArgumentLocation(PoolEntry* arg, Vector* args) {
  IRVariable* var = (IRVariable*)arg->pooled;
  TypeRecord* arg_type =
      var->symbol != NULL ? var->symbol->type : arg->pooled->type;
  size_t arg_num = var->symbol->value.arg_number;
  int int_reg = AARCH64_INT_ARG_START;
  int fp_reg = AARCH64_FP_ARG_START;
  // Offset (from the start of the caller-pushed argument area) of the next
  // argument that does not fit in a register.  The caller (see the call-site
  // lowering) allocates a uniform 8-byte slot per pushed argument, so the
  // callee must use the same stride here or incoming stack arguments will be
  // read from the wrong place.
  int stack_offset = 0;
  for (size_t i = 0; i < args->length; i++) {
    if (i == arg_num) {
      ArgLocation location = {0};
      if (TypeIsMemberPointerAggregate(arg_type)) {
        if (int_reg + 1 <= AARCH64_INT_ARG_END) {
          location.type = kArgLocationRegisterPair;
          location.location.offset = int_reg;
          location.second_offset = int_reg + 1;
        } else {
          location.type = kArgLocationPushedPair;
          location.location.offset = stack_offset;
          location.second_offset = stack_offset + 8;
        }
      } else if (TypeUsesAArch64FpArgReg(arg_type)) {
        if (fp_reg <= AARCH64_FP_ARG_END) {
          // Arg is in a floating point register.
          location.type = kArgLocationRegister;
          location.location.offset = fp_reg;
        } else {
          location.type = kArgLocationPushed;
          location.location.offset = stack_offset;
        }
      } else {
        if (int_reg <= AARCH64_INT_ARG_END) {
          // Arg is in an integer register.
          location.type = kArgLocationRegister;
          location.location.offset = int_reg;
        } else {
          location.type = kArgLocationPushed;
          location.location.offset = stack_offset;
        }
      }
      return location;
    }

    // Advance the per-type register counter (or the stack offset once the
    // argument registers are exhausted) for each preceding argument.  This
    // must use the *argument*-register limits (x0..x7 / d0..d7), the same
    // bounds the per-argument decision above uses; using the wider
    // callee-saved range here would keep incrementing the register counter
    // past x7 and never advance stack_offset, so every stacked argument would
    // be read from the same (first) stack slot.
    Symbol* arg_symbol = args->value.p[i];
    if (TypeIsMemberPointerAggregate(arg_symbol->type)) {
      if (int_reg + 1 <= AARCH64_INT_ARG_END) {
        int_reg += 2;
      } else {
        stack_offset += 16;
      }
    } else if (TypeUsesAArch64FpArgReg(arg_symbol->type)) {
      if (fp_reg <= AARCH64_FP_ARG_END) {
        fp_reg++;
      } else {
        stack_offset += 8;
      }
    } else {
      if (int_reg <= AARCH64_INT_ARG_END) {
        int_reg++;
      } else {
        stack_offset += 8;
      }
    }
  }
  // Can't find argument.
  assert(false);
  ArgLocation error = {0};
  return error;
}

static void AlignOffset(PoolEntry* entry, int* offset) {
  int alignment = PoolEntryStackAlignment(entry);
  *offset = (*offset + (alignment - 1)) & ~(alignment - 1);
}

static void SetDebugRegisterLocation(PoolEntry* entry, int reg) {
  VariableDIESetRegister(entry->value.symbol->die, reg);
}

static void SetDebugStackLocation(PoolEntry* entry, int offset) {
  VariableDIESetStackOffset(entry->value.symbol->die, offset);
}

static void SetDebugSymbolLocation(PoolEntry* entry) {
  VariableDIESetStatic(entry->value.symbol->die,
                       entry->value.symbol->name.value);
}

static TargetInstruction* LoadFpArgumentIntoRegisterVariable(AARCH64Generator* g,
                                                             int reg_var,
                                             ArgLocation arg_loc,
                                             IRNode* symbol) {
  switch (arg_loc.type) {
  case kArgLocationRegister:
    case kArgLocationPassedByReferenceInRegister: {
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var = FloatingPointVariableRegister(g, reg_var, sym->symbol);
      TargetInstruction* arg_reg = FloatingPointArgumentRegister(
          g, (int)arg_loc.location.offset - AARCH64_FP_ARG_START);
      // Reserve this incoming argument register from function entry.
      arg_reg->flags |= TARGET_INST_INCOMING_ARG;
      // fmov has no two-operand register-move form in the emitter (it would be
      // mis-assembled as a three-register instruction); emit a destination
      // move into the variable register instead, mirroring the integer path.
      int size = TypeUsesFloat64Representation(symbol->type)
                     ? kSize64Bit
                     : kSize32Bit;
      TargetInstruction* mv =
          Emit(g, SetInstructionSize(NewInstruction1(AARCH64_OP(fmov), arg_reg), size));
      mv->dest = var;
      return var;
    }
    case kArgLocationPushed:
    case kArgLocationPassedByReferenceOnStack: {
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var = FloatingPointVariableRegister(g, reg_var, sym->symbol);
      TargetInstruction* load = PopArg(g, symbol, arg_loc.location.offset);
      load->dest = var;
      return var;
    }
    case kArgLocationRegisterPair:
    case kArgLocationPushedPair:
      assert(false);
      return NULL;
  }
  return NULL;
}

static TargetInstruction* LoadIntArgumentIntoRegisterVariable(AARCH64Generator* g,
                                                              int reg_var,
                                             ArgLocation arg_loc,
                                             IRNode* symbol) {
  switch (arg_loc.type) {
  case kArgLocationRegister:
    case kArgLocationPassedByReferenceInRegister: {
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var = IntVariableRegister(g, reg_var, sym->symbol);
      TargetInstruction* arg_reg = IntArgumentRegister(
          g, (int)arg_loc.location.offset - AARCH64_INT_ARG_START);
      // Mark this as an incoming argument register so the allocator reserves
      // its physical register from function entry (it is live until this copy).
      arg_reg->flags |= TARGET_INST_INCOMING_ARG;
      TargetInstruction* mv = Emit(g, NewInstruction1(AARCH64_OP(mov), arg_reg));
      mv->dest = var;
      return var;
    }
    case kArgLocationPassedByReferenceOnStack:
    case kArgLocationPushed: {
      // Argument arrived on the stack but lives in a register variable.  Load
      // it from the incoming-argument area and bind the load's result to the
      // variable's register so it is not clobbered by later argument moves.
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var = IntVariableRegister(g, reg_var, sym->symbol);
      TargetInstruction* load = PopArg(g, symbol, arg_loc.location.offset);
      load->dest = var;
      return var;
    }
    case kArgLocationRegisterPair:
    case kArgLocationPushedPair:
      assert(false);
      return NULL;
  }
  return NULL;
}

// Assign a register to a variable or argument if possible.  The
// var_offset is below the stack frame.
static void AssignRegisterOrOffset(AARCH64Generator* g, PoolEntry* entry,
                                   Vector* args, int* var_offset) {
  switch (entry->pooled->opcode) {
    case IR_OP(argument):
    case IR_OP(localvar):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar): {
      IRVariable* variable = (IRVariable*)entry->pooled;
      if (variable->symbol != NULL && variable->symbol->type != NULL) {
        TypeRecordCalculateSize(variable->symbol->type);
        IRSetType(entry->pooled, variable->symbol->type);
      }
      break;
    }
    default:
      break;
  }

  // Variable length arrays are not given offsets until their block
  // is entered.
  if (TypeIsVLA(entry->pooled->type)) {
    return;
  }
  bool is_arg = entry->pooled->opcode == IR_OP(argument);
  TypeRecord* variable_type =
      is_arg && ((IRVariable*)entry->pooled)->symbol != NULL
          ? ((IRVariable*)entry->pooled)->symbol->type
          : entry->pooled->type;
  // Optimized-away locals remain in the variable pool for debug/type
  // bookkeeping, but they need neither a register nor a stack slot.
  if (!is_arg && OptLevel1() && entry->pooled->outputs.length == 0) {
    return;
  }
  TypeRecordCalculateSize(entry->pooled->type);
  int64_t size =
      is_arg ? CalculateArgumentSize(entry->pooled) : entry->pooled->type->size;
  assert(size != 0);

  // printf("var %s\n", ((IRVariable*)entry->pooled)->symbol->name.value);
  if (TypeIsVector(variable_type) && TypeUsesNativeVectorABI(variable_type)) {
    AlignOffset(entry, var_offset);
    if (is_arg) {
      ArgLocation location = ArgumentLocation(entry, args);
      if (location.type == kArgLocationRegister) {
        int offset = -24 - (int)g->saved_regs.length * 8;
        SavedArgumentRegister* saved = NewSavedArgumentRegister(
            (int)location.location.offset, AARCH64_FP_REG, offset, true);
        entry->pooled->data.ivalue = offset;
        VectorAppend(&g->saved_regs, saved);
        g->num_fp_arg_regs++;
        SetDebugStackLocation(entry, offset);
      } else {
        g->not_leaf = true;
        entry->pooled->data.ivalue = 16 + (int)location.location.offset;
        SetDebugStackLocation(entry, entry->pooled->data.ivalue);
      }
    } else {
      entry->pooled->data.ivalue = *var_offset;
      SetDebugStackLocation(entry, *var_offset);
      *var_offset += size;
    }
  } else if (TypeIsFloatingPoint(entry->pooled->type)) {
    if (UseRegisterForVariable(g, entry->pooled)) {
      int reg = g->num_fp_reg_vars++;
      entry->pooled->data.ivalue = AARCH64_REG_VAR | reg;
      SetDebugRegisterLocation(entry, reg);
      if (is_arg) {
        ArgLocation location = ArgumentLocation(entry, args);
        LoadFpArgumentIntoRegisterVariable(g, reg, location, entry->pooled);
        if (location.type == kArgLocationRegister) {
          // See the integer case below for why this is counted here too.
          g->num_fp_arg_regs++;
        }
      }
    } else {
      if (is_arg) {
        ArgLocation location = ArgumentLocation(entry, args);
        if (location.type == kArgLocationRegister) {
          int offset = -24 - (int)g->saved_regs.length * 8;
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset, AARCH64_FP_REG, offset, true);
          entry->pooled->data.ivalue = offset;
          VectorAppend(&g->saved_regs, saved);
          g->num_fp_arg_regs++;  // Named fp argument passed in a register.
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        } else {
          // Passed on the stack: lives at [x29, #16 + pushed_offset].
          g->not_leaf = true;
          entry->pooled->data.ivalue = 16 + (int)location.location.offset;
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        }
      } else {
        AlignOffset(entry, var_offset);
        entry->pooled->data.ivalue = *var_offset;
        SetDebugStackLocation(entry, *var_offset);
        *var_offset += size;
      }
    }
  } else if (TypeIsMemberPointerAggregate(variable_type)) {
    AlignOffset(entry, var_offset);
    if (is_arg) {
      ArgLocation location = ArgumentLocation(entry, args);
      if (location.type == kArgLocationRegisterPair) {
        int offset =
            -24 - ((int)g->saved_regs.length + 1) * 8;
        entry->pooled->data.ivalue = offset;
        SetDebugStackLocation(entry, offset);
        VectorAppend(
            &g->saved_regs,
            NewSavedArgumentRegister((int)location.location.offset,
                                     AARCH64_FP_REG, offset, false));
        VectorAppend(
            &g->saved_regs,
            NewSavedArgumentRegister((int)location.second_offset,
                                     AARCH64_FP_REG, offset + 8, false));
        g->num_int_arg_regs += 2;
      } else {
        g->not_leaf = true;
        entry->pooled->data.ivalue =
            16 + (int)location.location.offset;
        SetDebugStackLocation(entry, entry->pooled->data.ivalue);
      }
    } else {
      entry->pooled->data.ivalue = *var_offset;
      SetDebugStackLocation(entry, *var_offset);
      *var_offset += size;
    }
  } else if (TypePassedAsAArch64Aggregate(variable_type)) {
    if (is_arg) {
      if (size <= 8) {
        // A struct/union that fits in a single register is passed by value in
        // an integer argument register (or on the stack).  Because field
        // accesses need the struct's address, it must live in a stack slot
        // rather than a register variable: home the incoming argument register
        // into a frame slot exactly like a pointer-sized integer argument so
        // the prologue spills it and &param / a.field address that slot.
        ArgLocation location = ArgumentLocation(entry, args);
        if (location.type == kArgLocationRegister) {
          int offset = -24 - (int)g->saved_regs.length * 8;
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset, AARCH64_FP_REG, offset, false);
          entry->pooled->data.ivalue = offset;
          VectorAppend(&g->saved_regs, saved);
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        } else {
          // The struct was passed on the stack by the caller; it lives just
          // above the saved frame-pointer/link-register pair.
          g->not_leaf = true;
          entry->pooled->data.ivalue = 16 + (int)location.location.offset;
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        }
      } else {
        // Passed by reference: the caller copied the struct to its stack and
        // passed a pointer in an argument register or on the stack.  Locate
        // that pointer exactly like a pointer-sized integer argument; the
        // pointer *is* the address of the parameter (so &param and field
        // accesses dereference it - see GetRegAndOffset).
        ArgLocation location = ArgumentLocation(entry, args);
        if (location.type == kArgLocationRegister) {
          // Save the pointer-holding argument register into a frame slot.
          int offset = -24 - (int)g->saved_regs.length * 8;
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset, AARCH64_FP_REG, offset, false);
          entry->pooled->data.ivalue = offset;
          VectorAppend(&g->saved_regs, saved);
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        } else {
          // The pointer was passed on the stack at [x29, #16 + pushed_offset].
          g->not_leaf = true;
          entry->pooled->data.ivalue = 16 + (int)location.location.offset;
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        }
      }
    } else {
      // Not an argument.
      // TODO: it is possible to put small structs in registers.
      if ((entry->pooled->flags & kIRNrvoMarker) != 0) {
        // Named RVO symbol.  This assigned the same register as the
        // structreturn.
        entry->pooled->data.ivalue = AARCH64_REG_VAR | g->struct_return_reg;
        TargetInstruction* var = IntVariableRegister(g, g->struct_return_reg, entry->value.symbol);
        Emit(g, NewInstruction2(AARCH64_OP(mov), var,
                    StructReturnArgumentRegister(g)));
        SetDebugRegisterLocation(entry, g->struct_return_reg);
      } else {
        AlignOffset(entry, var_offset);
        entry->pooled->data.ivalue = *var_offset;
        SetDebugStackLocation(entry, *var_offset);
        *var_offset += size;
      }
    }
  } else if (TypeIsVLA(entry->pooled->type)) {
    // No stack spac allocated for it at entry.  It's allocated
    // by the generated code.
  } else if (TypeIsArray(entry->pooled->type) &&
             !is_arg) {
    // Array local variable, always on the stack.
    AlignOffset(entry, var_offset);
    entry->pooled->data.ivalue = *var_offset;
    SetDebugStackLocation(entry, *var_offset);
    *var_offset += size;
  } else if (TypeIsFunction(entry->pooled->type)) {
    IRVariable* var = (IRVariable*)entry->pooled;
    TargetInstruction* inst = GetSymbol(g, NULL, var->symbol);
    entry->pooled->data.ptr = inst;
    SetDebugSymbolLocation(entry);
  } else {
    // Integer or pointer.
    if (UseRegisterForVariable(g, entry->pooled)) {
      int reg = g->num_int_reg_vars++;
      entry->pooled->data.ivalue = AARCH64_REG_VAR | reg;
      SetDebugRegisterLocation(entry, reg);
      if (is_arg) {
        // Argument, load it into a register.
        ArgLocation location = ArgumentLocation(entry, args);
        LoadIntArgumentIntoRegisterVariable(g,
                                            reg, location, entry->pooled);
        if (location.type == kArgLocationRegister) {
          // A named argument arriving in a register counts whether or not it
          // goes on to live in one.  The count is what divides the named
          // argument registers from the variadic save area the prologue writes
          // and va_start describes, and when optimizing, only the last named
          // argument fails UseRegisterForVariable, because va_start takes its
          // address.  Counting just that one put the save area on top of the
          // earlier named registers, so the first va_arg returned a named
          // argument.
          g->num_int_arg_regs++;
        }
      }
    } else {
      if (is_arg) {
        // The argument is not going to be placed in a register.  We need
        // to make sure it's on the stack.  It is already on the stack
        // if it is not passed in x0..x7.  But if is in an arg reg
        // we need to save it to the stack frame.
        ArgLocation location = ArgumentLocation(entry, args);
        if (location.type == kArgLocationRegister) {
          // Argument is in a register so we need to save it to the stack. These
          // are stored immediately below the saved frame pointer (24 bytes
          // below the previous stack pointer).
          int offset = -24 - (int)g->saved_regs.length * 8;
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset, AARCH64_FP_REG, offset, false);
          entry->pooled->data.ivalue = offset;
          VectorAppend(&g->saved_regs, saved);
          g->num_int_arg_regs++;  // Argument was passed in a register.
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        } else {
          // Argument was passed on the stack by the caller.  It lives just
          // above the saved frame-pointer/link-register pair, i.e. at
          // [x29, #16 + pushed_offset].  Record that positive offset so it is
          // addressed directly from the frame pointer.  This requires a real
          // stack frame.
          g->not_leaf = true;
          entry->pooled->data.ivalue = 16 + (int)location.location.offset;
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        }
      } else {
        AlignOffset(entry, var_offset);
        entry->pooled->data.ivalue = *var_offset;
        SetDebugStackLocation(entry, *var_offset);
        *var_offset += size;
      }
    }
  }
}

static void FinalizeDebugStackLocations(AARCH64Generator* g, Vector* vars) {
  if (!compiler->debug_output) {
    return;
  }
  for (size_t i = 0; i < vars->length; i++) {
    PoolEntry* entry = vars->value.p[i];
    if (entry->value.symbol == NULL || entry->value.symbol->die == NULL) {
      continue;
    }
    VariableDIE* var = (VariableDIE*)entry->value.symbol->die;
    if (var->location.type != kLocationOnStack) {
      continue;
    }
    if (entry->pooled->opcode == IR_OP(localvar) ||
        entry->pooled->opcode == IR_OP(tempvar)) {
      VariableDIESetStackOffset(
          entry->value.symbol->die,
          LocalVariableOffset(g, var->location.v.stack_offset));
    }
  }
}

static void AssignRegisterVars(AARCH64Generator* g, Vector* vars, Vector* args) {
  // Variables are allocated below the frame, arguments are above or in
  // registers.
  // If the argument is in a register, the top bit of the data.ivalue is
  // set and the low order bits are the register number.

  // Sort the pooled local variables in reverse order of usage.  Those
  // with the largest number of references will be at the start of the
  // vector.
  // qsort(vars->value.p, vars->length, sizeof(IRNode*), CompareRegisterVar);

  int32_t var_offset = 0;

  for (size_t i = 0; i < vars->length; i++) {
    PoolEntry* entry = vars->value.p[i];
    AssignRegisterOrOffset(g, entry, args, &var_offset);
  }
  
  // We now know the stack frame size.  This includes the length of the saved
  // registers.
  g->base.stack_frame_size =
      (int32_t)var_offset + (int)g->saved_regs.length * 8;
  // Align to 16 byte boundary.
  g->base.stack_frame_size = (g->base.stack_frame_size + 15) & ~15;
  FinalizeDebugStackLocations(g, vars);
}

static void LowerVariables(AARCH64Generator* g, Generator* gen) {
  Vector local_vars = {0};
  
  // Collect all local variables so that we can assign some of them
  // to registers.
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    switch (entry->pooled->opcode) {
      case IR_OP(localvar):
      case IR_OP(tempvar):
        VectorAppend(&local_vars, entry);
        break;
      case IR_OP(argument):
        VectorAppend(&local_vars, entry);
        break;
      default: {
        // Static variables are referenced by a symbol instruction.
        IRVariable* var = (IRVariable*)entry->pooled;
        TargetInstruction* inst = GetSymbol(g, NULL, var->symbol);
        entry->pooled->data.ptr = inst;
        break;
      }
    }
  }

  AssignRegisterVars(g, &local_vars,
                     &compiler->current_function->info.function.prototype);
  VectorDestruct(&local_vars);
}

void AARCH64Lower(AARCH64Generator* g, Generator* gen) {
  TrapLower(&gen->func->info.function.symbol->name);
  
  // If the function returns in memory, allocate the struct result
  // register now.
  if (TypeReturnedThroughHiddenPointer(gen->func->next)) {
    g->struct_return_reg = g->num_int_reg_vars++;
    g->struct_return_spill_offset = -24;
    // The hidden x8 result pointer is copied into its dedicated variable
    // register by IR_OP(structreturn).  It only needs an additional stack home
    // when an exception landing pad must reconstruct this function's state
    // after unwinding.
    if (gen->exception_ranges.length != 0) {
      VectorAppend(&g->saved_regs,
                   NewSavedArgumentRegister(AARCH64_XR_REG, AARCH64_FP_REG,
                                            g->struct_return_spill_offset,
                                            /*is_fp=*/false));
    }
  }
  
  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(g, gen, node);
    node = IRNext(node);
  }
  ResolveExceptionRanges(g, gen);

  if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
    AARCH64Print(g, compiler->ir_output_file);
  }
  
  // Build basic blocks for.
  TargetBuildBasicBlocks(&g->base);
  
  if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
    TargetPrintBasicBlocks(&g->base, compiler->ir_output_file);
  }
  
  if (OptLevel2()) {
    // Optimize the code sequence for -O2 and above.
    AARCH64Optimize(g);
  
    if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
      fprintf(compiler->ir_output_file, "\n After AARCH64 optimization\n");
      TargetPrintBasicBlocks(&g->base, compiler->ir_output_file);
    }
  }
  // Allocate registers to the instructions.
  AARCH64AllocateRegisters(&g->register_allocator);
}

void AARCH64Print(AARCH64Generator* g, FILE* fp) {
  TargetInstruction* inst = TargetFirstInstruction(&g->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, AARCH64OpcodeName, fp);
    inst = TargetNext(inst);
  }
}
