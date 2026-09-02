//
//  asm_module.c
//  c_compiler
//

#include "asm_module.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "dwarf.h"
#include "elf.h"

static bool IsPowerOfTwo(int value) {
  return value > 0 && (value & (value - 1)) == 0;
}

static bool Require(AsmModule* module, bool condition) {
  if (!condition) {
    module->failed = true;
  }
  return condition;
}

static AsmModuleOp* NewOperation(AsmModule* module, AsmModuleOpKind kind) {
  AsmModuleOp* op = calloc(1, sizeof(*op));
  if (op == NULL) {
    module->failed = true;
    return NULL;
  }
  op->kind = kind;
  VectorAppend(&module->operations, op);
  return op;
}

void AsmExprInitConstant(AsmExpr* expr, int64_t value) {
  memset(expr, 0, sizeof(*expr));
  expr->kind = kAsmExprConstant;
  expr->addend = value;
}

static int64_t InitSymbolAndExtractAddend(String* result,
                                          const char* symbol) {
  const char* plus = strrchr(symbol, '+');
  const char* minus = strrchr(symbol, '-');
  const char* suffix = plus != NULL && (minus == NULL || plus > minus)
                           ? plus
                           : minus;
  int64_t addend = 0;
  if (suffix != NULL && suffix != symbol) {
    char* end = NULL;
    addend = strtoll(suffix, &end, 0);
    if (end == NULL || *end != '\0') {
      suffix = NULL;
      addend = 0;
    }
  }
  StringInit(result, symbol);
  if (suffix != NULL) {
    result->length = (size_t)(suffix - symbol);
    result->value[result->length] = '\0';
  }
  return addend;
}

void AsmExprInitSymbol(AsmExpr* expr, const char* symbol, int64_t addend) {
  memset(expr, 0, sizeof(*expr));
  expr->kind = kAsmExprSymbol;
  expr->addend = addend + InitSymbolAndExtractAddend(&expr->symbol, symbol);
}

void AsmExprInitDifference(AsmExpr* expr, const char* symbol,
                           const char* subtract_symbol, int64_t addend) {
  memset(expr, 0, sizeof(*expr));
  expr->kind = kAsmExprSymbolDifference;
  expr->addend = addend;
  StringInit(&expr->symbol, symbol);
  StringInit(&expr->subtract_symbol, subtract_symbol);
}

void AsmExprForceRelocation(AsmExpr* expr) {
  expr->force_relocation = true;
}

void AsmExprDestruct(AsmExpr* expr) {
  StringDestruct(&expr->symbol);
  StringDestruct(&expr->subtract_symbol);
}

static void CopyExpr(AsmExpr* to, const AsmExpr* from) {
  memset(to, 0, sizeof(*to));
  to->kind = from->kind;
  to->addend = from->addend;
  to->force_relocation = from->force_relocation;
  if (from->symbol.value != NULL) {
    StringInit(&to->symbol, from->symbol.value);
  }
  if (from->subtract_symbol.value != NULL) {
    StringInit(&to->subtract_symbol, from->subtract_symbol.value);
  }
}

void AsmModuleInit(AsmModule* module, const AsmModuleTargetOps* target) {
  VectorInit(&module->operations);
  module->target = target;
  module->failed = false;
}

static void OperationDelete(AsmModule* module, AsmModuleOp* op) {
  switch (op->kind) {
    case kAsmModuleOpSection:
      StringDestruct(&op->u.section.name);
      break;
    case kAsmModuleOpLabel:
      StringDestruct(&op->u.label.name);
      break;
    case kAsmModuleOpSymbol:
      StringDestruct(&op->u.symbol.name);
      break;
    case kAsmModuleOpSymbolSize:
      StringDestruct(&op->u.symbol_size.name);
      AsmExprDestruct(&op->u.symbol_size.value);
      break;
    case kAsmModuleOpInteger:
      AsmExprDestruct(&op->u.integer.value);
      break;
    case kAsmModuleOpBytes:
      free(op->u.bytes.value);
      break;
    case kAsmModuleOpString:
      StringDestruct(&op->u.string.value);
      break;
    case kAsmModuleOpUleb128:
    case kAsmModuleOpSleb128:
      AsmExprDestruct(&op->u.leb128.value);
      break;
    case kAsmModuleOpFile:
      StringDestruct(&op->u.file.name);
      break;
    case kAsmModuleOpComment:
      StringDestruct(&op->u.comment.text);
      break;
    case kAsmModuleOpInstruction:
      if (module->target != NULL &&
          module->target->destroy_instruction != NULL) {
        module->target->destroy_instruction(op->u.instruction.value);
      }
      break;
    case kAsmModuleOpText:
      StringDestruct(&op->u.text.name);
      StringDestruct(&op->u.text.value);
      break;
    case kAsmModuleOpFill:
    case kAsmModuleOpAlign:
    case kAsmModuleOpLocation:
      break;
  }
  free(op);
}

