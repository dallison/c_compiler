//
//  wasm32_module.c
//  c_compiler
//

#include "wasm32_module.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "wasm32_data.h"
#include "wasm32_reg_alloc.h"

// The object under construction.  There is one translation unit in flight at
// a time, so a single object serves the whole run and is torn down once it
// has been written.
static Wasm32ObjectFile current_object;
static bool current_object_ready;

Wasm32ObjectFile* Wasm32CurrentObject(void) {
  if (!current_object_ready) {
    Wasm32ObjectFileInit(&current_object, "");
    current_object_ready = true;
  }
  return &current_object;
}

void Wasm32ResetCurrentObject(void) {
  if (current_object_ready) {
    Wasm32ObjectFileDestruct(&current_object);
    current_object_ready = false;
  }
}

// What one function body's encoding needs to reach: the object, for the
// symbols it names, and the list its relocations go on.
typedef struct {
  Wasm32ObjectFile* object;
  Vector* relocs;
} Wasm32Encoder;

// Instruction encoding.

static void WriteOpcode(Buffer* buf, Wasm32Opcode opcode) {
  int encoding = Wasm32OpcodeEncoding(opcode);
  assert(encoding != WASM_NO_ENCODING);
  if ((encoding & WASM_PREFIX_FC) != 0) {
    BufferAppendByte(buf, (char)0xFC);
    WasmWriteULEB128(buf, encoding & ~WASM_PREFIX_FC);
  } else if ((encoding & WASM_PREFIX_FD) != 0) {
    BufferAppendByte(buf, (char)0xFD);
    WasmWriteULEB128(buf, encoding & ~WASM_PREFIX_FD);
  } else {
    BufferAppendByte(buf, (char)encoding);
  }
}

static int64_t ConstantIntValue(TargetInstruction* inst) {
  return ((TargetConstant*)inst)->value.ivalue;
}

static double ConstantFloatValue(TargetInstruction* inst) {
  return ((TargetConstant*)inst)->value.dvalue;
}

static void WriteF32(Buffer* buf, float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  BufferAppendWordLE(buf, bits);
}

static void WriteF64(Buffer* buf, double value) {
  uint64_t bits;
  memcpy(&bits, &value, sizeof(bits));
  BufferAppendLongLE(buf, bits);
}

static void WriteLocalGet(Buffer* buf, int index) {
  WriteOpcode(buf, W_OP(local_get));
  WasmWriteULEB128(buf, (uint64_t)index);
}

static void WriteLocalSet(Buffer* buf, int index) {
  WriteOpcode(buf, W_OP(local_set));
  WasmWriteULEB128(buf, (uint64_t)index);
}

// Reserve five bytes for a value only the linker knows and record what
// belongs there.  The placeholder is a valid encoding of whatever is written
// now, so the object still validates before it is linked.
static void WritePlaceholder(Wasm32Encoder* encoder, Buffer* buf, uint8_t type,
                             uint32_t symbol_index, int32_t addend,
                             uint32_t provisional) {
  VectorAppend(encoder->relocs, Wasm32NewReloc(type, (uint32_t)buf->length,
                                               symbol_index, addend));
  WasmWritePaddedU32(buf, provisional);
}

static uint32_t SymbolIndex(Wasm32Encoder* encoder, uint8_t kind,
                            const char* name) {
  Wasm32Symbol* symbol = Wasm32ObjectSymbol(encoder->object, kind, name);
  return (uint32_t)Wasm32ObjectSymbolIndex(encoder->object, symbol);
}

// The local an instruction's value ends up in.  An instruction with a 'dest'
// writes into that instruction's local instead of one of its own, which is
// how several IR result nodes share one return slot.
static int ResultLocal(TargetInstruction* inst) {
  if (inst->dest != NULL) {
    return Wasm32LocalIndex(inst->dest);
  }
  return Wasm32LocalIndex(inst);
}

static bool IsSkipped(TargetInstruction* inst) {
  if ((inst->flags & TARGET_INST_DEAD) != 0) {
    return true;
  }
  if (Wasm32IsPseudo(inst)) {
    return true;
  }
  switch ((Wasm32Opcode)inst->opcode) {
    // Labels carry no code of their own; the stackifier turns the control
    // flow they anchor into block structure.
    case W_OP(label):
    case W_OP(named_label):
      return true;
    default:
      return false;
  }
}

