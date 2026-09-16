//
//  main.c
//  elfdump
//
//  Handy ELF file dumper.
//
//  Created by David Allison on 1/14/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "disassembler.h"
#include "elf_reader.h"

static void Usage(FILE* fp, const char* program_name) {
  fprintf(fp,
          "usage: %s [option] filename\n"
          "\n"
          "Dump information from an ELF file.\n"
          "\n"
          "Options:\n"
          "  -h, --help       Show this help message\n"
          "  -H               Print ELF header (default)\n"
          "  -S               Print sections\n"
          "  -s               Print symbols\n"
          "  -l               Print program headers / segments\n"
          "  -r               Print relocations\n"
          "  -d               Print dynamic section\n"
          "  -c               Disassemble code sections\n"
          "  -x SECTION       Hex dump section number SECTION\n"
          "  -a               Print all standard sections\n",
          program_name);
}

static const char* ProgramHeaderType(int t) {
  const char* type = "unknown";
  switch (t) {
    case PT(null):
      type = "null";
      break;
    case PT(load):        // Loadable segment.
      type = "load";
      break;
    case PT(dynamic):     // DYNAMIC segment (for .so files)
      type = "dynamic";
      break;
    case PT(interp):      // Program interpreter.
      type = "interp";
      break;
    case PT(note):        // General note.
      type = "note";
      break;
    case PT(shlib):       // Shared library.
      type = "shlib";
      break;
    case PT(phdr):        // Program header
      type = "phdr";
      break;
    case PT(tls):         // Thread local storage.
      type = "tls";
      break;
  }
  return type;
}

static void PrintHeader(ELFReaderFile* elf) {
  ELFHeader* header = elf->header;
  const char* type = "unknown";
  const char* machine = "unknown";
  switch (header->machine) {
    case ELF_MACHINE_TYPE_PCODE:
      machine = "P-code";
      break;
    case ELF_MACHINE_TYPE_RISC_V:
      machine = "RISC-V";
      break;
    case ELF_MACHINE_TYPEW65C02:
      machine = "6502";
      break;
    case ELF_MACHINE_TYPE_AARCH64:
      machine = "AARCH64";
      break;
  }
  printf("Machine:\t%s\n", machine);
  switch (header->type) {
    case ET(none):   // No type specified.
      break;
    case ET(rel):    // File is relocatable (an object file, typically)
      type = "REL";
      break;
    case ET(exec):   // File is executable (a binary).
      type = "EXEC";
      break;
    case ET(dyn):    // File is a dynamic executable (shared object).
      type = "DYN";
      break;
    case ET(core):   // File is a core dump file.
      type = "CORE";
      break;
  }
  printf("Type:\t%s\n", type);
  
  String section_names = {0};
  for (size_t si = 0; si < elf->sections.length; si++) {
    ELFReaderSection* section = elf->sections.value.p[si];
    StringPrintf(&section_names, "  %zd: %s\n", si, section->name.value);
  }
  printf("Sections: %d\n%s\n", (int)header->shnum, section_names.value);
  StringDestruct(&section_names);
 
  String segment_types = {0};
  for (size_t si = 0; si < elf->segments.length; si++) {
    ELFProgramHeader* segment = elf->segments.value.p[si];
    StringPrintf(&segment_types, "  %zd: %s\n", si, ProgramHeaderType(segment->type));
  }

  printf("Segments: %d\n%s\n", (int)header->phnum, segment_types.value);
  StringDestruct(&segment_types);
}

static void AddFlag(ELF_Xword flags, ELF_Xword flag, const char* name, String* output) {
  if ((flags & flag) != 0) {
    StringAppend(output, name);
  }
}

