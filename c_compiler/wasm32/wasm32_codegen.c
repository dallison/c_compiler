//
//  wasm32_codegen.c
//  c_compiler
//
//  Lowering from the shared IR to WebAssembly instructions.
//
//  Every value produced here is round-tripped through a wasm local.  That
//  keeps lowering and encoding simple and always produces a module that
//  validates; the stackifier later folds single-use values into the operand
//  stack, which is purely an optimization.
//

#include "wasm32_codegen.h"

#include "wasm32_module.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"

static const char* const kOpcodeNames[] = {
#define WASM32_OPCODE_NAME(name, encoding) #name,
    WASM32_OPCODES(WASM32_OPCODE_NAME)
#undef WASM32_OPCODE_NAME
};

static const int kOpcodeEncodings[] = {
#define WASM32_OPCODE_ENCODING(name, encoding) (encoding),
    WASM32_OPCODES(WASM32_OPCODE_ENCODING)
#undef WASM32_OPCODE_ENCODING
};

// The first entries of Wasm32Opcode must line up with TargetOpcode.  If this
// ever drifts the shared target layer silently misreads instructions.
_Static_assert((int)W_OP(fvarreg) == (int)TARGET_OP(fvarreg),
               "wasm32 opcodes must mirror TargetOpcode");
_Static_assert((int)W_OP(save) == (int)TARGET_OP(save),
               "wasm32 opcodes must mirror TargetOpcode");
_Static_assert((int)W_OP(structreturn) == (int)TARGET_OP(structreturn),
               "wasm32 opcodes must mirror TargetOpcode");

const char* Wasm32OpcodeName(int op) {
  if (op < 0 || op >= kWasm32_num_opcodes) {
    return "<bad opcode>";
  }
  return kOpcodeNames[op];
}

int Wasm32OpcodeEncoding(int op) {
  if (op < 0 || op >= kWasm32_num_opcodes) {
    return WASM_NO_ENCODING;
  }
  return kOpcodeEncodings[op];
}

WasmValueType Wasm32InstructionType(TargetInstruction* inst) {
  switch (inst->flags & WASM32_FLAG_TYPE_MASK) {
    case WASM32_FLAG_TYPE_I64:
      return kWasmTypeI64;
    case WASM32_FLAG_TYPE_F32:
      return kWasmTypeF32;
    case WASM32_FLAG_TYPE_F64:
      return kWasmTypeF64;
    default:
      return kWasmTypeI32;
  }
}

void Wasm32SetInstructionType(TargetInstruction* inst, WasmValueType type) {
  int bits;
  switch (type) {
    case kWasmTypeI64:
      bits = WASM32_FLAG_TYPE_I64;
      break;
    case kWasmTypeF32:
      bits = WASM32_FLAG_TYPE_F32;
      break;
    case kWasmTypeF64:
      bits = WASM32_FLAG_TYPE_F64;
      break;
    default:
      bits = WASM32_FLAG_TYPE_I32;
      break;
  }
  inst->flags = (inst->flags & ~WASM32_FLAG_TYPE_MASK) | bits;
}

int Wasm32TypeIndex(WasmValueType type) {
  switch (type) {
    case kWasmTypeI64:
      return 1;
    case kWasmTypeF32:
      return 2;
    case kWasmTypeF64:
      return 3;
    default:
      return 0;
  }
}

// The wasm value type used to hold a C value of the given type.  Everything
// narrower than 32 bits lives in an i32; wasm has no smaller value type.
WasmValueType Wasm32TypeForCType(TypeRecord* type) {
  if (type == NULL) {
    return kWasmTypeI32;
  }
  if (TypeIsFloatingPoint(type)) {
    return type->size <= 4 ? kWasmTypeF32 : kWasmTypeF64;
  }
  if (TypeIsPointerOrArray(type) || TypeIsStructOrUnion(type)) {
    return kWasmTypeI32;
  }
  return type->size > 4 ? kWasmTypeI64 : kWasmTypeI32;
}

// How many bytes of a value a wasm local of this type holds.
static int WasmTypeSize(WasmValueType type) {
  return (type == kWasmTypeI64 || type == kWasmTypeF64) ? 8 : 4;
}

bool Wasm32IsPseudo(TargetInstruction* inst) {
  switch ((Wasm32Opcode)inst->opcode) {
    case W_OP(const8):
    case W_OP(const16):
    case W_OP(const32):
    case W_OP(const64):
    case W_OP(constf):
    case W_OP(constd):
    case W_OP(symbol):
    case W_OP(literal):
    case W_OP(tmp):
    case W_OP(fp):
    case W_OP(sp):
    case W_OP(tp):
    case W_OP(loc):
    case W_OP(param):
    case W_OP(slot):
      return true;
    default:
      return false;
  }
}

bool Wasm32ProducesValue(TargetInstruction* inst) {
  if ((inst->flags & WASM32_FLAG_NO_RESULT) != 0) {
    return false;
  }
  switch ((Wasm32Opcode)inst->opcode) {
    case W_OP(label):
    case W_OP(named_label):
    case W_OP(ret):
    case W_OP(return):
    case W_OP(br):
    case W_OP(br_if):
    case W_OP(br_table):
    case W_OP(block):
    case W_OP(loop):
    case W_OP(if):
    case W_OP(else):
    case W_OP(end):
    case W_OP(unreachable):
    case W_OP(nop):
    case W_OP(drop):
    case W_OP(local_set):
    case W_OP(global_set):
    case W_OP(i32_store):
    case W_OP(i64_store):
    case W_OP(f32_store):
    case W_OP(f64_store):
    case W_OP(i32_store8):
    case W_OP(i32_store16):
    case W_OP(i64_store8):
    case W_OP(i64_store16):
    case W_OP(i64_store32):
    case W_OP(memory_copy):
    case W_OP(memory_fill):
      return false;
    default:
      return !Wasm32IsPseudo(inst);
  }
}

// Operand slots that carry an immediate or a label rather than a value that
// must be pushed on the operand stack.
bool Wasm32NamesLocal(TargetInstruction* inst) {
  return inst != NULL && (inst->opcode == (TargetOpcode)W_OP(param) ||
                          inst->opcode == (TargetOpcode)W_OP(slot));
}

bool Wasm32OperandIsImmediate(TargetInstruction* inst, int index) {
  TargetInstruction* operand = inst->operand[index];
  // Constants, symbols and labels never leave anything on the operand
  // stack; whatever references them reads their value as an immediate.
  // Pseudos that name a live local are the exception - reading one is a
  // real local.get.
  if (operand != NULL && !Wasm32NamesLocal(operand) &&
      (Wasm32IsPseudo(operand) ||
       operand->opcode == (TargetOpcode)W_OP(label) ||
       operand->opcode == (TargetOpcode)W_OP(named_label))) {
    return true;
  }
  switch ((Wasm32Opcode)inst->opcode) {
    case W_OP(br):
    case W_OP(br_if):
    case W_OP(br_table):
      return index == 1;
    case W_OP(i32_load):
    case W_OP(i64_load):
    case W_OP(f32_load):
    case W_OP(f64_load):
    case W_OP(i32_load8_s):
    case W_OP(i32_load8_u):
    case W_OP(i32_load16_s):
    case W_OP(i32_load16_u):
    case W_OP(i64_load8_s):
    case W_OP(i64_load8_u):
    case W_OP(i64_load16_s):
    case W_OP(i64_load16_u):
    case W_OP(i64_load32_s):
    case W_OP(i64_load32_u):
      return index == 1;
    case W_OP(i32_store):
    case W_OP(i64_store):
    case W_OP(f32_store):
    case W_OP(f64_store):
    case W_OP(i32_store8):
    case W_OP(i32_store16):
    case W_OP(i64_store8):
    case W_OP(i64_store16):
    case W_OP(i64_store32):
      return index == 2;
    case W_OP(call):
      return index == 0;
    default:
      return false;
  }
}

static bool Wasm32IsBranch(TargetInstruction* inst) {
  switch ((Wasm32Opcode)inst->opcode) {
    case W_OP(br):
    case W_OP(br_if):
    case W_OP(br_table):
      return true;
    default:
      return false;
  }
}

static bool Wasm32IsConditionalBranch(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)W_OP(br_if);
}

static bool Wasm32IsReturn(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)W_OP(return) ||
         inst->opcode == (TargetOpcode)W_OP(ret);
}

static bool Wasm32IsCall(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)W_OP(call) ||
         inst->opcode == (TargetOpcode)W_OP(call_indirect);
}

static bool Wasm32IsLabel(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)W_OP(label);
}

static bool Wasm32IsSpill(TargetInstruction* inst) { return false; }

static bool Wasm32IsFloatingPoint(TargetInstruction* inst) {
  WasmValueType type = Wasm32InstructionType(inst);
  return type == kWasmTypeF32 || type == kWasmTypeF64;
}

static bool Wasm32IsFixedRegister(TargetInstruction* inst) { return false; }

static bool Wasm32IsConst(TargetInstruction* inst) {
  switch ((Wasm32Opcode)inst->opcode) {
    case W_OP(const8):
    case W_OP(const16):
    case W_OP(const32):
    case W_OP(const64):
    case W_OP(constf):
    case W_OP(constd):
      return true;
    default:
      return false;
  }
}

static bool Wasm32IsSymbol(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)W_OP(symbol);
}

static bool Wasm32IsExpression(TargetInstruction* inst) {
  return Wasm32ProducesValue(inst);
}

static bool Wasm32IsJumpTableEntry(TargetInstruction* inst) {
  return false;
}

static TargetInstruction* Wasm32GetBranchTarget(TargetInstruction* inst) {
  return inst->operand[1];
}

static TargetVirtuals virtuals = {
    .opcode_name = Wasm32OpcodeName,
    .is_branch = Wasm32IsBranch,
    .is_call = Wasm32IsCall,
    .is_return = Wasm32IsReturn,
    .is_spill = Wasm32IsSpill,
    .is_label = Wasm32IsLabel,
    .is_floating_point = Wasm32IsFloatingPoint,
    .is_conditional_branch = Wasm32IsConditionalBranch,
    .is_fixed_register = Wasm32IsFixedRegister,
    .is_const = Wasm32IsConst,
    .is_symbol = Wasm32IsSymbol,
    .is_expression = Wasm32IsExpression,
    .is_table_entry = Wasm32IsJumpTableEntry,
    .get_branch_target = Wasm32GetBranchTarget,
};

void Wasm32GeneratorInit(Wasm32Generator* wasm, Generator* gen) {
  TargetGeneratorInit(&wasm->base, gen, &virtuals);
  VectorInit(&wasm->signature.param_types);
  wasm->signature.result_type = kWasmTypeVoid;
  wasm->signature.has_result = false;
  wasm->num_params = 0;
  memset(wasm->num_locals, 0, sizeof(wasm->num_locals));
  memset(wasm->type_base, 0, sizeof(wasm->type_base));
  wasm->returns_struct = false;
  wasm->is_varargs = false;
  wasm->vararg_buffer = -1;
  wasm->frame_pointer = NULL;
  wasm->result_value = NULL;
  VectorInit(&wasm->params);
  VectorInit(&wasm->local_values);
  VectorInit(&wasm->local_variables);
  wasm->failed = false;
}

Wasm32Generator* NewWasm32Generator(Generator* gen) {
  Wasm32Generator* wasm = malloc(sizeof(Wasm32Generator));
  Wasm32GeneratorInit(wasm, gen);
  return wasm;
}

void Wasm32GeneratorDestruct(Wasm32Generator* wasm) {
  TargetGeneratorDestruct(&wasm->base);
  VectorDestruct(&wasm->signature.param_types);
  VectorDestruct(&wasm->params);
  VectorDestruct(&wasm->local_values);
  VectorDestruct(&wasm->local_variables);
}

void Wasm32GeneratorDelete(Wasm32Generator* wasm) {
  Wasm32GeneratorDestruct(wasm);
  free(wasm);
}

// Utilities that narrow the generic target helpers to this backend.

static TargetInstruction* NewInstruction(Wasm32Opcode opcode) {
  return TargetNewInstruction((TargetOpcode)opcode);
}

static TargetInstruction* NewInstruction1(Wasm32Opcode opcode,
                                          TargetInstruction* op1) {
  return TargetNewInstruction1((TargetOpcode)opcode, op1);
}

static TargetInstruction* NewInstruction2(Wasm32Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  return TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
}

static TargetInstruction* NewInstruction3(Wasm32Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3) {
  return TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
}

static TargetInstruction* Emit(Wasm32Generator* wasm, TargetInstruction* inst) {
  return TargetEmit(&wasm->base, inst);
}

static TargetInstruction* GetIntConstant(Wasm32Generator* wasm, IRNode* node,
                                         TargetType type, int64_t value) {
  return TargetGetIntConstant(&wasm->base, node, type, value);
}

static TargetInstruction* SetLoweredNode(IRNode* node,
                                         TargetInstruction* inst) {
  // Some values are stashed in a temporary as they are produced, and every
  // later read goes to the temporary rather than back to the node that made
  // it, so both names have to reach the same instruction.  On a machine with
  // registers the stash is a real move; here the value already lives in a
  // local, and naming it twice is the whole of the work.
  if (node != NULL && node->dest != NULL &&
      node->dest->opcode == IR_OP(tmp)) {
    TargetSetLoweredNode(node->dest, inst);
  }
  return TargetSetLoweredNode(node, inst);
}

