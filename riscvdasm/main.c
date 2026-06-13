//
//  main.c
//  riscvdasm
//

#include "disassembler.h"

int main(int argc, const char** argv) {
  return DAsmToolMain(argc, argv, kDAsmRiscV, "riscvdasm");
}
