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
#include "elf_reader.h"
#include "risc_v_interpreter.h"
#include "risc_v_disassembler.h"

static void Usage(void) {
  fprintf(stderr, "usage: elfdump filename\n");
  exit(1);
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
    case ELF_MACHINE_TYPE_6502:
      machine = "6502";
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
  
  String section_names;
  StringInit(&section_names, NULL);
  for (size_t si = 0; si < elf->sections.length; si++) {
    ELFReaderSection* section = elf->sections.value.p[si];
    StringPrintf(&section_names, "  %zd: %s\n", si, section->name.value);
  }
  printf("Sections: %d\n%s\n", (int)header->shnum, section_names.value);
  StringDestruct(&section_names);
 
  String segment_types;
  StringInit(&segment_types, NULL);
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
      case SHT(gnu_hash):     // GNU hash table.
         type = "gnu_hash";
         break;
    }
    
    String flags;
    StringInit(&flags, NULL);

    AddFlag(section->header->flags, SHF(write), "w", &flags);
    AddFlag(section->header->flags, SHF(alloc), "a", &flags);
    AddFlag(section->header->flags, SHF(execinstr), "x", &flags);
    AddFlag(section->header->flags, SHF(merge), "m", &flags);
    AddFlag(section->header->flags, SHF(strings), "s", &flags);
    AddFlag(section->header->flags, SHF(tls), "t", &flags);
    
    ELFReaderSection* link = elf->sections.value.p[section->header->link];
    printf("%3zd: %-20s %-8s %-4s %08llx %08llx %08llx %-8s %-3d\n", i, section->name.value, type, flags.value,
           section->header->addr, section->header->offset, section->header->size,
           link->name.value, section->header->info);
    StringDestruct(&flags);
  }
}

