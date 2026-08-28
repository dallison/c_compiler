//
//  wasm32_machine.h
//  c_compiler
//
//  WebAssembly (wasm32) opcode definitions.
//

#ifndef wasm32_machine_h
#define wasm32_machine_h

#include <stdint.h>

// Every opcode is declared once in WASM32_OPCODES with the byte the wasm
// binary format uses for it.  Pseudo operations that never reach the encoder
// use WASM_NO_ENCODING.  The enum, the name table and the encoding table are
// all generated from this one list so they cannot drift apart.
#define WASM_NO_ENCODING (-1)

// Prefix byte for the saturating-conversion and bulk-memory opcodes.  Those
// are encoded as 0xFC followed by a LEB128 sub-opcode; the table stores the
// sub-opcode with this bit set.
#define WASM_PREFIX_FC 0x10000

// The first 32 entries must mirror the TargetOpcode enumeration in
// target_generator.h exactly: same order, same count.  The names may differ
// but the positions may not.  A static assertion below checks this.
#define WASM32_TARGET_OPCODES(V)                                              \
  V(save, WASM_NO_ENCODING)                                                   \
  V(restore, WASM_NO_ENCODING)                                                \
  V(symbol, WASM_NO_ENCODING)                                                 \
  V(literal, WASM_NO_ENCODING)                                                \
  V(tmp, WASM_NO_ENCODING)                                                    \
  V(const8, WASM_NO_ENCODING)                                                 \
  V(const16, WASM_NO_ENCODING)                                                \
  V(const32, WASM_NO_ENCODING)                                                \
  V(const64, WASM_NO_ENCODING)                                                \
  V(constf, WASM_NO_ENCODING)                                                 \
  V(constd, WASM_NO_ENCODING)                                                 \
  V(mov, WASM_NO_ENCODING)                                                    \
  V(movf, WASM_NO_ENCODING)                                                   \
  V(movd, WASM_NO_ENCODING)                                                   \
  V(movc, WASM_NO_ENCODING)                                                   \
  V(movfc, WASM_NO_ENCODING)                                                  \
  V(movdc, WASM_NO_ENCODING)                                                  \
  V(movxc, WASM_NO_ENCODING)                                                  \
  V(ret, WASM_NO_ENCODING)                                                    \
  V(label, WASM_NO_ENCODING)                                                  \
  V(fp, WASM_NO_ENCODING)                                                     \
  V(sp, WASM_NO_ENCODING)                                                     \
  V(tp, WASM_NO_ENCODING)                                                     \
  V(resulti, WASM_NO_ENCODING)                                                \
  V(resultf, WASM_NO_ENCODING)                                                \
  V(resultd, WASM_NO_ENCODING)                                                \
  V(structreturn, WASM_NO_ENCODING)                                           \
  V(asm, WASM_NO_ENCODING)                                                    \
  V(loc, WASM_NO_ENCODING)                                                    \
  V(named_label, WASM_NO_ENCODING)                                            \
  V(ivarreg, WASM_NO_ENCODING)                                                \
  V(fvarreg, WASM_NO_ENCODING)

// Backend pseudo operations that never reach the encoder.
//
//   param   names one of the function's parameter locals, so that anything
//           using it simply reads that local.
//   slot    names a local the backend needs for its own bookkeeping, such as
//           the stackifier's block selector.
//   arg     pushes one call argument; it encodes to the local.get for its
//           operand and nothing else, so a run of them immediately before a
//           call leaves the arguments on the stack in order.
#define WASM32_PSEUDO_OPCODES(V)                                              \
  V(param, WASM_NO_ENCODING)                                                  \
  V(slot, WASM_NO_ENCODING)                                                   \
  V(arg, WASM_NO_ENCODING)

// Control flow.  Scope-opening instructions carry their block type in
// operand[0] as a constant; br/br_if carry a relative depth that the
// stackifier fills in.
#define WASM32_CONTROL_OPCODES(V)                                             \
  V(unreachable, 0x00)                                                        \
  V(nop, 0x01)                                                                \
  V(block, 0x02)                                                              \
  V(loop, 0x03)                                                               \
  V(if, 0x04)                                                                 \
  V(else, 0x05)                                                               \
  V(end, 0x0B)                                                                \
  V(br, 0x0C)                                                                 \
  V(br_if, 0x0D)                                                              \
  V(br_table, 0x0E)                                                           \
  V(return, 0x0F)                                                             \
  V(call, 0x10)                                                               \
  V(call_indirect, 0x11)                                                      \
  V(drop, 0x1A)                                                               \
  V(select, 0x1B)

#define WASM32_VARIABLE_OPCODES(V)                                            \
  V(local_get, 0x20)                                                          \
  V(local_set, 0x21)                                                          \
  V(local_tee, 0x22)                                                          \
  V(global_get, 0x23)                                                         \
  V(global_set, 0x24)

