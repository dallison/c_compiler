//
//  asm_object.c
//  c_compiler
//

#include "binary_tree.h"
#include "asm_object.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "asm_expr.h"
#include "dwarf.h"
#include "elf_writer.h"

AssemblerSection* NewAssemblerSection(String* name, int32_t type, int32_t flags,
                                      int32_t alignment) {
  AssemblerSection* section = malloc(sizeof(AssemblerSection));
  section->name = name;
  ELFWriterSectionContentsInit(&section->contents,
                               type == SHT(nobits) ? kSectionContentsNobits
                                                   : kSectionContentsBuffered);
  section->address = 0;
  section->flags = flags;
  section->type = type;
  section->alignment = alignment;
  section->has_subsections = false;
  section->current_subsection = 0;
  section->subsection_cap = 0;
  section->sub_size = NULL;
  section->prev_sub_size = NULL;
  section->sub_bytes = NULL;
  return section;
}

void AssemblerSectionDestruct(AssemblerSection* section) {
  ELFWriterSectionContentsDestruct(&section->contents);
  for (int i = 0; i < section->subsection_cap; i++) {
    BufferDestruct(&section->sub_bytes[i]);
  }
  free(section->sub_size);
  free(section->prev_sub_size);
  free(section->sub_bytes);
  if (section->name != NULL) {
    StringDelete(section->name);
  }
}

void AssemblerSectionDelete(AssemblerSection* section) {
  AssemblerSectionDestruct(section);
  free(section);
}

void AssemblerSectionAlign(AssemblerSection* section, int alignment) {
  BufferAlignLength(&section->contents.data.buffered, alignment);
}

AssemblerRelocation* NewAssemblerRelocation(AssemblerSymbol* sym, int32_t type,
                                            int32_t section, int32_t offset,
                                            int32_t addend) {
  AssemblerRelocation* r = malloc(sizeof(AssemblerRelocation));
  r->symbol = sym;
  r->type = type;
  r->section = section;
  r->offset = offset;
  r->addend = addend;
  return r;
}

void AssemblerRelocationDestruct(AssemblerRelocation* reloc) {
  (void)reloc;
}

void AssemblerRelocationDelete(AssemblerRelocation* reloc) {
  AssemblerRelocationDestruct(reloc);
  free(reloc);
}

static AssemblerSymbol* AllocAssemblerSymbol(const char* name, bool stable_name,
                                             int section,
                                             AssemblerSymbolType type,
                                             AssemblerSymbolBinding binding,
                                             int64_t value) {
  AssemblerSymbol* sym = malloc(sizeof(AssemblerSymbol));
  BinaryTreeNodeInit(&sym->header);
  if (stable_name) {
    StringInitImmutable(&sym->name, name);
  } else {
    StringInit(&sym->name, name);
  }
  sym->type = type;
  sym->binding = binding;
  sym->value = value;
  sym->size = 0;
  sym->defined = false;
  sym->defined_pass = 0;
  sym->section = section;
  sym->index = -1;
  sym->exported = false;
  sym->alignment = 1;
  sym->is_label = false;
  sym->is_constant = false;
  sym->is_forward_declared = false;
  sym->explicit_local = false;
  sym->is_eqv = false;
  sym->eqv_busy = false;
  sym->eqv_ast = NULL;
  return sym;
}

AssemblerSymbol* NewAssemblerSymbol(const char* name, int section,
                                    AssemblerSymbolType type,
                                    AssemblerSymbolBinding binding,
                                    int64_t value) {
  return AllocAssemblerSymbol(name, false, section, type, binding, value);
}

AssemblerSymbol* NewAssemblerSymbolWithStableName(
    const char* name, int section, AssemblerSymbolType type,
    AssemblerSymbolBinding binding, int64_t value) {
  return AllocAssemblerSymbol(name, true, section, type, binding, value);
}

void AssemblerSymbolDelete(AssemblerSymbol* sym) {
  if (sym->eqv_ast != NULL) {
    AssemblerReleaseExpression(sym->eqv_ast);
    sym->eqv_ast = NULL;
  }
  StringDestruct(&sym->name);
}

static int SymbolInsertCompare(BinaryTreeNode* node1, BinaryTreeNode* node2) {
  AssemblerSymbol* sym1 = (AssemblerSymbol*)node1;
  AssemblerSymbol* sym2 = (AssemblerSymbol*)node2;
  return StringCompareString(&sym1->name, &sym2->name);
}

