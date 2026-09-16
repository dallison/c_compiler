#include <stdio.h>
#include <string.h>

#include "c_compiler/xtensa/xtensa_isa.h"

static int failures;

#define CHECK(condition)                                                   \
  do {                                                                     \
    if (!(condition)) {                                                    \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
      failures++;                                                          \
    }                                                                      \
  } while (0)

static void CheckEncoding(XtensaInstruction instruction,
                          const uint8_t expected[3]) {
  uint8_t encoded[3];
  CHECK(XtensaEncode(&instruction, encoded));
  CHECK(memcmp(encoded, expected, 3) == 0);

  XtensaInstruction decoded;
  CHECK(XtensaDecode(encoded, sizeof(encoded), &decoded));
  CHECK(decoded.kind == instruction.kind);
  CHECK(decoded.rd == instruction.rd);
  CHECK(decoded.rs == instruction.rs);
  CHECK(decoded.rt == instruction.rt);
  CHECK(decoded.immediate == instruction.immediate);
  CHECK(decoded.size == 3);
}

static void CheckRoundTrip(XtensaInstruction instruction) {
  uint8_t encoded[3];
  CHECK(XtensaEncode(&instruction, encoded));

  XtensaInstruction decoded;
  CHECK(XtensaDecode(encoded, sizeof(encoded), &decoded));
  CHECK(decoded.kind == instruction.kind);
  CHECK(decoded.rd == instruction.rd);
  CHECK(decoded.rs == instruction.rs);
  CHECK(decoded.rt == instruction.rt);
  CHECK(decoded.immediate == instruction.immediate);
  CHECK(decoded.size == 3);
}

