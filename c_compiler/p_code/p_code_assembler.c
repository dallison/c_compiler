//
//  p_code_assembler.c
//  c_compiler
//
//  Created by David Allison on 12/30/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "p_code_assembler.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"
#include "p_code_machine.h"

static int CompareString(const void* a, const void* b) {
  MapKeyValue* s1 = (MapKeyValue*)a;
  MapKeyValue* s2 = (MapKeyValue*)b;
  return strcmp(s1->key.p, s2->key.p);
}

//
// Forward declarations of instruction assembly functions.
//

#define DECLARE_INST_FUNC(mnemonic) \
  static void Assemble_##mnemonic(PCodeAssembler*)

DECLARE_INST_FUNC(decsp);
DECLARE_INST_FUNC(incsp);
DECLARE_INST_FUNC(movc);
DECLARE_INST_FUNC(movfc);
DECLARE_INST_FUNC(movdc);
DECLARE_INST_FUNC(movxc);
DECLARE_INST_FUNC(mov);
DECLARE_INST_FUNC(movf);
DECLARE_INST_FUNC(movd);
DECLARE_INST_FUNC(push);
DECLARE_INST_FUNC(pushf);
DECLARE_INST_FUNC(pushd);
DECLARE_INST_FUNC(pushx);
DECLARE_INST_FUNC(pop);
DECLARE_INST_FUNC(popf);
DECLARE_INST_FUNC(popd);
DECLARE_INST_FUNC(popx);
DECLARE_INST_FUNC(add);
DECLARE_INST_FUNC(addf);
DECLARE_INST_FUNC(addd);
DECLARE_INST_FUNC(addc);
DECLARE_INST_FUNC(ldw);
DECLARE_INST_FUNC(ldh);
DECLARE_INST_FUNC(ldb);
DECLARE_INST_FUNC(lduw);
DECLARE_INST_FUNC(ldub);
DECLARE_INST_FUNC(lduh);
DECLARE_INST_FUNC(ldx);
DECLARE_INST_FUNC(ldf);
DECLARE_INST_FUNC(ldd);
DECLARE_INST_FUNC(stw);
DECLARE_INST_FUNC(sth);
DECLARE_INST_FUNC(stx);
DECLARE_INST_FUNC(stf);
DECLARE_INST_FUNC(std);
DECLARE_INST_FUNC(stb);
DECLARE_INST_FUNC(sub);
DECLARE_INST_FUNC(subf);
DECLARE_INST_FUNC(subd);
DECLARE_INST_FUNC(mul);
DECLARE_INST_FUNC(mulf);
DECLARE_INST_FUNC(muld);
DECLARE_INST_FUNC(div);
DECLARE_INST_FUNC(divu);
DECLARE_INST_FUNC(divf);
DECLARE_INST_FUNC(divd);
DECLARE_INST_FUNC(mod);
DECLARE_INST_FUNC(modu);
DECLARE_INST_FUNC(lsr);
DECLARE_INST_FUNC(asr);
DECLARE_INST_FUNC(lsl);
DECLARE_INST_FUNC(or);
DECLARE_INST_FUNC(and);
DECLARE_INST_FUNC(xor);
DECLARE_INST_FUNC(not);
DECLARE_INST_FUNC(inv);
DECLARE_INST_FUNC(neg);
DECLARE_INST_FUNC(negf);
DECLARE_INST_FUNC(negd);
DECLARE_INST_FUNC(cmpeq);
DECLARE_INST_FUNC(cmpne);
DECLARE_INST_FUNC(cmplt);
DECLARE_INST_FUNC(cmple);
DECLARE_INST_FUNC(cmpgt);
DECLARE_INST_FUNC(cmpge);
DECLARE_INST_FUNC(cmpltu);
DECLARE_INST_FUNC(cmpleu);
DECLARE_INST_FUNC(cmpgtu);
DECLARE_INST_FUNC(cmpgeu);
DECLARE_INST_FUNC(cmpeqf);
DECLARE_INST_FUNC(cmpnef);
DECLARE_INST_FUNC(cmpltf);
DECLARE_INST_FUNC(cmplef);
DECLARE_INST_FUNC(cmpgtf);
DECLARE_INST_FUNC(cmpgef);
DECLARE_INST_FUNC(cmpeqd);
DECLARE_INST_FUNC(cmpned);
DECLARE_INST_FUNC(cmpltd);
DECLARE_INST_FUNC(cmpled);
DECLARE_INST_FUNC(cmpgtd);
DECLARE_INST_FUNC(cmpged);
DECLARE_INST_FUNC(bnz);
DECLARE_INST_FUNC(bz);
DECLARE_INST_FUNC(bra);
DECLARE_INST_FUNC(cbra);
DECLARE_INST_FUNC(i2f);
DECLARE_INST_FUNC(i2d);
DECLARE_INST_FUNC(ui2f);
DECLARE_INST_FUNC(ui2d);
DECLARE_INST_FUNC(f2d);
DECLARE_INST_FUNC(d2f);
DECLARE_INST_FUNC(f2i);
DECLARE_INST_FUNC(d2i);
DECLARE_INST_FUNC(f2ui);
DECLARE_INST_FUNC(d2ui);
DECLARE_INST_FUNC(jmp);
DECLARE_INST_FUNC(cjmp);
DECLARE_INST_FUNC(adr);
DECLARE_INST_FUNC(adrs);
DECLARE_INST_FUNC(adrtls);
DECLARE_INST_FUNC(call);
DECLARE_INST_FUNC(rcall);
DECLARE_INST_FUNC(ret);
DECLARE_INST_FUNC(esc);

