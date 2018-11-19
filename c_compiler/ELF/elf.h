//
//  elf.h
//  c_compiler
//
//  Created by David Allison on 1/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef elf_h
#define elf_h

#include <stdio.h>
#include <stdint.h>
#include "vector.h"
#include "dstring.h"
#include "buffer.h"

// Structures and definitions for ELF64 files.  This must
// match the ELF specification.

// NOTES:
// 1. The symbol table section (.symtab) needs special values in its
//    'link' and 'info' fields.  The link field is the index of the string
//     table section (.strtab).  The info field is one greater than the
//     index of the last local symbol.
// 2. The symtab section has special ordering requirements.  The order must be:
//    a. File symbol.
//    b. Section symbols.
//    c. Local symbols.
//    d. Global or undefined symbols.
// 3. The symtab section must have an empty (all zeroes) entry at index 0.
// 4. The relocation sections are named for the section upon which their
//    relocations are applied.  The name is formed by adding the prefix ".rela"
//    (or .rel) to their target section name.  For example, the relocation
//    table for the .text section is called .rela.text.
// 5. The relocation sections also must refer to the section that they
//    relocate, exactly as in note 1 above.
// 6. The .symtab and relocation sections have fixed size entries so they must
//    have their 'entsize' field set to the size of their entries.
// 7. Section names are held in their own string table.  This is usually called
//    .shstrtab and its index must be set in the file header's 'shstrndx' field.

// ELF 64 types.
typedef uint64_t ELF_Addr;
typedef uint16_t ELF_Half;
typedef uint16_t ELF_SHalf;
typedef uint64_t ELF_Off;
typedef uint32_t ELF_Sword;
typedef uint32_t ELF_Word;
typedef uint64_t ELF_Xword;
typedef uint64_t ELF_Sxword;

// Type of the ELF file (in the 'type' field of the file header).
#define ET(t) kELFType_##t
typedef enum {
  ET(none),   // No type specified.
  ET(rel),    // File is relocatable (an object file, typically).
  ET(exec),   // File is executable (a binary).
  ET(dyn),    // File is a dynamic executable (shared object).
  ET(core),   // File is a core dump file.
} ELFType;

// ELF Segment type.
#define PT(x) kELFSegmentType_##x
typedef enum {
 PT(null),
 PT(load),
 PT(dynamic),
 PT(interp),
 PT(note),
 PT(shlib),
 PT(phdr),
 PT(tls),
} ELFSegmentType;

#define PF(x) kELFSegmentFlags_##x
typedef enum {
  PF(x) = 1 << 0,
  PF(w) = 1 << 1,
  PF(r) = 1 << 2,
} ELFSegmentFlags;

// Symbol binding values.
#define STB(x) kELFSymbolType_##x
typedef enum {
  STB(local),       // Local symbol (seen only in this file)
  STB(global),      // Global symbol (seen by other files).
  STB(weak),        // Weak symbol (might not exist).
} ELFSymbolBinding;

// Symbol type values.  Each symbol has a type.
#define STT(x) kELFSymbolType_##x
typedef enum {
  STT(notype),     // No type specified.
  STT(object),     // Symbol is an object (a variable).
  STT(func),       // A function (always take address).
  STT(section),    // A section name symbol.
  STT(file),       // A file name symbol.
  STT(common),     // A common symbol (might be merged with another symbol).
  STT(tls),        // Thread local storage.
} ELFSymbolType;

// For a symbol, the binding is the top 4 bits and
// the type is the lower 4 bits of the info field.
#define ELF_ST_BIND(x) ((x) >> 4)
#define ELF_ST_TYPE(x) (((uint32_t)x) & 0xf)

// An ELF symbol struct, appears exactly in this form in the symbol table.
typedef struct {
  ELF_Word name;    // Symbol name, index into string table.
  uint8_t info;       // Type and binding.
  uint8_t other;      // Not used (must be zero).
  ELF_Half shndx;    // Section index.
  ELF_Addr value;
  ELF_Xword size;    // Size in bytes.
} ELFSymbol;

// A relocation.  Used to apply a symbol's value to an offset within
// a section at load time.  The info field contains the symbol index
// and the relocation type.  The relocation type specifies how the symbol's
// value is to used to modify the memory in the target section.
typedef struct  {
  ELF_Addr offset;     // Location to apply the action.
  ELF_Xword info;      // Encoded index and type.
  ELF_Sxword addend;   // Constant used to compute value.
} ELFRelocation;

// The 'info' field of a relocation is encoded as two
// 32 bit words:
// +-------------------------------+-----------------------------+
// |     symbol index              |        relocation type      |
// +-------------------------------+-----------------------------+

#define ELF_R_SYM(info)             ((info)>>32)
#define ELF_R_TYPE(info)            ((ELF_Word)(info))
#define ELF_R_INFO(sym, type)       (((ELF_Xword)(sym)<<32)+(ELF_Xword)(type))

// Relocation types we support.
// P-CODE
#define R_PCODE_CALL 1
#define R_PCODE_MOVXC 2
#define R_PCODE_JMP 3
#define R_PCODE_DATA64 4
#define R_PCODE_DATA32 5
#define R_PCODE_ADD16 6
#define R_PCODE_ADD32 7
#define R_PCODE_ADD64 8
#define R_PCODE_SUB16 9
#define R_PCODE_SUB32 10
#define R_PCODE_SUB64 11

