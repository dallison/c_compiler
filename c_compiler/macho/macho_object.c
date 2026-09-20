//
//  macho_object.c
//  c_compiler
//
//  Write an MH_OBJECT Mach-O file from an AsmObject.  daveld and the
//  interpreters stay ELF-only; this exists so macOS can feed objects to
//  the system assembler/linker tools.
//

#include "macho_object.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "asm_object.h"
#include "binary_tree.h"
#include "buffer.h"
#include "elf.h"
#include "elf_writer.h"
#include "hashtable.h"
#include "macho.h"

typedef struct {
  char sectname[MACHO_SECTION_NAME_LEN];
  char segname[MACHO_SECTION_NAME_LEN];
  uint32_t flags;
  int32_t asm_index;
  uint32_t macho_index;
  uint64_t addr;
  uint64_t size;
  uint32_t offset;
  uint32_t align_log2;
  uint32_t reloff;
  uint32_t nreloc;
  uint8_t* data;
  bool zerofill;
} MachOSection;

typedef struct {
  String name;
  uint8_t type;
  uint8_t sect;
  uint16_t desc;
  uint64_t value;
  AssemblerSymbol* source;
  bool is_undefined;
} MachOSymbol;

typedef struct {
  uint32_t address;
  uint32_t packed;
  uint32_t section;
} MachOReloc;

typedef struct {
  Vector sections;
  Vector symbols;
  Vector relocs;
  Buffer strings;
  uint32_t nlocals;
  uint32_t nextdef;
  uint32_t nundef;
} MachOObject;

static void SetPaddedName(char* dest, const char* src) {
  memset(dest, 0, MACHO_SECTION_NAME_LEN);
  size_t n = strlen(src);
  if (n > MACHO_SECTION_NAME_LEN) {
    n = MACHO_SECTION_NAME_LEN;
  }
  memcpy(dest, src, n);
}

static uint32_t AlignLog2(int32_t alignment) {
  if (alignment <= 1) {
    return 0;
  }
  uint32_t log = 0;
  uint32_t value = (uint32_t)alignment;
  while ((value >> 1) != 0) {
    value >>= 1;
    log++;
  }
  return log;
}

static uint32_t AlignUp(uint32_t value, uint32_t align) {
  if (align <= 1) {
    return value;
  }
  return (value + align - 1) & ~(align - 1);
}

static bool ShouldEmitMachOSection(const AssemblerSection* section) {
  const char* name = section->name != NULL ? section->name->value : "";
  if (strcmp(name, ".eh_frame") == 0 ||
      strncmp(name, ".rela.", 6) == 0 ||
      strncmp(name, ".debug_", 7) == 0 ||
      strcmp(name, ".comment") == 0 ||
      strncmp(name, ".note", 5) == 0) {
    return false;
  }
  return true;
}