// Push the values this instruction consumes, in operand order.
static void WriteStackOperands(Buffer* buf, TargetInstruction* inst) {
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* operand = inst->operand[i];
    if (operand == NULL || Wasm32OperandIsImmediate(inst, i)) {
      continue;
    }
    int local = Wasm32LocalIndex(operand);
    assert(local >= 0 && "operand has no local assigned");
    WriteLocalGet(buf, local);
  }
}

// The alignment hint a memory access carries, as a log2 exponent.  Using the
// natural alignment of the access width is always safe: wasm treats the hint
// as advisory and never traps on a misaligned address.
static int MemoryAccessAlignment(Wasm32Opcode opcode) {
  switch (opcode) {
    case W_OP(i32_load8_s):
    case W_OP(i32_load8_u):
    case W_OP(i64_load8_s):
    case W_OP(i64_load8_u):
    case W_OP(i32_store8):
    case W_OP(i64_store8):
      return 0;
    case W_OP(i32_load16_s):
    case W_OP(i32_load16_u):
    case W_OP(i64_load16_s):
    case W_OP(i64_load16_u):
    case W_OP(i32_store16):
    case W_OP(i64_store16):
      return 1;
    case W_OP(i64_load):
    case W_OP(f64_load):
    case W_OP(i64_store):
    case W_OP(f64_store):
      return 3;
    case W_OP(v128_load):
    case W_OP(v128_store):
      return 4;
    default:
      return 2;
  }
}

static bool IsMemoryAccess(Wasm32Opcode opcode) {
  switch (opcode) {
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
    case W_OP(i32_store):
    case W_OP(i64_store):
    case W_OP(f32_store):
    case W_OP(f64_store):
    case W_OP(i32_store8):
    case W_OP(i32_store16):
    case W_OP(i64_store8):
    case W_OP(i64_store16):
    case W_OP(i64_store32):
    case W_OP(v128_load):
    case W_OP(v128_store):
      return true;
    default:
      return false;
  }
}

// A memarg is the alignment hint followed by the static byte offset.  Loads
// keep the offset in operand[1] and stores in operand[2].
static void WriteMemoryArgument(Buffer* buf, TargetInstruction* inst,
                                Wasm32Opcode opcode) {
  bool is_store = inst->operand[2] != NULL;
  TargetInstruction* offset = inst->operand[is_store ? 2 : 1];
  WasmWriteULEB128(buf, (uint64_t)MemoryAccessAlignment(opcode));
  WasmWriteULEB128(buf,
                   offset == NULL ? 0 : (uint64_t)ConstantIntValue(offset));
}