static int SymbolSearchCompare(BinaryTreeNode* node, void* name) {
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  return StringCompare(&sym->name, name);
}

static void SymbolDestructor(BinaryTreeNode* node, void* delete_symbols) {
  (void)delete_symbols;
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  AssemblerSymbolDelete(sym);
}

static bool InsertSymbolIntoHashTable(void* table, void* node, void** parent) {
  BinaryTree* tree = table;
  if (table == NULL) {
    tree = NewBinaryTree(SymbolInsertCompare, SymbolSearchCompare,
                         SymbolDestructor);
    *parent = tree;
  }
  return BinaryTreeInsert(tree, node);
}

static AssemblerSymbol* FindAssemblerSymbol(BinaryTree* table, String* name) {
  return (AssemblerSymbol*)BinaryTreeSearch(table, name);
}

static void* FindSymbolInHashTable(void* table, void* value) {
  return FindAssemblerSymbol(table, value);
}

static size_t HashSymbol(void* value, HashTable* table, HashMode mode) {
  const char* name;
  switch (mode) {
    case kHashInsert:
      name = ((AssemblerSymbol*)value)->name.value;
      break;
    case kHashSearch:
      name = (const char*)value;
      break;
  }
  (void)table;
  uint32_t hash = 5381;
  while (*name != '\0') {
    hash = (hash << 5) + hash + *name++;
  }
  return hash;
}

static void DeleteSymbolTable(void* table, void* data) {
  (void)data;
  BinaryTreeDestruct(table, NULL);
  free(table);
}

static uint64_t NewSymbolEpoch(void) {
  static uint64_t last_epoch = 0;
  return ++last_epoch;
}

static void ClearAssemblerSymbolTable(AsmObject* object) {
  HashTableTraverse(&object->symbol_table, DeleteSymbolTable, NULL);
  HashTableClear(&object->symbol_table);
  object->symbol_epoch = NewSymbolEpoch();
}

void AsmObjectInit(AsmObject* object, int16_t elf_machine_type,
                   uint16_t elf_flags, int* reloc_types) {
  VectorInit(&object->sections);
  VectorInit(&object->relocations);
  VectorInit(&object->orphan_symbols);
  VectorInit(&object->operations);
  object->pass = 0;
  object->allow_layout_pass_skip = false;
  object->requires_layout_pass = false;
  object->recording_operations = false;
  HashTableInit(&object->symbol_table, "assembler_symbols", 1009, HashSymbol,
                InsertSymbolIntoHashTable, FindSymbolInHashTable);
  object->symbol_epoch = NewSymbolEpoch();
  object->elf_machine_type = elf_machine_type;
  object->elf_flags = elf_flags;
  switch (elf_machine_type) {
    case ELF_MACHINE_TYPE_ARM:
    case ELF_MACHINE_TYPE_X86:
    case ELF_MACHINE_TYPE_XTENSA:
      object->is_64_bit = false;
      break;
    default:
      object->is_64_bit = true;
      break;
  }
  object->is_little_endian = true;
  object->reloc_types = reloc_types;
  object->pic = false;
  object->write_macho = false;
  object->absolute = false;
  object->current_section = 0;
  StringInit(&object->filename, "");
  DwarfInit(&object->dwarf);
  if (elf_machine_type == ELF_MACHINE_TYPEW65C02 ||
      elf_machine_type == ELF_MACHINE_TYPE_X86_64 ||
      elf_machine_type == ELF_MACHINE_TYPE_X86 ||
      elf_machine_type == ELF_MACHINE_TYPE_XTENSA) {
    object->dwarf.min_instruction_length = 1;
  }
}

void AsmObjectDestruct(AsmObject* object) {
  VectorDestructWithContents(
      &object->sections, (VectorElementDestructor)AssemblerSectionDestruct,
      /*free_element=*/true);
  VectorDestructWithContents(
      &object->relocations, (VectorElementDestructor)AssemblerRelocationDestruct,
      /*free_element=*/true);
  ClearAssemblerSymbolTable(object);
  HashTableDestruct(&object->symbol_table);
  for (size_t i = 0; i < object->orphan_symbols.length; i++) {
    AssemblerSymbol* sym = object->orphan_symbols.value.p[i];
    AssemblerSymbolDelete(sym);
    free(sym);
  }
  VectorDestruct(&object->orphan_symbols);
  VectorDestructWithContents(&object->operations, NULL, true);
  StringDestruct(&object->filename);
  DwarfDestruct(&object->dwarf);
}

