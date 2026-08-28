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
#include "wasm32_reg_alloc.h"

#define WASM_SECTION_TYPE 1
#define WASM_SECTION_IMPORT 2
#define WASM_SECTION_FUNCTION 3
#define WASM_SECTION_TABLE 4
#define WASM_SECTION_MEMORY 5
#define WASM_SECTION_GLOBAL 6
#define WASM_SECTION_EXPORT 7
#define WASM_SECTION_START 8
#define WASM_SECTION_ELEMENT 9
#define WASM_SECTION_CODE 10
#define WASM_SECTION_DATA 11

#define WASM_EXTERN_FUNC 0
#define WASM_EXTERN_TABLE 1
#define WASM_EXTERN_MEMORY 2
#define WASM_EXTERN_GLOBAL 3

#define WASM_FUNCTYPE 0x60

void WasmWriteULEB128(Buffer* buf, uint64_t value) {
  do {
    uint8_t byte = value & 0x7F;
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }
    BufferAppendByte(buf, (char)byte);
  } while (value != 0);
}

void WasmWriteSLEB128(Buffer* buf, int64_t value) {
  bool more = true;
  while (more) {
    uint8_t byte = value & 0x7F;
    value >>= 7;  // Arithmetic shift keeps the sign.
    bool sign_bit_set = (byte & 0x40) != 0;
    if ((value == 0 && !sign_bit_set) || (value == -1 && sign_bit_set)) {
      more = false;
    } else {
      byte |= 0x80;
    }
    BufferAppendByte(buf, (char)byte);
  }
}

void WasmWritePaddedU32(Buffer* buf, uint32_t value) {
  for (int i = 0; i < 5; i++) {
    uint8_t byte = value & 0x7F;
    value >>= 7;
    if (i < 4) {
      byte |= 0x80;
    }
    BufferAppendByte(buf, (char)byte);
  }
}

void WasmPatchPaddedU32(Buffer* buf, size_t offset, uint32_t value) {
  assert(offset + 5 <= buf->length);
  for (int i = 0; i < 5; i++) {
    uint8_t byte = value & 0x7F;
    value >>= 7;
    if (i < 4) {
      byte |= 0x80;
    }
    buf->value[offset + i] = (char)byte;
  }
}