static bool MapSectionNames(const AssemblerSection* section, MachOSection* out) {
  const char* name = section->name != NULL ? section->name->value : "";
  out->flags = MACHO_S_REGULAR;
  out->zerofill = section->type == SHT(nobits);
  if (strcmp(name, ".text") == 0) {
    SetPaddedName(out->sectname, "__text");
    SetPaddedName(out->segname, "__TEXT");
    out->flags = MACHO_S_REGULAR | MACHO_S_ATTR_PURE_INSTRUCTIONS |
                 MACHO_S_ATTR_SOME_INSTRUCTIONS;
  } else if (strcmp(name, ".data") == 0) {
    SetPaddedName(out->sectname, "__data");
    SetPaddedName(out->segname, "__DATA");
  } else if (strcmp(name, ".bss") == 0) {
    SetPaddedName(out->sectname, "__bss");
    SetPaddedName(out->segname, "__DATA");
    out->zerofill = true;
    out->flags = MACHO_S_ZEROFILL;
  } else if (strcmp(name, ".rodata") == 0) {
    SetPaddedName(out->sectname, "__const");
    SetPaddedName(out->segname, "__TEXT");
  } else if (strncmp(name, ".rodata.str", 11) == 0) {
    SetPaddedName(out->sectname, "__cstring");
    SetPaddedName(out->segname, "__TEXT");
    out->flags = MACHO_S_CSTRING_LITERALS;
  } else if (section->type == SHT(init_array) ||
             strcmp(name, ".init_array") == 0) {
    SetPaddedName(out->sectname, "__mod_init_func");
    SetPaddedName(out->segname, "__DATA");
    out->flags = MACHO_S_MOD_INIT_FUNC_POINTERS;
  } else if (section->type == SHT(fini_array) ||
             strcmp(name, ".fini_array") == 0) {
    SetPaddedName(out->sectname, "__mod_term_func");
    SetPaddedName(out->segname, "__DATA");
    out->flags = MACHO_S_MOD_TERM_FUNC_POINTERS;
  } else if (strcmp(name, ".eh_frame") == 0) {
    SetPaddedName(out->sectname, "__eh_frame");
    SetPaddedName(out->segname, "__TEXT");
  } else if (strncmp(name, ".debug_", 7) == 0) {
    char dwarf[MACHO_SECTION_NAME_LEN];
    snprintf(dwarf, sizeof(dwarf), "__%s", name + 1);
    SetPaddedName(out->sectname, dwarf);
    SetPaddedName(out->segname, "__DWARF");
    out->flags = MACHO_S_REGULAR | MACHO_S_ATTR_DEBUG;
  } else {
    const char* trimmed = name[0] == '.' ? name + 1 : name;
    char sect[MACHO_SECTION_NAME_LEN];
    snprintf(sect, sizeof(sect), "__%s", trimmed);
    SetPaddedName(out->sectname, sect);
    if ((section->flags & SHF(execinstr)) != 0) {
      SetPaddedName(out->segname, "__TEXT");
      out->flags |= MACHO_S_ATTR_SOME_INSTRUCTIONS;
    } else if ((section->flags & SHF(write)) != 0) {
      SetPaddedName(out->segname, "__DATA");
    } else {
      SetPaddedName(out->segname, "__TEXT");
    }
  }
  if (out->zerofill) {
    out->flags = MACHO_S_ZEROFILL;
  }
  return true;
}

static bool ShouldPrefixSymbol(const AssemblerSymbol* sym) {
  if (sym->name.length == 0) {
    return false;
  }
  char first = sym->name.value[0];
  if (first == '.' || first == 'L') {
    return false;
  }
  return sym->binding == SYM_BIND(global) || sym->binding == SYM_BIND(weak) ||
         !sym->defined;
}

static void CopyPrefixedName(String* dest, const AssemblerSymbol* sym) {
  if (ShouldPrefixSymbol(sym)) {
    StringInit(dest, "_");
    StringAppend(dest, sym->name.value);
  } else {
    StringInit(dest, sym->name.value);
  }
}

static bool MapAArch64Reloc(int32_t elf_type, uint32_t* macho_type,
                            uint32_t* length, uint32_t* pcrel) {
  *length = 2;
  *pcrel = 0;
  switch (elf_type) {
    case R_AARCH64_ABS64:
      *macho_type = MACHO_ARM64_RELOC_UNSIGNED;
      *length = 3;
      return true;
    case R_AARCH64_ABS32:
      *macho_type = MACHO_ARM64_RELOC_UNSIGNED;
      *length = 2;
      return true;
    case R_AARCH64_CALL26:
    case R_AARCH64_JUMP26:
    case R_AARCH64_CALL_PLT:
      *macho_type = MACHO_ARM64_RELOC_BRANCH26;
      *pcrel = 1;
      return true;
    case R_AARCH64_ADR_PREL_PG_HI21:
    case R_AARCH64_ADR_PREL_PG_HI21_NC:
      *macho_type = MACHO_ARM64_RELOC_PAGE21;
      *pcrel = 1;
      return true;
    case R_AARCH64_ADD_ABS_LO12_NC:
    case R_AARCH64_LDST8_ABS_LO12_NC:
    case R_AARCH64_LDST16_ABS_LO12_NC:
    case R_AARCH64_LDST32_ABS_LO12_NC:
    case R_AARCH64_LDST64_ABS_LO12_NC:
    case R_AARCH64_LDST128_ABS_LO12_NC:
      *macho_type = MACHO_ARM64_RELOC_PAGEOFF12;
      return true;
    case R_AARCH64_ADR_GOT_PAGE:
      *macho_type = MACHO_ARM64_RELOC_GOT_LOAD_PAGE21;
      *pcrel = 1;
      return true;
    case R_AARCH64_LD64_GOT_LO12_NC:
      *macho_type = MACHO_ARM64_RELOC_GOT_LOAD_PAGEOFF12;
      return true;
    default:
      return false;
  }
}