static void PrintSections(ELFReaderFile* elf) {
  for (size_t i = 0; i < elf->sections.length; i++) {
    ELFReaderSection* section = elf->sections.value.p[i];
    const char* type = "unknown";
    switch (section->header->type) {
      case SHT(null):         // No type.
        type = "null";
        break;
      case SHT(progbits):     // Contains code or data.
          type = "progbits";
          break;
      case SHT(symtab):       // Symbol table.
          type = "symtab";
          break;
      case SHT(strtab):       // String table.
          type = "strtab";
          break;
      case SHT(rela):         // Relocations with addend present.
          type = "rela";
          break;
      case SHT(hash):         // A symbol hash table.
          type = "hash";
          break;
      case SHT(dynamic):      // Dynamic section for shared objects.
          type = "dynamic";
          break;
      case SHT(note):         // A general note.
          type = "note";
          break;
      case SHT(nobits):       // No data is in section.
          type = "nobits";
          break;
      case SHT(rel):          // Relocations without addend.
          type = "rel";
          break;
      case SHT(shlib):        // Shared library information.
          type = "shlib";
          break;
      case SHT(dynsym):       // Dynamic symbol table.
          type = "dynsym";
          break;
      case SHT(init_array):
          type = "init_array";
          break;
      case SHT(fini_array):
          type = "fini_array";
          break;
      case SHT(preinit_array):
          type = "preinit_array";
          break;
      case SHT(gnu_hash):     // GNU hash table.
         type = "gnu_hash";
         break;
      case SHT(ARM_EXIDX):
         type = "ARM_EXIDX";
         break;
    }
    
    String flags;
    StringInit(&flags, "");

    AddFlag(section->header->flags, SHF(write), "w", &flags);
    AddFlag(section->header->flags, SHF(alloc), "a", &flags);
    AddFlag(section->header->flags, SHF(execinstr), "x", &flags);
    AddFlag(section->header->flags, SHF(merge), "m", &flags);
    AddFlag(section->header->flags, SHF(strings), "s", &flags);
    AddFlag(section->header->flags, SHF(tls), "t", &flags);
    AddFlag(section->header->flags, SHF(link_order), "l", &flags);
    
    ELFReaderSection* link = elf->sections.value.p[section->header->link];
    printf("%3zd: %-20s %-8s %-4s %08" PRIx64 " %08" PRIx64 " %08" PRIx64 " %-8s %-3d\n", i, section->name.value, type, flags.value,
           section->header->offset, section->header->addr, section->header->size,
           link->name.value, section->header->info);
    StringDestruct(&flags);
  }
}

static void PrintSegments(ELFReaderFile* elf) {
  for (size_t i = 0; i < elf->segments.length; i++) {
    ELFProgramHeader* segment = elf->segments.value.p[i];
    const char* type = ProgramHeaderType(segment->type);
 
    String flags = {0};

    AddFlag(segment->flags, PF(w), "w", &flags);
    AddFlag(segment->flags, PF(r), "r", &flags);
    AddFlag(segment->flags, PF(x), "x", &flags);
    
    String info = {0};
    
    switch (segment->type) {
      case PT(load): {
        uint64_t start = segment->offset;
        uint64_t end = start + segment->filesz;
        const char* sep = "";
        
        for (size_t si = 0; si < elf->sections.length; si++) {
          ELFReaderSection* section = elf->sections.value.p[si];
          uint64_t section_start = section->header->offset;
          uint64_t section_end = section_start + section->header->size;
          if (section_start >= start && section_end < end) {
            StringPrintf(&info, "%s%s", sep, section->name.value);
            sep = " ";
          }
        }
        break;
      }
      case PT(interp): {
        const char* interpreter = (const char*)elf->header + segment->offset;
        StringPrintf(&info, "%s", interpreter);
        break;
      }
      case PT(dynamic):
        StringPrintf(&info, "%zd entries", segment->filesz / sizeof(ELFDynamicSectionEntry));
        break;
    }
    printf("%3zd: %-8s %-4s %08" PRIx64 " %08" PRIx64 " %08" PRIx64 " %08" PRIx64 " %08" PRIx64 " %s\n", i, type, flags.value,
           segment->offset, segment->vaddr,
           segment->filesz, segment->memsz,
           segment->align,
           info.value);
    StringDestruct(&flags);
    StringDestruct(&info);
  }
}

static void PrintSymbol(ELFReaderFile* elf,
                        ELFSymbol* sym, size_t index, ELFReaderSection* strtab) {
  int64_t info = sym->info;
  int64_t binding = ELF_ST_BIND(info);
  const char* sym_name = (const char*)strtab->contents + sym->name;
  const char* bind = "";
  switch (binding) {
    case STB(local):
      bind = "local";
      break;
    case STB(global):
      bind = "global";
      break;
    case STB(weak):
      bind = "weal";
      break;
  }
  const char* type = "notype";
  switch (ELF_ST_TYPE(info)) {
    case STT(notype):     // No type specified.
      break;
    case STT(object):     // Symbol is an object (a variable).
      type = "object";
      break;
    case STT(func):       // A function (always take address).
      type = "func";
      break;
    case STT(section):    // A section name symbol.
      type = "section";
      break;
    case STT(file):       // A file name symbol.
      type = "file";
      break;
    case STT(common):     // A common symbol (might be merged with another symbol).
      type = "common";
      break;
    case STT(tls):        // Thread local storage.
      type = "tls";
      break;
  }
  
  const char* section_name = "unknown";
  if (sym->shndx == SHN_ABS) {
    section_name = "<abs>";
  } else if (sym->shndx == SHN_COM) {
    section_name = "<com>";
  } else {
    if (sym->shndx < elf->sections.length) {
      ELFReaderSection* section = elf->sections.value.p[sym->shndx];
      section_name = section->name.value;
    }
  }
  
  printf("%3zd: %08" PRIx64 " %5" PRId64 " %-8s %-6s %-8s(%d) %s\n", index,
         sym->value, sym->size,
         type, bind, section_name, sym->shndx, sym_name);
  
}

