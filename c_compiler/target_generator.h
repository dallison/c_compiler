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
  TARGET_OP(save),      // Save registers.
  TARGET_OP(restore),   // Restore registers.

  TARGET_OP(symbol),    // Static symbol.
  TARGET_OP(literal),   // String literal.
  TARGET_OP(tmp),       // Temporary result.

  // Constants.
  TARGET_OP(constb),    // 8-bit int constant.
  TARGET_OP(consth),    // 16-bit int constant.
  TARGET_OP(constw),    // 32-bit int constant.
  TARGET_OP(constx),    // 64-bit int constant.
  TARGET_OP(constf),    // 32-bit float constant.
  TARGET_OP(constd),    // 64-bit float constant.

  TARGET_OP(mov),       // Move int register.
  TARGET_OP(movf),      // Move 32-bit float register.
  TARGET_OP(movd),      // Move 64-bit float register.
  
  TARGET_OP(movc),      // Move 32-int constant.
  TARGET_OP(movfc),     // Move 32-bit float constant.
  TARGET_OP(movdc),     // Move 64-bit float constant.
  TARGET_OP(movxc),     // Move 64-bit int constant.

  TARGET_OP(rmov),      // Move int reg to reg.
  TARGET_OP(rmovf),     // Move 32-bit float reg to reg.
  TARGET_OP(rmovd),     // Move 64 bit float reg to reg.

  TARGET_OP(ret),       // Return from subroutine.

  TARGET_OP(label),     // Label.

  TARGET_OP(fp),  // Frame pointer pseudo operation.
  TARGET_OP(sp),  // Stack pointer pseudo operation.
  TARGET_OP(tp),  // Thread pointer pseudo operation.

  // Function result registers.
  TARGET_OP(resultx),     // Place in int result reg.
  TARGET_OP(resultf),     // Place in 32-bit float result reg.
  TARGET_OP(resultd),     // Place in 64-bit float result reg.

  TARGET_OP(structreturn),  // Struct return address.

  TARGET_OP(asm),       // Insert assembly language.

  TARGET_OP(loc),       // Code location.
  TARGET_OP(named_label),     // Named label.
  
  // Variable assigned to registers,
  TARGET_OP(ivarreg),
  TARGET_OP(fvarreg),
} TargetOpcode;

typedef enum {
  kTargetTypeByte,       // 8-bit int.
  kTargetTypeHalf,       // 16-bit int.
  kTargetTypeWord,       // 32-bit int.
  kTargetTypeExtended,   // 64-bit int.
  kTargetTypeFloat,      // 32-bit float.
  kTargetTypeDouble,     // 64-bit float.
  kTargetTypeAddress,    // 32 or 64-bit address.
} TargetType;

// A register.
typedef struct TargetRegister {
  int num;                          // Register number.
  bool reserved;                    // Is reserved.
  struct TargetInstruction* owner;  // Owner instruction.
} TargetRegister;

#define TARGET_MAX_OPERANDS 3

typedef struct TargetInstruction {
  ListElement header;
  TargetOpcode opcode;
  int id;                     // Unique instruction id.
  struct TargetInstruction* dest;  // Optional destination.
  struct TargetRegister* reg;  // Register assigned by register allocator.
  struct TargetInstruction* operand[TARGET_MAX_OPERANDS];
  Vector users;                // Instructions using this instruction's value.
  int uses;                    // Current number of uses.
  void* block;                 // Target basic block.
  
  // Flags.  The lower 16 bits are reserved for TargetInstruction use
  // The upper 16 bits are free for code generators.
  int flags;
} TargetInstruction;

// Flag for TargetInstruction to mark an instruction as dead.
#define TARGET_INST_DEAD 1

// Register has been spilled onto the stack.
#define TARGET_INST_SPILLED 2

// Instruction has been processed by something.
#define TARGET_INST_PROCESSED 4

// A table jump instruction.
#define TARGET_INST_TABLE_JUMP 8