void AsmObjectClearSymbols(AsmObject* object) {
  ClearAssemblerSymbolTable(object);
}

void AsmObjectReset(AsmObject* object, bool clear_symbols) {
  for (size_t i = 0; i < object->sections.length; i++) {
    AssemblerSection* section = object->sections.value.p[i];
    if (section->has_subsections) {
      for (int s = 0; s < section->subsection_cap; s++) {
        section->prev_sub_size[s] = section->sub_size[s];
        section->sub_size[s] = 0;
        BufferClear(&section->sub_bytes[s]);
      }
      section->current_subsection = 0;
    }
    section->address = 0;
  }
  if (clear_symbols) {
    AsmObjectClearSymbols(object);
  }
}

static void CollectSymbolNode(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  VectorAppend(data, node);
}

static void CollectAndFreeSymbolTree(void* table, void* data) {
  BinaryTreeTraverse(table, CollectSymbolNode, data);
  free(table);
}

void AsmObjectReserveSymbols(AsmObject* object, size_t count) {
  size_t size = (count + count / 2) | 1;
  HashTable* table = &object->symbol_table;
  if (size <= table->size) {
    return;
  }
  void** entries = calloc(size, sizeof(void*));
  if (entries == NULL) {
    return;
  }
  Vector symbols;
  VectorInit(&symbols);
  HashTableTraverse(table, CollectAndFreeSymbolTree, &symbols);
  free(table->entries);
  table->entries = entries;
  table->size = size;
  table->object_count = 0;
  for (size_t i = 0; i < symbols.length; i++) {
    BinaryTreeNodeInit(symbols.value.p[i]);
    HashTableInsert(table, symbols.value.p[i]);
  }
  VectorDestruct(&symbols);
}

AssemblerSymbol* AsmObjectFindSymbol(AsmObject* object, const char* name) {
  return HashTableSearch(&object->symbol_table, (void*)name);
}

void AsmObjectTrackOrphanSymbol(AsmObject* object, AssemblerSymbol* sym) {
  VectorAppend(&object->orphan_symbols, sym);
}

void AsmObjectInsertSymbol(AsmObject* object, AssemblerSymbol* sym) {
  if (object->pass != 1) {
    AsmObjectTrackOrphanSymbol(object, sym);
    return;
  }
  HashTableInsert(&object->symbol_table, sym);
}

static size_t SubsectionBase(const AssemblerSection* section, int subsection) {
  size_t base = 0;
  for (int i = 0; i < subsection && i < section->subsection_cap; i++) {
    base += section->prev_sub_size[i];
  }
  return base;
}

static void NoteEmitted(AssemblerSection* section, size_t count) {
  if (!section->has_subsections) {
    section->address += count;
    return;
  }
  int sub = section->current_subsection;
  section->sub_size[sub] += count;
  section->address = SubsectionBase(section, sub) + section->sub_size[sub];
}

static Buffer* EmitDestination(AssemblerSection* section) {
  if (section->has_subsections) {
    return &section->sub_bytes[section->current_subsection];
  }
  return &section->contents.data.buffered;
}

static bool EnsureSubsectionSlot(AssemblerSection* section, int subsection) {
  if (subsection < section->subsection_cap) {
    return true;
  }
  int cap = section->subsection_cap == 0 ? 4 : section->subsection_cap;
  while (cap <= subsection) {
    if (cap > 8192) {
      cap = 8193;
      break;
    }
    cap *= 2;
  }
  if (subsection >= cap) {
    return false;
  }
  size_t* sizes = realloc(section->sub_size, (size_t)cap * sizeof(size_t));
  if (sizes == NULL) {
    return false;
  }
  section->sub_size = sizes;
  size_t* prev = realloc(section->prev_sub_size, (size_t)cap * sizeof(size_t));
  if (prev == NULL) {
    return false;
  }
  section->prev_sub_size = prev;
  Buffer* bytes = realloc(section->sub_bytes, (size_t)cap * sizeof(Buffer));
  if (bytes == NULL) {
    return false;
  }
  section->sub_bytes = bytes;
  for (int i = section->subsection_cap; i < cap; i++) {
    section->sub_size[i] = 0;
    section->prev_sub_size[i] = 0;
    BufferInit(&section->sub_bytes[i]);
  }
  section->subsection_cap = cap;
  return true;
}

