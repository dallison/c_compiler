#include "disassembler_internal.h"

#include "p_code_machine.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  const char* name;
  char dest_type;
  char src1_type;
  char src2_type;
} PCodeFormat;

static const PCodeFormat kInst32[] = {
    {"add", 'r', 'r', 'r'},      {"sub", 'r', 'r', 'r'},
    {"addf", 'f', 'f', 'f'},     {"addd", 'd', 'd', 'd'},
    {"subf", 'f', 'f', 'f'},     {"subd", 'd', 'd', 'd'},
    {"mul", 'r', 'r', 'r'},      {"mulf", 'f', 'f', 'f'},
    {"muld", 'd', 'd', 'd'},     {"div", 'r', 'r', 'r'},
    {"divu", 'r', 'r', 'r'},     {"divf", 'f', 'f', 'f'},
    {"divd", 'd', 'd', 'd'},     {"mod", 'r', 'r', 'r'},
    {"modu", 'r', 'r', 'r'},     {"lsr", 'r', 'r', 'r'},
    {"asr", 'r', 'r', 'r'},      {"lsl", 'r', 'r', 'r'},
    {"or", 'r', 'r', 'r'},       {"and", 'r', 'r', 'r'},
    {"xor", 'r', 'r', 'r'},      {"not", 'r', 'r', 'x'},
    {"inv", 'r', 'r', 'x'},      {"neg", 'r', 'r', 'x'},
    {"negf", 'f', 'f', 'x'},     {"negd", 'd', 'd', 'x'},
    {"cmpeq", 'r', 'r', 'r'},    {"cmpne", 'r', 'r', 'r'},
    {"cmplt", 'r', 'r', 'r'},    {"cmple", 'r', 'r', 'r'},
    {"cmpgt", 'r', 'r', 'r'},    {"cmpge", 'r', 'r', 'r'},
    {"cmpltu", 'r', 'r', 'r'},   {"cmpleu", 'r', 'r', 'r'},
    {"cmpgtu", 'r', 'r', 'r'},   {"cmpgeu", 'r', 'r', 'r'},
    {"cmpeqf", 'r', 'f', 'f'},   {"cmpnef", 'r', 'f', 'f'},
    {"cmpltf", 'r', 'f', 'f'},   {"cmplef", 'r', 'f', 'f'},
    {"cmpgtf", 'r', 'f', 'f'},   {"cmpgef", 'r', 'f', 'f'},
    {"cmpeqd", 'r', 'd', 'd'},   {"cmpned", 'r', 'd', 'd'},
    {"cmpltd", 'r', 'd', 'd'},   {"cmpled", 'r', 'd', 'd'},
    {"cmpgtd", 'r', 'd', 'd'},   {"cmpged", 'r', 'd', 'd'},
    {"decsp", 'i', 'x', 'x'},    {"incsp", 'i', 'x', 'x'},
    {"push", 'r', 'x', 'x'},     {"pushf", 'd', 'x', 'x'},
    {"pushd", 'd', 'x', 'x'},    {"pushx", 'r', 'x', 'x'},
    {"pop", 'r', 'x', 'x'},      {"popf", 'd', 'x', 'x'},
    {"popd", 'd', 'x', 'x'},     {"popx", 'r', 'x', 'x'},
    {"mov", 'r', 'r', 'x'},      {"movf", 'f', 'f', 'x'},
    {"movd", 'd', 'd', 'x'},     {"ret", 'x', 'x', 'x'},
    {"cbra", 'r', 'x', 'x'},     {"i2f", 'f', 'r', 'x'},
    {"i2d", 'd', 'r', 'x'},      {"ui2f", 'f', 'r', 'x'},
    {"ui2d", 'd', 'r', 'x'},     {"f2d", 'd', 'f', 'x'},
    {"d2f", 'f', 'd', 'x'},      {"f2i", 'r', 'f', 'x'},
    {"d2i", 'r', 'd', 'x'},      {"f2ui", 'r', 'f', 'x'},
    {"d2ui", 'r', 'd', 'x'},     {"rcall", 'r', 'x', 'x'},
    {"esc", 'i', 'x', 'x'},      {"cmp3way", 'r', 'r', 'r'},
    {"cmp3wayu", 'r', 'r', 'r'}, {"cmp3wayf", 'r', 'f', 'f'},
    {"cmp3wayd", 'r', 'd', 'd'},
};

