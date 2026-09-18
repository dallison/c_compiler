//
//  6502_assembler.c
//  c_compiler_library
//
//  Created by David Allison on 5/17/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_assembler.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "6502_machine.h"
#include "elf.h"

static void CheckBranches(W65C02Assembler* assembler);

// A useful resource for the encoding for the 6502 instruction set can
// be found at:
// http://nparker.llx.com/a2/opcodes.html


//
// Forward declarations of instruction assembly functions.
//

#define DECLARE_INST_FUNC(mnemonic) \
  static void Assemble_##mnemonic(W65C02Assembler*)

// 6502(A)
DECLARE_INST_FUNC(brk);

DECLARE_INST_FUNC(bpl);
DECLARE_INST_FUNC(bmi);
DECLARE_INST_FUNC(bvc);
DECLARE_INST_FUNC(bvs);
DECLARE_INST_FUNC(bcc);
DECLARE_INST_FUNC(bcs);
DECLARE_INST_FUNC(bne);
DECLARE_INST_FUNC(beq);

DECLARE_INST_FUNC(jsr);
DECLARE_INST_FUNC(jmp);

DECLARE_INST_FUNC(rti);
DECLARE_INST_FUNC(rts);

DECLARE_INST_FUNC(lda);
DECLARE_INST_FUNC(ldx);
DECLARE_INST_FUNC(ldy);
DECLARE_INST_FUNC(sta);
DECLARE_INST_FUNC(stx);
DECLARE_INST_FUNC(sty);

DECLARE_INST_FUNC(cmp);
DECLARE_INST_FUNC(cpy);
DECLARE_INST_FUNC(cpx);
DECLARE_INST_FUNC(bit);

DECLARE_INST_FUNC(ora);
DECLARE_INST_FUNC(and);
DECLARE_INST_FUNC(eor);

DECLARE_INST_FUNC(adc);
DECLARE_INST_FUNC(sbc);

DECLARE_INST_FUNC(asl);
DECLARE_INST_FUNC(rol);
DECLARE_INST_FUNC(lsr);
DECLARE_INST_FUNC(ror);

DECLARE_INST_FUNC(dec);
DECLARE_INST_FUNC(inc);
DECLARE_INST_FUNC(dey);
DECLARE_INST_FUNC(dex);
DECLARE_INST_FUNC(iny);
DECLARE_INST_FUNC(inx);

DECLARE_INST_FUNC(php);
DECLARE_INST_FUNC(clc);
DECLARE_INST_FUNC(plp);
DECLARE_INST_FUNC(sec);
DECLARE_INST_FUNC(pha);
DECLARE_INST_FUNC(cli);
DECLARE_INST_FUNC(pla);
DECLARE_INST_FUNC(sei);
DECLARE_INST_FUNC(tay);
DECLARE_INST_FUNC(clv);
DECLARE_INST_FUNC(cld);
DECLARE_INST_FUNC(sed);

DECLARE_INST_FUNC(tya);
DECLARE_INST_FUNC(txa);
DECLARE_INST_FUNC(txs);
DECLARE_INST_FUNC(tax);
DECLARE_INST_FUNC(tsx);

DECLARE_INST_FUNC(nop);

// 65C02
DECLARE_INST_FUNC(tsb);
DECLARE_INST_FUNC(trb);
DECLARE_INST_FUNC(stz);
DECLARE_INST_FUNC(phy);
DECLARE_INST_FUNC(ply);
DECLARE_INST_FUNC(phx);
DECLARE_INST_FUNC(plx);
DECLARE_INST_FUNC(bra);

#undef DECLARE_INST_FUNC

#define INST(mnemonic)                \
  do {                                \
    MapKeyValue kv;                   \
    kv.key.p = #mnemonic;             \
    kv.value.p = Assemble_##mnemonic; \
    MapInsert(instructions, kv);      \
  } while (0)

#define INST2(mnemonic, inst)         \
  do {                                \
    MapKeyValue kv;                   \
    kv.key.p = #inst;                 \
    kv.value.p = Assemble_##mnemonic; \
    MapInsert(instructions, kv);      \
  } while (0)

// Add all instructions to the handler map.  This maps the instruction
// spelling to a handler function.
static void InitializeInstructions(Map* instructions) {
  INST(brk);

  INST(bpl);
  INST(bmi);
  INST(bvc);
  INST(bvs);
  INST(bcc);
  INST(bcs);
  INST(bne);
  INST(beq);

  INST(jsr);
  INST(jmp);

  INST(rti);
  INST(rts);

  INST(lda);
  INST(ldx);
  INST(ldy);
  INST(sta);
  INST(stx);
  INST(sty);

  INST(cmp);
  INST(cpy);
  INST(cpx);
  INST(bit);

  INST(ora);
  INST(and);
  INST(eor);

  INST(adc);
  INST(sbc);

  INST(asl);
  INST(rol);
  INST(lsr);
  INST(ror);

  INST(dec);
  INST(inc);
  INST(dey);
  INST(dex);
  INST(iny);
  INST(inx);

  INST(php);
  INST(clc);
  INST(plp);
  INST(sec);
  INST(pha);
  INST(cli);
  INST(pla);
  INST(sei);
  INST(tay);
  INST(clv);
  INST(cld);
  INST(sed);

  INST(tya);
  INST(txa);
  INST(txs);
  INST(tax);
  INST(tsx);

  INST(nop);

  // 65C02
  INST(tsb);
  INST(trb);
  INST(stz);
  INST(phy);
  INST(ply);
  INST(phx);
  INST(plx);
  INST(bra);
}

