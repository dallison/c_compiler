//
//  target_generator.h
//  c_compiler_library
//
//  Created by David Allison on 2/24/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef target_generator_h
#define target_generator_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"

#define TARGET_OP(op) kTarget_##op

typedef enum {
  // Pseudo ops.
  TARGET_OP(save),
  TARGET_OP(restore),

  TARGET_OP(symbol),   // Static symbol.
  TARGET_OP(literal),  // String literal.
  TARGET_OP(tmp),

  // Constants.
  TARGET_OP(constb),
  TARGET_OP(consth),
  TARGET_OP(constw),
  TARGET_OP(constx),
  TARGET_OP(constf),
  TARGET_OP(constd),

  TARGET_OP(mov),
  TARGET_OP(movf),
  TARGET_OP(movd),

  TARGET_OP(movc),
  TARGET_OP(movfc),
  TARGET_OP(movdc),
  TARGET_OP(movxc),

  TARGET_OP(rmov),
  TARGET_OP(rmovf),
  TARGET_OP(rmovd),

  TARGET_OP(ret),

  TARGET_OP(label),

  TARGET_OP(fp),  // Frame pointer pseudo operation.
  TARGET_OP(sp),  // Stack pointer pseudo operation.

  // Function result registers.
  TARGET_OP(resultx),
  TARGET_OP(resultf),
  TARGET_OP(resultd),

  TARGET_OP(structreturn),  // Struct return address.

  TARGET_OP(asm),

  TARGET_OP(loc),

} TargetOpcode;

typedef enum {
  kTargetTypeByte,
  kTargetTypeHalf,
  kTargetTypeWord,
  kTargetTypeExtended,
  kTargetTypeFloat,
  kTargetTypeDouble,
  kTargetTypeAddress,
} TargetType;

typedef struct TargetRegister {
  int num;
  bool reserved;
  struct TargetInstruction* owner;
} TargetRegister;

#define TARGET_MAX_OPERANDS 3

typedef struct TargetInstruction {
  ListElement header;
  TargetOpcode opcode;
  int id;
  int refs;                    // Number of references to this instruction.
  int uses;                    // Current number of uses.
  struct TargetRegister* reg;  // Register assigned by register allocator.
  struct TargetInstruction* operand[TARGET_MAX_OPERANDS];

  // Flags.  The lower 16 bits are reserved for TargetInstruction use
  // The upper 16 bits are free for code generators.
  int flags;
} TargetInstruction;

// Flag for TargetInstruction to mark an instruction as dead.
#define TARGET_INST_DEAD 1

// A constant.
typedef struct {
  TargetInstruction base;
  TargetType type;
  union {
    int64_t ivalue;
    double dvalue;
    String* svalue;
  } value;
} TargetConstant;

typedef struct {
  TargetInstruction base;
  SourceLocation location;
} TargetLocation;

// Symbol reference.
typedef struct {
  TargetInstruction base;
  Symbol* symbol;
} TargetSymbol;

// A String literal with an id.
typedef struct {
  TargetInstruction base;
  int literal_id;
} TargetLiteral;

typedef struct {
  TargetInstruction* inst;
  IRNode* target;
  int operand;
} TargetBranchFixup;

typedef struct TargetGenerator {
  String function_name;
  bool is_global;
  int num_calls;
  bool varargs;

  List code;
  TargetInstruction* last_constant;
  TargetInstruction* frame_pointer;
  TargetInstruction* stack_pointer;
  TargetInstruction* first_symbol;
  TargetInstruction* last_symbol;

  // Size of stack frame for current function.
  int32_t stack_frame_size;

  Vector fixups;

  // We use the libc functions memcpy and memset for automatic
  // array and struct operations.
  Symbol* memcpy;
  Symbol* memset;
} TargetGenerator;

void TargetGeneratorInit(TargetGenerator* Target, Generator* gen);
TargetGenerator* NewTargetGenerator(Generator* gen);

void TargetGeneratorDestruct(TargetGenerator* Target);
void TargetGeneratorDelete(TargetGenerator* Target);

void TargetPrintInstruction(TargetInstruction* inst,
                            const char* (*name_func)(int));