static void PrintSymbols(ELFReaderFile* elf) {
  Vector symbol_tables;
  VectorInit(&symbol_tables);
  
  // Find symbol and string tables.
  ELFReaderFileFindSectionsByType(elf, SHT(symtab), &symbol_tables);
  ELFReaderFileFindSectionsByType(elf, SHT(dynsym), &symbol_tables);

  for (size_t i = 0; i < symbol_tables.length; i++) {
    ELFReaderSection* symtab = symbol_tables.value.p[i];
    printf("symtab: %s\n", symtab->name.value);
    int strtab_index = symtab->header->link;
    if (strtab_index >= elf->sections.length) {
      // Invalid string table.
      continue;
    }
    ELFReaderSection* strtab = elf->sections.value.p[strtab_index];
    size_t num_symbols = symtab->header->size / symtab->header->entsize;
    const char* symbol_addr = elf->base + symtab->header->offset;
    
    for (size_t i = 0; i < num_symbols; i++) {
      ELFSymbol sym;
      elf->ops->ReadSymbol(&sym, symbol_addr);
      PrintSymbol(elf, &sym, i, strtab);
      symbol_addr += symtab->header->entsize;
    }

  }
  VectorDestruct(&symbol_tables);
}

static const char* RISCVRelocType(int32_t reloc_type) {
  const char* type = "unknown";
   switch (reloc_type) {
     case R_RISCV_NONE:       // No action.
       type = "R_RISCV_NONE";
       break;
     case R_RISCV_32:          // Add 32 bit symbol value.
       type = "R_RISCV_32";
       break;
     case R_RISCV_64:          // Add 64 bit symbol value.
       type = "R_RISCV_64";
       break;
     case R_RISCV_RELATIVE:    // Add load address of shared object.
       type = "R_RISCV_RELATIVE";
       break;
     case R_RISCV_COPY:        // Copy data from shared object.
       type = "R_RISCV_COPY";
       break;
     case R_RISCV_JUMP_SLOT:   // Set GOT entry.
       type = "R_RISCV_JUMP_SLOT";
       break;
     case R_RISCV_TLS_DTPMOD32:  // TLS DTV module ID
       type = "R_RISCV_TLS_DTPMOD32";
       break;
     case R_RISCV_TLS_DTPMOD64:  // TLS DTV module ID
       type = "R_RISCV_TLS_DTPMOD64";
       break;
     case R_RISCV_TLS_DTPREL32:  // TLS
       type = "R_RISCV_TLS_DTPREL32";
       break;
     case R_RISCV_TLS_DTPREL64:
       type = "R_RISCV_TLS_DTPREL64";
       break;
     case R_RISCV_TLS_TPREL32:
       type = "R_RISCV_TLS_TPREL32";
       break;
     case R_RISCV_TLS_TPREL64:
       type = "R_RISCV_TLS_TPREL64";
       break;
     case R_RISCV_BRANCH :     // PC relative branch.
       type = "R_RISCV_BRANCH";
       break;
     case R_RISCV_JAL:        // PC relative jump.
       type = "R_RISCV_JAL";
       break;
     case R_RISCV_CALL:       // PC relative call.
       type = "R_RISCV_CALL";
       break;
     case R_RISCV_CALL_PLT:   // PC relative call via PLT.
       type = "R_RISCV_CALL_PLT";
       break;
     case R_RISCV_GOT_HI20:   // PC relative GOT high 20 bits.
       type = "R_RISCV_GOT_HI20";
       break;
     case R_RISCV_TLS_GOT_HI20: // TLS IE high 20 bits.
       type = "R_RISCV_TLS_GOT_HI20";
       break;
     case R_RISCV_TLS_GD_HI20:  // TLS GD high 20.
       type = "R_RISCV_TLS_GD_HI20";
       break;
     case R_RISCV_PCREL_HI20:   // PC relative high 20 bits.
       type = "R_RISCV_PCREL_HI20";
       break;
     case R_RISCV_PCREL_LO12_I: // PC relative low 12 bits (I-type)
       type = "R_RISCV_PCREL_LO12_I";
       break;
     case R_RISCV_PCREL_LO12_S: // PC relaitve low 12 bits (S-type)
       type = "R_RISCV_PCREL_LO12_S";
       break;
     case R_RISCV_HI20:         // Absolute high 20 bits.
       type = "R_RISCV_HI20";
       break;
     case R_RISCV_LO12_I:       // Absolute low 12 bits (I-type)
       type = "R_RISCV_LO12_I";
       break;
     case R_RISCV_LO12_S:       // Absolute low 12 bits (S-type)
       type = "R_RISCV_LO12_S";
       break;
     case R_RISCV_TPREL_HI20:
       type = "R_RISCV_TPREL_HI20";
       break;
     case R_RISCV_TPREL_LO12_I:
       type = "R_RISCV_TPREL_LO12_I";
       break;
     case R_RISCV_TPREL_LO12_S:
       type = "R_RISCV_TPREL_LO12_S";
       break;
     case R_RISCV_TPREL_ADD:
       type = "R_RISCV_TPREL_ADD";
       break;
     case R_RISCV_ADD8:         // Add 8 bits.
       type = "R_RISCV_ADD8";
       break;
     case R_RISCV_ADD16:        // Add 16 bits.
       type = "R_RISCV_ADD16";
       break;
     case R_RISCV_ADD32:        // Add 32 bits.
       type = "R_RISCV_ADD32";
       break;
     case R_RISCV_ADD64:        // Add 64 bits.
       type = "R_RISCV_ADD64";
       break;
     case R_RISCV_SUB8:         // Subtract 8 bits.
       type = "R_RISCV_SUB8";
       break;
     case R_RISCV_SUB16:        // Subtract 16 bits.
       type = "R_RISCV_SUB16";
       break;
     case R_RISCV_SUB32:        // Subtract 32 bits.
       type = "R_RISCV_SUB32";
       break;
     case R_RISCV_SUB64:        // Subtract 64 bits.
       type = "R_RISCV_SUB64";
       break;
     case R_RISCV_ALIGN:        // Align
       type = "R_RISCV_ALIGN";
       break;
     case R_RISCV_RVC_BRANCH:   // Branch.
       type = "R_RISCV_RVC_BRANCH";
       break;
     case R_RISCV_RVC_JUMP:     // Jump.
       type = "R_RISCV_RVC_JUMP";
       break;
     case R_RISCV_RVC_LUI:      // Address.
       type = "R_RISCV_RVC_LUI";
       break;
     case R_RISCV_TPREL_I:
       type = "R_RISCV_TPREL_I";
       break;
     case R_RISCV_TPREL_S:
       type = "R_RISCV_TPREL_S";
       break;
     case R_RISCV_RELAX:        // Relax instruction pair.
       type = "R_RISCV_RELAX";
       break;
   }
  return type;
}

