//
//  6502_assembler.h
//  c_compiler_library
//
//  Created by David Allison on 5/17/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef _6502_assembler_h
#define _6502_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"
#include "risc_v_emitter.h"
#include "6502_machine.h"

// Branches and labels.
// -------------------
// The assembler has to take care of branches that are out of range.
// The 6502 branch instructions have only a signed 128-bit offset, giving
// a possibility of 0x7f bytes forward or backward from the PC.
// If the branch offset is bigger than than, we have to use a branch
// over a JMP instruction.
//
// We keep maps of all branches and all labels.  The branches map is
// keyed by a SourceLocation and contains an entry for each branch
// instruction.  A branch refers to a label but we might not know
// what the label is (which Label*) it refers to when the Branch
// is created.
//
// For each label, we keep a map of label name vs struct Label.  The
// Label contains the name of the label (again) and a vector of pointers
// to Branch structs that use this label.

typedef enum {
  kBranchShort,     // Short, 2-byte branch.
  kBranchLong,      // Long, 5-byte branch,
} BranchType;

typedef struct {
  _6502OpcodeValue opcode;    // BEQ, BCS, etc.
  BranchType type;            // Short or long.
  String label_name;          // Label name branched to.
  struct Label* label;        // Resolved Label.
  int64_t address;
} Branch;

typedef struct Label {
  String* label_name;   // Doesn't own string.
  Vector branches;      // Doesn't own Branch*.
  AssemblerSymbol* symbol;
} Label;

// The name _6502 is because we can't use 6502Assembler as an
// identifier.
typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;  // Section id for .bss section.
  Map branches;       // Map of SourceLocation to Branch*
  Map labels;         // Map of label name vs Label*.
  AssemblerSymbol* (*default_define_label)(struct Assembler*, String*);
} _6502Assembler;

bool _6502AssemblerInit(_6502Assembler* assembler, String* infile, String* outfile);
_6502Assembler* New6502Assembler(String* infile, String* outfile);
void _6502AssemblerDestruct(_6502Assembler* assembler);
void _6502AssemblerDelete(_6502Assembler* assembler);
void Assemble6502Instruction(Assembler* assembler, String* word);
#endif /* _6502_assembler_h */