static uint32_t PackReloc(uint32_t symbolnum, uint32_t pcrel, uint32_t length,
                          uint32_t ext, uint32_t type) {
  return (symbolnum & 0xffffffu) | ((pcrel & 1u) << 24) |
         ((length & 3u) << 25) | ((ext & 1u) << 27) | ((type & 0xfu) << 28);
}

static void WriteU16(FILE* fp, uint16_t value) {
  fputc((int)(value & 0xff), fp);
  fputc((int)((value >> 8) & 0xff), fp);
}

static void WriteU32(FILE* fp, uint32_t value) {
  fputc((int)(value & 0xff), fp);
  fputc((int)((value >> 8) & 0xff), fp);
  fputc((int)((value >> 16) & 0xff), fp);
  fputc((int)((value >> 24) & 0xff), fp);
}

static void WriteU64(FILE* fp, uint64_t value) {
  WriteU32(fp, (uint32_t)value);
  WriteU32(fp, (uint32_t)(value >> 32));
}

static void WriteZeros(FILE* fp, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    fputc(0, fp);
  }
}

static void WritePaddedName(FILE* fp, const char* name) {
  char padded[MACHO_SECTION_NAME_LEN];
  SetPaddedName(padded, name);
  fwrite(padded, 1, MACHO_SECTION_NAME_LEN, fp);
}

static MachOSection* FindMachOSection(Vector* sections, int32_t asm_index) {
  for (size_t i = 0; i < sections->length; i++) {
    MachOSection* section = sections->value.p[i];
    if (section->asm_index == asm_index) {
      return section;
    }
  }
  return NULL;
}

static void CollectSymbol(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  Vector* symbols = data;
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  if (!sym->exported && sym->defined) {
    return;
  }
  MachOSymbol* out = calloc(1, sizeof(*out));
  CopyPrefixedName(&out->name, sym);
  out->source = sym;
  out->is_undefined = !sym->defined ||
                      (sym->defined && sym->section == SHN_COM);
  if (sym->defined && sym->section == SHN_COM) {
    out->type = MACHO_N_UNDF | MACHO_N_EXT;
    out->value = (uint64_t)(sym->size > 0 ? sym->size : 1);
  } else if (!sym->defined) {
    out->type = MACHO_N_UNDF | MACHO_N_EXT;
    if (sym->binding == SYM_BIND(weak)) {
      out->desc = MACHO_N_WEAK_REF;
    }
  } else {
    out->type = MACHO_N_SECT;
    if (sym->binding == SYM_BIND(global) || sym->binding == SYM_BIND(weak)) {
      out->type |= MACHO_N_EXT;
    }
    if (sym->binding == SYM_BIND(weak)) {
      out->desc = MACHO_N_WEAK_DEF;
    }
    out->value = (uint64_t)sym->value;
  }
  VectorAppend(symbols, out);
}

static void CollectSymbolsFromTree(void* entry, void* data) {
  BinaryTreeTraverse(entry, CollectSymbol, data);
}

static int MachOSymbolGroup(const MachOSymbol* sym) {
  if ((sym->type & MACHO_N_TYPE) == MACHO_N_UNDF) {
    return 2;
  }
  if ((sym->type & MACHO_N_EXT) != 0) {
    return 1;
  }
  return 0;
}