static const char* W65C02RelocType(int32_t reloc_type) {
  switch (reloc_type) {
    case R_W65C02_JSR:
      return "R_W65C02_JSR";
    case R_W65C02_JMP:
      return "R_W65C02_JMP";
    case R_W65C02_DATA16:
      return "R_W65C02_DATA16";
    case R_W65C02_DATA32:
      return "R_W65C02_DATA32";
    case R_W65C02_DATA64:
      return "R_W65C02_DATA64";
    case R_W65C02_BYTE0:
      return "R_W65C02_BYTE0";
    case R_W65C02_BYTE1:
      return "R_W65C02_BYTE1";
    case R_W65C02_BYTE2:
      return "R_W65C02_BYTE2";
    case R_W65C02_BYTE3:
      return "R_W65C02_BYTE3";
    case R_W65C02_BYTE4:
      return "R_W65C02_BYTE4";
    case R_W65C02_BYTE5:
      return "R_W65C02_BYTE5";
    case R_W65C02_BYTE6:
      return "R_W65C02_BYTE6";
    case R_W65C02_BYTE7:
      return "R_W65C02_BYTE7";
    case R_W65C02_ADD8:
      return "R_W65C02_ADD8";
    case R_W65C02_ADD16:
      return "R_W65C02_ADD16";
    case R_W65C02_ADD32:
      return "R_W65C02_ADD32";
    case R_W65C02_ADD64:
      return "R_W65C02_ADD64";
    case R_W65C02_SUB8:
      return "R_W65C02_SUB8";
    case R_W65C02_SUB16:
      return "R_W65C02_SUB16";
    case R_W65C02_SUB32:
      return "R_W65C02_SUB32";
    case R_W65C02_SUB64:
      return "R_W65C02_SUB64";
  }
  return "unknown";
}