// Defined below.
static AssemblerSymbol* DefineLabel(Assembler* base_asm, String* spelling);
static void LabelDestruct(Label* label);

// Initialize the assembler.  Returns true if it worked.
bool W65C02AssemblerInit(W65C02Assembler* assembler, String* infile,
                        String* outfile) {
  static int reloc_types[] = {
      R_W65C02_DATA16, R_W65C02_DATA32, R_W65C02_DATA64, R_W65C02_ADD16, R_W65C02_ADD32,
      R_W65C02_ADD64,  R_W65C02_SUB16,  R_W65C02_SUB32,  R_W65C02_SUB64,
  };

  // 4 for the flags specifies the 64 bit float ABI.
  if (!AssemblerInit(&assembler->base, ELF_MACHINE_TYPEW65C02, 1, reloc_types,
                     infile, outfile)) {
    return false;
  }
  // All symbols are absolute value.
  assembler->base.object.absolute = true;

  MapInitForCaseBlindCharPointerKeys(&assembler->instructions);
  MapInitForStringKeys(&assembler->labels);
  MapInitForInt64Keys(&assembler->branches);

  InitializeInstructions(&assembler->instructions);

  // Add a NULL section at the start of the file.
  AssemblerAddSection(&assembler->base, NULL, SHT(null), 0, 0);
  // Add a .bss section.
  assembler->bss = AssemblerAddSection(&assembler->base, NewString(".bss"),
                                       SHT(nobits), SHF(alloc) | SHF(write), 8);

  // Add our own label definer funciton.
  assembler->default_define_label = assembler->base.define_label;
  assembler->base.define_label = DefineLabel;
  return true;
}

static void BranchDelete(Branch* bra) {
  StringDestruct(&bra->label_name);
  free(bra);
}

static void BranchMapDestruct(MapKeyValue* kv) {
  BranchDelete((Branch*)kv->value.p);
}

static void LabelMapDestruct(MapKeyValue* kv) {
  StringDelete((String*)kv->key.p);
  LabelDestruct((Label*)kv->value.p);
  free(kv->value.p);
}

W65C02Assembler* New6502Assembler(String* infile, String* outfile) {
  W65C02Assembler* assembler = malloc(sizeof(W65C02Assembler));
  W65C02AssemblerInit(assembler, infile, outfile);
  return assembler;
}

void W65C02AssemblerFinalize(W65C02Assembler* assembler) {
  CheckBranches(assembler);
}

// Destruct the assembler.
void W65C02AssemblerDestruct(W65C02Assembler* assembler) {
  AssemblerDestruct(&assembler->base);
  MapDestruct(&assembler->instructions);
  MapDestructWithContents(&assembler->branches, BranchMapDestruct);
  MapDestructWithContents(&assembler->labels, LabelMapDestruct);
}

void W65C02AssemblerDelete(W65C02Assembler* assembler) {
  W65C02AssemblerDestruct(assembler);
  free(assembler);
}

// Main assembly function.  This is called by the assembler driver.  It will be
// called twice, one for each pass.
// In pass 1 we parse everything and define all the symbols.
// In pass 2 we also parse everything but we also insert the binary instructions
//    and data into the buffers and expect all symbols to be defined.
void Assemble6502Instruction(Assembler* base, String* word) {
  W65C02Assembler* assembler = (W65C02Assembler*)base;

  void* asm_func = MapFindPointerKey(&assembler->instructions, word->value);
  if (asm_func != NULL) {
    void (*func)(W65C02Assembler*) = asm_func;
    func(assembler);
  } else {
    AssemblerError(&assembler->base, "Syntax error; unknown instruction: %s",
                   word->value);
  }
}

// Shortcut macro avoid typing assembler->base. everywhere we want to access
// the base assembler.
#define ASM assembler->base
#define ASMO (assembler->base.object)

static AssemblerSymbol* GetOrCreateSymbol(W65C02Assembler* assembler,
                                          const char* symbol_name) {
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, symbol_name);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(symbol_name, ASMO.current_section, SYM_TYPE(func),
                             SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  return sym;
}

static Branch* NewBranch(W65C02OpcodeValue opcode, const char* label_name,
                         SourceLocation location) {
  Branch* branch = malloc(sizeof(Branch));
  branch->opcode = opcode;
  StringInit(&branch->label_name, label_name);
  branch->label = NULL;         // Not known yet.
  branch->address = 0;
  branch->location = location;
  return branch;
}

static Label* NewLabel(String* label_name, AssemblerSymbol* symbol) {
  Label* label = malloc(sizeof(Label));
  label->label_name = label_name;
  VectorInit(&label->branches);
  label->symbol = symbol;
  return label;
}

static void LabelDestruct(Label* label) { VectorDestruct(&label->branches); }

// Function sections each start at address 0.  Keying only by offset
// makes a later function reuse an earlier function's Branch, so a
// beq can be emitted as that earlier bne (or the reverse).
static int64_t BranchMapKey(W65C02Assembler* assembler, int64_t addr) {
  return ((int64_t)(uint32_t)ASMO.current_section << 32) | (uint32_t)addr;
}

