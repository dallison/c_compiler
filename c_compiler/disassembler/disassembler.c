//
//  disassembler.c
//  c_compiler
//

#include "disassembler_internal.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dstring.h"

void DAsmInitInstruction(DAsmInstruction* inst, const void* bytes,
                         size_t available, uint64_t address, size_t size) {
  memset(inst, 0, sizeof(*inst));
  inst->address = address;
  inst->size = size;
  inst->num_bytes = size < available ? size : available;
  if (inst->num_bytes > DASM_MAX_BYTES) {
    inst->num_bytes = DASM_MAX_BYTES;
  }
  memcpy(inst->bytes, bytes, inst->num_bytes);
}

void DAsmFormat(DAsmInstruction* inst, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(inst->text, sizeof(inst->text), fmt, ap);
  va_end(ap);
  inst->known = true;
}

void DAsmSetTarget(DAsmInstruction* inst, uint64_t target_address) {
  inst->has_target_address = true;
  inst->target_address = target_address;
}

void DAsmUnknownInstruction(DAsmInstruction* inst, const char* directive_fmt,
                            uint64_t value) {
  snprintf(inst->text, sizeof(inst->text), directive_fmt, value);
  inst->known = false;
}

bool DAsmArchitectureFromELFMachine(int machine, DAsmArchitecture* arch) {
  switch (machine) {
    case ELF_MACHINE_TYPEW65C02:
      *arch = kDAsm6502;
      return true;
    case ELF_MACHINE_TYPE_RISC_V:
      *arch = kDAsmRiscV;
      return true;
    case ELF_MACHINE_TYPE_AARCH64:
      *arch = kDAsmAArch64;
      return true;
    case ELF_MACHINE_TYPE_ARM:
      *arch = kDAsmARM;
      return true;
    case ELF_MACHINE_TYPE_X86_64:
      *arch = kDAsmX86_64;
      return true;
    default:
      *arch = kDAsmUnknown;
      return false;
  }
}

const char* DAsmArchitectureName(DAsmArchitecture arch) {
  switch (arch) {
    case kDAsm6502:
      return "6502";
    case kDAsmRiscV:
      return "riscv";
    case kDAsmAArch64:
      return "aarch64";
    case kDAsmARM:
      return "arm";
    case kDAsmX86_64:
      return "x86_64";
    case kDAsmUnknown:
      return "unknown";
  }
  return "unknown";
}

DAsmArchitecture DAsmArchitectureFromName(const char* name) {
  if (strcmp(name, "6502") == 0 || strcmp(name, "65c02") == 0) {
    return kDAsm6502;
  }
  if (strcmp(name, "riscv") == 0 || strcmp(name, "risc-v") == 0) {
    return kDAsmRiscV;
  }
  if (strcmp(name, "aarch64") == 0 || strcmp(name, "arm64") == 0) {
    return kDAsmAArch64;
  }
  if (strcmp(name, "arm") == 0 || strcmp(name, "armv7") == 0 ||
      strcmp(name, "armv7-a") == 0 || strcmp(name, "arm32") == 0) {
    return kDAsmARM;
  }
  if (strcmp(name, "x86_64") == 0 || strcmp(name, "x86-64") == 0 ||
      strcmp(name, "amd64") == 0) {
    return kDAsmX86_64;
  }
  return kDAsmUnknown;
}

size_t DAsmDefaultInstructionSize(DAsmArchitecture arch) {
  switch (arch) {
    case kDAsm6502:
      return 1;
    case kDAsmRiscV:
    case kDAsmAArch64:
    case kDAsmARM:
      return 4;
    case kDAsmX86_64:
      return 1;
    case kDAsmUnknown:
      return 1;
  }
  return 1;
}