#undef DECLARE_INST_FUNC

#define INST(mnemonic) \
do {\
MapKeyValue kv;\
kv.key.p = #mnemonic;\
kv.value.p = Assemble_##mnemonic;\
MapInsert(instructions, kv);\
} while(0)

// Add all instructions to the handler map.  This maps the instruction
// spelling to a handler function.
static void InitializeInstructions(Map* instructions) {
  INST(decsp);
  INST(incsp);
  INST(movc);
  INST(movfc);
  INST(movdc);
  INST(movxc);
  INST(mov);
  INST(movf);
  INST(movd);
  INST(push);
  INST(pushf);
  INST(pushd);
  INST(pushx);
  INST(pop);
  INST(popf);
  INST(popd);
  INST(popx);
  INST(add);
  INST(addf);
  INST(addd);
  INST(addc);
  INST(ldw);
  INST(ldh);
  INST(ldb);
  INST(lduw);
  INST(ldub);
  INST(lduh);
  INST(ldx);
  INST(ldf);
  INST(ldd);
  INST(stw);
  INST(sth);
  INST(stx);
  INST(stf);
  INST(std);
  INST(stb);
  INST(sub);
  INST(subf);
  INST(subd);
  INST(mul);
  INST(mulf);
  INST(muld);
  INST(div);
  INST(divu);
  INST(divf);
  INST(divd);
  INST(mod);
  INST(modu);
  INST(lsr);
  INST(asr);
  INST(lsl);
  INST(or);
  INST(and);
  INST(xor);
  INST(not);
  INST(inv);
  INST(neg);
  INST(negf);
  INST(negd);
  INST(cmpeq);
  INST(cmpne);
  INST(cmplt);
  INST(cmple);
  INST(cmpgt);
  INST(cmpge);
  INST(cmpltu);
  INST(cmpleu);
  INST(cmpgtu);
  INST(cmpgeu);
  INST(cmpeqf);
  INST(cmpnef);
  INST(cmpltf);
  INST(cmplef);
  INST(cmpgtf);
  INST(cmpgef);
  INST(cmpeqd);
  INST(cmpned);
  INST(cmpltd);
  INST(cmpled);
  INST(cmpgtd);
  INST(cmpged);
  INST(bnz);
  INST(bz);
  INST(bra);
  INST(cbra);
  INST(i2f);
  INST(i2d);
  INST(ui2f);
  INST(ui2d);
  INST(f2d);
  INST(d2f);
  INST(f2i);
  INST(d2i);
  INST(f2ui);
  INST(d2ui);
  INST(jmp);
  INST(cjmp);
  INST(adr);
  INST(adrs);
  INST(adrtls);
  INST(call);
  INST(rcall);
  INST(ret);
  INST(esc);
}

#undef INST

// Initialize the assembler.  Returns true if it worked.
bool PCodeAssemblerInit(PCodeAssembler* assembler, String* infile,
                        String* outfile) {
  static int reloc_types[] = {
      R_PCODE_ADD16, R_PCODE_DATA32, R_PCODE_DATA64, R_PCODE_ADD16, R_PCODE_ADD32,
      R_PCODE_ADD64,  R_PCODE_SUB16,  R_PCODE_SUB32, R_PCODE_SUB64,
  };

  // NOTE: since this is not a real machine we can make up a machine type
  // for the ELF file header. 
  if (!AssemblerInit(&assembler->base, ELF_MACHINE_TYPE_PCODE, 0,
                     reloc_types, infile, outfile)) {
    return false;
  }

  MapInit(&assembler->instructions, CompareString);

  InitializeInstructions(&assembler->instructions);

  // Add a NULL section at the start of the file.
  AssemblerAddSection(&assembler->base, NULL, SHT(null), 0, 0);
  // Add a .bss section.
  assembler->bss = AssemblerAddSection(&assembler->base, NewString(".bss"),
                                       SHT(nobits), SHF(alloc) | SHF(write), 8);
  return true;
}

PCodeAssembler* NewPCodeAssembler(String* infile, String* outfile) {
  PCodeAssembler* assembler = malloc(sizeof(PCodeAssembler));
  PCodeAssemblerInit(assembler, infile, outfile);
  return assembler;
}

// Destruct the assembler.
void PCodeAssemblerDestruct(PCodeAssembler* assembler) {
  AssemblerDestruct(&assembler->base);
  MapDestruct(&assembler->instructions);
}