static void InsertBranch(W65C02Assembler* assembler, int64_t addr,
                         Branch* branch) {
  MapKeyValue kv;
  kv.key.w = BranchMapKey(assembler, addr);
  kv.value.p = branch;
  MapInsert(&assembler->branches, kv);
}

static Branch* FindBranch(W65C02Assembler* assembler, int64_t addr) {
  return MapFindInt64Key(&assembler->branches, BranchMapKey(assembler, addr));
}

static void FixupBranch(MapKeyValue* kv, void* data) {
  Branch* branch = kv->value.p;
  Label* label = data;
  if (branch->label == NULL &&
      StringEqualString(&branch->label_name, label->label_name)) {
    branch->label = label;
    VectorAppend(&label->branches, branch);
  }
}

static void FixupBranches(W65C02Assembler* assembler, Label* label) {
  MapTraverse(&assembler->branches, FixupBranch, label);
}

static void CheckBranch(MapKeyValue* kv, void* data) {
  Branch* branch = kv->value.p;
  Assembler* assembler = data;
  if (branch->label == NULL) {
    AssemblerErrorAtLocation(assembler, branch->location,
                             "Branch to unresolved label %s",
                             branch->label_name.value);
  } else {
    int offset = (int)(branch->label->symbol->value - (branch->address + 2));
    if (offset < -128 || offset > 127) {
      AssemblerErrorAtLocation(assembler, branch->location,
                                "Branch out of range: %d; label %s is at address 0x%x and the branch is address 0x%x",
                                offset, branch->label_name.value, branch->label->symbol->value, branch->address );
    }

  }
}

static void CheckBranches(W65C02Assembler* assembler) {
  MapTraverse(&assembler->branches, CheckBranch, assembler);
}

static Label* InsertLabel(W65C02Assembler* assembler, String* label_name,
                          AssemblerSymbol* symbol) {
  Label* label = NewLabel(label_name, symbol);
  MapKeyValue kv;
  kv.key.p = label_name;
  kv.value.p = label;
  MapInsert(&assembler->labels, kv);
  return label;
}

static Label* FindLabel(W65C02Assembler* assembler, String* label_name) {
  return MapFindPointerKey(&assembler->labels, label_name);
}


// We are defining a label.  This will be referred to by possibly
// multiple branches.
static AssemblerSymbol* DefineLabel(Assembler* base_asm, String* spelling) {
  W65C02Assembler* assembler = (W65C02Assembler*)base_asm;
  AssemblerSymbol* sym = assembler->default_define_label(base_asm, spelling);
  Label* label = FindLabel(assembler, spelling);
  if (label == NULL) {
    label = InsertLabel(assembler, NewString(spelling->value), sym);
  } else {
    label->symbol = sym;
  }

  // In pass 2 we will have all the branches and labels resolved, so we
  // only need to do it in pass 1.
  if (base_asm->object.pass == 1) {
    FixupBranches(assembler, label);
  }
  return sym;
}

static bool AssemblerFunction(W65C02Assembler* assembler, String* func,
                              String* symbol) {
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    StringSet(func, ASM.lex.spelling.value);  // Already initialized.
    LexNextToken(&ASM.lex);
    if (LexMatch(&ASM.lex, TOK(lparen))) {
      if (LexLookingAt(&ASM.lex, TOK(identifier))) {
        // Symbol name.
        StringSet(symbol, ASM.lex.spelling.value);
        LexNextToken(&ASM.lex);
        if (!LexMatch(&ASM.lex, TOK(rparen))) {
          AssemblerError(&ASM, "Syntax error in assembler function: missing )");
          return false;
        }
      } else {
        AssemblerError(
            &ASM, "Syntax error in assembler function: missing symbol name");
        return false;
      }
    } else {
      AssemblerError(&ASM, "Syntax error in assembler function: missing (");
      return false;
    }
  } else {
    AssemblerError(&ASM, "Syntax error in assembler function: missing name");
    return false;
  }
  return true;
}
static void CheckWidth(W65C02Assembler* assembler, int64_t value, int bits) {
  int64_t v = value;
  if (v < 0) {
    v = -v;
  }
  int max = 1 << bits;
  if (v > max - 1) {
    AssemblerError(&assembler->base, "Value 0x%x won't fit in %d bits", value,
                   bits);
  }
}

static void CheckZeroPage(W65C02Assembler* assembler, int64_t value) {
  int64_t v = value;
  if (v < 0) {
    v = -v;
  }
  if (v > 255) {
    AssemblerError(&assembler->base, "0x%x is not a zero page address", value);
  }
}

static void NeedIndexReg(W65C02Assembler* assembler, const char* regname) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&assembler->base, "Expected index register %s", regname);
  } else {
    if (!StringEqualCaseBlind(&ASM.lex.spelling, regname)) {
      AssemblerError(&ASM, "Expected index register %s, not %s", regname,
                     &ASM.lex.spelling);
    } else {
      LexNextToken(&ASM.lex);
    }
  }
}