bool AsmObjectSelectSubsection(AsmObject* object, int subsection) {
  if (subsection < 0 || subsection > 8192) {
    return false;
  }
  if (object->current_section < 0 ||
      (size_t)object->current_section >= object->sections.length) {
    return false;
  }
  AssemblerSection* section =
      object->sections.value.p[object->current_section];
  if (!section->has_subsections && subsection == section->current_subsection) {
    return true;
  }
  if (!EnsureSubsectionSlot(section, subsection)) {
    return false;
  }
  if (!section->has_subsections) {
    section->sub_size[0] = (size_t)section->address;
    section->has_subsections = true;
    object->requires_layout_pass = true;
    if (object->pass == ASM_OBJECT_FINAL_PASS &&
        section->contents.data_location == kSectionContentsBuffered &&
        section->contents.data.buffered.length > 0) {
      BufferAppend(&section->sub_bytes[0], section->contents.data.buffered.value,
                   section->contents.data.buffered.length);
      BufferClear(&section->contents.data.buffered);
    }
  }
  section->current_subsection = subsection;
  section->address =
      SubsectionBase(section, subsection) + section->sub_size[subsection];
  return true;
}

void AsmObjectFinishSubsections(AsmObject* object) {
  for (size_t i = 0; i < object->sections.length; i++) {
    AssemblerSection* section = object->sections.value.p[i];
    if (!section->has_subsections) {
      continue;
    }
    size_t total = 0;
    Buffer combined;
    BufferInit(&combined);
    for (int s = 0; s < section->subsection_cap; s++) {
      total += section->sub_size[s];
      if (section->sub_bytes[s].length > 0) {
        BufferAppend(&combined, section->sub_bytes[s].value,
                     section->sub_bytes[s].length);
      }
    }
    if (section->contents.data_location == kSectionContentsBuffered) {
      BufferDestruct(&section->contents.data.buffered);
      section->contents.data.buffered = combined;
    } else {
      BufferDestruct(&combined);
    }
    section->contents.size = total;
  }
}

static void EmitBytes(AsmObject* object, int section, const void* bytes,
                      size_t count) {
  AssemblerSection* sect = object->sections.value.p[section];
  if (object->pass == ASM_OBJECT_FINAL_PASS) {
    if (sect->contents.data_location == kSectionContentsBuffered) {
      BufferAppend(EmitDestination(sect), (char*)bytes, count);
    }
    if (!sect->has_subsections) {
      sect->contents.size += count;
    }
  }
  NoteEmitted(sect, count);
}

void AsmObjectEmitWord(AsmObject* object, int section, int32_t word) {
  EmitBytes(object, section, &word, 4);
}

void AsmObjectEmitByte(AsmObject* object, int section, uint8_t byte) {
  EmitBytes(object, section, &byte, 1);
}

void AsmObjectEmitHalf(AsmObject* object, int section, uint16_t half) {
  EmitBytes(object, section, &half, 2);
}

void AsmObjectEmitLong(AsmObject* object, int section, uint64_t l) {
  EmitBytes(object, section, &l, 8);
}

void AsmObjectEmitFill(AsmObject* object, int section, int64_t num_bytes,
                        int fill) {
  if (num_bytes < 0) {
    return;
  }
  AssemblerSection* sect = object->sections.value.p[section];
  if (object->pass == ASM_OBJECT_FINAL_PASS) {
    if (sect->contents.data_location == kSectionContentsBuffered) {
      BufferFill(EmitDestination(sect), (size_t)num_bytes, (char)fill);
    }
    if (!sect->has_subsections) {
      sect->contents.size += (size_t)num_bytes;
    }
  }
  NoteEmitted(sect, (size_t)num_bytes);
}

void AsmObjectEmitUleb128(AsmObject* object, int section, uint64_t value) {
  do {
    uint8_t byte = (uint8_t)(value & 0x7f);
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }
    AsmObjectEmitByte(object, section, byte);
  } while (value != 0);
}

void AsmObjectEmitSleb128(AsmObject* object, int section, int64_t value) {
  int more = 1;
  while (more) {
    uint8_t byte = (uint8_t)(value & 0x7f);
    value >>= 7;
    if ((value == 0 && (byte & 0x40) == 0) ||
        (value == -1 && (byte & 0x40) != 0)) {
      more = 0;
    } else {
      byte |= 0x80;
    }
    AsmObjectEmitByte(object, section, byte);
  }
}

void AsmObjectAlignCurrentSection(AsmObject* object, int alignment) {
  AsmObjectAlignCurrentSectionWithFill(object, alignment, 0, -1);
}