void PCodeAssemblerDelete(PCodeAssembler* assembler) {
  PCodeAssemblerDestruct(assembler);
  free(assembler);
}

// Shortcut macro avoid typing assembler->base. everywhere we want to access
// the base assembler.
#define ASM assembler->base

// Main assembly function.  This is called by the assembler driver.  It will be
// called twice, one for each pass.
// In pass 1 we parse everything and define all the symbols.
// In pass 2 we also parse everything but we also insert the binary instructions
//    and data into the buffers and expect all symbols to be defined.
void AssemblePCodeInstruction(Assembler* base, String* word) {
  PCodeAssembler* assembler = (PCodeAssembler*)base;

  void* asm_func = MapFindPointerKey(&assembler->instructions, word->value);
  if (asm_func != NULL) {
    void (*func)(PCodeAssembler*) = asm_func;
    func(assembler);
  } else {
    AssemblerError(&ASM, "Syntax error; unknown instruction: %s", word->value);
  }
}

// Extract register number from register name.  The format is 'x#' where
// 'x' is the register type (r, f or d) and # is the number.
static int RegNumber(PCodeAssembler* assembler, String* reg_name) {
  int n = 0;
  size_t i = 1;
  if (reg_name->length == 1) {
    AssemblerError(&ASM, "Illegal register name");
  }
  while (i < reg_name->length) {
    n = n * 10 + reg_name->value[i] - '0';
    i++;
  }
  if (n > 255) {
    AssemblerError(&ASM, "Illegal register number %d", n);
  }
  return n;
}

static bool RegisterName(PCodeAssembler* assembler, int* num, char* type) {
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String reg_name;
    StringInit(&reg_name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);

    // We allow special register names: sp, ap and fp, for the stack pointer,
    // argument pointer and frame pointer respectively.
    if (StringEqual(&reg_name, "sp")) {
      *num = PCODE_SP_REG;
      *type = 'i';
      return true;
    }

    if (StringEqual(&reg_name, "fp")) {
      *num = PCODE_FP_REG;
      *type = 'i';
      return true;
    }

    if (StringEqual(&reg_name, "ap")) {
      *num = PCODE_AP_REG;
      *type = 'i';
      return true;
    }
    
    if (StringEqual(&reg_name, "tp")) {
      *num = PCODE_TP_REG;
      *type = 'i';
      return true;
    }

    switch (reg_name.value[0]) {
      case 'r':
      case 'R':
        *type = 'i';
        break;
      case 'f':
      case 'F':
        *type = 'f';
        break;
      case 'd':
      case 'D':
        *type = 'd';
        break;
      default:
        return false;
    }
    *num = RegNumber(assembler, &reg_name);
    StringDestruct(&reg_name);
    return true;
  }
  return false;
}

static int Register(PCodeAssembler* assembler, char type_needed,
                    const char* type_name) {
  int num;
  char type;
  if (!RegisterName(assembler, &num, &type)) {
    AssemblerError(&ASM, "Expected %s register name", type_name);
    return 0;
  }

  if (type != type_needed) {
    AssemblerError(&ASM, "Invalid register type; got %c expected %c", type,
                   type_needed);
    return 0;
  }
  return num;
}

static bool ParseRegisterTriple(PCodeAssembler* assembler, char type_needed,
                                const char* type_name, int* regs) {
  regs[0] = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return false;
  }
  regs[1] = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return false;
  }
  regs[2] = Register(assembler, type_needed, type_name);
  return true;
}

// Comparisons always use an integer destination reg.
static bool ParseComparisonRegisterTriple(PCodeAssembler* assembler,
                                          char type_needed,
                                          const char* type_name, int* regs) {
  regs[0] = Register(assembler, 'i', "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return false;
  }
  regs[1] = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return false;
  }
  regs[2] = Register(assembler, type_needed, type_name);
  return true;
}

static bool ParseRegisterPair(PCodeAssembler* assembler, char type_needed,
                              const char* type_name, int* regs) {
  regs[0] = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return false;
  }
  regs[1] = Register(assembler, type_needed, type_name);
  regs[2] = 0;
  return true;
}

//
// Individual instruction handling functions.
//

static void AssembleALU(PCodeAssembler* assembler, int opcode, int* regs) {
  int32_t inst = opcode << 24 | regs[0] << 16 | regs[1] << 8 | regs[2];
  AssemblerEmitWord(&assembler->base, assembler->base.current_section, inst);
}

#define ASSEMBLE_INT_ALU(inst)                                  \
  static void Assemble_##inst(PCodeAssembler* assembler) {      \
    int regs[3];                                                \
    if (ParseRegisterTriple(assembler, 'i', "integer", regs)) { \
      AssembleALU(assembler, PCODE_OP(inst), regs);                   \
    }                                                           \
  }

#define ASSEMBLE_FLOAT_ALU(inst)                              \
  static void Assemble_##inst(PCodeAssembler* assembler) {    \
    int regs[3];                                              \
    if (ParseRegisterTriple(assembler, 'f', "float", regs)) { \
      AssembleALU(assembler, PCODE_OP(inst), regs);                 \
    }                                                         \
  }

