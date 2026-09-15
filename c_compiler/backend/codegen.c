//
//  codegen.c
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "assembler.h"
#include "ast.h"
#include "basic_block.h"
#include "compiler.h"
#include "errors.h"
#include "expr_codegen.h"
#include "gvn.h"
#include "list.h"
#include "member_pointer.h"
#include "optimizer.h"
#include "rtti.h"
#include "sccp.h"
#include "ssa.h"
#include "statement_codegen.h"
#include <assert.h>
#include "constprop.h"
#include "codemotion.h"
#include "copyprop.h"
#include "dce.h"
#include "induction.h"
#include "loop_info.h"
#include "memopt.h"
#include "sroa.h"
#include "unroll.h"
#include "vectorize.h"

static void Trap() {}
static void TrapInstruction(IRNode* inst) {
  if (inst->id == 19) {
    Trap();
  }
}

static uint64_t HashExceptionTypePart(uint64_t hash, uint64_t value) {
  hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
  return hash;
}

uint64_t CXXExceptionTypeID(TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  while (TypeIsReference(type)) {
    type = type->next;
  }

  uint64_t hash = 0xcbf29ce484222325ULL;
  while (type != NULL) {
    hash = HashExceptionTypePart(hash, (uint64_t)type->declarator);
    switch (type->declarator) {
      case kDeclPointer:
        hash = HashExceptionTypePart(hash, (uint64_t)(type->qualifiers & ~kQualConst));
        type = type->next;
        continue;
      case kDeclMemberPointer:
        hash = HashExceptionTypePart(hash,
                                    (uint64_t)(uintptr_t)type->info.struct_info);
        type = type->next;
        continue;
      case kDeclArray:
        hash = HashExceptionTypePart(hash, (uint64_t)type->info.array.size.fixed);
        type = type->next;
        continue;
      case kDeclVector:
        hash = HashExceptionTypePart(hash,
                                    (uint64_t)type->info.array.size.fixed);
        type = type->next;
        continue;
      case kDeclFunction:
        hash = HashExceptionTypePart(hash, (uint64_t)kDeclFunction);
        type = type->next;
        continue;
      case kDeclReference:
      case kDeclRValueReference:
        type = type->next;
        continue;
      case kDeclPrimitive:
        hash = HashExceptionTypePart(hash, (uint64_t)type->type);
        if (TypeIsStructOrUnion(type)) {
          hash = HashExceptionTypePart(hash,
                                      (uint64_t)(uintptr_t)type->info.struct_info);
        } else if (TypeIsEnum(type)) {
          hash = HashExceptionTypePart(hash,
                                      (uint64_t)(uintptr_t)type->info.enum_info);
        }
        return hash == 0 ? 1 : hash;
    }
  }
  return hash == 0 ? 1 : hash;
}

static TypeRecord* CXXExceptionCanonicalType(TypeRecord* type) {
  while (type != NULL && TypeIsReference(type)) {
    type = type->next;
  }
  return type;
}

static void CXXExceptionTypeName(TypeRecord* type, String* result) {
  type = CXXExceptionCanonicalType(type);
  if (type == NULL) {
    StringAppend(result, "<unknown>");
    return;
  }
  Qualifiers old_qualifiers = type->qualifiers;
  type->qualifiers &= ~kQualConst;
  TypeRecordToString(type, result);
  type->qualifiers = old_qualifiers;
}

static void SanitizeTypeInfoSymbolName(String* name) {
  for (size_t i = 0; i < name->length; i++) {
    char ch = name->value[i];
    bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                 (ch >= '0' && ch <= '9') || ch == '_';
    if (!valid) {
      name->value[i] = '_';
    }
  }
}

// Flattens the public, non-virtual base graph of an exception class into
// `out` (EHTypeInfoBase* entries), recording each base's exception type name
// and its byte offset from the most-derived object.  Because exceptions are
// thrown with their static type, these offsets are exact for adjusting the
// exception object pointer when a handler names a base class.  Virtual and
// non-public bases are skipped: a handler only matches an accessible base, and
// virtual-base offsets are not modelled here (matching RTTI's limitations).
static void CollectExceptionBaseTypes(TypeRecord* type, int64_t base_offset,
                                      Vector* out) {
  type = CXXExceptionCanonicalType(type);
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return;
  }
  Struct* str = type->info.struct_info;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL || base->is_virtual ||
        base->access != kAccessPublic) {
      continue;
    }
    int64_t offset = base_offset + base->byte_offset;
    EHTypeInfoBase* entry = malloc(sizeof(EHTypeInfoBase));
    StringInit(&entry->base_name, "");
    CXXExceptionTypeName(base->type, &entry->base_name);
    entry->offset = offset;
    VectorAppend(out, entry);
    CollectExceptionBaseTypes(base->type, offset, out);
  }
}

EHTypeInfo* GeneratorGetExceptionTypeInfo(Generator* gen, TypeRecord* type) {
  TypeRecord* rtti_type =
      type != NULL && TypeIsReference(type) ? type->next : type;
  Symbol* canonical_typeinfo = NULL;
  // LSDA type tables name canonical Itanium type_info objects, never the
  // legacy DaveCC matching records.
  if (RttiUsesItaniumABI() && rtti_type != NULL) {
    canonical_typeinfo = RttiGetTypeInfoSymbol(rtti_type);
    CompilerMarkVariableReferenced(canonical_typeinfo);
  }
  String type_name = {0};
  CXXExceptionTypeName(type, &type_name);
  for (size_t i = 0; i < gen->exception_typeinfos.length; i++) {
    EHTypeInfo* info = gen->exception_typeinfos.value.p[i];
    if (StringEqualString(&info->type_name, &type_name)) {
      StringDestruct(&type_name);
      return info;
    }
  }

  EHTypeInfo* info = malloc(sizeof(EHTypeInfo));
  info->canonical_typeinfo = canonical_typeinfo;
  info->lsda_type_filter = gen->exception_typeinfos.length + 1;
  StringInit(&info->type_name, type_name.value);
  StringDestruct(&type_name);
  if (type != NULL) {
    TypeRecordCalculateSize(type);
  }
  info->object_size = type != NULL ? type->size : 0;
  info->object_is_class = type != NULL && TypeIsStructOrUnion(type);
  VectorInit(&info->bases);
  CollectExceptionBaseTypes(type, 0, &info->bases);
  StringInit(&info->symbol_name, "__davecc_typeinfo_");
  if (gen->func != NULL && gen->func->info.function.symbol != NULL) {
    // Use the mangled assembler name (not the source name) so that distinct
    // instantiations of a same-named function template (e.g. std::get<0> for
    // different variant types) get distinct, non-colliding typeinfo labels.
    Symbol* func_symbol = gen->func->info.function.symbol;
    String* func_key = func_symbol->asm_name.length > 0
                           ? &func_symbol->asm_name
                           : &func_symbol->name;
    StringAppendString(&info->symbol_name, func_key);
    StringAppendChar(&info->symbol_name, '_');
  }
  StringPrintf(&info->symbol_name, "%zu_", gen->exception_typeinfos.length);
  StringAppendString(&info->symbol_name, &info->type_name);
  SanitizeTypeInfoSymbolName(&info->symbol_name);
  VectorAppend(&gen->exception_typeinfos, info);
  return info;
}

static void TrapFunctionBeforeCodegen(Generator* gen) {
  if (StringEqual(&gen->func->info.function.symbol->name, "InitializeInstructions")) {
    Trap();
  }
}

static void TrapFunctionAfterCodegen(Generator* gen) {
  if (StringEqual(&gen->func->info.function.symbol->name, "foobar")) {
    Trap();
  }
}

void GeneratorInit(Generator* gen, Syntax* syntax, TypeRecord* func) {
  gen->syntax = syntax;
  gen->func = func;
  TypeRecordIncRef(func);
  ListInit(&gen->code);
  gen->last_constant = NULL;
  gen->last_variable = NULL;
  gen->break_label = NULL;
  gen->continue_label = NULL;
  gen->struct_return_value = NULL;
  gen->current_struct_address = NULL;
  gen->inlined_constructor_this = NULL;
  gen->return_label = NULL;
  VectorInit(&gen->int_constant_pool);
  VectorInit(&gen->fp_constant_pool);
  VectorInit(&gen->variable_pool);
  VectorInit(&gen->exception_ranges);
  VectorInit(&gen->exception_keep_labels);
  VectorInit(&gen->exception_typeinfos);
  VectorInit(&gen->cleanup_pads);
  VectorInit(&gen->basic_blocks);
  VectorInit(&gen->loops);
  gen->for_constant_evaluation = false;
  gen->source_pointer_size =
      compiler->target != NULL ? compiler->target->pointer_size : SizeofPointer();

  IRResetNodeId();
}

static void PrintIR(ListElement* hdr, void* fp) {
  IRNode* node = (IRNode*)hdr;
  IRPrint(node, fp);
}

void GeneratorPrintIR(Generator* gen, FILE* fp) {
  for (int i = 0; i < 80; i++) {
    fprintf(fp, "-");
  }
  fprintf(fp, "\n");
  fprintf(fp, "**** IR for function %s\n\n", gen->func->info.function.symbol->name.value);
  ListTraverse(&gen->code, PrintIR, fp);
}

static void DestructIRNode(ListElement* hdr, void* data) {
  IRNode* node = (IRNode*)hdr;
  IRDestruct(node);
}

void GeneratorDestruct(Generator* gen) {
  TypeRecordDelete(gen->func);

  // Delete all IR nodes.
  ListTraverse(&gen->code, DestructIRNode, NULL);
  ListDestruct(&gen->code);

  // The pools own the PoolEntry wrappers (the pooled IR nodes themselves live
  // in gen->code and were destructed above), so free the entries here.
  VectorDestructWithContents(&gen->int_constant_pool, NULL, /*free_element=*/true);
  VectorDestructWithContents(&gen->fp_constant_pool, NULL, /*free_element=*/true);
  VectorDestructWithContents(&gen->variable_pool, NULL, /*free_element=*/true);
  VectorDestructWithContents(&gen->exception_ranges, NULL, /*free_element=*/true);
  VectorDestruct(&gen->exception_keep_labels);
  FreeCleanupPads(gen);
  // Target generators keep pointers to these records until final assembly
  // emission, which can happen after the transient IR generator is destroyed.
  VectorDestruct(&gen->exception_typeinfos);

  // Delete the basic blocks.
  LoopInfoClear(gen);
  VectorDestruct(&gen->loops);
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    BasicBlockDelete(block);
  }
  VectorDestruct(&gen->basic_blocks);
}