void AsmModuleDestruct(AsmModule* module) {
  for (size_t i = 0; i < module->operations.length; i++) {
    OperationDelete(module, module->operations.value.p[i]);
  }
  VectorDestruct(&module->operations);
}

void AsmModuleSection(AsmModule* module, const char* name, int32_t type,
                      int32_t flags, int32_t alignment) {
  if (!Require(module, name != NULL && name[0] != '\0' &&
                           IsPowerOfTwo(alignment))) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpSection);
  if (op == NULL) {
    return;
  }
  StringInit(&op->u.section.name, name);
  op->u.section.type = type;
  op->u.section.flags = flags;
  op->u.section.alignment = alignment;
}

void AsmModuleLabel(AsmModule* module, const char* name) {
  if (!Require(module, name != NULL && name[0] != '\0')) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpLabel);
  if (op != NULL) {
    StringInit(&op->u.label.name, name);
  }
}

void AsmModuleSymbol(AsmModule* module, const char* name,
                     AssemblerSymbolType type, AssemblerSymbolBinding binding,
                     int32_t size, int32_t alignment, bool defined,
                     bool exported, bool common) {
  if (!Require(module, name != NULL && name[0] != '\0' && size >= 0 &&
                           IsPowerOfTwo(alignment))) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpSymbol);
  if (op == NULL) {
    return;
  }
  StringInit(&op->u.symbol.name, name);
  op->u.symbol.type = type;
  op->u.symbol.binding = binding;
  op->u.symbol.size = size;
  op->u.symbol.alignment = alignment;
  op->u.symbol.defined = defined;
  op->u.symbol.exported = exported;
  op->u.symbol.common = common;
}

void AsmModuleSymbolSize(AsmModule* module, const char* name,
                         const AsmExpr* value) {
  if (!Require(module, name != NULL && name[0] != '\0' && value != NULL)) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpSymbolSize);
  if (op == NULL) {
    return;
  }
  StringInit(&op->u.symbol_size.name, name);
  CopyExpr(&op->u.symbol_size.value, value);
}

void AsmModuleInteger(AsmModule* module, int width, const AsmExpr* value) {
  if (!Require(module, value != NULL &&
                           (width == 1 || width == 2 || width == 4 ||
                            width == 8))) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpInteger);
  if (op == NULL) {
    return;
  }
  op->u.integer.width = width;
  CopyExpr(&op->u.integer.value, value);
}

void AsmModuleBytes(AsmModule* module, const void* bytes, size_t length) {
  if (length == 0) {
    return;
  }
  if (!Require(module, bytes != NULL)) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpBytes);
  if (op == NULL) {
    return;
  }
  op->u.bytes.value = malloc(length);
  if (op->u.bytes.value == NULL) {
    module->failed = true;
    op->u.bytes.length = 0;
    return;
  }
  memcpy(op->u.bytes.value, bytes, length);
  op->u.bytes.length = length;
}

void AsmModuleString(AsmModule* module, const char* value, bool zero_terminate) {
  if (!Require(module, value != NULL)) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpString);
  if (op == NULL) {
    return;
  }
  StringInit(&op->u.string.value, value);
  op->u.string.zero_terminate = zero_terminate;
}

void AsmModuleFill(AsmModule* module, int64_t size, int fill) {
  if (!Require(module, size >= 0 && fill >= 0 && fill <= UINT8_MAX)) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpFill);
  if (op != NULL) {
    op->u.fill.size = size;
    op->u.fill.fill = fill;
  }
}