#define ASSEMBLE_DOUBLE_ALU(inst)                              \
  static void Assemble_##inst(PCodeAssembler* assembler) {     \
    int regs[3];                                               \
    if (ParseRegisterTriple(assembler, 'd', "double", regs)) { \
      AssembleALU(assembler, PCODE_OP(inst), regs);                  \
    }                                                          \
  }

#define ASSEMBLE_UNARY_INT_ALU(inst)                          \
  static void Assemble_##inst(PCodeAssembler* assembler) {    \
    int regs[3];                                              \
    if (ParseRegisterPair(assembler, 'i', "integer", regs)) { \
      AssembleALU(assembler, PCODE_OP(inst), regs);                 \
    }                                                         \
  }

#define ASSEMBLE_UNARY_FLOAT_ALU(inst)                      \
  static void Assemble_##inst(PCodeAssembler* assembler) {  \
    int regs[3];                                            \
    if (ParseRegisterPair(assembler, 'f', "float", regs)) { \
      AssembleALU(assembler, PCODE_OP(inst), regs);               \
    }                                                       \
  }

#define ASSEMBLE_UNARY_DOUBLE_ALU(inst)                      \
  static void Assemble_##inst(PCodeAssembler* assembler) {   \
    int regs[3];                                             \
    if (ParseRegisterPair(assembler, 'd', "double", regs)) { \
      AssembleALU(assembler, PCODE_OP(inst), regs);                \
    }                                                        \
  }

#define ASSEMBLE_INT_CMP(inst)                                            \
  static void Assemble_##inst(PCodeAssembler* assembler) {                \
    int regs[3];                                                          \
    if (ParseComparisonRegisterTriple(assembler, 'i', "integer", regs)) { \
      AssembleALU(assembler, PCODE_OP(inst), regs);                             \
    }                                                                     \
  }

#define ASSEMBLE_FLOAT_CMP(inst)                                        \
  static void Assemble_##inst(PCodeAssembler* assembler) {              \
    int regs[3];                                                        \
    if (ParseComparisonRegisterTriple(assembler, 'f', "float", regs)) { \
      AssembleALU(assembler, PCODE_OP(inst), regs);                           \
    }                                                                   \
  }

#define ASSEMBLE_DOUBLE_CMP(inst)                                        \
  static void Assemble_##inst(PCodeAssembler* assembler) {               \
    int regs[3];                                                         \
    if (ParseComparisonRegisterTriple(assembler, 'd', "double", regs)) { \
      AssembleALU(assembler, PCODE_OP(inst), regs);                            \
    }                                                                    \
  }

ASSEMBLE_INT_ALU(add)
ASSEMBLE_INT_ALU(sub)
ASSEMBLE_FLOAT_ALU(addf)
ASSEMBLE_DOUBLE_ALU(addd)
ASSEMBLE_FLOAT_ALU(subf);
ASSEMBLE_DOUBLE_ALU(subd);
ASSEMBLE_INT_ALU(mul);
ASSEMBLE_FLOAT_ALU(mulf);
ASSEMBLE_DOUBLE_ALU(muld);
ASSEMBLE_INT_ALU(div);
ASSEMBLE_INT_ALU(divu);
ASSEMBLE_FLOAT_ALU(divf);
ASSEMBLE_DOUBLE_ALU(divd);
ASSEMBLE_INT_ALU(mod);
ASSEMBLE_INT_ALU(modu);
ASSEMBLE_INT_ALU(lsr);
ASSEMBLE_INT_ALU(asr);
ASSEMBLE_INT_ALU(lsl);
ASSEMBLE_INT_ALU(or);
ASSEMBLE_INT_ALU(and);
ASSEMBLE_INT_ALU(xor);
ASSEMBLE_UNARY_INT_ALU(not);
ASSEMBLE_UNARY_INT_ALU(inv);
ASSEMBLE_UNARY_INT_ALU(neg);
ASSEMBLE_UNARY_FLOAT_ALU(negf);
ASSEMBLE_UNARY_DOUBLE_ALU(negd);
ASSEMBLE_INT_CMP(cmpeq);
ASSEMBLE_INT_CMP(cmpne);
ASSEMBLE_INT_CMP(cmplt);
ASSEMBLE_INT_CMP(cmple);
ASSEMBLE_INT_CMP(cmpgt);
ASSEMBLE_INT_CMP(cmpge);
ASSEMBLE_INT_CMP(cmpltu);
ASSEMBLE_INT_CMP(cmpleu);
ASSEMBLE_INT_CMP(cmpgtu);
ASSEMBLE_INT_CMP(cmpgeu);
ASSEMBLE_FLOAT_CMP(cmpeqf);
ASSEMBLE_FLOAT_CMP(cmpnef);
ASSEMBLE_FLOAT_CMP(cmpltf);
ASSEMBLE_FLOAT_CMP(cmplef);
ASSEMBLE_FLOAT_CMP(cmpgtf);
ASSEMBLE_FLOAT_CMP(cmpgef);
ASSEMBLE_DOUBLE_CMP(cmpeqd);
ASSEMBLE_DOUBLE_CMP(cmpned);
ASSEMBLE_DOUBLE_CMP(cmpltd);
ASSEMBLE_DOUBLE_CMP(cmpled);
ASSEMBLE_DOUBLE_CMP(cmpgtd);
ASSEMBLE_DOUBLE_CMP(cmpged);

