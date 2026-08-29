#include "p_code_object.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "eh_metadata.h"
#include "p_code_encoding.h"
#include "p_code_machine.h"

static void AppendBytes(String* buffer, const void* bytes, size_t size) {
  StringAppendSegment(buffer, bytes, size);
}

static void AppendU32(String* buffer, uint32_t value) {
  unsigned char bytes[4] = {
      (unsigned char)value,
      (unsigned char)(value >> 8),
      (unsigned char)(value >> 16),
      (unsigned char)(value >> 24),
  };
  AppendBytes(buffer, bytes, sizeof(bytes));
}

static void AppendU64(String* buffer, uint64_t value) {
  unsigned char bytes[8];
  for (size_t i = 0; i < sizeof(bytes); i++) {
    bytes[i] = (unsigned char)(value >> (i * 8));
  }
  AppendBytes(buffer, bytes, sizeof(bytes));
}

static void AlignBuffer(String* buffer, size_t alignment) {
  while (alignment != 0 && buffer->length % alignment != 0) {
    unsigned char zero = 0;
    AppendBytes(buffer, &zero, 1);
  }
}

String* PCodeObjectSectionData(PCodeObject* object,
                               PCodeObjectSection section) {
  switch (section) {
    case kPCodeObjectText:
      return &object->text;
    case kPCodeObjectROData:
      return &object->rodata;
    case kPCodeObjectExceptionTable:
      return &object->exception_table;
  }
  return NULL;
}

const String* PCodeObjectSectionDataConst(const PCodeObject* object,
                                          PCodeObjectSection section) {
  return PCodeObjectSectionData((PCodeObject*)object, section);
}

void PCodeObjectInit(PCodeObject* object) {
  StringInit(&object->text, "");
  StringInit(&object->rodata, "");
  StringInit(&object->exception_table, "");
  VectorInit(&object->symbols);
  VectorInit(&object->fixups);
}

void PCodeObjectDestruct(PCodeObject* object) {
  for (size_t i = 0; i < object->symbols.length; i++) {
    PCodeObjectSymbol* symbol = object->symbols.value.p[i];
    if (symbol != NULL) {
      StringDestruct(&symbol->name);
      free(symbol);
    }
  }
  VectorDestruct(&object->symbols);
  for (size_t i = 0; i < object->fixups.length; i++) {
    PCodeObjectFixup* fixup = object->fixups.value.p[i];
    if (fixup != NULL) {
      StringDestruct(&fixup->symbol_name);
      free(fixup);
    }
  }
  VectorDestruct(&object->fixups);
  StringDestruct(&object->text);
  StringDestruct(&object->rodata);
  StringDestruct(&object->exception_table);
}

PCodeObjectSymbol* PCodeObjectFindSymbol(const PCodeObject* object,
                                         const char* name) {
  if (object == NULL || name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < object->symbols.length; i++) {
    PCodeObjectSymbol* symbol = object->symbols.value.p[i];
    if (symbol != NULL && symbol->name.value != NULL &&
        strcmp(symbol->name.value, name) == 0) {
      return symbol;
    }
  }
  return NULL;
}

static bool DefineSymbol(PCodeObject* object, const char* name,
                         PCodeObjectSection section, size_t offset, bool weak,
                         const char** reason) {
  PCodeObjectSymbol* existing = PCodeObjectFindSymbol(object, name);
  if (existing != NULL) {
    if (existing->weak && !weak) {
      existing->section = section;
      existing->offset = offset;
      existing->defined = true;
      existing->weak = false;
      return true;
    }
    if (weak) {
      return true;
    }
    if (reason != NULL) {
      *reason = "duplicate pcode object symbol";
    }
    return false;
  }
  PCodeObjectSymbol* symbol = malloc(sizeof(*symbol));
  if (symbol == NULL) {
    if (reason != NULL) {
      *reason = "could not allocate pcode object symbol";
    }
    return false;
  }
  StringInit(&symbol->name, name);
  symbol->section = section;
  symbol->offset = offset;
  symbol->defined = true;
  symbol->weak = weak;
  VectorAppend(&object->symbols, symbol);
  return true;
}

static bool AddFixup(PCodeObject* object, const char* symbol_name,
                     PCodeObjectSection section, size_t offset,
                     PCodeFixupKind kind, int64_t addend,
                     const char** reason) {
  PCodeObjectFixup* fixup = malloc(sizeof(*fixup));
  if (fixup == NULL) {
    if (reason != NULL) {
      *reason = "could not allocate pcode object fixup";
    }
    return false;
  }
  StringInit(&fixup->symbol_name, symbol_name);
  fixup->section = section;
  fixup->offset = offset;
  fixup->kind = kind;
  fixup->addend = addend;
  VectorAppend(&object->fixups, fixup);
  return true;
}

