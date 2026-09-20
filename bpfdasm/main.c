#include "disassembler.h"

int main(int argc, const char** argv) {
  return DAsmToolMain(argc, argv, kDAsmBPF, "bpfdasm");
}