static void AssembleConversion(PCodeAssembler* assembler, int opcode,
                               char from_type, const char* from_type_name,
                               char to_type, const char* to_type_name) {
  int regs[2];
  regs[0] = Register(assembler, to_type, to_type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  regs[1] = Register(assembler, from_type, from_type_name);
  AssemblerEmitWord(&ASM, ASM.current_section,
                    opcode << 24 | regs[0] << 16 | regs[1] << 8);
}

static void Assemble_i2f(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(i2f), 'i', "integer", 'f', "float");
}

static void Assemble_ui2f(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(ui2f), 'i', "integer", 'f', "float");
}

static void Assemble_i2d(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(i2d), 'i', "integer", 'd', "double");
}

static void Assemble_ui2d(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(ui2d), 'i', "integer", 'd', "double");
}

static void Assemble_d2f(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(d2f), 'd', "double", 'f', "float");
}

static void Assemble_f2d(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(f2d), 'f', "float", 'd', "double");
}

static void Assemble_f2i(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(f2i), 'f', "float", 'i', "integer");
}

static void Assemble_d2i(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(d2i), 'd', "double", 'i', "integer");
}

static void Assemble_f2ui(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(f2ui), 'f', "float", 'i', "integer");
}

static void Assemble_d2ui(PCodeAssembler* assembler) {
  AssembleConversion(assembler, PCODE_OP(d2ui), 'd', "double", 'i', "integer");
}

static void Assemble_decsp(PCodeAssembler* assembler) {
  if (LexMatch(&ASM.lex, TOK(hash))) {
    int64_t value = AssemblerEvaluateExpression(&ASM);
    AssemblerEmitWord(&ASM, ASM.current_section,
                      (PCODE_OP(decsp) << 24 | (int)(value & 0xffffff)));
  } else {
    AssemblerError(&ASM, "Immediate expression expected");
  }
}

static void Assemble_incsp(PCodeAssembler* assembler) {
  if (LexMatch(&ASM.lex, TOK(hash))) {
    int64_t value = AssemblerEvaluateExpression(&ASM);
    AssemblerEmitWord(&ASM, ASM.current_section,
                      (PCODE_OP(incsp) << 24 | (int)(value & 0xffffff)));
  } else {
    AssemblerError(&ASM, "Immediate expression expected");
  }
}

static void AssembleLoadStore(PCodeAssembler* assembler, int opcode,
                              char type_needed, const char* type_name) {
  int regs[2];
  regs[0] = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (!LexMatch(&ASM.lex, TOK(lsquare))) {
    AssemblerError(&ASM, "Missing [");
    return;
  }

  // Base register is always integer register.
  regs[1] = Register(assembler, 'i', "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  // Now an immediate offset.
  if (!LexMatch(&ASM.lex, TOK(hash))) {
    AssemblerError(&ASM, "Missing # offset");
    return;
  }
  int64_t offset = AssemblerEvaluateExpression(&ASM);
  if (!LexMatch(&ASM.lex, TOK(rsquare))) {
    AssemblerError(&ASM, "Missing ]");
    return;
  }

  // These are 64 bit instructions with the first word containing the
  // two registers and the second containing the offset.
  AssemblerEmitWord(&ASM, ASM.current_section,
                    0x80000000U | opcode << 24 | regs[0] << 16 | regs[1] << 8);
  AssemblerEmitWord(&ASM, ASM.current_section, (int32_t)offset);
}

#define ASSEMBLE_INT_LOAD_STORE(inst)                       \
  static void Assemble_##inst(PCodeAssembler* assembler) {  \
    AssembleLoadStore(assembler, PCODE_OP(inst), 'i', "integer"); \
  }

ASSEMBLE_INT_LOAD_STORE(ldb)
ASSEMBLE_INT_LOAD_STORE(ldh)
ASSEMBLE_INT_LOAD_STORE(ldw)
ASSEMBLE_INT_LOAD_STORE(ldub)
ASSEMBLE_INT_LOAD_STORE(lduh)
ASSEMBLE_INT_LOAD_STORE(lduw)
ASSEMBLE_INT_LOAD_STORE(ldx)

static void Assemble_ldf(PCodeAssembler* assembler) {
  AssembleLoadStore(assembler, PCODE_OP(ldf), 'f', "float");
}

static void Assemble_ldd(PCodeAssembler* assembler) {
  AssembleLoadStore(assembler, PCODE_OP(ldd), 'd', "double");
}

ASSEMBLE_INT_LOAD_STORE(stb);
ASSEMBLE_INT_LOAD_STORE(stw);
ASSEMBLE_INT_LOAD_STORE(sth);
ASSEMBLE_INT_LOAD_STORE(stx);