static bool IsAccumulator(W65C02Assembler* assembler) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    return false;
  }
  if (!StringEqualCaseBlind(&ASM.lex.spelling, "A")) {
    return false;
  }
  LexNextToken(&ASM.lex);
  return true;
}

static void NeedCloseParenthesis(W65C02Assembler* assembler) {
  if (!LexMatch(&ASM.lex, TOK(rparen))) {
    AssemblerError(&assembler->base, "Missing )");
  }
}

static void AssembleSingleByteInstruction(W65C02Assembler* assembler,
                                          int opcode) {
  AssemblerEmitByte(&assembler->base, assembler->base.object.current_section, opcode);
}

// Emit the binary for a branch,
static void EmitBranchBinary(W65C02Assembler* assembler, Branch* branch) {
  AssemblerEmitByte(&ASM, ASMO.current_section, branch->opcode);
  // NOTE: PC is incremented by 2 before being added to the offset.
  int64_t offset = branch->label == NULL
                       ? 0
                       : branch->label->symbol->value - (branch->address + 2);
  AssemblerEmitByte(&ASM, ASMO.current_section, (int8_t)offset);
}

static void AssembleBranch(W65C02Assembler* assembler, int opcode) {
  // The branch instruction must refer to a label.
  Branch* branch = NULL;
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    int64_t branch_address = AssemblerCurrentAddress(&ASM);
    String label_name;
    StringInit(&label_name, ASM.lex.spelling.value);
    branch = FindBranch(assembler, branch_address);
    if (ASMO.pass == 1) {
      if (branch == NULL) {
        branch =
            NewBranch(opcode, label_name.value, ASM.lex.current_token_location);
        InsertBranch(assembler, branch_address, branch);
      }
      branch->address = branch_address;

      Label* label = FindLabel(assembler, &label_name);
      if (label != NULL) {
        branch->label = label;
        VectorAppend(&label->branches, branch);
      } else {
        // Label is not known yet.
      }
    } else {
      // Pass 2: branch must exist.
      if (branch == NULL) {
        AssemblerError(&ASM,
                       "Phase error: branch to %s at address 0x%" PRIx64 " not found; "
                       "check for forward references to labels as expressions, "
                       "possibly with abs,X mode",
                       label_name.value, branch_address);
        StringDestruct(&label_name);
        return;
      }
    }
    StringDestruct(&label_name);
  }
  LexNextToken(&ASM.lex);

  // Emit the binary for the branch, either long or short.
  EmitBranchBinary(assembler, branch);
}

static void AssembleJump(W65C02Assembler* assembler) {
  int opcode = W65C02_OPCODE(jmp);
  if (LexMatch(&ASM.lex, TOK(lparen))) {
    opcode = W65C02_OPCODE(jmpr);
  }
  opcode |= 0xc;  // Both JMP a and JMP (a) have the bottom bits 0xc.
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    // Allow expression for non-section-relative JSR.
    int64_t operand = (int)AssemblerEvaluateExpression(&ASM);
    AssemblerEmitByte(&ASM, ASMO.current_section, opcode);
    AssemblerEmitHalf(&ASM, ASMO.current_section, operand);
    return;
  }
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, ASM.lex.spelling.value);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(ASM.lex.spelling.value, ASMO.current_section,
                             SYM_TYPE(func), SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  sym->exported = true;  // Needs to be exported so we can relocate to it.
  LexNextToken(&ASM.lex);
  
  // Allow +expr or -expr to be used for addend for relocation.
  int addend = 0;
  if (LexLookingAt(&ASM.lex, TOK(plus)) || LexLookingAt(&ASM.lex, TOK(minus))) {
    addend = (int)AssemblerEvaluateExpression(&ASM);
  }

  if (!sym->is_constant) {
    AssemblerRelocation* reloc =
      NewAssemblerRelocation(sym, R_W65C02_JMP, ASMO.current_section,
                             (int32_t)AssemblerCurrentAddress(&ASM), addend);
    AssemblerAddRelocation(&ASM, reloc);
  }
  if (opcode == W65C02_OPCODE(jmpr) && LexMatch(&ASM.lex, TOK(comma))) {
    // 65c02 JMP (abs,X): opcode 0x7c
    opcode = 0x7c;
    NeedIndexReg(assembler, "X");
  }

  AssemblerEmitByte(&ASM, ASMO.current_section, opcode);
  if (sym->is_constant) {
    AssemblerEmitHalf(&ASM, ASMO.current_section, sym->value);
  } else {
    AssemblerEmitHalf(&ASM, ASMO.current_section, 0);
  }
  if (opcode == W65C02_OPCODE(jmpr)) {
    if (!LexMatch(&ASM.lex, TOK(rparen))) {
      AssemblerError(&ASM, "Missing close paren for jmp instruction");
    }
  }
}

static struct {
  const char* func;
  int reloc_type;
} assembler_functions[] = {
    {"lo", R_W65C02_BYTE0},    {"hi", R_W65C02_BYTE1},    {"byte0", R_W65C02_BYTE0},
    {"byte1", R_W65C02_BYTE1}, {"byte2", R_W65C02_BYTE2}, {"byte3", R_W65C02_BYTE3},
    {"byte4", R_W65C02_BYTE4}, {"byte5", R_W65C02_BYTE5}, {"byte6", R_W65C02_BYTE6},
    {"byte7", R_W65C02_BYTE7}, {"abs", R_W65C02_DATA16},
};