static void PrintSegments(ELFReaderFile* elf) {
  for (size_t i = 0; i < elf->segments.length; i++) {
    ELFProgramHeader* segment = elf->segments.value.p[i];
    const char* type = ProgramHeaderType(segment->type);
 
    String flags;
    StringInit(&flags, NULL);

    AddFlag(segment->flags, PF(w), "w", &flags);
    AddFlag(segment->flags, PF(r), "r", &flags);
    AddFlag(segment->flags, PF(x), "x", &flags);
    
    String info;
    StringInit(&info, NULL);
    
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
    printf("%3zd: %-8s %-4s %08llx %08llx %-6lld %-6lld %s\n", i, type, flags.value,
           segment->offset, segment->vaddr,
           segment->filesz, segment->memsz,
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
    ELFReaderSection* section = elf->sections.value.p[sym->shndx];
    section_name = section->name.value;
  }
  
  printf("%3zd: %08llx %5lld %-8s %-6s %-8s %s\n", index,
         sym->value, sym->size,
         type, bind, section_name, sym_name);
  
}

static void PrintSymbols(ELFReaderFile* elf) {
  Vector symbol_tables;
  VectorInit(&symbol_tables);
  
  // Find symbol and string tables.
  ELFReaderFileFindSectionsByType(elf, SHT(symtab), &symbol_tables);
  
  for (size_t i = 0; i < symbol_tables.length; i++) {
    ELFReaderSection* symtab = symbol_tables.value.p[i];
    int strtab_index = symtab->header->link;
    if (strtab_index >= elf->sections.length) {
      // Invalid string table.
      continue;
    }
    ELFReaderSection* strtab = elf->sections.value.p[strtab_index];
    size_t num_symbols = symtab->header->size / symtab->header->entsize;
    const char* symbol_addr = (const char*)elf->header + symtab->header->offset;
    
    for (size_t i = 0; i < num_symbols; i++) {
      ELFSymbol* sym = (ELFSymbol*)symbol_addr;
      PrintSymbol(elf, sym, i, strtab);
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

static void DisassembleRISCV(void* interpreter, void* addr) {
  DisassembleRiscVInstruction((RISCVInterpreter*)interpreter, addr, stdout);
}

static void PrintRelocation(ELFReaderFile* elf, size_t i, ELFRelocation* reloc,
                            const char* symbol_table_address,
                            ELFReaderSection* reloc_section,
                            ELFReaderSection* symtab,
                            ELFReaderSection* strtab) {
  int32_t symbol_index = ELF_R_SYM(reloc->info);
  int32_t reloc_type = ELF_R_TYPE(reloc->info);
  int64_t addend = reloc_section->header->type == SHT(rela) ? reloc->addend : 0;
  
  ELFSymbol* elf_sym = (ELFSymbol*)(symbol_table_address + symbol_index * symtab->header->entsize);

  // Get the target section index from the info field in the section header.
  int target_section_index = reloc_section->header->info;
  if (target_section_index < 0 || target_section_index >= elf->sections.length) {
    return;
  }
  const char* string_table_address = (const char*)elf->header +
       strtab->header->offset;

  void (*disassembler)(void*, void*) = NULL;
  void *interpreter = NULL;
  String type;
  StringInit(&type, NULL);
  switch (elf->header->machine) {
    case ELF_MACHINE_TYPE_PCODE:
      StringPrintf(&type, "%08x", reloc_type);
      disassembler = NULL;
       break;
    case ELF_MACHINE_TYPE_RISC_V:
      StringSet(&type, RISCVRelocType(reloc_type));
      interpreter = malloc(sizeof(RISCVInterpreter));
      RISCVInterpreterInit(interpreter, false, false);
      disassembler = DisassembleRISCV;
      break;
    case ELF_MACHINE_TYPE_6502:
      StringPrintf(&type, "%08x", reloc_type);
      disassembler = NULL;
      break;
  }
  ELFReaderSection* target_section = elf->sections.value.p[target_section_index];
  printf("%3zd: %-8s %08llx %-24s %-16s\t", i, target_section->name.value,
         reloc->offset, type.value, string_table_address + elf_sym->name);
  if (addend != 0) {
    printf(" + %lld", addend);
  }
  void* target = (char*)target_section->contents + reloc->offset;
  if ((target_section->header->flags & SHF(execinstr)) != 0
      && disassembler != NULL) {
    disassembler(interpreter, target);
  } else {
    printf("\n");
  }
  StringDestruct(&type);
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
    int32_t symtab_section_index = reloc_section->header->link;
    if (symtab_section_index >= elf->sections.length) {
      continue;
    }
    ELFReaderSection* symtab = elf->sections.value.p[symtab_section_index];
    
    if (symtab->header->link >= elf->sections.length) {
      continue;
    }
    ELFReaderSection* strtab = elf->sections.value.p[symtab->header->link];
    const char* symbol_table_address = (const char*)elf->header +
        symtab->header->offset;

    int64_t num_relocations = reloc_section->header->size /
      reloc_section->header->entsize;
    
    const char* reloc_addr = (const char*)elf->header +
      reloc_section->header->offset;
    for (int64_t ri = 0; ri < num_relocations; ri++) {
      ELFRelocation* reloc = (ELFRelocation*)reloc_addr;
      
      PrintRelocation(elf, ri, reloc, symbol_table_address,
                     reloc_section, symtab, strtab);
      reloc_addr += reloc_section->header->entsize;
    }
  }
  VectorDestruct(&relocation_sections);
}

static void Disassemble(ELFReaderFile* elf) {
  Vector code_sections;
  VectorInit(&code_sections);

  ELFReaderFileFindSectionsByType(elf, SHT(progbits), &code_sections);
  void* interpreter = NULL;

  switch (elf->header->machine) {
    case ELF_MACHINE_TYPE_PCODE:
       break;
    case ELF_MACHINE_TYPE_RISC_V:
      interpreter = malloc(sizeof(RISCVInterpreter));
      RISCVInterpreterInit(interpreter, false, false);
      break;
    case ELF_MACHINE_TYPE_6502:
      break;
  }
  
  for (size_t i = 0; i < code_sections.length; i++) {
    ELFReaderSection* code_section = code_sections.value.p[i];
    if ((code_section->header->flags & SHF(execinstr)) != 0) {
      uint64_t length = code_section->header->size;
      char* start = (char*)code_section->contents;
      char* end = start + length;
      
      char* p = start;
      while (p < end) {
        switch (elf->header->machine) {
          case ELF_MACHINE_TYPE_PCODE:
             break;
          case ELF_MACHINE_TYPE_RISC_V:
            DisassembleRISCV(interpreter, p);
            p += 4;
            break;
          case ELF_MACHINE_TYPE_6502:
            break;
        }
      }
    }
  }
  VectorDestruct(&code_sections);
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
  kLast,
};

static uint64_t FindDynamicSectionEntryValue(ELFDynamicSectionEntry* entry, int tag) {
  while (entry->tag != DT(null)) {
    if (entry->tag == tag) {
      return entry->un.val;
    }
    entry++;
  }
  return 0;
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
    ELFDynamicSectionEntry* entries = (ELFDynamicSectionEntry*)dynamic_section->contents;
    uint64_t strtab_offset = FindDynamicSectionEntryValue(entries, DT(strtab));

    if (elf->header->type == ET(exec)) {
      strtab_offset &= 0xffffffff;      // in bottom 4GB.
    }
    char* strtab_addr = (char*)elf->header + strtab_offset;
    
    uint64_t length = dynamic_section->header->size;
    char* start = (char*)entries;
    char* end = start + length;
    
    char* p = start;
    while (p < end) {
      ELFDynamicSectionEntry* entry = (ELFDynamicSectionEntry*)p;
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
          printf("%s\n", strtab_addr + entry->un.val);
          break;
        default:
          // Integers
           printf("%lld\n", entry->un.val);
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
          printf("0x%llx\n", entry->un.ptr);
          break;
      }
      p += sizeof(*entry);
    }
  }
  
  VectorDestruct(&dynamic_sections);
}

static void RunCommand(ELFReaderFile* elf_file, enum Command command,
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
    case kAll:
    case kLast:
      break;
  }
}

int main(int argc, const char * argv[]) {
  String filename;
  StringInit(&filename, NULL);
 enum Command command = kHeader;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // Flags here.
      switch (argv[i][1]) {
        case 'h':
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
        case 'a':
          command = kAll;
          break;
        default:
          Usage();
      }
    } else {
      if (filename.length != 0) {
        Usage();
      }
      StringSet(&filename, argv[i]);
    }
  }
  ELFReaderFile* elf_file = NewELFReaderFile(&filename);
  bool ok = ELFReaderFileRead(elf_file, 0, 0);
  if (!ok) {
    fprintf(stderr, "Can't open file %s\n", filename.value);
    exit(1);
  }
  if (command == kAll) {
    for (command = kHeader; command < kLast; command++) {
      RunCommand(elf_file, command, true);
    }
  } else {
    RunCommand(elf_file, command, false);
  }

  ELFReaderFileDelete(elf_file);
  StringDestruct(&filename);
}