static void Assemble_stf(PCodeAssembler* assembler) {
  AssembleLoadStore(assembler, PCODE_OP(stf), 'f', "float");
}

static void Assemble_std(PCodeAssembler* assembler) {
  AssembleLoadStore(assembler, PCODE_OP(std), 'd', "double");
}

#undef ASSEMBLE_INT_LOAD_STORE

static void AssembleMoveConstant(PCodeAssembler* assembler, int opcode,
                                 char type_needed, const char* type_name) {
  int reg = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  // Now an immediate value or a symbol
  if (!LexMatch(&ASM.lex, TOK(hash))) {
    if (opcode != PCODE_OP(movxc)) {
      AssemblerError(&ASM, "Illegal symbol reference instruction");
      return;
    }
    // Symbol.  We are moving a symbol into a register.
    if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
      AssemblerError(&ASM, "Invalid mov operand");
      return;
    }
    String symbol_name;
    String suffix;
    StringInit(&symbol_name, NULL);
    StringInit(&suffix, NULL);
    AssemblerExtractSymbolSuffix(&ASM.lex.spelling, &symbol_name, &suffix);
    
    AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, symbol_name.value);
    if (sym == NULL) {
      sym = NewAssemblerSymbol(symbol_name.value, ASM.current_section,
                               SYM_TYPE(object), SYM_BIND(local), 0);
      AssemblerInsertSymbol(&ASM, sym);
    }
    LexNextToken(&ASM.lex);
    int reloc_type;
    if (StringEqual(&suffix, "tls")) {
      reloc_type = assembler->base.pic ? R_PCODE_GOT_TLS_IE : R_PCODE_TLS_TP_OFF;
    } else {
      reloc_type = assembler->base.pic ? R_PCODE_GOT_ENTRY : R_PCODE_ABS;
    }
    AssemblerRelocation* reloc =
      NewAssemblerRelocation(sym,
                           reloc_type,
                           ASM.current_section,
                               (int32_t)AssemblerCurrentAddress(&ASM), 0);
    AssemblerAddRelocation(&ASM, reloc);
    AssemblerEmitWord(&ASM, ASM.current_section,
                      0xc0000000U | opcode << 24 | reg << 16);
    AssemblerEmitLong(&ASM, ASM.current_section, 0);
    StringDestruct(&symbol_name);
    StringDestruct(&suffix);
    return;
  }

  // These are 64 or 96 bit instructions with the first word containing the
  // register and the second [two] containing the value.
  switch (type_needed) {
    case 'i': {
      int64_t value = AssemblerEvaluateExpression(&ASM);

      if (opcode == PCODE_OP(movxc)) {
        AssemblerEmitWord(&ASM, ASM.current_section,
                          0xc0000000U | opcode << 24 | reg << 16);
        AssemblerEmitLong(&ASM, ASM.current_section, value);
      } else {
        AssemblerEmitWord(&ASM, ASM.current_section,
                          0x80000000U | opcode << 24 | reg << 16);
        AssemblerEmitWord(&ASM, ASM.current_section, (int32_t)value);
      }
      break;
    }
    case 'f': {
      AssemblerEmitWord(&ASM, ASM.current_section,
                        0x80000000U | opcode << 24 | reg << 16);
      float f = AssemblerGetDoubleConst(&ASM);
      uint32_t* p = (uint32_t*)&f;
      AssemblerEmitWord(&ASM, ASM.current_section, *p);
      break;
    }
    case 'd': {
      double f = AssemblerGetDoubleConst(&ASM);
      uint64_t* p = (uint64_t*)&f;
      AssemblerEmitWord(&ASM, ASM.current_section,
                        0xc0000000U | opcode << 24 | reg << 16);
      AssemblerEmitLong(&ASM, ASM.current_section, *p);
      break;
    }
    default:
      assert(false);
  }
}

#define ASSEMBLE_MOVC(inst, reg_type, type_name)                    \
  static void Assemble_##inst(PCodeAssembler* assembler) {          \
    AssembleMoveConstant(assembler, PCODE_OP(inst), reg_type, type_name); \
  }

#define UNDEFINED_INST(m) \
  static void Assemble_##m(PCodeAssembler* assembler) {}

ASSEMBLE_MOVC(movc, 'i', "integer");
ASSEMBLE_MOVC(movfc, 'f', "float");
ASSEMBLE_MOVC(movdc, 'd', "double");
ASSEMBLE_MOVC(movxc, 'i', "integer");

#undef ASSEMBLE_MOVC

static void AssembleMove(PCodeAssembler* assembler, int opcode,
                         char type_needed, const char* type_name) {
  int regs[3];
  if (ParseRegisterPair(assembler, type_needed, type_name, regs)) {
    AssemblerEmitWord(&ASM, ASM.current_section,
                      opcode << 24 | regs[0] << 16 | regs[1] << 8);
  }
}