void GeneratorStealCode(Generator* gen, List* dst) {
  for (IRNode* node = GeneratorFirstInstruction(gen); node != NULL;
       node = IRNext(node)) {
    node->block = NULL;
  }
  *dst = gen->code;
  ListInit(&gen->code);
}

void GeneratorAdoptCode(Generator* gen, List* src) {
  ListTraverse(&gen->code, DestructIRNode, NULL);
  ListDestruct(&gen->code);
  gen->code = *src;
  ListInit(src);
}

void GeneratorRebuildPools(Generator* gen) {
  VectorDestructWithContents(&gen->variable_pool, NULL, /*free_element=*/true);
  VectorInit(&gen->variable_pool);
  VectorDestructWithContents(&gen->int_constant_pool, NULL,
                             /*free_element=*/true);
  VectorInit(&gen->int_constant_pool);
  VectorDestructWithContents(&gen->fp_constant_pool, NULL,
                             /*free_element=*/true);
  VectorInit(&gen->fp_constant_pool);
  for (IRNode* node = GeneratorFirstInstruction(gen); node != NULL;
       node = IRNext(node)) {
    if (IRIsVariable(node) && node->opcode != IR_OP(tmp) &&
        node->opcode != IR_OP(structreturn)) {
      IRVariable* var = (IRVariable*)node;
      PoolEntry* entry = malloc(sizeof(PoolEntry));
      entry->value.symbol = var->symbol;
      entry->type = var->symbol != NULL && var->symbol->type != NULL
                        ? var->symbol->type->type
                        : 0;
      entry->pooled = node;
      VectorAppend(&gen->variable_pool, entry);
    } else if (IRIsIntConst(node)) {
      PoolEntry* entry = malloc(sizeof(PoolEntry));
      entry->value.ivalue = IRIntConstValue(node);
      entry->type = node->type != NULL ? node->type->type : 0;
      entry->pooled = node;
      VectorAppend(&gen->int_constant_pool, entry);
    } else if (IRIsConst(node) && !IRIsIntConst(node)) {
      PoolEntry* entry = malloc(sizeof(PoolEntry));
      entry->value.fvalue = ((IRConstant*)node)->value.fvalue;
      entry->type = node->type != NULL ? node->type->type : 0;
      entry->pooled = node;
      VectorAppend(&gen->fp_constant_pool, entry);
    }
  }
}

// Check if the node is using (reading) a variable.  If so,
// mark the write instruction with the VarUse flag.  This is necessary
// so that the SSA conversions know that this is a read from a variable.
void CheckForVarUse(IRNode* read, ASTNode* node) {
  switch (node->op) {
    case AST_OP(identifier): {
      IdentifierASTNode* id = (IdentifierASTNode*)node;
      if (!TypeIsVLA(id->symbol->type) &&
          !SymbolNeedsDynamicStackAllocation(id->symbol)) {
        IRSetVarUse(read, id->symbol);
      }
      break;
    }
    case AST_OP(dot):
    case AST_OP(arrow): {
      BinaryASTNode* b = (BinaryASTNode*)node;
      if (b->right != NULL && b->right->op == AST_OP(structmember)) {
        StructMemberASTNode* member = (StructMemberASTNode*)b->right;
        if (member->member->is_static) {
          IRSetVarUse(read, member->member->symbol);
          break;
        }
      }
      CheckForVarUse(read, b->left);
      break;
    }
    case AST_OP(subscript): {
      BinaryASTNode* b = (BinaryASTNode*)node;
      CheckForVarUse(read, b->left);
      break;
    }

    default:
      break;
  }
}

// Check if the node is defining (writing to) a variable.  If so,
// mark the write instruction with the VarDef flag.  This is necessary
// so that the SSA conversions know that this is a write to a variable.
void CheckForVarDef(IRNode* write, ASTNode* node) {
  switch (node->op) {
    case AST_OP(identifier): {
      IdentifierASTNode* id = (IdentifierASTNode*)node;
      if (!TypeIsVLA(id->symbol->type) &&
          !SymbolNeedsDynamicStackAllocation(id->symbol)) {
        IRSetVarDef(write, id->symbol);
      }
      break;
    }
    case AST_OP(init):
    case AST_OP(subscript):{
      BinaryASTNode* b = (BinaryASTNode*)node;
      CheckForVarDef(write, b->left);
      break;
    }
    case AST_OP(dot): {
      BinaryASTNode* b = (BinaryASTNode*)node;
      if (b->right != NULL && b->right->op == AST_OP(structmember)) {
        StructMemberASTNode* member = (StructMemberASTNode*)b->right;
        if (member->member->is_static) {
          IRSetVarDef(write, member->member->symbol);
          break;
        }
      }
      CheckForVarDef(write, b->left);
      break;
    }
    case AST_OP(arrow): {
      BinaryASTNode* b = (BinaryASTNode*)node;
      if (b->right != NULL && b->right->op == AST_OP(structmember)) {
        StructMemberASTNode* member = (StructMemberASTNode*)b->right;
        if (member->member->is_static) {
          IRSetVarDef(write, member->member->symbol);
        }
      }
      // Writing through a pointer mutates the pointee, not the pointer
      // variable used to form the address.
      break;
    }
    case AST_OP(address): {
      UnaryASTNode* n = (UnaryASTNode*)node;
      CheckForVarDef(write, n->sub);
      break;
    }
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_arg):
    case AST_OP(builtin_va_copy): {
      VectorASTNode* v = (VectorASTNode*)node;
      CheckForVarDef(write, v->children->value.p[0]);
      break;
    }
    case AST_OP(builtin_atomic_store):
    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
    case AST_OP(builtin_atomic_compare_exchange_bool):
    case AST_OP(builtin_atomic_compare_exchange_val):
    case AST_OP(builtin_atomic_compare_exchange_n): {
      VectorASTNode* v = (VectorASTNode*)node;
      CheckForVarDef(write, v->children->value.p[0]);
      break;
    }
    case AST_OP(return): {
      CombinedStatementASTNode* r = (CombinedStatementASTNode*)node;
      CheckForVarDef(write, r->cond);
    }
      
    default:
      break;
  }
}


// We've been asked for the label for the return.  If there
// isn't one, generate it now and store it.
IRNode* GeneratorGetReturnLabel(Generator* gen) {
  if (gen->return_label == NULL) {
    gen->return_label = GeneratorEmit(gen, NewIR(IR_OP(label)));
    GeneratorEmit(gen, NewIR(IR_OP(leave)));
    GeneratorEmit(gen, NewIR(IR_OP(ret)));
  }
  return gen->return_label;
}

IRNode* GeneratorFirstInstruction(Generator* gen) {
  return (IRNode*)gen->code.first;
}

IRNode* GeneratorLastInstruction(Generator* gen) {
  return (IRNode*)gen->code.last;
}

void GeneratorError(Generator* gen, ASTNode* node, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VGeneratorError(gen, node, format, ap);
  va_end(ap);
}

void VGeneratorError(Generator* gen, ASTNode* node, const char* format,
                     va_list ap) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  VReportError(filename, lineno, format, ap);
}

void GeneratorWarning(Generator* gen, ASTNode* node, const char* warn,
                      const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VGeneratorWarning(gen, node, warn, format, ap);
  va_end(ap);
}

void VGeneratorWarning(Generator* gen, ASTNode* node, const char* warn,
                       const char* format, va_list ap) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  VReportWarning(filename, lineno, warn, format, ap);
}

IRNode* GeneratorEmit(Generator* gen, IRNode* inst) {
  TrapInstruction(inst);
  if (IRInList(inst)) {
    return inst;
  }
  ListAppend(&gen->code, &inst->header);
  return inst;
}

IRNode* GeneratorEmitBefore(Generator* gen, IRNode* inst, IRNode* pos) {
  TrapInstruction(inst);
  if (IRInList(inst)) {
    return inst;
  }
  ListInsertBefore(&gen->code, &inst->header, &pos->header);
  return inst;
}

IRNode* GeneratorEmitAfter(Generator* gen, IRNode* inst, IRNode* pos) {
  TrapInstruction(inst);
  if (pos == NULL) {
    // No instruction to emit after means emit at end of list.
    return GeneratorEmit(gen, inst);
  }
  if (IRInList(inst)) {
    return inst;
  }
  ListInsertAfter(&gen->code, &inst->header, &pos->header);
  return inst;
}

// Constants are at the start of the code.
IRNode* GeneratorEmitConstant(Generator* gen, IRNode* inst) {
  if (IRInList(inst)) {
    return inst;
  }
  if (gen->last_constant == NULL) {
    gen->last_constant =
        GeneratorEmitBefore(gen, inst, (IRNode*)gen->code.first);
  } else {
    gen->last_constant = GeneratorEmitAfter(gen, inst, gen->last_constant);
  }
  return inst;
}

// Variables follow constants.
IRNode* GeneratorEmitVariable(Generator* gen, IRNode* inst) {
  if (IRInList(inst)) {
    return inst;
  }
  if (gen->last_variable == NULL) {
    gen->last_variable = GeneratorEmitBefore(gen, inst, (IRNode*)gen->code.first);
  } else {
    gen->last_variable = GeneratorEmitAfter(gen, inst, gen->last_variable);
  }
  return inst;
}

void GeneratorRemoveInstruction(Generator* gen, IRNode* inst) {
  ListDeleteElement(&gen->code, &inst->header);
  IRRemoveNode(inst);
  // IRRemoveNode only unlinks the def-use edges; release the node's own
  // inputs/outputs vectors and type reference before freeing the struct.
  IRDestruct(inst);
  free(inst);
}