bool DAsmDisassembleInstruction(DAsmArchitecture arch, const void* bytes,
                                size_t length, uint64_t address,
                                DAsmInstruction* inst) {
  switch (arch) {
    case kDAsm6502:
      return DAsmDisassemble6502(bytes, length, address, inst);
    case kDAsmRiscV:
      return DAsmDisassembleRiscV(bytes, length, address, inst);
    case kDAsmAArch64:
      return DAsmDisassembleAArch64(bytes, length, address, inst);
    case kDAsmARM:
      return DAsmDisassembleARM(bytes, length, address, inst);
    case kDAsmX86_64:
      return DAsmDisassembleX86_64(bytes, length, address, inst);
    case kDAsmUnknown:
      return false;
  }
  return false;
}

static const char* FindRelocationName(const DAsmOptions* options,
                                      uint64_t address) {
  if (options == NULL || options->relocations == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < options->num_relocations; i++) {
    if (options->relocations[i].address == address &&
        options->relocations[i].name != NULL &&
        options->relocations[i].name[0] != '\0') {
      return options->relocations[i].name;
    }
  }
  return NULL;
}

static const char* FindSymbolName(const DAsmOptions* options,
                                  uint64_t address) {
  if (options == NULL || options->symbols == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < options->num_symbols; i++) {
    if (options->symbols[i].address == address &&
        options->symbols[i].name != NULL && options->symbols[i].name[0] != '\0') {
      return options->symbols[i].name;
    }
  }
  return NULL;
}

static bool IsFunctionLabel(const DAsmOptions* options, uint64_t address,
                            size_t index) {
  if (options == NULL || options->symbols == NULL ||
      index >= options->num_symbols) {
    return false;
  }
  return options->symbols[index].is_function &&
         options->symbols[index].address == address &&
         options->symbols[index].name != NULL &&
         options->symbols[index].name[0] != '\0';
}

static void PrintFunctionLabels(FILE* fp, const DAsmOptions* options,
                                uint64_t address) {
  if (options == NULL || options->symbols == NULL) {
    return;
  }
  for (size_t i = 0; i < options->num_symbols; i++) {
    if (!IsFunctionLabel(options, address, i)) {
      continue;
    }
    bool duplicate = false;
    for (size_t j = 0; j < i; j++) {
      if (IsFunctionLabel(options, address, j) &&
          strcmp(options->symbols[j].name, options->symbols[i].name) == 0) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      fprintf(fp, "%s:\n", options->symbols[i].name);
    }
  }
}

static void ReplaceLastHexTarget(DAsmInstruction* inst, const char* name) {
  char* hex = NULL;
  for (char* p = strstr(inst->text, "0x"); p != NULL;
       p = strstr(p + 2, "0x")) {
    hex = p;
  }
  if (hex == NULL) {
    return;
  }

  char* end = hex + 2;
  while ((*end >= '0' && *end <= '9') || (*end >= 'a' && *end <= 'f') ||
         (*end >= 'A' && *end <= 'F')) {
    end++;
  }

  char replacement[DASM_MAX_TEXT];
  size_t prefix_len = (size_t)(hex - inst->text);
  if (prefix_len >= sizeof(replacement)) {
    return;
  }
  memcpy(replacement, inst->text, prefix_len);
  snprintf(replacement + prefix_len, sizeof(replacement) - prefix_len, "%s%s",
           name, end);
  snprintf(inst->text, sizeof(inst->text), "%s", replacement);
}

static void SymbolizeInstruction(DAsmInstruction* inst,
                                 const DAsmOptions* options) {
  const char* name = FindRelocationName(options, inst->address);
  if (name == NULL && inst->has_target_address) {
    name = FindSymbolName(options, inst->target_address);
  }
  if (name != NULL) {
    ReplaceLastHexTarget(inst, name);
  }
}

void DAsmPrintInstruction(FILE* fp, const DAsmInstruction* inst) {
  fprintf(fp, "%08" PRIx64 ":  ", inst->address);
  for (size_t i = 0; i < inst->num_bytes; i++) {
    fprintf(fp, "%02x ", inst->bytes[i]);
  }
  for (size_t i = inst->num_bytes; i < 8; i++) {
    fprintf(fp, "   ");
  }
  fprintf(fp, " %s\n", inst->text);
}