#define ASSEMBLE_MOV(inst, reg_type, type_name)             \
  static void Assemble_##inst(PCodeAssembler* assembler) {  \
    AssembleMove(assembler, PCODE_OP(inst), reg_type, type_name); \
  }

ASSEMBLE_MOV(mov, 'i', "integer");
ASSEMBLE_MOV(movf, 'f', "float");
ASSEMBLE_MOV(movd, 'd', "double");

#undef ASSEMBLE_MOV

static void AssemblePushPPCODE_OP(PCodeAssembler* assembler, int opcode,
                            char type_needed, const char* type_name) {
  int reg = Register(assembler, type_needed, type_name);
  AssemblerEmitWord(&ASM, ASM.current_section, opcode << 24 | reg << 16);
}

#define ASSEMBLE_PUSH_PPCODE_OP(inst, reg_type, type_name)           \
  static void Assemble_##inst(PCodeAssembler* assembler) {     \
    AssemblePushPPCODE_OP(assembler, PCODE_OP(inst), reg_type, type_name); \
  }

ASSEMBLE_PUSH_PPCODE_OP(push, 'i', "integer");
ASSEMBLE_PUSH_PPCODE_OP(pushf, 'f', "float");
ASSEMBLE_PUSH_PPCODE_OP(pushd, 'd', "double");
ASSEMBLE_PUSH_PPCODE_OP(pushx, 'i', "integer");
ASSEMBLE_PUSH_PPCODE_OP(pop, 'i', "integer");
ASSEMBLE_PUSH_PPCODE_OP(popf, 'f', "float");
ASSEMBLE_PUSH_PPCODE_OP(popd, 'd', "double");
ASSEMBLE_PUSH_PPCODE_OP(popx, 'i', "integer");

static void Assemble_ret(PCodeAssembler* assembler) {
  AssemblerEmitWord(&ASM, ASM.current_section, PCODE_OP(ret) << 24);
}

static void AssembleConditionalBranch(PCodeAssembler* assembler, int opcode) {
  int reg = Register(assembler, 'i', "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  int64_t addr = AssemblerEvaluateExpression(&ASM);
  int64_t offset = addr - (AssemblerCurrentAddress(&ASM) + 8);
  AssemblerEmitWord(&ASM, ASM.current_section,
                    0x80000000 | opcode << 24 | reg << 16);
  AssemblerEmitWord(&ASM, ASM.current_section, (int32_t)offset);
}

static void Assemble_bz(PCodeAssembler* assembler) {
  AssembleConditionalBranch(assembler, PCODE_OP(bz));
}

static void Assemble_bnz(PCodeAssembler* assembler) {
  AssembleConditionalBranch(assembler, PCODE_OP(bnz));
}

static void Assemble_bra(PCodeAssembler* assembler) {
  int64_t addr = AssemblerEvaluateExpression(&ASM);
  int64_t offset = addr - (AssemblerCurrentAddress(&ASM) + 8);
  AssemblerEmitWord(&ASM, ASM.current_section, 0x80000000 | PCODE_OP(bra) << 24);
  AssemblerEmitWord(&ASM, ASM.current_section, (int32_t)offset);
}

static void Assemble_addc(PCodeAssembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, 'i', "integer", regs)) {
    if (!LexMatch(&ASM.lex, TOK(comma))) {
      AssemblerError(&ASM, "Missing comma");
    }
    if (!LexMatch(&ASM.lex, TOK(hash))) {
      AssemblerError(&ASM, "Missing #constant");
    }
    int64_t value = AssemblerEvaluateExpression(&ASM);

    AssemblerEmitWord(
        &ASM, ASM.current_section,
        0x80000000 | PCODE_OP(addc) << 24 | regs[0] << 16 | regs[1] << 8);
    AssemblerEmitWord(&ASM, ASM.current_section, (int32_t)value);
  }
}

static void Assemble_cbra(PCodeAssembler* assembler) {
  int reg = Register(assembler, 'i', "integer");
  AssemblerEmitWord(&ASM, ASM.current_section, PCODE_OP(cbra) << 24 | reg << 16);
}

static void Assemble_rcall(PCodeAssembler* assembler) {
  int reg = Register(assembler, 'i', "integer");
  AssemblerEmitWord(&ASM, ASM.current_section, PCODE_OP(rcall) << 24 | reg << 16);
}

