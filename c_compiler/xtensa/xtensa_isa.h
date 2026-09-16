//
//  xtensa_isa.h
//  c_compiler
//
//  Encoding and decoding for the scalar Xtensa instructions emitted by the
//  ESP32 backend.  Xtensa instruction bytes are little-endian even though
//  instruction fields are conventionally documented from bit 23 to bit 0.
//

#ifndef xtensa_isa_h
#define xtensa_isa_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  kXtensaInvalid,
  kXtensaAdd,
  kXtensaSub,
  kXtensaAnd,
  kXtensaOr,
  kXtensaXor,
  kXtensaNeg,
  kXtensaAddi,
  kXtensaMovi,
  kXtensaL8ui,
  kXtensaL16ui,
  kXtensaL16si,
  kXtensaL32i,
  kXtensaS8i,
  kXtensaS16i,
  kXtensaS32i,
  kXtensaBeq,
  kXtensaBne,
  kXtensaBlt,
  kXtensaBge,
  kXtensaBltu,
  kXtensaBgeu,
  kXtensaBeqz,
  kXtensaBnez,
  kXtensaBltz,
  kXtensaBgez,
  kXtensaL32r,
  kXtensaJ,
  kXtensaJx,
  kXtensaCall8,
  kXtensaCallx8,
  kXtensaEntry,
  kXtensaRetw,
  kXtensaBreak,
  kXtensaSlli,
  kXtensaSrai,
  kXtensaSrli,
  kXtensaSsl,
  kXtensaSsr,
  kXtensaSll,
  kXtensaSrl,
  kXtensaSra,
  kXtensaMull,
  kXtensaQuou,
  kXtensaQuos,
  kXtensaRemu,
  kXtensaRems,
} XtensaInstructionKind;

typedef struct {
  XtensaInstructionKind kind;
  uint8_t rd;
  uint8_t rs;
  uint8_t rt;
  int32_t immediate;
  uint8_t size;
} XtensaInstruction;

const char* XtensaInstructionName(XtensaInstructionKind kind);
const char* XtensaRegisterName(unsigned reg);
bool XtensaParseRegister(const char* text, unsigned* reg);

// Encode one 24-bit instruction.  The immediate is a byte offset for memory,
// branch, jump, call, ENTRY, and L32R instructions.
bool XtensaEncode(const XtensaInstruction* inst, uint8_t out[3]);

// Decode one instruction.  Density encodings are recognized as two-byte
// instructions but return false until their individual decoders are enabled.
bool XtensaDecode(const uint8_t* bytes, size_t available,
                  XtensaInstruction* inst);

// Apply R_XTENSA_SLOT0_OP to an instruction at place.  target and place are
// linked virtual addresses.
bool XtensaPatchSlot0(uint8_t bytes[3], uint32_t place, uint32_t target);

#endif /* xtensa_isa_h */