void AsmModuleAlign(AsmModule* module, int alignment) {
  if (!Require(module, IsPowerOfTwo(alignment))) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpAlign);
  if (op != NULL) {
    op->u.align.alignment = alignment;
  }
}

void AsmModuleUleb128(AsmModule* module, const AsmExpr* value) {
  if (!Require(module, value != NULL)) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpUleb128);
  if (op != NULL) {
    CopyExpr(&op->u.leb128.value, value);
  }
}

void AsmModuleSleb128(AsmModule* module, const AsmExpr* value) {
  if (!Require(module, value != NULL)) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpSleb128);
  if (op != NULL) {
    CopyExpr(&op->u.leb128.value, value);
  }
}

void AsmModuleFile(AsmModule* module, int index, const char* name) {
  if (!Require(module, name != NULL && name[0] != '\0')) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpFile);
  if (op == NULL) {
    return;
  }
  op->u.file.index = index;
  StringInit(&op->u.file.name, name);
}

void AsmModuleLocation(AsmModule* module, int file, int line, int column) {
  if (!Require(module, file > 0 && line >= 0 && column >= 0)) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpLocation);
  if (op != NULL) {
    op->u.location.file = file;
    op->u.location.line = line;
    op->u.location.column = column;
  }
}

void AsmModuleComment(AsmModule* module, const char* text) {
  if (!Require(module, text != NULL)) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpComment);
  if (op != NULL) {
    StringInit(&op->u.comment.text, text);
  }
}

void AsmModuleInstruction(AsmModule* module, void* instruction) {
  if (!Require(module, instruction != NULL && module->target != NULL &&
                           module->target->emit_instruction != NULL &&
                           module->target->write_instruction != NULL)) {
    if (instruction != NULL && module->target != NULL &&
        module->target->destroy_instruction != NULL) {
      module->target->destroy_instruction(instruction);
    }
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpInstruction);
  if (op != NULL) {
    op->u.instruction.value = instruction;
  } else if (module->target != NULL &&
             module->target->destroy_instruction != NULL) {
    module->target->destroy_instruction(instruction);
  }
}

void AsmModuleText(AsmModule* module, const char* name, const char* text,
                   size_t length) {
  if (!Require(module, text != NULL && (length == 0 || text[0] != '\0'))) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpText);
  if (op == NULL) {
    return;
  }
  StringInit(&op->u.text.name, name != NULL ? name : "<assembly>");
  StringInitFromSegment(&op->u.text.value, text, length);
  if (op->u.text.value.length != 0 &&
      op->u.text.value.value[op->u.text.value.length - 1] != '\n') {
    StringAppendChar(&op->u.text.value, '\n');
  }
}

static AssemblerSymbol* EnsureSymbol(Assembler* assembler, const char* name) {
  AssemblerSymbol* symbol = AssemblerFindSymbol(assembler, name);
  if (symbol == NULL && assembler->object.pass == 1) {
    symbol = NewAssemblerSymbol(name, 0, SYM_TYPE(none), SYM_BIND(global), 0);
    AssemblerInsertSymbol(assembler, symbol);
  }
  return symbol;
}

static bool EvaluateExpr(Assembler* assembler, const AsmExpr* expr,
                         int64_t* value) {
  if (expr->kind == kAsmExprConstant) {
    *value = expr->addend;
    return true;
  }
  AssemblerSymbol* symbol =
      AssemblerFindSymbol(assembler, expr->symbol.value);
  if (symbol == NULL || !symbol->defined) {
    return false;
  }
  *value = symbol->value + expr->addend;
  if (expr->kind == kAsmExprSymbolDifference) {
    AssemblerSymbol* subtract =
        AssemblerFindSymbol(assembler, expr->subtract_symbol.value);
    if (subtract == NULL || !subtract->defined ||
        subtract->section != symbol->section) {
      return false;
    }
    *value -= subtract->value;
  }
  return true;
}