bool PCodeObjectCopy(PCodeObject* dest, const PCodeObject* source) {
  PCodeObjectInit(dest);
  StringAppendSegment(&dest->text, source->text.value, source->text.length);
  StringAppendSegment(&dest->rodata, source->rodata.value,
                      source->rodata.length);
  StringAppendSegment(&dest->exception_table, source->exception_table.value,
                      source->exception_table.length);
  const char* reason = NULL;
  for (size_t i = 0; i < source->symbols.length; i++) {
    PCodeObjectSymbol* symbol = source->symbols.value.p[i];
    if (!DefineSymbol(dest, symbol->name.value, symbol->section, symbol->offset,
                      symbol->weak, &reason)) {
      PCodeObjectDestruct(dest);
      return false;
    }
  }
  for (size_t i = 0; i < source->fixups.length; i++) {
    PCodeObjectFixup* fixup = source->fixups.value.p[i];
    if (!AddFixup(dest, fixup->symbol_name.value, fixup->section,
                  fixup->offset, fixup->kind, fixup->addend, &reason)) {
      PCodeObjectDestruct(dest);
      return false;
    }
  }
  return true;
}

bool PCodeObjectAppend(PCodeObject* dest, const PCodeObject* source,
                       const char** reason) {
  AlignBuffer(&dest->text, 4);
  AlignBuffer(&dest->rodata, 8);
  AlignBuffer(&dest->exception_table, 8);
  size_t bases[] = {
      dest->text.length,
      dest->rodata.length,
      dest->exception_table.length,
  };
  StringAppendSegment(&dest->text, source->text.value, source->text.length);
  StringAppendSegment(&dest->rodata, source->rodata.value,
                      source->rodata.length);
  StringAppendSegment(&dest->exception_table, source->exception_table.value,
                      source->exception_table.length);
  for (size_t i = 0; i < source->symbols.length; i++) {
    PCodeObjectSymbol* symbol = source->symbols.value.p[i];
    if (!DefineSymbol(dest, symbol->name.value, symbol->section,
                      bases[symbol->section] + symbol->offset, symbol->weak,
                      reason)) {
      return false;
    }
  }
  for (size_t i = 0; i < source->fixups.length; i++) {
    PCodeObjectFixup* fixup = source->fixups.value.p[i];
    if (!AddFixup(dest, fixup->symbol_name.value, fixup->section,
                  bases[fixup->section] + fixup->offset, fixup->kind,
                  fixup->addend, reason)) {
      return false;
    }
  }
  return true;
}