static void Fail(Wasm32Generator* wasm, const char* what) {
  if (!wasm->failed) {
    fprintf(stderr,
            "wasm32: cannot yet generate code for %s.  This construct is not "
            "supported by the wasm32 backend.\n",
            what);
    wasm->failed = true;
  }
}

// Materialize an IR node into an instruction that leaves its value on the
// operand stack.  Constants become the corresponding wasm const instruction.
static TargetInstruction* Materialize(Wasm32Generator* wasm, IRNode* node);

static TargetInstruction* MaterializeConstant(Wasm32Generator* wasm,
                                              IRNode* node) {
  IRConstant* c = (IRConstant*)node;
  TargetInstruction* inst;
  switch (node->opcode) {
    case IR_OP(const8):
    case IR_OP(const16):
    case IR_OP(const32):
      inst = NewInstruction1(
          W_OP(i32_const),
          GetIntConstant(wasm, NULL, kTargetType32Bit, c->value.ivalue));
      Wasm32SetInstructionType(inst, kWasmTypeI32);
      break;
    case IR_OP(const64):
      inst = NewInstruction1(
          W_OP(i64_const),
          GetIntConstant(wasm, NULL, kTargetType64Bit, c->value.ivalue));
      Wasm32SetInstructionType(inst, kWasmTypeI64);
      break;
    case IR_OP(consta):
      inst = NewInstruction1(
          W_OP(i32_const),
          GetIntConstant(wasm, NULL, kTargetType32Bit, c->value.ivalue));
      Wasm32SetInstructionType(inst, kWasmTypeI32);
      break;
    case IR_OP(constf):
      inst = NewInstruction1(W_OP(f32_const),
                             TargetGetFloatingPointConstant(
                                 &wasm->base, NULL, kTargetTypeFloat,
                                 c->value.fvalue));
      Wasm32SetInstructionType(inst, kWasmTypeF32);
      break;
    case IR_OP(constd):
      inst = NewInstruction1(W_OP(f64_const),
                             TargetGetFloatingPointConstant(
                                 &wasm->base, NULL, kTargetTypeDouble,
                                 c->value.fvalue));
      Wasm32SetInstructionType(inst, kWasmTypeF64);
      break;
    default:
      assert(false);
      return NULL;
  }
  TargetUpdateOperandUsers(inst);
  TargetInstruction* result = Emit(wasm, inst);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerIRNode(Wasm32Generator* wasm, IRNode* node);
static TargetInstruction* MaterializeVariableAddress(Wasm32Generator* wasm,
                                                     IRNode* node);
static TargetInstruction* MaterializeStaticAddress(Wasm32Generator* wasm,
                                                   IRNode* node);

static TargetInstruction* Materialize(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* existing = node->data.ptr;
  if (existing != NULL) {
    return existing;
  }
  if (IRIsConstant(node)) {
    return MaterializeConstant(wasm, node);
  }
  if (IRIsAutoVariable(node) || IRIsArgument(node)) {
    return MaterializeVariableAddress(wasm, node);
  }
  if (IRIsStaticVariable(node)) {
    return MaterializeStaticAddress(wasm, node);
  }
  TargetInstruction* inst = LowerIRNode(wasm, node);
  if (inst == NULL) {
    Fail(wasm, IROpcodeName(node->opcode));
  }
  return inst;
}

// The IR assumes one uniform register width, but wasm makes i32 and i64
// distinct types that no instruction mixes.  Every value therefore has to be
// converted explicitly wherever the two disagree.
static TargetInstruction* Coerce(Wasm32Generator* wasm,
                                 TargetInstruction* value, WasmValueType want,
                                 bool is_unsigned) {
  if (value == NULL) {
    return NULL;
  }
  WasmValueType have = Wasm32InstructionType(value);
  if (have == want) {
    return value;
  }
  Wasm32Opcode opcode;
  switch (have) {
    case kWasmTypeI32:
      switch (want) {
        case kWasmTypeI64:
          opcode = is_unsigned ? W_OP(i64_extend_i32_u) : W_OP(i64_extend_i32_s);
          break;
        case kWasmTypeF32:
          opcode = is_unsigned ? W_OP(f32_convert_i32_u) : W_OP(f32_convert_i32_s);
          break;
        default:
          opcode = is_unsigned ? W_OP(f64_convert_i32_u) : W_OP(f64_convert_i32_s);
          break;
      }
      break;
    case kWasmTypeI64:
      switch (want) {
        case kWasmTypeI32:
          opcode = W_OP(i32_wrap_i64);
          break;
        case kWasmTypeF32:
          opcode = is_unsigned ? W_OP(f32_convert_i64_u) : W_OP(f32_convert_i64_s);
          break;
        default:
          opcode = is_unsigned ? W_OP(f64_convert_i64_u) : W_OP(f64_convert_i64_s);
          break;
      }
      break;
    case kWasmTypeF32:
      switch (want) {
        case kWasmTypeF64:
          opcode = W_OP(f64_promote_f32);
          break;
        case kWasmTypeI64:
          // The saturating forms match C's behaviour closely enough to be
          // useful; the plain ones trap on out-of-range input.
          opcode = is_unsigned ? W_OP(i64_trunc_sat_f32_u)
                               : W_OP(i64_trunc_sat_f32_s);
          break;
        default:
          opcode = is_unsigned ? W_OP(i32_trunc_sat_f32_u)
                               : W_OP(i32_trunc_sat_f32_s);
          break;
      }
      break;
    default:
      switch (want) {
        case kWasmTypeF32:
          opcode = W_OP(f32_demote_f64);
          break;
        case kWasmTypeI64:
          opcode = is_unsigned ? W_OP(i64_trunc_sat_f64_u)
                               : W_OP(i64_trunc_sat_f64_s);
          break;
        default:
          opcode = is_unsigned ? W_OP(i32_trunc_sat_f64_u)
                               : W_OP(i32_trunc_sat_f64_s);
          break;
      }
      break;
  }
  TargetInstruction* inst = NewInstruction1(opcode, value);
  Wasm32SetInstructionType(inst, want);
  TargetUpdateOperandUsers(inst);
  return Emit(wasm, inst);
}

// Map an IR expression opcode onto the wasm instruction for it.  'type' is
// the wasm type of the operands and 'is_unsigned' selects between the signed
// and unsigned forms where wasm distinguishes them.
static Wasm32Opcode IR2Wasm(Wasm32Generator* wasm, IROpcode op,
                            WasmValueType type, bool is_unsigned) {
  bool wide = type == kWasmTypeI64;
#define INT_OP(name) (wide ? W_OP(i64_##name) : W_OP(i32_##name))
#define INT_OP_SU(name) \
  (wide ? (is_unsigned ? W_OP(i64_##name##_u) : W_OP(i64_##name##_s)) \
        : (is_unsigned ? W_OP(i32_##name##_u) : W_OP(i32_##name##_s)))
  switch (op) {
    case IR_OP(addi):
    case IR_OP(adda):
      return INT_OP(add);
    case IR_OP(subi):
    case IR_OP(suba):
      return INT_OP(sub);
    case IR_OP(muli):
      return INT_OP(mul);
    case IR_OP(divi):
      return INT_OP_SU(div);
    case IR_OP(modi):
      return INT_OP_SU(rem);
    case IR_OP(ori):
      return INT_OP(or);
    case IR_OP(andi):
      return INT_OP(and);
    case IR_OP(xori):
      return INT_OP(xor);
    case IR_OP(lsli):
      return INT_OP(shl);
    case IR_OP(lsri):
      return wide ? W_OP(i64_shr_u) : W_OP(i32_shr_u);
    case IR_OP(asri):
      return wide ? W_OP(i64_shr_s) : W_OP(i32_shr_s);
    case IR_OP(rotli):
      return INT_OP(rotl);
    case IR_OP(rotri):
      return INT_OP(rotr);
    case IR_OP(clzi):
      return INT_OP(clz);
    case IR_OP(ctzi):
      return INT_OP(ctz);
    case IR_OP(popcounti):
      return INT_OP(popcnt);

    case IR_OP(addf):
      return W_OP(f32_add);
    case IR_OP(subf):
      return W_OP(f32_sub);
    case IR_OP(mulf):
      return W_OP(f32_mul);
    case IR_OP(divf):
      return W_OP(f32_div);
    case IR_OP(negf):
      return W_OP(f32_neg);
    case IR_OP(addd):
      return W_OP(f64_add);
    case IR_OP(subd):
      return W_OP(f64_sub);
    case IR_OP(muld):
      return W_OP(f64_mul);
    case IR_OP(divd):
      return W_OP(f64_div);
    case IR_OP(negd):
      return W_OP(f64_neg);

    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
      return INT_OP(eq);
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
      return INT_OP(ne);
    case IR_OP(cmplti):
      return INT_OP_SU(lt);
    case IR_OP(cmplei):
      return INT_OP_SU(le);
    case IR_OP(cmpgti):
      return INT_OP_SU(gt);
    case IR_OP(cmpgei):
      return INT_OP_SU(ge);
    case IR_OP(cmplta):
      return wide ? W_OP(i64_lt_u) : W_OP(i32_lt_u);
    case IR_OP(cmplea):
      return wide ? W_OP(i64_le_u) : W_OP(i32_le_u);
    case IR_OP(cmpgta):
      return wide ? W_OP(i64_gt_u) : W_OP(i32_gt_u);
    case IR_OP(cmpgea):
      return wide ? W_OP(i64_ge_u) : W_OP(i32_ge_u);

    case IR_OP(cmpeqf):
      return W_OP(f32_eq);
    case IR_OP(cmpnef):
      return W_OP(f32_ne);
    case IR_OP(cmpltf):
      return W_OP(f32_lt);
    case IR_OP(cmplef):
      return W_OP(f32_le);
    case IR_OP(cmpgtf):
      return W_OP(f32_gt);
    case IR_OP(cmpgef):
      return W_OP(f32_ge);
    case IR_OP(cmpeqd):
      return W_OP(f64_eq);
    case IR_OP(cmpned):
      return W_OP(f64_ne);
    case IR_OP(cmpltd):
      return W_OP(f64_lt);
    case IR_OP(cmpled):
      return W_OP(f64_le);
    case IR_OP(cmpgtd):
      return W_OP(f64_gt);
    case IR_OP(cmpged):
      return W_OP(f64_ge);

    default:
      return kWasm32_num_opcodes;
  }
#undef INT_OP
#undef INT_OP_SU
}

static bool IRIsComparisonOpcode(IROpcode op) {
  return op >= IR_OP(cmpeqi) && op <= IR_OP(cmpgea);
}

// The wasm type an expression's operands have.  Comparisons produce a bool
// but operate on their input type, so the inputs decide.  This is the only
// place the width is chosen: picking the instruction from the C type's byte
// size instead would make an i32 address whose C type is a large array or
// struct come out as an i64 operation.
static WasmValueType ExpressionOperandType(IRNode* node) {
  if (IRIsComparisonOpcode(node->opcode) && node->inputs.length > 0) {
    IRNode* input = node->inputs.value.p[0];
    return Wasm32TypeForCType(input != NULL ? input->type : NULL);
  }
  return Wasm32TypeForCType(node->type);
}

static bool ExpressionIsUnsigned(IRNode* node) {
  if (IRIsComparisonOpcode(node->opcode) && node->inputs.length > 0) {
    IRNode* input = node->inputs.value.p[0];
    return input != NULL && input->type != NULL && TypeIsUnsigned(input->type);
  }
  return node->type != NULL && TypeIsUnsigned(node->type);
}

static TargetInstruction* LowerExpression(Wasm32Generator* wasm,
                                          IRNode* node) {
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  // Both operands of a wasm instruction have to be exactly its own type, so
  // the same type that selects the instruction also drives the coercions.
  WasmValueType operand_type = ExpressionOperandType(node);
  bool is_unsigned = ExpressionIsUnsigned(node);
  Wasm32Opcode opcode = IR2Wasm(wasm, node->opcode, operand_type, is_unsigned);
  if (opcode == kWasm32_num_opcodes) {
    Fail(wasm, IROpcodeName(node->opcode));
    return NULL;
  }

  TargetInstruction* inst = NewInstruction(opcode);
  assert(node->inputs.length <= 2);
  for (size_t i = 0; i < node->inputs.length && i < 2; i++) {
    TargetInstruction* value = Materialize(wasm, node->inputs.value.p[i]);
    inst->operand[i] = Coerce(wasm, value, operand_type, is_unsigned);
  }
  // Comparisons yield an i32 boolean whatever their operands were.
  if (IRIsComparisonOpcode(node->opcode)) {
    Wasm32SetInstructionType(inst, kWasmTypeI32);
  } else {
    Wasm32SetInstructionType(inst, Wasm32TypeForCType(node->type));
  }
  TargetUpdateOperandUsers(inst);
  TargetInstruction* result = Emit(wasm, inst);
  SetLoweredNode(node, result);
  return result;
}

// !x is (x == 0), which wasm spells eqz.
static TargetInstruction* LowerNot(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* value = Materialize(wasm, node->inputs.value.p[0]);
  if (value == NULL) {
    return NULL;
  }
  Wasm32Opcode opcode = Wasm32InstructionType(value) == kWasmTypeI64
                            ? W_OP(i64_eqz)
                            : W_OP(i32_eqz);
  TargetInstruction* inst = NewInstruction1(opcode, value);
  Wasm32SetInstructionType(inst, kWasmTypeI32);
  TargetUpdateOperandUsers(inst);
  TargetInstruction* result = Emit(wasm, inst);
  SetLoweredNode(node, result);
  return result;
}

// -x has no wasm instruction for integers; it is 0 - x.
static TargetInstruction* LowerNegate(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* value = Materialize(wasm, node->inputs.value.p[0]);
  if (value == NULL) {
    return NULL;
  }
  bool wide = Wasm32InstructionType(value) == kWasmTypeI64;
  TargetInstruction* zero = Emit(
      wasm, NewInstruction1(wide ? W_OP(i64_const) : W_OP(i32_const),
                            GetIntConstant(wasm, NULL,
                                           wide ? kTargetType64Bit
                                                : kTargetType32Bit,
                                           0)));
  Wasm32SetInstructionType(zero, wide ? kWasmTypeI64 : kWasmTypeI32);
  TargetInstruction* inst = NewInstruction2(
      wide ? W_OP(i64_sub) : W_OP(i32_sub), zero, value);
  Wasm32SetInstructionType(inst, wide ? kWasmTypeI64 : kWasmTypeI32);
  TargetUpdateOperandUsers(inst);
  TargetInstruction* result = Emit(wasm, inst);
  SetLoweredNode(node, result);
  return result;
}

// ~x is x ^ -1.
static TargetInstruction* LowerOnesComplement(Wasm32Generator* wasm,
                                              IRNode* node) {
  TargetInstruction* value = Materialize(wasm, node->inputs.value.p[0]);
  if (value == NULL) {
    return NULL;
  }
  bool wide = Wasm32InstructionType(value) == kWasmTypeI64;
  TargetInstruction* ones = Emit(
      wasm, NewInstruction1(wide ? W_OP(i64_const) : W_OP(i32_const),
                            GetIntConstant(wasm, NULL,
                                           wide ? kTargetType64Bit
                                                : kTargetType32Bit,
                                           -1)));
  Wasm32SetInstructionType(ones, wide ? kWasmTypeI64 : kWasmTypeI32);
  TargetInstruction* inst = NewInstruction2(
      wide ? W_OP(i64_xor) : W_OP(i32_xor), value, ones);
  Wasm32SetInstructionType(inst, wide ? kWasmTypeI64 : kWasmTypeI32);
  TargetUpdateOperandUsers(inst);
  TargetInstruction* result = Emit(wasm, inst);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerMove(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* value = Materialize(wasm, node->inputs.value.p[0]);
  if (value == NULL) {
    return NULL;
  }
  SetLoweredNode(node, value);
  return value;
}

// The shadow stack.  Wasm's call stack is not addressable, so anything that
// needs an address - every local, since the IR reaches them through loads
// and stores - lives in a frame carved out of linear memory.  The frame base
// is held in a local for the duration of the call.

static TargetInstruction* EmitI32Constant(Wasm32Generator* wasm,
                                          int64_t value) {
  TargetInstruction* inst = NewInstruction1(
      W_OP(i32_const), GetIntConstant(wasm, NULL, kTargetType32Bit, value));
  Wasm32SetInstructionType(inst, kWasmTypeI32);
  TargetUpdateOperandUsers(inst);
  return Emit(wasm, inst);
}

// An integer constant of a given wasm type, for use as the other operand of
// a typed instruction.
static TargetInstruction* EmitIntConstant(Wasm32Generator* wasm,
                                          WasmValueType type, int64_t value) {
  bool wide = type == kWasmTypeI64;
  TargetInstruction* inst = NewInstruction1(
      wide ? W_OP(i64_const) : W_OP(i32_const),
      GetIntConstant(wasm, NULL, wide ? kTargetType64Bit : kTargetType32Bit,
                     value));
  Wasm32SetInstructionType(inst, type);
  TargetUpdateOperandUsers(inst);
  return Emit(wasm, inst);
}

static TargetInstruction* EmitBinary(Wasm32Generator* wasm, Wasm32Opcode op,
                                     WasmValueType type,
                                     TargetInstruction* left,
                                     TargetInstruction* right) {
  TargetInstruction* inst = NewInstruction2(op, left, right);
  Wasm32SetInstructionType(inst, type);
  TargetUpdateOperandUsers(inst);
  return Emit(wasm, inst);
}

static TargetInstruction* EmitGlobalGet(Wasm32Generator* wasm, int global) {
  TargetInstruction* inst = NewInstruction(W_OP(global_get));
  inst->operand[1] = GetIntConstant(wasm, NULL, kTargetType32Bit, global);
  Wasm32SetInstructionType(inst, kWasmTypeI32);
  TargetUpdateOperandUsers(inst);
  return Emit(wasm, inst);
}

static TargetInstruction* EmitGlobalSet(Wasm32Generator* wasm, int global,
                                        TargetInstruction* value) {
  TargetInstruction* inst = NewInstruction1(W_OP(global_set), value);
  inst->operand[1] = GetIntConstant(wasm, NULL, kTargetType32Bit, global);
  inst->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(inst);
  return Emit(wasm, inst);
}

static Wasm32Opcode StoreOpcodeForType(WasmValueType type, int size) {
  switch (type) {
    case kWasmTypeF32:
      return W_OP(f32_store);
    case kWasmTypeF64:
      return W_OP(f64_store);
    case kWasmTypeI64:
      switch (size) {
        case 1:
          return W_OP(i64_store8);
        case 2:
          return W_OP(i64_store16);
        case 4:
          return W_OP(i64_store32);
        default:
          return W_OP(i64_store);
      }
    default:
      switch (size) {
        case 1:
          return W_OP(i32_store8);
        case 2:
          return W_OP(i32_store16);
        default:
          return W_OP(i32_store);
      }
  }
}

static TargetInstruction* EmitStore(Wasm32Generator* wasm, Wasm32Opcode opcode,
                                    TargetInstruction* address,
                                    TargetInstruction* value, int32_t offset) {
  TargetInstruction* inst = NewInstruction2(opcode, address, value);
  inst->operand[2] = GetIntConstant(wasm, NULL, kTargetType32Bit, offset);
  inst->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(inst);
  return Emit(wasm, inst);
}

static TargetInstruction* EmitLoad(Wasm32Generator* wasm, Wasm32Opcode opcode,
                                   TargetInstruction* address, int32_t offset,
                                   WasmValueType result_type) {
  TargetInstruction* inst = NewInstruction1(opcode, address);
  inst->operand[1] = GetIntConstant(wasm, NULL, kTargetType32Bit, offset);
  Wasm32SetInstructionType(inst, result_type);
  TargetUpdateOperandUsers(inst);
  return Emit(wasm, inst);
}

// The address of a byte offset within the current frame.
static TargetInstruction* EmitFrameAddress(Wasm32Generator* wasm,
                                           int32_t offset) {
  if (offset == 0) {
    return wasm->frame_pointer;
  }
  return EmitBinary(wasm, W_OP(i32_add), kWasmTypeI32, wasm->frame_pointer,
                    EmitI32Constant(wasm, offset));
}

static TargetInstruction* EmitMemoryCopy(Wasm32Generator* wasm,
                                         TargetInstruction* destination,
                                         TargetInstruction* source,
                                         TargetInstruction* length) {
  TargetInstruction* copy =
      NewInstruction3(W_OP(memory_copy), destination, source, length);
  copy->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(copy);
  return Emit(wasm, copy);
}

static void LowerEnter(Wasm32Generator* wasm) {
  int32_t size = wasm->base.stack_frame_size;
  if (size > 0) {
    TargetInstruction* stack_pointer =
        EmitGlobalGet(wasm, WASM32_STACK_POINTER_GLOBAL);
    TargetInstruction* frame_size = EmitI32Constant(wasm, size);
    TargetInstruction* base =
        NewInstruction2(W_OP(i32_sub), stack_pointer, frame_size);
    Wasm32SetInstructionType(base, kWasmTypeI32);
    TargetUpdateOperandUsers(base);
    wasm->frame_pointer = Emit(wasm, base);
    EmitGlobalSet(wasm, WASM32_STACK_POINTER_GLOBAL, wasm->frame_pointer);
  }

  // Parameters arrive in wasm locals but the IR reads them out of memory, so
  // copy each one into its frame slot on entry.  The hidden struct-result
  // pointer is not one of them: it is read where the result is written, not
  // through a frame slot.  Nor is a parameter that kept its local, which was
  // given no frame slot to be copied into.
  size_t first = wasm->returns_struct ? 1 : 0;
  Vector* prototype = &compiler->current_function->info.function.prototype;
  for (size_t i = 0; i + first < wasm->params.length && i < prototype->length;
       i++) {
    Symbol* arg = prototype->value.p[i];
    if (arg == NULL || arg->type == NULL || arg->stack_offset < 0) {
      continue;
    }
    TargetInstruction* param = wasm->params.value.p[i + first];
    if (TypeIsStructOrUnion(arg->type)) {
      // The parameter is a pointer to the caller's copy.  Taking our own
      // copy now is what makes the argument pass by value, and it lets the
      // rest of the function treat the parameter like any other local.
      EmitMemoryCopy(wasm,
                     EmitFrameAddress(wasm, arg->stack_offset), param,
                     EmitI32Constant(wasm, arg->type->size));
      continue;
    }
    WasmValueType type = Wasm32InstructionType(param);
    EmitStore(wasm, StoreOpcodeForType(type, arg->type->size),
              wasm->frame_pointer, param, arg->stack_offset);
  }
}

// Undo the frame allocation.  This has to run before every return, not just
// the last one, or the stack pointer leaks on any path that returns early.
static void EmitEpilogueBefore(Wasm32Generator* wasm,
                               TargetInstruction* position) {
  int32_t size = wasm->base.stack_frame_size;
  if (size == 0 || wasm->frame_pointer == NULL) {
    return;
  }
  TargetInstruction* frame_size = NewInstruction1(
      W_OP(i32_const), GetIntConstant(wasm, NULL, kTargetType32Bit, size));
  Wasm32SetInstructionType(frame_size, kWasmTypeI32);
  TargetUpdateOperandUsers(frame_size);
  TargetEmitBefore(&wasm->base, frame_size, position);

  TargetInstruction* restored =
      NewInstruction2(W_OP(i32_add), wasm->frame_pointer, frame_size);
  Wasm32SetInstructionType(restored, kWasmTypeI32);
  TargetUpdateOperandUsers(restored);
  TargetEmitBefore(&wasm->base, restored, position);

  TargetInstruction* set = NewInstruction1(W_OP(global_set), restored);
  set->operand[1] = GetIntConstant(wasm, NULL, kTargetType32Bit,
                                   WASM32_STACK_POINTER_GLOBAL);
  set->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(set);
  TargetEmitBefore(&wasm->base, set, position);
}

static bool IRIsVLAVariable(IRNode* node) {
  if (!IRIsAutoVariable(node)) {
    return false;
  }
  Symbol* symbol = ((IRVariable*)node)->symbol;
  return symbol != NULL && TypeIsVLA(symbol->type);
}

// Where a variable-length array actually lives.  The declaration lowers to a
// savesp whose value is the array's base, and the symbol records which IR
// node that was.
static TargetInstruction* VLAAddress(Wasm32Generator* wasm, IRNode* node) {
  IRNode* holder = ((IRVariable*)node)->symbol->value.other;
  TargetInstruction* address = holder == NULL ? NULL : holder->data.ptr;
  if (address == NULL) {
    Fail(wasm, "a variable-length array used before it is allocated");
    return NULL;
  }
  return address;
}

// The address of a static variable, or of a string literal's bytes.  Neither
// is known until the module's data layout is fixed, so the instruction names
// what it wants and the encoder turns that into the i32 immediate.
static TargetInstruction* EmitDataAddress(Wasm32Generator* wasm, IRNode* node,
                                          Wasm32Opcode opcode,
                                          TargetInstruction* operand) {
  TargetInstruction* address = NewInstruction1(opcode, operand);
  Wasm32SetInstructionType(address, kWasmTypeI32);
  TargetUpdateOperandUsers(address);
  TargetInstruction* result = Emit(wasm, address);
  if (node != NULL) {
    SetLoweredNode(node, result);
  }
  return result;
}

static bool BuildSignatureForType(TypeRecord* type, Wasm32Signature* signature);
static void Wasm32SignatureDestruct(Wasm32Signature* signature);

static TargetInstruction* MaterializeStaticAddress(Wasm32Generator* wasm,
                                                   IRNode* node) {
  if (IRIsThreadVariable(node)) {
    Fail(wasm, "a thread-local variable");
    return NULL;
  }
  Symbol* symbol = ((IRVariable*)node)->symbol;
  if (!TypeIsFunction(symbol->type)) {
    return EmitDataAddress(wasm, node, W_OP(symbol_address),
                           NewTargetSymbol(symbol));
  }

  // A function has no address in linear memory, so a pointer to one is its
  // slot in the module's function table instead.
  TargetInstruction* address = EmitDataAddress(wasm, node, W_OP(function_index),
                                               NewTargetSymbol(symbol));
  // Taking an address is the only mention some functions get, and an import
  // standing in for one still has to be declared with a signature.  A host
  // call keeps that signature to the end, so it had better be the real one.
  Wasm32Signature signature;
  if (address != NULL && BuildSignatureForType(symbol->type, &signature)) {
    address->addr = Wasm32InternSignature(&signature);
  }
  Wasm32SignatureDestruct(&signature);
  return address;
}

static TargetInstruction* LowerLiteralReference(Wasm32Generator* wasm,
                                                IRNode* node) {
  IRConstant* id = node->inputs.value.p[0];
  return EmitDataAddress(wasm, node, W_OP(literal_address),
                         TargetNewLiteral((int)id->value.ivalue));
}

// Resolve an address IR node into a base value plus a static byte offset,
// which is exactly the shape of a wasm memarg.
// A variable the return value optimization named: it was never given a slot,
// because it is the caller's result buffer under a local's name, and the
// hidden first parameter is where that buffer is.
static TargetInstruction* NrvoAddress(Wasm32Generator* wasm, IRNode* node) {
  if (!wasm->returns_struct || wasm->params.length == 0) {
    Fail(wasm, "a return-value-optimized variable in a function that returns "
               "no struct");
    return NULL;
  }
  return wasm->params.value.p[0];
}

static bool IsNrvoVariable(IRNode* node) {
  return (node->flags & kIRNrvoMarker) != 0;
}

// A variable whose type is a function is not storage at all: it is the
// function itself, named by a declaration written inside a block.  It needs
// no frame slot, and reaching it means its table slot rather than an address.
static bool IsFunctionReference(IRNode* node) {
  if (!IRIsAutoVariable(node) && !IRIsArgument(node) &&
      !IRIsStaticVariable(node)) {
    return false;
  }
  Symbol* symbol = ((IRVariable*)node)->symbol;
  return symbol != NULL && TypeIsFunction(symbol->type);
}

// A variable that was kept in a wasm local instead of the shadow frame.  It
// has no address at all: reading it is a local.get and writing it a local.set,
// and anything that would need to point at it kept it out of a local in the
// first place.
static bool IsLocalVariable(IRNode* node) {
  return (IRIsAutoVariable(node) || IRIsArgument(node)) &&
         (node->data.ivalue & WASM32_LOCAL_VAR) != 0;
}

static TargetInstruction* LocalVariableSlot(Wasm32Generator* wasm,
                                            IRNode* node) {
  size_t index = (size_t)(node->data.ivalue & ~WASM32_LOCAL_VAR);
  assert(index < wasm->local_variables.length);
  return wasm->local_variables.value.p[index];
}

// Drop the bits a store of this width would not have kept and restore the
// sign the matching load would have put back.  A local is as wide as its
// value type, so without this a variable narrower than that would remember
// bits the frame slot it replaced would have thrown away.
static TargetInstruction* NarrowToVariableWidth(Wasm32Generator* wasm,
                                                TargetInstruction* value,
                                                TypeRecord* type) {
  WasmValueType have = Wasm32InstructionType(value);
  if (TypeIsFloatingPoint(type) || type->size >= WasmTypeSize(have)) {
    return value;
  }
  bool wide = have == kWasmTypeI64;
  TargetInstruction* narrowed;
  if (TypeIsUnsigned(type)) {
    uint64_t mask = (UINT64_C(1) << (type->size * 8)) - 1;
    TargetInstruction* mask_value = Emit(
        wasm, NewInstruction1(wide ? W_OP(i64_const) : W_OP(i32_const),
                              GetIntConstant(wasm, NULL,
                                             wide ? kTargetType64Bit
                                                  : kTargetType32Bit,
                                             (int64_t)mask)));
    Wasm32SetInstructionType(mask_value, have);
    narrowed = NewInstruction2(wide ? W_OP(i64_and) : W_OP(i32_and), value,
                               mask_value);
  } else {
    // A local is four bytes or eight and holds a value of its own width, so
    // the only widths narrower than one are a byte and a halfword.
    assert(type->size == 1 || type->size == 2);
    Wasm32Opcode extend;
    if (type->size == 1) {
      extend = wide ? W_OP(i64_extend8_s) : W_OP(i32_extend8_s);
    } else {
      extend = wide ? W_OP(i64_extend16_s) : W_OP(i32_extend16_s);
    }
    narrowed = NewInstruction1(extend, value);
  }
  Wasm32SetInstructionType(narrowed, have);
  TargetUpdateOperandUsers(narrowed);
  return Emit(wasm, narrowed);
}

// Write a value into the local standing in for a variable.
static TargetInstruction* EmitLocalVariableSet(Wasm32Generator* wasm,
                                               IRNode* node,
                                               TargetInstruction* value) {
  TargetInstruction* slot = LocalVariableSlot(wasm, node);
  TypeRecord* type = ((IRVariable*)node)->symbol->type;
  value = Coerce(wasm, value, Wasm32InstructionType(slot),
                 TypeIsUnsigned(type));
  if (value == NULL) {
    return NULL;
  }
  value = NarrowToVariableWidth(wasm, value, type);

  TargetInstruction* set = NewInstruction1(W_OP(local_set), value);
  set->dest = slot;
  set->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(set);
  return Emit(wasm, set);
}

// Read the local standing in for a variable.  The copy is what pins the read
// to this point in the stream: handing the slot itself to whatever consumes
// the value would instead read it wherever that consumer ends up, which is
// the wrong value if the variable is assigned in between.
static TargetInstruction* EmitLocalVariableGet(Wasm32Generator* wasm,
                                               IRNode* node) {
  TargetInstruction* slot = LocalVariableSlot(wasm, node);
  TargetInstruction* copy = NewInstruction1(W_OP(mov), slot);
  Wasm32SetInstructionType(copy, Wasm32InstructionType(slot));
  TargetUpdateOperandUsers(copy);
  return Emit(wasm, copy);
}

static bool GetAddressAndOffset(Wasm32Generator* wasm, IRNode* address_node,
                                TargetInstruction** base, int32_t* offset) {
  if (IsFunctionReference(address_node)) {
    *base = MaterializeStaticAddress(wasm, address_node);
    *offset = 0;
    return *base != NULL;
  }
  if (IsLocalVariable(address_node)) {
    Fail(wasm, "the address of a variable that has no address");
    return false;
  }
  if (IsNrvoVariable(address_node)) {
    *base = NrvoAddress(wasm, address_node);
    *offset = 0;
    return *base != NULL;
  }
  if (IRIsAutoVariable(address_node) || IRIsArgument(address_node)) {
    if (IRIsVLAVariable(address_node)) {
      *base = VLAAddress(wasm, address_node);
      *offset = 0;
      return *base != NULL;
    }
    if (wasm->frame_pointer == NULL) {
      Fail(wasm, "a frame variable in a function with no frame");
      return false;
    }
    *base = wasm->frame_pointer;
    *offset = (int32_t)address_node->data.ivalue;
    return true;
  }
  if (IRIsThreadVariable(address_node)) {
    Fail(wasm, "a thread-local variable");
    return false;
  }
  if (IRIsStaticVariable(address_node)) {
    *base = MaterializeStaticAddress(wasm, address_node);
    *offset = 0;
    return *base != NULL;
  }
  TargetInstruction* value = Materialize(wasm, address_node);
  if (value == NULL) {
    return false;
  }
  *base = value;
  *offset = 0;
  return true;
}

// Using a variable where a value is expected means its address: an array
// that decayed to a pointer, or the base of a member or element access.
static TargetInstruction* MaterializeVariableAddress(Wasm32Generator* wasm,
                                                     IRNode* node) {
  if (IsFunctionReference(node)) {
    return MaterializeStaticAddress(wasm, node);
  }
  if (IsLocalVariable(node)) {
    Fail(wasm, "the address of a variable that has no address");
    return NULL;
  }
  if (IsNrvoVariable(node)) {
    TargetInstruction* result = NrvoAddress(wasm, node);
    if (result != NULL) {
      SetLoweredNode(node, result);
    }
    return result;
  }
  if (IRIsVLAVariable(node)) {
    return VLAAddress(wasm, node);
  }
  if (wasm->frame_pointer == NULL) {
    Fail(wasm, "the address of a variable in a function with no frame");
    return NULL;
  }
  TargetInstruction* result =
      EmitFrameAddress(wasm, (int32_t)node->data.ivalue);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerLoad(Wasm32Generator* wasm, IRNode* node) {
  IRNode* address_node = node->inputs.value.p[0];
  if (IsLocalVariable(address_node)) {
    // The local already holds what a reload of the slot would have produced,
    // so the widening the load opcode names has nothing left to do.
    TargetInstruction* result = EmitLocalVariableGet(wasm, address_node);
    SetLoweredNode(node, result);
    return result;
  }
  TargetInstruction* base;
  int32_t offset;
  if (!GetAddressAndOffset(wasm, address_node, &base, &offset)) {
    return NULL;
  }
  Wasm32Opcode opcode;
  WasmValueType type = kWasmTypeI32;
  switch (node->opcode) {
    case IR_OP(load8):
      opcode = W_OP(i32_load8_s);
      break;
    case IR_OP(loadu8):
      opcode = W_OP(i32_load8_u);
      break;
    case IR_OP(load16):
      opcode = W_OP(i32_load16_s);
      break;
    case IR_OP(loadu16):
      opcode = W_OP(i32_load16_u);
      break;
    case IR_OP(load32):
    case IR_OP(loadu32):
    case IR_OP(loada):
      opcode = W_OP(i32_load);
      break;
    case IR_OP(load64):
      opcode = W_OP(i64_load);
      type = kWasmTypeI64;
      break;
    case IR_OP(loadf):
      opcode = W_OP(f32_load);
      type = kWasmTypeF32;
      break;
    case IR_OP(loadd):
      opcode = W_OP(f64_load);
      type = kWasmTypeF64;
      break;
    default:
      Fail(wasm, IROpcodeName(node->opcode));
      return NULL;
  }
  TargetInstruction* result = EmitLoad(wasm, opcode, base, offset, type);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerStore(Wasm32Generator* wasm, IRNode* node) {
  IRNode* address_node = node->inputs.value.p[0];
  if (IsLocalVariable(address_node)) {
    TargetInstruction* value = Materialize(wasm, node->inputs.value.p[1]);
    if (value == NULL) {
      return NULL;
    }
    TargetInstruction* result =
        EmitLocalVariableSet(wasm, address_node, value);
    if (result == NULL) {
      return NULL;
    }
    SetLoweredNode(node, result);
    return result;
  }
  TargetInstruction* base;
  int32_t offset;
  if (!GetAddressAndOffset(wasm, address_node, &base, &offset)) {
    return NULL;
  }
  TargetInstruction* value = Materialize(wasm, node->inputs.value.p[1]);
  if (value == NULL) {
    return NULL;
  }
  WasmValueType value_type = Wasm32InstructionType(value);
  Wasm32Opcode opcode;
  switch (node->opcode) {
    case IR_OP(store8):
      opcode = StoreOpcodeForType(value_type, 1);
      break;
    case IR_OP(store16):
      opcode = StoreOpcodeForType(value_type, 2);
      break;
    case IR_OP(store32):
    case IR_OP(storea):
      opcode = StoreOpcodeForType(value_type, 4);
      break;
    case IR_OP(store64):
      opcode = StoreOpcodeForType(value_type, 8);
      break;
    case IR_OP(storef):
      opcode = W_OP(f32_store);
      break;
    case IR_OP(stored):
      opcode = W_OP(f64_store);
      break;
    default:
      Fail(wasm, IROpcodeName(node->opcode));
      return NULL;
  }
  TargetInstruction* result = EmitStore(wasm, opcode, base, value, offset);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerAddressOf(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* base;
  int32_t offset;
  if (!GetAddressAndOffset(wasm, node->inputs.value.p[0], &base, &offset)) {
    return NULL;
  }
  TargetInstruction* result = base;
  if (offset != 0) {
    TargetInstruction* delta = EmitI32Constant(wasm, offset);
    TargetInstruction* sum = NewInstruction2(W_OP(i32_add), base, delta);
    Wasm32SetInstructionType(sum, kWasmTypeI32);
    TargetUpdateOperandUsers(sum);
    result = Emit(wasm, sum);
  }
  SetLoweredNode(node, result);
  return result;
}

// An inc or dec node is a read-modify-write on an address whose result is
// the new value.  The IR opcode names both the access width and whether the
// value is signed.
static TargetInstruction* LowerIncrement(Wasm32Generator* wasm, IRNode* node,
                                         bool is_increment) {
  Wasm32Opcode load;
  int store_size;
  WasmValueType type = kWasmTypeI32;
  switch (node->opcode) {
    case IR_OP(inc8):
    case IR_OP(dec8):
      load = W_OP(i32_load8_s);
      store_size = 1;
      break;
    case IR_OP(uinc8):
    case IR_OP(udec8):
      load = W_OP(i32_load8_u);
      store_size = 1;
      break;
    case IR_OP(inc16):
    case IR_OP(dec16):
      load = W_OP(i32_load16_s);
      store_size = 2;
      break;
    case IR_OP(uinc16):
    case IR_OP(udec16):
      load = W_OP(i32_load16_u);
      store_size = 2;
      break;
    case IR_OP(inc32):
    case IR_OP(dec32):
    case IR_OP(uinc32):
    case IR_OP(udec32):
    case IR_OP(inca):
    case IR_OP(deca):
      load = W_OP(i32_load);
      store_size = 4;
      break;
    case IR_OP(inc64):
    case IR_OP(dec64):
    case IR_OP(uinc64):
    case IR_OP(udec64):
      load = W_OP(i64_load);
      store_size = 8;
      type = kWasmTypeI64;
      break;
    case IR_OP(incf):
    case IR_OP(decf):
      load = W_OP(f32_load);
      store_size = 4;
      type = kWasmTypeF32;
      break;
    case IR_OP(incd):
    case IR_OP(decd):
      load = W_OP(f64_load);
      store_size = 8;
      type = kWasmTypeF64;
      break;
    default:
      Fail(wasm, IROpcodeName(node->opcode));
      return NULL;
  }

  IRNode* address_node = node->inputs.value.p[0];
  bool in_local = IsLocalVariable(address_node);
  TargetInstruction* base = NULL;
  int32_t offset = 0;
  TargetInstruction* old;
  if (in_local) {
    old = EmitLocalVariableGet(wasm, address_node);
  } else {
    if (!GetAddressAndOffset(wasm, address_node, &base, &offset)) {
      return NULL;
    }
    old = EmitLoad(wasm, load, base, offset, type);
  }

  TargetInstruction* amount = Materialize(wasm, node->inputs.value.p[1]);
  amount = Coerce(wasm, amount, type, /*is_unsigned=*/false);
  if (amount == NULL) {
    return NULL;
  }

  Wasm32Opcode operation;
  switch (type) {
    case kWasmTypeI64:
      operation = is_increment ? W_OP(i64_add) : W_OP(i64_sub);
      break;
    case kWasmTypeF32:
      operation = is_increment ? W_OP(f32_add) : W_OP(f32_sub);
      break;
    case kWasmTypeF64:
      operation = is_increment ? W_OP(f64_add) : W_OP(f64_sub);
      break;
    default:
      operation = is_increment ? W_OP(i32_add) : W_OP(i32_sub);
      break;
  }
  TargetInstruction* updated = NewInstruction2(operation, old, amount);
  Wasm32SetInstructionType(updated, type);
  TargetUpdateOperandUsers(updated);
  updated = Emit(wasm, updated);

  if (in_local) {
    EmitLocalVariableSet(wasm, address_node, updated);
  } else {
    EmitStore(wasm, StoreOpcodeForType(type, store_size), base, updated,
              offset);
  }
  SetLoweredNode(node, updated);
  return updated;
}

// Narrow a value to 'keep_bytes' significant bits with zeroes above them,
// then widen it to whatever the result type is.
static TargetInstruction* LowerZeroExtend(Wasm32Generator* wasm,
                                          IRNode* node) {
  IRNode* source = node->inputs.value.p[0];
  TargetInstruction* value = Materialize(wasm, source);
  if (value == NULL) {
    return NULL;
  }
  int source_size = source->type != NULL ? source->type->size : node->type->size;
  int keep_bytes =
      source_size < node->type->size ? source_size : node->type->size;

  WasmValueType have = Wasm32InstructionType(value);
  if (keep_bytes < WasmTypeSize(have)) {
    uint64_t mask = (UINT64_C(1) << (keep_bytes * 8)) - 1;
    bool wide = have == kWasmTypeI64;
    TargetInstruction* mask_value = Emit(
        wasm, NewInstruction1(wide ? W_OP(i64_const) : W_OP(i32_const),
                              GetIntConstant(wasm, NULL,
                                             wide ? kTargetType64Bit
                                                  : kTargetType32Bit,
                                             (int64_t)mask)));
    Wasm32SetInstructionType(mask_value, have);
    TargetInstruction* masked = NewInstruction2(
        wide ? W_OP(i64_and) : W_OP(i32_and), value, mask_value);
    Wasm32SetInstructionType(masked, have);
    TargetUpdateOperandUsers(masked);
    value = Emit(wasm, masked);
  }
  value = Coerce(wasm, value, Wasm32TypeForCType(node->type),
                 /*is_unsigned=*/true);
  SetLoweredNode(node, value);
  return value;
}

// Replicate the sign bit of the source's narrow width across the result.
static TargetInstruction* LowerSignExtend(Wasm32Generator* wasm,
                                          IRNode* node) {
  IRNode* source = node->inputs.value.p[0];
  TargetInstruction* value = Materialize(wasm, source);
  if (value == NULL) {
    return NULL;
  }
  int source_size = source->type != NULL ? source->type->size : node->type->size;
  WasmValueType have = Wasm32InstructionType(value);
  bool wide = have == kWasmTypeI64;

  Wasm32Opcode extend = kWasm32_num_opcodes;
  switch (source_size) {
    case 1:
      extend = wide ? W_OP(i64_extend8_s) : W_OP(i32_extend8_s);
      break;
    case 2:
      extend = wide ? W_OP(i64_extend16_s) : W_OP(i32_extend16_s);
      break;
    case 4:
      if (wide) {
        extend = W_OP(i64_extend32_s);
      }
      break;
    default:
      break;
  }
  if (extend != kWasm32_num_opcodes) {
    TargetInstruction* inst = NewInstruction1(extend, value);
    Wasm32SetInstructionType(inst, have);
    TargetUpdateOperandUsers(inst);
    value = Emit(wasm, inst);
  }
  value = Coerce(wasm, value, Wasm32TypeForCType(node->type),
                 /*is_unsigned=*/false);
  SetLoweredNode(node, value);
  return value;
}

static TargetInstruction* LowerAlign(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* value = Materialize(wasm, node->inputs.value.p[0]);
  if (value == NULL) {
    return NULL;
  }
  int64_t alignment = ((IRConstant*)node->inputs.value.p[1])->value.ivalue;
  WasmValueType type = Wasm32InstructionType(value);
  bool wide = type == kWasmTypeI64;
  TargetType constant_type = wide ? kTargetType64Bit : kTargetType32Bit;

  TargetInstruction* bias =
      Emit(wasm, NewInstruction1(wide ? W_OP(i64_const) : W_OP(i32_const),
                                 GetIntConstant(wasm, NULL, constant_type,
                                                alignment - 1)));
  Wasm32SetInstructionType(bias, type);
  TargetInstruction* sum =
      NewInstruction2(wide ? W_OP(i64_add) : W_OP(i32_add), value, bias);
  Wasm32SetInstructionType(sum, type);
  TargetUpdateOperandUsers(sum);
  sum = Emit(wasm, sum);

  TargetInstruction* mask =
      Emit(wasm, NewInstruction1(wide ? W_OP(i64_const) : W_OP(i32_const),
                                 GetIntConstant(wasm, NULL, constant_type,
                                                ~(alignment - 1))));
  Wasm32SetInstructionType(mask, type);
  TargetInstruction* result =
      NewInstruction2(wide ? W_OP(i64_and) : W_OP(i32_and), sum, mask);
  Wasm32SetInstructionType(result, type);
  TargetUpdateOperandUsers(result);
  result = Emit(wasm, result);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerConvert(Wasm32Generator* wasm, IRNode* node) {
  IRNode* source = node->inputs.value.p[0];
  TargetInstruction* value = Materialize(wasm, source);
  if (value == NULL) {
    return NULL;
  }
  bool is_unsigned =
      source->type != NULL && TypeIsUnsigned(source->type);
  value = Coerce(wasm, value, Wasm32TypeForCType(node->type), is_unsigned);
  SetLoweredNode(node, value);
  return value;
}

// The function type a call reaches, whether it names a function directly or
// goes through a pointer.
static TypeRecord* CalleeFunctionType(IRNode* call) {
  IRNode* callee = call->inputs.value.p[0];
  TypeRecord* type = IRIsStaticVariable(callee)
                         ? ((IRVariable*)callee)->symbol->type
                         : callee->type;
  while (type != NULL && TypeIsPointer(type)) {
    type = type->next;
  }
  return TypeIsFunction(type) ? type : NULL;
}

// How many of a call's arguments are ordinary wasm parameters.  Anything
// past them is variadic and travels in the buffer instead.  The hidden
// aggregate-result pointer counts, because it is passed like a parameter
// even though the prototype does not mention it.
static size_t NumFixedArguments(TypeRecord* function) {
  size_t fixed = TypeReturnedThroughHiddenPointer(function->next) ? 1 : 0;
  Vector* prototype = &function->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* argument = prototype->value.p[i];
    if (argument != NULL && argument->type != NULL) {
      fixed++;
    }
  }
  return fixed;
}

// The wasm signature a C function type describes.  An indirect call has to
// name one, and the run-time check compares it against the signature the
// callee was defined with, so this has to agree with what Wasm32Lower builds
// for a definition.  Always destruct the result, even on failure.
static bool BuildSignatureForType(TypeRecord* type,
                                  Wasm32Signature* signature) {
  VectorInit(&signature->param_types);
  signature->has_result = false;
  signature->result_type = kWasmTypeVoid;

  while (type != NULL && TypeIsPointer(type)) {
    type = type->next;
  }
  if (!TypeIsFunction(type)) {
    return false;
  }
  if (TypeReturnedThroughHiddenPointer(type->next)) {
    VectorAppend(&signature->param_types, (void*)(intptr_t)kWasmTypeI32);
  }
  Vector* prototype = &type->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* argument = prototype->value.p[i];
    if (argument == NULL || argument->type == NULL) {
      continue;
    }
    VectorAppend(&signature->param_types,
                 (void*)(intptr_t)Wasm32TypeForCType(argument->type));
  }
  if (type->info.function.varargs) {
    VectorAppend(&signature->param_types, (void*)(intptr_t)kWasmTypeI32);
  }
  if (type->next != NULL && !TypeIsVoid(type->next)) {
    signature->has_result = true;
    signature->result_type = Wasm32TypeForCType(type->next);
  }
  return true;
}

static void Wasm32SignatureDestruct(Wasm32Signature* signature) {
  VectorDestruct(&signature->param_types);
}

// The signature a particular call needs.  A function type written without a
// prototype describes no parameters at all, but the call still passes them,
// and wasm insists the signature account for everything on the stack.  The
// arguments are then the only description of the callee there is, which is
// also what the definition's own signature will have been built from.
static bool BuildSignatureForCall(TypeRecord* type, TargetInstruction** values,
                                  size_t num_arguments,
                                  Wasm32Signature* signature) {
  if (!BuildSignatureForType(type, signature)) {
    return false;
  }
  while (type != NULL && TypeIsPointer(type)) {
    type = type->next;
  }
  if (type->info.function.varargs) {
    // The extra arguments travel in a buffer, so the signature already
    // accounts for every value the call leaves on the stack.
    return true;
  }
  for (size_t i = signature->param_types.length; i < num_arguments; i++) {
    VectorAppend(&signature->param_types,
                 (void*)(intptr_t)Wasm32InstructionType(values[i]));
  }
  return true;
}

static TargetInstruction* LowerCall(Wasm32Generator* wasm, IRNode* node) {
  assert(node->inputs.length >= 1);
  size_t num_arguments = node->inputs.length - 1;

  // Compute every argument first.  Only once they are all in locals is it
  // safe to start pushing, because anything emitted between two pushes
  // would land in the middle of the argument list on the operand stack.
  TargetInstruction** values = NULL;
  if (num_arguments > 0) {
    values = malloc(num_arguments * sizeof(TargetInstruction*));
    for (size_t i = 0; i < num_arguments; i++) {
      IRNode* argument = node->inputs.value.p[i + 1];
      // A struct argument is passed as a pointer to the caller's copy; the
      // callee takes its own copy on entry, which is where the by-value
      // semantics come from.
      values[i] = Materialize(wasm, argument);
      if (TypeIsStructOrUnion(argument->type)) {
        values[i] = Coerce(wasm, values[i], kWasmTypeI32, /*is_unsigned=*/true);
      }
      if (values[i] == NULL) {
        free(values);
        return NULL;
      }
    }
  }

  // Naming a function directly calls it by index.  Anything else is a value
  // holding a table slot, which only call_indirect can reach, and that has
  // to state the signature it expects.
  IRNode* callee_node = node->inputs.value.p[0];
  bool direct = IsFunctionReference(callee_node);
  TargetInstruction* callee;
  int type_index = -1;
  if (direct) {
    callee = TargetGetSymbol(&wasm->base, callee_node,
                             ((IRVariable*)callee_node)->symbol);
    // A direct call does not name a signature, but if the callee turns out
    // to live in another object the import standing in for it needs one, and
    // the call site is the only place that knows what it should be.
    Wasm32Signature signature;
    if (BuildSignatureForCall(((IRVariable*)callee_node)->symbol->type, values,
                              num_arguments, &signature)) {
      type_index = Wasm32InternSignature(&signature);
    }
    Wasm32SignatureDestruct(&signature);
  } else {
    Wasm32Signature signature;
    if (!BuildSignatureForCall(callee_node->type, values, num_arguments,
                               &signature)) {
      Wasm32SignatureDestruct(&signature);
      Fail(wasm, "a call through a pointer with no usable prototype");
      free(values);
      return NULL;
    }
    type_index = Wasm32InternSignature(&signature);
    Wasm32SignatureDestruct(&signature);
    callee = Coerce(wasm, Materialize(wasm, callee_node), kWasmTypeI32,
                    /*is_unsigned=*/true);
    if (callee == NULL) {
      free(values);
      return NULL;
    }
  }

  // A variadic call passes its fixed arguments as wasm parameters and packs
  // the rest into a frame buffer, whose address becomes the last parameter.
  // That keeps the wasm signature fixed however many arguments there are.
  TypeRecord* callee_type = CalleeFunctionType(node);
  size_t fixed = num_arguments;
  bool variadic = callee_type != NULL && callee_type->info.function.varargs;
  TargetInstruction* buffer = NULL;
  if (variadic) {
    fixed = NumFixedArguments(callee_type);
    if (fixed > num_arguments) {
      fixed = num_arguments;
    }
    if (wasm->vararg_buffer < 0) {
      Fail(wasm, "a variadic call the frame has no buffer for");
      free(values);
      return NULL;
    }
    for (size_t i = fixed; i < num_arguments; i++) {
      WasmValueType type = Wasm32InstructionType(values[i]);
      Wasm32Opcode store = type == kWasmTypeI64   ? W_OP(i64_store)
                           : type == kWasmTypeF64 ? W_OP(f64_store)
                           : type == kWasmTypeF32 ? W_OP(f32_store)
                                                  : W_OP(i32_store);
      EmitStore(wasm, store, wasm->frame_pointer, values[i],
                wasm->vararg_buffer +
                    (int32_t)((i - fixed) * WASM32_VARARG_SLOT));
    }
    // The buffer's address has to be in a local before the pushes start, for
    // the same reason the arguments do.
    buffer = EmitFrameAddress(wasm, wasm->vararg_buffer);
  }

  for (size_t i = 0; i < fixed; i++) {
    TargetInstruction* push = NewInstruction1(W_OP(arg), values[i]);
    push->flags |= WASM32_FLAG_NO_RESULT;
    TargetUpdateOperandUsers(push);
    Emit(wasm, push);
  }
  free(values);

  if (buffer != NULL) {
    TargetInstruction* push = NewInstruction1(W_OP(arg), buffer);
    push->flags |= WASM32_FLAG_NO_RESULT;
    TargetUpdateOperandUsers(push);
    Emit(wasm, push);
  }

  TargetInstruction* call =
      NewInstruction1(direct ? W_OP(call) : W_OP(call_indirect), callee);
  call->addr = type_index;
  bool returns_value = node->type != NULL && !TypeIsVoid(node->type);
  if (returns_value) {
    Wasm32SetInstructionType(call, Wasm32TypeForCType(node->type));
  } else {
    call->flags |= WASM32_FLAG_NO_RESULT;
  }
  TargetUpdateOperandUsers(call);
  call = Emit(wasm, call);
  SetLoweredNode(node, call);
  return call;
}

// memset to zero and memcpy map onto the bulk memory instructions, which
// take their operands as (destination, value, length) and (destination,
// source, length).
static TargetInstruction* LowerMemoryZero(Wasm32Generator* wasm,
                                          IRNode* node) {
  // memzero names only what to clear; how much comes from the object's own
  // type.  A destination with no symbol behind it, such as an aggregate
  // return slot, carries the size on the memzero node instead.
  assert(node->inputs.length == 1);
  IRNode* destination_node = node->inputs.value.p[0];
  IRVariable* variable = (IRVariable*)destination_node;
  int64_t size = (IRIsVariable(destination_node) && variable->symbol != NULL)
                     ? variable->symbol->type->size
                     : (node->type != NULL ? node->type->size : 0);
  if (size <= 0) {
    return NULL;
  }
  TargetInstruction* destination = Materialize(wasm, destination_node);
  if (destination == NULL) {
    return NULL;
  }
  TargetInstruction* zero = EmitI32Constant(wasm, 0);
  TargetInstruction* fill =
      NewInstruction3(W_OP(memory_fill), destination, zero,
                      EmitI32Constant(wasm, size));
  fill->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(fill);
  TargetInstruction* result = Emit(wasm, fill);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerMemoryCopy(Wasm32Generator* wasm,
                                          IRNode* node) {
  TargetInstruction* destination = Materialize(wasm, node->inputs.value.p[0]);
  TargetInstruction* source = Materialize(wasm, node->inputs.value.p[1]);
  TargetInstruction* length = Materialize(wasm, node->inputs.value.p[2]);
  if (destination == NULL || source == NULL || length == NULL) {
    return NULL;
  }
  TargetInstruction* copy = NewInstruction3(
      W_OP(memory_copy), destination, source,
      Coerce(wasm, length, kWasmTypeI32, /*is_unsigned=*/true));
  copy->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(copy);
  TargetInstruction* result = Emit(wasm, copy);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerLabel(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* inst = Emit(wasm, NewInstruction(W_OP(label)));
  inst->flags |= WASM32_FLAG_NO_RESULT;
  node->data.ptr = inst;
  TargetApplyFixups(&wasm->base, node);
  return inst;
}

static TargetInstruction* LowerNamedLabel(Wasm32Generator* wasm,
                                          IRNode* node) {
  IRNamedLabel* n = (IRNamedLabel*)node;
  TargetInstruction* inst = Emit(wasm, TargetNewNamedLabel(n->name));
  inst->flags |= WASM32_FLAG_NO_RESULT;
  node->data.ptr = inst;
  return inst;
}

static TargetInstruction* LowerBranch(Wasm32Generator* wasm, IRNode* node) {
  assert(node->inputs.length == 1);
  IRNode* target_node = node->inputs.value.p[0];
  TargetInstruction* inst = Emit(wasm, NewInstruction(W_OP(br)));
  inst->flags |= WASM32_FLAG_NO_RESULT;
  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    VectorAppend(&wasm->base.fixups, NewBranchFixup(inst, target_node, 1));
  } else {
    inst->operand[1] = target;
  }
  return inst;
}

static TargetInstruction* LowerConditionalBranch(Wasm32Generator* wasm,
                                                 IRNode* node) {
  assert(node->inputs.length == 2);
  IRNode* expr = node->inputs.value.p[0];
  IRNode* target_node = node->inputs.value.p[1];

  TargetInstruction* condition = Materialize(wasm, expr);
  if (condition == NULL) {
    return NULL;
  }
  // bfalse branches when the value is zero; wasm only has "branch if
  // non-zero", so invert the condition first.
  if (node->opcode == IR_OP(bfalse)) {
    Wasm32Opcode eqz = Wasm32InstructionType(condition) == kWasmTypeI64
                           ? W_OP(i64_eqz)
                           : W_OP(i32_eqz);
    TargetInstruction* inverted = NewInstruction1(eqz, condition);
    Wasm32SetInstructionType(inverted, kWasmTypeI32);
    TargetUpdateOperandUsers(inverted);
    condition = Emit(wasm, inverted);
  }

  TargetInstruction* inst = NewInstruction1(W_OP(br_if), condition);
  inst->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(inst);
  inst = Emit(wasm, inst);

  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    VectorAppend(&wasm->base.fixups, NewBranchFixup(inst, target_node, 1));
  } else {
    inst->operand[1] = target;
  }
  return inst;
}

// The one local this function's return value is parked in.  Every result
// node writes it and every return reads it, so it cannot belong to whichever
// of the two happens to be lowered first: which that is depends on how the
// blocks ended up ordered, and a return reached first would otherwise hand
// back nothing at all.
static TargetInstruction* ResultValue(Wasm32Generator* wasm) {
  if (wasm->result_value == NULL && wasm->signature.has_result) {
    TargetInstruction* slot = NewInstruction(W_OP(slot));
    Wasm32SetInstructionType(slot, wasm->signature.result_type);
    wasm->result_value = Emit(wasm, slot);
  }
  return wasm->result_value;
}

static TargetInstruction* LowerResult(Wasm32Generator* wasm, IRNode* node) {
  assert(node->inputs.length == 1);
  TargetInstruction* value = Materialize(wasm, node->inputs.value.p[0]);
  if (value == NULL) {
    return NULL;
  }
  if (!wasm->signature.has_result) {
    // A function whose declared type promised no result but that returns one
    // regardless.  Its callers read the same declared type, so this is the
    // one place the shape of the signature can still be learned.
    wasm->signature.has_result = true;
    wasm->signature.result_type = Wasm32InstructionType(value);
  }
  TargetInstruction* slot = ResultValue(wasm);
  TypeRecord* return_type = compiler->current_function->next;
  value = Coerce(wasm, value, Wasm32InstructionType(slot),
                 return_type != NULL && TypeIsUnsigned(return_type));
  if (value == NULL) {
    return NULL;
  }

  // The value has to survive until the function's return, which may be
  // several blocks away.
  TargetInstruction* mov = NewInstruction1(W_OP(mov), value);
  Wasm32SetInstructionType(mov, Wasm32InstructionType(slot));
  mov->dest = slot;
  TargetUpdateOperandUsers(mov);
  Emit(wasm, mov);
  SetLoweredNode(node, slot);
  return slot;
}

static TargetInstruction* LowerReturn(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* inst = NewInstruction(W_OP(return));
  inst->flags |= WASM32_FLAG_NO_RESULT;
  TargetInstruction* result = ResultValue(wasm);
  if (result != NULL) {
    inst->operand[0] = result;
    TargetUpdateOperandUsers(inst);
  }
  return Emit(wasm, inst);
}

static TargetInstruction* LowerLocation(Wasm32Generator* wasm, IRNode* node) {
  IRLocation* loc = (IRLocation*)node;
  TargetInstruction* inst = Emit(wasm, TargetNewLocation(loc));
  inst->flags |= WASM32_FLAG_NO_RESULT;
  return inst;
}

// The all-ones value for the low 'width' bits, in a container of 'container'
// bits.  Shifting by the full width is undefined in C, so that case is split
// out rather than relying on it.
static uint64_t LowBitMask(int width, int container) {
  if (width >= container) {
    return ~(uint64_t)0;
  }
  return ((uint64_t)1 << width) - 1;
}

// A bit field lives inside a wider storage unit, so reading one is a shift
// and a mask.  Everything happens in the width wasm actually holds the unit
// in, since that is what decides where the sign bit is.
static TargetInstruction* LowerGetBitField(Wasm32Generator* wasm,
                                           IRNode* node) {
  WasmValueType type = Wasm32TypeForCType(node->type);
  bool wide = type == kWasmTypeI64;
  bool is_unsigned = TypeIsUnsigned(node->type);
  TargetInstruction* value = Coerce(
      wasm, Materialize(wasm, node->inputs.value.p[0]), type, is_unsigned);
  if (value == NULL) {
    return NULL;
  }
  int position = (int)IRIntConstValue(node->inputs.value.p[1]);
  int width = (int)IRIntConstValue(node->inputs.value.p[2]);
  int container = wide ? 64 : 32;

  TargetInstruction* result;
  if (is_unsigned) {
    TargetInstruction* shifted =
        EmitBinary(wasm, wide ? W_OP(i64_shr_u) : W_OP(i32_shr_u), type, value,
                   EmitIntConstant(wasm, type, position));
    result = EmitBinary(
        wasm, wide ? W_OP(i64_and) : W_OP(i32_and), type, shifted,
        EmitIntConstant(wasm, type, (int64_t)LowBitMask(width, container)));
  } else {
    // Push the field's top bit up to the sign bit, then bring the field back
    // down arithmetically so that the sign extends through the rest.
    TargetInstruction* high =
        EmitBinary(wasm, wide ? W_OP(i64_shl) : W_OP(i32_shl), type, value,
                   EmitIntConstant(wasm, type, container - (position + width)));
    result =
        EmitBinary(wasm, wide ? W_OP(i64_shr_s) : W_OP(i32_shr_s), type, high,
                   EmitIntConstant(wasm, type, container - width));
  }
  SetLoweredNode(node, result);
  return result;
}

// Writing a bit field replaces the field's bits in the storage unit and
// leaves every other bit of it alone.
static TargetInstruction* LowerSetBitField(Wasm32Generator* wasm,
                                           IRNode* node) {
  WasmValueType type = Wasm32TypeForCType(node->type);
  bool wide = type == kWasmTypeI64;
  TargetInstruction* output =
      Coerce(wasm, Materialize(wasm, node->inputs.value.p[0]), type, true);
  TargetInstruction* input =
      Coerce(wasm, Materialize(wasm, node->inputs.value.p[1]), type, true);
  if (output == NULL || input == NULL) {
    return NULL;
  }
  int position = (int)IRIntConstValue(node->inputs.value.p[2]);
  int width = (int)IRIntConstValue(node->inputs.value.p[3]);
  int container = wide ? 64 : 32;
  uint64_t mask = LowBitMask(width, container) << position;

  TargetInstruction* placed =
      EmitBinary(wasm, wide ? W_OP(i64_shl) : W_OP(i32_shl), type, input,
                 EmitIntConstant(wasm, type, position));
  TargetInstruction* field =
      EmitBinary(wasm, wide ? W_OP(i64_and) : W_OP(i32_and), type, placed,
                 EmitIntConstant(wasm, type, (int64_t)mask));
  TargetInstruction* rest =
      EmitBinary(wasm, wide ? W_OP(i64_and) : W_OP(i32_and), type, output,
                 EmitIntConstant(wasm, type, (int64_t)~mask));
  TargetInstruction* result = EmitBinary(
      wasm, wide ? W_OP(i64_or) : W_OP(i32_or), type, field, rest);
  SetLoweredNode(node, result);
  return result;
}

// Dynamic allocation off the shadow stack, which is what a variable-length
// array and alloca need.  The epilogue restores the stack pointer from the
// frame pointer, so a function that returns without a restoresp still gives
// the space back.
static TargetInstruction* LowerStackPointerOp(Wasm32Generator* wasm,
                                              IRNode* node) {
  switch (node->opcode) {
    case IR_OP(savesp): {
      // The saved value belongs to the temporary named as the input, so that
      // a later restoresp or a VLA reference can find it.
      TargetInstruction* saved =
          EmitGlobalGet(wasm, WASM32_STACK_POINTER_GLOBAL);
      SetLoweredNode(node->inputs.value.p[0], saved);
      return SetLoweredNode(node, saved);
    }
    case IR_OP(decsp): {
      TargetInstruction* size = Coerce(
          wasm, Materialize(wasm, node->inputs.value.p[0]), kWasmTypeI32,
          /*is_unsigned=*/true);
      if (size == NULL) {
        return NULL;
      }
      TargetInstruction* stack_pointer =
          EmitGlobalGet(wasm, WASM32_STACK_POINTER_GLOBAL);
      TargetInstruction* lowered = EmitBinary(wasm, W_OP(i32_sub),
                                              kWasmTypeI32, stack_pointer, size);
      // Keep the shadow stack 16-byte aligned, the same as the frame the
      // prologue hands out.
      TargetInstruction* aligned =
          EmitBinary(wasm, W_OP(i32_and), kWasmTypeI32, lowered,
                     EmitI32Constant(wasm, -16));
      EmitGlobalSet(wasm, WASM32_STACK_POINTER_GLOBAL, aligned);
      return SetLoweredNode(node, aligned);
    }
    case IR_OP(restoresp): {
      TargetInstruction* saved = Materialize(wasm, node->inputs.value.p[0]);
      if (saved == NULL) {
        return NULL;
      }
      EmitGlobalSet(wasm, WASM32_STACK_POINTER_GLOBAL, saved);
      return SetLoweredNode(node, saved);
    }
    default:
      return NULL;
  }
}

// A va_list is a bare pointer into the caller's buffer of variadic
// arguments.  va_start hands out the pointer the caller passed, va_arg reads
// through it and steps it on to the next slot.
static TargetInstruction* LowerVaStart(Wasm32Generator* wasm, IRNode* node) {
  if (!wasm->is_varargs) {
    Fail(wasm, "va_start in a function that is not variadic");
    return NULL;
  }
  TargetInstruction* base;
  int32_t offset;
  if (!GetAddressAndOffset(wasm, node->inputs.value.p[0], &base, &offset)) {
    return NULL;
  }
  TargetInstruction* buffer = wasm->params.value.p[wasm->num_params - 1];
  return SetLoweredNode(
      node, EmitStore(wasm, W_OP(i32_store), base, buffer, offset));
}

static TargetInstruction* LowerVaArg(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* base;
  int32_t offset;
  if (!GetAddressAndOffset(wasm, node->inputs.value.p[0], &base, &offset)) {
    return NULL;
  }
  if (TypeIsStructOrUnion(node->type)) {
    Fail(wasm, "va_arg of a struct");
    return NULL;
  }
  TargetInstruction* cursor =
      EmitLoad(wasm, W_OP(i32_load), base, offset, kWasmTypeI32);

  // Arguments reach the buffer already promoted, so a slot always holds a
  // whole value of the wasm type and never a narrower one.
  WasmValueType type = Wasm32TypeForCType(node->type);
  Wasm32Opcode load = type == kWasmTypeI64   ? W_OP(i64_load)
                      : type == kWasmTypeF64 ? W_OP(f64_load)
                      : type == kWasmTypeF32 ? W_OP(f32_load)
                                             : W_OP(i32_load);
  TargetInstruction* value = EmitLoad(wasm, load, cursor, 0, type);

  TargetInstruction* next =
      EmitBinary(wasm, W_OP(i32_add), kWasmTypeI32, cursor,
                 EmitI32Constant(wasm, WASM32_VARARG_SLOT));
  EmitStore(wasm, W_OP(i32_store), base, next, offset);
  return SetLoweredNode(node, value);
}

static TargetInstruction* LowerVaCopy(Wasm32Generator* wasm, IRNode* node) {
  TargetInstruction* destination;
  int32_t destination_offset;
  TargetInstruction* source;
  int32_t source_offset;
  if (!GetAddressAndOffset(wasm, node->inputs.value.p[0], &destination,
                           &destination_offset) ||
      !GetAddressAndOffset(wasm, node->inputs.value.p[1], &source,
                           &source_offset)) {
    return NULL;
  }
  TargetInstruction* value =
      EmitLoad(wasm, W_OP(i32_load), source, source_offset, kWasmTypeI32);
  return SetLoweredNode(node, EmitStore(wasm, W_OP(i32_store), destination,
                                        value, destination_offset));
}

static TargetInstruction* LowerIRNode(Wasm32Generator* wasm, IRNode* node) {
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  switch (node->opcode) {
    // Declarations of storage produce no code; their address is computed
    // where they are used.
    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    // A bare temporary is only a name for a value another node produces,
    // such as the stack pointer a savesp records.
    case IR_OP(tmp):
    case IR_OP(staticvar):
    case IR_OP(externvar):
    case IR_OP(nop):
    case IR_OP(leave):
    case IR_OP(observable_checkpoint):
    case last_ir_opcode:
      return NULL;

    case IR_OP(enter):
      LowerEnter(wasm);
      return NULL;

    case IR_OP(load8):
    case IR_OP(loadu8):
    case IR_OP(load16):
    case IR_OP(loadu16):
    case IR_OP(load32):
    case IR_OP(loadu32):
    case IR_OP(load64):
    case IR_OP(loada):
    case IR_OP(loadf):
    case IR_OP(loadd):
      return LowerLoad(wasm, node);

    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store32):
    case IR_OP(store64):
    case IR_OP(storea):
    case IR_OP(storef):
    case IR_OP(stored):
      return LowerStore(wasm, node);

    case IR_OP(addressof):
      return LowerAddressOf(wasm, node);

    case IR_OP(literalref):
      return LowerLiteralReference(wasm, node);

    case IR_OP(savesp):
    case IR_OP(decsp):
    case IR_OP(restoresp):
      return LowerStackPointerOp(wasm, node);

    case IR_OP(builtin_va_start):
      return LowerVaStart(wasm, node);

    case IR_OP(builtin_va_arg):
      return LowerVaArg(wasm, node);

    case IR_OP(builtin_va_end):
      // A va_list owns nothing, so there is nothing to release.
      return NULL;

    case IR_OP(builtin_va_copy):
      return LowerVaCopy(wasm, node);

    case IR_OP(structreturn):
      // Where the caller wants the result written: the hidden first
      // parameter, which this function also returns.
      if (!wasm->returns_struct) {
        Fail(wasm, "an aggregate result this function has no slot for");
        return NULL;
      }
      return SetLoweredNode(node, wasm->params.value.p[0]);

    case IR_OP(getbit):
      return LowerGetBitField(wasm, node);

    case IR_OP(setbit):
      return LowerSetBitField(wasm, node);

    case IR_OP(inc8):
    case IR_OP(inc16):
    case IR_OP(inc32):
    case IR_OP(inc64):
    case IR_OP(uinc8):
    case IR_OP(uinc16):
    case IR_OP(uinc32):
    case IR_OP(uinc64):
    case IR_OP(inca):
    case IR_OP(incf):
    case IR_OP(incd):
      return LowerIncrement(wasm, node, /*is_increment=*/true);

    case IR_OP(dec8):
    case IR_OP(dec16):
    case IR_OP(dec32):
    case IR_OP(dec64):
    case IR_OP(udec8):
    case IR_OP(udec16):
    case IR_OP(udec32):
    case IR_OP(udec64):
    case IR_OP(deca):
    case IR_OP(decf):
    case IR_OP(decd):
      return LowerIncrement(wasm, node, /*is_increment=*/false);

    case IR_OP(zeroextendi):
      return LowerZeroExtend(wasm, node);
    case IR_OP(signextendi):
      return LowerSignExtend(wasm, node);
    case IR_OP(aligni):
      return LowerAlign(wasm, node);

    case IR_OP(pusharg):
    case IR_OP(structarg):
      return SetLoweredNode(node,
                            Materialize(wasm, node->inputs.value.p[0]));

    case IR_OP(calla):
      return LowerCall(wasm, node);

    case IR_OP(memzero):
      return LowerMemoryZero(wasm, node);
    case IR_OP(memcpy):
      return LowerMemoryCopy(wasm, node);

    case IR_OP(cast):
    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2d):
    case IR_OP(d2f):
    case IR_OP(f2i):
    case IR_OP(d2i):
      return LowerConvert(wasm, node);

    case IR_OP(const8):
    case IR_OP(const16):
    case IR_OP(const32):
    case IR_OP(const64):
    case IR_OP(constf):
    case IR_OP(constd):
    case IR_OP(consta):
      return MaterializeConstant(wasm, node);

    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova):
      return LowerMove(wasm, node);

    case IR_OP(noti):
    case IR_OP(nota):
      return LowerNot(wasm, node);

    case IR_OP(negi):
      return LowerNegate(wasm, node);

    case IR_OP(onescomp):
      return LowerOnesComplement(wasm, node);

    case IR_OP(addi):
    case IR_OP(adda):
    case IR_OP(subi):
    case IR_OP(suba):
    case IR_OP(muli):
    case IR_OP(divi):
    case IR_OP(modi):
    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):
    case IR_OP(lsli):
    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(rotli):
    case IR_OP(rotri):
    case IR_OP(clzi):
    case IR_OP(ctzi):
    case IR_OP(popcounti):
    case IR_OP(addf):
    case IR_OP(subf):
    case IR_OP(mulf):
    case IR_OP(divf):
    case IR_OP(negf):
    case IR_OP(addd):
    case IR_OP(subd):
    case IR_OP(muld):
    case IR_OP(divd):
    case IR_OP(negd):
    case IR_OP(cmpeqi):
    case IR_OP(cmpnei):
    case IR_OP(cmplti):
    case IR_OP(cmplei):
    case IR_OP(cmpgti):
    case IR_OP(cmpgei):
    case IR_OP(cmpeqf):
    case IR_OP(cmpnef):
    case IR_OP(cmpltf):
    case IR_OP(cmplef):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgef):
    case IR_OP(cmpeqd):
    case IR_OP(cmpned):
    case IR_OP(cmpltd):
    case IR_OP(cmpled):
    case IR_OP(cmpgtd):
    case IR_OP(cmpged):
    case IR_OP(cmpeqa):
    case IR_OP(cmpnea):
    case IR_OP(cmplta):
    case IR_OP(cmplea):
    case IR_OP(cmpgta):
    case IR_OP(cmpgea):
      return LowerExpression(wasm, node);

    case IR_OP(label):
      return LowerLabel(wasm, node);
    case IR_OP(named_label):
      return LowerNamedLabel(wasm, node);
    case IR_OP(bra):
      return LowerBranch(wasm, node);
    case IR_OP(btrue):
    case IR_OP(bfalse):
      return LowerConditionalBranch(wasm, node);

    case IR_OP(resulti):
    case IR_OP(resulta):
    case IR_OP(resultf):
    case IR_OP(resultd):
      return LowerResult(wasm, node);

    case IR_OP(ret):
      return LowerReturn(wasm, node);

    case IR_OP(loc):
      return LowerLocation(wasm, node);

    default:
      Fail(wasm, IROpcodeName(node->opcode));
      return NULL;
  }
}

// The uses that read or write the whole of a variable and so want no address
// for it.  Everything else - a member or element access, an argument passed
// by reference, a pointer taken - has to name real storage.
static bool UseAccessesWholeVariable(IRNode* use) {
  switch (use->opcode) {
    case IR_OP(load8):
    case IR_OP(loadu8):
    case IR_OP(load16):
    case IR_OP(loadu16):
    case IR_OP(load32):
    case IR_OP(loadu32):
    case IR_OP(load64):
    case IR_OP(loada):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store32):
    case IR_OP(store64):
    case IR_OP(storea):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(inc8):
    case IR_OP(uinc8):
    case IR_OP(inc16):
    case IR_OP(uinc16):
    case IR_OP(inc32):
    case IR_OP(uinc32):
    case IR_OP(inc64):
    case IR_OP(uinc64):
    case IR_OP(inca):
    case IR_OP(incf):
    case IR_OP(incd):
    case IR_OP(dec8):
    case IR_OP(udec8):
    case IR_OP(dec16):
    case IR_OP(udec16):
    case IR_OP(dec32):
    case IR_OP(udec32):
    case IR_OP(dec64):
    case IR_OP(udec64):
    case IR_OP(deca):
    case IR_OP(decf):
    case IR_OP(decd):
      return true;
    default:
      return false;
  }
}

// True when a wasm local can stand in for the variable's storage: the value
// fits in one, and nothing in the function ever needs somewhere to point.
static bool CanKeepVariableInLocal(IRNode* node) {
  if (OptLevel0()) {
    // Keeping everything in the frame at -O0 leaves each variable somewhere a
    // debugger can find it, and keeps the plain path exercised.
    return false;
  }
  if (node->opcode != IR_OP(localvar) && node->opcode != IR_OP(tempvar) &&
      node->opcode != IR_OP(argument)) {
    return false;
  }
  if ((node->flags & kIRNrvoMarker) != 0) {
    // The caller's result buffer under a local's name, so it is memory by
    // definition.
    return false;
  }
  Symbol* symbol = ((IRVariable*)node)->symbol;
  if (symbol == NULL || symbol->type == NULL || symbol->flags.address_taken) {
    return false;
  }
  TypeRecord* type = symbol->type;
  if (!TypeIsPointer(type) && !TypeIsIntegral(type) &&
      !TypeIsFloatingPoint(type)) {
    return false;
  }
  if (TypeIsVolatile(type) || TypeIsAtomic(type) || TypeIsReference(type)) {
    return false;
  }
  if (type->size == 0) {
    TypeRecordCalculateSize(type);
  }
  if (type->size != 1 && type->size != 2 && type->size != 4 &&
      type->size != 8) {
    return false;
  }
  for (size_t i = 0; i < node->outputs.length; i++) {
    IRNode* use = node->outputs.value.p[i];
    if (!UseAccessesWholeVariable(use)) {
      return false;
    }
    // Only the first input of such a use is the thing being accessed; the
    // variable anywhere else is a value, which for a variable means its
    // address.
    if (use->inputs.length == 0 || use->inputs.value.p[0] != node) {
      return false;
    }
    for (size_t j = 1; j < use->inputs.length; j++) {
      if (use->inputs.value.p[j] == node) {
        return false;
      }
    }
  }
  return true;
}

// The wasm parameter a declared argument arrives in, or NULL when the symbol
// is not one of this function's parameters.
static TargetInstruction* ParameterFor(Wasm32Generator* wasm, Symbol* symbol) {
  Vector* prototype = &compiler->current_function->info.function.prototype;
  size_t first = wasm->returns_struct ? 1 : 0;
  for (size_t i = 0; i < prototype->length; i++) {
    if (prototype->value.p[i] != symbol) {
      continue;
    }
    return i + first < wasm->params.length ? wasm->params.value.p[i + first]
                                           : NULL;
  }
  return NULL;
}

// The local a promoted variable lives in.
static TargetInstruction* MakeVariableSlot(Wasm32Generator* wasm,
                                           IRNode* node) {
  Symbol* symbol = ((IRVariable*)node)->symbol;
  WasmValueType type = Wasm32TypeForCType(symbol->type);
  TargetInstruction* param = ParameterFor(wasm, symbol);
  if (param == NULL && node->opcode == IR_OP(argument)) {
    // Nothing would ever write the local, so the argument would read as zero.
    Fail(wasm, "an argument with no wasm parameter to arrive in");
  }
  if (param != NULL && symbol->type->size >= WasmTypeSize(type)) {
    // The argument already arrived in a local holding exactly what the
    // variable should hold, so the parameter is the variable.
    return param;
  }

  TargetInstruction* slot = NewInstruction(W_OP(slot));
  Wasm32SetInstructionType(slot, type);
  Emit(wasm, slot);
  if (param != NULL) {
    // A parameter narrower than its local arrives with whatever the caller
    // left in the spare bits.  Trimming it once on entry is what lets every
    // later read take the local exactly as it stands.
    TargetInstruction* set = NewInstruction1(
        W_OP(local_set), NarrowToVariableWidth(wasm, param, symbol->type));
    set->dest = slot;
    set->flags |= WASM32_FLAG_NO_RESULT;
    TargetUpdateOperandUsers(set);
    Emit(wasm, set);
  }
  return slot;
}

static bool VectorHolds(Vector* vector, void* value) {
  for (size_t i = 0; i < vector->length; i++) {
    if (vector->value.p[i] == value) {
      return true;
    }
  }
  return false;
}

void Wasm32Lower(Wasm32Generator* wasm, Generator* gen) {
  // Build the wasm signature from the C prototype.  A struct is too big to
  // be a wasm value, so both a struct parameter and a struct result travel
  // as an i32 pointer into linear memory.
  TypeRecord* return_type = compiler->current_function->next;
  wasm->returns_struct = TypeReturnedThroughHiddenPointer(return_type);

  // Settle which variables can live in a wasm local before laying out the
  // frame, because a parameter among them wants neither frame space nor the
  // copy into it that every other parameter gets on entry.
  Vector arguments_in_locals;
  VectorInit(&arguments_in_locals);
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    if (!CanKeepVariableInLocal(entry->pooled)) {
      continue;
    }
    // Which local it is cannot be settled until the parameters exist, so for
    // now the tag alone marks the variable.
    entry->pooled->data.ivalue = WASM32_LOCAL_VAR;
    if (entry->pooled->opcode == IR_OP(argument)) {
      VectorAppend(&arguments_in_locals, ((IRVariable*)entry->pooled)->symbol);
    }
  }

  if (wasm->returns_struct) {
    // The hidden destination pointer comes first and is also what the
    // function hands back, so a caller can use the call's value directly.
    VectorAppend(&wasm->signature.param_types, (void*)(intptr_t)kWasmTypeI32);
  }

  Vector* prototype = &compiler->current_function->info.function.prototype;
  int32_t var_offset = 0;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* arg = prototype->value.p[i];
    if (arg == NULL || arg->type == NULL) {
      continue;
    }
    bool by_reference = TypeIsStructOrUnion(arg->type);
    VectorAppend(&wasm->signature.param_types,
                 (void*)(intptr_t)(by_reference ? kWasmTypeI32
                                                : Wasm32TypeForCType(arg->type)));

    if (VectorHolds(&arguments_in_locals, arg)) {
      // The parameter stays in a wasm local, so it wants no frame slot and
      // nothing copied into one.
      arg->stack_offset = -1;
      continue;
    }

    // Every other parameter gets a frame slot, because the IR reads
    // parameters through the same loads and stores it uses for locals.  A
    // struct's slot holds the whole struct, not the pointer to it.
    int32_t alignment = TypeRecordAlignment(arg->type);
    var_offset = (var_offset + alignment - 1) & ~(alignment - 1);
    arg->stack_offset = var_offset;
    var_offset += arg->type->size;
  }

  // A variadic function's extra arguments arrive in a buffer the caller owns
  // rather than as wasm parameters, since a wasm signature is fixed.  One
  // more parameter points at that buffer and is what va_start hands out.
  wasm->is_varargs = compiler->current_function->info.function.varargs;
  if (wasm->is_varargs) {
    VectorAppend(&wasm->signature.param_types, (void*)(intptr_t)kWasmTypeI32);
  }

  // C lets main be written with none of its arguments, or two, or three, and
  // leaves the caller free to pass all three regardless.  Elsewhere the
  // surplus sits in registers nobody reads, but a wasm call has to agree
  // with the callee's signature exactly, so main is given the full one and
  // the arguments it did not ask for go unread.
  if (strcmp(wasm->base.function_name.value, "main") == 0 &&
      !wasm->is_varargs && !wasm->returns_struct) {
    while (wasm->signature.param_types.length < 3) {
      VectorAppend(&wasm->signature.param_types, (void*)(intptr_t)kWasmTypeI32);
    }
  }
  wasm->num_params = (int)wasm->signature.param_types.length;

  for (int i = 0; i < wasm->num_params; i++) {
    TargetInstruction* param = NewInstruction(W_OP(param));
    TargetRegister* reg = malloc(sizeof(TargetRegister));
    TargetRegisterInit(reg, i);
    reg->owner = param;
    param->reg = reg;
    Wasm32SetInstructionType(
        param,
        (WasmValueType)(intptr_t)wasm->signature.param_types.value.p[i]);
    VectorAppend(&wasm->params, param);
  }

  if (wasm->returns_struct) {
    // Returning the destination pointer needs no separate result local: the
    // parameter already holds it and nothing writes to that local.
    wasm->signature.has_result = true;
    wasm->signature.result_type = kWasmTypeI32;
    wasm->result_value = wasm->params.value.p[0];
  } else if (return_type != NULL && !TypeIsVoid(return_type)) {
    wasm->signature.has_result = true;
    wasm->signature.result_type = Wasm32TypeForCType(return_type);
  }

  // The parameters exist now, so each variable that is staying out of the
  // frame can be given the local it lives in.
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    if (!IsLocalVariable(entry->pooled)) {
      continue;
    }
    entry->pooled->data.ivalue =
        WASM32_LOCAL_VAR | (int32_t)wasm->local_variables.length;
    VectorAppend(&wasm->local_variables, MakeVariableSlot(wasm, entry->pooled));
  }
  VectorDestruct(&arguments_in_locals);

  // Give every variable that is left a shadow-stack slot.
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    if (IsLocalVariable(entry->pooled)) {
      continue;
    }
    if (entry->pooled->opcode == IR_OP(localvar) ||
        entry->pooled->opcode == IR_OP(tempvar)) {
      if ((entry->pooled->flags & kIRNrvoMarker) != 0) {
        continue;
      }
      if (IsFunctionReference(entry->pooled)) {
        // A function declared inside a block.  It is a name for code, not
        // storage, so there is nothing to make room for.
        continue;
      }
      TypeRecord* type = entry->value.symbol->type;
      if (TypeIsVLA(type)) {
        // A variable-length array has no fixed slot.  Its storage comes off
        // the shadow stack when the declaration is reached, and its address
        // is whatever the savesp at that point recorded.
        continue;
      }
      if (type->size == 0) {
        TypeRecordCalculateSize(type);
      }
      int32_t size = type->size;
      int32_t alignment = TypeRecordAlignment(type);
      assert(size > 0);
      assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
      var_offset = (var_offset + alignment - 1) & ~(alignment - 1);
      entry->pooled->data.ivalue = var_offset;
      var_offset += size;
    } else if (entry->pooled->opcode == IR_OP(argument)) {
      entry->pooled->data.ivalue = entry->value.symbol->stack_offset;
    }
  }

  // Reserve one buffer for outgoing variadic arguments, sized for the
  // largest variadic call this function makes.  A single shared buffer is
  // enough because every argument of a call is evaluated before any of them
  // is stored, so a nested variadic call is finished with the buffer before
  // the outer one starts filling it.
  int32_t vararg_bytes = 0;
  bool moves_stack_pointer = false;
  for (IRNode* call = GeneratorFirstInstruction(gen); call != NULL;
       call = IRNext(call)) {
    if (call->opcode == IR_OP(decsp)) {
      moves_stack_pointer = true;
      continue;
    }
    if (call->opcode != IR_OP(calla) || call->inputs.length < 1) {
      continue;
    }
    TypeRecord* callee = CalleeFunctionType(call);
    if (callee == NULL || !callee->info.function.varargs) {
      continue;
    }
    size_t fixed = NumFixedArguments(callee);
    size_t passed = call->inputs.length - 1;
    size_t extra = passed > fixed ? passed - fixed : 0;
    // A call that passes nothing variadic still hands over the buffer's
    // address, because that is what the signature says and what the callee's
    // va_start will take, so it too needs somewhere real to point.
    int32_t bytes =
        (int32_t)((extra == 0 ? 1 : extra) * WASM32_VARARG_SLOT);
    if (bytes > vararg_bytes) {
      vararg_bytes = bytes;
    }
  }
  if (vararg_bytes > 0) {
    var_offset = (var_offset + WASM32_VARARG_SLOT - 1) & ~(WASM32_VARARG_SLOT - 1);
    wasm->vararg_buffer = var_offset;
    var_offset += vararg_bytes;
  }

  // A function that allocates off the shadow stack must have a frame even if
  // it has nothing else to put in one, because it is the epilogue's restore
  // of the stack pointer that gives the space back on every exit path.
  if (moves_stack_pointer && var_offset == 0) {
    var_offset = 16;
  }

  wasm->base.stack_frame_size = (var_offset + 15) & ~15;

  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(wasm, node);
    node = IRNext(node);
  }

  TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
  while (inst != NULL) {
    TargetInstruction* next = TargetNext(inst);
    if (inst->opcode == (TargetOpcode)W_OP(return)) {
      EmitEpilogueBefore(wasm, inst);
    }
    inst = next;
  }

  if (compiler->print_back_end) {
    Wasm32Print(wasm);
  }
}

void Wasm32Print(Wasm32Generator* wasm) {
  TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, Wasm32OpcodeName, stdout);
    inst = TargetNext(inst);
  }
}