void AsmObjectAlignCurrentSectionWithFill(AsmObject* object, int alignment,
                                          int fill, int64_t max_skip) {
  if (object->current_section < 0 ||
      (size_t)object->current_section >= object->sections.length ||
      alignment <= 1) {
    return;
  }
  AssemblerSection* sect =
      object->sections.value.p[object->current_section];
  size_t remainder = sect->address % (size_t)alignment;
  size_t num_bytes = remainder == 0 ? 0 : (size_t)alignment - remainder;
  if (max_skip >= 0 && num_bytes > (uint64_t)max_skip) {
    return;
  }
  if (alignment > sect->alignment) {
    sect->alignment = alignment;
  }
  if (fill >= 0) {
    AsmObjectEmitFill(object, object->current_section, (int64_t)num_bytes,
                      fill);
    return;
  }
  if (object->elf_machine_type == ELF_MACHINE_TYPE_AARCH64 ||
      object->elf_machine_type == ELF_MACHINE_TYPE_RISC_V) {
    // Fixed 4-byte NOPs; a leading partial word can't be reached by
    // execution, so it is zero.
    uint32_t nop = object->elf_machine_type == ELF_MACHINE_TYPE_AARCH64
                       ? 0xd503201fu
                       : 0x00000013u;  // addi x0, x0, 0
    AsmObjectEmitFill(object, object->current_section,
                      (int64_t)(num_bytes % 4), 0);
    for (size_t i = 0; i < num_bytes / 4; i++) {
      for (int b = 0; b < 4; b++) {
        AsmObjectEmitByte(object, object->current_section,
                          (uint8_t)(nop >> (8 * b)));
      }
    }
    return;
  }
  // The recommended x86 NOP sequences, indexed by length - 1.
  static const uint8_t kX86Nops[8][8] = {
      {0x90},
      {0x66, 0x90},
      {0x0f, 0x1f, 0x00},
      {0x0f, 0x1f, 0x40, 0x00},
      {0x0f, 0x1f, 0x44, 0x00, 0x00},
      {0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00},
      {0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00},
      {0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00},
  };
  while (num_bytes > 0) {
    size_t n = num_bytes < 8 ? num_bytes : 8;
    for (size_t i = 0; i < n; i++) {
      AsmObjectEmitByte(object, object->current_section, kX86Nops[n - 1][i]);
    }
    num_bytes -= n;
  }
}

static bool ElfRelocationIsTls(uint16_t machine, int32_t type) {
  switch (machine) {
    case ELF_MACHINE_TYPE_AARCH64:
      return (type >= R_AARCH64_P32_TLS_DTPREL &&
              type <= R_AARCH64_P32_TLSDESC) ||
             (type >= R_AARCH64_TLSGD_ADR_PREL21 &&
              type <= R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC) ||
             (type >= R_AARCH64_TLS_DTPMOD && type <= R_AARCH64_TLSDESC);
    case ELF_MACHINE_TYPE_ARM:
      return type == R_ARM_TLS_DTPMOD32 || type == R_ARM_TLS_DTPREL32 ||
             type == R_ARM_TLS_TPOFF32 ||
             (type >= R_ARM_TLS_GD32 && type <= R_ARM_TLS_LE32);
    case ELF_MACHINE_TYPE_X86_64:
      return type == R_X86_64_DTPMOD64 || type == R_X86_64_DTPOFF64 ||
             type == R_X86_64_TPOFF64 || type == R_X86_64_TLSGD ||
             type == R_X86_64_TLSLD || type == R_X86_64_DTPOFF32 ||
             type == R_X86_64_GOTTPOFF || type == R_X86_64_TPOFF32;
    case ELF_MACHINE_TYPE_X86:
      return type == R_386_TLS_LE;
    case ELF_MACHINE_TYPE_RISC_V:
      return (type >= R_RISCV_TLS_DTPMOD32 && type <= R_RISCV_TLS_TPREL64) ||
             type == R_RISCV_TLS_GOT_HI20 || type == R_RISCV_TLS_GD_HI20 ||
             (type >= R_RISCV_TPREL_HI20 && type <= R_RISCV_TPREL_ADD) ||
             type == R_RISCV_TPREL_I || type == R_RISCV_TPREL_S;
    default:
      return false;
  }
}

void AsmObjectAddRelocation(AsmObject* object, AssemblerRelocation* reloc) {
  if (object->pass != ASM_OBJECT_FINAL_PASS) {
    AssemblerRelocationDelete(reloc);
    return;
  }
  reloc->symbol->exported = true;
  // GNU ld requires an undefined TLS symbol to be STT_TLS.  daveld is
  // lenient; host ld is not.
  if (ElfRelocationIsTls(object->elf_machine_type, reloc->type)) {
    reloc->symbol->type = SYM_TYPE(tls);
  }
  VectorAppend(&object->relocations, reloc);
}