static const PCodeFormat kInst64[] = {
    {"ldw", 'r', 'r', 'i'},  {"ldh", 'r', 'r', 'i'},  {"ldb", 'r', 'r', 'i'},
    {"lduw", 'r', 'r', 'i'}, {"ldub", 'r', 'r', 'i'}, {"lduh", 'r', 'r', 'i'},
    {"ldx", 'r', 'r', 'i'},  {"ldf", 'f', 'r', 'i'},  {"ldd", 'd', 'r', 'i'},
    {"stw", 'r', 'r', 'i'},  {"sth", 'r', 'r', 'i'},  {"stx", 'r', 'r', 'i'},
    {"stf", 'f', 'r', 'i'},  {"std", 'd', 'r', 'i'},  {"stb", 'r', 'r', 'i'},
    {"movc", 'r', 'i', 'x'}, {"movfc", 'f', 'i', 'x'}, {"bz", 'r', 'i', 'x'},
    {"bnz", 'r', 'i', 'x'},  {"bra", 'i', 'x', 'x'},  {"addc", 'r', 'r', 'i'},
    {"lda", 'r', 'r', 'i'},
};

static const PCodeFormat kInst96[] = {
    {"movdc", 'd', 'i', 'x'}, {"movxc", 'r', 'i', 'x'}, {"jmp", 'i', 'x', 'x'},
    {"call", 'i', 'x', 'x'},  {"cjmp", 'i', 'x', 'x'},  {"adr", 'i', 'x', 'x'},
};

static int Dest(uint32_t inst) { return (int)((inst >> 16) & 0xff); }
static int Src1(uint32_t inst) { return (int)((inst >> 8) & 0xff); }
static int Src2(uint32_t inst) { return (int)(inst & 0xff); }

static void AppendOperand(char* text, size_t size, const char** sep, int op,
                          char type) {
  if (type == 'x') {
    return;
  }
  size_t used = strlen(text);
  if (type == 'i') {
    snprintf(text + used, size - used, "%s#%d", *sep, op);
  } else {
    snprintf(text + used, size - used, "%s%c%d", *sep, type, op);
  }
  *sep = ", ";
}

