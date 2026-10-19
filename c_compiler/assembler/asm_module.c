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

enum { kNamePoolBlockSize = 64 * 1024 };

// FNV-1a hash of a NUL-terminated name.
static uint64_t HashName(const char* name) {
  uint64_t hash = 14695981039346656037ull;
  for (const unsigned char* p = (const unsigned char*)name; *p != '\0'; p++) {
    hash = (hash ^ *p) * 1099511628211ull;
  }
  return hash;
}

// Doubles the pool's table (or creates it) and rehashes every name into it.
static bool GrowNamePool(AsmNamePool* pool) {
  size_t capacity = pool->capacity == 0 ? 1024 : pool->capacity * 2;
  const char** slots = calloc(capacity, sizeof(*slots));
  if (slots == NULL) {
    return false;
  }
  for (size_t i = 0; i < pool->capacity; i++) {
    const char* name = pool->slots[i];
    if (name == NULL) {
      continue;
    }
    size_t j = HashName(name) & (capacity - 1);
    while (slots[j] != NULL) {
      j = (j + 1) & (capacity - 1);
    }
    slots[j] = name;
  }
  free(pool->slots);
  pool->slots = slots;
  pool->capacity = capacity;
  return true;
}

// Carves `size` bytes out of the pool's current block, starting a new block
// when it runs out.  A name too large to share a block gets its own.
static char* AllocateName(AsmNamePool* pool, size_t size) {
  if (size > pool->free_length) {
    bool dedicated = size > kNamePoolBlockSize / 4;
    char* block = malloc(dedicated ? size : kNamePoolBlockSize);
    if (block == NULL) {
      return NULL;
    }
    VectorAppend(&pool->blocks, block);
    if (dedicated) {
      return block;
    }
    pool->free_space = block;
    pool->free_length = kNamePoolBlockSize;
  }
  char* result = pool->free_space;
  pool->free_space += size;
  pool->free_length -= size;
  return result;
}

static void NamePoolDestruct(AsmNamePool* pool) {
  for (size_t i = 0; i < pool->blocks.length; i++) {
    free(pool->blocks.value.p[i]);
  }
  VectorDestruct(&pool->blocks);
  free(pool->slots);
  memset(pool, 0, sizeof(*pool));
}

const char* AsmModuleInternName(AsmModule* module, const char* name) {
  AsmNamePool* pool = &module->names;
  // Stay at most half full so probe sequences remain short.
  if ((pool->count + 1) * 2 > pool->capacity && !GrowNamePool(pool)) {
    module->failed = true;
    return NULL;
  }
  size_t mask = pool->capacity - 1;
  size_t i = HashName(name) & mask;
  for (; pool->slots[i] != NULL; i = (i + 1) & mask) {
    if (strcmp(pool->slots[i], name) == 0) {
      return pool->slots[i];
    }
  }
  size_t size = strlen(name) + 1;
  char* copy = AllocateName(pool, size);
  if (copy == NULL) {
    module->failed = true;
    return NULL;
  }
  memcpy(copy, name, size);
  pool->slots[i] = copy;
  pool->count++;
  return copy;
}

// Bytes needed for an operation of `kind`: the header plus only its own union
// member.  Instructions dominate the stream and need a pointer, not the full
// union, so operations are allocated at this size and must never be copied
// or accessed through a different member.
static size_t OperationSize(AsmModuleOpKind kind) {
#define OP_SIZE(member) \
  (offsetof(AsmModuleOp, u) + sizeof(((AsmModuleOp*)0)->u.member))
  switch (kind) {
    case kAsmModuleOpSection:
      return OP_SIZE(section);
    case kAsmModuleOpLabel:
      return OP_SIZE(label);
    case kAsmModuleOpSymbol:
      return OP_SIZE(symbol);
    case kAsmModuleOpSymbolSize:
      return OP_SIZE(symbol_size);
    case kAsmModuleOpInteger:
      return OP_SIZE(integer);
    case kAsmModuleOpBytes:
      return OP_SIZE(bytes);
    case kAsmModuleOpString:
      return OP_SIZE(string);
    case kAsmModuleOpFill:
      return OP_SIZE(fill);
    case kAsmModuleOpAlign:
      return OP_SIZE(align);
    case kAsmModuleOpUleb128:
    case kAsmModuleOpSleb128:
      return OP_SIZE(leb128);
    case kAsmModuleOpFile:
      return OP_SIZE(file);
    case kAsmModuleOpLocation:
      return OP_SIZE(location);
    case kAsmModuleOpComment:
      return OP_SIZE(comment);
    case kAsmModuleOpInstruction:
      return OP_SIZE(instruction);
    case kAsmModuleOpText:
      return OP_SIZE(text);
  }
#undef OP_SIZE
  return sizeof(AsmModuleOp);
}