static void WriteName(Buffer* buf, const char* name) {
  size_t length = strlen(name);
  WasmWriteULEB128(buf, length);
  BufferAppend(buf, (char*)name, length);
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

// Append 'body' to 'out' as a section with the given id.
static void WriteSection(Buffer* out, int id, Buffer* body) {
  if (body->length == 0) {
    return;
  }
  BufferAppendByte(out, (char)id);
  WasmWriteULEB128(out, body->length);
  BufferAppend(out, body->value, body->length);
}

// Instruction encoding.

static void WriteOpcode(Buffer* buf, Wasm32Opcode opcode) {
  int encoding = Wasm32OpcodeEncoding(opcode);
  assert(encoding != WASM_NO_ENCODING);
  if ((encoding & WASM_PREFIX_FC) != 0) {
    BufferAppendByte(buf, (char)0xFC);
    WasmWriteULEB128(buf, encoding & ~WASM_PREFIX_FC);
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

static void WriteLocalGet(Buffer* buf, int index) {
  WriteOpcode(buf, W_OP(local_get));
  WasmWriteULEB128(buf, (uint64_t)index);
}

static void WriteLocalSet(Buffer* buf, int index) {
  WriteOpcode(buf, W_OP(local_set));
  WasmWriteULEB128(buf, (uint64_t)index);
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

static void WriteInstruction(Wasm32Generator* wasm, Wasm32DataLayout* layout,
                             Buffer* buf, TargetInstruction* inst) {
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
      WasmWriteSLEB128(
          buf, (int32_t)Wasm32LiteralAddress(layout, literal->literal_id));
      break;
    }
    case W_OP(symbol_address): {
      TargetSymbol* symbol = (TargetSymbol*)inst->operand[0];
      char name[256];
      WasmWriteSLEB128(
          buf, (int32_t)Wasm32SymbolAddress(
                   layout, TargetSymbolName(symbol->symbol, name,
                                            sizeof(name))));
      break;
    }
    case W_OP(function_index): {
      TargetSymbol* symbol = (TargetSymbol*)inst->operand[0];
      char name[256];
      const char* callee = TargetSymbolName(symbol->symbol, name,
                                            sizeof(name));
      int index = Wasm32FunctionIndex(callee);
      if (index < 0) {
        fprintf(stderr,
                "wasm32: '%s' has no table slot, because this translation "
                "unit does not define it.  Taking the address of a function "
                "from another object needs the wasm linker.\n",
                callee);
        layout->failed = true;
        index = -1;
      }
      // Slot 0 is the null slot, so a function's slot is one past its index.
      WasmWriteSLEB128(buf, index + 1);
      break;
    }
    case W_OP(call_indirect):
      WasmWriteULEB128(buf, (uint64_t)inst->addr);
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
    case W_OP(global_get):
    case W_OP(global_set):
      WasmWriteULEB128(buf, (uint64_t)ConstantIntValue(inst->operand[1]));
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
      int index = Wasm32FunctionIndex(name);
      if (index < 0) {
        fprintf(stderr,
                "wasm32: call to '%s', which this translation unit does not "
                "define.  Calls across objects need the wasm linker.\n",
                name);
        index = 0;
      }
      WasmWriteULEB128(buf, (uint64_t)index);
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

void Wasm32EncodeFunctionBody(Wasm32Generator* wasm, Wasm32DataLayout* layout,
                              Buffer* out) {
  // Local declarations, grouped by type in the order the indices assume.
  static const WasmValueType kTypeOrder[4] = {kWasmTypeI32, kWasmTypeI64,
                                              kWasmTypeF32, kWasmTypeF64};
  int groups = 0;
  for (int i = 0; i < 4; i++) {
    if (wasm->num_locals[i] > 0) {
      groups++;
    }
  }
  WasmWriteULEB128(out, (uint64_t)groups);
  for (int i = 0; i < 4; i++) {
    if (wasm->num_locals[i] > 0) {
      WasmWriteULEB128(out, (uint64_t)wasm->num_locals[i]);
      BufferAppendByte(out, (char)kTypeOrder[i]);
    }
  }

  TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
  while (inst != NULL) {
    if (!IsSkipped(inst)) {
      WriteInstruction(wasm, layout, out, inst);
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
// functype bytes match.  The table is module-wide and outlives any single
// function because lowering interns the signature of every indirect call
// before the writer gets to the functions themselves; indices handed out
// then have to still mean the same thing at encode time, so the table only
// ever grows until the module is written.

static Vector module_types;  // Buffer* for each distinct functype.
static bool module_types_ready;

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
  if (!module_types_ready) {
    VectorInit(&module_types);
    module_types_ready = true;
  }
  Buffer* encoding = NewBuffer();
  EncodeSignature(signature, encoding);
  for (size_t i = 0; i < module_types.length; i++) {
    Buffer* existing = module_types.value.p[i];
    if (BufferCompare(existing, encoding) == 0) {
      BufferDelete(encoding);
      return (int)i;
    }
  }
  VectorAppend(&module_types, encoding);
  return (int)(module_types.length - 1);
}

static void ResetTypeTable(void) {
  if (!module_types_ready) {
    return;
  }
  for (size_t i = 0; i < module_types.length; i++) {
    BufferDelete(module_types.value.p[i]);
  }
  VectorDestruct(&module_types);
  module_types_ready = false;
}

bool Wasm32WriteModule(String* filename) {
  for (size_t i = 0; i < compiler->functions.length; i++) {
    Wasm32Generator* wasm = compiler->functions.value.p[i];
    if (wasm->failed) {
      // Lowering already explained what it could not translate.  Writing the
      // module anyway would produce something that fails validation with a
      // far less useful message.
      ResetTypeTable();
      return false;
    }
  }

  // Addresses have to be settled before any body is encoded, because a body
  // that takes the address of a literal or a static encodes it as an
  // immediate.
  Wasm32DataLayout layout;
  if (!Wasm32BuildDataLayout(&layout)) {
    Wasm32DataLayoutDestruct(&layout);
    ResetTypeTable();
    return false;
  }

  Vector type_indices;
  VectorInit(&type_indices);

  Buffer code_section;
  BufferInit(&code_section);
  WasmWriteULEB128(&code_section, compiler->functions.length);

  for (size_t i = 0; i < compiler->functions.length; i++) {
    Wasm32Generator* wasm = compiler->functions.value.p[i];
    int type_index = Wasm32InternSignature(&wasm->signature);
    VectorAppend(&type_indices, (void*)(intptr_t)type_index);

    Buffer body;
    BufferInit(&body);
    Wasm32EncodeFunctionBody(wasm, &layout, &body);
    WasmWriteULEB128(&code_section, body.length);
    BufferAppend(&code_section, body.value, body.length);
    BufferDestruct(&body);
  }

  // Encoding a body can discover a reference to something this translation
  // unit does not define, which the layout could not have caught earlier.
  if (layout.failed) {
    Wasm32DataLayoutDestruct(&layout);
    BufferDestruct(&code_section);
    VectorDestruct(&type_indices);
    ResetTypeTable();
    return false;
  }

  Buffer module;
  BufferInit(&module);
  BufferAppendByte(&module, 0x00);
  BufferAppendByte(&module, 0x61);
  BufferAppendByte(&module, 0x73);
  BufferAppendByte(&module, 0x6D);
  BufferAppendWordLE(&module, 1);

  // Type section.
  Buffer section;
  BufferInit(&section);
  WasmWriteULEB128(&section, module_types.length);
  for (size_t i = 0; i < module_types.length; i++) {
    Buffer* encoding = module_types.value.p[i];
    BufferAppend(&section, encoding->value, encoding->length);
  }
  WriteSection(&module, WASM_SECTION_TYPE, &section);

  // Function section.
  BufferClear(&section);
  WasmWriteULEB128(&section, type_indices.length);
  for (size_t i = 0; i < type_indices.length; i++) {
    WasmWriteULEB128(&section, (uint64_t)(intptr_t)type_indices.value.p[i]);
  }
  WriteSection(&module, WASM_SECTION_FUNCTION, &section);

  // Table section: one funcref table holding every function, so that any of
  // them can be reached through a pointer.  Slot 0 is left empty and never
  // filled, which is what makes a null function pointer trap rather than
  // call whichever function happened to be first.
  BufferClear(&section);
  WasmWriteULEB128(&section, 1);
  BufferAppendByte(&section, (char)kWasmTypeFuncRef);
  BufferAppendByte(&section, 0x01);  // Both a minimum and a maximum.
  WasmWriteULEB128(&section, compiler->functions.length + 1);
  WasmWriteULEB128(&section, compiler->functions.length + 1);
  WriteSection(&module, WASM_SECTION_TABLE, &section);

  // Memory section: one memory with no maximum, holding the data and the
  // shadow stack with room above them for a heap.  Counting the heap pages
  // on top of what is already spoken for matters once the static data is
  // large enough on its own to reach the default size.
  BufferClear(&section);
  uint32_t pages = (layout.heap_start + 0xFFFF) / 0x10000 +
                   WASM32_DEFAULT_HEAP_PAGES;
  if (pages < WASM32_DEFAULT_MEMORY_PAGES) {
    pages = WASM32_DEFAULT_MEMORY_PAGES;
  }
  WasmWriteULEB128(&section, 1);
  BufferAppendByte(&section, 0x00);
  WasmWriteULEB128(&section, pages);
  WriteSection(&module, WASM_SECTION_MEMORY, &section);

  // Global section: the shadow stack pointer, initialized to the top of the
  // reserved stack region.
  BufferClear(&section);
  WasmWriteULEB128(&section, 1);
  BufferAppendByte(&section, (char)kWasmTypeI32);
  BufferAppendByte(&section, 0x01);  // Mutable.
  WriteOpcode(&section, W_OP(i32_const));
  WasmWriteSLEB128(&section, layout.stack_top);
  WriteOpcode(&section, W_OP(end));
  WriteSection(&module, WASM_SECTION_GLOBAL, &section);

  // Export section: the memory plus every function with external linkage.
  BufferClear(&section);
  size_t num_exports = 1;
  for (size_t i = 0; i < compiler->functions.length; i++) {
    Wasm32Generator* wasm = compiler->functions.value.p[i];
    if (wasm->base.is_global) {
      num_exports++;
    }
  }
  WasmWriteULEB128(&section, num_exports);
  WriteName(&section, "memory");
  BufferAppendByte(&section, WASM_EXTERN_MEMORY);
  WasmWriteULEB128(&section, 0);
  for (size_t i = 0; i < compiler->functions.length; i++) {
    Wasm32Generator* wasm = compiler->functions.value.p[i];
    if (!wasm->base.is_global) {
      continue;
    }
    WriteName(&section, wasm->base.function_name.value);
    BufferAppendByte(&section, WASM_EXTERN_FUNC);
    WasmWriteULEB128(&section, i);
  }
  WriteSection(&module, WASM_SECTION_EXPORT, &section);

  // Element section: fill the table from slot 1 on, so that a function's
  // table index is its function index plus one.
  if (compiler->functions.length > 0) {
    BufferClear(&section);
    WasmWriteULEB128(&section, 1);
    WasmWriteULEB128(&section, 0);  // Table 0, active.
    WriteOpcode(&section, W_OP(i32_const));
    WasmWriteSLEB128(&section, 1);
    WriteOpcode(&section, W_OP(end));
    WasmWriteULEB128(&section, compiler->functions.length);
    for (size_t i = 0; i < compiler->functions.length; i++) {
      WasmWriteULEB128(&section, i);
    }
    WriteSection(&module, WASM_SECTION_ELEMENT, &section);
  }

  WriteSection(&module, WASM_SECTION_CODE, &code_section);

  // Data section: one active segment holding every literal and initialized
  // static.  Uninitialized ones sit above it and need no bytes, since linear
  // memory starts out zeroed.
  if (layout.bytes.length > 0) {
    BufferClear(&section);
    WasmWriteULEB128(&section, 1);
    WasmWriteULEB128(&section, 0);  // Memory 0, active.
    WriteOpcode(&section, W_OP(i32_const));
    WasmWriteSLEB128(&section, layout.start);
    WriteOpcode(&section, W_OP(end));
    WasmWriteULEB128(&section, layout.bytes.length);
    BufferAppend(&section, layout.bytes.value, layout.bytes.length);
    WriteSection(&module, WASM_SECTION_DATA, &section);
  }

  bool ok = false;
  FILE* fp = fopen(filename->value, "wb");
  if (fp == NULL) {
    fprintf(stderr, "wasm32: cannot open %s for writing\n", filename->value);
  } else {
    size_t written = fwrite(module.value, 1, module.length, fp);
    ok = fclose(fp) == 0 && written == module.length;
    if (!ok) {
      fprintf(stderr, "wasm32: cannot write %s\n", filename->value);
    }
  }

  Wasm32DataLayoutDestruct(&layout);
  BufferDestruct(&section);
  BufferDestruct(&module);
  BufferDestruct(&code_section);
  VectorDestruct(&type_indices);
  ResetTypeTable();

  return ok;
}