static const char* ARMRelocType(int32_t reloc_type) {
  switch (reloc_type) {
    case R_ARM_NONE:
      return "R_ARM_NONE";
    case R_ARM_ABS32:
      return "R_ARM_ABS32";
    case R_ARM_REL32:
      return "R_ARM_REL32";
    case R_ARM_TARGET2:
      return "R_ARM_TARGET2";
    case R_ARM_PREL31:
      return "R_ARM_PREL31";
    case R_ARM_CALL:
      return "R_ARM_CALL";
    case R_ARM_JUMP24:
      return "R_ARM_JUMP24";
    case R_ARM_PC24:
      return "R_ARM_PC24";
    case R_ARM_LDR_PC_G0:
      return "R_ARM_LDR_PC_G0";
    case R_ARM_ADD32:
      return "R_ARM_ADD32";
    case R_ARM_SUB32:
      return "R_ARM_SUB32";
    case R_ARM_TLS_DTPMOD32:
      return "R_ARM_TLS_DTPMOD32";
    case R_ARM_TLS_DTPREL32:
      return "R_ARM_TLS_DTPREL32";
    case R_ARM_TLS_TPOFF32:
      return "R_ARM_TLS_TPOFF32";
    case R_ARM_COPY:
      return "R_ARM_COPY";
    case R_ARM_GLOB_DAT:
      return "R_ARM_GLOB_DAT";
    case R_ARM_JUMP_SLOT:
      return "R_ARM_JUMP_SLOT";
    case R_ARM_RELATIVE:
      return "R_ARM_RELATIVE";
    case R_ARM_GOT_BREL:
      return "R_ARM_GOT_BREL";
    case R_ARM_PLT32:
      return "R_ARM_PLT32";
    case R_ARM_TARGET1:
      return "R_ARM_TARGET1";
    case R_ARM_V4BX:
      return "R_ARM_V4BX";
    case R_ARM_MOVW_ABS_NC:
      return "R_ARM_MOVW_ABS_NC";
    case R_ARM_MOVT_ABS:
      return "R_ARM_MOVT_ABS";
    case R_ARM_MOVW_PREL_NC:
      return "R_ARM_MOVW_PREL_NC";
    case R_ARM_MOVT_PREL:
      return "R_ARM_MOVT_PREL";
    case R_ARM_GOT_PREL:
      return "R_ARM_GOT_PREL";
    case R_ARM_TLS_LE32:
      return "R_ARM_TLS_LE32";
    default:
      return "unknown";
  }
}

static void PrintRelocation(ELFReaderFile* elf, size_t i, ELFRelocation* reloc,
                            const char* symbol_table_address,
                            ELFReaderSection* reloc_section,
                            ELFReaderSection* symtab,
                            ELFReaderSection* strtab) {
  int num_symbols = (int)symtab->header->size / symtab->header->entsize;
  int32_t symbol_index = ELF_R_SYM(reloc->info);
  bool bad_symbol = false;
  if (symbol_index < 0 || symbol_index >= num_symbols) {
    bad_symbol = true;
  }
  int32_t reloc_type = ELF_R_TYPE(reloc->info);
  int64_t addend = reloc_section->header->type == SHT(rela) ? reloc->addend : 0;
  
  // Decode the referenced symbol from the on-disk symbol table.  For ELF32 the
  // on-disk symbol is narrower than the canonical struct, so it has to go
  // through the format ops rather than being cast in place.
  ELFSymbol sym_storage;
  ELFSymbol* elf_sym = NULL;
  if (!bad_symbol) {
    const char* sym_addr =
        symbol_table_address + symbol_index * symtab->header->entsize;
    if (elf->ops->is_64_bit) {
      elf_sym = (ELFSymbol*)sym_addr;
    } else {
      elf->ops->ReadSymbol(&sym_storage, sym_addr);
      elf_sym = &sym_storage;
    }
  }

  // Get the target section index from the info field in the section header.
  int target_section_index = reloc_section->header->info;
  if (target_section_index < 0 || target_section_index >= elf->sections.length) {
    return;
  }
  const char* string_table_address = (const char*)strtab->contents;

  String type = {0};
  switch (elf->header->machine) {
    case ELF_MACHINE_TYPE_PCODE:
      StringPrintf(&type, "%08x", reloc_type);
       break;
    case ELF_MACHINE_TYPE_RISC_V:
      StringSet(&type, RISCVRelocType(reloc_type));
      break;
    case ELF_MACHINE_TYPEW65C02:
      StringSet(&type, W65C02RelocType(reloc_type));
      break;
    case ELF_MACHINE_TYPE_ARM:
      StringSet(&type, ARMRelocType(reloc_type));
      break;
    default:
      StringPrintf(&type, "%08x", reloc_type);
      break;
  }
  String sym_name = {0};
  if (bad_symbol) {
    StringPrintf(&sym_name, "<bad symbol index 0x%x>", symbol_index);
  } else {
    StringInit(&sym_name, string_table_address + elf_sym->name);
  }
  ELFReaderSection* target_section = elf->sections.value.p[target_section_index];
  printf("%3zd: %-8s %08" PRIx64 " %-24s %-16s\t", i, target_section->name.value,
         reloc->offset, type.value,
         sym_name.value);
  if (addend != 0) {
    printf(" + %" PRId64 "", addend);
  }
  if (bad_symbol) {
    printf("\n");
    StringDestruct(&type);
    StringDestruct(&sym_name);
    return;
  }
  DAsmArchitecture arch;
  if ((target_section->header->flags & SHF(execinstr)) != 0 &&
      DAsmArchitectureFromELFMachine(elf->header->machine, &arch)) {
    DAsmInstruction inst;
    const void* target = (const char*)target_section->contents + reloc->offset;
    uint64_t addr = target_section->header->addr + reloc->offset;
    if (DAsmDisassembleInstruction(arch, target,
                                   target_section->header->size - reloc->offset,
                                   addr, &inst)) {
      printf("%s\n", inst.text);
    } else {
      printf("\n");
    }
  } else {
    printf("\n");
  }
  StringDestruct(&type);
  StringDestruct(&sym_name);
}