static void WriteInstruction(Wasm32Encoder* encoder, Buffer* buf,
                             TargetInstruction* inst) {
  Wasm32Opcode opcode = (Wasm32Opcode)inst->opcode;

  // A move has no wasm instruction of its own; it copies one local to
  // another.
  if (opcode == W_OP(mov) || opcode == W_OP(movf) || opcode == W_OP(movd)) {
    WriteLocalGet(buf, Wasm32LocalIndex(inst->operand[0]));
    WriteLocalSet(buf, ResultLocal(inst));
    return;
  }

  // Pushing a call argument is just reading its local.
  if (opcode == W_OP(arg)) {
    WriteLocalGet(buf, Wasm32LocalIndex(inst->operand[0]));
    return;
  }

  WriteStackOperands(buf, inst);
  WriteOpcode(buf, opcode);

  // Immediates.
  switch (opcode) {
    case W_OP(i32_const):
      // An i32 literal is encoded as a signed 32-bit LEB128, so a value the
      // source wrote as unsigned has to be reinterpreted, not widened.
      WasmWriteSLEB128(buf, (int32_t)ConstantIntValue(inst->operand[0]));
      break;
    case W_OP(i64_const):
      WasmWriteSLEB128(buf, ConstantIntValue(inst->operand[0]));
      break;
    case W_OP(f32_const):
      WriteF32(buf, (float)ConstantFloatValue(inst->operand[0]));
      break;
    case W_OP(f64_const):
      WriteF64(buf, ConstantFloatValue(inst->operand[0]));
      break;
    case W_OP(literal_address): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      char name[256];
      WritePlaceholder(
          encoder, buf, R_WASM_MEMORY_ADDR_SLEB,
          SymbolIndex(encoder, WASM_SYMBOL_DATA,
                      Wasm32LiteralSymbolName(literal->literal_id, name,
                                              sizeof(name))),
          0, 0);
      break;
    }
    case W_OP(symbol_address): {
      TargetSymbol* symbol = (TargetSymbol*)inst->operand[0];
      char buffer[256];
      const char* name =
          TargetSymbolName(symbol->symbol, buffer, sizeof(buffer));
      WritePlaceholder(encoder, buf, R_WASM_MEMORY_ADDR_SLEB,
                       SymbolIndex(encoder, WASM_SYMBOL_DATA, name), 0, 0);
      break;
    }
    case W_OP(function_index): {
      TargetSymbol* symbol = (TargetSymbol*)inst->operand[0];
      char buffer[256];
      const char* name =
          TargetSymbolName(symbol->symbol, buffer, sizeof(buffer));
      uint32_t index = SymbolIndex(encoder, WASM_SYMBOL_FUNCTION, name);
      Wasm32Symbol* function = encoder->object->symbols.value.p[index];
      if ((function->flags & WASM_SYM_UNDEFINED) != 0 && inst->addr >= 0 &&
          function->type_index == WASM32_NO_TYPE) {
        function->type_index = (uint32_t)inst->addr;
      }
      WritePlaceholder(encoder, buf, R_WASM_TABLE_INDEX_SLEB, index, 0, 0);
      break;
    }
    case W_OP(call_indirect):
      WritePlaceholder(encoder, buf, R_WASM_TYPE_INDEX_LEB,
                       (uint32_t)inst->addr, 0, (uint32_t)inst->addr);
      WasmWriteULEB128(buf, 0);  // Table 0.
      break;
    case W_OP(br):
    case W_OP(br_if):
      WasmWriteULEB128(buf, (uint64_t)(inst->addr < 0 ? 0 : inst->addr));
      break;
    case W_OP(block):
    case W_OP(loop):
    case W_OP(if):
      BufferAppendByte(buf, (char)kWasmTypeVoid);
      break;
    case W_OP(try_table):
      encoder->object->has_tag = true;
      BufferAppendByte(buf, (char)kWasmTypeVoid);
      WasmWriteULEB128(buf, 1);   // One catch clause.
      BufferAppendByte(buf, 0x02);  // catch_all: tag payload is in memory.
      WasmWriteULEB128(buf, (uint64_t)(inst->addr < 0 ? 0 : inst->addr));
      break;
    case W_OP(throw):
      encoder->object->has_tag = true;
      WasmWriteULEB128(buf, 0);  // Tag 0.
      break;
    case W_OP(setjmp_cont):
      WasmWriteSLEB128(buf, inst->addr);
      break;
    case W_OP(global_get):
    case W_OP(global_set):
      // The only global is the shadow stack pointer, which the linker
      // supplies; the object imports it as global 0.
      WritePlaceholder(
          encoder, buf, R_WASM_GLOBAL_INDEX_LEB,
          SymbolIndex(encoder, WASM_SYMBOL_GLOBAL, WASM32_STACK_POINTER), 0,
          (uint32_t)ConstantIntValue(inst->operand[1]));
      break;
    case W_OP(local_set):
    case W_OP(local_tee):
      WasmWriteULEB128(buf, (uint64_t)Wasm32LocalIndex(inst->dest));
      break;
    case W_OP(call): {
      TargetSymbol* symbol = (TargetSymbol*)inst->operand[0];
      char buffer[256];
      // The cached full name is the authoritative one; the scratch buffer is
      // only used when the symbol has not been named yet.
      const char* name =
          TargetSymbolName(symbol->symbol, buffer, sizeof(buffer));
      Wasm32Symbol* callee =
          Wasm32ObjectSymbol(encoder->object, WASM_SYMBOL_FUNCTION, name);
      // The import standing in for a callee defined elsewhere is declared
      // with the signature this call expects, which is what lets the object
      // validate on its own.
      if ((callee->flags & WASM_SYM_UNDEFINED) != 0 && inst->addr >= 0) {
        callee->type_index = (uint32_t)inst->addr;
      }
      WritePlaceholder(
          encoder, buf, R_WASM_FUNCTION_INDEX_LEB,
          (uint32_t)Wasm32ObjectSymbolIndex(encoder->object, callee), 0, 0);
      break;
    }
    case W_OP(memory_fill):
      BufferAppendByte(buf, 0x00);
      break;
    case W_OP(memory_copy):
      BufferAppendByte(buf, 0x00);
      BufferAppendByte(buf, 0x00);
      break;
    case W_OP(br_table): {
      // The stackifier's dispatch table sends selector value k to depth k,
      // with block 0 as the default.  addr holds the number of entries.
      int count = inst->addr;
      WasmWriteULEB128(buf, (uint64_t)count);
      for (int i = 0; i < count; i++) {
        WasmWriteULEB128(buf, (uint64_t)i);
      }
      WasmWriteULEB128(buf, 0);
      break;
    }
    default:
      if (IsMemoryAccess(opcode)) {
        WriteMemoryArgument(buf, inst, opcode);
      }
      break;
  }

  if (Wasm32ProducesValue(inst)) {
    WriteLocalSet(buf, ResultLocal(inst));
  }
}