static void Assemble_call(PCodeAssembler* assembler) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing symbol for call instruction");
    return;
  }
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, ASM.lex.spelling.value);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(ASM.lex.spelling.value, ASM.current_section,
                             SYM_TYPE(func), SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  LexNextToken(&ASM.lex);
  AssemblerRelocation* reloc =
  NewAssemblerRelocation(sym, assembler->base.pic ?
                         R_PCODE_CALL_PLT : R_PCODE_CALL,
                         ASM.current_section,
                         (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);
  AssemblerEmitWord(&ASM, ASM.current_section, 0xc0000000 | PCODE_OP(call) << 24);
  AssemblerEmitLong(&ASM, ASM.current_section, 0);
}

static void Assemble_jmp(PCodeAssembler* assembler) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing symbol for jmp instruction");
    return;
  }
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, ASM.lex.spelling.value);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(ASM.lex.spelling.value, ASM.current_section,
                             SYM_TYPE(func), SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  LexNextToken(&ASM.lex);
  AssemblerRelocation* reloc =
      NewAssemblerRelocation(sym, R_PCODE_JMP, ASM.current_section,
                             (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);
  AssemblerEmitWord(&ASM, ASM.current_section, 0xc0000000 | PCODE_OP(jmp) << 24);
  AssemblerEmitLong(&ASM, ASM.current_section, 0);
}

static void Assemble_cjmp(PCodeAssembler* assembler) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing symbol for cjmp instruction");
    return;
  }
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, ASM.lex.spelling.value);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(ASM.lex.spelling.value, ASM.current_section,
                             SYM_TYPE(object), SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  LexNextToken(&ASM.lex);
  AssemblerRelocation* reloc =
  NewAssemblerRelocation(sym, R_PCODE_JMP, ASM.current_section,
                         (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);
  AssemblerEmitWord(&ASM, ASM.current_section, 0xc0000000 |
                    PCODE_OP(cjmp) << 24);
  AssemblerEmitLong(&ASM, ASM.current_section, 0);
}

static void Assemble_adr(PCodeAssembler* assembler) {
  int reg = Register(assembler, 'i', "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing symbol for adr instruction");
    return;
  }
  String symbol_name;
  String suffix;
  StringInit(&symbol_name, NULL);
  StringInit(&suffix, NULL);
  AssemblerExtractSymbolSuffix(&ASM.lex.spelling, &symbol_name, &suffix);
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, symbol_name.value);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(symbol_name.value, ASM.current_section,
                             SYM_TYPE(func), SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  LexNextToken(&ASM.lex);
  int reloc_type;
  if (StringEqual(&suffix, "tls")) {
    reloc_type = R_PCODE_GOT_TLS_GD;
  } else {
    reloc_type = assembler->base.pic ? R_PCODE_GOT_ENTRY : R_PCODE_ABS;
  }
  AssemblerRelocation* reloc =
  NewAssemblerRelocation(sym, reloc_type,
                         ASM.current_section,
                         (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);
  AssemblerEmitWord(&ASM, ASM.current_section, 0xc0000000 |
                    PCODE_OP(adr) << 24 | reg << 16);
  AssemblerEmitLong(&ASM, ASM.current_section, 0);
  StringDestruct(&symbol_name);
  StringDestruct(&suffix);
}

static void Assemble_adrs(PCodeAssembler* assembler) {
  int reg = Register(assembler, 'i', "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing symbol for adrs instruction");
    return;
  }
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, ASM.lex.spelling.value);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(ASM.lex.spelling.value, ASM.current_section,
                             SYM_TYPE(func), SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  LexNextToken(&ASM.lex);
  AssemblerRelocation* reloc =
  NewAssemblerRelocation(sym, R_PCODE_PCREL,
                         ASM.current_section,
                         (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);
  AssemblerEmitWord(&ASM, ASM.current_section, 0xc0000000 |
                    PCODE_OP(adr) << 24 | reg << 16);
  AssemblerEmitLong(&ASM, ASM.current_section, 0);
  
}

// Initial exec TLS address.
static void Assemble_adrtls(PCodeAssembler* assembler) {
  int reg = Register(assembler, 'i', "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing symbol for adrtls instruction");
    return;
  }
  String symbol_name;
  String suffix;
  StringInit(&symbol_name, NULL);
  StringInit(&suffix, NULL);
  AssemblerExtractSymbolSuffix(&ASM.lex.spelling, &symbol_name, &suffix);
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, symbol_name.value);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(symbol_name.value, ASM.current_section,
                             SYM_TYPE(func), SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  LexNextToken(&ASM.lex);
  int reloc_type;
  if (StringEqual(&suffix, "tls")) {
    reloc_type = R_PCODE_GOT_TLS_IE;
  } else {
    reloc_type = assembler->base.pic ? R_PCODE_GOT_ENTRY : R_PCODE_ABS;
  }
  AssemblerRelocation* reloc =
  NewAssemblerRelocation(sym, reloc_type,
                         ASM.current_section,
                         (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);
  AssemblerEmitWord(&ASM, ASM.current_section, 0xc0000000 |
                    PCODE_OP(adr) << 24 | reg << 16);
  AssemblerEmitLong(&ASM, ASM.current_section, 0);
  StringDestruct(&symbol_name);
  StringDestruct(&suffix);
}

static void Assemble_esc(PCodeAssembler* assembler) {
  if (LexMatch(&ASM.lex, TOK(hash))) {
    int64_t value = AssemblerEvaluateExpression(&ASM);
    AssemblerEmitWord(&ASM, ASM.current_section,
                      (PCODE_OP(esc) << 24 | (int)(value & 0xffffff)));
  } else {
    AssemblerError(&ASM, "Missing #value for esc instruction");
  }
}

#undef UNDEFINED_INST
