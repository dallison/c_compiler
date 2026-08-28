//
//  wasm32_codegen.h
//  c_compiler
//
//  Lowering from the shared IR to WebAssembly instructions.
//

#ifndef wasm32_codegen_h
#define wasm32_codegen_h

#include "codegen.h"
#include "list.h"
#include "target_generator.h"
#include "vector.h"
#include "wasm32_machine.h"

// Operand conventions for the wasm instructions built here.  The shared
// target layer only requires that a branch keep its destination label in
// operand[1] (see TargetVirtuals::get_branch_target), everything else is
// ours to choose.
//
//   br              operand[1] = destination label
//   br_if           operand[0] = condition, operand[1] = destination label
//   loads           operand[0] = address, operand[1] = constant byte offset
//   stores          operand[0] = address, operand[1] = value,
//                   operand[2] = constant byte offset
//   call            operand[0] = callee symbol, arguments follow as a
//                   pusharg chain recorded in the argument vector
//   local_get/set   operand[0] = the value (set only); the local index is
//                   the allocated register number
//
// A constant operand that only supplies an immediate is never emitted as an
// instruction of its own; the encoder reads its value directly.
//
// TargetInstruction::addr carries the resolved relative branch depth after
// the stackifier has run.  The shared layer only ever prints that field.

// One wasm value type slot in a function signature.
typedef struct {
  Vector param_types;  // WasmValueType values, boxed as intptr_t.
  WasmValueType result_type;
  bool has_result;
} Wasm32Signature;

typedef struct Wasm32Generator {
  TargetGenerator base;

  Wasm32Signature signature;

  // Locals beyond the parameters, grouped by value type.  The final local
  // index of an instruction is num_params + type_base[t] + ordinal.
  int num_params;
  int num_locals[4];
  int type_base[4];

  // The instruction whose local holds the base of this function's shadow
  // frame, or NULL when the function needs no frame.
  // The function returns an aggregate, so its first wasm parameter is the
  // caller's destination pointer rather than a declared argument.
  bool returns_struct;
  // The function is variadic, so its last wasm parameter points at the
  // caller's buffer of variadic arguments.
  bool is_varargs;
  // Frame offset of this function's own outgoing variadic argument buffer,
  // or -1 when it makes no variadic calls.
  int32_t vararg_buffer;
  TargetInstruction* frame_pointer;

  // One 'param' pseudo instruction per wasm parameter, so that lowering can
  // refer to a parameter local without an encoded instruction.
  Vector params;

  // The instruction that owns the local holding this function's return
  // value.  Every IR result node writes into that one local so the single
  // return site can read it.
  TargetInstruction* result_value;

  // Instructions that were assigned a local, in allocation order.
  Vector local_values;

  // Set when lowering hit something it cannot yet translate.  The target
  // reports this as a compile error rather than emitting a bad module.
  bool failed;
} Wasm32Generator;

void Wasm32GeneratorInit(Wasm32Generator* wasm, Generator* gen);
Wasm32Generator* NewWasm32Generator(Generator* gen);
void Wasm32GeneratorDestruct(Wasm32Generator* wasm);
void Wasm32GeneratorDelete(Wasm32Generator* wasm);

// Lower the IR held in 'gen' into wasm instructions.
void Wasm32Lower(Wasm32Generator* wasm, Generator* gen);

void Wasm32Print(Wasm32Generator* wasm);

const char* Wasm32OpcodeName(int op);

// The value type an instruction leaves on the stack.  Only meaningful when
// the instruction actually produces a value.
WasmValueType Wasm32InstructionType(TargetInstruction* inst);
void Wasm32SetInstructionType(TargetInstruction* inst, WasmValueType type);

// True when the instruction pushes a value that something must consume.
bool Wasm32ProducesValue(TargetInstruction* inst);

// True for the pseudo instructions that exist only to carry an immediate or
// a symbol and are never encoded on their own.
bool Wasm32IsPseudo(TargetInstruction* inst);

// True when operand 'index' supplies an immediate or a branch target rather
// than a value the operand stack must carry.
bool Wasm32OperandIsImmediate(TargetInstruction* inst, int index);

// True for a pseudo instruction that stands for a local: reading it is a
// local.get even though the pseudo itself encodes to nothing.
bool Wasm32NamesLocal(TargetInstruction* inst);

// Index into the per-type local arrays: i32, i64, f32, f64.
int Wasm32TypeIndex(WasmValueType type);

// The wasm value type that holds a C value of the given type.
WasmValueType Wasm32TypeForCType(TypeRecord* type);

#endif /* wasm32_codegen_h */