static void EmitInteger(Assembler* assembler, int width, const AsmExpr* expr) {
  AsmObject* object = &assembler->object;
  int64_t value = 0;
  bool known = EvaluateExpr(assembler, expr, &value);
  if (expr->force_relocation) {
    known = false;
  }
  if (expr->kind == kAsmExprSymbol) {
    AssemblerSymbol* symbol = EnsureSymbol(assembler, expr->symbol.value);
    if (object->pass == ASM_OBJECT_FINAL_PASS && symbol != NULL) {
      int reloc_index =
          width == 8 ? kRelocSet64 : width == 4 ? kRelocSet32 : kRelocSet16;
      AsmObjectAddRelocationForSymbol(
          object, symbol, object->reloc_types[reloc_index],
          object->current_section, (int32_t)AsmObjectCurrentAddress(object),
          (int32_t)expr->addend);
    }
    value = 0;
    known = true;
  } else if (expr->kind == kAsmExprSymbolDifference && !known &&
             width == 4) {
    AssemblerSymbol* symbol = EnsureSymbol(assembler, expr->symbol.value);
    AssemblerSymbol* subtract =
        EnsureSymbol(assembler, expr->subtract_symbol.value);
    if (subtract != NULL && subtract->defined &&
        subtract->section == object->current_section) {
      if (object->pass == ASM_OBJECT_FINAL_PASS && symbol != NULL) {
        int64_t addend =
            AsmObjectCurrentAddress(object) - subtract->value + expr->addend;
        AsmObjectAddRelocationForSymbol(
            object, symbol, AssemblerRelocTypeForWord(assembler),
            object->current_section, (int32_t)AsmObjectCurrentAddress(object),
            (int32_t)addend);
      }
      value = 0;
      known = true;
    }
  }
  if (!known && object->pass == ASM_OBJECT_FINAL_PASS) {
    AssemblerError(assembler, "Unable to resolve assembly expression");
  }
  switch (width) {
    case 1:
      AsmObjectEmitByte(object, object->current_section, (uint8_t)value);
      break;
    case 2:
      AsmObjectEmitHalf(object, object->current_section, (uint16_t)value);
      break;
    case 4:
      AsmObjectEmitWord(object, object->current_section, (int32_t)value);
      break;
    case 8:
      AsmObjectEmitLong(object, object->current_section, (uint64_t)value);
      break;
    default:
      AssemblerError(assembler, "Unsupported integer width %d", width);
      break;
  }
}

static void EmitLabel(Assembler* assembler, const char* name) {
  AssemblerSymbol* symbol = AssemblerFindSymbol(assembler, name);
  if (assembler->object.pass == 1) {
    if (symbol == NULL) {
      String spelling;
      StringInit(&spelling, name);
      AsmObjectDefineLabel(&assembler->object, assembler, &spelling);
      StringDestruct(&spelling);
      return;
    }
    if (symbol->defined) {
      AssemblerError(assembler, "Duplicate symbol %s", name);
      return;
    }
    symbol->defined = true;
    symbol->is_label = true;
  }
  if (symbol != NULL) {
    symbol->section = assembler->object.current_section;
    symbol->value = AssemblerCurrentAddress(assembler);
  }
}

static void EmitSymbol(Assembler* assembler, const AsmModuleOp* op) {
  AssemblerSymbol* symbol =
      EnsureSymbol(assembler, op->u.symbol.name.value);
  if (symbol == NULL) {
    return;
  }
  if (op->u.symbol.type != SYM_TYPE(none)) {
    symbol->type = op->u.symbol.type;
  }
  if (op->u.symbol.binding != SYM_BIND(global) ||
      symbol->binding != SYM_BIND(weak)) {
    symbol->binding = op->u.symbol.binding;
  }
  if (op->u.symbol.size != 0) {
    symbol->size = op->u.symbol.size;
  }
  if (op->u.symbol.alignment > symbol->alignment) {
    symbol->alignment = op->u.symbol.alignment;
  }
  symbol->exported |= op->u.symbol.exported;
  if (op->u.symbol.common) {
    symbol->section = SHN_COM;
    symbol->value = op->u.symbol.alignment;
    symbol->defined = true;
    symbol->type = SYM_TYPE(common);
  } else if (op->u.symbol.defined) {
    symbol->defined = true;
  }
}