#define NUM_ASM_FUNCS \
  (sizeof(assembler_functions) / sizeof(assembler_functions[0]))

// 'offset' is the offset from the current address to which the relocation will
// apply.  If this is an instruction the offset will typically be 1 to
// use the operand and not the opcode.
// Returns length of relocation.
static int AssembleAbsoluteAddress(W65C02Assembler* assembler, int offset) {
  int length = 1;
  String func;
  StringInit(&func, "");
  String symbol_name;
  StringInit(&symbol_name, "");
  bool ok = AssemblerFunction(assembler, &func, &symbol_name);
  if (!ok) {
    goto error;
  }

  AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name.value);
  sym->exported = true;  // Needs to be exported so we can relocate to it.

  int reloc_type = -1;
  for (size_t i = 0; i < NUM_ASM_FUNCS; i++) {
    if (StringEqual(&func, assembler_functions[i].func)) {
      reloc_type = assembler_functions[i].reloc_type;
      break;
    }
  }
  if (reloc_type == -1) {
    AssemblerError(&ASM, "Unknown function %%%s", func.value);
    goto error;
  }

  if (reloc_type == R_W65C02_DATA16) {
    length = 2;
  }
  // Allow a +expr to be the addend for the relocation.
  int addend = 0;
  if (LexMatch(&ASM.lex, TOK(plus))) {
    addend = (int)AssemblerEvaluateExpression(&ASM);
  }
  AssemblerRelocation* reloc = NewAssemblerRelocation(
      sym, reloc_type, ASMO.current_section,
      (int32_t)AssemblerCurrentAddress(&ASM) + offset, addend);
  AssemblerAddRelocation(&ASM, reloc);
error:
  StringDestruct(&func);
  StringDestruct(&symbol_name);
  return length;
}

static int AbsoluteSymbolOrExpression(W65C02Assembler* assembler, bool* is_symbol) {
  int value = 0;
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, ASM.lex.spelling.value);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(ASM.lex.spelling.value, ASMO.current_section,
                             SYM_TYPE(func), SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  LexNextToken(&ASM.lex);
  bool expression_follows = false;
  bool add = true;
  int expr_value = 0;
  
  // Allow an expression to be added or subtracted from the symbol.
  if (LexMatch(&ASM.lex, TOK(plus))) {
    expression_follows = true;
  } else if (LexMatch(&ASM.lex, TOK(minus))) {
    expression_follows = true;
    add = false;
  }
  if (expression_follows) {
    expr_value = (int)AssemblerEvaluateExpression(&ASM);
    if (!add) {
      expr_value = -expr_value;
    }
  }

  // If the symbol is not a constant we have a relocated symbol reference.
  if (!sym->is_constant) {
    sym->exported = true;  // Needs to be exported so we can relocate to it.
    AssemblerRelocation* reloc =
      NewAssemblerRelocation(sym, R_W65C02_JMP, ASMO.current_section,
                             (int32_t)AssemblerCurrentAddress(&ASM), expr_value);
    AssemblerAddRelocation(&ASM, reloc);
    *is_symbol = true;
  } else {
    value = (int)sym->value + expr_value;
  }
  return value;
}