void GeneratorMoveInstructionBefore(Generator* gen, IRNode* inst, IRNode* pos) {
  ListDeleteElement(&gen->code, &inst->header);
  ListInsertBefore(&gen->code, &inst->header, &pos->header);
}

void GeneratorMoveInstructionAfter(Generator* gen, IRNode* inst, IRNode* pos) {
  ListDeleteElement(&gen->code, &inst->header);
  ListInsertAfter(&gen->code, &inst->header, &pos->header);
}


void GeneratorReplaceInstruction(Generator* gen, IRNode* old, IRNode* new) {
  // Replace references to the old node with those to the new one.
  for (size_t i = 0; i < old->outputs.length; i++) {
    IRNode* ref = old->outputs.value.p[i];
    for (size_t j = 0; j < ref->inputs.length; j++) {
      IRNode* input = ref->inputs.value.p[j];
      if (input == old) {
        ref->inputs.value.p[j] = new;
        VectorAppend(&new->outputs, ref);
      }
    }
  }
  VectorClear(&old->outputs);
}

IRNode* GeneratorGetIntConstant(Generator* gen, TypeRecord* type,
                                int64_t value) {
  PoolEntry* entry;
  for (size_t i = 0; i < gen->int_constant_pool.length; i++) {
    entry = gen->int_constant_pool.value.p[i];
    if (entry->value.ivalue == value &&
        TypeEqual(entry->pooled->type, type)) {
      return entry->pooled;
    }
  }
  if (value == 42) {
    printf("");
  }
  // No constant found, add a new one.
  entry = malloc(sizeof(PoolEntry));
  entry->value.ivalue = value;
  if (type != NULL) {
    entry->type = type->type;
  } else {
    entry->type = kTypeInt;
  }
  if (type == NULL) {
    type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }

  entry->pooled = GeneratorEmitConstant(gen, NewIntIRConstant(type, value));
  VectorAppend(&gen->int_constant_pool, entry);
  return entry->pooled;
}

IRNode* GeneratorGetFloatingPointConstant(Generator* gen, TypeRecord* type,
                                          double value) {
  PoolEntry* entry;
  Type type_spec = kTypeDouble;
  if (type != NULL) {
    type_spec = type->type;
  }
  for (size_t i = 0; i < gen->fp_constant_pool.length; i++) {
    entry = gen->fp_constant_pool.value.p[i];
    if (entry->value.fvalue == value && entry->type == type_spec) {
      return entry->pooled;
    }
  }

  // No constant found, add a new one.
  entry = malloc(sizeof(PoolEntry));
  entry->value.fvalue = value;
  if (type != NULL) {
    entry->type = type->type;
  } else {
    entry->type = kTypeDouble;
  }
  entry->pooled =
      GeneratorEmitConstant(gen, NewFloatingPointIRConstant(type, value));
  VectorAppend(&gen->int_constant_pool, entry);
  return entry->pooled;
}

int PoolEntryStackAlignment(PoolEntry* entry) {
  if (entry->value.symbol != NULL) {
    return SymbolStackAlignment(entry->value.symbol);
  }
  int alignment = TypeRecordAlignment(entry->pooled->type);
  return alignment > 0 ? alignment : 1;
}

int SymbolStackAlignment(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL) {
    return 1;
  }
  int alignment = TypeRecordAlignment(symbol->type);
  if (symbol->alignment > alignment) {
    alignment = symbol->alignment;
  }
  return alignment > 0 ? alignment : 1;
}

bool SymbolNeedsDynamicStackAllocation(Symbol* symbol) {
  return symbol != NULL && symbol->type != NULL &&
         symbol->flags.is_local && !symbol->flags.is_argument &&
         !TypeIsVLA(symbol->type) &&
         !StorageIs(symbol->storage, STO(static) | STO(extern) | STO(thread)) &&
         compiler->target != NULL &&
         SymbolStackAlignment(symbol) > compiler->target->stack_alignment;
}

IRNode* GeneratorGetVariable(Generator* gen, Symbol* sym) {
  // Record symbols materialized by real target code so discardable C++ inline
  // functions and variables can remain parsed and checked without all being
  // emitted merely because their headers were included.
  if (!gen->for_constant_evaluation && sym != NULL && sym->type != NULL) {
    if (TypeIsFunction(sym->type)) {
      CompilerMarkFunctionReferenced(sym);
    } else {
      CompilerMarkVariableReferenced(sym);
    }
  }
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = gen->variable_pool.value.p[i];
    if (entry->value.symbol == sym) {
      return entry->pooled;
    }
  }

  // No variable found, add a new one.
  PoolEntry* entry = malloc(sizeof(PoolEntry));
  entry->value.symbol = sym;
  entry->type = sym->type->type;
  IRNode* variable = NewIRVariable(sym);
  if (sym->is_nrvo) {
    variable->flags |= kIRNrvoMarker;
  }
  entry->pooled = GeneratorEmitVariable(gen, variable);
  VectorAppend(&gen->variable_pool, entry);
  return entry->pooled;
}

//
// Basic block building.
//

static BasicBlock* GeneratorNewBasicBlock(Generator* gen) {
  BasicBlock* b = NewBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, b);
  return b;
}

static BasicBlock* FindBasicBlock(Generator* gen, BlockId id) {
  return VectorGet(&gen->basic_blocks, id);
}

static void PrintBasicBlocks(Generator* gen, FILE* fp) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    BasicBlockPrint(gen, b, gen->entry_block, gen->exit_block, fp);
  }
}

static void CreateBasicBlocks(Generator* gen, Vector* branches,
                              BitSet* exception_calls) {
  gen->entry_block = GeneratorNewBasicBlock(gen);
  BasicBlock* current = gen->entry_block;

  // First instruction is in first block.
  current->code = GeneratorFirstInstruction(gen);
  
  // Find the boundary IRNodes (labels, branches and returns).  Each
  // one of these either ends a block or starts a new one.
  for (IRNode* inst = current->code; inst != NULL; inst = IRNext(inst)) {
    // Check if this node defines (writes to) a variable.  If so
    // keep track of this in the basic block.
    if (IRIsVarDef(inst)) {
      MapKeyValue kv;
      kv.key.p = inst->var.def;
      kv.value.p = NULL;
      MapInsert(&current->defined_vars, kv);
    }
    if (IRIsVarRef(inst)) {
      SetInsert(&current->referenced_vars, inst->var.use);
    }
    
    // Call instruction?  Mark block as containing call.
    if (inst->opcode == IR_OP(calla) || inst->opcode == IR_OP(memcpy) ||
        inst->opcode == IR_OP(memzero)) {
      current->num_calls++;
    }

    if (inst->opcode == IR_OP(label)) {
      current->end_code = IRPrev(inst);
      
      // A label marks the start of a block.
      BasicBlock* b = GeneratorNewBasicBlock(gen);
      inst->block = b;
      b->code = inst;
      current = b;
    } else if (IRIsCall(inst) &&
               !BitSetContains(exception_calls, inst->id)) {
      inst->block = current;
    } else if (IRIsBranch(inst) || IRIsReturn(inst) || IRIsCall(inst)) {
      // A branch (and return) ends a block.
      current->return_block = IRIsReturn(inst);
      current->end_code = inst;
      inst->block = current;
      VectorAppend(branches, inst);
      
      // Allocate a new block starting at the next instruction provided it's
      // not a label (because that will be created in next iteration).
      IRNode* next = IRNext(inst);
      if (next != NULL && next->opcode != IR_OP(label)) {
        BasicBlock* b = GeneratorNewBasicBlock(gen);
        b->code = next;
        current = b;
      }
    } else {
      inst->block = current;
    }
  }
  current->end_code = GeneratorLastInstruction(gen);
  
  // Allocate exit block.
  gen->exit_block = GeneratorNewBasicBlock(gen);
}

// Process all branches and link their targets to the appropriate
// block.
static void BuildBasicBlockGraph(Generator* gen,
                                  Vector* branches) {
  for (size_t i = 0; i < branches->length; i++) {
    IRNode* inst = branches->value.p[i];
    BasicBlock* block = inst->block;
    if (IRIsConditionalBranch(inst)) {
      // Conditional branch links to both its taken and fallthrough blocks.
      IRNode* fallthrough = IRNext(inst);
      IRNode* taken = inst->inputs.value.p[1];
      BasicBlockAddEdge(block, fallthrough->block);
      BasicBlockAddEdge(block, taken->block);
    } else if (inst->opcode == IR_OP(cbra)) {
      // Table jump is followed by a branch table.  These are bra instructions.
      // Find all of them and link to this block.
      IRNode* bra = IRNext(inst);
      while (bra->opcode == IR_OP(bra)) {
        BasicBlockAddEdge(block, bra->block);
        bra = IRNext(bra);
      }
    } else if (IRIsReturn(inst)) {
      // Return always links to the exit block.
      BasicBlockAddEdge(block, gen->exit_block);
    } else if (IRIsCall(inst)) {
      // A call just links to next block.
      IRNode* fallthrough = IRNext(inst);
      BasicBlockAddEdge(block, fallthrough->block);
    } else {
      // Unconditional branch only links to its target.
      IRNode* target = inst->inputs.value.p[0];
      BasicBlockAddEdge(block, target->block);
    }
  }
}

static void LinkToLandingPadOnce(BasicBlock* from, BasicBlock* pad) {
  if (from == NULL || pad == NULL || from == pad) {
    return;
  }
  // Several ranges can share one pad (a try with more than one handler, or a
  // handler naming several types), and a duplicate edge would be counted twice
  // when phis line their operands up with predecessors.
  for (size_t i = 0; i < from->out_edges.length; i++) {
    if (from->out_edges.value.w[i] == (int64_t)pad->block_id) {
      return;
    }
  }
  BasicBlockAddEdge(from, pad);
}

