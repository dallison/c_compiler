//
//  main.c
//  aarch64dasm
//

#include "disassembler.h"

int main(int argc, const char** argv) {
  return DAsmToolMain(argc, argv, kDAsmAArch64, "aarch64dasm");
}