void AsmModuleEmit(AsmModule* module, Assembler* assembler) {
  if (module->failed) {
    return;
  }
  for (size_t i = 0;
       i < module->operations.length && assembler->num_errors == 0; i++) {
    AsmModuleOp* op = module->operations.value.p[i];
    switch (op->kind) {
      case kAsmModuleOpSection: {
        String* name = NewString(op->u.section.name.value);
        int section = AsmObjectEnsureSection(
            &assembler->object, name, op->u.section.type, op->u.section.flags,
            op->u.section.alignment);
        AsmObjectSwitchSection(&assembler->object, section);
        break;
      }
      case kAsmModuleOpLabel:
        EmitLabel(assembler, op->u.label.name.value);
        break;
      case kAsmModuleOpSymbol:
        EmitSymbol(assembler, op);
        break;
      case kAsmModuleOpSymbolSize: {
        int64_t value;
        AssemblerSymbol* symbol =
            EnsureSymbol(assembler, op->u.symbol_size.name.value);
        if (EvaluateExpr(assembler, &op->u.symbol_size.value, &value)) {
          symbol->size = (int32_t)value;
        } else if (assembler->object.pass == ASM_OBJECT_FINAL_PASS) {
          AssemblerError(assembler, "Unable to resolve size of %s",
                         op->u.symbol_size.name.value);
        }
        break;
      }
      case kAsmModuleOpInteger:
        EmitInteger(assembler, op->u.integer.width, &op->u.integer.value);
        break;
      case kAsmModuleOpBytes:
        for (size_t j = 0; j < op->u.bytes.length; j++) {
          AsmObjectEmitByte(&assembler->object,
                            assembler->object.current_section,
                            op->u.bytes.value[j]);
        }
        break;
      case kAsmModuleOpString:
        for (size_t j = 0; j < op->u.string.value.length; j++) {
          AsmObjectEmitByte(&assembler->object,
                            assembler->object.current_section,
                            (uint8_t)op->u.string.value.value[j]);
        }
        if (op->u.string.zero_terminate) {
          AsmObjectEmitByte(&assembler->object,
                            assembler->object.current_section, 0);
        }
        break;
      case kAsmModuleOpFill:
        AsmObjectEmitFill(&assembler->object,
                          assembler->object.current_section, op->u.fill.size,
                          op->u.fill.fill);
        break;
      case kAsmModuleOpAlign:
        AsmObjectAlignCurrentSection(&assembler->object,
                                     op->u.align.alignment);
        break;
      case kAsmModuleOpUleb128:
      case kAsmModuleOpSleb128: {
        if (op->u.leb128.value.kind != kAsmExprConstant) {
          assembler->object.requires_layout_pass = true;
        }
        int64_t value;
        if (!EvaluateExpr(assembler, &op->u.leb128.value, &value)) {
          if (assembler->object.pass == ASM_OBJECT_FINAL_PASS) {
            AssemblerError(assembler, "Unable to resolve LEB128 expression");
          }
          value = 0;
        }
        if (op->kind == kAsmModuleOpUleb128) {
          AsmObjectEmitUleb128(&assembler->object,
                               assembler->object.current_section,
                               (uint64_t)value);
        } else {
          AsmObjectEmitSleb128(&assembler->object,
                               assembler->object.current_section, value);
        }
        break;
      }
      case kAsmModuleOpFile: {
        if (assembler->object.pass == 1) {
          String name;
          StringInit(&name, op->u.file.name.value);
          DwarfAddFile(&assembler->object.dwarf, &name);
          if (op->u.file.index < 0) {
            StringSet(&assembler->object.filename, name.value);
          }
          StringDestruct(&name);
        }
        break;
      }
      case kAsmModuleOpLocation:
        if (assembler->object.pass == 1) {
          DwarfAddLocation(&assembler->object.dwarf, op->u.location.file,
                           op->u.location.line, op->u.location.column,
                           AssemblerCurrentAddress(assembler));
        }
        break;
      case kAsmModuleOpComment:
        break;
      case kAsmModuleOpInstruction:
        if (module->target == NULL ||
            module->target->emit_instruction == NULL) {
          AssemblerError(assembler,
                         "Target instruction emitted without target hooks");
        } else {
          module->target->emit_instruction(
              assembler, op->u.instruction.value);
        }
        break;
      case kAsmModuleOpText:
        if (module->target == NULL || module->target->assemble_text == NULL) {
          AssemblerError(assembler,
                         "Assembly text emitted without a target parser");
        } else {
          AssemblerAssembleInput(assembler, op->u.text.name.value,
                                 &op->u.text.value,
                                 module->target->assemble_text);
        }
        break;
    }
  }
}