static int MachineOpcode(PCodeOpcode opcode) {
#define MAP_OPCODE(name) \
  case P_OP(name):       \
    return PCODE_OP(name)
  switch (opcode) {
    MAP_OPCODE(decsp);
    MAP_OPCODE(incsp);
    MAP_OPCODE(push);
    MAP_OPCODE(pushf);
    MAP_OPCODE(pushd);
    MAP_OPCODE(pushx);
    MAP_OPCODE(pop);
    MAP_OPCODE(popf);
    MAP_OPCODE(popd);
    MAP_OPCODE(popx);
    MAP_OPCODE(ldw);
    MAP_OPCODE(ldh);
    MAP_OPCODE(ldb);
    MAP_OPCODE(lduw);
    MAP_OPCODE(ldub);
    MAP_OPCODE(lduh);
    MAP_OPCODE(lda);
    MAP_OPCODE(ldx);
    MAP_OPCODE(ldf);
    MAP_OPCODE(ldd);
    MAP_OPCODE(stw);
    MAP_OPCODE(sth);
    MAP_OPCODE(stx);
    MAP_OPCODE(stf);
    MAP_OPCODE(std);
    MAP_OPCODE(stb);
    MAP_OPCODE(add);
    MAP_OPCODE(addf);
    MAP_OPCODE(addd);
    MAP_OPCODE(addc);
    MAP_OPCODE(sub);
    MAP_OPCODE(subf);
    MAP_OPCODE(subd);
    MAP_OPCODE(mul);
    MAP_OPCODE(mulf);
    MAP_OPCODE(muld);
    MAP_OPCODE(div);
    MAP_OPCODE(divu);
    MAP_OPCODE(divf);
    MAP_OPCODE(divd);
    MAP_OPCODE(mod);
    MAP_OPCODE(modu);
    MAP_OPCODE(lsr);
    MAP_OPCODE(asr);
    MAP_OPCODE(lsl);
    MAP_OPCODE(or);
    MAP_OPCODE(and);
    MAP_OPCODE(xor);
    MAP_OPCODE(not);
    MAP_OPCODE(inv);
    MAP_OPCODE(neg);
    MAP_OPCODE(negf);
    MAP_OPCODE(negd);
    MAP_OPCODE(cmpeq);
    MAP_OPCODE(cmpne);
    MAP_OPCODE(cmplt);
    MAP_OPCODE(cmple);
    MAP_OPCODE(cmpgt);
    MAP_OPCODE(cmpge);
    MAP_OPCODE(cmpltu);
    MAP_OPCODE(cmpleu);
    MAP_OPCODE(cmpgtu);
    MAP_OPCODE(cmpgeu);
    MAP_OPCODE(cmpeqf);
    MAP_OPCODE(cmpnef);
    MAP_OPCODE(cmpltf);
    MAP_OPCODE(cmplef);
    MAP_OPCODE(cmpgtf);
    MAP_OPCODE(cmpgef);
    MAP_OPCODE(cmpeqd);
    MAP_OPCODE(cmpned);
    MAP_OPCODE(cmpltd);
    MAP_OPCODE(cmpled);
    MAP_OPCODE(cmpgtd);
    MAP_OPCODE(cmpged);
    MAP_OPCODE(cmp3way);
    MAP_OPCODE(cmp3wayu);
    MAP_OPCODE(cmp3wayf);
    MAP_OPCODE(cmp3wayd);
    MAP_OPCODE(bnz);
    MAP_OPCODE(bz);
    MAP_OPCODE(bra);
    MAP_OPCODE(cbra);
    MAP_OPCODE(i2f);
    MAP_OPCODE(i2d);
    MAP_OPCODE(ui2f);
    MAP_OPCODE(ui2d);
    MAP_OPCODE(f2d);
    MAP_OPCODE(d2f);
    MAP_OPCODE(f2i);
    MAP_OPCODE(d2i);
    MAP_OPCODE(f2ui);
    MAP_OPCODE(d2ui);
    MAP_OPCODE(jmp);
    MAP_OPCODE(cjmp);
    MAP_OPCODE(adr);
    MAP_OPCODE(call);
    MAP_OPCODE(rcall);
    MAP_OPCODE(ret);
    MAP_OPCODE(mov);
    MAP_OPCODE(movf);
    MAP_OPCODE(movd);
    MAP_OPCODE(movc);
    MAP_OPCODE(movfc);
    MAP_OPCODE(movdc);
    MAP_OPCODE(movxc);
    case P_OP(adrs):
    case P_OP(adrtls):
      return PCODE_OP(adr);
    case P_OP(callf):
    case P_OP(calld):
      return PCODE_OP(call);
    case P_OP(rcallf):
    case P_OP(rcalld):
      return PCODE_OP(rcall);
    default:
      return -1;
  }
#undef MAP_OPCODE
}

static int RegisterNumber(TargetInstruction* instruction) {
  return instruction != NULL && instruction->reg != NULL
             ? instruction->reg->num
             : -1;
}

static int OperandRegister(TargetInstruction* instruction, size_t operand) {
  return instruction != NULL && operand < TARGET_MAX_OPERANDS
             ? RegisterNumber(instruction->operand[operand])
             : -1;
}

static void FunctionLabelName(char* buffer, size_t size, const char* function,
                              TargetInstruction* label) {
  snprintf(buffer, size, ".%s_label_%d", function, label->id);
}