static void PrintRelocations(ELFReaderFile* elf) {
  Vector relocation_sections;
  VectorInit(&relocation_sections);
  
  // Find all relocation sections.  Finds both REL and RELA sections.
  ELFReaderFileFindSectionsByType(elf, SHT(rela), &relocation_sections);
  ELFReaderFileFindSectionsByType(elf, SHT(rel), &relocation_sections);
  
  // Process all relocation sections.
  for (size_t i = 0; i < relocation_sections.length; i++) {
    ELFReaderSection* reloc_section = relocation_sections.value.p[i];
    printf("Section: %s\n", reloc_section->name.value);
    int32_t symtab_section_index = reloc_section->header->link;
    if (symtab_section_index == 0 || symtab_section_index >= elf->sections.length) {
      continue;
    }
    ELFReaderSection* symtab = elf->sections.value.p[symtab_section_index];
    
    if (symtab->header->link >= elf->sections.length) {
      continue;
    }
    ELFReaderSection* strtab = elf->sections.value.p[symtab->header->link];
    // elf->header is a canonical wide structure, which is only the start of the
    // mapping for ELF64; file offsets have to be taken from elf->base.
    const char* symbol_table_address = elf->base + symtab->header->offset;

    int64_t num_relocations = reloc_section->header->size /
      reloc_section->header->entsize;
    
    const char* reloc_addr = elf->base + reloc_section->header->offset;
    ELFRelocation reloc_storage;
    for (int64_t ri = 0; ri < num_relocations; ri++) {
      ELFFormatReadRelocation(
          elf->ops, reloc_section->header->type == SHT(rela),
          &reloc_storage, reloc_addr);
      PrintRelocation(elf, ri, &reloc_storage, symbol_table_address,
                     reloc_section, symtab, strtab);
      reloc_addr += reloc_section->header->entsize;
    }
  }
  VectorDestruct(&relocation_sections);
}

static void Disassemble(ELFReaderFile* elf) {
  DAsmOptions options = {
      .arch = kDAsmUnknown,
      .print_section_names = true,
  };
  DAsmDisassembleELF(elf, &options, stdout);
}

enum Command {
  kAll,
  kHeader,
  kSections,
  kSymbols,
  kSegments,
  kRelocations,
  kDynamic,
  kDisassemble,
  kHexDump,
  kLast,
};

// Decode the dynamic section entry at 'p' into the canonical (ELF64) form.  An
// ELF32 entry is half the width, so it can neither be read in place nor strided
// over with sizeof.
static void ReadDynamicEntry(ELFReaderFile* elf, const char* p,
                             ELFDynamicSectionEntry* out) {
  if (elf->ops->is_64_bit) {
    memcpy(out, p, sizeof(ELF64DynamicSectionEntry));
    return;
  }
  ELF32DynamicSectionEntry in;
  memcpy(&in, p, sizeof(in));
  out->tag = in.tag;
  out->un.val = in.un.val;
}