void AsmObjectAddRelocationForSymbol(AsmObject* object, AssemblerSymbol* symbol,
                                     int32_t type, int32_t section,
                                     int32_t offset, int32_t addend) {
  if (object->pass != ASM_OBJECT_FINAL_PASS) {
    return;
  }
  AsmObjectAddRelocation(
      object, NewAssemblerRelocation(symbol, type, section, offset, addend));
}

int AsmObjectAddSection(AsmObject* object, String* name, int32_t type,
                        int32_t flags, int32_t alignment) {
  if (name != NULL) {
    if (StringEqual(name, ".text") || StringStartsWith(name, ".text.")) {
      flags |= SHF(alloc) | SHF(execinstr);
      type = SHT(progbits);
    } else if (StringEqual(name, ".data") || StringStartsWith(name, ".data.")) {
      flags |= SHF(alloc) | SHF(write);
      type = SHT(progbits);
    } else if (StringEqual(name, ".rodata") ||
               StringStartsWith(name, ".rodata.")) {
      flags |= SHF(alloc);
      if (type == SHT(null)) {
        type = SHT(progbits);
      }
    } else if (StringEqual(name, ".ARM.exidx")) {
      type = SHT(ARM_EXIDX);
      flags |= SHF(link_order) | SHF(alloc);
      if (alignment < 4) {
        alignment = 4;
      }
    } else if (StringEqual(name, ".ARM.extab")) {
      type = SHT(progbits);
      flags |= SHF(alloc);
      if (alignment < 2) {
        alignment = 2;
      }
    }
  }
  AssemblerSection* section = NewAssemblerSection(name, type, flags, alignment);
  VectorAppend(&object->sections, section);
  return (int)object->sections.length - 1;
}

int AsmObjectFindSection(AsmObject* object, String* name) {
  for (size_t i = 0; i < object->sections.length; i++) {
    AssemblerSection* section = object->sections.value.p[i];
    if (section->name != NULL && StringEqualString(section->name, name)) {
      return (int)i;
    }
  }
  return -1;
}

int AsmObjectEnsureSection(AsmObject* object, String* name, int32_t type,
                           int32_t flags, int32_t alignment) {
  int section = AsmObjectFindSection(object, name);
  if (section >= 0) {
    StringDelete(name);
    return section;
  }
  return AsmObjectAddSection(object, name, type, flags, alignment);
}

void AsmObjectSwitchSection(AsmObject* object, int section) {
  AsmObjectSetCurrentSection(object, section);
}

void AsmObjectSetSectionSize(AsmObject* object, size_t index, size_t size) {
  if (index < object->sections.length) {
    AssemblerSection* section = object->sections.value.p[index];
    section->contents.size = size;
  }
}

void AsmObjectSetCurrentSection(AsmObject* object, int32_t section) {
  object->current_section = section;
}

int64_t AsmObjectCurrentAddress(AsmObject* object) {
  AssemblerSection* section = object->sections.value.p[object->current_section];
  return section->address;
}

static AssemblerSymbol* NewLabel(AsmObject* object, const char* name,
                                 bool stable_name) {
  AssemblerSymbol* sym = AllocAssemblerSymbol(
      name, stable_name, object->current_section, SYM_TYPE(none),
      SYM_BIND(local), AsmObjectCurrentAddress(object));
  sym->is_label = true;
  sym->defined = true;
  sym->defined_pass = object->pass;
  AsmObjectInsertSymbol(object, sym);
  return sym;
}

AssemblerSymbol* AsmObjectDefineLabel(AsmObject* object, Assembler* assembler,
                                      String* spelling) {
  const char* name = spelling->value;
  AssemblerSymbol* sym = AsmObjectFindSymbol(object, name);
  if (sym != NULL) {
    if (!sym->defined) {
      if (name[0] == '.') {
        sym->binding = SYM_BIND(local);
      }
      sym->defined = true;
      sym->defined_pass = object->pass;
      sym->is_label = true;
      sym->section = object->current_section;
      sym->value = AsmObjectCurrentAddress(object);
    } else {
      AssemblerError(assembler, "Duplicate symbol %s", name);
    }
  } else {
    sym = NewLabel(object, name, false);
  }
  return sym;
}

