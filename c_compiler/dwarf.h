//
//  dwarf.h
//  c_compiler_library
//
//  Created by David Allison on 6/6/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdint.h>
#include "buffer.h"
#include "dstring.h"
#include "map.h"
#include "vector.h"

// This is the DWARF debug format handler for the assembler.

#ifndef dwarf_h
#define dwarf_h

struct AssemblerRelocation;
struct AssemblerSymbol;

// DWARF line standard opcode values.
#define DW_LNS_copy 1
#define DW_LNS_advance_pc 2
#define DW_LNS_advance_line 3
#define DW_LNS_set_file 4
#define DW_LNS_set_column 5
#define DW_LNS_negate_stmt 6
#define DW_LNS_set_basic_block 7
#define DW_LNS_const_add_pc 8
#define DW_LNS_fixed_advance_pc 9
#define DW_LNS_set_prologue_end 10
#define DW_LNS_set_epilogue_begin 11
#define DW_LNS_set_isa 12

// DWARF extended opcodes.
#define DW_LNE_end_sequence 1
#define DW_LNE_set_address 2
#define DW_LNE_define_file 3
#define DW_LNE_set_discriminator 4

// This is a single location, file, line and column vs address.
typedef struct {
  int file;
  int line;
  int col;
  uint64_t address;
} LocationEntry;

LocationEntry* NewLocationEntry(int file, int line, int col, uint64_t address);

typedef struct {
  String* filename;
  int dir;
} FileEntry;

FileEntry* NewFileEntry(String* filename, int dir);
void FileEntryDestruct(FileEntry* file);
void FileEntryDelete(FileEntry* file);

typedef struct {
  Map directory_table;  // Mapping of dir name vs index+1.
  int64_t next_dir_index;

  Vector file_table;  // Vector of FileEntry.
  Vector locations;   // Vector Location.
  uint8_t min_instruction_length;
  int8_t line_base;
  uint8_t line_range;
  int32_t
      address_offset;  // Offset into debug_line section for initial address.
} Dwarf;

void DwarfInit(Dwarf* dwarf);
void DwarfDestruct(Dwarf* dwarf);

void DwarfBuildDebugLineContents(Dwarf* dwarf, Buffer* debug_line);
struct AssemblerRelocation* DwarfDebugLineRelocation(
    Dwarf* dwarf, struct AssemblerSymbol* symbol, int reloc_type,
    int section_index);
void DwarfAddFile(Dwarf* dwarf, String* filename);
void DwarfAddLocation(Dwarf* dwarf, int file, int line, int col,
                      uint64_t address);

#endif /* dwarf_h */
