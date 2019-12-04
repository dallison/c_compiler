//
//  dwarf.c
//  c_compiler_library
//
//  Created by David Allison on 6/6/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "dwarf.h"
#include <stdlib.h>
#include <string.h>
#include "assembler.h"
#include "map.h"

static int CompareString(const void* a, const void* b) {
  MapKeyValue* k1 = (MapKeyValue*)a;
  MapKeyValue* k2 = (MapKeyValue*)b;
  String* s1 = k1->key.p;
  String* s2 = k2->key.p;
  return strcmp(s1->value, s2->value);
}

void DwarfInit(Dwarf* dwarf) {
  MapInitForStringKeys(&dwarf->directory_table);

  // The next directory index.  This starts at 1 because 0 is the same
  // as NULL and that is returned from MapFind to signify that the key
  // wasn't found.  The actual index for a directory is this minus 1.
  // And, the first directory index is encoded as 1 in the debug_line
  // section, so we start at 2.
  dwarf->next_dir_index = 2;

  VectorInit(&dwarf->file_table);
  VectorInit(&dwarf->locations);

  // These are default values anc can be overwritten.  The defaults
  // are chosen for a RISC machine.  The line base and range are
  // the same as GCC outputs.
  dwarf->min_instruction_length = 4;
  dwarf->line_base = -5;
  dwarf->line_range = 14;
  dwarf->address_offset = 0;
}

// A directory in the directory_table is a mapping of dir name
// vs index (an integer).  The name is a String*
static void DeleteDirectoryEntry(MapKeyValue* kv) {
  StringDelete((String*)kv->key.p);
}

void DwarfDestruct(Dwarf* dwarf) {
  MapDestructWithContents(&dwarf->directory_table, DeleteDirectoryEntry);
  VectorDestructWithContents(&dwarf->file_table, NULL);
  VectorDestructWithContents(&dwarf->locations, NULL);
}

FileEntry* NewFileEntry(String* filename, int dir) {
  FileEntry* file = malloc(sizeof(FileEntry));
  file->filename = filename;
  file->dir = dir;
  return file;
}

void FileEntryDestruct(FileEntry* file) { StringDelete(file->filename); }

void FileEntryDelete(FileEntry* file) {
  FileEntryDestruct(file);
  free(file);
}

void DwarfAddFile(Dwarf* dwarf, String* filename) {
  int64_t dir = 0;
  size_t basename_index = StringLastIndexOf(filename, "/");
  String* basename = NewString(NULL);
  if (basename_index != -1) {
    String dir_name;
    StringInitFromSegment(&dir_name, filename->value, basename_index);
    void* dir_plus_one = MapFindPointerKey(&dwarf->directory_table, &dir_name);
    if (dir_plus_one == NULL) {
      // No dir in map.
      String* stored_dir_name = NewString(dir_name.value);
      dir = dwarf->next_dir_index - 1;
      MapKeyValue kv;
      kv.key.p = stored_dir_name;
      kv.value.w = dwarf->next_dir_index++;
      MapInsert(&dwarf->directory_table, kv);
    } else {
      dir = (int64_t)dir_plus_one - 1;
    }
    StringInitFromSegment(basename, filename->value + basename_index + 1,
                          filename->length - basename_index - 1);
  } else {
    StringSetString(basename, filename);
  }
  VectorAppend(&dwarf->file_table, NewFileEntry(basename, (int)dir));
}

void DwarfAddLocation(Dwarf* dwarf, int file, int line, int col,
                      uint64_t address) {
  if (dwarf->locations.length > 0) {
    LocationEntry* last_loc = VectorLast(&dwarf->locations);
    if (last_loc->address == address) {
      return;
    }
  }
  VectorAppend(&dwarf->locations, NewLocationEntry(file, line, col, address));
}

// These come directly from the DWARF4 spec:
// http://www.dwarfstd.org/doc/DWARF4.pdf
static void WriteULEB128(int32_t value, Buffer* buffer) {
  do {
    char byte = value & 0x7f;
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }
    BufferAppendByte(buffer, byte);
  } while (value != 0);
}

static void WriteSLEB128(uint32_t value, Buffer* buffer) {
  bool more = true;
  while (more) {
    char byte = value & 0x7f;
    value >>= 7;
    // sign bit of byte is 2nd high order bit (0x40)
    if ((value == 0 && (byte & 0x40) == 0) ||
        (value == -1 && (byte & 0x40) != 0)) {
      more = false;
    } else {
      byte |= 0x80;
    }
    BufferAppendByte(buffer, byte);
  }
}

static void EmitDirectory(MapKeyValue* kv, void* data) {
  const String* dir = kv->key.p;
  Buffer* buffer = data;
  BufferAppend(buffer, dir->value, dir->length + 1);  // Includes \0 at end.
}

