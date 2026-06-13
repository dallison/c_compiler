//
//  main.c
//  x86_64dasm
//

#include "disassembler.h"

int main(int argc, const char** argv) {
  return DAsmToolMain(argc, argv, kDAsmX86_64, "x86_64dasm");
}