// RISC-V
#define R_RISCV_NONE 0
#define R_RISCV_32 1
#define R_RISCV_64 2
#define R_RISCV_RELATIVE 3
#define R_RISCV_COPY 4
#define R_RISCV_JUMP_SLOT 5
#define R_RISCV_BRANCH 16
#define R_RISCV_JAL 17
#define R_RISCV_CALL 18
#define R_RISCV_CALL_PLT 19
#define R_RISCV_GOT_HI20 20
#define R_RISCV_PCREL_HI20 23
#define R_RISCV_PCREL_LO12_I 24
#define R_RISCV_PCREL_LO12_S 25
#define R_RISCV_HI20 26
#define R_RISCV_LO12_I 27
#define R_RISCV_LO12_S 28
#define R_RISCV_ADD8 33
#define R_RISCV_ADD16 34
#define R_RISCV_ADD32 35
#define R_RISCV_ADD64 36
#define R_RISCV_SUB8 37
#define R_RISCV_SUB16 38
#define R_RISCV_SUB32 39
#define R_RISCV_SUB64 40
#define R_RISCV_ALIGN 43
#define R_RISCV_RVC_BRANCH 44
#define R_RISCV_RVC_JUMP 45
#define R_RISCV_RVC_LUI 46
#define R_RISCV_RELAX 51


// A program header.
typedef struct {
  ELF_Word type;
  ELF_Word flags;
  ELF_Off offset;     // Segment file offset.
  ELF_Addr vaddr;     // Segment virtual address.
  ELF_Addr paddr;     // Segment physical address.
  ELF_Xword filesz;    // Segment size in file.
  ELF_Xword memsz;    // Segment size in memory.
  ELF_Xword align;    // Segment alignment, file & memory.
} ELFProgramHeader;

// Section type.  Each section has a specific type and therefore contains
// information in a specific format.  For example, the symtab section contains
// ELFSymbol structs.
#define SHT(x) kELFSectionType_##x
typedef enum {
  SHT(null),         // No type.
  SHT(progbits),     // Contains code or data.
  SHT(symtab),       // Symbol table.
  SHT(strtab),       // String table.
  SHT(rela),         // Relocations with addend present.
  SHT(hash),         // A symbol hash table.
  SHT(dynamic),      // Dynamic section for shared objects.
  SHT(note),         // A general note.
  SHT(nobits),       // No data is in section.
  SHT(rel),          // Relocations without addend.
  SHT(shlib),        // Shared library information.
  SHT(dynsym),       // Dynamic symbol table.
  SHT(num),
} ELFSectionType;

// Section flags.  These are ORed together.
#define SHF(x) kELFSectionFlag_##x
typedef enum {
  SHF(write) = 0x1,                  // Section is writeable.
  SHF(alloc) = 0x2,                  // Memory allocated in executable.
  SHF(execinstr) = 0x4,              // Contains executable code.
                                     // 0x08 is missing.
  SHF(merge) = 0x10,                 // Mergeable.
  SHF(strings) = 0x20,               // Contains strings.
} ELFSectionFlags;

// A section header.
typedef struct  {
  ELF_Word name;        // Section name, index in section name table.
  ELF_Word type;        // Section type.
  ELF_Xword flags;
  ELF_Addr addr;        // Section virtual address.
  ELF_Off offset;       // Offset into file.
  ELF_Xword size;       // Size of section on disk.
  ELF_Word link;        // Linked section index (if any).
  ELF_Word info;        // Special info about section.
  ELF_Xword addralign;  // Alignment requirements.
  ELF_Xword entsize;    // Size of a fixed size entry insection (or 0).
} ELFSectionHeader;


// Reserved section indexes.
#define SHN_LORESERVE 0xff00  // Start of reserved section indices.
#define SHN_ABS 0xfff1        // Absolute.
#define SHN_COM 0xfff2        // Common.

// Header for ELF file.
typedef struct {
  uint8_t ident[16];      // "\x7fELF" + indentification bytes.
  ELF_Half type;          // Type of ELF file (ET macro).
  ELF_Half machine;       // Machine this is for (architecture).
  ELF_Word version;       // The value 1.
  ELF_Addr entry;         // Entry point virtual address.
  ELF_Off phoff;          // Program header table file offset.
  ELF_Off shoff;          // Section header table file offset.
  ELF_Word flags;         // Some flags.
  ELF_Half ehsize;        // Size of ELF header (this struct).
  ELF_Half phentsize;     // Size of a program header.
  ELF_Half phnum;         // Number of program headers.
  ELF_Half shentsize;     // Size of a section header.
  ELF_Half shnum;         // Number of sections.
  ELF_Half shstrndx;      // Section index for section name string table.
} ELFHeader;

// Indexes into 'ident' field in header.
#define	EI_MAG0 0
#define	EI_MAG1 1
#define	EI_MAG2 2
#define	EI_MAG3 3
#define	EI_CLASS 4
#define	EI_DATA 5
#define	EI_VERSION 6
#define	EI_OSABI 7
#define	EI_PAD 8


#endif /* elf_h */