static bool EmitRegisterSaveRestore(PCodeObject* object,
                                    PCodeRegisterAllocator* allocator,
                                    bool save, const char** reason) {
  (void)reason;
  Vector regs = {0};
  if (save) {
    BitSetExpand(&allocator->used_int_regs, &regs);
    for (size_t i = 0; i < regs.length; i++) {
      int reg = (int)regs.value.w[i];
      if (reg != PCODE_INT_RETURN_REG) {
        AppendU32(&object->text,
                  PCodeEncodeRegister32(PCODE_OP(pushx), reg));
      }
    }
    VectorClear(&regs);
    BitSetExpand(&allocator->used_float_regs, &regs);
    for (size_t i = 0; i < regs.length; i++) {
      int reg = (int)regs.value.w[i];
      if (reg != PCODE_FLOAT_RETURN_REG) {
        AppendU32(&object->text,
                  PCodeEncodeRegister32(PCODE_OP(pushf), reg));
      }
    }
    VectorClear(&regs);
    BitSetExpand(&allocator->used_double_regs, &regs);
    for (size_t i = 0; i < regs.length; i++) {
      int reg = (int)regs.value.w[i];
      if (reg != PCODE_DOUBLE_RETURN_REG) {
        AppendU32(&object->text,
                  PCodeEncodeRegister32(PCODE_OP(pushd), reg));
      }
    }
  } else {
    BitSetExpand(&allocator->used_double_regs, &regs);
    for (size_t i = regs.length; i > 0; i--) {
      int reg = (int)regs.value.w[i - 1];
      if (reg != PCODE_DOUBLE_RETURN_REG) {
        AppendU32(&object->text,
                  PCodeEncodeRegister32(PCODE_OP(popd), reg));
      }
    }
    VectorClear(&regs);
    BitSetExpand(&allocator->used_float_regs, &regs);
    for (size_t i = regs.length; i > 0; i--) {
      int reg = (int)regs.value.w[i - 1];
      if (reg != PCODE_FLOAT_RETURN_REG) {
        AppendU32(&object->text,
                  PCodeEncodeRegister32(PCODE_OP(popf), reg));
      }
    }
    VectorClear(&regs);
    BitSetExpand(&allocator->used_int_regs, &regs);
    for (size_t i = regs.length; i > 0; i--) {
      int reg = (int)regs.value.w[i - 1];
      if (reg != PCODE_INT_RETURN_REG) {
        AppendU32(&object->text,
                  PCodeEncodeRegister32(PCODE_OP(popx), reg));
      }
    }
  }
  VectorDestruct(&regs);
  return true;
}

static void EmitLandingRestore(PCodeObject* object, PCodeGenerator* pcode) {
  Vector regs = {0};
  int offset = 8;
  BitSetExpand(&pcode->register_allocator.used_double_regs, &regs);
  for (size_t i = regs.length; i > 0; i--) {
    int reg = (int)regs.value.w[i - 1];
    if (reg != PCODE_DOUBLE_RETURN_REG) {
      AppendU32(&object->text,
                PCodeEncodeRegisters64(PCODE_OP(ldd), reg, PCODE_FP_REG));
      AppendU32(&object->text, (uint32_t)offset);
      offset += 8;
    }
  }
  VectorClear(&regs);
  BitSetExpand(&pcode->register_allocator.used_float_regs, &regs);
  for (size_t i = regs.length; i > 0; i--) {
    int reg = (int)regs.value.w[i - 1];
    if (reg != PCODE_FLOAT_RETURN_REG) {
      AppendU32(&object->text,
                PCodeEncodeRegisters64(PCODE_OP(ldf), reg, PCODE_FP_REG));
      AppendU32(&object->text, (uint32_t)offset);
      offset += 4;
    }
  }
  VectorClear(&regs);
  BitSetExpand(&pcode->register_allocator.used_int_regs, &regs);
  for (size_t i = regs.length; i > 0; i--) {
    int reg = (int)regs.value.w[i - 1];
    if (reg != PCODE_INT_RETURN_REG) {
      AppendU32(&object->text,
                PCodeEncodeRegisters64(PCODE_OP(ldx), reg, PCODE_FP_REG));
      AppendU32(&object->text, (uint32_t)offset);
      offset += 8;
    }
  }
  VectorDestruct(&regs);
  AppendU32(&object->text, PCodeEncodeRegisters32(
                               PCODE_OP(mov), PCODE_SP_REG, PCODE_FP_REG, 0));
  if (pcode->base.stack_frame_size > 0) {
    AppendU32(&object->text,
              PCodeEncodeImmediate24(PCODE_OP(decsp),
                                     pcode->base.stack_frame_size));
  }
}

static const char* TargetSymbolText(TargetInstruction* instruction,
                                    char* buffer, size_t size) {
  if (instruction == NULL ||
      instruction->opcode != (TargetOpcode)P_OP(symbol)) {
    return NULL;
  }
  return TargetSymbolName(((TargetSymbol*)instruction)->symbol, buffer, size);
}