static void WriteExpr(FILE* out, const AsmExpr* expr) {
  if (expr->kind == kAsmExprConstant) {
    fprintf(out, "%" PRId64, expr->addend);
    return;
  }
  if (expr->kind == kAsmExprSymbolDifference && !expr->force_relocation) {
    fprintf(out, "(%s-%s", expr->symbol.value,
            expr->subtract_symbol.value);
  } else {
    fputs(expr->symbol.value, out);
    if (expr->kind == kAsmExprSymbolDifference) {
      fprintf(out, "-%s", expr->subtract_symbol.value);
    }
  }
  if (expr->addend != 0) {
    fprintf(out, "%+" PRId64, expr->addend);
  }
  if (expr->kind == kAsmExprSymbolDifference && !expr->force_relocation) {
    fputc(')', out);
  }
}

static void WriteEscapedString(FILE* out, const String* value) {
  fputc('"', out);
  for (size_t i = 0; i < value->length; i++) {
    unsigned char c = (unsigned char)value->value[i];
    switch (c) {
      case '\\':
      case '"':
        fprintf(out, "\\%c", c);
        break;
      case '\n':
        fputs("\\n", out);
        break;
      case '\t':
        fputs("\\t", out);
        break;
      default:
        if (c >= 32 && c < 127) {
          fputc(c, out);
        } else {
          fprintf(out, "\\%03o", c);
        }
        break;
    }
  }
  fputc('"', out);
}

static void WriteSection(FILE* out, const AsmModuleOp* op,
                         bool emit_alignment) {
  bool shorthand = false;
  if (strcmp(op->u.section.name.value, ".text") == 0 ||
      strcmp(op->u.section.name.value, ".data") == 0) {
    fprintf(out, "\t%s\n", op->u.section.name.value);
    shorthand = true;
  }
  if (!shorthand) {
    char flags[8];
    size_t n = 0;
    if ((op->u.section.flags & SHF(alloc)) != 0) flags[n++] = 'a';
    if ((op->u.section.flags & SHF(write)) != 0) flags[n++] = 'w';
    if ((op->u.section.flags & SHF(merge)) != 0) flags[n++] = 'M';
    if ((op->u.section.flags & SHF(strings)) != 0) flags[n++] = 'S';
    if ((op->u.section.flags & SHF(tls)) != 0) flags[n++] = 'T';
    flags[n] = '\0';
    const char* type =
        op->u.section.type == SHT(nobits)
            ? "nobits"
            : op->u.section.type == SHT(init_array)
                  ? "init_array"
                  : op->u.section.type == SHT(fini_array) ? "fini_array"
                                                          : "progbits";
    fprintf(out, "\t.section \"%s\", \"%s\", @%s\n",
            op->u.section.name.value, flags, type);
  }
  if (emit_alignment && op->u.section.alignment > 1) {
    int power = 0;
    for (int alignment = op->u.section.alignment; alignment > 1;
         alignment >>= 1) {
      power++;
    }
    fprintf(out, "\t.p2align %d\n", power);
  }
}