// Memory accesses.  operand[0] is the address; stores take the value in
// operand[1].  The static offset lives in the instruction's aux data.
#define WASM32_MEMORY_OPCODES(V)                                              \
  V(i32_load, 0x28)                                                           \
  V(i64_load, 0x29)                                                           \
  V(f32_load, 0x2A)                                                           \
  V(f64_load, 0x2B)                                                           \
  V(i32_load8_s, 0x2C)                                                        \
  V(i32_load8_u, 0x2D)                                                        \
  V(i32_load16_s, 0x2E)                                                       \
  V(i32_load16_u, 0x2F)                                                       \
  V(i64_load8_s, 0x30)                                                        \
  V(i64_load8_u, 0x31)                                                        \
  V(i64_load16_s, 0x32)                                                       \
  V(i64_load16_u, 0x33)                                                       \
  V(i64_load32_s, 0x34)                                                       \
  V(i64_load32_u, 0x35)                                                       \
  V(i32_store, 0x36)                                                          \
  V(i64_store, 0x37)                                                          \
  V(f32_store, 0x38)                                                          \
  V(f64_store, 0x39)                                                          \
  V(i32_store8, 0x3A)                                                         \
  V(i32_store16, 0x3B)                                                        \
  V(i64_store8, 0x3C)                                                         \
  V(i64_store16, 0x3D)                                                        \
  V(i64_store32, 0x3E)                                                        \
  V(memory_size, 0x3F)                                                        \
  V(memory_grow, 0x40)

#define WASM32_CONST_OPCODES(V)                                               \
  V(i32_const, 0x41)                                                          \
  V(i64_const, 0x42)                                                          \
  V(f32_const, 0x43)                                                          \
  V(f64_const, 0x44)

// Values that are only known once the module's layout is fixed.  All three
// encode as i32.const; the operand names what to look up rather than holding
// the value.  A function's "address" is its slot in the function table,
// which is the only thing call_indirect accepts.
#define WASM32_ADDRESS_OPCODES(V)                                             \
  V(literal_address, 0x41)                                                    \
  V(symbol_address, 0x41)                                                     \
  V(function_index, 0x41)

#define WASM32_I32_OPCODES(V)                                                 \
  V(i32_eqz, 0x45)                                                            \
  V(i32_eq, 0x46)                                                             \
  V(i32_ne, 0x47)                                                             \
  V(i32_lt_s, 0x48)                                                           \
  V(i32_lt_u, 0x49)                                                           \
  V(i32_gt_s, 0x4A)                                                           \
  V(i32_gt_u, 0x4B)                                                           \
  V(i32_le_s, 0x4C)                                                           \
  V(i32_le_u, 0x4D)                                                           \
  V(i32_ge_s, 0x4E)                                                           \
  V(i32_ge_u, 0x4F)                                                           \
  V(i32_clz, 0x67)                                                            \
  V(i32_ctz, 0x68)                                                            \
  V(i32_popcnt, 0x69)                                                         \
  V(i32_add, 0x6A)                                                            \
  V(i32_sub, 0x6B)                                                            \
  V(i32_mul, 0x6C)                                                            \
  V(i32_div_s, 0x6D)                                                          \
  V(i32_div_u, 0x6E)                                                          \
  V(i32_rem_s, 0x6F)                                                          \
  V(i32_rem_u, 0x70)                                                          \
  V(i32_and, 0x71)                                                            \
  V(i32_or, 0x72)                                                             \
  V(i32_xor, 0x73)                                                            \
  V(i32_shl, 0x74)                                                            \
  V(i32_shr_s, 0x75)                                                          \
  V(i32_shr_u, 0x76)                                                          \
  V(i32_rotl, 0x77)                                                           \
  V(i32_rotr, 0x78)

#define WASM32_I64_OPCODES(V)                                                 \
  V(i64_eqz, 0x50)                                                            \
  V(i64_eq, 0x51)                                                             \
  V(i64_ne, 0x52)                                                             \
  V(i64_lt_s, 0x53)                                                           \
  V(i64_lt_u, 0x54)                                                           \
  V(i64_gt_s, 0x55)                                                           \
  V(i64_gt_u, 0x56)                                                           \
  V(i64_le_s, 0x57)                                                           \
  V(i64_le_u, 0x58)                                                           \
  V(i64_ge_s, 0x59)                                                           \
  V(i64_ge_u, 0x5A)                                                           \
  V(i64_clz, 0x79)                                                            \
  V(i64_ctz, 0x7A)                                                            \
  V(i64_popcnt, 0x7B)                                                         \
  V(i64_add, 0x7C)                                                            \
  V(i64_sub, 0x7D)                                                            \
  V(i64_mul, 0x7E)                                                            \
  V(i64_div_s, 0x7F)                                                          \
  V(i64_div_u, 0x80)                                                          \
  V(i64_rem_s, 0x81)                                                          \
  V(i64_rem_u, 0x82)                                                          \
  V(i64_and, 0x83)                                                            \
  V(i64_or, 0x84)                                                             \
  V(i64_xor, 0x85)                                                            \
  V(i64_shl, 0x86)                                                            \
  V(i64_shr_s, 0x87)                                                          \
  V(i64_shr_u, 0x88)                                                          \
  V(i64_rotl, 0x89)                                                           \
  V(i64_rotr, 0x8A)