// A constant.
typedef struct {
  TargetInstruction base;
  TargetType type;
  union {
    int64_t ivalue;
    double dvalue;
    String* svalue;
  } value;
  int literal_id;
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

typedef struct {
  TargetInstruction base;
  const char* name;
} TargetNamedLabel;

typedef const char* (*TargetOpcodeNameFunc)(int op);
typedef bool (*TargetInferenceFunc)(TargetInstruction* inst);
typedef TargetInstruction* (*TargetGetBranchTargetFunc)(TargetInstruction* inst);

// Target virtual functions, provided by individual architecture specific
// target generators.
typedef struct {
  TargetOpcodeNameFunc opcode_name;
  TargetInferenceFunc is_branch;
  TargetInferenceFunc is_return;
  TargetInferenceFunc is_call;
  TargetInferenceFunc is_spill;
  TargetInferenceFunc is_label;
  TargetInferenceFunc is_floating_point;
  TargetInferenceFunc is_conditional_branch;
  TargetInferenceFunc is_fixed_register;
  TargetInferenceFunc is_const;
  TargetInferenceFunc is_symbol;
  TargetInferenceFunc is_expression;
  TargetInferenceFunc is_table_entry;
  TargetGetBranchTargetFunc get_branch_target;
} TargetVirtuals;

typedef struct TargetGenerator {
  TargetVirtuals* virtuals;
  String function_name;   // Current function name.
  bool is_global;         // Function is global.
  int num_calls;          // Number of calls in function.
  bool varargs;           // Function uses variable args.

  List code;                         // The code.
  TargetInstruction* last_constant;  // Last constant.
  TargetInstruction* first_symbol;   // First symbol.
  TargetInstruction* last_symbol;    // Last symbol.
  
  TargetInstruction* frame_pointer;  // Frame pointer.
  TargetInstruction* stack_pointer;  // Stack pointer.
  TargetInstruction* thread_pointer;  // Thread pointer.

  // Size of stack frame for current function.
  int32_t stack_frame_size;

  Vector fixups;    // Fixups to be applied.

  Vector basic_blocks;
  struct TargetBasicBlock* entry_block;
  struct TargetBasicBlock* exit_block;

  // We use the libc functions memcpy and memset for automatic
  // array and struct operations.
  Symbol* memcpy;
  Symbol* memset;
  
  Symbol* __tls_get_addr;   // Get address of TLS variable.
} TargetGenerator;

void TargetGeneratorInit(TargetGenerator* Target, Generator* gen, TargetVirtuals* virtuals);
TargetGenerator* NewTargetGenerator(Generator* gen);

void TargetGeneratorDestruct(TargetGenerator* Target);
void TargetGeneratorDelete(TargetGenerator* Target);

void TargetPrintInstruction(TargetInstruction* inst,
                            const char* (*name_func)(int), FILE* fp);

TargetInstruction* TargetFirstInstruction(TargetGenerator* Target);
TargetInstruction* TargetLastInstruction(TargetGenerator* Target);
TargetInstruction* TargetNext(TargetInstruction* inst);
TargetInstruction* TargetPrev(TargetInstruction* inst);

void TargetAddUser(TargetInstruction* inst, TargetInstruction* user);
void TargetRemoveUser(TargetInstruction* inst, TargetInstruction* user);

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
void TargetReplaceInstruction(TargetGenerator* target, TargetInstruction* old, TargetInstruction* new);
void TargetRetargetInstruction(TargetInstruction* old, TargetInstruction* new);
void TargetRetargetInstructionIf(TargetInstruction* old, TargetInstruction* new, bool (*predicate)(TargetInstruction*));
void TargetReplaceOperand(TargetInstruction* inst, int op, TargetInstruction* new);

bool TargetIsConst(TargetInstruction* inst);

int64_t TargetIntValue(TargetInstruction* inst);

// Get the lowered instruction from the IR node.  This is
// held in the data.ptr field of the node.  It must
// have already been set.
TargetInstruction* TargetGetLoweredNode(IRNode* node);

TargetInstruction* TargetSetLoweredNode(IRNode* node, TargetInstruction* inst);

void TargetInitInstruction(TargetInstruction* inst, TargetOpcode opcode);
void TargetUpdateOperandUsers(TargetInstruction* inst);

TargetInstruction* TargetNewInstruction(TargetOpcode opcode);

TargetInstruction* TargetNewInstruction1(TargetOpcode opcode,
                                         TargetInstruction* op1);

TargetInstruction* TargetNewInstruction2(TargetOpcode opcode,
                                         TargetInstruction* op1,
                                         TargetInstruction* op2);
TargetInstruction* TargetNewInstruction3(TargetOpcode opcode,
                                         TargetInstruction* op1,
                                         TargetInstruction* op2,
                                         TargetInstruction* op3);
TargetInstruction* TargetSetDest(TargetInstruction* inst,
                                 TargetInstruction* dest);

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
TargetInstruction* TargetThreadPointer(TargetGenerator* target);

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
                                                  double value);

TargetInstruction* TargetNewLocation(IRLocation* loc);
TargetInstruction* TargetNewNamedLabel(const char* name);

TargetInstruction* NewTargetSymbol(Symbol* symbol);

TargetInstruction* TargetGetSymbol(TargetGenerator* target, IRNode* node,
                                   Symbol* symbol);
TargetBranchFixup* NewBranchFixup(TargetInstruction* inst, IRNode* target,
                                  int operand);

void TargetApplyFixups(TargetGenerator* target, IRNode* label_node);

void TargetRegisterInit(TargetRegister* reg, int num);

// Generate the name of a symbol in the name given.
const char* TargetSymbolName(Symbol* symbol, char* buf, size_t len);

#endif /* target_generator_h */