AssemblerSymbol* AsmObjectNewLabelWithStableName(AsmObject* object,
                                                 const char* name) {
  return NewLabel(object, name, true);
}

void AsmObjectBeginRecording(AsmObject* object) {
  object->recording_operations = true;
}

void AsmObjectEndRecording(AsmObject* object) {
  object->recording_operations = false;
}

void AsmObjectRecordOperation(AsmObject* object, const AsmOperation* op) {
  if (!object->recording_operations) {
    return;
  }
  AsmOperation* copy = malloc(sizeof(AsmOperation));
  *copy = *op;
  VectorAppend(&object->operations, copy);
}

void AsmObjectReplayOperations(AsmObject* object, Assembler* assembler) {
  (void)assembler;
  for (size_t i = 0; i < object->operations.length; i++) {
    AsmOperation* op = object->operations.value.p[i];
    switch (op->kind) {
      case kAsmOpAlignSection:
        if (op->section == object->current_section) {
          AsmObjectAlignCurrentSection(object, op->u.align.alignment);
        }
        break;
      case kAsmOpSpace:
        AsmObjectEmitFill(object, op->section, op->u.space.size, op->u.space.fill);
        break;
      case kAsmOpEmitUleb128:
        AsmObjectEmitUleb128(object, op->section, (uint64_t)op->u.uleb128.value);
        break;
      case kAsmOpEmitSleb128:
        AsmObjectEmitSleb128(object, op->section, op->u.sleb128.value);
        break;
    }
  }
}

static int32_t SymbolTypeToELFType(AssemblerSymbolType type) {
  switch (type) {
    case SYM_TYPE(none):
      return STT(notype);
    case SYM_TYPE(func):
      return STT(func);
    case SYM_TYPE(object):
      return STT(object);
    case SYM_TYPE(common):
      return STT(common);
    case SYM_TYPE(tls):
      return STT(tls);
    default:
      assert(false);
      return 0;
  }
}

static int32_t SymbolBindingToELFBinding(AssemblerSymbolBinding binding) {
  switch (binding) {
    case SYM_BIND(global):
      return STB(global);
    case SYM_BIND(local):
      return STB(local);
    case SYM_BIND(weak):
      return STB(weak);
    default:
      assert(false);
      return 0;
  }
}

static int64_t SymbolELFValue(AssemblerSymbol* sym) {
  if (sym->defined && sym->section == SHN_COM) {
    return sym->alignment > 0 ? sym->alignment : 1;
  }
  return sym->value;
}

// A `.set` constant is a number, not an offset into the section it was
// defined in; the linker must not add that section's address.
static int32_t SymbolELFSection(const AssemblerSymbol* sym) {
  if (!sym->defined) {
    return 0;
  }
  return sym->is_constant ? SHN_ABS : sym->section;
}

static bool SymbolIsELFGlobal(const AssemblerSymbol* sym) {
  return sym->binding == SYM_BIND(global) || sym->binding == SYM_BIND(weak) ||
         !sym->defined;
}

static void AddLocalSymbolFunc(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  ELFWriterFile* elf = data;
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  // ELF requires every local to precede sh_info. Implicit undefined
  // names (`bl foo` without `.global`) are emitted as global UNDEF, so
  // they must not be written in this pass or GNU ld treats them as local
  // and leaves references unresolved.
  if (sym->exported && sym->binding == SYM_BIND(local) &&
      !SymbolIsELFGlobal(sym)) {
    ELFWriterAddSymbol(elf, &sym->name, SymbolELFSection(sym),
                       SymbolTypeToELFType(sym->type), STB(local), sym->size,
                       SymbolELFValue(sym), &sym->index);
  }
}

static void AddLocalSymbolToELFFile(void* entry, void* data) {
  BinaryTreeTraverse(entry, AddLocalSymbolFunc, data);
}

static void AddGlobalSymbolFunc(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  ELFWriterFile* elf = data;
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  if (sym->exported && SymbolIsELFGlobal(sym)) {
    AssemblerSymbolBinding binding = sym->binding;
    if (binding == SYM_BIND(local)) {
      binding = SYM_BIND(global);
    }
    ELFWriterAddSymbol(elf, &sym->name, SymbolELFSection(sym),
                       SymbolTypeToELFType(sym->type),
                       SymbolBindingToELFBinding(binding), sym->size,
                       SymbolELFValue(sym), &sym->index);
  }
}

