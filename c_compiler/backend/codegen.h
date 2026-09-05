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

// Marker written into an exception-table entry's typeinfo slot to distinguish a
// cleanup range (landing pad destroys one automatic object and resumes
// unwinding) from a handler.  Must match DAVECC_EH_CLEANUP in libc/eh_throw.c.
#define DAVECC_EH_CLEANUP_MARKER 1

typedef struct {
  IRNode* try_start;
  IRNode* try_end;
  IRNode* catch_label;      // For a cleanup range this is the cleanup pad.
  struct EHTypeInfo* catch_typeinfo;
  bool is_cleanup;          // Landing pad runs a destructor then resumes.
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
  Symbol* canonical_typeinfo;
  size_t lsda_type_filter;
  Vector bases;  // EHTypeInfoBase* entries (flattened public base graph).
  int64_t object_size;
  bool object_is_class;
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
  Vector cleanup_pads;       // PendingCleanupPad* entries, emitted at fn end.

  Vector basic_blocks;      // Basic Blocks (indexed by block id).
  Vector loops;             // LoopInfo* records for natural loops.
  BasicBlock* entry_block;  // Entry block.
  BasicBlock* exit_block;   // Exit block.

  // True when generating IR for compile-time constant evaluation (the constexpr
  // p-code interpreter) rather than a real target.  Runtime-only constructs
  // such as the noexcept terminate guard are suppressed in this mode.
  bool for_constant_evaluation;

  // ABI pointer width of the source target at initialization time. Constexpr
  // p-code lowering temporarily switches compiler->target to p-code, but the
  // already-analyzed object layouts still use this original width.
  int source_pointer_size;
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

// Frame-slot alignment for a pooled variable.  An `alignas`/`aligned(N)`
// specifier on the declaration raises the type's natural alignment, so the
// symbol's requested alignment has to be consulted as well as the type's.
int PoolEntryStackAlignment(PoolEntry* entry);
int SymbolStackAlignment(Symbol* symbol);
bool SymbolNeedsDynamicStackAllocation(Symbol* symbol);

// True for a return type the caller receives through the hidden pointer that
// IR_OP(structreturn) holds, rather than in a result register.  Besides classes
// and unions this covers a pointer to member function, whose pair layout is too
// wide for one register and which is already passed by address as an argument.
bool TypeReturnedThroughHiddenPointer(TypeRecord* type);
bool TypeUsesNativeVectorABI(TypeRecord* type);

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
