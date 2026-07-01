//
//  codegen.h
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef codegen_h
#define codegen_h

#include <stdint.h>
#include <stdarg.h>

#include "ast.h"
#include "basic_block.h"
#include "buffer.h"
#include "ir.h"
#include "list.h"
#include "syntax.h"
#include "type.h"
#include "vector.h"

// A pooled constant or variable.  We keep these together
// to tidy up the code and eliminate multiple nodes for the
// same value.
typedef struct {
  Type type;
  union {
    int64_t ivalue;
    double fvalue;
    Symbol* symbol;
  } value;
  IRNode* pooled;
} PoolEntry;

typedef struct {
  IRNode* try_start;
  IRNode* try_end;
  IRNode* catch_label;
  struct EHTypeInfo* catch_typeinfo;
} ExceptionHandlerRange;

// A (transitive, non-virtual, public) base subobject of an exception type,
// recorded so the runtime can match a handler naming a base class and adjust
// the exception object pointer to that base subobject.
typedef struct EHTypeInfoBase {
  String base_name;  // Same name format as EHTypeInfo::type_name.
  int64_t offset;    // Byte offset of the base subobject from the object start.
} EHTypeInfoBase;

typedef struct EHTypeInfo {
  String symbol_name;
  String type_name;
  Vector bases;  // EHTypeInfoBase* entries (flattened public base graph).
} EHTypeInfo;

// Main IR code generator.
typedef struct Generator {
  Syntax* syntax;
  TypeRecord* func;       // Type for function being generated.
  List code;              // Generated IR.
  IRNode* last_constant;  // Last constant instruction.
  IRNode* last_variable;  // Last variable reference.

  IRNode* break_label;     // Label that 'break' goes to.
  IRNode* continue_label;  // Label that 'continue' goes to.

  IRNode* struct_return_value;  // Struct return value node.
  IRNode* current_struct_address;

  IRNode* return_label;  // All returns branch to the same label.

  Vector int_constant_pool;  // Pooled integer constants.
  Vector fp_constant_pool;   // Pooled floating point constants.
  Vector variable_pool;      // Pooled variables.
  Vector exception_ranges;   // ExceptionHandlerRange* entries.
  Vector exception_keep_labels;  // IR labels reachable only through EH pads.
  Vector exception_typeinfos; // EHTypeInfo* entries emitted for this function.

  Vector basic_blocks;      // Basic Blocks (indexed by block id).
  BasicBlock* entry_block;  // Entry block.
  BasicBlock* exit_block;   // Exit block.

  // True when generating IR for compile-time constant evaluation (the constexpr
  // p-code interpreter) rather than a real target.  Runtime-only constructs
  // such as the noexcept terminate guard are suppressed in this mode.
  bool for_constant_evaluation;
} Generator;

void GeneratorInit(Generator* gen, Syntax* syntax, TypeRecord* func);
void GeneratorPrintIR(Generator* gen, FILE* fp);
void GeneratorDestruct(Generator* gen);

IRNode* GeneratorGetReturnLabel(Generator* gen);

IRNode* GeneratorFirstInstruction(Generator* gen);
IRNode* GeneratorLastInstruction(Generator* gen);

// Emit instruction at the end of the instruction list.
IRNode* GeneratorEmit(Generator* gen, IRNode* inst);
IRNode* GeneratorEmitBefore(Generator* gen, IRNode* inst, IRNode* pos);
IRNode* GeneratorEmitAfter(Generator* gen, IRNode* inst, IRNode* pos);
IRNode* GeneratorEmitConstant(Generator* gen, IRNode* inst);
IRNode* GeneratorEmitVariable(Generator* gen, IRNode* inst);
void GeneratorRemoveInstruction(Generator* gen, IRNode* inst);
void GeneratorMoveInstructionBefore(Generator* gen, IRNode* inst, IRNode* pos);
void GeneratorMoveInstructionAfter(Generator* gen, IRNode* inst, IRNode* pos);
void GeneratorReplaceInstruction(Generator* gen, IRNode* old, IRNode* new);

IRNode* GeneratorGetIntConstant(Generator* gen, TypeRecord* type,
                                int64_t value);
IRNode* GeneratorGetFloatingPointConstant(Generator* gen, TypeRecord* type,
                                          double value);
IRNode* GeneratorGetVariable(Generator* gen, Symbol* sym);

void* GenerateFunction(Generator* gen);

int GeneratorNumCalls(Generator* gen);
uint64_t CXXExceptionTypeID(TypeRecord* type);
EHTypeInfo* GeneratorGetExceptionTypeInfo(Generator* gen, TypeRecord* type);

void GeneratorError(Generator* gen, ASTNode* node, const char* format, ...);
void VGeneratorError(Generator* gen, ASTNode* node, const char* format,
                     va_list ap);

void GeneratorWarning(Generator* gen, ASTNode* node, const char* warn,
                      const char* format, ...);
void VGeneratorWarning(Generator* gen, ASTNode* node, const char* warn,
                       const char* format, va_list ap);

void CheckForVarDef(IRNode* write, ASTNode* node);
void CheckForVarUse(IRNode* read, ASTNode* node);

#endif /* codegen_h */