bool DAsmDisassemblePCode(const void* bytes, size_t length, uint64_t address,
                          DAsmInstruction* inst) {
  if (length < 4) {
    return false;
  }
  const unsigned char* p = bytes;
  uint32_t word = DAsmRead32LE(p);
  if ((word & 0x80000000u) == 0) {
    unsigned opcode = (word >> 24) & 0x7f;
    DAsmInitInstruction(inst, bytes, length, address, 4);
    if (opcode >= sizeof(kInst32) / sizeof(kInst32[0])) {
      DAsmUnknownInstruction(inst, ".word 0x%08" PRIx64, word);
      return true;
    }
    const PCodeFormat* fmt = &kInst32[opcode];
    char text[DASM_MAX_TEXT];
    snprintf(text, sizeof(text), "%-8s", fmt->name);
    const char* sep = "";
    if (opcode == (unsigned)PCODE_OP(incsp) ||
        opcode == (unsigned)PCODE_OP(decsp) ||
        opcode == (unsigned)PCODE_OP(esc)) {
      snprintf(text + strlen(text), sizeof(text) - strlen(text), "#%d",
               (int)(word & 0xffffff));
    } else {
      AppendOperand(text, sizeof(text), &sep, Dest(word), fmt->dest_type);
      AppendOperand(text, sizeof(text), &sep, Src1(word), fmt->src1_type);
      AppendOperand(text, sizeof(text), &sep, Src2(word), fmt->src2_type);
    }
    DAsmFormat(inst, "%s", text);
    return true;
  }

  if ((word & 0x40000000u) == 0) {
    if (length < 8) {
      return false;
    }
    int32_t value = (int32_t)DAsmRead32LE(p + 4);
    unsigned opcode = (word >> 24) & 0x3f;
    DAsmInitInstruction(inst, bytes, length, address, 8);
    if (opcode >= sizeof(kInst64) / sizeof(kInst64[0])) {
      DAsmUnknownInstruction(inst, ".word 0x%08" PRIx64, word);
      return true;
    }
    const PCodeFormat* fmt = &kInst64[opcode];
    char text[DASM_MAX_TEXT];
    if (opcode < (unsigned)PCODE_OP(movc) || opcode == (unsigned)PCODE_OP(lda)) {
      snprintf(text, sizeof(text), "%-8s%c%d, [r%d, #%d]", fmt->name,
               fmt->dest_type, Dest(word), Src1(word), value);
    } else if (opcode == (unsigned)PCODE_OP(bz) ||
               opcode == (unsigned)PCODE_OP(bnz)) {
      uint64_t target = address + 8 + (uint64_t)(int64_t)value;
      snprintf(text, sizeof(text), "%-8sr%d, 0x%" PRIx64, fmt->name, Dest(word),
               target);
      DAsmFormat(inst, "%s", text);
      DAsmSetTarget(inst, target);
      return true;
    } else if (opcode == (unsigned)PCODE_OP(bra)) {
      uint64_t target = address + 8 + (uint64_t)(int64_t)value;
      snprintf(text, sizeof(text), "%-8s0x%" PRIx64, fmt->name, target);
      DAsmFormat(inst, "%s", text);
      DAsmSetTarget(inst, target);
      return true;
    } else if (opcode == (unsigned)PCODE_OP(movc)) {
      snprintf(text, sizeof(text), "%-8s%c%d, #%d", fmt->name, fmt->dest_type,
               Dest(word), value);
    } else if (opcode == (unsigned)PCODE_OP(movfc)) {
      float f;
      memcpy(&f, &value, sizeof(f));
      snprintf(text, sizeof(text), "%-8s%c%d, #%g", fmt->name, fmt->dest_type,
               Dest(word), f);
    } else {
      snprintf(text, sizeof(text), "%-8s%c%d, %c%d, #%d", fmt->name,
               fmt->dest_type, Dest(word), fmt->src1_type, Src1(word), value);
    }
    DAsmFormat(inst, "%s", text);
    return true;
  }

  if (length < 12) {
    return false;
  }
  int64_t value = (int64_t)DAsmRead64LE(p + 4);
  unsigned opcode = (word >> 24) & 0x3f;
  DAsmInitInstruction(inst, bytes, length, address, 12);
  if (opcode >= sizeof(kInst96) / sizeof(kInst96[0])) {
    DAsmUnknownInstruction(inst, ".word 0x%08" PRIx64, word);
    return true;
  }
  const PCodeFormat* fmt = &kInst96[opcode];
  char text[DASM_MAX_TEXT];
  uint64_t after = address + 12;
  switch (opcode) {
    case 0:  // movdc
    case 1:  // movxc
      snprintf(text, sizeof(text), "%-8s%c%d, #0x%" PRIx64, fmt->name,
               fmt->dest_type, Dest(word), (uint64_t)value);
      break;
    case 2:  // jmp
    case 3:  // call
    case 4: {  // cjmp
      uint64_t target = after + (uint64_t)value;
      snprintf(text, sizeof(text), "%-8s0x%" PRIx64, fmt->name, target);
      DAsmFormat(inst, "%s", text);
      DAsmSetTarget(inst, target);
      return true;
    }
    case 5: {  // adr
      uint64_t target = after + (uint64_t)value;
      snprintf(text, sizeof(text), "%-8sr%d, 0x%" PRIx64, fmt->name, Dest(word),
               target);
      DAsmFormat(inst, "%s", text);
      DAsmSetTarget(inst, target);
      return true;
    }
    default:
      DAsmUnknownInstruction(inst, ".word 0x%08" PRIx64, word);
      return true;
  }
  DAsmFormat(inst, "%s", text);
  return true;
}