// Link each protected region to its landing pad.  Without these edges the pad
// has no predecessor, so it stays out of the dominator tree and the values it
// defines are invisible to SSA construction and to every dataflow pass: a local
// assigned in a catch block reads back as its pre-try value.
//
// Every call in the region gets its own edge, because a call is where control
// leaves for the pad and the values reaching the pad are the ones live at that
// call.  Linking only the region's start label instead would tell SSA the pad
// sees exactly the state before the region ran, and a cleanup that reads a
// counter the region maintains -- how many elements of an array are constructed
// so far, say -- would read the counter's initial value.  Constant propagation
// then folds the cleanup away and the constructed elements are never destroyed.
//
// The start label is linked as well.  If it is not itself a call block the value
// its edge carries was already defined inside the region, so the pad's phi ends
// up with an operand no throw can actually deliver.  That only costs precision:
// a phi with more operands cannot fold to a constant that a narrower one would
// not, whereas a missing operand is a wrong answer.
static void AddExceptionHandlerEdges(Generator* gen) {
  for (size_t i = 0; i < gen->exception_ranges.length; i++) {
    ExceptionHandlerRange* range = gen->exception_ranges.value.p[i];
    if (range->try_start == NULL || range->catch_label == NULL) {
      continue;
    }
    BasicBlock* pad = range->catch_label->block;
    if (pad == NULL) {
      continue;
    }
    LinkToLandingPadOnce(range->try_start->block, pad);
    // Find the end of the region before linking anything inside it.  Walking
    // straight to |try_end| would run to the end of the function if the label is
    // missing or sits before the start, and the pad would gain a predecessor
    // that follows it -- a back edge, which every loop pass would then believe
    // in.
    bool bounded = false;
    for (IRNode* inst = range->try_start; inst != NULL; inst = IRNext(inst)) {
      if (inst == range->try_end) {
        bounded = true;
        break;
      }
    }
    if (!bounded) {
      continue;
    }
    for (IRNode* inst = range->try_start; inst != range->try_end;
         inst = IRNext(inst)) {
      if (IRIsCall(inst)) {
        LinkToLandingPadOnce(inst->block, pad);
      }
    }
  }
}

// All blocks with no output edges link to exit block.  Also blocks
// that do not end in a branch or return fall through to next block.
static void AddMissingLinks(Generator* gen, BitSet* exception_calls) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    if (b == gen->exit_block) {
      continue;
    }
    bool call_ends_block =
        b->end_code != NULL && IRIsCall(b->end_code) &&
        BitSetContains(exception_calls, b->end_code->id);
    if (b->end_code == NULL ||
        (!IRIsBranch(b->end_code) && !IRIsReturn(b->end_code) &&
         !call_ends_block)) {
      // No branch or return, fall through to next block.
      BasicBlockAddEdge(b, FindBasicBlock(gen, b->block_id + 1));
    }
    if (b->out_edges.length == 0) {
      BasicBlockAddEdge(b, gen->exit_block);
    }
  }
}

static void FindExceptionCallBoundaries(Generator* gen,
                                        BitSet* exception_calls) {
  for (size_t i = 0; i < gen->exception_ranges.length; i++) {
    ExceptionHandlerRange* range = gen->exception_ranges.value.p[i];
    if (range->try_start == NULL || range->try_end == NULL) {
      continue;
    }
    bool bounded = false;
    for (IRNode* inst = range->try_start; inst != NULL; inst = IRNext(inst)) {
      if (inst == range->try_end) {
        bounded = true;
        break;
      }
    }
    if (!bounded) {
      continue;
    }
    for (IRNode* inst = range->try_start; inst != range->try_end;
         inst = IRNext(inst)) {
      if (IRIsCall(inst)) {
        BitSetInsert(exception_calls, inst->id);
      }
    }
  }
}

static bool HasForwardSinglePredecessorCFG(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    if (block == gen->entry_block || block->in_edges.length == 0) {
      continue;
    }
    if (block->in_edges.length != 1 ||
        block->in_edges.value.w[0] >= block->block_id) {
      return false;
    }
  }
  return true;
}

static void CalculateForwardSinglePredecessorDominators(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    BitSetClear(&block->dominators);
    block->idom = NULL;
    block->num_dominators = 0;
    block->reachability_known = true;
    block->is_unreachable = block != gen->entry_block;
    if (block != gen->entry_block && block->in_edges.length == 1) {
      BasicBlock* predecessor =
          VectorGet(&gen->basic_blocks, block->in_edges.value.w[0]);
      if (!predecessor->is_unreachable) {
        BitSetCopy(&block->dominators, &predecessor->dominators);
        block->idom = predecessor;
        block->is_unreachable = false;
      }
    }
    if (!block->is_unreachable) {
      BitSetInsert(&block->dominators, block->block_id);
      block->num_dominators = BitSetCount(&block->dominators);
    }
  }
}

static void ComputeReversePostorder(Generator* gen, Vector* postorder,
                                    BitSet* reachable) {
  Vector work;
  VectorInit(&work);
  VectorAppend(&work, (void*)(gen->entry_block->block_id << 1));
  while (work.length > 0) {
    uintptr_t item = (uintptr_t)VectorLast(&work);
    VectorPop(&work);
    BlockId id = (BlockId)(item >> 1);
    bool expanded = (item & 1) != 0;
    if (expanded) {
      VectorAppend(postorder, (void*)id);
      continue;
    }
    if (BitSetContains(reachable, id)) {
      continue;
    }
    BitSetInsert(reachable, id);
    VectorAppend(&work, (void*)((id << 1) | 1));
    BasicBlock* block = VectorGet(&gen->basic_blocks, id);
    for (size_t i = block->out_edges.length; i > 0; i--) {
      BlockId successor = block->out_edges.value.w[i - 1];
      if (!BitSetContains(reachable, successor)) {
        VectorAppend(&work, (void*)(successor << 1));
      }
    }
  }
  VectorDestruct(&work);
}

static BasicBlock* IntersectImmediateDominators(BasicBlock* left,
                                                BasicBlock* right,
                                                BasicBlock** idoms,
                                                const size_t* rpo_number) {
  while (left != right) {
    while (rpo_number[left->block_id] > rpo_number[right->block_id]) {
      left = idoms[left->block_id];
    }
    while (rpo_number[right->block_id] > rpo_number[left->block_id]) {
      right = idoms[right->block_id];
    }
  }
  return left;
}

// Cooper-Harvey-Kennedy immediate dominators over reverse postorder.  The old
// implementation repeatedly intersected full per-block bitsets to a fixed
// point and then scanned each resulting set to recover the immediate
// dominator.  Compute idoms directly, then materialize the full sets once for
// the few clients that still query them.
static void CalculateDominators(Generator* gen) {
  if (HasForwardSinglePredecessorCFG(gen)) {
    CalculateForwardSinglePredecessorDominators(gen);
    return;
  }

  size_t num_blocks = gen->basic_blocks.length;
  Vector postorder;
  VectorInit(&postorder);
  BitSet reachable;
  BitSetInit(&reachable);
  ComputeReversePostorder(gen, &postorder, &reachable);

  size_t* rpo_number = malloc(num_blocks * sizeof(*rpo_number));
  BasicBlock** idoms = calloc(num_blocks, sizeof(*idoms));
  for (size_t i = 0; i < num_blocks; i++) {
    rpo_number[i] = SIZE_MAX;
  }
  for (size_t i = 0; i < postorder.length; i++) {
    BlockId id = postorder.value.w[postorder.length - i - 1];
    rpo_number[id] = i;
  }

  BlockId entry_id = gen->entry_block->block_id;
  idoms[entry_id] = gen->entry_block;
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t i = 1; i < postorder.length; i++) {
      BlockId id = postorder.value.w[postorder.length - i - 1];
      BasicBlock* block = VectorGet(&gen->basic_blocks, id);
      BasicBlock* new_idom = NULL;
      for (size_t j = 0; j < block->in_edges.length; j++) {
        BlockId predecessor_id = block->in_edges.value.w[j];
        if (idoms[predecessor_id] == NULL) {
          continue;
        }
        BasicBlock* predecessor =
            VectorGet(&gen->basic_blocks, predecessor_id);
        new_idom =
            new_idom == NULL
                ? predecessor
                : IntersectImmediateDominators(new_idom, predecessor, idoms,
                                               rpo_number);
      }
      if (new_idom != NULL && idoms[id] != new_idom) {
        idoms[id] = new_idom;
        changed = true;
      }
    }
  }

  for (size_t i = 0; i < num_blocks; i++) {
    BasicBlock* block = VectorGet(&gen->basic_blocks, i);
    BitSetClear(&block->dominators);
    block->idom = NULL;
    block->num_dominators = 0;
    block->reachability_known = true;
    block->is_unreachable = !BitSetContains(&reachable, i);
    if (block->is_unreachable) {
      continue;
    }
    block->idom = i == entry_id ? NULL : idoms[i];
    for (BasicBlock* dominator = block; dominator != NULL;
         dominator = dominator->block_id == entry_id
                         ? NULL
                         : idoms[dominator->block_id]) {
      BitSetInsert(&block->dominators, dominator->block_id);
      block->num_dominators++;
    }
  }

  free(idoms);
  free(rpo_number);
  BitSetDestruct(&reachable);
  VectorDestruct(&postorder);
}

static void CalculateDominanceFrontier(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    BasicBlockCalculateDominanceFrontier(gen, b, &gen->basic_blocks);
  }
}

// Build dominator tree.  If a block has an
// immediate dominator (idom) add the block to the idom's
// dominatees set.
static void BuildDominatorTree(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    if (b->idom != NULL) {
      VectorAppend(&b->idom->dominatees, (void*)b->block_id);
    }
  }
}

static void ResetCFGAnalysis(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    BitSetClear(&block->dominators);
    block->num_dominators = 0;
    VectorClear(&block->dominatees);
    BitSetClear(&block->dominance_frontier);
    block->idom = NULL;
    block->reachability_known = false;
    block->is_unreachable = false;
  }
}

static void AnalyzeCFG(Generator* gen) {
  ResetCFGAnalysis(gen);
  CalculateDominators(gen);
  CalculateDominanceFrontier(gen);
  BuildDominatorTree(gen);
  LoopInfoBuild(gen);
}