static void AssembleMemoryInstruction(W65C02Assembler* assembler, int opcode) {
  int bbb = 0;
  int cc = opcode & 3;
  int operand_size = 1;
  int operand = 0;
  bool value_known = true;
  bool override_opcode = false;
  bool absolute = false;
  if (LexMatch(&ASM.lex, TOK(hash))) {
    // Immediate.
    switch (cc) {
      case 0:
        bbb = W65C02_ADDR_MODE(00, imm);
        switch (opcode) {
          case W65C02_OPCODE(ldy):
          case W65C02_OPCODE(cpy):
          case W65C02_OPCODE(cpx):
            break;

          case W65C02_OPCODE(bit):
            // This has opcode 0x89.
            opcode = 0x89;
            override_opcode = true;
            break;
          default:
            AssemblerError(
                &ASM, "This instruction does not support immediate operands");
            break;
        }
        break;
      case 1:
        bbb = W65C02_ADDR_MODE(01, imm);
        // STA has no immediate.
        if (opcode == W65C02_OPCODE(sta)) {
          AssemblerError(&ASM, "Invalid immediate operand for STA instruction");
        }
        break;
      case 2:
        bbb = W65C02_ADDR_MODE(10, imm);
        // Only LDX has an immediate variant.
        if (opcode != W65C02_OPCODE(ldx)) {
          AssemblerError(&ASM,
                         "Invalid immediate operand for this instruction");
        }
        break;
    }
    if (LexMatch(&ASM.lex, TOK(percent))) {
      operand_size = AssembleAbsoluteAddress(assembler, 1);
      absolute = operand_size == 2;
    } else {
      int64_t value = AssemblerEvaluateExpression(&ASM);
      CheckWidth(assembler, value, 8);
      operand = (int)value;
    }
  } else {
    switch (cc) {
      case 0: {
        // This can be one of:
        // 1. zero page
        // 2. absolute
        // 3. zero page,X
        // 4. absolute,X
        // NOTE: this is not used for JMP or JMP (addr).
        if (LexMatch(&ASM.lex, TOK(percent))) {
          operand_size = AssembleAbsoluteAddress(assembler, 1);
          absolute = operand_size == 2;
         } else if (LexLookingAt(&ASM.lex, TOK(identifier))) {
          operand = AbsoluteSymbolOrExpression(assembler, &absolute);
        } else {
          operand = (int)AssemblerEvaluateExpression(&ASM);
        }
        if (LexMatch(&ASM.lex, TOK(comma))) {
          NeedIndexReg(assembler, "X");

          // zp,X or abs,X depending on size of value.
          if (!absolute && operand >= 0 && operand < 256) {
            // Only STY and LDY allowed.
            if (opcode != W65C02_OPCODE(sty) && opcode != W65C02_OPCODE(ldy)) {
              AssemblerError(&ASM, "Illegal zeropage,X instruction");
            }
            bbb = W65C02_ADDR_MODE(00, zpx);
          } else {
            // Only LDY.
            if (opcode != W65C02_OPCODE(ldy)) {
              AssemblerError(&ASM, "Illegal abs,X instruction");
            }
            bbb = W65C02_ADDR_MODE(00, absx);
            operand_size = 2;
          }
        } else {
          if (!absolute && operand >= 0 && operand < 256) {
            bbb = W65C02_ADDR_MODE(10, zp);
          } else {
            // Absolute.
            bbb = W65C02_ADDR_MODE(10, abs);
            operand_size = 2;
          }
        }
        break;
      }
      case 1:
        if (LexMatch(&ASM.lex, TOK(lparen))) {
          int64_t value = AssemblerEvaluateExpression(&ASM);
          CheckZeroPage(assembler, value);
          if (LexMatch(&ASM.lex, TOK(rparen))) {
            // (zeropage), Y or (zeropage)
            if (!LexMatch(&ASM.lex, TOK(comma))) {
              bbb = W65C02_ADDR_MODE(10, zpi);
              // These have cc = 2.
              opcode &= ~03;
              opcode |= 2;
            } else {
              NeedIndexReg(assembler, "Y");
              bbb = W65C02_ADDR_MODE(01, zpy);
            }
          } else if (LexMatch(&ASM.lex, TOK(comma))) {
            // (zeropage, X)
            bbb = W65C02_ADDR_MODE(01, zpx);
            NeedIndexReg(assembler, "X");
            NeedCloseParenthesis(assembler);
          }
          operand = (int)value;
        } else {
          // This can be one of:
          // 1. zero page
          // 2. absolute
          // 3. zero page,X
          // 4. absolute,X
          // 5. absolute,Y
          if (LexMatch(&ASM.lex, TOK(percent))) {
            operand_size = AssembleAbsoluteAddress(assembler, 1);
             absolute = operand_size == 2;
           } else if (LexLookingAt(&ASM.lex, TOK(identifier))) {
             operand = AbsoluteSymbolOrExpression(assembler, &absolute);
             if (!absolute) {
               CheckWidth(assembler, operand, 16);
             }
          } else {
            operand = (int)AssemblerEvaluateKnownExpression(&ASM, &value_known);
            CheckWidth(assembler, operand, 16);
          }

          if (LexMatch(&ASM.lex, TOK(comma))) {
            // zero page,X or absolute,X or absolute,Y
            if (LexLookingAt(&ASM.lex, TOK(identifier))) {
              bool is_zero_page =
                  value_known && !absolute ? operand >= 0 && operand < 256 : false;
              operand_size = 2;
              if (StringEqualCaseBlind(&ASM.lex.spelling, "X")) {
                if (is_zero_page) {
                  bbb = W65C02_ADDR_MODE(01, zpxa);
                  operand_size = 1;
                } else {
                  bbb = W65C02_ADDR_MODE(01, absx);
                }
              } else if (StringEqualCaseBlind(&ASM.lex.spelling, "Y")) {
                bbb = W65C02_ADDR_MODE(01, absy);
              }
              LexNextToken(&ASM.lex);
            }
          } else {
            // No comma, so this is zero page or absolute.
            if (value_known && !absolute && operand >= 0 && operand < 256 && operand_size == 1) {
              bbb = W65C02_ADDR_MODE(01, zp);
            } else {
              // Absolute.
              bbb = W65C02_ADDR_MODE(01, abs);
              operand_size = 2;
            }
          }
        }
        break;

      case 2:
        // This can be one of:
        // 1. zero page
        // 2. absolute
        // 3. accumulator
        // 4. zero page,X
        // 5. absolute,X
        // 6. zero page,Y (for LDX and STX)
        // 7. absolute,Y (for LDX and STX)
        if (LexMatch(&ASM.lex, TOK(lparen))) {
          AssemblerError(&ASM, "Illegal addressing mode this instruction");
        } else if (IsAccumulator(assembler) || ASM.lex.current_token == TOK(bad)) {
          bbb = W65C02_ADDR_MODE(10, acc);
          switch (opcode) {
            // Only shifts and rotates have accumulator mode on 6502.
            // On 65c02, we can have inc and dec.
            case W65C02_OPCODE(asl):
            case W65C02_OPCODE(rol):
            case W65C02_OPCODE(lsr):
            case W65C02_OPCODE(ror):
              break;
            case W65C02_OPCODE(inc):
              opcode = W65C02_OPCODE(inca);
              break;
            case W65C02_OPCODE(dec):
              opcode = W65C02_OPCODE(deca);
              break;
            default:
              AssemblerError(&ASM, "Illegal accumulator instruction");
          }
          operand_size = 0;
        } else {
          if (LexMatch(&ASM.lex, TOK(percent))) {
            operand_size = AssembleAbsoluteAddress(assembler, 1);
             absolute = operand_size == 2;
           } else if (LexLookingAt(&ASM.lex, TOK(identifier))) {
             operand = AbsoluteSymbolOrExpression(assembler, &absolute);
           } else {
            operand = (int)AssemblerEvaluateExpression(&ASM);
          }
          if (LexMatch(&ASM.lex, TOK(comma))) {
            const char* index = "X";
            if (opcode == W65C02_OPCODE(ldx) || opcode == W65C02_OPCODE(stx)) {
              index = "Y";
            }
            NeedIndexReg(assembler, index);

            // zp,X or abs,X depending on size of value.
            if (!absolute && operand >= 0 && operand < 256 && operand_size == 1 && index[0] == 'X') {
              bbb = W65C02_ADDR_MODE(10, zpx);
            } else {
              if (index[0] == 'X') {
                bbb = W65C02_ADDR_MODE(10, absx);
              } else {
                bbb = W65C02_ADDR_MODE(01, absy);
              }
              operand_size = 2;
            }
          } else {
            if (!absolute && operand >= 0 && operand < 256 && operand_size == 1) {
              bbb = W65C02_ADDR_MODE(10, zp);
            } else {
              // Absolute.
              bbb = W65C02_ADDR_MODE(10, abs);
              operand_size = 2;
            }
          }
        }
        break;

      default:
        assert(false);
    }
  }

  // Emit opcode byte consiting of aaa, bbb and cc fields.
  if (!override_opcode) {
    opcode |= bbb << 2;
  }
  AssemblerEmitByte(&ASM, ASMO.current_section, opcode);
  switch (operand_size) {
    default:
      assert(false);
      break;
    case 0:
      break;
    case 2:
      // Emit low byte first.
      AssemblerEmitByte(&ASM, ASMO.current_section, operand);
      // Move to high byte.
      operand >>= 8;
    // Fall through.
    case 1:
      AssemblerEmitByte(&ASM, ASMO.current_section, operand);
      break;
  }
}

