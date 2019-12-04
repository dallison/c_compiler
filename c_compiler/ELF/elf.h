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
 PT(load),        // Loadable segment.
 PT(dynamic),     // DYNAMIC segment (for .so files)
 PT(interp),      // Program interpreter.
 PT(note),        // General note.
 PT(shlib),       // Shared library.
 PT(phdr),        // Program header
 PT(tls),         // Thread local storage.
} ELFSegmentType;

#define PF(x) kELFSegmentFlags_##x
typedef enum {
  PF(x) = 1 << 0,   // Executable.
  PF(w) = 1 << 1,   // Writeable.
  PF(r) = 1 << 2,   // Readable.
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
#define R_PCODE_CALL 1        // Call direct to symbol.
#define R_PCODE_ABS 2         // Move symbol address to reg.
#define R_PCODE_JMP 3         // Jump to symbol.
#define R_PCODE_DATA64 4      // 64-bit data.
#define R_PCODE_DATA32 5      // 32-bit data.
#define R_PCODE_ADD16 6       // Add 16-bit.
#define R_PCODE_ADD32 7       // Add 32-bit.
#define R_PCODE_ADD64 8       // Add 64-bit.
#define R_PCODE_SUB16 9       // Subtract 16-bit.
#define R_PCODE_SUB32 10      // Subtract 32-bit.
#define R_PCODE_SUB64 11      // Subtract 64-bit.
#define R_PCODE_CALL_PLT 12   // Call via PLT.
#define R_PCODE_GOT_ENTRY 13  // Address of GOT entry for data.
#define R_PCODE_GOT_DATA 14       // Value of data in GOT.
#define R_PCODE_GOT_FUNC 15       // Value of function in GOT.
#define R_PCODE_PCREL 16        // PC relative address.
#define R_PCODE_GOT_TLS_IE 17    // TLS IE GOT entry.
#define R_PCODE_GOT_TLS_GD 18    // TLS GD GOT entry.
#define R_PCODE_TLS_TP_OFF 19    // TLS Thread pointer offset.
#define R_PCODE_GOT_TLS_OFFSET 20
#define R_PCODE_GOT_TLS_MODID 21

// RISC-V
#define R_RISCV_NONE 0        // No action.
#define R_RISCV_32 1          // Add 32 bit symbol value.
#define R_RISCV_64 2          // Add 64 bit symbol value.
#define R_RISCV_RELATIVE 3    // Add load address of shared object.
#define R_RISCV_COPY 4        // Copy data from shared object.
#define R_RISCV_JUMP_SLOT 5   // Set GOT entry.
#define R_RISCV_TLS_DTPMOD32 6  // TLS DTV module ID
#define R_RISCV_TLS_DTPMOD64 7  // TLS DTV module ID
#define R_RISCV_TLS_DTPREL32 8  // TLS
#define R_RISCV_TLS_DTPREL64 9
#define R_RISCV_TLS_TPREL32 10
#define R_RISCV_TLS_TPREL64 11
#define R_RISCV_BRANCH 16     // PC relative branch.
#define R_RISCV_JAL 17        // PC relative jump.
#define R_RISCV_CALL 18       // PC relative call.
#define R_RISCV_CALL_PLT 19   // PC relative call via PLT.
#define R_RISCV_GOT_HI20 20   // PC relative GOT high 20 bits.
#define R_RISCV_TLS_GOT_HI20 21 // TLS IE high 20 bits.
#define R_RISCV_TLS_GD_HI20 22  // TLS GD high 20.
#define R_RISCV_PCREL_HI20 23   // PC relative high 20 bits.
#define R_RISCV_PCREL_LO12_I 24 // PC relative low 12 bits (I-type)
#define R_RISCV_PCREL_LO12_S 25 // PC relaitve low 12 bits (S-type)
#define R_RISCV_HI20 26         // Absolute high 20 bits.
#define R_RISCV_LO12_I 27       // Absolute low 12 bits (I-type)
#define R_RISCV_LO12_S 28       // Absolute low 12 bits (S-type)
#define R_RISCV_TPREL_HI20 29
#define R_RISCV_TPREL_LO12_I 30
#define R_RISCV_TPREL_LO12_S 31
#define R_RISCV_TPREL_ADD 32
#define R_RISCV_ADD8 33         // Add 8 bits.
#define R_RISCV_ADD16 34        // Add 16 bits.
#define R_RISCV_ADD32 35        // Add 32 bits.
#define R_RISCV_ADD64 36        // Add 64 bits.
#define R_RISCV_SUB8 37         // Subtract 8 bits.
#define R_RISCV_SUB16 38        // Subtract 16 bits.
#define R_RISCV_SUB32 39        // Subtract 32 bits.
#define R_RISCV_SUB64 40        // Subtract 64 bits.
#define R_RISCV_ALIGN 43        // Align
#define R_RISCV_RVC_BRANCH 44   // Branch.
#define R_RISCV_RVC_JUMP 45     // Jump.
#define R_RISCV_RVC_LUI 46      // Address.
#define R_RISCV_TPREL_I 49
#define R_RISCV_TPREL_S 50
#define R_RISCV_RELAX 51        // Relax instruction pair.

// 6502 processor
#define R_6502_JSR 1         // Call direct to symbol.
#define R_6502_JMP 2         // Move symbol address to reg.
#define R_6502_DATA16 3      // 16-bit data.
#define R_6502_JSR_PLT 4    // Call via PLT.
#define R_6502_GOT_ENTRY 5  // Address of GOT entry for data.
#define R_6502_GOT_DATA 6       // Value of data in GOT.
#define R_6502_GOT_FUNC 7       // Value of function in GOT.
#define R_6502_PCREL 8        // PC relative address.
#define R_6502_GOT_TLS_IE 9    // TLS IE GOT entry.
#define R_6502_GOT_TLS_GD 10    // TLS GD GOT entry.
#define R_6502_TLS_TP_OFF 11    // TLS Thread pointer offset.
#define R_6502_GOT_TLS_OFFSET 12
#define R_6502_GOT_TLS_MODID 13
#define R_6502_ADD16 14       // Add 16-bit.
#define R_6502_ADD32 15       // Add 32-bit.
#define R_6502_ADD64 16       // Add 64-bit.
#define R_6502_SUB16 17       // Subtract 16-bit.
#define R_6502_SUB32 18      // Subtract 32-bit.
#define R_6502_SUB64 19      // Subtract 64-bit.
#define R_6502_DATA32 21      // 32-bit data.
#define R_6502_DATA64 22      // 64-bit data.
#define R_6502_BYTE0 23
#define R_6502_BYTE1 24
#define R_6502_BYTE2 25
#define R_6502_BYTE3 26
#define R_6502_BYTE4 27
#define R_6502_BYTE5 28
#define R_6502_BYTE6 29
#define R_6502_BYTE7 30

#define ELF_MACHINE_TYPE_PCODE 6500
#define ELF_MACHINE_TYPE_RISC_V 243
#define ELF_MACHINE_TYPE_6502 6502

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
  SHT(gnu_hash) = 0x6ffffff6,     // GNU hash table.
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
  SHF(tls) = (1 << 10),              // Thread Local Storage.
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

// Dynamic section.

// Dynamic section tags.
#define DT(x) kELFDynamicTag_##x
typedef enum {
  DT(null)        = 0,
  DT(needed)      = 1,
  DT(pltrelsz)    = 2,
  DT(pltgot)      = 3,
  DT(hash)        = 4,
  DT(strtab)      = 5,
  DT(symtab)      = 6,
  DT(rela)        = 7,
  DT(relasz)      = 8,
  DT(relaent)     = 9,
  DT(strsz)       = 10,
  DT(syment)      = 11,
  DT(init)        = 12,
  DT(fini)        = 13,
  DT(soname)      = 14,
  DT(rpath)       = 15,
  DT(symbolic)    = 16,
  DT(rel)         = 17,
  DT(relsz)       = 18,
  DT(relent)      = 19,
  DT(pltrel)      = 20,
  DT(debug)       = 21,
  DT(textrel)     = 22,
  DT(jmprel)      = 23,
  DT(bind_now)    = 24,
  DT(init_array)  = 25,
  DT(fini_array)  = 26,
  DT(init_arraysz) = 27,
  DT(fini_arraysz) = 28,
  DT(runpath)     = 29,
  DT(flags)       = 30,
  DT(preinit_array) = 32,
  DT(preinit_arraysz) = 33,
  DT(maxpostags)  = 34,
  // OS extensions in here.
  DT(checksum)    = 0x6ffffdf8,
  DT(pltpadsz)    = 0x6ffffdf9,
  DT(moveent)     = 0x6ffffdfa,
  DT(movsz)       = 0x6ffffdfb,
  DT(posflag_1)   = 0x6ffffdfd,
  DT(syminsz)     = 0x6ffffdfe,
  DT(syminent)    = 0x6ffffdff,
  DT(gnu_hash)    = 0x6ffffef5,
  DT(config)      = 0x6ffffefa,
  DT(depaudit)    = 0x6ffffefb,
  DT(audit)       = 0x6ffffefc,
  DT(pltpad)      = 0x6ffffefd,
  DT(movetab)     = 0x6ffffefe,
  DT(syminfo)     = 0x6ffffeff,
  DT(relacount)   = 0x6ffffff9,
  DT(relcount)    = 0x6ffffffa,
  DT(flags_1)     = 0x6ffffffb,
  DT(verdef)      = 0x6ffffffc,
  DT(verdefnum)   = 0x6ffffffd,
  DT(verneed)     = 0x6ffffffe,
  DT(verneednum)  = 0x6fffffff,
  DT(auxilliary)  = 0x7ffffffd,
  DT(used)        = 0x7ffffffe,
  DT(filter)      = 0x7fffffff,
} ELFDynamicTag;

typedef struct {
  ELF_Xword tag;
  union {
    ELF_Xword val;
    ELF_Addr ptr;
  } un;
} ELFDynamicSectionEntry;

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