bool AsmModuleWriteText(const AsmModule* module, FILE* out) {
  if (module->failed || out == NULL) {
    return false;
  }
  Vector written_sections;
  VectorInit(&written_sections);
  for (size_t i = 0; i < module->operations.length; i++) {
    const AsmModuleOp* op = module->operations.value.p[i];
    switch (op->kind) {
      case kAsmModuleOpSection: {
        bool first = true;
        for (size_t j = 0; j < written_sections.length; j++) {
          const String* name = written_sections.value.p[j];
          if (strcmp(name->value, op->u.section.name.value) == 0) {
            first = false;
            break;
          }
        }
        if (first) {
          VectorAppend(&written_sections, (void*)&op->u.section.name);
        }
        WriteSection(out, op, first);
        break;
      }
      case kAsmModuleOpLabel:
        fprintf(out, "%s:\n", op->u.label.name.value);
        break;
      case kAsmModuleOpSymbol:
        if (op->u.symbol.binding == SYM_BIND(weak)) {
          fprintf(out, "\t.weak %s\n", op->u.symbol.name.value);
        } else if (op->u.symbol.binding == SYM_BIND(global)) {
          fprintf(out, "\t.global %s\n", op->u.symbol.name.value);
        } else {
          fprintf(out, "\t.local %s\n", op->u.symbol.name.value);
        }
        if (op->u.symbol.type != SYM_TYPE(none)) {
          const char* type =
              op->u.symbol.type == SYM_TYPE(func) ? "function" : "object";
          fprintf(out, "\t.type %s, @%s\n", op->u.symbol.name.value, type);
        }
        if (op->u.symbol.common) {
          fprintf(out, "\t.comm %s,%d,%d\n", op->u.symbol.name.value,
                  op->u.symbol.size, op->u.symbol.alignment);
        } else if (op->u.symbol.size != 0) {
          fprintf(out, "\t.size %s,%d\n", op->u.symbol.name.value,
                  op->u.symbol.size);
        }
        break;
      case kAsmModuleOpSymbolSize:
        fprintf(out, "\t.size %s, ", op->u.symbol_size.name.value);
        WriteExpr(out, &op->u.symbol_size.value);
        fputc('\n', out);
        break;
      case kAsmModuleOpInteger:
        fprintf(out, "\t%s ", op->u.integer.width == 1
                                   ? ".byte"
                                   : op->u.integer.width == 2
                                         ? ".short"
                                         : op->u.integer.width == 4 ? ".word"
                                                                    : ".8byte");
        WriteExpr(out, &op->u.integer.value);
        fputc('\n', out);
        break;
      case kAsmModuleOpBytes:
        for (size_t j = 0; j < op->u.bytes.length; j++) {
          fprintf(out, "\t.byte 0x%02x\n", op->u.bytes.value[j]);
        }
        break;
      case kAsmModuleOpString:
        fprintf(out, "\t.%s ", op->u.string.zero_terminate ? "string" : "ascii");
        WriteEscapedString(out, &op->u.string.value);
        fputc('\n', out);
        break;
      case kAsmModuleOpFill:
        fprintf(out, "\t.space %" PRId64, op->u.fill.size);
        if (op->u.fill.fill != 0) {
          fprintf(out, ", %d", op->u.fill.fill);
        }
        fputc('\n', out);
        break;
      case kAsmModuleOpAlign: {
        int power = 0;
        int alignment = op->u.align.alignment;
        while (alignment > 1) {
          power++;
          alignment >>= 1;
        }
        fprintf(out, "\t.p2align %d\n", power);
        break;
      }
      case kAsmModuleOpUleb128:
      case kAsmModuleOpSleb128:
        fprintf(out, "\t.%s ",
                op->kind == kAsmModuleOpUleb128 ? "uleb128" : "sleb128");
        WriteExpr(out, &op->u.leb128.value);
        fputc('\n', out);
        break;
      case kAsmModuleOpFile:
        if (op->u.file.index >= 0) {
          fprintf(out, "\t.file %d ", op->u.file.index);
        } else {
          fputs("\t.file ", out);
        }
        WriteEscapedString(out, &op->u.file.name);
        fputc('\n', out);
        break;
      case kAsmModuleOpLocation:
        fprintf(out, "\t.loc %d %d %d\n", op->u.location.file,
                op->u.location.line, op->u.location.column);
        break;
      case kAsmModuleOpComment:
        fprintf(out, "\t// %s\n", op->u.comment.text.value);
        break;
      case kAsmModuleOpInstruction:
        if (module->target == NULL ||
            module->target->write_instruction == NULL ||
            !module->target->write_instruction(
                out, op->u.instruction.value)) {
          VectorDestruct(&written_sections);
          return false;
        }
        break;
      case kAsmModuleOpText:
        if (fwrite(op->u.text.value.value, 1, op->u.text.value.length, out) !=
            op->u.text.value.length) {
          VectorDestruct(&written_sections);
          return false;
        }
        if (op->u.text.value.length != 0 &&
            op->u.text.value.value[op->u.text.value.length - 1] != '\n') {
          fputc('\n', out);
        }
        break;
    }
    if (ferror(out)) {
      VectorDestruct(&written_sections);
      return false;
    }
  }
  VectorDestruct(&written_sections);
  return true;
}