#define ASSEMBLE_SIMPLE_INST(inst)                                \
  static void Assemble_##inst(W65C02Assembler* assembler) {        \
    AssembleSingleByteInstruction(assembler, W65C02_OPCODE(inst)); \
  }

// BRK is a one byte instruction with an optional operand.  If the operand
// is present it is added immediately after the instruction.
static void Assemble_brk(W65C02Assembler* assembler) {
  int8_t value = 0;
  bool value_present = false;
  if (LexMatch(&ASM.lex, TOK(hash))) {
    // Operand is prefixed by # and must be an 8-bit constant.
    value = AssemblerEvaluateExpression(&ASM);
    value_present = true;
  }
  AssemblerEmitByte(&ASM, ASMO.current_section, W65C02_OPCODE(brk));
  if (value_present) {
    AssemblerEmitByte(&ASM, ASMO.current_section, value);
  }
}

ASSEMBLE_SIMPLE_INST(rti)
ASSEMBLE_SIMPLE_INST(rts)
ASSEMBLE_SIMPLE_INST(php)
ASSEMBLE_SIMPLE_INST(plp)
ASSEMBLE_SIMPLE_INST(pha)
ASSEMBLE_SIMPLE_INST(pla)
ASSEMBLE_SIMPLE_INST(dey)
ASSEMBLE_SIMPLE_INST(tay)
ASSEMBLE_SIMPLE_INST(iny)
ASSEMBLE_SIMPLE_INST(inx)
ASSEMBLE_SIMPLE_INST(clc)
ASSEMBLE_SIMPLE_INST(sec)
ASSEMBLE_SIMPLE_INST(cli)
ASSEMBLE_SIMPLE_INST(sei)
ASSEMBLE_SIMPLE_INST(tya)
ASSEMBLE_SIMPLE_INST(clv)
ASSEMBLE_SIMPLE_INST(cld)
ASSEMBLE_SIMPLE_INST(sed)
ASSEMBLE_SIMPLE_INST(txa)
ASSEMBLE_SIMPLE_INST(txs)
ASSEMBLE_SIMPLE_INST(dex)
ASSEMBLE_SIMPLE_INST(nop)
ASSEMBLE_SIMPLE_INST(tax);
ASSEMBLE_SIMPLE_INST(tsx);

#define ASSEMBLE_BRANCH(inst)                              \
  static void Assemble_##inst(W65C02Assembler* assembler) { \
    AssembleBranch(assembler, W65C02_OPCODE(inst));         \
  }

ASSEMBLE_BRANCH(bpl)
ASSEMBLE_BRANCH(bmi)
ASSEMBLE_BRANCH(bvc)
ASSEMBLE_BRANCH(bvs)
ASSEMBLE_BRANCH(bcc)
ASSEMBLE_BRANCH(bcs)
ASSEMBLE_BRANCH(bne)
ASSEMBLE_BRANCH(beq)
ASSEMBLE_BRANCH(bra)