static bool AddressInRange(uint64_t addr, const DAsmOptions* options) {
  if (options == NULL || !options->has_start_address) {
    return true;
  }
  if (addr < options->start_address) {
    return false;
  }
  if (!options->has_length) {
    return true;
  }
  return addr < options->start_address + options->length;
}

static void DisassembleBytes(DAsmArchitecture arch, const unsigned char* bytes,
                             size_t length, uint64_t address,
                             const DAsmOptions* options, FILE* fp) {
  size_t offset = 0;
  while (offset < length) {
    uint64_t pc = address + offset;
    if (!AddressInRange(pc, options)) {
      offset++;
      continue;
    }
    PrintFunctionLabels(fp, options, pc);
    DAsmInstruction inst;
    if (!DAsmDisassembleInstruction(arch, bytes + offset, length - offset, pc,
                                    &inst) ||
        inst.size == 0) {
      DAsmInitInstruction(&inst, bytes + offset, length - offset, pc, 1);
      DAsmUnknownInstruction(&inst, ".byte 0x%02" PRIx64, inst.bytes[0]);
    }
    SymbolizeInstruction(&inst, options);
    DAsmPrintInstruction(fp, &inst);
    offset += inst.size;
  }
}

static void CollectSymbols(ELFReaderFile* elf, Vector* symbols) {
  Vector symbol_sections;
  VectorInit(&symbol_sections);
  ELFReaderFileFindSectionsByType(elf, SHT(symtab), &symbol_sections);
  ELFReaderFileFindSectionsByType(elf, SHT(dynsym), &symbol_sections);
  for (size_t si = 0; si < symbol_sections.length; si++) {
    ELFReaderSection* symtab = symbol_sections.value.p[si];
    if (symtab->header->link >= elf->sections.length || symtab->header->entsize == 0) {
      continue;
    }
    ELFReaderSection* strtab = elf->sections.value.p[symtab->header->link];
    const char* sym = symtab->contents;
    size_t num_symbols = (size_t)(symtab->header->size / symtab->header->entsize);
    for (size_t i = 0; i < num_symbols; i++) {
      ELFSymbol elf_sym;
      elf->ops->ReadSymbol(&elf_sym, sym);
      sym += symtab->header->entsize;
      if (elf_sym.name == 0) {
        continue;
      }
      int type = ELF_ST_TYPE(elf_sym.info);
      if (type == STT(section) || type == STT(file)) {
        continue;
      }
      DAsmSymbol* dasm_sym = malloc(sizeof(*dasm_sym));
      dasm_sym->address = elf_sym.value;
      dasm_sym->name = (const char*)strtab->contents + elf_sym.name;
      dasm_sym->is_function = type == STT(func);
      VectorAppend(symbols, dasm_sym);
    }
  }
  VectorDestruct(&symbol_sections);
}

static void CollectRelocations(ELFReaderFile* elf, Vector* relocations) {
  Vector relocation_sections;
  VectorInit(&relocation_sections);
  ELFReaderFileFindSectionsByType(elf, SHT(rela), &relocation_sections);
  ELFReaderFileFindSectionsByType(elf, SHT(rel), &relocation_sections);
  for (size_t ri = 0; ri < relocation_sections.length; ri++) {
    ELFReaderSection* reltab = relocation_sections.value.p[ri];
    if (reltab->header->info >= elf->sections.length ||
        reltab->header->link >= elf->sections.length ||
        reltab->header->entsize == 0) {
      continue;
    }
    ELFReaderSection* target_section = elf->sections.value.p[reltab->header->info];
    if ((target_section->header->flags & SHF(execinstr)) == 0) {
      continue;
    }
    ELFReaderSection* symtab = elf->sections.value.p[reltab->header->link];
    if (symtab->header->link >= elf->sections.length || symtab->header->entsize == 0) {
      continue;
    }
    ELFReaderSection* strtab = elf->sections.value.p[symtab->header->link];
    const char* rel = reltab->contents;
    size_t num_relocations = (size_t)(reltab->header->size / reltab->header->entsize);
    for (size_t i = 0; i < num_relocations; i++) {
      ELFRelocation elf_rel;
      elf->ops->ReadRelocation(&elf_rel, rel);
      rel += reltab->header->entsize;
      uint32_t symbol_index = ELF_R_SYM(elf_rel.info);
      if (symbol_index * symtab->header->entsize >= symtab->header->size) {
        continue;
      }
      ELFSymbol elf_sym;
      const char* sym_addr = (const char*)symtab->contents +
                             symbol_index * symtab->header->entsize;
      elf->ops->ReadSymbol(&elf_sym, sym_addr);
      if (elf_sym.name == 0) {
        continue;
      }
      DAsmRelocation* dasm_rel = malloc(sizeof(*dasm_rel));
      dasm_rel->address = target_section->header->addr + elf_rel.offset;
      dasm_rel->name = (const char*)strtab->contents + elf_sym.name;
      VectorAppend(relocations, dasm_rel);
    }
  }
  VectorDestruct(&relocation_sections);
}

