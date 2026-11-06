//
//  main.c
//  6502dasm
//
//  Created by David Allison on 10/6/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "disassembler.h"

int main(int argc, const char** argv) {
  return DAsmToolMain(argc, argv, kDAsm6502, "6502dasm");
}