static AsmModuleOp* NewOperation(AsmModule* module, AsmModuleOpKind kind) {
  AsmModuleOp* op = calloc(1, OperationSize(kind));
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

static void CopyExpr(AsmModule* module, AsmModuleExpr* to,
                     const AsmExpr* from) {
  to->kind = from->kind;
  to->addend = from->addend;
  to->force_relocation = from->force_relocation;
  to->symbol = from->symbol.value != NULL
                   ? AsmModuleInternName(module, from->symbol.value)
                   : NULL;
  to->subtract_symbol =
      from->subtract_symbol.value != NULL
          ? AsmModuleInternName(module, from->subtract_symbol.value)
          : NULL;
}

void AsmModuleInit(AsmModule* module, const AsmModuleTargetOps* target) {
  VectorInit(&module->operations);
  module->target = target;
  module->failed = false;
  module->keep_text = true;
  memset(&module->names, 0, sizeof(module->names));
  VectorInit(&module->names.blocks);
}

static void OperationDelete(AsmModule* module, AsmModuleOp* op) {
  switch (op->kind) {
    case kAsmModuleOpBytes:
      free(op->u.bytes.value);
      break;
    case kAsmModuleOpString:
      StringDestruct(&op->u.string.value);
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
    case kAsmModuleOpSection:
    case kAsmModuleOpLabel:
    case kAsmModuleOpSymbol:
    case kAsmModuleOpSymbolSize:
    case kAsmModuleOpInteger:
    case kAsmModuleOpUleb128:
    case kAsmModuleOpSleb128:
    case kAsmModuleOpFile:
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
  NamePoolDestruct(&module->names);
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
  op->u.section.name = AsmModuleInternName(module, name);
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
    op->u.label.name = AsmModuleInternName(module, name);
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
  op->u.symbol.name = AsmModuleInternName(module, name);
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
  op->u.symbol_size.name = AsmModuleInternName(module, name);
  CopyExpr(module, &op->u.symbol_size.value, value);
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
  CopyExpr(module, &op->u.integer.value, value);
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
    CopyExpr(module, &op->u.leb128.value, value);
  }
}

void AsmModuleSleb128(AsmModule* module, const AsmExpr* value) {
  if (!Require(module, value != NULL)) {
    return;
  }
  AsmModuleOp* op = NewOperation(module, kAsmModuleOpSleb128);
  if (op != NULL) {
    CopyExpr(module, &op->u.leb128.value, value);
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
  op->u.file.name = AsmModuleInternName(module, name);
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
  if (!Require(module, text != NULL) || !module->keep_text) {
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

static bool EvaluateExpr(Assembler* assembler, const AsmModuleExpr* expr,
                         int64_t* value) {
  if (expr->kind == kAsmExprConstant) {
    *value = expr->addend;
    return true;
  }
  AssemblerSymbol* symbol =
      AssemblerFindSymbol(assembler, expr->symbol);
  if (symbol == NULL || !symbol->defined) {
    return false;
  }
  *value = symbol->value + expr->addend;
  if (expr->kind == kAsmExprSymbolDifference) {
    AssemblerSymbol* subtract =
        AssemblerFindSymbol(assembler, expr->subtract_symbol);
    if (subtract == NULL || !subtract->defined ||
        subtract->section != symbol->section) {
      return false;
    }
    *value -= subtract->value;
  }
  return true;
}

static void EmitInteger(Assembler* assembler, int width, const AsmModuleExpr* expr) {
  AsmObject* object = &assembler->object;
  int64_t value = 0;
  bool known = EvaluateExpr(assembler, expr, &value);
  if (expr->force_relocation) {
    known = false;
  }
  if (expr->kind == kAsmExprSymbol) {
    AssemblerSymbol* symbol = EnsureSymbol(assembler, expr->symbol);
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
    AssemblerSymbol* symbol = EnsureSymbol(assembler, expr->symbol);
    AssemblerSymbol* subtract =
        EnsureSymbol(assembler, expr->subtract_symbol);
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
    AssemblerError(assembler,
                   "Unable to resolve assembly expression (kind=%d width=%d %s - %s)",
                   (int)expr->kind, width,
                   expr->symbol != NULL ? expr->symbol : "?",
                   expr->subtract_symbol != NULL
                       ? expr->subtract_symbol
                       : "?");
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
      EnsureSymbol(assembler, op->u.symbol.name);
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
  if (op->u.symbol.common && op->u.symbol.binding == SYM_BIND(local)) {
    AssemblerAllocateLocalCommon(assembler, symbol, op->u.symbol.size,
                                 op->u.symbol.alignment);
  } else if (op->u.symbol.common) {
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
        String* name = NewString(op->u.section.name);
        int section = AsmObjectEnsureSection(
            &assembler->object, name, op->u.section.type, op->u.section.flags,
            op->u.section.alignment);
        AsmObjectSwitchSection(&assembler->object, section);
        break;
      }
      case kAsmModuleOpLabel:
        EmitLabel(assembler, op->u.label.name);
        break;
      case kAsmModuleOpSymbol:
        EmitSymbol(assembler, op);
        break;
      case kAsmModuleOpSymbolSize: {
        int64_t value;
        AssemblerSymbol* symbol =
            EnsureSymbol(assembler, op->u.symbol_size.name);
        if (EvaluateExpr(assembler, &op->u.symbol_size.value, &value)) {
          symbol->size = (int32_t)value;
        } else if (assembler->object.pass == ASM_OBJECT_FINAL_PASS) {
          AssemblerError(assembler, "Unable to resolve size of %s",
                         op->u.symbol_size.name);
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
          StringInit(&name, op->u.file.name);
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
                           assembler->object.current_section,
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

static void WriteExpr(FILE* out, const AsmModuleExpr* expr) {
  if (expr->kind == kAsmExprConstant) {
    fprintf(out, "%" PRId64, expr->addend);
    return;
  }
  if (expr->kind == kAsmExprSymbolDifference && !expr->force_relocation) {
    fprintf(out, "(%s-%s", expr->symbol,
            expr->subtract_symbol);
  } else {
    fputs(expr->symbol, out);
    if (expr->kind == kAsmExprSymbolDifference) {
      fprintf(out, "-%s", expr->subtract_symbol);
    }
  }
  if (expr->addend != 0) {
    fprintf(out, "%+" PRId64, expr->addend);
  }
  if (expr->kind == kAsmExprSymbolDifference && !expr->force_relocation) {
    fputc(')', out);
  }
}

static void WriteEscapedString(FILE* out, const char* value, size_t length) {
  fputc('"', out);
  for (size_t i = 0; i < length; i++) {
    unsigned char c = (unsigned char)value[i];
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
  if (strcmp(op->u.section.name, ".text") == 0 ||
      strcmp(op->u.section.name, ".data") == 0) {
    fprintf(out, "\t%s\n", op->u.section.name);
    shorthand = true;
  }
  if (!shorthand) {
    char flags[8];
    size_t n = 0;
    if ((op->u.section.flags & SHF(alloc)) != 0) flags[n++] = 'a';
    if ((op->u.section.flags & SHF(write)) != 0) flags[n++] = 'w';
    if ((op->u.section.flags & SHF(execinstr)) != 0) flags[n++] = 'x';
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
            op->u.section.name, flags, type);
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
          const char* name = written_sections.value.p[j];
          if (strcmp(name, op->u.section.name) == 0) {
            first = false;
            break;
          }
        }
        if (first) {
          VectorAppend(&written_sections, (void*)op->u.section.name);
        }
        WriteSection(out, op, first);
        break;
      }
      case kAsmModuleOpLabel:
        fprintf(out, "%s:\n", op->u.label.name);
        break;
      case kAsmModuleOpSymbol:
        if (op->u.symbol.binding == SYM_BIND(weak)) {
          fprintf(out, "\t.weak %s\n", op->u.symbol.name);
        } else if (op->u.symbol.binding == SYM_BIND(global)) {
          fprintf(out, "\t.global %s\n", op->u.symbol.name);
        } else {
          fprintf(out, "\t.local %s\n", op->u.symbol.name);
        }
        if (op->u.symbol.type != SYM_TYPE(none)) {
          const char* type =
              op->u.symbol.type == SYM_TYPE(func) ? "function" : "object";
          fprintf(out, "\t.type %s, @%s\n", op->u.symbol.name, type);
        }
        if (op->u.symbol.common) {
          fprintf(out, "\t.comm %s,%d,%d\n", op->u.symbol.name,
                  op->u.symbol.size, op->u.symbol.alignment);
        } else if (op->u.symbol.size != 0) {
          fprintf(out, "\t.size %s,%d\n", op->u.symbol.name,
                  op->u.symbol.size);
        }
        break;
      case kAsmModuleOpSymbolSize:
        fprintf(out, "\t.size %s, ", op->u.symbol_size.name);
        WriteExpr(out, &op->u.symbol_size.value);
        fputc('\n', out);
        break;
      case kAsmModuleOpInteger:
        fprintf(out, "\t%s ", op->u.integer.width == 1
                                   ? ".byte"
                                   : op->u.integer.width == 2
                                         ? ".short"
                                         : op->u.integer.width == 4 ? ".4byte"
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
        WriteEscapedString(out, op->u.string.value.value,
                           op->u.string.value.length);
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
        WriteEscapedString(out, op->u.file.name, strlen(op->u.file.name));
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