static void FreePointerVector(Vector* v) {
  for (size_t i = 0; i < v->length; i++) {
    free(v->value.p[i]);
  }
  VectorDestruct(v);
}

bool DAsmDisassembleELF(ELFReaderFile* elf, const DAsmOptions* options,
                        FILE* fp) {
  DAsmArchitecture arch = kDAsmUnknown;
  if (options != NULL && options->arch != kDAsmUnknown) {
    arch = options->arch;
  } else if (!DAsmArchitectureFromELFMachine(elf->header->machine, &arch)) {
    fprintf(stderr, "unsupported ELF machine %d\n", elf->header->machine);
    return false;
  }

  Vector symbols;
  Vector relocations;
  VectorInit(&symbols);
  VectorInit(&relocations);
  CollectSymbols(elf, &symbols);
  CollectRelocations(elf, &relocations);

  DAsmSymbol* symbol_array = NULL;
  DAsmRelocation* relocation_array = NULL;
  if (symbols.length != 0) {
    symbol_array = malloc(symbols.length * sizeof(*symbol_array));
    for (size_t i = 0; i < symbols.length; i++) {
      symbol_array[i] = *(DAsmSymbol*)symbols.value.p[i];
    }
  }
  if (relocations.length != 0) {
    relocation_array = malloc(relocations.length * sizeof(*relocation_array));
    for (size_t i = 0; i < relocations.length; i++) {
      relocation_array[i] = *(DAsmRelocation*)relocations.value.p[i];
    }
  }

  DAsmOptions elf_options = options != NULL ? *options : (DAsmOptions){0};
  if (elf_options.symbols == NULL) {
    elf_options.symbols = symbol_array;
    elf_options.num_symbols = symbols.length;
  }
  if (elf_options.relocations == NULL) {
    elf_options.relocations = relocation_array;
    elf_options.num_relocations = relocations.length;
  }

  Vector code_sections;
  VectorInit(&code_sections);
  ELFReaderFileFindSectionsByType(elf, SHT(progbits), &code_sections);
  for (size_t i = 0; i < code_sections.length; i++) {
    ELFReaderSection* section = code_sections.value.p[i];
    if ((section->header->flags & SHF(execinstr)) == 0) {
      continue;
    }
    if (options != NULL && options->print_section_names) {
      fprintf(fp, "%s:\n", section->name.value);
    }
    DisassembleBytes(arch, section->contents, (size_t)section->header->size,
                     section->header->addr, &elf_options, fp);
  }
  VectorDestruct(&code_sections);
  FreePointerVector(&symbols);
  FreePointerVector(&relocations);
  free(symbol_array);
  free(relocation_array);
  return true;
}

bool DAsmDisassembleFile(const char* filename, const DAsmOptions* options,
                         FILE* fp) {
  String name;
  StringInit(&name, filename);
  ELFReaderFile* elf = NewELFReaderFile(&name);
  bool ok = ELFReaderFileRead(elf, 0, 0);
  if (ok) {
    ok = DAsmDisassembleELF(elf, options, fp);
  } else {
    fprintf(stderr, "unable to open or read ELF file '%s'\n", filename);
  }
  ELFReaderFileDelete(elf);
  StringDestruct(&name);
  return ok;
}