static void Assemble_jmp(W65C02Assembler* assembler) { AssembleJump(assembler); }

static void Assemble_jsr(W65C02Assembler* assembler) {
  int opcode = W65C02_OPCODE(jsr);
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    // Allow expression for non-section-relative JSR.
    int64_t operand = (int)AssemblerEvaluateExpression(&ASM);
    AssemblerEmitByte(&ASM, ASMO.current_section, opcode);
    AssemblerEmitHalf(&ASM, ASMO.current_section, operand);
    return;
  }
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, ASM.lex.spelling.value);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(ASM.lex.spelling.value, ASMO.current_section,
                             SYM_TYPE(func), SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  if (sym->is_constant) {
    AssemblerEmitByte(&ASM, ASMO.current_section, opcode);
    AssemblerEmitHalf(&ASM, ASMO.current_section, sym->value);
    return;
  }
  sym->exported = true;  // Needs to be exported so we can relocate to it.
  LexNextToken(&ASM.lex);

  // Allow +expr or -expr to be used for addend for relocation.
  int addend = 0;
  if (LexLookingAt(&ASM.lex, TOK(plus)) || LexLookingAt(&ASM.lex, TOK(minus))) {
    addend = (int)AssemblerEvaluateExpression(&ASM);
  }
  AssemblerRelocation* reloc = NewAssemblerRelocation(
      sym, R_W65C02_JSR,
      ASMO.current_section, (int32_t)AssemblerCurrentAddress(&ASM), addend);
  AssemblerAddRelocation(&ASM, reloc);
  AssemblerEmitByte(&ASM, ASMO.current_section, opcode);
  AssemblerEmitHalf(&ASM, ASMO.current_section, 0);
}

// STZ has 4 addressing modes:
// 1. zp
// 2. zp, X
// 3. abs
// 4. abs, X
static void Assemble_stz(W65C02Assembler* assembler) {
  int operand = 0;
  int operand_size = 1;
  int opcode = 0;
  bool absolute = false;
  if (LexMatch(&ASM.lex, TOK(percent))) {
    operand_size = AssembleAbsoluteAddress(assembler, 1);
  } else if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    operand = AbsoluteSymbolOrExpression(assembler, &absolute);
  } else {
    operand = (int)AssemblerEvaluateExpression(&ASM);
    CheckWidth(assembler, operand, 16);
  }

  if (LexMatch(&ASM.lex, TOK(comma))) {
    // zero page,X or absolute,X
    if (LexLookingAt(&ASM.lex, TOK(identifier))) {
      bool is_zero_page = operand >= 0 && operand < 256;
      operand_size = 2;
      if (StringEqualCaseBlind(&ASM.lex.spelling, "X")) {
        if (is_zero_page) {
          opcode = 0x74;  // zp, X
          operand_size = 1;
        } else {
          opcode = 0x9e;  // abs, X
        }
      } else if (StringEqualCaseBlind(&ASM.lex.spelling, "Y")) {
        AssemblerError(&ASM, "Invalid STZ addressing mode");
      }
      LexNextToken(&ASM.lex);
    }
  } else {
    // No comma, so this is zero page or absolute.
    if (!absolute && operand >= 0 && operand < 256) {
      opcode = 0x64;  // zp
    } else {
      // Absolute.
      opcode = 0x9c;
      operand_size = 2;
    }
  }
  AssemblerEmitByte(&ASM, ASMO.current_section, opcode);
  if (operand_size == 1) {
    AssemblerEmitByte(&ASM, ASMO.current_section, operand);
  } else {
    AssemblerEmitHalf(&ASM, ASMO.current_section, operand);
  }
}

#define ASSEMBLE_MEM(inst)                                    \
  static void Assemble_##inst(W65C02Assembler* assembler) {    \
    AssembleMemoryInstruction(assembler, W65C02_OPCODE(inst)); \
  }

ASSEMBLE_MEM(lda);
ASSEMBLE_MEM(ldx);
ASSEMBLE_MEM(ldy);
ASSEMBLE_MEM(sta);
ASSEMBLE_MEM(stx);
ASSEMBLE_MEM(sty);

ASSEMBLE_MEM(cmp);
ASSEMBLE_MEM(cpy);
ASSEMBLE_MEM(cpx);
ASSEMBLE_MEM(bit);

ASSEMBLE_MEM(ora);
ASSEMBLE_MEM(and);
ASSEMBLE_MEM(eor);

ASSEMBLE_MEM(adc);
ASSEMBLE_MEM(sbc);

ASSEMBLE_MEM(asl);
ASSEMBLE_MEM(rol);
ASSEMBLE_MEM(lsr);
ASSEMBLE_MEM(ror);

ASSEMBLE_MEM(dec);
ASSEMBLE_MEM(inc);

#define UNIMPLEMENTED(inst) \
  static void Assemble_##inst(W65C02Assembler* assembler) {}

UNIMPLEMENTED(tsb);
UNIMPLEMENTED(trb);
ASSEMBLE_SIMPLE_INST(phy);
ASSEMBLE_SIMPLE_INST(ply);
ASSEMBLE_SIMPLE_INST(phx);
ASSEMBLE_SIMPLE_INST(plx);