// DT tag number to name.
static const char* EntryTagName(ELFDynamicSectionEntry* entry) {
  const char* name = "unknown";
  switch (entry->tag) {
    case DT(null):
      name = "NULL";
      break;
    case DT(needed):
      name = "NEEDED";
      break;
    case DT(pltrelsz):
      name = "PLTRELSZ";
      break;
    case DT(pltgot):
      name = "PLTGOT";
      break;
    case DT(hash):
      name = "HASH";
      break;
    case DT(strtab):
      name = "STRTAB";
      break;
    case DT(symtab):
      name = "SYMTAB";
      break;
    case DT(rela):
      name = "RELA";
      break;
    case DT(relasz):
      name = "RELASZ";
      break;
    case DT(relaent):
       name = "RELAENT";
       break;
     case DT(strsz):
       name = "STRSZ";
       break;
     case DT(syment):
       name = "SYMENT";
       break;
     case DT(init):
       name = "INIT";
       break;
     case DT(fini):
       name = "FINI";
       break;
     case DT(soname):
       name = "SONAME";
       break;
     case  DT(rpath):
       name = "RPATH";
       break;
     case DT(symbolic):
       name = "SYMBOLIC";
       break;
     case DT(rel):
       name = "REL";
       break;
     case DT(relsz):
       name = "RELSZ";
       break;
     case DT(relent):
       name = "RELENT";
       break;
     case DT(pltrel):
       name = "PLTREL";
       break;
     case DT(debug):
       name = "DEBUG";
       break;
     case DT(textrel):
       name = "TEXTREL";
       break;
     case DT(jmprel):
       name = "JMPREL";
       break;
     case DT(bind_now):
       name = "BIND_NOW";
       break;
     case DT(init_array):
       name = "INIT_ARRAY";
       break;
     case DT(fini_array):
       name = "FINI_ARRAY";
       break;
     case DT(init_arraysz):
       name = "INIT_ARRAYSZ";
       break;
     case DT(fini_arraysz):
       name = "FINI_ARRAYSZ";
       break;
     case DT(runpath):
       name = "RUNPATH";
       break;
     case DT(flags):
       name = "FLAGS";
       break;
     case DT(preinit_array):
       name = "PREINIT_ARRAY";
       break;
     case DT(preinit_arraysz):
       name = "PREINIT_ARRAYSZ";
       break;
     case DT(maxpostags):
       name = "MAXPOSTAGS";
       break;
     case DT(checksum):
       name = "CHECKSUM";
       break;
     case DT(pltpadsz):
       name = "PLTPADSZ";
       break;
     case DT(moveent):
       name = "MOVEENT";
       break;
     case DT(movsz):
       name = "MOVESZ";
       break;
     case DT(posflag_1):
       name = "POSFLAG_1";
       break;
     case DT(syminsz):
       name = "SYMINSZ";
       break;
     case DT(syminent):
       name = "SYNINENT";
       break;
     case DT(gnu_hash):
       name = "GNU_HASH";
       break;
     case DT(config):
       name = "CONFIG";
       break;
     case DT(depaudit):
       name = "DEPAUDIT";
       break;
     case DT(audit):
       name = "AUDIT";
       break;
     case DT(pltpad):
       name = "PLTPAD";
       break;
     case DT(movetab):
       name = "MOVETAB";
       break;
     case DT(syminfo):
       name = "SYMINFO";
       break;
     case DT(relacount):
       name = "RELACOUNT";
       break;
     case DT(relcount):
       name = "RELCOUNT";
       break;
     case DT(flags_1):
       name = "FLAGS_1";
       break;
     case DT(verdef):
       name = "VERDEF";
       break;
     case DT(verdefnum):
       name = "VERDEFNUM";
       break;
     case DT(verneed):
       name = "VERNEED";
       break;
     case DT(verneednum):
       name = "VERNEEDNUM";
       break;
     case DT(auxilliary):
       name = "AUXILLIARY";
       break;
     case DT(used):
       name = "USED";
      break;
     case DT(filter):
      name = "FILTER";
      break;
  }
  return name;
}

static void PrintDynamicSection(ELFReaderFile* elf) {
  Vector dynamic_sections;
  VectorInit(&dynamic_sections);

  ELFReaderFileFindSectionsByType(elf, SHT(dynamic), &dynamic_sections);
  
  for (size_t i = 0; i < dynamic_sections.length; i++) {
    ELFReaderSection* dynamic_section = dynamic_sections.value.p[i];

    // DT_STRTAB is a virtual address, and turning one back into a position in
    // the file needs the section table anyway, so just use the section.
    ELFReaderSection* dynstr = ELFReaderFileFindSection(elf, ".dynstr");
    const char* strtab_addr = dynstr != NULL ? dynstr->contents : NULL;

    size_t entry_size = elf->ops->dynamic_entry_size;
    const char* start = dynamic_section->contents;
    const char* end = start + dynamic_section->header->size;

    const char* p = start;
    while (p + entry_size <= end) {
      ELFDynamicSectionEntry storage;
      ReadDynamicEntry(elf, p, &storage);
      ELFDynamicSectionEntry* entry = &storage;
      printf("  %-16s", EntryTagName(entry));
      switch (entry->tag) {
        case DT(null):
          printf("\n");
          break;
        // Strings.
        case DT(needed):
        case DT(soname):
        case DT(rpath):
        case DT(runpath):
          if (strtab_addr == NULL) {
            printf("0x%" PRIx64 " (no .dynstr)\n", entry->un.val);
          } else {
            printf("%s\n", strtab_addr + entry->un.val);
          }
          break;
        default:
          // Integers
           printf("%" PRId64 "\n", entry->un.val);
          break;
          
        // Hex.
        case DT(pltgot):
        case DT(rela):
        case DT(init):
        case DT(fini):
        case DT(pltrel):
        case DT(jmprel):
        case DT(init_array):
        case DT(fini_array):
        case DT(preinit_array):
        case DT(gnu_hash):
        case DT(strtab):
        case DT(symtab):
          printf("0x%" PRIx64 "\n", entry->un.ptr);
          break;
      }
      p += entry_size;
    }
  }
  
  VectorDestruct(&dynamic_sections);
}