void Wasm32EncodeFunctionBody(Wasm32Generator* wasm, Wasm32ObjectFile* object,
                              Buffer* out, Vector* relocs) {
  Wasm32Encoder encoder = {object, relocs};

  // Local declarations, grouped by type in the order the indices assume.
  static const WasmValueType kTypeOrder[WASM32_NUM_VALUE_TYPES] = {
      kWasmTypeI32, kWasmTypeI64, kWasmTypeF32, kWasmTypeF64, kWasmTypeV128};
  int groups = 0;
  for (int i = 0; i < WASM32_NUM_VALUE_TYPES; i++) {
    if (wasm->num_locals[i] > 0) {
      groups++;
    }
  }
  WasmWriteULEB128(out, (uint64_t)groups);
  for (int i = 0; i < WASM32_NUM_VALUE_TYPES; i++) {
    if (wasm->num_locals[i] > 0) {
      WasmWriteULEB128(out, (uint64_t)wasm->num_locals[i]);
      BufferAppendByte(out, (char)kTypeOrder[i]);
    }
  }

  TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
  while (inst != NULL) {
    if (!IsSkipped(inst)) {
      WriteInstruction(&encoder, out, inst);
    }
    inst = TargetNext(inst);
  }

  // Falling off the end of a function that returns a value is only valid if
  // the code there is unreachable.  Every real return is an explicit
  // 'return', so mark the tail unreachable rather than trying to prove it.
  if (wasm->signature.has_result) {
    WriteOpcode(out, W_OP(unreachable));
  }
  WriteOpcode(out, W_OP(end));
}

// Signature interning.  Two functions share a type index when their encoded
// functype bytes match.

static void EncodeSignature(Wasm32Signature* signature, Buffer* out) {
  BufferAppendByte(out, (char)WASM_FUNCTYPE);
  WasmWriteULEB128(out, signature->param_types.length);
  for (size_t i = 0; i < signature->param_types.length; i++) {
    intptr_t type = (intptr_t)signature->param_types.value.p[i];
    BufferAppendByte(out, (char)type);
  }
  if (signature->has_result) {
    WasmWriteULEB128(out, 1);
    BufferAppendByte(out, (char)signature->result_type);
  } else {
    WasmWriteULEB128(out, 0);
  }
}

int Wasm32InternSignature(Wasm32Signature* signature) {
  Buffer encoding;
  BufferInit(&encoding);
  EncodeSignature(signature, &encoding);
  int index = Wasm32ObjectInternType(Wasm32CurrentObject(), &encoding);
  BufferDestruct(&encoding);
  return index;
}

// Object assembly.

// Assign the index space positions the object's own encoding refers to:
// imported functions first, then the ones defined here.
static void AssignFunctionIndices(Wasm32ObjectFile* object) {
  uint32_t next = 0;
  for (size_t i = 0; i < object->symbols.length; i++) {
    Wasm32Symbol* symbol = object->symbols.value.p[i];
    if (symbol->kind == WASM_SYMBOL_FUNCTION &&
        (symbol->flags & WASM_SYM_UNDEFINED) != 0) {
      symbol->index = next++;
      VectorAppend(&object->imported_functions, symbol);
    }
  }
  for (size_t i = 0; i < object->functions.length; i++) {
    Wasm32Function* function = object->functions.value.p[i];
    function->symbol->index = next++;
  }
}