static void CoalesceBlocks(Generator* gen, BasicBlock* dest, BasicBlock* src) {
  // Move all instructions from the source block to the destination, placing
  // them before the last unconditional branch.
  IRNode* next = NULL;
  IRNode* last = src->code;
  for (IRNode* inst = BasicBlockBegin(src); !BasicBlockIsEmpty(src) && inst != BasicBlockEnd(src); inst = next) {
    next = IRNext(inst);
    last = inst;
    BasicBlockMoveInstructionBefore(gen, inst, dest->end_code);
  }
  
  // Copy all defined variables from src to dest.
  MapCopy(&dest->defined_vars, &src->defined_vars);
  
  // Copy all referenced vars too.
  SetCopy(&dest->referenced_vars, &src->referenced_vars);
  
  // Clear defined and referenced vars in src.
  MapClear(&src->defined_vars);
  SetClear(&src->referenced_vars);
  
  dest->num_calls += src->num_calls;
  
  if (src->return_block) {
    dest->return_block = true;
  }
  // The source block is now empty.
  if (IRIsConditionalBranch(last)) {
    // If it ended in a conditional
    // branch we need to add an unconditional branch to its fallthrough
    // output.  The fallthrough block is the one that is not used in
    // the conditional branch.
    assert(src->out_edges.length == 2);
    IRNode* taken = last->inputs.value.p[1];
    BasicBlock* fallthrough = VectorGet(&gen->basic_blocks, src->out_edges.value.w[0]);
    if (taken->block == fallthrough) {
      fallthrough = VectorGet(&gen->basic_blocks, src->out_edges.value.w[1]);
    }
    // If the fallthrough block doesn't begin with a label, add a label
    // for it.
    if (fallthrough != gen->exit_block &&
        fallthrough->code->opcode != IR_OP(label)) {
      IRNode* label = GeneratorEmitBefore(gen, NewIR(IR_OP(label)), fallthrough->code);
      label->block = fallthrough;
      fallthrough->code = label;
    }
    if (fallthrough != gen->exit_block) {
      IRNode* bra = GeneratorEmitBefore(
          gen, NewIR1(IR_OP(bra), fallthrough->code), dest->end_code);
      bra->block = dest;
    }
  } else if (!IRIsUnconditionalBranch(last) && !IRIsReturn(last)) {
    // Block doesn't end in a branch or return. Add a single unconditional
    // branch to its single output edge.
    assert(src->out_edges.length == 1);
    BasicBlock* output = VectorGet(&gen->basic_blocks, src->out_edges.value.w[0]);
    // The branch target must be a label so it can be resolved during target
    // lowering.  A block reached only by fall-through (e.g. the code after a
    // call, or after an empty-condition `for(;;)` loop) need not begin with a
    // label; synthesize one, matching the conditional-branch case above, or the
    // emitted `bra` would point at a non-label node and never get a fixup,
    // producing an unconditional jump with a NULL target that crashes codegen.
    if (output != gen->exit_block && output->code->opcode != IR_OP(label)) {
      IRNode* label = GeneratorEmitBefore(gen, NewIR(IR_OP(label)), output->code);
      label->block = output;
      output->code = label;
    }
    if (output != gen->exit_block) {
      IRNode* bra = GeneratorEmitBefore(
          gen, NewIR1(IR_OP(bra), output->code), dest->end_code);
      bra->block = dest;
    }
  }
  
  // Remove branch at end of dest block.
  BasicBlockRemoveInstruction(gen, dest, dest->end_code);
  
  // Remove link between dest and src.
  BasicBlockRemoveEdge(dest, src);
  
  // Move all out edges from src to dest.
  Vector out_edges = {0};
  VectorCopy(&out_edges, &src->out_edges);
  for (size_t i = 0; i < out_edges.length; i++) {
    BasicBlock* output = VectorGet(&gen->basic_blocks, out_edges.value.w[i]);
    BasicBlockRemoveEdge(src, output);
    BasicBlockAddEdge(dest, output);
  }
  VectorDestruct(&out_edges);
}

// Look for blocks with one input edgeand that input block ends
// in an unconditional branch.  We can merge these block together,
// remove the branch and possibly add a branch to the fall-through
// block if the block being moved ends in a conditional branch.
//
// This is just a linear traversal of the basic blocks.
static void StraightenGraph(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    if (block->in_edges.length == 1) {
      // Found a possible movable block.
      BasicBlock* input = VectorGet(&gen->basic_blocks, block->in_edges.value.w[0]);
      // A single-block loop is its own sole predecessor and successor. It is
      // not a coalescing candidate: moving a block's instructions into itself
      // continually reinserts each instruction before its own terminator.
      if (input != block && input->out_edges.length == 1) {
        assert(block == VectorGet(&gen->basic_blocks, input->out_edges.value.w[0]));
        IRNode* terminator = input->end_code;
        // Unconditional branch, not in a jump table.
        // Also check that the block is not just a label.
        bool is_candidate = terminator != NULL &&
              IRIsUnconditionalBranch(terminator) &&
            (terminator->flags & kIRJumpTableBranch) == 0;
        if (is_candidate) {
          // Don't merge a block that ends in a label or a result.
          is_candidate = block != gen->exit_block &&
            block->end_code != NULL &&
            block->end_code->opcode != IR_OP(label) &&
            !IRIsResult(block->end_code);
        }
        
        if (is_candidate) {
          CoalesceBlocks(gen, input, block);
        }
      }
    }
  }
  
}

// We have all the instructions available.  Divide them into basic blocks where
// a basic block is a sequence of instructions with no branches (flow must hit
// every instruction in the block if the block is entered).
//
// Block boundaries are marked by labels and branches.
void BuildBasicBlocks(Generator* gen) {
  // The branches vector holds a list of all the branches we encounter in the
  // code.  Each branch adds an edge from its block to the block starting with
  // the label to which it is branching.
  Vector branches;
  VectorInit(&branches);
  BitSet exception_calls;
  BitSetInit(&exception_calls);
  FindExceptionCallBoundaries(gen, &exception_calls);

  // Create all basic blocks.
  CreateBasicBlocks(gen, &branches, &exception_calls);
  
  // Link the blocks into a graph.
  BuildBasicBlockGraph(gen, &branches);

  // All blocks with no output edges link to exit block.  Also blocks
  // that do not end in a branch or return fall through to next block.
  AddMissingLinks(gen, &exception_calls);
    
  // PrintBasicBlocks(gen, stdout);
  
  // Straighten graph, coalescing blocks that just link to each other.
  StraightenGraph(gen);

  // Protected regions reach their landing pads without any branch.  This runs
  // after straightening because coalescing assumes a block that does not end in
  // a branch has exactly one successor, which an unwind edge would break.
  AddExceptionHandlerEdges(gen);

  // Build dominators and persistent natural-loop information.  Dedicated
  // preheaders are inserted before SSA conversion, so no phi repair is needed.
  // Recompute the analysis after each insertion because nested loops can make
  // one another's entry edges change.
  AnalyzeCFG(gen);
  if (OptLevel2() && compiler->ir_optimizations.code_motion &&
      compiler->ir_optimizations.loop_preheaders) {
    while (LoopInfoCreatePreheaders(gen)) {
      AnalyzeCFG(gen);
    }
  }

  // Tidy up.
  // Delete the branches vector.
  BitSetDestruct(&exception_calls);
  VectorDestruct(&branches);
}

// Remove all unreachable basic blocks.  These will never be executed.  The
// blocks aren't removed from the generator; only their instructions are
// cleared.  Exception landing labels are implicit CFG roots, so retain their
// forward-reachable blocks as well as every block named directly by exception
// metadata.
static void RemoveUnreachableBlocks(Generator* gen) {
  if (gen->exception_ranges.length == 0 &&
      gen->exception_keep_labels.length == 0) {
    for (size_t i = 0; i < gen->basic_blocks.length; i++) {
      BasicBlock* block = gen->basic_blocks.value.p[i];
      if (BasicBlockIsUnreachable(gen, block)) {
        BasicBlockClear(gen, block);
      }
    }
    return;
  }

  BitSet exception_reachable;
  BitSetInit(&exception_reachable);
  BitSet traversal_visited;
  BitSetInit(&traversal_visited);
  Vector work;
  VectorInit(&work);

  for (size_t i = 0; i < gen->exception_ranges.length; i++) {
    ExceptionHandlerRange* range = gen->exception_ranges.value.p[i];
    IRNode* metadata[] = {range->try_start, range->try_end,
                          range->catch_label};
    for (size_t j = 0; j < sizeof(metadata) / sizeof(metadata[0]); j++) {
      if (metadata[j] != NULL && metadata[j]->block != NULL) {
        BitSetInsert(&exception_reachable, metadata[j]->block->block_id);
      }
    }
    if (range->catch_label != NULL && range->catch_label->block != NULL) {
      VectorAppend(&work, range->catch_label->block);
    }
  }
  for (size_t i = 0; i < gen->exception_keep_labels.length; i++) {
    IRNode* label = gen->exception_keep_labels.value.p[i];
    if (label != NULL && label->block != NULL) {
      BitSetInsert(&exception_reachable, label->block->block_id);
      VectorAppend(&work, label->block);
    }
  }

  while (work.length > 0) {
    BasicBlock* block = VectorLast(&work);
    VectorPop(&work);
    if (BitSetContains(&traversal_visited, block->block_id)) {
      continue;
    }
    BitSetInsert(&traversal_visited, block->block_id);
    BitSetInsert(&exception_reachable, block->block_id);
    for (size_t i = 0; i < block->out_edges.length; i++) {
      BasicBlock* successor =
          VectorGet(&gen->basic_blocks, block->out_edges.value.w[i]);
      if (!BitSetContains(&traversal_visited, successor->block_id)) {
        VectorAppend(&work, successor);
      }
    }
  }

  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    if (BasicBlockIsUnreachable(gen, b) &&
        !BitSetContains(&exception_reachable, b->block_id)) {
      BasicBlockClear(gen, b);
    }
  }

  VectorDestruct(&work);
  BitSetDestruct(&traversal_visited);
  BitSetDestruct(&exception_reachable);
}