TargetInstruction* TargetFirstInstruction(TargetGenerator* Target);
TargetInstruction* TargetLastInstruction(TargetGenerator* Target);
TargetInstruction* TargetNext(TargetInstruction* inst);
TargetInstruction* TargetPrev(TargetInstruction* inst);

// Lower the IR to Target.
void TargetLower(TargetGenerator* Target, Generator* gen);
void TargetPrint(TargetGenerator* Target);

const char* TargetOpcodeName(int op);
bool TargetIsConst(TargetInstruction* inst);
bool TargetIsZero(TargetInstruction* inst);
int64_t TargetIntValue(TargetInstruction* inst);

TargetInstruction* TargetFirstInstruction(TargetGenerator* target);

TargetInstruction* TargetLastConstant(TargetGenerator* target);

TargetInstruction* TargetFirstSymbol(TargetGenerator* target);

TargetInstruction* TargetLastSymbol(TargetGenerator* target);

TargetInstruction* TargetNext(TargetInstruction* inst);

TargetInstruction* TargetPrev(TargetInstruction* inst);

void TargetDeleteInstruction(TargetGenerator* target, TargetInstruction* inst);

bool TargetIsConst(TargetInstruction* inst);

int64_t TargetIntValue(TargetInstruction* inst);

// Get the lowered instruction from the IR node.  This is
// held in the data.ptr field of the node.  It must
// have already been set.
TargetInstruction* TargetGetLoweredNode(IRNode* node);

TargetInstruction* TargetSetLoweredNode(IRNode* node, TargetInstruction* inst);

void TargetInitInstruction(TargetInstruction* inst, TargetOpcode opcode);
void TargetUpdateRefCount(TargetInstruction* inst);

TargetInstruction* TargetNewInstruction(TargetOpcode opcode);

TargetInstruction* TargetNewInstruction2(TargetOpcode opcode,
                                         TargetInstruction* op1);

TargetInstruction* TargetNewInstruction3(TargetOpcode opcode,
                                         TargetInstruction* op1,
                                         TargetInstruction* op2);
TargetInstruction* TargetNewInstruction4(TargetOpcode opcode,
                                         TargetInstruction* op1,
                                         TargetInstruction* op2,
                                         TargetInstruction* op3);

TargetInstruction* TargetEmit(TargetGenerator* target, TargetInstruction* inst);
TargetInstruction* TargetEmitBefore(TargetGenerator* target,
                                    TargetInstruction* inst,
                                    TargetInstruction* pos);

TargetInstruction* TargetEmitAfter(TargetGenerator* target,
                                   TargetInstruction* inst,
                                   TargetInstruction* pos);

TargetInstruction* TargetEmitConstant(TargetGenerator* target,
                                      TargetInstruction* c);

TargetInstruction* TargetEmitSymbol(TargetGenerator* target,
                                    TargetInstruction* c);
TargetInstruction* TargetFramePointer(TargetGenerator* target);

TargetInstruction* TargetStackPointer(TargetGenerator* target);
TargetInstruction* TargetNewLiteral(int id);

bool TargetIsConst(TargetInstruction* inst);

TargetInstruction* TargetNewIntConstant(IRNode* node, TargetType type,
                                        int64_t value);
TargetInstruction* TargetNewFloatingPointConstant(IRNode* node, TargetType type,
                                                  double value);

TargetInstruction* TargetGetIntConstant(TargetGenerator* target, IRNode* node,
                                        TargetType type, int64_t value);
TargetInstruction* TargetGetFloatingPointConstant(TargetGenerator* target,
                                                  IRNode* node, TargetType type,
                                                  int64_t value);

TargetInstruction* TargetNewLocation(IRLocation* loc);

TargetInstruction* NewTargetSymbol(Symbol* symbol);

TargetInstruction* TargetGetSymbol(TargetGenerator* target, IRNode* node,
                                   Symbol* symbol);
TargetBranchFixup* NewBranchFixup(TargetInstruction* inst, IRNode* target,
                                  int operand);

void TargetApplyFixups(TargetGenerator* target, IRNode* label_node);

void TargetRegisterInit(TargetRegister* reg, int num);

#endif /* target_generator_h */