static bool EmitInstruction(PCodeObject* object, PCodeGenerator* pcode,
                            TargetInstruction* instruction,
                            const char* function_name, const char** reason) {
  PCodeOpcode opcode = (PCodeOpcode)instruction->opcode;
  int machine_opcode = MachineOpcode(opcode);
  int dest = RegisterNumber(instruction);
  int src0 = OperandRegister(instruction, 0);
  int src1 = OperandRegister(instruction, 1);

  switch (opcode) {
    case P_OP(tmp):
    case P_OP(fp):
    case P_OP(sp):
    case P_OP(ap):
    case P_OP(tp):
    case P_OP(literal):
    case P_OP(structreturn):
    case P_OP(resulti):
    case P_OP(resultf):
    case P_OP(resultd):
    case P_OP(loc):
    case P_OP(symbol):
    case P_OP(ivarreg):
    case P_OP(fvarreg):
      return true;
    case P_OP(label): {
      char name[1024];
      FunctionLabelName(name, sizeof(name), function_name, instruction);
      if (!DefineSymbol(object, name, kPCodeObjectText, object->text.length,
                        false, reason)) {
        return false;
      }
      if ((instruction->flags & TARGET_INST_EXCEPTION_LANDING) != 0) {
        EmitLandingRestore(object, pcode);
      }
      return true;
    }
    case P_OP(named_label): {
      TargetNamedLabel* label = (TargetNamedLabel*)instruction;
      return DefineSymbol(object, label->name, kPCodeObjectText,
                          object->text.length, false, reason);
    }
    case P_OP(save):
      return EmitRegisterSaveRestore(object, &pcode->register_allocator, true,
                                     reason);
    case P_OP(restore):
      return EmitRegisterSaveRestore(object, &pcode->register_allocator, false,
                                     reason);
    case P_OP(asm):
      *reason = "inline assembly is not supported in constexpr pcode";
      return false;
    default:
      break;
  }

  if (TargetIsConst(instruction)) {
    return true;
  }

  if ((opcode == P_OP(mov) || opcode == P_OP(movf) ||
       opcode == P_OP(movd)) &&
      instruction->dest != NULL) {
    int move_dest = RegisterNumber(instruction->dest);
    PCodeOpcode source_opcode =
        instruction->operand[0] != NULL
            ? (PCodeOpcode)instruction->operand[0]->opcode
            : P_OP(tmp);
    int move_source = src0;
    if (source_opcode == P_OP(call) || source_opcode == P_OP(rcall)) {
      move_source = PCODE_INT_RETURN_REG;
    } else if (source_opcode == P_OP(callf) ||
               source_opcode == P_OP(rcallf)) {
      move_source = PCODE_FLOAT_RETURN_REG;
    } else if (source_opcode == P_OP(calld) ||
               source_opcode == P_OP(rcalld)) {
      move_source = PCODE_DOUBLE_RETURN_REG;
    }
    if (move_dest == move_source) {
      return true;
    }
    AppendU32(&object->text, PCodeEncodeRegisters32(
                                 machine_opcode, move_dest, move_source, 0));
    return true;
  }

#define IS_ALU(name) opcode == P_OP(name)
  if (IS_ALU(add) || IS_ALU(addf) || IS_ALU(addd) || IS_ALU(sub) ||
      IS_ALU(subf) || IS_ALU(subd) || IS_ALU(mul) || IS_ALU(mulf) ||
      IS_ALU(muld) || IS_ALU(div) || IS_ALU(divu) || IS_ALU(divf) ||
      IS_ALU(divd) || IS_ALU(mod) || IS_ALU(modu) || IS_ALU(lsr) ||
      IS_ALU(asr) || IS_ALU(lsl) || IS_ALU(or) || IS_ALU(and) ||
      IS_ALU(xor) || IS_ALU(cmpeq) || IS_ALU(cmpne) || IS_ALU(cmplt) ||
      IS_ALU(cmple) || IS_ALU(cmpgt) || IS_ALU(cmpge) || IS_ALU(cmpltu) ||
      IS_ALU(cmpleu) || IS_ALU(cmpgtu) || IS_ALU(cmpgeu) ||
      IS_ALU(cmpeqf) || IS_ALU(cmpnef) || IS_ALU(cmpltf) ||
      IS_ALU(cmplef) || IS_ALU(cmpgtf) || IS_ALU(cmpgef) ||
      IS_ALU(cmpeqd) || IS_ALU(cmpned) || IS_ALU(cmpltd) ||
      IS_ALU(cmpled) || IS_ALU(cmpgtd) || IS_ALU(cmpged) ||
      IS_ALU(cmp3way) || IS_ALU(cmp3wayu) || IS_ALU(cmp3wayf) ||
      IS_ALU(cmp3wayd)) {
    AppendU32(&object->text,
              PCodeEncodeRegisters32(machine_opcode, dest, src0, src1));
    return true;
  }
  if (IS_ALU(not) || IS_ALU(inv) || IS_ALU(neg) || IS_ALU(negf) ||
      IS_ALU(negd) || IS_ALU(i2f) || IS_ALU(i2d) || IS_ALU(ui2f) ||
      IS_ALU(ui2d) || IS_ALU(f2d) || IS_ALU(d2f) || IS_ALU(f2i) ||
      IS_ALU(d2i) || IS_ALU(f2ui) || IS_ALU(d2ui) || IS_ALU(mov) ||
      IS_ALU(movf) || IS_ALU(movd)) {
    AppendU32(&object->text,
              PCodeEncodeRegisters32(machine_opcode, dest, src0, 0));
    return true;
  }
#undef IS_ALU

  switch (opcode) {
    case P_OP(decsp):
    case P_OP(incsp):
      AppendU32(&object->text,
                PCodeEncodeImmediate24(machine_opcode,
                                       TargetIntValue(instruction->operand[0])));
      return true;
    case P_OP(push):
    case P_OP(pushf):
    case P_OP(pushd):
    case P_OP(pushx):
    case P_OP(pop):
    case P_OP(popf):
    case P_OP(popd):
    case P_OP(popx):
      AppendU32(&object->text,
                PCodeEncodeRegister32(machine_opcode,
                                      dest >= 0 ? dest : src0));
      return true;
    case P_OP(ldw):
    case P_OP(ldh):
    case P_OP(ldb):
    case P_OP(lduw):
    case P_OP(ldub):
    case P_OP(lduh):
    case P_OP(lda):
    case P_OP(ldx):
    case P_OP(ldf):
    case P_OP(ldd):
      AppendU32(&object->text,
                PCodeEncodeRegisters64(machine_opcode, dest, src0));
      AppendU32(&object->text,
                (uint32_t)TargetIntValue(instruction->operand[1]));
      return true;
    case P_OP(stw):
    case P_OP(sth):
    case P_OP(stx):
    case P_OP(stf):
    case P_OP(std):
    case P_OP(stb):
      AppendU32(&object->text,
                PCodeEncodeRegisters64(machine_opcode, src0, src1));
      AppendU32(&object->text,
                (uint32_t)TargetIntValue(instruction->operand[2]));
      return true;
    case P_OP(addc):
      AppendU32(&object->text,
                PCodeEncodeRegisters64(machine_opcode, dest, src0));
      AppendU32(&object->text,
                (uint32_t)TargetIntValue(instruction->operand[1]));
      return true;
    case P_OP(movc): {
      int32_t value = (int32_t)TargetIntValue(instruction->operand[0]);
      AppendU32(&object->text,
                PCodeEncodeRegister64(machine_opcode, dest));
      AppendU32(&object->text, (uint32_t)value);
      return true;
    }
    case P_OP(movfc): {
      float value = (float)((TargetConstant*)instruction->operand[0])->value.dvalue;
      uint32_t bits;
      memcpy(&bits, &value, sizeof(bits));
      AppendU32(&object->text,
                PCodeEncodeRegister64(machine_opcode, dest));
      AppendU32(&object->text, bits);
      return true;
    }
    case P_OP(movdc): {
      double value = ((TargetConstant*)instruction->operand[0])->value.dvalue;
      uint64_t bits;
      memcpy(&bits, &value, sizeof(bits));
      AppendU32(&object->text,
                PCodeEncodeRegister96(machine_opcode, dest));
      AppendU64(&object->text, bits);
      return true;
    }
    case P_OP(movxc): {
      size_t offset = object->text.length;
      AppendU32(&object->text,
                PCodeEncodeRegister96(machine_opcode, dest));
      AppendU64(&object->text, 0);
      TargetInstruction* operand = instruction->operand[0];
      if (TargetIsConst(operand)) {
        uint64_t value = (uint64_t)TargetIntValue(operand);
        memcpy(object->text.value + offset + 4, &value, sizeof(value));
        return true;
      }
      char buffer[1024];
      const char* name = TargetSymbolText(operand, buffer, sizeof(buffer));
      if (name == NULL && operand != NULL &&
          operand->opcode == (TargetOpcode)P_OP(literal)) {
        snprintf(buffer, sizeof(buffer), ".str.%d",
                 ((TargetLiteral*)operand)->literal_id);
        name = buffer;
      }
      if (name == NULL) {
        *reason = "unsupported constexpr pcode movxc operand";
        return false;
      }
      return AddFixup(object, name, kPCodeObjectText, offset,
                      kPCodeFixupAbsolute, 0, reason);
    }
    case P_OP(bz):
    case P_OP(bnz): {
      size_t offset = object->text.length;
      AppendU32(&object->text,
                PCodeEncodeRegister64(machine_opcode, src0));
      AppendU32(&object->text, 0);
      char name[1024];
      FunctionLabelName(name, sizeof(name), function_name,
                        instruction->operand[1]);
      return AddFixup(object, name, kPCodeObjectText, offset,
                      kPCodeFixupBranch, 0, reason);
    }
    case P_OP(bra): {
      size_t offset = object->text.length;
      AppendU32(&object->text, PCodeEncodeOpcode64(machine_opcode));
      AppendU32(&object->text, 0);
      char name[1024];
      FunctionLabelName(name, sizeof(name), function_name,
                        instruction->operand[0]);
      return AddFixup(object, name, kPCodeObjectText, offset,
                      kPCodeFixupBranch, 0, reason);
    }
    case P_OP(cbra):
    case P_OP(rcall):
    case P_OP(rcallf):
    case P_OP(rcalld):
      AppendU32(&object->text,
                PCodeEncodeRegister32(machine_opcode, src0));
      return true;
    case P_OP(ret):
      AppendU32(&object->text, PCodeEncodeOpcode32(machine_opcode));
      return true;
    case P_OP(call):
    case P_OP(callf):
    case P_OP(calld):
    case P_OP(jmp):
    case P_OP(cjmp): {
      char buffer[1024];
      const char* name =
          TargetSymbolText(instruction->operand[0], buffer, sizeof(buffer));
      if (name == NULL) {
        *reason = "constexpr pcode call or jump has no symbol";
        return false;
      }
      size_t offset = object->text.length;
      AppendU32(&object->text, PCodeEncodeOpcode96(machine_opcode));
      AppendU64(&object->text, 0);
      PCodeFixupKind kind =
          opcode == P_OP(call) || opcode == P_OP(callf) ||
                  opcode == P_OP(calld)
              ? kPCodeFixupCall
              : kPCodeFixupJump;
      return AddFixup(object, name, kPCodeObjectText, offset, kind, 0,
                      reason);
    }
    case P_OP(adr):
    case P_OP(adrs):
    case P_OP(adrtls): {
      TargetInstruction* operand = instruction->operand[0];
      char buffer[1024];
      const char* name = TargetSymbolText(operand, buffer, sizeof(buffer));
      if (name == NULL && operand != NULL &&
          operand->opcode == (TargetOpcode)P_OP(literal)) {
        snprintf(buffer, sizeof(buffer), ".str.%d",
                 ((TargetLiteral*)operand)->literal_id);
        name = buffer;
      }
      if (name == NULL || opcode == P_OP(adrtls)) {
        *reason = "unsupported constexpr pcode address operand";
        return false;
      }
      size_t offset = object->text.length;
      AppendU32(&object->text,
                PCodeEncodeRegister96(machine_opcode, dest));
      AppendU64(&object->text, 0);
      return AddFixup(object, name, kPCodeObjectText, offset,
                      opcode == P_OP(adrs) ? kPCodeFixupPCRelative
                                           : kPCodeFixupAddress,
                      0, reason);
    }
    default:
      *reason = "unsupported instruction in direct constexpr pcode image";
      return false;
  }
}