static int CompareMachOSymbol(const void* a, const void* b) {
  const MachOSymbol* left = *(const MachOSymbol* const*)a;
  const MachOSymbol* right = *(const MachOSymbol* const*)b;
  int left_group = MachOSymbolGroup(left);
  int right_group = MachOSymbolGroup(right);
  if (left_group != right_group) {
    return left_group - right_group;
  }
  return strcmp(left->name.value, right->name.value);
}

static bool PrepareSections(AsmObject* object, MachOObject* macho) {
  uint64_t addr = 0;
  for (size_t i = 0; i < object->sections.length; i++) {
    AssemblerSection* section = object->sections.value.p[i];
    if (!ShouldEmitMachOSection(section)) {
      continue;
    }
    size_t size = ELFWriterSectionContentsGetLength(&section->contents);
    if (size == 0 && section->type != SHT(nobits)) {
      continue;
    }
    if (section->type == SHT(nobits) && section->contents.size == 0 &&
        size == 0) {
      continue;
    }
    MachOSection* out = calloc(1, sizeof(*out));
    MapSectionNames(section, out);
    out->asm_index = (int32_t)i;
    out->align_log2 = AlignLog2(section->alignment);
    if (section->type == SHT(nobits)) {
      out->size = section->contents.size != 0 ? section->contents.size : size;
      out->zerofill = true;
      out->data = NULL;
    } else if (section->contents.data_location == kSectionContentsBuffered) {
      out->size = section->contents.data.buffered.length;
      out->data = (uint8_t*)section->contents.data.buffered.value;
    } else {
      out->size = size;
      out->data = NULL;
    }
    uint64_t align = 1ull << out->align_log2;
    addr = (addr + align - 1) & ~(align - 1);
    out->addr = addr;
    addr += out->size;
    VectorAppend(&macho->sections, out);
  }
  for (size_t i = 0; i < macho->sections.length; i++) {
    MachOSection* section = macho->sections.value.p[i];
    section->macho_index = (uint32_t)(i + 1);
  }
  return true;
}

static bool PrepareSymbols(AsmObject* object, MachOObject* macho) {
  HashTableTraverse(&object->symbol_table, CollectSymbolsFromTree,
                     &macho->symbols);
  size_t kept = 0;
  for (size_t i = 0; i < macho->symbols.length; i++) {
    MachOSymbol* sym = macho->symbols.value.p[i];
    if (!sym->is_undefined && (sym->type & MACHO_N_SECT) == MACHO_N_SECT &&
        FindMachOSection(&macho->sections, sym->source->section) == NULL) {
      StringDestruct(&sym->name);
      free(sym);
      continue;
    }
    macho->symbols.value.p[kept++] = sym;
  }
  macho->symbols.length = kept;
  if (macho->symbols.length > 0) {
    qsort(macho->symbols.value.p, macho->symbols.length, sizeof(void*),
          CompareMachOSymbol);
  }
  for (size_t i = 0; i < macho->symbols.length; i++) {
    MachOSymbol* sym = macho->symbols.value.p[i];
    if (sym->source != NULL) {
      sym->source->index = (int32_t)i;
    }
    if (!sym->is_undefined && (sym->type & MACHO_N_SECT) == MACHO_N_SECT) {
      MachOSection* section =
          FindMachOSection(&macho->sections, sym->source->section);
      if (section == NULL) {
        fprintf(stderr,
                "Mach-O: symbol '%s' is in a dropped section\n",
                sym->name.value);
        return false;
      }
      sym->sect = (uint8_t)section->macho_index;
      sym->value += section->addr;
    }
    if (MachOSymbolGroup(sym) == 2) {
      macho->nundef++;
    } else if (MachOSymbolGroup(sym) == 1) {
      macho->nextdef++;
    } else {
      macho->nlocals++;
    }
  }
  /* Apple ld requires symbol index 0 to be a defined local. */
  if (macho->sections.length > 0) {
    MachOSymbol* dummy = calloc(1, sizeof(*dummy));
    StringInit(&dummy->name, "ltmp0");
    dummy->type = MACHO_N_SECT;
    dummy->sect =
        ((MachOSection*)macho->sections.value.p[0])->macho_index;
    VectorInsertBefore(&macho->symbols, 0, dummy);
    macho->nlocals++;
    for (size_t i = 0; i < macho->symbols.length; i++) {
      MachOSymbol* sym = macho->symbols.value.p[i];
      if (sym->source != NULL) {
        sym->source->index = (int32_t)i;
      }
    }
  }
  return true;
}

