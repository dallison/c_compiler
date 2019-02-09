//
//  p_code_disassembler.c
//  p_code_interpreter
//
//  Created by David Allison on 1/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "p_code_disassembler.h"

// Meaning of dest_type and src_type:
// 'r': integer register
// 'f': float register
// 'd': double register
// 'x': no present

typedef struct {
  const char* name;
  char dest_type;
  char src1_type;
  char src2_type;
} Instruction;

static Instruction inst_32[] = {
  {"add", 'r', 'r', 'r'},
  {"sub", 'r', 'r', 'r'},
  {"addf", 'f', 'f', 'f'},
  {"addd", 'd', 'd', 'd'},
  {"subf", 'f', 'f', 'f'},
  {"subd", 'd', 'd', 'd'},
  {"mul", 'r', 'r', 'r'},
  {"mulf", 'f', 'f', 'f'},
  {"muld", 'd', 'd', 'd'},
  {"div", 'r', 'r', 'r'},
  {"divf", 'f', 'f', 'f'},
  {"divd", 'd', 'd', 'd'},
  {"mod", 'r', 'r', 'r'},
  {"lsr", 'r', 'r', 'r'},
  {"asr", 'r', 'r', 'r'},
  {"lsl", 'r', 'r', 'r'},
  {"or", 'r', 'r', 'r'},
  {"and", 'r', 'r', 'r'},
  {"xor", 'r', 'r', 'r'},
  {"not", 'r', 'r', 'x'},
  {"inv", 'r', 'r', 'x'},
  {"neg", 'r', 'r', 'x'},
  {"negf", 'f', 'f', 'x'},
  {"negd", 'd', 'd', 'x'},
  {"cmpeq", 'r', 'r', 'r'},
  {"cmpne", 'r', 'r', 'r'},
  {"cmplt", 'r', 'r', 'r'},
  {"cmple", 'r', 'r', 'r'},
  {"cmpgt", 'r', 'r', 'r'},
  {"cmpge", 'r', 'r', 'r'},
  {"cmpeqf", 'r', 'f', 'f'},
  {"cmpnef", 'r', 'f', 'f'},
  {"cmpltf", 'r', 'f', 'f'},
  {"cmplef", 'r', 'f', 'f'},
  {"cmpgtf", 'r', 'f', 'f'},
  {"cmpgef", 'r', 'f', 'f'},
  {"cmpeqd", 'r', 'd', 'd'},
  {"cmpned", 'r', 'd', 'd'},
  {"cmpltd", 'r', 'd', 'd'},
  {"cmpled", 'r', 'd', 'd'},
  {"cmpgtd", 'r', 'd', 'd'},
  {"cmpged", 'r', 'd', 'd'},
  {"decsp", 'i', 'x', 'x'},
  {"incsp", 'i', 'x', 'x'},
  {"push", 'r', 'x', 'x'},
  {"pushf", 'd', 'x', 'x'},
  {"pushd", 'd', 'x', 'x'},
  {"pushx", 'r', 'x', 'x'},
  {"pop", 'r', 'x', 'x'},
  {"popf", 'd', 'x', 'x'},
  {"popd", 'd', 'x', 'x'},
  {"popx", 'r', 'x', 'x'},
  {"mov", 'r', 'r', 'x'},
  {"movf", 'f', 'f', 'x'},
  {"movd", 'd', 'd', 'x'},
  {"ret", 'x', 'x', 'x'},
  {"cbra", 'r', 'x', 'x'},
  {"i2f", 'f', 'r', 'x'},
  {"i2d", 'd', 'r', 'x'},
  {"f2d", 'd', 'f', 'x'},
  {"d2f", 'f', 'd', 'x'},
  {"f2i", 'r', 'f', 'x'},
  {"d2i", 'r', 'd', 'x'},
  {"rcall", 'r', 'x', 'x'},
  {"esc", 'i', 'x', 'x'},
};

static Instruction inst_64[] = {
  {"ldw", 'r', 'r', 'i'},
  {"ldh", 'r', 'r', 'i'},
  {"ldb", 'r', 'r', 'i'},
  {"lduw", 'r', 'r', 'i'},
  {"ldub", 'r', 'r', 'i'},
  {"lduh", 'r', 'r', 'i'},
  {"ldx", 'r', 'r', 'i'},
  {"ldf", 'f', 'r', 'i'},
  {"ldd", 'd', 'r', 'i'},
  {"stw", 'r', 'r', 'i'},
  {"sth", 'r', 'r', 'i'},
  {"stx", 'r', 'r', 'i'},
  {"stf", 'f', 'r', 'i'},
  {"std", 'd', 'r', 'i'},
  {"stb", 'r', 'r', 'i'},

  {"movc", 'r', 'i', 'x'},
  {"movfc", 'f', 'i', 'x'},

  {"bz", 'r', 'i', 'x'},
  {"bnz", 'r', 'i', 'x'},
  {"bra", 'i', 'x', 'x'},
  {"addc", 'r', 'r', 'i'},
};