// Look in all the basic blocks for one that contains a call instruction.
// We have already marked these during basic block generation.
int GeneratorNumCalls(Generator* gen) {
  int calls = 0;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    calls += b->num_calls;
  }
  return calls;
}


// Checks for missing return.
typedef struct {
  BitSet visited;     // Map of visited block ids.
  bool result_known;  // We have reached a return or end of path.
  bool found_result;  // We have found an assignemnt to the result.
} ReturnVisitor;

static bool VisitBlockForResultInstruction(IRNode* inst, ReturnVisitor* v) {
  if (inst == NULL) {
    return false;
  }

  if (IRIsCall(inst)) {
    IRNode* callee = inst->inputs.value.p[0];
    if (IRIsVariable(callee)) {
      Symbol* callee_symbol = ((IRVariable*)callee)->symbol;
      if (callee_symbol != NULL &&
          (callee_symbol->flags.noreturn ||
           SymbolHasAttribute(callee_symbol, "noreturn"))) {
        v->found_result = true;
        v->result_known = true;
        return true;
      }
    }
  }

  // Scalar result return;
  if (IRIsResult(inst)) {
    v->found_result = true;
    v->result_known = true;
    return true;
  }
  
  // Struct result return.
  if (inst->opcode == IR_OP(memcpy)) {
    IRNode* dest = inst->inputs.value.p[0];
    if (dest->opcode == IR_OP(structreturn)) {
      v->found_result = true;
      v->result_known = true;
      return true;
    }
  }
  
  // RVO (Return Value Optimization) call.
  // NRVO (Named Return Value Optimization) branch.
  if ((inst->flags & (kIRRvoCall | kIRNrvoMarker)) != 0) {
    v->found_result = true;
    v->result_known = true;
    return true;
  }
  
  return false;
}

static void VisitBlockForResult(Generator* gen, BasicBlock* block, ReturnVisitor* v) {
  if (block == NULL) {
    return;
  }
  if (v->result_known) {
    return;
  }
  if (BitSetContains(&v->visited, block->block_id)) {
    return;
  }
  BitSetInsert(&v->visited, block->block_id);
  
  IRNode* last_inst = block->end_code;
  if (last_inst == NULL) {
    return;
  }
  if (VisitBlockForResultInstruction(last_inst, v)) {
    return;
  }
  // `return asm(...)` emits the asm immediately before the shared epilogue
  // label, so this block ends on the asm rather than on a result opcode.
  if (last_inst->opcode == IR_OP(asm)) {
    v->found_result = true;
    v->result_known = true;
    return;
  }
  // Look in the block for a result assignment.  A `ret` terminator is not
  // itself a result; it usually follows resulti/resulta/memcpy-to-structreturn
  // in the same block.  `return asm(...)` is a DaveCC extension that leaves
  // the value in the ABI return register, so an asm followed by a return jump
  // also counts.
  bool saw_asm = false;
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (VisitBlockForResultInstruction(inst, v)) {
      return;
    }
    if (inst->opcode == IR_OP(asm)) {
      saw_asm = true;
    }
    if (saw_asm && (inst->flags & kIRReturnJump) != 0) {
      v->found_result = true;
      v->result_known = true;
      return;
    }
  }

  if (IRIsReturn(last_inst) || block == gen->exit_block) {
    v->result_known = true;
    return;
  }
  for (size_t i = 0; i < block->out_edges.length; i++) {
    // Every block reached here is on a path that has not produced a result
    // yet.  Keep one visited set for the whole search so converging branches
    // and loops are traversed once rather than enumerating every simple path.
    v->result_known = false;
    v->found_result = false;
    BlockId child_id = block->out_edges.value.w[i];
    BasicBlock* child = VectorGet(&gen->basic_blocks, child_id);
    VisitBlockForResult(gen, child, v);
    if (v->result_known && !v->found_result) {
      break;
    }
  }
}

// Check that we can't reach the end block.
static void CheckReturn(Generator* gen) {
  // Don't check void functions.
  if (TypeIsVoidFunction(gen->func->info.function.symbol->type)) {
    return;
  }
  
  // main is special.
  if (StringEqual(&gen->func->info.function.symbol->name, "main")) {
    return;
  }

  // A function marked __attribute__((noreturn)) / _Noreturn is not expected to
  // return, so don't warn about reaching its end.
  Symbol* func_symbol = gen->func->info.function.symbol;
  if (func_symbol->flags.noreturn ||
      SymbolHasAttribute(func_symbol, "noreturn")) {
    return;
  }

  ReturnVisitor v = {{0}, false, false};
  VisitBlockForResult(gen, gen->basic_blocks.value.p[0], &v);
  if (v.result_known && !v.found_result) {
    // Falling off the end of a non-void function is undefined behaviour, but
    // like GCC/Clang we only warn rather than reject the program.
    SyntaxWarning(gen->syntax, "return-type",
                  "Control reaches the end of non-void function '%s'",
                  gen->func->info.function.symbol->name.value);
  }
  BitSetDestruct(&v.visited);
}


// Look for uninitialized variables.  This is really easy in SSA form
// as you just have to look for a localvar node with more than zero references.
// As all writes to variables result in an ssavar node being created, any
// references to the original variable are uninitialized by definition.
//
// All variables are in the entry block.
//
// NOTE: this can't check arrays.  Consider:
//  char buf[16];
//  strcpy(buf, "hello");     // REF but is actually write.
static void DetectUninitializedVars(Generator* gen) {
  BasicBlock* block = gen->entry_block;
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (inst->opcode == IR_OP(localvar)) {
      if (inst->outputs.length > 0) {
        IRVariable* var = (IRVariable*)inst;
        if (TypeIsArray(var->symbol->type) ||
            TypeIsStructOrUnion(var->symbol->type)) {
          // Only scalars.  No way to check for arrays, etc.
          continue;
        }
        for (size_t i = 0; i < inst->outputs.length; i++) {
          IRNode* ref = inst->outputs.value.p[i];
          if (IRIsVarDef(ref)) {
            continue;
          }
          const char* filename;
          int lineno;
          int start, end;
          DecodeSourceLocation(ref->location, &filename, &lineno, &start, &end);
          if (ref->value_state == kValueStateIndeterminate) {
            ReportWarning(
                filename, lineno, "uninitialized",
                "Variable '%s' is read with an indeterminate value here in "
                "function '%s'",
                var->symbol->name.value,
                gen->func->info.function.symbol->name.value);
          } else {
            ReportWarning(
                filename, lineno, "uninitialized",
                "Variable '%s' is used uninitialized here in function '%s'",
                var->symbol->name.value,
                gen->func->info.function.symbol->name.value);
          }
        }
      }
    }
  }
}

static void DetectInvalidValueReads(Generator* gen) {
  if (gen->for_constant_evaluation) {
    return;
  }
  for (size_t block_index = 0; block_index < gen->basic_blocks.length;
       block_index++) {
    BasicBlock* block = gen->basic_blocks.value.p[block_index];
    if (block == NULL || block->is_unreachable || BasicBlockIsEmpty(block)) {
      continue;
    }
    for (IRNode* inst = BasicBlockBegin(block); inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (!IRIsVarRef(inst) ||
          inst->value_state != kValueStateErroneous ||
          inst->var.use == NULL) {
        continue;
      }
      const char* filename;
      int lineno;
      int start;
      int end;
      DecodeSourceLocation(inst->location, &filename, &lineno, &start, &end);
      ReportWarning(
          filename, lineno, "uninitialized",
          "Variable '%s' is read with an erroneous value here in function '%s'",
          inst->var.use->name.value,
          gen->func->info.function.symbol->name.value);
    }
  }
}

// Binding a reference, or handing a variable's storage to anything that wants
// an address rather than the value in it, makes the variable's address escape.
// The front end only records that for the forms it can see as address-of, so
// look at the finished IR instead: every variable has exactly one node in the
// pool, and its users say how it was used.  Alias analysis, the constant
// propagators and the register-variable choice in each backend all read the
// symbol flag, so it has to be right before any of them run.
static void MarkVariablesWhoseAddressEscapes(Generator* gen) {
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = gen->variable_pool.value.p[i];
    Symbol* sym = entry->value.symbol;
    if (sym == NULL || sym->flags.address_taken) {
      continue;
    }
    if (IRVariableAddressEscapes(entry->pooled)) {
      sym->flags.address_taken = true;
    }
  }
}

// A VLA as an argument may be used in the funtion and the size of
// all dimensions will need to be known.  We generate a code sequence
// to calculate the size of each dimension and emit the code.  It needs
// to be done in the entry block to ensure that it's available for all
// basic blocks (all blocks are dominated by the entry block).  If the
// size expressions aren't used they will be eliminated later.
// The IRNode for the size is stored in the type record's codegen_info.
static void GenerateVLASizeExpressions(Generator* gen, Vector* prototype) {
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    if (TypeIsVLA(formal->type)) {
      GenerateVLASize(gen, formal->type);
    }
  }
}

static void ResetASTIRLabel(ASTNode* node, void* data, int child_id,
                            VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren) {
    return;
  }
  if (node->op == AST_OP(label)) {
    ((LabelASTNode*)node)->label = NULL;
  } else if (node->op == AST_OP(case)) {
    ((CaseLabelASTNode*)node)->label = NULL;
  }
}

bool TypeUsesNativeVectorABI(TypeRecord* type) {
  if (!TypeIsVector(type) || type->size <= 0 || type->size > 16) {
    return false;
  }
  if (StringEqual(compiler->target_name, "x86_64")) {
    return true;
  }
  // AAPCS64 uses the low 64 bits of v0-v7 for short vectors.  The AArch64
  // backend does not yet allocate full Q registers, so keep 128-bit vectors
  // on the target-independent indirect ABI until Q-register allocation lands.
  return StringEqual(compiler->target_name, "aarch64") && type->size <= 8;
}

bool TypeReturnedThroughHiddenPointer(TypeRecord* type) {
  return TypeIsStructOrUnion(type) ||
         (TypeIsVector(type) && !TypeUsesNativeVectorABI(type)) ||
         TypeIsMemberPointerAggregate(type);
}

