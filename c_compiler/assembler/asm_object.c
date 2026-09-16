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
  return section;
}

void AssemblerSectionDestruct(AssemblerSection* section) {
  ELFWriterSectionContentsDestruct(&section->contents);
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

AssemblerSymbol* NewAssemblerSymbol(const char* name, int section,
                                    AssemblerSymbolType type,
                                    AssemblerSymbolBinding binding,
                                    int64_t value) {
  AssemblerSymbol* sym = malloc(sizeof(AssemblerSymbol));
  BinaryTreeNodeInit(&sym->header);
  StringInit(&sym->name, name);
  sym->type = type;
  sym->binding = binding;
  sym->value = value;
  sym->size = 0;
  sym->defined = false;
  sym->section = section;
  sym->index = -1;
  sym->exported = false;
  sym->alignment = 1;
  sym->is_label = false;
  sym->is_constant = false;
  sym->is_forward_declared = false;
  return sym;
}

void AssemblerSymbolDelete(AssemblerSymbol* sym) { StringDestruct(&sym->name); }

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

static void ClearAssemblerSymbolTable(HashTable* table) {
  HashTableTraverse(table, DeleteSymbolTable, NULL);
  HashTableClear(table);
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
  object->elf_machine_type = elf_machine_type;
  object->elf_flags = elf_flags;
  switch (elf_machine_type) {
    case ELF_MACHINE_TYPE_ARM:
    case ELF_MACHINE_TYPE_X86:
      object->is_64_bit = false;
      break;
    default:
      object->is_64_bit = true;
      break;
  }
  object->is_little_endian = true;
  object->reloc_types = reloc_types;
  object->pic = false;
  object->absolute = false;
  object->current_section = 0;
  StringInit(&object->filename, "");
  DwarfInit(&object->dwarf);
  if (elf_machine_type == ELF_MACHINE_TYPEW65C02 ||
      elf_machine_type == ELF_MACHINE_TYPE_X86_64 ||
      elf_machine_type == ELF_MACHINE_TYPE_X86) {
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
  ClearAssemblerSymbolTable(&object->symbol_table);
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
  ClearAssemblerSymbolTable(&object->symbol_table);
}

void AsmObjectReset(AsmObject* object, bool clear_symbols) {
  for (size_t i = 0; i < object->sections.length; i++) {
    AssemblerSection* section = object->sections.value.p[i];
    section->address = 0;
  }
  if (clear_symbols) {
    AsmObjectClearSymbols(object);
  }
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

void AsmObjectEmitWord(AsmObject* object, int section, int32_t word) {
  AssemblerSection* sect = object->sections.value.p[section];
  if (object->pass == ASM_OBJECT_FINAL_PASS) {
    BufferAppend(&sect->contents.data.buffered, (char*)&word, 4);
    sect->contents.size += 4;
  }
  sect->address += 4;
}

void AsmObjectEmitByte(AsmObject* object, int section, uint8_t byte) {
  AssemblerSection* sect = object->sections.value.p[section];
  if (object->pass == ASM_OBJECT_FINAL_PASS) {
    BufferAppend(&sect->contents.data.buffered, (char*)&byte, 1);
    sect->contents.size += 1;
  }
  sect->address += 1;
}

void AsmObjectEmitHalf(AsmObject* object, int section, uint16_t half) {
  AssemblerSection* sect = object->sections.value.p[section];
  if (object->pass == ASM_OBJECT_FINAL_PASS) {
    BufferAppend(&sect->contents.data.buffered, (char*)&half, 2);
    sect->contents.size += 2;
  }
  sect->address += 2;
}

void AsmObjectEmitLong(AsmObject* object, int section, uint64_t l) {
  AssemblerSection* sect = object->sections.value.p[section];
  if (object->pass == ASM_OBJECT_FINAL_PASS) {
    BufferAppend(&sect->contents.data.buffered, (char*)&l, 8);
    sect->contents.size += 8;
  }
  sect->address += 8;
}

void AsmObjectEmitFill(AsmObject* object, int section, int64_t num_bytes,
                        int fill) {
  AssemblerSection* sect = object->sections.value.p[section];
  if (object->pass == ASM_OBJECT_FINAL_PASS) {
    if (sect->contents.data_location == kSectionContentsBuffered) {
      BufferFill(&sect->contents.data.buffered, num_bytes, fill);
    }
    sect->contents.size += num_bytes;
  }
  sect->address += num_bytes;
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
  if (object->current_section < 0 ||
      (size_t)object->current_section >= object->sections.length ||
      alignment <= 1) {
    return;
  }
  AssemblerSection* sect =
      object->sections.value.p[object->current_section];
  if (alignment > sect->alignment) {
    sect->alignment = alignment;
  }
  size_t remainder = sect->address % (size_t)alignment;
  size_t next_address =
      remainder == 0 ? sect->address
                     : sect->address + (size_t)alignment - remainder;
  size_t num_bytes = next_address - sect->address;
  if (object->pass == ASM_OBJECT_FINAL_PASS) {
    if (sect->contents.data_location == kSectionContentsBuffered) {
      BufferAddSpace(&sect->contents.data.buffered, num_bytes);
    }
    sect->contents.size += num_bytes;
  }
  sect->address = next_address;
}

void AsmObjectAddRelocation(AsmObject* object, AssemblerRelocation* reloc) {
  if (object->pass != ASM_OBJECT_FINAL_PASS) {
    AssemblerRelocationDelete(reloc);
    return;
  }
  reloc->symbol->exported = true;
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

AssemblerSymbol* AsmObjectDefineLabel(AsmObject* object, Assembler* assembler,
                                      String* spelling) {
  AssemblerSymbol* sym = AsmObjectFindSymbol(object, spelling->value);
  if (sym != NULL) {
    if (!sym->defined) {
      if (spelling->value[0] == '.') {
        sym->binding = SYM_BIND(local);
      }
      sym->defined = true;
      sym->is_label = true;
      sym->section = object->current_section;
      sym->value = AsmObjectCurrentAddress(object);
    } else {
      AssemblerError(assembler, "Duplicate symbol %s", spelling->value);
    }
  } else {
    sym = NewAssemblerSymbol(spelling->value, object->current_section,
                             SYM_TYPE(none), SYM_BIND(local),
                             AsmObjectCurrentAddress(object));
    sym->is_label = true;
    sym->defined = true;
    AsmObjectInsertSymbol(object, sym);
  }
  return sym;
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

static void AddLocalSymbolFunc(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  ELFWriterFile* elf = data;
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  if (sym->exported && sym->binding == SYM_BIND(local)) {
    int section_index = sym->section;
    AssemblerSymbolBinding binding = sym->binding;
    if (!sym->defined) {
      section_index = 0;
      binding = SYM_BIND(global);
    }
    ELFWriterAddSymbol(elf, &sym->name, section_index,
                       SymbolTypeToELFType(sym->type),
                       SymbolBindingToELFBinding(binding), sym->size,
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
  if (sym->exported &&
      (sym->binding == SYM_BIND(global) || sym->binding == SYM_BIND(weak))) {
    ELFWriterAddSymbol(elf, &sym->name, sym->defined ? sym->section : 0,
                       SymbolTypeToELFType(sym->type),
                       SymbolBindingToELFBinding(sym->binding), sym->size,
                       SymbolELFValue(sym), &sym->index);
  }
}

static void AddGlobalSymbolToELFFile(void* entry, void* data) {
  BinaryTreeTraverse(entry, AddGlobalSymbolFunc, data);
}

static void AddSections(AsmObject* object, ELFWriterFile* elf) {
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
      if (object->dwarf.address_fixups.length == 0) {
        AssemblerSymbol* text = AsmObjectFindSymbol(object, ".text");
        if (text != NULL) {
          AssemblerRelocation* addr_reloc = DwarfDebugLineRelocation(
              &object->dwarf, text, object->reloc_types[kRelocSet64],
              elf_section->index);
          AsmObjectAddRelocation(object, addr_reloc);
        }
      }
      for (size_t f = 0; f < object->dwarf.address_fixups.length; f++) {
        DwarfAddressFixup* fixup = object->dwarf.address_fixups.value.p[f];
        AssemblerSymbol* sym = NULL;
        if (fixup->section >= 0 &&
            (size_t)fixup->section < object->sections.length) {
          AssemblerSection* src = object->sections.value.p[fixup->section];
          if (src->name != NULL) {
            sym = AsmObjectFindSymbol(object, src->name->value);
          }
        }
        if (sym == NULL) {
          sym = AsmObjectFindSymbol(object, ".text");
        }
        if (sym != NULL) {
          object->dwarf.address_offset = fixup->offset;
          AssemblerRelocation* addr_reloc = DwarfDebugLineRelocation(
              &object->dwarf, sym, object->reloc_types[kRelocSet64],
              elf_section->index);
          AsmObjectAddRelocation(object, addr_reloc);
        }
      }
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
    ELFWriterAddSectionSymbol(elf, &elf_section->name, elf_section->index);
  }
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