static void AddGlobalSymbolToELFFile(void* entry, void* data) {
  BinaryTreeTraverse(entry, AddGlobalSymbolFunc, data);
}

static void AddSections(AsmObject* object, ELFWriterFile* elf) {
  int32_t debug_line_index = -1;
  int32_t* section_symbol_indexes =
      malloc(sizeof(int32_t) * (object->sections.length + 1));
  for (size_t i = 0; i < object->sections.length; i++) {
    AssemblerSection* section = object->sections.value.p[i];

    bool align = true;
    bool is_debug_line_section = StringEqual(section->name, ".debug_line");
    bool is_debug_section =
        section->name != NULL && section->name->length >= 7 &&
        memcmp(section->name->value, ".debug_", 7) == 0;

    if (is_debug_section) {
      section->alignment = 1;
    }

    if (is_debug_line_section) {
      section->alignment = 1;
      align = false;
      ELFWriterSectionContentsInit(&section->contents, kSectionContentsBuffered);
      DwarfBuildDebugLineContents(&object->dwarf,
                                  &section->contents.data.buffered);
    }

    if (align) {
      AssemblerSectionAlign(section, section->alignment);
    }

    int64_t entry_size = 0;
    if (section->type == SHT(init_array) ||
        section->type == SHT(fini_array) ||
        section->type == SHT(preinit_array)) {
      entry_size = object->elf_machine_type == ELF_MACHINE_TYPEW65C02
                       ? 2
                       : (object->is_64_bit ? 8 : 4);
    }
    ELFWriterSection* elf_section =
        ELFWriterAddSection(elf, section->name, section->type, section->flags,
                            section->alignment, &section->contents, entry_size);

    if (is_debug_line_section) {
      debug_line_index = elf_section->index;
    }

    if (section->name != NULL &&
        ELFWriterSectionContentsGetLength(&section->contents) != 0) {
      AssemblerSymbol* section_symbol =
          NewAssemblerSymbol(section->name->value, elf_section->index,
                             SYM_TYPE(none), SYM_BIND(local), 0);
      section_symbol->exported = true;
      section_symbol->defined = true;
      AsmObjectInsertSymbol(object, section_symbol);
    }
    // The null section has no section symbol; readers that resolve a section
    // symbol's name through its st_shndx reject one for index 0.
    if (elf_section->index == 0) {
      section_symbol_indexes[i] = 0;
      continue;
    }
    section_symbol_indexes[i] = (int32_t)elf->symbol_table.length;
    ELFWriterAddSectionSymbol(elf, &elf_section->name, elf_section->index);
  }

  // Each line sequence's DW_LNE_set_address is relocated against the section
  // symbol of the section holding its code.
  for (size_t f = 0; debug_line_index >= 0 &&
                     f < object->dwarf.address_fixups.length; f++) {
    DwarfAddressFixup* fixup = object->dwarf.address_fixups.value.p[f];
    if (fixup->section < 0 ||
        (size_t)fixup->section >= object->sections.length) {
      continue;
    }
    ELFWriterAddRelocationWithAddend(
        elf, debug_line_index, fixup->offset,
        section_symbol_indexes[fixup->section], 0,
        object->reloc_types[kRelocSet64]);
  }
  free(section_symbol_indexes);
}

static void AddRelocations(AsmObject* object, ELFWriterFile* elf) {
  for (size_t i = 0; i < object->relocations.length; i++) {
    AssemblerRelocation* reloc = object->relocations.value.p[i];
    ELFWriterAddRelocationWithAddend(elf, reloc->section, reloc->offset,
                                     reloc->symbol->index, reloc->addend,
                                     reloc->type);
  }
}

void AsmObjectWriteELF(AsmObject* object, FILE* out) {
  ELFWriterFile elf;
  ELFWriterFileInit(&elf, ET(rel), object->elf_machine_type, object->elf_flags,
                    NULL, object->is_64_bit, object->is_little_endian);

  if (object->filename.length != 0) {
    ELFWriterAddFileSymbol(&elf, &object->filename);
  }

  AddSections(object, &elf);

  HashTableTraverse(&object->symbol_table, AddLocalSymbolToELFFile, &elf);
  // sh_info is one greater than the index of the last local symbol.
  elf.last_local_symbol_index = (int32_t)elf.symbol_table.length - 1;

  HashTableTraverse(&object->symbol_table, AddGlobalSymbolToELFFile, &elf);

  AddRelocations(object, &elf);

  ELFWriterFileWrite(&elf, out);
  ELFWriterFileDestruct(&elf);
}