static void Hexdump(const void* addr, int size) {
  char buf[16];
  const char* caddr = (const char*)addr;
  const char* endaddr = caddr + size;
  int len = size;
  while (len > 0) {
    int filled_bytes = 0;
    for (int i = 0; i < 16; i++) {
      if (caddr < endaddr) {
         buf[i] = *caddr++;
        filled_bytes++;
        printf("%02X ", buf[i] & 0xff);
      } else {
        printf("   ");
      }
    }
    printf("  ");
    for (int i = 0; i < filled_bytes; i++) {
      if (buf[i] >= 0x20 && buf[i] < 0x7f) {
        printf("%c", buf[i]);
      } else {
        printf("%c", '.');
      }
    }
    len -= 16;
    printf("\n");
  }
}

static void DumpSection(ELFReaderFile* elf_file, int section_number) {
  if (section_number < 0 || section_number >= elf_file->sections.length) {
    fprintf(stderr, "Invalid section number %d\n", section_number);
    exit(1);
  }
  ELFReaderSection* section = elf_file->sections.value.p[section_number];
  Hexdump(section->contents, (int)section->header->size);
}

static void RunCommand(ELFReaderFile* elf_file, enum Command command, int command_arg,
                       bool print_name) {
  switch (command) {
    case kHeader:
      if (print_name) {
        printf("Header\n");
      }
      PrintHeader(elf_file);
      break;
    case kSections:
      if (print_name) {
        printf("Sections\n");
      }
      PrintSections(elf_file);
      break;
    case kSymbols:
      if (print_name) {
        printf("Symbols\n");
      }
      PrintSymbols(elf_file);
      break;
    case kSegments:
      if (print_name) {
        printf("Segments\n");
      }
      PrintSegments(elf_file);
      break;
    case kRelocations:
      if (print_name) {
        printf("Relocations\n");
      }
      PrintRelocations(elf_file);
      break;
    case kDynamic:
      if (print_name) {
        printf("Dynamic\n");
      }
      PrintDynamicSection(elf_file);
      break;
    case kDisassemble:
      if (print_name) {
        printf("Disassembly\n");
      }
      Disassemble(elf_file);
      break;
    case kHexDump:
      DumpSection(elf_file, command_arg);
      break;
    case kAll:
    case kLast:
      break;
  }
}

int main(int argc, const char * argv[]) {
  String filename = {0};
  enum Command command = kHeader;
  int command_arg = 0;
  const char* program_name = argc > 0 ? argv[0] : "elfdump";
  
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-help") == 0) {
      Usage(stdout, program_name);
      return 0;
    }
    if (argv[i][0] == '-') {
      // Flags here.
      switch (argv[i][1]) {
        case 'h':
          Usage(stdout, program_name);
          return 0;
        case 'H':
          command = kHeader;
          break;
        case 'S':
          command = kSections;
          break;
        case 's':
          command = kSymbols;
          break;
        case 'l':
          command = kSegments;
          break;
        case 'r':
           command = kRelocations;
           break;
        case 'd':
           command = kDynamic;
           break;
        case 'c':
           command = kDisassemble;
           break;
        case 'x':
          command = kHexDump;
          i++;
          if (i >= argc) {
            fprintf(stderr, "elfdump: -x needs a section number\n\n");
            Usage(stderr, program_name);
            return 1;
          }
          command_arg = atoi(argv[i]);
          break;
        case 'a':
          command = kAll;
          break;
        default:
          fprintf(stderr, "elfdump: unknown option '%s'\n\n", argv[i]);
          Usage(stderr, program_name);
          return 1;
      }
    } else {
      if (filename.length != 0) {
        fprintf(stderr, "elfdump: too many input files: '%s'\n\n", argv[i]);
        Usage(stderr, program_name);
        StringDestruct(&filename);
        return 1;
      }
      StringSet(&filename, argv[i]);
    }
  }
  if (filename.length == 0) {
    fprintf(stderr, "elfdump: missing input file\n\n");
    Usage(stderr, program_name);
    return 1;
  }
  ELFReaderFile* elf_file = NewELFReaderFile(&filename);
  bool ok = ELFReaderFileRead(elf_file, 0, 0);
  if (!ok) {
    fprintf(stderr, "elfdump: unable to open or read ELF file '%s'\n",
            filename.value);
    exit(1);
  }
  if (command == kAll) {
    for (command = kHeader; command < kLast; command++) {
      if (command == kHexDump) {
        // Don't include hexdump.
        continue;
      }
      RunCommand(elf_file, command, -1, true);
    }
  } else {
    RunCommand(elf_file, command, command_arg, false);
  }

  ELFReaderFileDelete(elf_file);
  StringDestruct(&filename);
}