static Instruction inst_96[] = {
  {"movdc", 'd', 'i', 'x'},
  {"movxc", 'r', 'i', 'x'},
  {"jmp", 'i', 'x', 'x'},
  {"call", 'i', 'x', 'x'},
};

#define DEST(inst) (inst >> 16) & 0xff
#define SRC1(inst) (inst >> 8) & 0xff
#define SRC2(inst) (inst & 0xff)

static const char* PrintOperand(int op, char type, const char* sep, FILE* fp) {
  if (type == 'x') {
    return "";
  }
  fprintf(fp, "%s%c%d", sep, type, op);
  return ", ";
}

static void Print32(int32_t inst, FILE* fp) {
  int opcode = (inst >> 24) & 0x7f;
  fprintf(fp, "%-8s", inst_32[opcode].name);
  const char* sep = "";
  if (opcode == OP(incsp) || opcode == OP(decsp) || opcode == OP(esc)) {
    fprintf(fp, "#%d", inst & 0xffffff);
  } else {
    sep = PrintOperand(DEST(inst), inst_32[opcode].dest_type, sep, fp);
    sep = PrintOperand(SRC1(inst), inst_32[opcode].src1_type, sep, fp);
    sep = PrintOperand(SRC2(inst), inst_32[opcode].src2_type, sep, fp);
  }
  printf("\n");
}

static void Print64(int32_t inst, int32_t value, FILE* fp) {
  int opcode = (inst >> 24) & 0x3f;
  fprintf(fp, "%-8s", inst_64[opcode].name);
  const char* sep = "";
  if (opcode < OP(movc)) {
    // Load/Store instruction.
    PrintOperand(DEST(inst), inst_64[opcode].dest_type, "", fp);
    fprintf(fp, ", [r%d, #%d]", SRC1(inst), value);
  } else if (opcode == OP(bz) || opcode == OP(bnz)) {
    fprintf(fp, "r%d, %d", DEST(inst), value);
  } else if (opcode == OP(bra)) {
    fprintf(fp, "%d", value);
  } else if (opcode == OP(movc)) {
    sep = PrintOperand(DEST(inst), inst_64[opcode].dest_type, "", fp);
    fprintf(fp, "%s#%d", sep, value);
  } else if (opcode == OP(movfc)) {
    sep = PrintOperand(DEST(inst), inst_64[opcode].dest_type, "", fp);
    fprintf(fp, "%s#%f", sep, *(float*)(&value));
  } else {
    sep = PrintOperand(DEST(inst), inst_64[opcode].dest_type, "", fp);
    sep = PrintOperand(SRC1(inst), inst_64[opcode].src1_type, sep, fp);
    if (opcode == OP(addc)) {
      fprintf(fp, "%s#%d", sep, value);
    }
  }
  printf("\n");
}

static void Print96(int32_t inst, int64_t value, FILE* fp) {
  int opcode = (inst >> 24) & 0x3f;
  fprintf(fp, "%-8s", inst_96[opcode].name);
  const char* sep = "";
  switch (opcode) {
    case OP(movdc):
    case OP(movxc):
      sep = PrintOperand(DEST(inst), inst_96[opcode].dest_type, "", fp);
      fprintf(fp, "%s#0x%llx", sep, value);
      break;
    case OP(jmp):
    case OP(call):
      fprintf(fp, "0x%llx", value);
      break;
  }
  printf("\n");
}

void* DisassemblePCodeInstruction(Interpreter* interpreter, void* p, FILE* fp) {
  int32_t* pc = p;
  const char* symbol_name = "???";
  uint64_t offset = 0;
  if (interpreter->current_symbol != NULL) {
    symbol_name = interpreter->current_symbol->name.value;
    offset = (uint64_t)p - interpreter->current_symbol->address;
  }
  fprintf(fp, "%s+%lld: %p  ", symbol_name, offset, p);
  int32_t inst = *pc++;
  if ((inst & 0x80000000) == 0) {
    Print32(inst, fp);
    return pc;
  }
  if ((inst & 0x40000000) == 0) {
    Print64(inst, *pc++, fp);
    return pc;
  }
  Print96(inst, *(int64_t*)pc, fp);
  return pc + 2;
}