static bool RelocUsesAddendPair(uint32_t macho_type) {
  return macho_type == MACHO_ARM64_RELOC_PAGE21 ||
         macho_type == MACHO_ARM64_RELOC_PAGEOFF12 ||
         macho_type == MACHO_ARM64_RELOC_BRANCH26 ||
         macho_type == MACHO_ARM64_RELOC_GOT_LOAD_PAGE21 ||
         macho_type == MACHO_ARM64_RELOC_GOT_LOAD_PAGEOFF12;
}

static bool WriteIntegerIntoSection(MachOSection* section, uint32_t offset,
                                    int64_t addend, uint32_t nbytes) {
  if (section->data == NULL || offset + nbytes > section->size) {
    fprintf(stderr, "Mach-O: cannot write a %u-byte addend at %#x\n", nbytes,
            offset);
    return false;
  }
  uint64_t value = 0;
  memcpy(&value, section->data + offset, nbytes);
  value += (uint64_t)addend;
  memcpy(section->data + offset, &value, nbytes);
  return true;
}

static bool PrepareRelocs(AsmObject* object, MachOObject* macho) {
  for (size_t i = 0; i < object->relocations.length; i++) {
    AssemblerRelocation* reloc = object->relocations.value.p[i];
    MachOSection* section =
        FindMachOSection(&macho->sections, reloc->section);
    if (section == NULL) {
      continue;
    }
    uint32_t type = 0;
    uint32_t length = 2;
    uint32_t pcrel = 0;
    if (!MapAArch64Reloc(reloc->type, &type, &length, &pcrel)) {
      fprintf(stderr,
              "Mach-O: unsupported AArch64 relocation %d for '%s'\n",
              reloc->type,
              reloc->symbol != NULL ? reloc->symbol->name.value : "?");
      return false;
    }
    int symbol = reloc->symbol != NULL ? reloc->symbol->index : -1;
    if (symbol < 0) {
      fprintf(stderr, "Mach-O: relocation has no symbol\n");
      return false;
    }
    if (reloc->addend != 0) {
      if (RelocUsesAddendPair(type)) {
        if (reloc->addend < -0x800000 || reloc->addend > 0x7fffff) {
          fprintf(stderr, "Mach-O: relocation addend %#x does not fit\n",
                  reloc->addend);
          return false;
        }
        MachOReloc* addend = malloc(sizeof(*addend));
        addend->address = (uint32_t)reloc->offset;
        addend->section = section->macho_index;
        addend->packed =
            PackReloc((uint32_t)reloc->addend & 0xffffffu, 0, 2, 0,
                      MACHO_ARM64_RELOC_ADDEND);
        VectorAppend(&macho->relocs, addend);
        section->nreloc++;
      } else if (!WriteIntegerIntoSection(section, (uint32_t)reloc->offset,
                                          reloc->addend,
                                          length == 3 ? 8 : 4)) {
        return false;
      }
    }
    MachOReloc* out = malloc(sizeof(*out));
    out->address = (uint32_t)reloc->offset;
    out->section = section->macho_index;
    out->packed = PackReloc((uint32_t)symbol, pcrel, length, 1, type);
    VectorAppend(&macho->relocs, out);
    section->nreloc++;
  }
  return true;
}

static uint32_t AppendCString(Buffer* strings, const char* text) {
  uint32_t offset = (uint32_t)strings->length;
  size_t n = strlen(text) + 1;
  BufferAppend(strings, (char*)text, n);
  return offset;
}