static bool EmitTypeInfoRecords(PCodeObject* object, PCodeGenerator* pcode,
                                const char** reason) {
  for (size_t i = 0; i < pcode->exception_typeinfos.length; i++) {
    EHTypeInfo* info = pcode->exception_typeinfos.value.p[i];
    const char* name = info->symbol_name.value;
    char symbol[2048];
    AlignBuffer(&object->rodata, 8);
    snprintf(symbol, sizeof(symbol), "%s_name", name);
    if (!DefineSymbol(object, symbol, kPCodeObjectROData,
                      object->rodata.length, true, reason)) {
      return false;
    }
    AppendBytes(&object->rodata, info->type_name.value,
                info->type_name.length + 1);
    for (size_t b = 0; b < info->bases.length; b++) {
      EHTypeInfoBase* base = info->bases.value.p[b];
      snprintf(symbol, sizeof(symbol), "%s_base%zu_name", name, b);
      if (!DefineSymbol(object, symbol, kPCodeObjectROData,
                        object->rodata.length, true, reason)) {
        return false;
      }
      AppendBytes(&object->rodata, base->base_name.value,
                  base->base_name.length + 1);
    }
    if (info->bases.length > 0) {
      AlignBuffer(&object->rodata, 8);
      snprintf(symbol, sizeof(symbol), "%s_bases", name);
      if (!DefineSymbol(object, symbol, kPCodeObjectROData,
                        object->rodata.length, true, reason)) {
        return false;
      }
      for (size_t b = 0; b < info->bases.length; b++) {
        EHTypeInfoBase* base = info->bases.value.p[b];
        size_t offset = object->rodata.length;
        AppendU64(&object->rodata, 0);
        snprintf(symbol, sizeof(symbol), "%s_base%zu_name", name, b);
        if (!AddFixup(object, symbol, kPCodeObjectROData, offset,
                      kPCodeFixupData64, 0, reason)) {
          return false;
        }
        AppendU64(&object->rodata, (uint64_t)base->offset);
      }
    }
    AlignBuffer(&object->rodata, 8);
    if (!DefineSymbol(object, name, kPCodeObjectROData, object->rodata.length,
                      true, reason)) {
      return false;
    }
    size_t offset = object->rodata.length;
    AppendU64(&object->rodata, 0);
    snprintf(symbol, sizeof(symbol), "%s_name", name);
    if (!AddFixup(object, symbol, kPCodeObjectROData, offset,
                  kPCodeFixupData64, 0, reason)) {
      return false;
    }
    AppendU64(&object->rodata, info->bases.length);
    if (info->bases.length > 0) {
      offset = object->rodata.length;
      AppendU64(&object->rodata, 0);
      snprintf(symbol, sizeof(symbol), "%s_bases", name);
      if (!AddFixup(object, symbol, kPCodeObjectROData, offset,
                    kPCodeFixupData64, 0, reason)) {
        return false;
      }
    } else {
      AppendU64(&object->rodata, 0);
    }
    AppendU64(&object->rodata, (uint64_t)info->object_size);
    AppendU64(&object->rodata, info->object_is_class ? 1 : 0);
  }
  return true;
}