static bool IsVectorIROpcode(IROpcode opcode) {
  return opcode >= IR_OP(vadd) && opcode <= IR_OP(vcmpgeu);
}

static TypeRecord* VectorObjectTypeFromAddress(IRNode* address) {
  TypeRecord* type = address != NULL ? address->type : NULL;
  if (TypeIsPointer(type) && TypeIsVector(type->next)) {
    return type->next;
  }
  return TypeIsVector(type) ? type : NULL;
}

static IROpcode VectorScalarOpcode(IROpcode vector_opcode,
                                   TypeRecord* element) {
  bool f32 = TypeUsesFloat32Representation(element);
  bool f64 = TypeUsesFloat64Representation(element);
  switch (vector_opcode) {
    case IR_OP(vadd): return f32 ? IR_OP(addf) : f64 ? IR_OP(addd) : IR_OP(addi);
    case IR_OP(vsub): return f32 ? IR_OP(subf) : f64 ? IR_OP(subd) : IR_OP(subi);
    case IR_OP(vmul): return f32 ? IR_OP(mulf) : f64 ? IR_OP(muld) : IR_OP(muli);
    case IR_OP(vdiv): return f32 ? IR_OP(divf) : f64 ? IR_OP(divd) : IR_OP(divi);
    case IR_OP(vmod): return IR_OP(modi);
    case IR_OP(vlsl): return IR_OP(lsli);
    case IR_OP(vlsr): return IR_OP(lsri);
    case IR_OP(vasr): return IR_OP(asri);
    case IR_OP(vand): return IR_OP(andi);
    case IR_OP(vor): return IR_OP(ori);
    case IR_OP(vxor): return IR_OP(xori);
    case IR_OP(vneg): return f32 ? IR_OP(negf) : f64 ? IR_OP(negd) : IR_OP(negi);
    case IR_OP(vonescomp): return IR_OP(onescomp);
    case IR_OP(vcmpeq): return f32 ? IR_OP(cmpeqf) : f64 ? IR_OP(cmpeqd)
                                                     : IR_OP(cmpeqi);
    case IR_OP(vcmpne): return f32 ? IR_OP(cmpnef) : f64 ? IR_OP(cmpned)
                                                     : IR_OP(cmpnei);
    case IR_OP(vcmplt): return f32 ? IR_OP(cmpltf) : f64 ? IR_OP(cmpltd)
                                                     : IR_OP(cmplti);
    case IR_OP(vcmple): return f32 ? IR_OP(cmplef) : f64 ? IR_OP(cmpled)
                                                     : IR_OP(cmplei);
    case IR_OP(vcmpgt): return f32 ? IR_OP(cmpgtf) : f64 ? IR_OP(cmpgtd)
                                                     : IR_OP(cmpgti);
    case IR_OP(vcmpge): return f32 ? IR_OP(cmpgef) : f64 ? IR_OP(cmpged)
                                                     : IR_OP(cmpgei);
    case IR_OP(vcmpltu): return IR_OP(cmplti);
    case IR_OP(vcmpleu): return IR_OP(cmplei);
    case IR_OP(vcmpgtu): return IR_OP(cmpgti);
    case IR_OP(vcmpgeu): return IR_OP(cmpgei);
    default: return IR_OP(nop);
  }
}

static IRNode* EmitVectorLaneAddressBefore(Generator* gen, BasicBlock* block,
                                           IRNode* position, IRNode* base,
                                           TypeRecord* element, int offset) {
  if (offset == 0) {
    return base;
  }
  IRNode* address = IRSetType(
      NewIR2(IR_OP(adda), base,
             GeneratorGetIntConstant(gen, NULL, offset)),
      NewPointerTo(kQualPlain, element));
  BasicBlockEmitBefore(gen, block, address, position);
  return address;
}

static void ScalarizeVectorOperation(Generator* gen, IRNode* operation) {
  BasicBlock* block = operation->block;
  IRNode* destination = operation->inputs.value.p[0];
  IRNode* left = operation->inputs.value.p[1];
  IRNode* right = operation->inputs.length > 2
                      ? operation->inputs.value.p[2]
                      : NULL;
  TypeRecord* source_vector =
      TypeIsVector((TypeRecord*)operation->aux)
          ? (TypeRecord*)operation->aux
          : VectorObjectTypeFromAddress(left);
  TypeRecord* result_vector = operation->type;
  assert(block != NULL && source_vector != NULL &&
         TypeIsVector(result_vector));
  TypeRecord* source_element = TypeVectorElement(source_vector);
  TypeRecord* result_element = TypeVectorElement(result_vector);
  IROpcode scalar_opcode =
      VectorScalarOpcode(operation->opcode, source_element);
  bool comparison =
      operation->opcode >= IR_OP(vcmpeq) &&
      operation->opcode <= IR_OP(vcmpgeu);
  bool signed_comparison =
      operation->opcode == IR_OP(vcmplt) ||
      operation->opcode == IR_OP(vcmple) ||
      operation->opcode == IR_OP(vcmpgt) ||
      operation->opcode == IR_OP(vcmpge);

  for (int lane = 0; lane < TypeVectorLaneCount(source_vector); lane++) {
    IRNode* left_address = EmitVectorLaneAddressBefore(
        gen, block, operation, left, source_element,
        lane * source_element->size);
    IRNode* left_value = IRSetType(
        NewIR1(GetLoadOpcodeForType(source_element), left_address),
        source_element);
    BasicBlockEmitBefore(gen, block, left_value, operation);
    if (signed_comparison && source_element->size < 4) {
      TypeRecord* promoted =
          NewTypeRecordWithSize(kTypeInt, kQualPlain);
      left_value = IRSetType(
          NewIR2(IR_OP(signextendi), left_value,
                 GeneratorGetIntConstant(
                     gen, NULL, (4 - source_element->size) * 8)),
          promoted);
      BasicBlockEmitBefore(gen, block, left_value, operation);
    }

    IRNode* value = NULL;
    if (right != NULL) {
      IRNode* right_address = EmitVectorLaneAddressBefore(
          gen, block, operation, right, source_element,
          lane * source_element->size);
      IRNode* right_value = IRSetType(
          NewIR1(GetLoadOpcodeForType(source_element), right_address),
          source_element);
      BasicBlockEmitBefore(gen, block, right_value, operation);
      if (signed_comparison && source_element->size < 4) {
        TypeRecord* promoted =
            NewTypeRecordWithSize(kTypeInt, kQualPlain);
        right_value = IRSetType(
            NewIR2(IR_OP(signextendi), right_value,
                   GeneratorGetIntConstant(
                       gen, NULL, (4 - source_element->size) * 8)),
            promoted);
        BasicBlockEmitBefore(gen, block, right_value, operation);
      }
      value = IRSetType(NewIR2(scalar_opcode, left_value, right_value),
                        comparison
                            ? NewTypeRecordWithSize(kTypeBool, kQualPlain)
                            : source_element);
    } else {
      value = IRSetType(NewIR1(scalar_opcode, left_value), source_element);
    }
    BasicBlockEmitBefore(gen, block, value, operation);

    if (comparison) {
      value = IRSetType(NewIR1(IR_OP(negi), value), result_element);
      BasicBlockEmitBefore(gen, block, value, operation);
    }
    IRNode* destination_address = EmitVectorLaneAddressBefore(
        gen, block, operation, destination, result_element,
        lane * result_element->size);
    IRNode* store = IRSetType(
        NewIR2(GetStoreOpcodeForType(result_element), destination_address,
               value),
        result_element);
    BasicBlockEmitBefore(gen, block, store, operation);
  }
  BasicBlockRemoveInstruction(gen, block, operation);
}