bool AsmObjectWriteMachO(AsmObject* object, FILE* out) {
  if (object->elf_machine_type != ELF_MACHINE_TYPE_AARCH64) {
    fprintf(stderr, "-fnative currently writes Mach-O only for AArch64\n");
    return false;
  }

  MachOObject macho = {0};
  VectorInit(&macho.sections);
  VectorInit(&macho.symbols);
  VectorInit(&macho.relocs);
  BufferInit(&macho.strings);
  BufferAppend(&macho.strings, "\0", 1);

  bool ok = PrepareSections(object, &macho) && PrepareSymbols(object, &macho) &&
            PrepareRelocs(object, &macho);
  if (!ok) {
    goto done;
  }

  uint32_t nsects = (uint32_t)macho.sections.length;
  uint32_t cmd_segment = 72 + 80 * nsects;
  uint32_t cmd_symtab = 24;
  uint32_t cmd_dysymtab = 80;
  uint32_t cmd_build = 24;
  uint32_t sizeofcmds = cmd_segment + cmd_symtab + cmd_dysymtab + cmd_build;
  uint32_t header_size = 32;
  uint32_t data_off = header_size + sizeofcmds;

  for (size_t i = 0; i < macho.sections.length; i++) {
    MachOSection* section = macho.sections.value.p[i];
    if (section->zerofill) {
      section->offset = 0;
      continue;
    }
    data_off = AlignUp(data_off, 1u << section->align_log2);
    section->offset = data_off;
    data_off += (uint32_t)section->size;
  }

  uint32_t reloc_start = AlignUp(data_off, 8);
  uint32_t reloc_off = reloc_start;
  for (size_t i = 0; i < macho.sections.length; i++) {
    MachOSection* section = macho.sections.value.p[i];
    if (section->nreloc == 0) {
      section->reloff = 0;
      continue;
    }
    section->reloff = reloc_off;
    reloc_off += section->nreloc * 8;
  }

  uint32_t nsyms = (uint32_t)macho.symbols.length;
  uint32_t symoff = AlignUp(reloc_off, 8);
  uint32_t stroff = symoff + nsyms * 16;
  Vector strx = {0};
  VectorInit(&strx);
  for (size_t i = 0; i < macho.symbols.length; i++) {
    MachOSymbol* sym = macho.symbols.value.p[i];
    uint32_t* offset = malloc(sizeof(*offset));
    *offset = AppendCString(&macho.strings, sym->name.value);
    VectorAppend(&strx, offset);
  }
  uint32_t strsize = (uint32_t)macho.strings.length;

  WriteU32(out, MACHO_MH_MAGIC_64);
  WriteU32(out, MACHO_CPU_TYPE_ARM64);
  WriteU32(out, MACHO_CPU_SUBTYPE_ARM64_ALL);
  WriteU32(out, MACHO_MH_OBJECT);
  WriteU32(out, 4);
  WriteU32(out, sizeofcmds);
  WriteU32(out, MACHO_MH_SUBSECTIONS_VIA_SYMBOLS);
  WriteU32(out, 0);

  WriteU32(out, MACHO_LC_SEGMENT_64);
  WriteU32(out, cmd_segment);
  WritePaddedName(out, "");
  WriteU64(out, 0);
  WriteU64(out, data_off > header_size + sizeofcmds
                    ? data_off - (header_size + sizeofcmds)
                    : 0);
  WriteU64(out, header_size + sizeofcmds);
  WriteU64(out, data_off > header_size + sizeofcmds
                    ? data_off - (header_size + sizeofcmds)
                    : 0);
  WriteU32(out, 7);
  WriteU32(out, 7);
  WriteU32(out, nsects);
  WriteU32(out, 0);
  for (size_t i = 0; i < macho.sections.length; i++) {
    MachOSection* section = macho.sections.value.p[i];
    fwrite(section->sectname, 1, MACHO_SECTION_NAME_LEN, out);
    fwrite(section->segname, 1, MACHO_SECTION_NAME_LEN, out);
    WriteU64(out, section->addr);
    WriteU64(out, section->size);
    WriteU32(out, section->offset);
    WriteU32(out, section->align_log2);
    WriteU32(out, section->reloff);
    WriteU32(out, section->nreloc);
    WriteU32(out, section->flags);
    WriteU32(out, 0);
    WriteU32(out, 0);
    WriteU32(out, 0);
  }

  WriteU32(out, MACHO_LC_SYMTAB);
  WriteU32(out, cmd_symtab);
  WriteU32(out, symoff);
  WriteU32(out, nsyms);
  WriteU32(out, stroff);
  WriteU32(out, strsize);

  WriteU32(out, MACHO_LC_DYSYMTAB);
  WriteU32(out, cmd_dysymtab);
  WriteU32(out, 0);
  WriteU32(out, macho.nlocals);
  WriteU32(out, macho.nlocals);
  WriteU32(out, macho.nextdef);
  WriteU32(out, macho.nlocals + macho.nextdef);
  WriteU32(out, macho.nundef);
  for (int i = 0; i < 12; i++) {
    WriteU32(out, 0);
  }

  WriteU32(out, MACHO_LC_BUILD_VERSION);
  WriteU32(out, cmd_build);
  WriteU32(out, MACHO_PLATFORM_MACOS);
  WriteU32(out, 0x000e0000);
  WriteU32(out, 0x000e0000);
  WriteU32(out, 0);

  uint32_t pos = header_size + sizeofcmds;
  for (size_t i = 0; i < macho.sections.length; i++) {
    MachOSection* section = macho.sections.value.p[i];
    if (section->zerofill) {
      continue;
    }
    if (section->offset > pos) {
      WriteZeros(out, section->offset - pos);
      pos = section->offset;
    }
    if (section->data != NULL && section->size != 0) {
      fwrite(section->data, 1, (size_t)section->size, out);
    } else {
      WriteZeros(out, (uint32_t)section->size);
    }
    pos += (uint32_t)section->size;
  }
  if (reloc_start > pos) {
    WriteZeros(out, reloc_start - pos);
    pos = reloc_start;
  }
  for (size_t s = 0; s < macho.sections.length; s++) {
    MachOSection* section = macho.sections.value.p[s];
    for (size_t i = 0; i < macho.relocs.length; i++) {
      MachOReloc* reloc = macho.relocs.value.p[i];
      if (reloc->section != section->macho_index) {
        continue;
      }
      WriteU32(out, reloc->address);
      WriteU32(out, reloc->packed);
      pos += 8;
    }
  }
  if (symoff > pos) {
    WriteZeros(out, symoff - pos);
    pos = symoff;
  }
  for (size_t i = 0; i < macho.symbols.length; i++) {
    MachOSymbol* sym = macho.symbols.value.p[i];
    uint32_t* offset = strx.value.p[i];
    WriteU32(out, *offset);
    fputc(sym->type, out);
    fputc(sym->sect, out);
    WriteU16(out, sym->desc);
    WriteU64(out, sym->value);
    pos += 16;
  }
  if (stroff > pos) {
    WriteZeros(out, stroff - pos);
  }
  fwrite(macho.strings.value, 1, macho.strings.length, out);

  VectorDestructWithContents(&strx, NULL, true);

done:
  for (size_t i = 0; i < macho.sections.length; i++) {
    free(macho.sections.value.p[i]);
  }
  for (size_t i = 0; i < macho.symbols.length; i++) {
    MachOSymbol* sym = macho.symbols.value.p[i];
    StringDestruct(&sym->name);
    free(sym);
  }
  for (size_t i = 0; i < macho.relocs.length; i++) {
    free(macho.relocs.value.p[i]);
  }
  VectorDestruct(&macho.sections);
  VectorDestruct(&macho.symbols);
  VectorDestruct(&macho.relocs);
  BufferDestruct(&macho.strings);
  return ok;
}