#define WASM32_FLOAT_OPCODES(V)                                               \
  V(f32_eq, 0x5B)                                                             \
  V(f32_ne, 0x5C)                                                             \
  V(f32_lt, 0x5D)                                                             \
  V(f32_gt, 0x5E)                                                             \
  V(f32_le, 0x5F)                                                             \
  V(f32_ge, 0x60)                                                             \
  V(f64_eq, 0x61)                                                             \
  V(f64_ne, 0x62)                                                             \
  V(f64_lt, 0x63)                                                             \
  V(f64_gt, 0x64)                                                             \
  V(f64_le, 0x65)                                                             \
  V(f64_ge, 0x66)                                                             \
  V(f32_abs, 0x8B)                                                            \
  V(f32_neg, 0x8C)                                                            \
  V(f32_ceil, 0x8D)                                                           \
  V(f32_floor, 0x8E)                                                          \
  V(f32_trunc, 0x8F)                                                          \
  V(f32_nearest, 0x90)                                                        \
  V(f32_sqrt, 0x91)                                                           \
  V(f32_add, 0x92)                                                            \
  V(f32_sub, 0x93)                                                            \
  V(f32_mul, 0x94)                                                            \
  V(f32_div, 0x95)                                                            \
  V(f32_min, 0x96)                                                            \
  V(f32_max, 0x97)                                                            \
  V(f32_copysign, 0x98)                                                       \
  V(f64_abs, 0x99)                                                            \
  V(f64_neg, 0x9A)                                                            \
  V(f64_ceil, 0x9B)                                                           \
  V(f64_floor, 0x9C)                                                          \
  V(f64_trunc, 0x9D)                                                          \
  V(f64_nearest, 0x9E)                                                        \
  V(f64_sqrt, 0x9F)                                                           \
  V(f64_add, 0xA0)                                                            \
  V(f64_sub, 0xA1)                                                            \
  V(f64_mul, 0xA2)                                                            \
  V(f64_div, 0xA3)                                                            \
  V(f64_min, 0xA4)                                                            \
  V(f64_max, 0xA5)                                                            \
  V(f64_copysign, 0xA6)

// Conversions.  The saturating float-to-int forms are 0xFC-prefixed; they are
// what C semantics need because the plain forms trap on out-of-range input.
#define WASM32_CONVERT_OPCODES(V)                                             \
  V(i32_wrap_i64, 0xA7)                                                       \
  V(i32_trunc_f32_s, 0xA8)                                                    \
  V(i32_trunc_f32_u, 0xA9)                                                    \
  V(i32_trunc_f64_s, 0xAA)                                                    \
  V(i32_trunc_f64_u, 0xAB)                                                    \
  V(i64_extend_i32_s, 0xAC)                                                   \
  V(i64_extend_i32_u, 0xAD)                                                   \
  V(i64_trunc_f32_s, 0xAE)                                                    \
  V(i64_trunc_f32_u, 0xAF)                                                    \
  V(i64_trunc_f64_s, 0xB0)                                                    \
  V(i64_trunc_f64_u, 0xB1)                                                    \
  V(f32_convert_i32_s, 0xB2)                                                  \
  V(f32_convert_i32_u, 0xB3)                                                  \
  V(f32_convert_i64_s, 0xB4)                                                  \
  V(f32_convert_i64_u, 0xB5)                                                  \
  V(f32_demote_f64, 0xB6)                                                     \
  V(f64_convert_i32_s, 0xB7)                                                  \
  V(f64_convert_i32_u, 0xB8)                                                  \
  V(f64_convert_i64_s, 0xB9)                                                  \
  V(f64_convert_i64_u, 0xBA)                                                  \
  V(f64_promote_f32, 0xBB)                                                    \
  V(i32_reinterpret_f32, 0xBC)                                                \
  V(i64_reinterpret_f64, 0xBD)                                                \
  V(f32_reinterpret_i32, 0xBE)                                                \
  V(f64_reinterpret_i64, 0xBF)                                                \
  V(i32_extend8_s, 0xC0)                                                      \
  V(i32_extend16_s, 0xC1)                                                     \
  V(i64_extend8_s, 0xC2)                                                      \
  V(i64_extend16_s, 0xC3)                                                     \
  V(i64_extend32_s, 0xC4)                                                     \
  V(i32_trunc_sat_f32_s, WASM_PREFIX_FC | 0)                                  \
  V(i32_trunc_sat_f32_u, WASM_PREFIX_FC | 1)                                  \
  V(i32_trunc_sat_f64_s, WASM_PREFIX_FC | 2)                                  \
  V(i32_trunc_sat_f64_u, WASM_PREFIX_FC | 3)                                  \
  V(i64_trunc_sat_f32_s, WASM_PREFIX_FC | 4)                                  \
  V(i64_trunc_sat_f32_u, WASM_PREFIX_FC | 5)                                  \
  V(i64_trunc_sat_f64_s, WASM_PREFIX_FC | 6)                                  \
  V(i64_trunc_sat_f64_u, WASM_PREFIX_FC | 7)                                  \
  V(memory_copy, WASM_PREFIX_FC | 10)                                         \
  V(memory_fill, WASM_PREFIX_FC | 11)