static bool EmitExceptionTable(PCodeObject* object, PCodeGenerator* pcode,
                               const char* function_name,
                               const char** reason) {
  if (pcode->exception_ranges.length == 0) {
    return true;
  }
  AlignBuffer(&object->exception_table, 8);
  for (size_t i = 0; i < pcode->exception_ranges.length; i++) {
    PCodeExceptionRange* range = pcode->exception_ranges.value.p[i];
    TargetInstruction* labels[] = {
        range->try_start,
        range->try_end,
        range->catch_label,
    };
    for (size_t label = 0; label < 3; label++) {
      char name[1024];
      FunctionLabelName(name, sizeof(name), function_name, labels[label]);
      size_t offset = object->exception_table.length;
      AppendU64(&object->exception_table, 0);
      if (!AddFixup(object, name, kPCodeObjectExceptionTable, offset,
                    kPCodeFixupData64, 0, reason)) {
        return false;
      }
    }
    if (range->is_cleanup) {
      AppendU64(&object->exception_table, DAVECC_EH_CLEANUP_MARKER);
    } else if (range->catch_typeinfo != NULL) {
      size_t offset = object->exception_table.length;
      AppendU64(&object->exception_table, 0);
      if (!AddFixup(object, range->catch_typeinfo->symbol_name.value,
                    kPCodeObjectExceptionTable, offset, kPCodeFixupData64, 0,
                    reason)) {
        return false;
      }
    } else {
      AppendU64(&object->exception_table, 0);
    }
  }
  return true;
}

bool PCodeObjectBuildFunction(PCodeObject* object, PCodeGenerator* pcode,
                              const char** reason) {
  if (object == NULL || pcode == NULL) {
    *reason = "invalid direct pcode object input";
    return false;
  }
  const char* function_name = pcode->base.function_name.value;
  if (!DefineSymbol(object, function_name, kPCodeObjectText,
                    object->text.length, pcode->base.is_weak, reason)) {
    return false;
  }
  for (TargetInstruction* instruction = TargetFirstInstruction(&pcode->base);
       instruction != NULL; instruction = TargetNext(instruction)) {
    if (!EmitInstruction(object, pcode, instruction, function_name, reason)) {
      return false;
    }
  }
  return EmitTypeInfoRecords(object, pcode, reason) &&
         EmitExceptionTable(object, pcode, function_name, reason);
}