// Fill in the placeholders whose values this object already knows, so that
// it is a module that validates and that wasm-objdump can make sense of.
// The rest stay zero until the linker gets to them.
static void PatchLocalIndices(Wasm32ObjectFile* object) {
  for (size_t i = 0; i < object->functions.length; i++) {
    Wasm32Function* function = object->functions.value.p[i];
    for (size_t j = 0; j < function->relocs.length; j++) {
      Wasm32Reloc* reloc = function->relocs.value.p[j];
      if (reloc->type != R_WASM_FUNCTION_INDEX_LEB) {
        continue;
      }
      Wasm32Symbol* symbol = object->symbols.value.p[reloc->index];
      WasmPatchPaddedU32(&function->body, reloc->offset, symbol->index);
    }
  }
}

// A call site gives the import it reaches a signature.  One that is only
// ever named as an address has no call site to learn from, and since a table
// slot says nothing about the shape of what sits in it, any signature will
// do; the linker replaces the import with the definition either way.
static void GiveImportsSignatures(Wasm32ObjectFile* object) {
  int fallback = -1;
  for (size_t i = 0; i < object->imported_functions.length; i++) {
    Wasm32Symbol* symbol = object->imported_functions.value.p[i];
    if (symbol->type_index != WASM32_NO_TYPE) {
      continue;
    }
    if (fallback < 0) {
      Buffer encoding;
      BufferInit(&encoding);
      BufferAppendByte(&encoding, (char)WASM_FUNCTYPE);
      WasmWriteULEB128(&encoding, 0);
      WasmWriteULEB128(&encoding, 0);
      fallback = Wasm32ObjectInternType(object, &encoding);
      BufferDestruct(&encoding);
    }
    symbol->type_index = (uint32_t)fallback;
  }
}

bool Wasm32WriteObject(String* filename) {
  Wasm32ObjectFile* object = Wasm32CurrentObject();
  bool ok = true;

  for (size_t i = 0; i < compiler->functions.length; i++) {
    Wasm32Generator* wasm = compiler->functions.value.p[i];
    if (wasm->failed) {
      // Lowering already explained what it could not translate.  Writing the
      // object anyway would produce something that fails far less usefully.
      Wasm32ResetCurrentObject();
      return false;
    }
  }

  // Define every function before encoding any of them, so that a call to one
  // that appears later still lands on the symbol it will define.
  for (size_t i = 0; i < compiler->functions.length; i++) {
    Wasm32Generator* wasm = compiler->functions.value.p[i];
    const char* name = wasm->base.function_name.value;
    Wasm32Symbol* symbol =
        Wasm32ObjectSymbol(object, WASM_SYMBOL_FUNCTION, name);
    symbol->flags = wasm->base.is_global ? 0 : WASM_SYM_BINDING_LOCAL;

    Wasm32Function* function = malloc(sizeof(Wasm32Function));
    function->name = strdup(name);
    function->type_index = (uint32_t)Wasm32InternSignature(&wasm->signature);
    function->symbol = symbol;
    BufferInit(&function->body);
    VectorInit(&function->relocs);
    VectorAppend(&object->functions, function);
  }

  if (!Wasm32BuildDataSegments(object)) {
    ok = false;
  }

  for (size_t i = 0; i < compiler->functions.length; i++) {
    Wasm32Generator* wasm = compiler->functions.value.p[i];
    Wasm32Function* function = object->functions.value.p[i];
    Wasm32EncodeFunctionBody(wasm, object, &function->body, &function->relocs);
  }

  // The stack pointer is a global every object shares.
  Wasm32Symbol* stack_pointer =
      Wasm32ObjectSymbol(object, WASM_SYMBOL_GLOBAL, WASM32_STACK_POINTER);
  stack_pointer->flags = WASM_SYM_UNDEFINED;
  stack_pointer->index = 0;  // The only global an object imports.

  AssignFunctionIndices(object);
  GiveImportsSignatures(object);
  PatchLocalIndices(object);

  ok = ok && Wasm32WriteObjectFile(object, filename);
  Wasm32ResetCurrentObject();
  return ok;
}