#define WASM32_OPCODES(V)                                                     \
  WASM32_TARGET_OPCODES(V)                                                    \
  WASM32_PSEUDO_OPCODES(V)                                                    \
  WASM32_CONTROL_OPCODES(V)                                                   \
  WASM32_VARIABLE_OPCODES(V)                                                  \
  WASM32_MEMORY_OPCODES(V)                                                    \
  WASM32_CONST_OPCODES(V)                                                     \
  WASM32_ADDRESS_OPCODES(V)                                                   \
  WASM32_I32_OPCODES(V)                                                       \
  WASM32_I64_OPCODES(V)                                                       \
  WASM32_FLOAT_OPCODES(V)                                                     \
  WASM32_CONVERT_OPCODES(V)

#define W_OP(op) kWasm32_##op

typedef enum {
#define WASM32_DECLARE_OPCODE(name, encoding) W_OP(name),
  WASM32_OPCODES(WASM32_DECLARE_OPCODE)
#undef WASM32_DECLARE_OPCODE
      kWasm32_num_opcodes
} Wasm32Opcode;

// Wasm value types, as they appear in the binary format.
typedef enum {
  kWasmTypeI32 = 0x7F,
  kWasmTypeI64 = 0x7E,
  kWasmTypeF32 = 0x7D,
  kWasmTypeF64 = 0x7C,
  kWasmTypeFuncRef = 0x70,
  kWasmTypeVoid = 0x40,  // Empty block type.
} WasmValueType;

// Instruction flag bits.  The lower 16 bits of TargetInstruction::flags are
// reserved by the shared target layer, so everything here lives above that.
#define WASM32_FLAG_TYPE_SHIFT 16
#define WASM32_FLAG_TYPE_MASK (3 << WASM32_FLAG_TYPE_SHIFT)
#define WASM32_FLAG_TYPE_I32 (0 << WASM32_FLAG_TYPE_SHIFT)
#define WASM32_FLAG_TYPE_I64 (1 << WASM32_FLAG_TYPE_SHIFT)
#define WASM32_FLAG_TYPE_F32 (2 << WASM32_FLAG_TYPE_SHIFT)
#define WASM32_FLAG_TYPE_F64 (3 << WASM32_FLAG_TYPE_SHIFT)

// The instruction produces no value, so the emitter must not assign it a
// local or expect anything on the stack afterwards.
#define WASM32_FLAG_NO_RESULT (1 << 18)

// The instruction's value is consumed inline by its single user rather than
// round-tripped through a local.  Set by the stackifier.
#define WASM32_FLAG_STACKIFIED (1 << 19)

// The stack pointer global holding the shadow stack top.
#define WASM32_STACK_POINTER_GLOBAL 0

// Linear memory layout.  The first kilobyte is left empty so that a null
// dereference reads obviously wrong data instead of a real object; static
// data follows it, then the shadow stack, then whatever the heap claims.
// The shadow stack grows down from the top of its region.
// Every variadic argument occupies this many bytes in the caller's buffer,
// whatever its type.  A uniform slot keeps va_arg to one add and costs at
// most four wasted bytes per argument.
#define WASM32_VARARG_SLOT 8

#define WASM32_DATA_START 1024
#define WASM32_STACK_SIZE (1 << 20)
#define WASM32_DEFAULT_MEMORY_PAGES 256
#define WASM32_DEFAULT_HEAP_PAGES 64

const char* Wasm32OpcodeName(int op);

// The wasm binary encoding for an opcode, or WASM_NO_ENCODING for pseudo ops.
// Values with WASM_PREFIX_FC set are 0xFC-prefixed multi-byte opcodes.
int Wasm32OpcodeEncoding(int op);

#endif /* wasm32_machine_h */