int main(void) {
  CheckEncoding((XtensaInstruction){
                    .kind = kXtensaAdd, .rd = 2, .rs = 3, .rt = 4},
                (const uint8_t[]){0x40, 0x23, 0x80});
  CheckEncoding(
      (XtensaInstruction){.kind = kXtensaMovi, .rd = 2, .immediate = 42},
      (const uint8_t[]){0x22, 0xa0, 0x2a});
  CheckEncoding((XtensaInstruction){
                    .kind = kXtensaL32i, .rd = 2, .rs = 1, .immediate = 16},
                (const uint8_t[]){0x22, 0x21, 0x04});
  CheckEncoding(
      (XtensaInstruction){.kind = kXtensaEntry, .rs = 1, .immediate = 32},
      (const uint8_t[]){0x36, 0x41, 0x00});
  CheckEncoding((XtensaInstruction){.kind = kXtensaRetw},
                (const uint8_t[]){0x90, 0x00, 0x00});
  CheckEncoding((XtensaInstruction){.kind = kXtensaCallx8, .rs = 8},
                (const uint8_t[]){0xe0, 0x08, 0x00});

  uint8_t call[3];
  CHECK(XtensaEncode(&(XtensaInstruction){
                         .kind = kXtensaCall8, .immediate = 0},
                     call));
  CHECK(XtensaPatchSlot0(call, 0x40080000, 0x40080100));
  XtensaInstruction decoded;
  CHECK(XtensaDecode(call, sizeof(call), &decoded));
  CHECK(decoded.kind == kXtensaCall8);
  CHECK(decoded.immediate == 0xfc);

  const XtensaInstruction instructions[] = {
      {.kind = kXtensaAdd, .rd = 2, .rs = 3, .rt = 4},
      {.kind = kXtensaSub, .rd = 2, .rs = 3, .rt = 4},
      {.kind = kXtensaAnd, .rd = 2, .rs = 3, .rt = 4},
      {.kind = kXtensaOr, .rd = 2, .rs = 3, .rt = 4},
      {.kind = kXtensaXor, .rd = 2, .rs = 3, .rt = 4},
      {.kind = kXtensaNeg, .rd = 2, .rs = 3},
      {.kind = kXtensaAddi, .rd = 2, .rs = 3, .immediate = -7},
      {.kind = kXtensaMovi, .rd = 2, .immediate = -42},
      {.kind = kXtensaL8ui, .rd = 2, .rs = 3, .immediate = 7},
      {.kind = kXtensaL16ui, .rd = 2, .rs = 3, .immediate = 14},
      {.kind = kXtensaL16si, .rd = 2, .rs = 3, .immediate = 14},
      {.kind = kXtensaL32i, .rd = 2, .rs = 3, .immediate = 28},
      {.kind = kXtensaS8i, .rd = 2, .rs = 3, .immediate = 7},
      {.kind = kXtensaS16i, .rd = 2, .rs = 3, .immediate = 14},
      {.kind = kXtensaS32i, .rd = 2, .rs = 3, .immediate = 28},
      {.kind = kXtensaBeq, .rs = 2, .rt = 3, .immediate = -4},
      {.kind = kXtensaBne, .rs = 2, .rt = 3, .immediate = -4},
      {.kind = kXtensaBlt, .rs = 2, .rt = 3, .immediate = -4},
      {.kind = kXtensaBge, .rs = 2, .rt = 3, .immediate = -4},
      {.kind = kXtensaBltu, .rs = 2, .rt = 3, .immediate = -4},
      {.kind = kXtensaBgeu, .rs = 2, .rt = 3, .immediate = -4},
      {.kind = kXtensaBeqz, .rs = 2, .immediate = -4},
      {.kind = kXtensaBnez, .rs = 2, .immediate = -4},
      {.kind = kXtensaBltz, .rs = 2, .immediate = -4},
      {.kind = kXtensaBgez, .rs = 2, .immediate = -4},
      {.kind = kXtensaL32r, .rd = 2, .immediate = -16},
      {.kind = kXtensaJ, .immediate = -4},
      {.kind = kXtensaJx, .rs = 3},
      {.kind = kXtensaCall8, .immediate = -4},
      {.kind = kXtensaCallx8, .rs = 8},
      {.kind = kXtensaEntry, .rs = 1, .immediate = 32},
      {.kind = kXtensaRetw},
      {.kind = kXtensaBreak, .immediate = 0x12},
      {.kind = kXtensaSlli, .rd = 2, .rs = 3, .immediate = 19},
      {.kind = kXtensaSrai, .rd = 2, .rs = 3, .immediate = 19},
      {.kind = kXtensaSrli, .rd = 2, .rs = 3, .immediate = 11},
      {.kind = kXtensaSsl, .rs = 3},
      {.kind = kXtensaSsr, .rs = 3},
      {.kind = kXtensaSll, .rd = 2, .rs = 3},
      {.kind = kXtensaSrl, .rd = 2, .rs = 3},
      {.kind = kXtensaSra, .rd = 2, .rs = 3},
      {.kind = kXtensaMull, .rd = 2, .rs = 3, .rt = 4},
      {.kind = kXtensaQuou, .rd = 2, .rs = 3, .rt = 4},
      {.kind = kXtensaQuos, .rd = 2, .rs = 3, .rt = 4},
      {.kind = kXtensaRemu, .rd = 2, .rs = 3, .rt = 4},
      {.kind = kXtensaRems, .rd = 2, .rs = 3, .rt = 4},
  };
  for (size_t i = 0; i < sizeof(instructions) / sizeof(instructions[0]);
       ++i) {
    CheckRoundTrip(instructions[i]);
  }

  uint8_t jump[3];
  CHECK(XtensaEncode(
      &(XtensaInstruction){.kind = kXtensaJ, .immediate = 0}, jump));
  CHECK(XtensaPatchSlot0(jump, 0x40080000, 0x40080100));
  CHECK(XtensaDecode(jump, sizeof(jump), &decoded));
  CHECK(decoded.kind == kXtensaJ);
  CHECK(decoded.immediate == 0xfc);
  CHECK(!XtensaPatchSlot0(call, 0x40080000, 0x40080102));

  unsigned reg = 0;
  CHECK(XtensaParseRegister("a15", &reg));
  CHECK(reg == 15);
  CHECK(strcmp(XtensaRegisterName(reg), "a15") == 0);
  CHECK(!XtensaParseRegister("a16", &reg));

  return failures != 0;
}