// Build the contents of the .debug_line section from the directory table
// file table and locations held in the assembler.
// This emits a DWARF4 32-bit debug_line section contents.
// The reason for using 32 bit DWARF4 is that it is the recommended
// mode unless 64 bit is needed.  I don't think we need 64 bit.
// See http://www.dwarfstd.org/doc/DWARF4.pdf for details.
void DwarfBuildDebugLineContents(Dwarf* dwarf, Buffer* debug_line) {
  const int opcode_base = 13;

  // Space for the unit length.  This will be filled in when we know it.
  BufferAddSpace(debug_line, 4);

  // DWARF version 4.
  BufferAppendHalfLE(debug_line, 4);

  // Number of bytes from the header_length to the start of the
  // statement program.  Will be filled in later.  In 32 bit DWARF4
  // this is 4 bytes long.
  BufferAddSpace(debug_line, 4);

  // Mininum instruction length.
  BufferAppendByte(debug_line, dwarf->min_instruction_length);

  // maximum_operations_per_instruction
  BufferAppendByte(debug_line, 1);

  // default_is_stmt.
  BufferAppendByte(debug_line, 1);

  // Line base and range
  BufferAppendByte(debug_line, dwarf->line_base);
  BufferAppendByte(debug_line, dwarf->line_range);

  // opcode_base
  BufferAppendByte(debug_line, opcode_base);

  // Number of args for each standard opcode.
  // These come from the DWARF4 spec.
  static char standard_opcode_lengths[] = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};
  BufferAppend(debug_line, standard_opcode_lengths,
               sizeof(standard_opcode_lengths));

  // Emit dir table.
  MapTraverse(&dwarf->directory_table, EmitDirectory, debug_line);

  // Terminated by a single 0 byte.
  BufferAppendByte(debug_line, 0);

  // Emit filenames.
  for (size_t i = 0; i < dwarf->file_table.length; i++) {
    FileEntry* file = dwarf->file_table.value.p[i];
    BufferAppend(debug_line, file->filename->value, file->filename->length + 1);
    WriteULEB128(file->dir, debug_line);

    // Just omit the date and length fields.
    // TODO: do we need these, ever?  Does anything?
    WriteULEB128(0, debug_line);
    WriteULEB128(0, debug_line);
  }
  // Terminated by a single 0 byte.
  BufferAppendByte(debug_line, 0);

  // Overwrite header_length from header_length to this point.
  uint32_t* header_length = (uint32_t*)(debug_line->value + 6);
  *header_length = (uint32_t)debug_line->length - 10;

  // Now we have the statement program itself.  The statement program is a set
  // of bytecode instructions, each with an opcode and some operands.  There are
  // three types:
  // 1. Standard opcode - single byte opcode followed by operands.
  // 2. Extended opcode - zero, followed by an unsigned LEB128 instruction
  // length
  //                      followed by the operand.
  // 3. Special opcode - opcode that changes both line number and address in a
  // single
  //                     byte.  Limited range of values allowed.
  LocationEntry* prev_loc = NULL;
  for (size_t i = 0; i < dwarf->locations.length; i++) {
    LocationEntry* loc = dwarf->locations.value.p[i];
    if (prev_loc == NULL) {
      // No previous location means this is the first location.  We need
      // to set the initial line and address.  The address is associated
      // with a relocation referring to the .text of the current file.
      BufferAppendByte(debug_line, DW_LNS_advance_line);
      WriteSLEB128(loc->line - 1,
                   debug_line);  //  First line is 1 so we subtract 1.

      // Use extended opcode DW_LNE_set_address to set the address.
      // First byte is zero.
      BufferAppendByte(debug_line, 0);

      // Followed by the length of the instruction.
      WriteULEB128(9, debug_line);  // Address is 8 bytes, plus sub-opcode.

      // Followed by the sub-opcode.
      BufferAppendByte(debug_line, DW_LNE_set_address);

      // Followed by the address (this will be relocated).  We record the offset
      // so that the relocation can be applied.
      dwarf->address_offset = (int32_t)debug_line->length;
      BufferAppendLongLE(debug_line, 0);

      // We advance the address.  The address is going to be relative to
      // the start of .text so it will be small.  We cast it to 32 bits.
      BufferAppendByte(debug_line, DW_LNS_advance_pc);
      WriteULEB128((int32_t)loc->address / dwarf->min_instruction_length,
                   debug_line);
    } else {
      int line_diff = loc->line - prev_loc->line;
      int32_t address_diff = (int32_t)((loc->address - prev_loc->address) /
                                       dwarf->min_instruction_length);

      // Calculate special opcode and check if we can use it.  The DWARF
      // standard has a small set of standard and extended opcodes but majority
      // of the byte values are occupied by "special opcodes".  These are single
      // byte instructions that can increment both the line number and address
      // in a single byte.  There are limits to the ranges for both line and
      // address changes and if these are out of range, we fall back to using a
      // standard or extended opcode.
      int special_opcode = (line_diff - dwarf->line_base) +
                           (dwarf->line_range * address_diff) + opcode_base;
      if (special_opcode > 255) {
        // Need to use a standard opcodes.  A standard opcode uses a single byte
        // for the opcode and this is followed by zero or more operands.  Each
        // instruction specifies how many operands it takes and the format of
        // the operands.
        if (line_diff != 0) {
          BufferAppendByte(debug_line, DW_LNS_advance_line);
          WriteSLEB128(line_diff, debug_line);
        }
        if (address_diff != 0) {
          BufferAppendByte(debug_line, DW_LNS_advance_pc);
          WriteULEB128(address_diff, debug_line);
        }
      } else {
        // Special opcode is in range.
        BufferAppendByte(debug_line, special_opcode);
      }
    }
    prev_loc = loc;
  }

  // Now that we know the total length, we can write it into the buffer.
  uint32_t* unit_length = (uint32_t*)(debug_line->value);
  *unit_length = (uint32_t)debug_line->length -
                 4;  // Doesn't include unit_length field itself.
  BufferAlignLength(debug_line, 8);
}

AssemblerRelocation* DwarfDebugLineRelocation(Dwarf* dwarf,
                                              AssemblerSymbol* symbol,
                                              int reloc_type,
                                              int section_index) {
  return NewAssemblerRelocation(symbol, reloc_type, section_index,
                                dwarf->address_offset);
}