static void ScalarizeVectorOperations(Generator* gen) {
  for (IRNode* node = (IRNode*)gen->code.first; node != NULL;) {
    IRNode* next = IRNext(node);
    if (IsVectorIROpcode(node->opcode)) {
      TypeRecord* vector_type =
          TypeIsVector((TypeRecord*)node->aux)
              ? (TypeRecord*)node->aux
              : node->inputs.length > 0
                    ? VectorObjectTypeFromAddress(node->inputs.value.p[0])
                    : NULL;
      TypeRecord* element =
          vector_type != NULL ? TypeVectorElement(vector_type) : NULL;
      bool native_x86 =
          StringEqual(compiler->target_name, "x86_64") &&
          vector_type != NULL && vector_type->size == 16 &&
          element != NULL &&
          ((TypeIsIntegral(element) &&
            ((node->opcode == IR_OP(vadd) ||
              node->opcode == IR_OP(vsub) ||
              node->opcode == IR_OP(vand) ||
              node->opcode == IR_OP(vor) ||
              node->opcode == IR_OP(vxor)) ||
             ((node->opcode == IR_OP(vcmpeq)) && element->size <= 4) ||
             ((node->opcode == IR_OP(vcmpgt) ||
               node->opcode == IR_OP(vcmplt)) &&
              !TypeIsUnsigned(element) && element->size <= 4))) ||
           ((TypeUsesFloat32Representation(element) ||
             TypeUsesFloat64Representation(element)) &&
            (node->opcode == IR_OP(vadd) ||
             node->opcode == IR_OP(vsub) ||
             node->opcode == IR_OP(vmul) ||
             node->opcode == IR_OP(vdiv))));
      bool native_aarch64 =
          StringEqual(compiler->target_name, "aarch64") &&
          vector_type != NULL &&
          (vector_type->size == 8 || vector_type->size == 16) &&
          element != NULL &&
          ((TypeIsIntegral(element) &&
            (node->opcode == IR_OP(vadd) ||
             node->opcode == IR_OP(vsub) ||
             node->opcode == IR_OP(vand) ||
             node->opcode == IR_OP(vor) ||
             node->opcode == IR_OP(vxor) ||
             node->opcode == IR_OP(vcmpeq) ||
             node->opcode == IR_OP(vcmplt) ||
             node->opcode == IR_OP(vcmple) ||
             node->opcode == IR_OP(vcmpgt) ||
             node->opcode == IR_OP(vcmpge) ||
             node->opcode == IR_OP(vcmpltu) ||
             node->opcode == IR_OP(vcmpleu) ||
             node->opcode == IR_OP(vcmpgtu) ||
             node->opcode == IR_OP(vcmpgeu))) ||
           ((TypeUsesFloat32Representation(element) ||
             TypeUsesFloat64Representation(element)) &&
            (node->opcode == IR_OP(vadd) ||
             node->opcode == IR_OP(vsub) ||
             node->opcode == IR_OP(vmul) ||
             node->opcode == IR_OP(vdiv))));
      // ARM NEON has no vector fdiv, and integer compares are only 8/16/32-bit.
      bool native_arm =
          StringEqual(compiler->target_name, "arm") &&
          vector_type != NULL &&
          (vector_type->size == 8 || vector_type->size == 16) &&
          element != NULL &&
          ((TypeIsIntegral(element) &&
            (node->opcode == IR_OP(vadd) ||
             node->opcode == IR_OP(vsub) ||
             node->opcode == IR_OP(vand) ||
             node->opcode == IR_OP(vor) ||
             node->opcode == IR_OP(vxor) ||
             ((node->opcode == IR_OP(vcmpeq) ||
               node->opcode == IR_OP(vcmplt) ||
               node->opcode == IR_OP(vcmple) ||
               node->opcode == IR_OP(vcmpgt) ||
               node->opcode == IR_OP(vcmpge) ||
               node->opcode == IR_OP(vcmpltu) ||
               node->opcode == IR_OP(vcmpleu) ||
               node->opcode == IR_OP(vcmpgtu) ||
               node->opcode == IR_OP(vcmpgeu)) &&
              element->size <= 4))) ||
           ((TypeUsesFloat32Representation(element) ||
             TypeUsesFloat64Representation(element)) &&
            (node->opcode == IR_OP(vadd) ||
             node->opcode == IR_OP(vsub) ||
             node->opcode == IR_OP(vmul))));
      bool native_riscv =
          StringEqual(compiler->target_name, "riscv") &&
          vector_type != NULL &&
          (vector_type->size == 8 || vector_type->size == 16) &&
          element != NULL &&
          ((TypeIsIntegral(element) &&
            (node->opcode == IR_OP(vadd) ||
             node->opcode == IR_OP(vsub) ||
             node->opcode == IR_OP(vmul) ||
             node->opcode == IR_OP(vdiv) ||
             node->opcode == IR_OP(vmod) ||
             node->opcode == IR_OP(vlsl) ||
             node->opcode == IR_OP(vlsr) ||
             node->opcode == IR_OP(vasr) ||
             node->opcode == IR_OP(vand) ||
             node->opcode == IR_OP(vor) ||
             node->opcode == IR_OP(vxor) ||
             node->opcode == IR_OP(vcmpeq) ||
             node->opcode == IR_OP(vcmpne) ||
             node->opcode == IR_OP(vcmplt) ||
             node->opcode == IR_OP(vcmple) ||
             node->opcode == IR_OP(vcmpgt) ||
             node->opcode == IR_OP(vcmpge) ||
             node->opcode == IR_OP(vcmpltu) ||
             node->opcode == IR_OP(vcmpleu) ||
             node->opcode == IR_OP(vcmpgtu) ||
             node->opcode == IR_OP(vcmpgeu))) ||
           ((TypeUsesFloat32Representation(element) ||
             TypeUsesFloat64Representation(element)) &&
            (node->opcode == IR_OP(vadd) ||
             node->opcode == IR_OP(vsub) ||
             node->opcode == IR_OP(vmul) ||
             node->opcode == IR_OP(vdiv))));
      if (native_x86 || native_aarch64 || native_arm || native_riscv) {
        node = next;
        continue;
      }
      ScalarizeVectorOperation(gen, node);
    }
    node = next;
  }
}

void OptimizeFunctionIR(Generator* gen) {
  MarkVariablesWhoseAddressEscapes(gen);

  if (OptLevel2()) {
    ScalarReplacementOptimization(gen);
  }

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    GeneratorPrintIR(gen, compiler->ir_output_file);
  }

  BuildBasicBlocks(gen);

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    fprintf(compiler->ir_output_file, "Before SSA conversion\n");
    PrintBasicBlocks(gen, compiler->ir_output_file);
  }

  CheckReturn(gen);
  RemoveUnreachableBlocks(gen);

  if (OptLevel2() && compiler->ir_optimizations.code_motion &&
      compiler->ir_optimizations.derived_induction_vars &&
      compiler->ir_optimizations.loop_preheaders) {
    DerivedInductionVariableOptimization(gen);
  }

  GeneratorConvertToSSA(gen);

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    fprintf(compiler->ir_output_file, "After SSA conversion\n");
    PrintBasicBlocks(gen, compiler->ir_output_file);
  }

  DetectUninitializedVars(gen);

  if (OptLevel2()) {
    StrengthReductionOptimization(gen);

    if (compiler->ir_optimizations.gvn) {
      GlobalValueNumberingOptimization(gen);
    }

    if (compiler->ir_optimizations.sccp) {
      if (SparseConditionalConstantPropagation(gen, NULL)) {
        AnalyzeCFG(gen);
      }
    }

    if (compiler->ir_optimizations.const_prop) {
      ConstantPropagationOptimization(gen);
    }

    if (compiler->ir_optimizations.copy_prop) {
      CopyPropagationOptimization(gen);
    }

    if (compiler->ir_optimizations.dce) {
      DeadCodeEliminationOptimization(gen);
    }

    MemoryOptimization(gen);

    if (compiler->ir_optimizations.code_motion &&
        compiler->ir_optimizations.loop_preheaders) {
      CodeMotionOptimization(gen);
    }

    if (compiler->ir_optimizations.induction_vars) {
      InductionVariableOptimization(gen);
    }

    if (compiler->ir_optimizations.copy_prop) {
      CopyPropagationOptimization(gen);
    }
    MemoryOptimization(gen);
    if (compiler->ir_optimizations.dce) {
      DeadCodeEliminationOptimization(gen);
    }
  }

  DetectInvalidValueReads(gen);

  if (!compiler->keep_ssa) {
    GeneratorRemoveSSA(gen);
  }

  if (OptLevel2() && !compiler->keep_ssa) {
    AutoVectorizeOptimization(gen);
  }

  if (OptLevel3() && !compiler->keep_ssa) {
    LoopUnrollOptimization(gen);
    MemoryOptimization(gen);
    AutoVectorizeOptimization(gen);
  }

  RemoveUnreachableBlocks(gen);

  if (OptLevel2() && compiler->ir_optimizations.tail_call) {
    TailCallOptimization(gen);
  }

  if (compiler->print_back_end) {
    fprintf(compiler->ir_output_file, "After SSA has been removed\n");
  }

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    PrintBasicBlocks(gen, compiler->ir_output_file);
  }

  ScalarizeVectorOperations(gen);
  TrapFunctionAfterCodegen(gen);
}

void GenerateFunctionIR(Generator* gen) {
  CompoundStatementASTNode* body = (CompoundStatementASTNode*)gen->func->info.function.body;
  // Template and inline ASTs can be emitted more than once. IR label nodes are
  // owned by one Generator and are destroyed with its IR, so never reuse the
  // cached pointer left by an earlier emission of the same AST.
  ASTNodeVisit(&body->base, ResetASTIRLabel, 0, NULL);
  if (body->statements->length == 0 &&
      gen->func->info.function.contract_assertions.length == 0) {
    // Empty function, just return.
    GeneratorEmit(gen, NewIR(IR_OP(ret)));
  } else {
    GeneratorEmit(gen, NewIR(IR_OP(enter)));

    // If the return value comes back in memory generate a holder.
    if (TypeReturnedThroughHiddenPointer(gen->func->next)) {
      gen->struct_return_value = GeneratorEmit(gen, NewIR(IR_OP(structreturn)));
      IRSetType(gen->struct_return_value,
                NewPointerTo(kQualPlain, TypeRecordCopy(gen->func->next)));
    }
 
    TrapFunctionBeforeCodegen(gen);

    // Generate size expression for all VLAs in the prototype.
    GenerateVLASizeExpressions(gen, &gen->func->info.function.prototype);

    // A noexcept function gets a function-wide guard so that any exception
    // escaping it calls std::terminate, as required by [except.spec].
    NoexceptTerminateGuard noexcept_guard;
    GenerateNoexceptGuardEnter(gen, &noexcept_guard);

    GenerateFunctionContractAssertions(gen, kContractPrecondition);
    GenerateStatement(gen, &body->base);
    // Explicit returns evaluate postconditions in GenerateReturnStatement.
    // Only a void-returning function can normally exit by falling through.
    if (TypeIsVoid(gen->func->next)) {
      GenerateFunctionContractAssertions(gen, kContractPostcondition);
    }

    GenerateNoexceptGuardLeave(gen, &noexcept_guard);

    if (gen->return_label == NULL) {
      if (strcmp(gen->func->info.function.symbol->name.value, "main") == 0) {
        // main: add a resulti 0.
        IRNode* zero = GeneratorEmitConstant(gen,
                                             NewIntIRConstant(
                                                              NewTypeRecordWithSize(kTypeInt, kQualPlain), 0));
        GeneratorEmit(gen, NewIR1(IR_OP(resulti), zero));
      }
      GeneratorEmit(gen, NewIR(IR_OP(leave)));
      GeneratorEmit(gen, NewIR(IR_OP(ret)));
    } else {
      IRNode* return_label = GeneratorGetReturnLabel(gen);
      GeneratorEmit(gen, NewIR1(IR_OP(bra), return_label));
    }

    // Placed after the return path so they are only entered via the unwinder.
    GenerateCleanupLandingPads(gen);
    GenerateNoexceptGuardTerminate(gen, &noexcept_guard);
  }

  OptimizeFunctionIR(gen);
}

void* GenerateFunction(Generator* gen) {
  GenerateFunctionIR(gen);
  return compiler->target->codegen(gen);
}
