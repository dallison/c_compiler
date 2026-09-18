//
//  6502_assembler.h
//  c_compiler_library
//
//  Created by David Allison on 5/17/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef W65C02_assembler_h
#define W65C02_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"
#include "6502_machine.h"


typedef struct {
  W65C02OpcodeValue opcode;    // BEQ, BCS, etc.
  String label_name;          // Label name branched to.
  struct Label* label;        // Resolved Label.
  int64_t address;
  SourceLocation location;
} Branch;

typedef struct Label {
  String* label_name;   // Doesn't own string.
  Vector branches;      // Doesn't own Branch*.
  AssemblerSymbol* symbol;
} Label;

// The name W65C02 is because we can't use 6502Assembler as an
// identifier.
typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;  // Section id for .bss section.
  Map branches;       // Map of (section, address) to Branch*
  Map labels;         // Map of label name vs Label*.
  AssemblerSymbol* (*default_define_label)(struct Assembler*, String*);
} W65C02Assembler;

bool W65C02AssemblerInit(W65C02Assembler* assembler, String* infile, String* outfile);
W65C02Assembler* New6502Assembler(String* infile, String* outfile);
void W65C02AssemblerFinalize(W65C02Assembler* assembler);
void W65C02AssemblerDestruct(W65C02Assembler* assembler);
void W65C02AssemblerDelete(W65C02Assembler* assembler);
void Assemble6502Instruction(Assembler* assembler, String* word);
#endif /* W65C02_assembler_h */
