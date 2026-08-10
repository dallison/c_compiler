//
//  assembler.c
//  c_compiler
//
//  Created by David Allison on 1/1/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "assembler.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "elf_writer.h"
#include "asm_expr.h"
#include "errors.h"

#define ASSEMBLER_FINAL_PASS 3

// Default label defining function.
static AssemblerSymbol* DefineLabel(Assembler* assembler, String* spelling);

//
// Forward declaration of static assembler directive handlers.
//
#define DECLARE_DIRECTIVE_FUNC(spelling) \
  static void HandleDirective_##spelling(Assembler*)

DECLARE_DIRECTIVE_FUNC(globl);
DECLARE_DIRECTIVE_FUNC(global);
DECLARE_DIRECTIVE_FUNC(type);
DECLARE_DIRECTIVE_FUNC(size);
DECLARE_DIRECTIVE_FUNC(align);
DECLARE_DIRECTIVE_FUNC(byte);
DECLARE_DIRECTIVE_FUNC(short);
DECLARE_DIRECTIVE_FUNC(2byte);
DECLARE_DIRECTIVE_FUNC(hword);
DECLARE_DIRECTIVE_FUNC(word);
DECLARE_DIRECTIVE_FUNC(4byte);
DECLARE_DIRECTIVE_FUNC(long);
DECLARE_DIRECTIVE_FUNC(8byte);
DECLARE_DIRECTIVE_FUNC(space);
DECLARE_DIRECTIVE_FUNC(p2align);
DECLARE_DIRECTIVE_FUNC(ascii);
DECLARE_DIRECTIVE_FUNC(asciz);
DECLARE_DIRECTIVE_FUNC(string);
DECLARE_DIRECTIVE_FUNC(set);
DECLARE_DIRECTIVE_FUNC(section);
DECLARE_DIRECTIVE_FUNC(text);
DECLARE_DIRECTIVE_FUNC(data);
DECLARE_DIRECTIVE_FUNC(comm);
DECLARE_DIRECTIVE_FUNC(local);
DECLARE_DIRECTIVE_FUNC(weak);
DECLARE_DIRECTIVE_FUNC(file);
DECLARE_DIRECTIVE_FUNC(loc);
DECLARE_DIRECTIVE_FUNC(option);
DECLARE_DIRECTIVE_FUNC(set);
DECLARE_DIRECTIVE_FUNC(chkaddr);
DECLARE_DIRECTIVE_FUNC(uleb128);
DECLARE_DIRECTIVE_FUNC(sleb128);

#undef DECLARE_DIRECTIVE_FUNC

// Add all directives to the map of directive name vs handling function.
#define DIRECTIVE(spelling) \
  do { \
    MapKeyValue kv; \
    kv.key.p = "." #spelling;\
    kv.value.p = HandleDirective_##spelling;\
    MapInsert(directives, kv); \
    } while(0)

static void InitializeDirectives(Map* directives) {
  DIRECTIVE(globl);
  DIRECTIVE(global);
  DIRECTIVE(type);
  DIRECTIVE(size);
  DIRECTIVE(align);
  DIRECTIVE(byte);
  DIRECTIVE(short);
  DIRECTIVE(2byte);
  DIRECTIVE(hword);
  DIRECTIVE(word);
  DIRECTIVE(4byte);
  DIRECTIVE(long);
  DIRECTIVE(8byte);
  DIRECTIVE(space);
  DIRECTIVE(p2align);
  DIRECTIVE(ascii);
  DIRECTIVE(asciz);
  DIRECTIVE(string);
  DIRECTIVE(set);
  DIRECTIVE(section);
  DIRECTIVE(text);
  DIRECTIVE(data);
  DIRECTIVE(comm);
  DIRECTIVE(local);
  DIRECTIVE(weak);
  DIRECTIVE(file);
  DIRECTIVE(loc);
  DIRECTIVE(option);
  DIRECTIVE(set);
  DIRECTIVE(chkaddr);
  DIRECTIVE(uleb128);
  DIRECTIVE(sleb128);
}

#undef DIRECTIVE

AssemblerSection* NewAssemblerSection(String* name, int32_t type, int32_t flags,
                                      int32_t alignment) {
  AssemblerSection* section = malloc(sizeof(AssemblerSection));
  section->name = name;
  ELFWriterSectionContentsInit(&section->contents, type == SHT(nobits) ?
                               kSectionContentsNobits :
                               kSectionContentsBuffered);
  section->address = 0;
  section->flags = flags;
  section->type = type;
  section->alignment = alignment;
  return section;
}

void AssemblerSectionDestruct(AssemblerSection* section) {
  ELFWriterSectionContentsDestruct(&section->contents);
  // The section owns its name string (handed over at creation in pass 1).
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

void AssemblerRelocationDestruct(AssemblerRelocation* reloc) {}

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

//
// Symbol table.  This is a hash table of binary trees.
//

static int SymbolInsertCompare(BinaryTreeNode* node1,
                                   BinaryTreeNode* node2) {
  AssemblerSymbol* sym1 = (AssemblerSymbol*)node1;
  AssemblerSymbol* sym2 = (AssemblerSymbol*)node2;
  return StringCompareString(&sym1->name, &sym2->name);
}

static int SymbolSearchCompare(BinaryTreeNode* node, void* name) {
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  return StringCompare(&sym->name, name);
}

static void SymbolDestructor(BinaryTreeNode* node, void* delete_symbols) {
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  AssemblerSymbolDelete(sym);
}

// Mapping function for hash table inserter.
static bool InsertSymbolIntoHashTable(void* table, void* node, void** parent) {
  BinaryTree* tree = table;
  if (table == NULL) {
    tree = NewBinaryTree(
                         SymbolInsertCompare,
                         SymbolSearchCompare,
                         SymbolDestructor);

    *parent = tree;
  }
  return BinaryTreeInsert(tree, node);
}

// Find a symbol given its name in the given symbol table.  This
// searches the binary tree using a recursive algorithm.
static AssemblerSymbol* FindAssemblerSymbol(BinaryTree* table, String* name) {
  AssemblerSymbol* node = (AssemblerSymbol*)BinaryTreeSearch(table, name);
  return node;
}

// Mapping function for hash table searcher.
static void* FindSymbolInHashTable(void* table, void* value) {
  BinaryTree* tree = table;
  return FindAssemblerSymbol(tree, value);
}

// Create a hash value from a given symbol node (passed as void* from
// hash table inserter and searcher functions.
static size_t HashSymbol(void* value, HashTable* table, HashMode mode) {
  const char* name;
  switch (mode) {
    case kHashInsert:
      // For insertion we have a pointer to symbol node.
      name = ((AssemblerSymbol*)value)->name.value;
      break;
    case kHashSearch:
      // For search we have pointer to a char containing the name
      // to find.
      name = (const char*)value;
      break;
  }
  uint32_t hash = 5381;
  while (*name != '\0') {
    hash = (hash << 5) + hash + *name++;
  }
  return hash;
}

static void DeleteSymbolTable(void* table, void* data) {
  BinaryTreeDestruct(table, data);
  free(table);
}


static void ClearAssemblerSymbolTable(HashTable* table) {
  HashTableTraverse(table, DeleteSymbolTable, NULL);
  HashTableClear(table);
}

AssemblerSymbol* AssemblerFindSymbol(Assembler* assembler, const char* name) {
  return HashTableSearch(&assembler->symbol_table, (void*)name);
}

void AssemblerExtractSymbolSuffix(String* symbol, String* name, String* suffix) {
  size_t index = StringIndexOf(symbol, "@");
  if (index == -1) {
    StringSetString(name, symbol);
    return;
  }
  StringAppendSegment(name, symbol->value, index);
  StringSet(suffix, symbol->value + index + 1);
}

void AssemblerTrackOrphanSymbol(Assembler* assembler, AssemblerSymbol* sym) {
  VectorAppend(&assembler->orphan_symbols, sym);
}

void AssemblerInsertSymbol(Assembler* assembler, AssemblerSymbol* sym) {
  // After pass 1 we don't insert any more symbols, but the caller has already
  // allocated this one (e.g. a symbol referenced only in a later pass). Track it so
  // it can be freed at destruct rather than leaked.
  if (assembler->pass != 1) {
    AssemblerTrackOrphanSymbol(assembler, sym);
    return;
  }
  HashTableInsert(&assembler->symbol_table, sym);
}

void AssemblerEmitWord(Assembler* assembler, int section, int32_t word) {
  AssemblerSection* sect = assembler->sections.value.p[section];
  if (assembler->pass == ASSEMBLER_FINAL_PASS) {
    BufferAppend(&sect->contents.data.buffered, (char*)&word, 4);
    sect->contents.size += 4;
  }
  sect->address += 4;
}

void AssemblerEmitByte(Assembler* assembler, int section, uint8_t byte) {
  AssemblerSection* sect = assembler->sections.value.p[section];
  if (assembler->pass == ASSEMBLER_FINAL_PASS) {
    BufferAppend(&sect->contents.data.buffered, (char*)&byte, 1);
    sect->contents.size += 1;
  }
  sect->address += 1;
}

void AssemblerEmitHalf(Assembler* assembler, int section, uint16_t half) {
  AssemblerSection* sect = assembler->sections.value.p[section];
  if (assembler->pass == ASSEMBLER_FINAL_PASS) {
    BufferAppend(&sect->contents.data.buffered, (char*)&half, 2);
    sect->contents.size += 2;
  }
  sect->address += 2;
}

void AssemblerEmitLong(Assembler* assembler, int section, uint64_t l) {
  AssemblerSection* sect = assembler->sections.value.p[section];
  if (assembler->pass == ASSEMBLER_FINAL_PASS) {
    BufferAppend(&sect->contents.data.buffered, (char*)&l, 8);
    sect->contents.size += 8;
  }
  sect->address += 8;
}

int64_t AssemblerEvaluateExpression(Assembler* assembler) {
  return AssemblerEvaluateKnownExpression(assembler, NULL);
}

int64_t AssemblerEvaluateKnownExpression(Assembler* assembler, bool* known) {
  int64_t value = 0;
  bool ok = AssemblerEvaluateExpressionInternal(assembler, &value);
  if (known != NULL) {
    *known = ok;
  }
  if (!ok && assembler->pass == ASSEMBLER_FINAL_PASS) {
    AssemblerError(assembler, "Invalid expression");
  }
  return value;
}

double AssemblerGetDoubleConst(Assembler* assembler) {
  double v = 0;
  bool negative = LexMatch(&assembler->lex, TOK(minus));
  if (LexLookingAt(&assembler->lex, TOK(fnumber))) {
    v = assembler->lex.fnumber;
    LexNextToken(&assembler->lex);
  } else if (LexLookingAt(&assembler->lex, TOK(number))) {
    v = (double)assembler->lex.number;
    LexNextToken(&assembler->lex);
  } else {
    AssemblerError(assembler, "Floating point constant expected");
  }
  if (negative) {
    v = -v;
  }
  return v;
}

LocationEntry* NewLocationEntry(int file, int line, int col, uint64_t address) {
  LocationEntry* loc = malloc(sizeof(LocationEntry));
  loc->file = file;
  loc->line = line;
  loc->col = col;
  loc->address = address;
  return loc;
}

static int CompareCharPointer(const void* a, const void* b) {
  MapKeyValue* s1 = (MapKeyValue*)a;
  MapKeyValue* s2 = (MapKeyValue*)b;
  return strcmp(s1->key.p, s2->key.p);
}

static bool AssemblerInitCommon(Assembler* assembler, int16_t elf_machine_type,
                                uint16_t elf_flags, int* reloc_types,
                                String* outfile) {
  // Open output file.
  assembler->out = fopen(outfile->value, "w");
  if (assembler->out == NULL) {
    fprintf(stderr, "Cannot open object file %s\n", outfile->value);
    return false;
  }

  PreprocessorInit(&assembler->preprocessor);
  StringInit(&assembler->filename, "");

  MapInit(&assembler->directives, CompareCharPointer);
  InitializeDirectives(&assembler->directives);

  VectorInit(&assembler->sections);
  VectorInit(&assembler->relocations);
  VectorInit(&assembler->orphan_symbols);
  assembler->pass = 0;
  assembler->num_errors = 0;
  HashTableInit(&assembler->symbol_table, "assembler_symbols", 1009, HashSymbol,
                InsertSymbolIntoHashTable, FindSymbolInHashTable);
  assembler->elf_machine_type = elf_machine_type;
  assembler->elf_flags = elf_flags;
  // 32-bit machines emit ELF32; everything else emits ELF64.  ARM (EABI) is
  // a little-endian 32-bit target.
  switch (elf_machine_type) {
    case ELF_MACHINE_TYPE_ARM:
      assembler->is_64_bit = false;
      break;
    default:
      assembler->is_64_bit = true;
      break;
  }
  assembler->is_little_endian = true;
  assembler->reloc_types = reloc_types;
  assembler->pic = false;
  assembler->absolute = false;

  DwarfInit(&assembler->dwarf);

  // Default label defining function.
  assembler->define_label = DefineLabel;
  return true;
}

bool AssemblerInit(Assembler* assembler, int16_t elf_machine_type,
                   uint16_t elf_flags, int* reloc_types, String* infile,
                   String* outfile) {
  if (!AssemblerInitCommon(assembler, elf_machine_type, elf_flags, reloc_types,
                           outfile)) {
    return false;
  }
  if (!LexInitFromFile(&assembler->lex, infile->value,
                       &assembler->preprocessor)) {
    MapDestruct(&assembler->directives);
    VectorDestruct(&assembler->sections);
    VectorDestruct(&assembler->relocations);
    VectorDestruct(&assembler->orphan_symbols);
    HashTableDestruct(&assembler->symbol_table);
    DwarfDestruct(&assembler->dwarf);
    PreprocessorDestruct(&assembler->preprocessor);
    StringDestruct(&assembler->filename);
    fclose(assembler->out);
    return false;
  }
  assembler->lex.assembler_mode = true;
  SyntaxInit(&assembler->syntax, &assembler->lex);
  SyntaxOpenScope(&assembler->syntax);
  return true;
}

bool AssemblerInitFromString(Assembler* assembler, int16_t elf_machine_type,
                             uint16_t elf_flags, int* reloc_types,
                             const char* name, String* input,
                             String* outfile) {
  if (!AssemblerInitCommon(assembler, elf_machine_type, elf_flags, reloc_types,
                           outfile)) {
    return false;
  }
  String* owned_input = malloc(sizeof(String));
  if (owned_input == NULL) {
    MapDestruct(&assembler->directives);
    VectorDestruct(&assembler->sections);
    VectorDestruct(&assembler->relocations);
    VectorDestruct(&assembler->orphan_symbols);
    HashTableDestruct(&assembler->symbol_table);
    DwarfDestruct(&assembler->dwarf);
    PreprocessorDestruct(&assembler->preprocessor);
    StringDestruct(&assembler->filename);
    fclose(assembler->out);
    return false;
  }
  StringInitFromSegment(owned_input, input->value, input->length);
  if (!LexInitFromString(&assembler->lex, name, owned_input,
                         &assembler->preprocessor)) {
    StringDestruct(owned_input);
    free(owned_input);
    MapDestruct(&assembler->directives);
    VectorDestruct(&assembler->sections);
    VectorDestruct(&assembler->relocations);
    VectorDestruct(&assembler->orphan_symbols);
    HashTableDestruct(&assembler->symbol_table);
    DwarfDestruct(&assembler->dwarf);
    PreprocessorDestruct(&assembler->preprocessor);
    StringDestruct(&assembler->filename);
    fclose(assembler->out);
    return false;
  }
  assembler->lex.assembler_mode = true;
  SyntaxInit(&assembler->syntax, &assembler->lex);
  SyntaxOpenScope(&assembler->syntax);
  return true;
}

void AssemblerDestruct(Assembler* assembler) {
  // Close the object file.
  fclose(assembler->out);

  SyntaxCloseScope(&assembler->syntax);
  SyntaxDestruct(&assembler->syntax);
  LexDestruct(&assembler->lex);

  StringDestruct(&assembler->filename);

  PreprocessorDestruct(&assembler->preprocessor);

  VectorDestructWithContents(&assembler->sections,
                             (VectorElementDestructor)AssemblerSectionDestruct, /*free_element=*/true);
  VectorDestructWithContents(
      &assembler->relocations,
      (VectorElementDestructor)AssemblerRelocationDestruct, /*free_element=*/true);

  ClearAssemblerSymbolTable(&assembler->symbol_table);
  HashTableDestruct(&assembler->symbol_table);

  // Free the symbols that were never inserted into the table.
  for (size_t i = 0; i < assembler->orphan_symbols.length; i++) {
    AssemblerSymbol* sym = assembler->orphan_symbols.value.p[i];
    AssemblerSymbolDelete(sym);
    free(sym);
  }
  VectorDestruct(&assembler->orphan_symbols);

  // Keys are string literals and values are function pointers, so only the
  // map's backing storage needs to be freed.
  MapDestruct(&assembler->directives);

  DwarfDestruct(&assembler->dwarf);
}

void AssemblerAddRelocation(Assembler* assembler, AssemblerRelocation* reloc) {
  // Only final-pass relocations describe the emitted section contents.
  if (assembler->pass != ASSEMBLER_FINAL_PASS) {
    // Delete it.
    AssemblerRelocationDelete(reloc);
    return;
  }
  // There is a relocation pointing to this symbol so it must be in the
  // symbol table.
  reloc->symbol->exported = true;
  VectorAppend(&assembler->relocations, reloc);
}

int AssemblerAddSection(Assembler* assembler, String* name, int32_t type,
                        int32_t flags, int32_t alignment) {
  if (name != NULL) {
    if (StringEqual(name, ".text")) {
      flags |= SHF(execinstr);
      type = SHT(progbits);
    } else if (StringEqual(name, ".data")) {
      flags |= SHF(write);
      type = SHT(progbits);
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
  VectorAppend(&assembler->sections, section);
  return (int)assembler->sections.length - 1;
}

int AssemblerFindSection(Assembler* assembler, String* name) {
  for (size_t i = 0; i < assembler->sections.length; i++) {
    AssemblerSection* section = assembler->sections.value.p[i];
    if (section->name != NULL && StringEqualString(section->name, name)) {
      return (int)i;
    }
  }
  return -1;
}

void AssemblerSetSectionSize(Assembler* assembler, size_t index, size_t size) {
  if (index < assembler->sections.length) {
    AssemblerSection* section = assembler->sections.value.p[index];
    section->contents.size = size;
  }
}

int64_t AssemblerCurrentAddress(Assembler* assembler) {
  AssemblerSection* section =
      assembler->sections.value.p[assembler->current_section];
  return section->address;
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

static void AddLocalSymbolFunc(BinaryTreeNode* node, int depth, void* data) {
  ELFWriterFile* elf = data;
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  if (sym->exported && sym->binding == SYM_BIND(local)) {
    int section_index = sym->section;
    AssemblerSymbolBinding binding = sym->binding;
    // An undefined local symbol is treated as global.
    if (!sym->defined) {
      section_index = 0;
      binding = SYM_BIND(global);
    }
    ELFWriterAddSymbol(elf, &sym->name, section_index,
                       SymbolTypeToELFType(sym->type),
                       SymbolBindingToELFBinding(binding), sym->size,
                       sym->value, &sym->index);
  }
}

// Add a local assembler symbol to the ELF file's symbol table.
static void AddLocalSymbolToELFFile(void* entry, void* data) {
  BinaryTreeTraverse(entry, AddLocalSymbolFunc, data);
}

static void AddGlobalSymbolFunc(BinaryTreeNode* node, int depth, void* data) {
  ELFWriterFile* elf = data;
  AssemblerSymbol* sym = (AssemblerSymbol*)node;
  if (sym->exported &&
      (sym->binding == SYM_BIND(global) || sym->binding == SYM_BIND(weak))) {
    ELFWriterAddSymbol(elf, &sym->name, sym->defined ? sym->section : 0,
                       SymbolTypeToELFType(sym->type),
                       SymbolBindingToELFBinding(sym->binding), sym->size,
                       sym->value, &sym->index);
  }
}

static void AddGlobalSymbolToELFFile(void* entry, void* data) {
  BinaryTreeTraverse(entry, AddGlobalSymbolFunc, data);
}

// Default label defining function.  Copies spelling into symbol defined.
static AssemblerSymbol* DefineLabel(Assembler* assembler, String* spelling) {
  AssemblerSymbol* sym = AssemblerFindSymbol(assembler, spelling->value);
  if (sym != NULL) {
    if (!sym->defined) {
      // Dot-prefixed labels are object-local.  A forward reference initially
      // creates an undefined global symbol so it can be relocated, but once
      // its definition is seen it must not remain globally visible.
      if (spelling->value[0] == '.') {
        sym->binding = SYM_BIND(local);
      }
      sym->defined = true;
      sym->is_label = true;
      sym->section = assembler->current_section;
      sym->value = AssemblerCurrentAddress(assembler);
    } else {
      AssemblerError(assembler, "Duplicate symbol %s", spelling->value);
    }
  } else {
    // Label is new, define it as a local.
    sym = NewAssemblerSymbol(spelling->value, assembler->current_section,
                             SYM_TYPE(none), SYM_BIND(local),
                             AssemblerCurrentAddress(assembler));
    sym->is_label = true;
    sym->defined = true;
    AssemblerInsertSymbol(assembler, sym);
  }
  return sym;
}

static void Assemble(Assembler* assembler,
                     void (*run_func)(Assembler*, String*)) {
  String word = {0};
  while (!LexEof(&assembler->lex)) {
    if (LexLookingAt(&assembler->lex, TOK(identifier))) {
      // Starts with identifier.  Could be a symbol definition
      // or instruction.  A symbol definition is followed by a colon.
      StringClear(&word);
      StringSetString(&word, &assembler->lex.spelling);
      LexNextToken(&assembler->lex);
      if (LexMatch(&assembler->lex, TOK(colon))) {
        // Defining a label.
        if (assembler->pass == 1) {
          assembler->define_label(assembler, &word);
        } else {
          // Variable-length directives can make pass-two offsets differ from
          // the provisional pass-one layout. Refresh labels as they are
          // encountered so following relocations use the emitted location.
          AssemblerSymbol* symbol =
              AssemblerFindSymbol(assembler, word.value);
          if (symbol != NULL) {
            symbol->section = assembler->current_section;
            symbol->value = AssemblerCurrentAddress(assembler);
          }
        }
      } else {
        // Try as a directive name.
        void* dir_func = MapFindPointerKey(&assembler->directives, word.value);
        if (dir_func != NULL) {
          void (*func)(Assembler*) = dir_func;
          func(assembler);
        } else {
          // Must be an instruction, pass on.
          run_func(assembler, &word);  // Run the assembly pass.
        }
      }
    }
    // In assembler mode the lexical analyzer doesn't read past the
    // end of line.  Read another line now.
    LexReadLine(&assembler->lex);
    LexNextToken(&assembler->lex);
  }
  StringDestruct(&word);
}

// Add all the sections to the ELF file.
static void AddSections(Assembler* assembler, ELFWriterFile* elf) {
  for (size_t i = 0; i < assembler->sections.length; i++) {
    AssemblerSection* section = assembler->sections.value.p[i];
    
    bool align = true;
    // We need to treat debug_line specially since its contents are generated
    // from .loc and .file directives.  This section will only be present if
    // -g is specifed on the compiler command line.
    bool is_debug_line_section = StringEqual(section->name, ".debug_line");
    
    // If the section is .debug_line build the data.
    if (is_debug_line_section) {
      // This section is not aligned.
      section->alignment = 1;
      align = false;
      ELFWriterSectionContentsInit(&section->contents,
                                   kSectionContentsBuffered);
      DwarfBuildDebugLineContents(&assembler->dwarf,
                                  &section->contents.data.buffered);
    }
    
    if (align) {
      // Align the section length to next 8 byte boundary.
      // TODO: do we align the beginning of the section?  I don't think we do,
      // but should.
      AssemblerSectionAlign(section, section->alignment);
    }
    
    // Add the section to the ELF file.
    int64_t entry_size = 0;
    if (section->type == SHT(init_array) ||
        section->type == SHT(fini_array) ||
        section->type == SHT(preinit_array)) {
      entry_size = assembler->elf_machine_type == ELF_MACHINE_TYPEW65C02
                       ? 2
                       : (assembler->is_64_bit ? 8 : 4);
    }
    ELFWriterSection* elf_section =
    ELFWriterAddSection(elf, section->name, section->type, section->flags,
                        section->alignment, &section->contents, entry_size);
    
    // For debug_line we need to add a relocation for the initial address.  We
    // can only do this when we know the section index.
    if (is_debug_line_section) {
      AssemblerSymbol* text = AssemblerFindSymbol(assembler, ".text");
      if (text != NULL) {
        AssemblerRelocation* addr_reloc = DwarfDebugLineRelocation(
                                &assembler->dwarf,
                                text,
                                assembler->reloc_types[kRelocSet64],
                                elf_section->index);
        AssemblerAddRelocation(assembler, addr_reloc);
      }
    }
    
    // Add a symbol for the section name as long as it's not an empty section.
    if (section->name != NULL && ELFWriterSectionContentsGetLength(&section->contents) != 0) {
      AssemblerSymbol* section_symbol =
      NewAssemblerSymbol(section->name->value, elf_section->index,
                         SYM_TYPE(none), SYM_BIND(local), 0);
      section_symbol->exported = true;
      section_symbol->defined = true;
      AssemblerInsertSymbol(assembler, section_symbol);
    }
    ELFWriterAddSectionSymbol(elf, &elf_section->name, elf_section->index);
  }
}

// Add all the relocations now that we have the sections and symbol
// indexes.
static void AddRelocations(Assembler* assembler, ELFWriterFile* elf) {
  for (size_t i = 0; i < assembler->relocations.length; i++) {
    AssemblerRelocation* reloc = assembler->relocations.value.p[i];
    ELFWriterAddRelocationWithAddend(elf, reloc->section, reloc->offset,
                           reloc->symbol->index, reloc->addend, reloc->type);
  }
}

void AssemblerReset(Assembler* assembler, bool clear_symbols) {
  // Reset all section addresses to zero.
  for (size_t i = 0; i < assembler->sections.length; i++) {
    AssemblerSection* section = assembler->sections.value.p[i];
    section->address = 0;
  }
  LexRewind(&assembler->lex);
  LexNextToken(&assembler->lex);
  PreprocessorReset(&assembler->preprocessor);
  if (clear_symbols) {
    ClearAssemblerSymbolTable(&assembler->symbol_table);
  }
}

void AssemblerRun(Assembler* assembler, void (*run_func)(Assembler*, String*)) {
  // Pass 1 discovers symbols, pass 2 converges variable-length directive
  // offsets, and pass 3 emits the final contents.
  assembler->pass = 1;
  LexNextToken(&assembler->lex);
  while (assembler->num_errors == 0 &&
         assembler->pass <= ASSEMBLER_FINAL_PASS) {
    Assemble(assembler, run_func);  // Run the assembly pass.
    assembler->pass++;
    if (assembler->pass <= ASSEMBLER_FINAL_PASS) {
      AssemblerReset(assembler, false);
    }
  }

  // Produce the ELF file.
  ELFWriterFile elf;
  ELFWriterFileInit(&elf, ET(rel), assembler->elf_machine_type,
                    assembler->elf_flags, NULL, assembler->is_64_bit,
                    assembler->is_little_endian);

  // Add the file symbol.
  if (assembler->filename.length != 0) {
    ELFWriterAddFileSymbol(&elf, &assembler->filename);
  }

  // Add all sections.
  AddSections(assembler, &elf);

  // Add all the symbols to the ELF file.
  // NOTE: ELF symbol table conventions dictate that the local
  // symbols must appear before global or undefined symbols.
  // Also, there must be a file symbol as entry 1 if there are
  // any local symbols.  The file and section symbol are
  // added just above this comment.

  // Add local symbols.
  HashTableTraverse(&assembler->symbol_table, AddLocalSymbolToELFFile, &elf);
  elf.last_local_symbol_index = (int32_t)elf.symbol_table.length;

  // Add global symbols.
  HashTableTraverse(&assembler->symbol_table, AddGlobalSymbolToELFFile, &elf);

  // Add all the relocations now that we have the sections and symbol
  // indexes.
  AddRelocations(assembler, &elf);

  // Write the ELF file to disk.
  ELFWriterFileWrite(&elf, assembler->out);
  
  // And we're done with the ELF writer.
  ELFWriterFileDestruct(&elf);
}

void AssemblerError(Assembler* assembler, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VLexError(&assembler->lex, format, ap);
  va_end(ap);
  assembler->num_errors++;
}

void AssemblerErrorAtLocation(Assembler* assembler, SourceLocation location, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  const char* filename;
  int lineno, s, e;
  DecodeSourceLocation(location, &filename, &lineno, &s, &e);
  VReportError(filename, lineno, format, ap);
  va_end(ap);
  assembler->num_errors++;
}

void AssemblerWarning(Assembler* assembler, const char* warn,
                      const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VLexWarning(&assembler->lex, warn, format, ap);
  va_end(ap);
}

//
// Directive implementation.
//

#define UNDEFINED_DIRECTIVE(spelling) \
  static void HandleDirective_##spelling(Assembler* assembler) {}

UNDEFINED_DIRECTIVE(align);

static void SymbolDirective(Assembler* assembler,
                            AssemblerSymbolBinding binding) {
  if (LexLookingAt(&assembler->lex, TOK(identifier))) {
    AssemblerSymbol* sym =
    AssemblerFindSymbol(assembler, assembler->lex.spelling.value);
    if (sym != NULL) {
      // A later reference can emit `.global name` after an inline definition
      // has already emitted `.weak name`.  ELF weak binding must survive that
      // redundant global declaration; otherwise two translation units that
      // use the same inline function become strong duplicate definitions.
      if (binding != SYM_BIND(global) ||
          sym->binding != SYM_BIND(weak)) {
        sym->binding = binding;
      }
      sym->exported = true;
    } else {
      // No symbol, add it as a global, but undefined.
      sym = NewAssemblerSymbol(assembler->lex.spelling.value,
                               assembler->current_section, SYM_TYPE(none),
                               binding, 0);
      AssemblerInsertSymbol(assembler, sym);
      sym->exported = true;
    }
    LexNextToken(&assembler->lex);
  } else {
    AssemblerError(assembler, "Symbol name expected");
  }
}

static void HandleDirective_globl(Assembler* assembler) {
  SymbolDirective(assembler, SYM_BIND(global));
}

static void HandleDirective_global(Assembler* assembler) {
  HandleDirective_globl(assembler);
}

static void HandleDirective_local(Assembler* assembler) {
  SymbolDirective(assembler, SYM_BIND(local));
}

static void HandleDirective_weak(Assembler* assembler) {
  SymbolDirective(assembler, SYM_BIND(weak));
}

static void HandleDirective_comm(Assembler* assembler) {
  if (LexLookingAt(&assembler->lex, TOK(identifier))) {
    AssemblerSymbol* sym =
        AssemblerFindSymbol(assembler, assembler->lex.spelling.value);
    if (sym != NULL) {
      if (assembler->pass == 1 && sym->defined && sym->section != SHN_COM) {
        AssemblerError(assembler, "Duplicate common symbol %s", sym->name.value);
        return;
      }
      sym->exported = true;
      sym->defined = true;
      sym->section = SHN_COM;
      sym->value = 0;
      sym->binding = SYM_BIND(global);
    } else {
      // No symbol, add it as a global in the COM section..
      sym = NewAssemblerSymbol(assembler->lex.spelling.value, SHN_COM,
                               SYM_TYPE(object), SYM_BIND(global), 0);
      sym->defined = true;
      AssemblerInsertSymbol(assembler, sym);
      sym->exported = true;
    }
    LexNextToken(&assembler->lex);
    if (!LexMatch(&assembler->lex, TOK(comma))) {
      AssemblerError(assembler, "Missing size in .comm directive");
      return;
    }
    if (LexLookingAt(&assembler->lex, TOK(number))) {
      int32_t new_size = (int32_t)assembler->lex.number;
      if (new_size > sym->size) {
        sym->size = new_size;
      }
      LexNextToken(&assembler->lex);
    }
    if (LexMatch(&assembler->lex, TOK(comma))) {
      if (LexLookingAt(&assembler->lex, TOK(number))) {
        int32_t new_align = (int32_t)assembler->lex.number;
        if (new_align > sym->alignment) {
          sym->alignment = new_align;
        }
        LexNextToken(&assembler->lex);
      }
    }
  } else {
    AssemblerError(assembler, "Symbol name expected");
  }
}

static void HandleDirective_type(Assembler* assembler) {
  if (LexLookingAt(&assembler->lex, TOK(identifier))) {
    String name;
    StringInit(&name, assembler->lex.spelling.value);
    LexNextToken(&assembler->lex);
    if (LexMatch(&assembler->lex, TOK(comma))) {
      AssemblerSymbol* sym = AssemblerFindSymbol(assembler, name.value);
      if (sym == NULL) {
        // No symbol, add it as a local, but undefined.
        sym = NewAssemblerSymbol(name.value, assembler->current_section,
                                 SYM_TYPE(none), SYM_BIND(local), 0);
        AssemblerInsertSymbol(assembler, sym);
      }

      // If the symbol has a .type then it is exported to the object file.
      sym->exported = true;
      if (LexLookingAt(&assembler->lex, TOK(identifier))) {
        AssemblerSection* sect = sym->section == SHN_COM ? NULL :
          assembler->sections.value.p[sym->section];
        String type;
        StringInit(&type, assembler->lex.spelling.value);
        if (StringEqual(&type, "function") || StringEqual(&type, "@function")) {
          sym->type = SYM_TYPE(func);
        } else if (StringEqual(&type, "object") ||
                   StringEqual(&type, "@object")) {
          // An object is TLS if the section it's in has the TLS flag.
          if (sect != NULL && (sect->flags & SHF(tls)) != 0) {
            sym->type = SYM_TYPE(tls);
          } else {
            sym->type = SYM_TYPE(object);
          }
        } else {
          AssemblerError(assembler, "Invalid .type syntax; unsupported type %s",
                         type.value);
        }
        StringDestruct(&type);
        LexNextToken(&assembler->lex);
      } else {
        AssemblerError(assembler, "Invalid .type syntax; missing type");
      }
    } else {
      AssemblerError(assembler, "Invalid .type syntax; no comma");
    }
    StringDestruct(&name);
  } else {
    AssemblerError(assembler, "Symbol name expected");
  }
}

static void HandleDirective_size(Assembler* assembler) {
  if (LexLookingAt(&assembler->lex, TOK(identifier))) {
    String name;
    StringInit(&name, assembler->lex.spelling.value);
    LexNextToken(&assembler->lex);
    if (LexMatch(&assembler->lex, TOK(comma))) {
      AssemblerSymbol* sym = AssemblerFindSymbol(assembler, name.value);
      if (sym == NULL) {
        AssemblerError(assembler, "Undefined symbol %s in .size", name.value);
      } else {
        sym->size = (int32_t)AssemblerEvaluateExpression(assembler);
        sym->exported = true;
      }
    } else {
      AssemblerError(assembler, "Invalid .size syntax; no comma");
    }
    StringDestruct(&name);
  } else {
    AssemblerError(assembler, "Symbol name expected");
  }
}

static AssemblerSymbol* ExpressionPrimary(Assembler* assembler) {
  if (LexLookingAt(&assembler->lex, TOK(identifier))) {
    String spelling = assembler->lex.spelling;
    AssemblerSymbol* sym = AssemblerFindSymbol(assembler, spelling.value);
    LexNextToken(&assembler->lex);
    if (sym == NULL) {
      sym = NewAssemblerSymbol(spelling.value, 0, SYM_TYPE(none),
                               SYM_BIND(global), 0);
      if (strcmp(spelling.value, ".") == 0) {
        sym->is_label = true;
        sym->defined = true;
        sym->section = assembler->current_section;
        sym->value = AssemblerCurrentAddress(assembler);
      } else {
        sym->exported = true;
        sym->defined = false;
      }
      AssemblerInsertSymbol(assembler, sym);
    }
    return sym;
  }
  return NULL;
}

static int EhTableWordRelocType(Assembler* assembler) {
  switch (assembler->elf_machine_type) {
    case ELF_MACHINE_TYPE_X86_64:
      return R_X86_64_PC32;
    case ELF_MACHINE_TYPE_AARCH64:
      return R_AARCH64_PREL32;
    default:
      return assembler->reloc_types[kRelocSet32];
  }
}

static bool IsEhTableSection(const Assembler* assembler) {
  if (assembler->current_section < 0 ||
      (size_t)assembler->current_section >= assembler->sections.length) {
    return false;
  }
  AssemblerSection* section =
      assembler->sections.value.p[assembler->current_section];
  if (section->name == NULL) {
    return false;
  }
  if (assembler->elf_machine_type == ELF_MACHINE_TYPE_ARM &&
      (strcmp(section->name->value, ".ARM.exidx") == 0 ||
       strcmp(section->name->value, ".ARM.extab") == 0)) {
    return true;
  }
  return strcmp(section->name->value, ".gcc_except_table") == 0 ||
         strcmp(section->name->value, ".eh_frame") == 0;
}

static int RelocTypeForWord(Assembler* assembler) {
  if (IsEhTableSection(assembler)) {
    if (assembler->elf_machine_type == ELF_MACHINE_TYPE_ARM) {
      AssemblerSection* section =
          assembler->sections.value.p[assembler->current_section];
      if (strcmp(section->name->value, ".ARM.exidx") == 0 ||
          strcmp(section->name->value, ".ARM.extab") == 0) {
        return R_ARM_PREL31;
      }
      return R_ARM_REL32;
    }
    return EhTableWordRelocType(assembler);
  }
  return assembler->reloc_types[kRelocSet32];
}

// We only support simple expressions involving assembler symbols.  These can
// only be symbols separated by + or -.  They generate relocations for the
// symbols.
// If all symbols are labels we can do the calculations here since their
// values are known relative to their section at assembly time.
static int64_t SimpleSymbolExpression(Assembler* assembler, int bits) {
  AssemblerSymbol* left = ExpressionPrimary(assembler);
  if (left == NULL) {
    return 0;
  }
  int reloc_index = kRelocSet32;
  (void)reloc_index;
  switch (bits) {
    case 16:
      reloc_index = kRelocSet16;
      break;
    case 32:
      reloc_index = kRelocSet32;
      break;
    case 64:
      reloc_index = kRelocSet64;
      break;
    default:
      abort();
  }
  Vector relocations = {0};

  // TODO: an optimization here would be to check for an internal label
  // and use its section symbol plus an addend instead of exporting the
  // label.  Maybe later.  The problem is that the section symbols aren't
  // created until we add the sections to the ELF file.
  int initial_reloc_type =
      bits == 32 ? RelocTypeForWord(assembler)
                 : assembler->reloc_types[reloc_index];
  AssemblerRelocation* reloc = NewAssemblerRelocation(
      left, initial_reloc_type, assembler->current_section,
      (int32_t)AssemblerCurrentAddress(assembler), 0);
  
  VectorAppend(&relocations, reloc);
  bool known_values = left->is_label && left->type == SYM_TYPE(none) &&
      assembler->current_section == left->section && !assembler->absolute;
  // The section base address cancels out of an expression only when its
  // additive and subtractive same-section terms balance (e.g. a `label2 -
  // label1` difference).  Such a value is position-independent and can be
  // resolved here with no relocation.  A lone `label` (or otherwise unbalanced
  // expression) is section-base-relative, so it MUST keep its relocation for
  // the linker to patch in the final address; folding it to the assembly-time
  // section offset would leave a bogus small value at run time.
  int additive_terms = 1;
  int subtractive_terms = 0;

  while (LexLookingAt(&assembler->lex, TOK(plus)) ||
         LexLookingAt(&assembler->lex, TOK(minus))) {
    Token tok = assembler->lex.current_token;
    LexNextToken(&assembler->lex);
    AssemblerSymbol* right = ExpressionPrimary(assembler);
    if (right == NULL) {
      break;
    }
    int reloc_type = 0;
    if (bits == 64) {
      reloc_type = tok == TOK(plus) ? kRelocAdd64 : kRelocSub64;
    } else if (bits == 32) {
      reloc_type = tok == TOK(plus) ? kRelocAdd32 : kRelocSub32;
    } else if (bits == 16) {
      reloc_type = tok == TOK(plus) ? kRelocAdd16 : kRelocSub16;
    } else {
      assert(false);
    }
    AssemblerRelocation* reloc = NewAssemblerRelocation(
                right, assembler->reloc_types[reloc_type], assembler->current_section,
                            (int32_t)AssemblerCurrentAddress(assembler), 0);
    known_values &= right->is_label && right->type == SYM_TYPE(none) &&
        assembler->current_section == right->section;
    if (tok == TOK(plus)) {
      additive_terms++;
    } else {
      subtractive_terms++;
    }
    VectorAppend(&relocations, reloc);
  }
  int64_t value = 0;
  if (known_values && additive_terms == subtractive_terms) {
    value = left->value;
    for (size_t i = 1; i < relocations.length; i++) {
      AssemblerRelocation* reloc = relocations.value.p[i];
      if (reloc->type == assembler->reloc_types[kRelocAdd64] ||
          reloc->type == assembler->reloc_types[kRelocAdd32] ||
          reloc->type == assembler->reloc_types[kRelocAdd16]) {
        value += reloc->symbol->value;
      } else {
        value -= reloc->symbol->value;
      }
    }
    VectorDestructWithContents(&relocations,
                               (VectorElementDestructor)AssemblerRelocationDestruct, /*free_element=*/true);
  } else if (relocations.length == 2 && additive_terms == 1 &&
             subtractive_terms == 1) {
    AssemblerRelocation* add_reloc = relocations.value.p[0];
    AssemblerRelocation* sub_reloc = relocations.value.p[1];
    bool sub_is_dot = sub_reloc->symbol->name.value[0] == '.' &&
                      sub_reloc->symbol->name.value[1] == '\0';
    bool direct_pcrel32 =
        assembler->elf_machine_type == ELF_MACHINE_TYPE_X86_64 ||
        assembler->elf_machine_type == ELF_MACHINE_TYPE_AARCH64 ||
        assembler->elf_machine_type == ELF_MACHINE_TYPE_ARM;
    if (direct_pcrel32 &&
        (sub_is_dot ||
         (sub_reloc->symbol->is_label && sub_reloc->symbol->defined &&
          sub_reloc->symbol->section == assembler->current_section &&
          sub_reloc->type == assembler->reloc_types[kRelocSub32] &&
          sub_reloc->symbol->value ==
              (uint64_t)AssemblerCurrentAddress(assembler)))) {
      AssemblerRelocation* merged = NewAssemblerRelocation(
          add_reloc->symbol, RelocTypeForWord(assembler),
          assembler->current_section,
          (int32_t)AssemblerCurrentAddress(assembler), add_reloc->addend);
      VectorDestructWithContents(
          &relocations, (VectorElementDestructor)AssemblerRelocationDestruct,
          /*free_element=*/true);
      AssemblerAddRelocation(assembler, merged);
    } else {
      for (size_t i = 0; i < relocations.length; i++) {
        AssemblerAddRelocation(assembler, relocations.value.p[i]);
      }
    }
    VectorDestruct(&relocations);
  } else {
    for (size_t i = 0; i < relocations.length; i++) {
      AssemblerAddRelocation(assembler, relocations.value.p[i]);
    }
    VectorDestruct(&relocations);
  }
  return value;
}

static void HandleDirective_p2align(Assembler* assembler) {
  if (LexLookingAt(&assembler->lex, TOK(number))) {
    int num_bits = (int)assembler->lex.number;
    int alignment = (1 << num_bits) - 1;
    LexNextToken(&assembler->lex);

    // TODO: implement second and third args to .p2align?

    AssemblerSection* sect =
        assembler->sections.value.p[assembler->current_section];
    size_t next_address = (sect->address + alignment) & ~alignment;
    size_t num_bytes = next_address - sect->address;
    if (assembler->pass == ASSEMBLER_FINAL_PASS) {
      BufferAddSpace(&sect->contents.data.buffered, num_bytes);
      sect->contents.size += num_bytes;
    }
    sect->address += num_bytes;
  } else {
    AssemblerError(assembler, "Alignment expected");
  }
}

static void HandleDataDirective(Assembler* assembler, int bits) {
  while (!LexEof(&assembler->lex)) {
    int64_t value = AssemblerEvaluateExpression(assembler);
    switch (bits) {
      case 8:
        AssemblerEmitByte(assembler, assembler->current_section,
                          (uint8_t)value);
        break;
      case 16:
        AssemblerEmitHalf(assembler, assembler->current_section,
                          (uint16_t)value);
        break;
      case 32:
        AssemblerEmitWord(assembler, assembler->current_section,
                          (uint32_t)value);
        break;
      case 64:
        // This is not handled here.
        assert(false);
        break;
      default:
        assert(false);
    }
    if (!LexMatch(&assembler->lex, TOK(comma))) {
      break;
    }
  }
}

static void HandleDirective_space(Assembler* assembler) {
  int64_t num_bytes = AssemblerEvaluateExpression(assembler);
  if (num_bytes < 0) {
    AssemblerError(assembler, "Invalid .space size %" PRId64 "", num_bytes);
    return;
  }
  int value = 0;
  if (LexMatch(&assembler->lex, TOK(comma))) {
    value = (int)AssemblerEvaluateExpression(assembler);
  }
  AssemblerSection* sect =
      assembler->sections.value.p[assembler->current_section];
  if (assembler->pass == ASSEMBLER_FINAL_PASS) {
    BufferFill(&sect->contents.data.buffered, num_bytes, value);
    sect->contents.size += num_bytes;
  }
  sect->address += num_bytes;
}

// A string output with optional \0 after the characters.
static void StringDirective(Assembler* assembler, bool append_zero) {
  for (;;) {
    if (LexLookingAt(&assembler->lex, TOK(string))) {
      String str;
      StringInit(&str, assembler->lex.spelling.value);
      for (size_t i = 0; i < str.length + append_zero; i++) {
        AssemblerEmitByte(assembler, assembler->current_section, str.value[i]);
      }
      StringDestruct(&str);
      LexNextToken(&assembler->lex);
    } else {
      AssemblerError(assembler, "Missing string for string directive");
    }
    if (!LexMatch(&assembler->lex, TOK(comma))) {
      break;
    }
  }
}

static void HandleDirective_asciz(Assembler* assembler) {
  StringDirective(assembler, true);
}

static void AssemblerEmitUleb128(Assembler* assembler, uint64_t value) {
  do {
    uint8_t byte = (uint8_t)(value & 0x7f);
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }
    AssemblerEmitByte(assembler, assembler->current_section, byte);
  } while (value != 0);
}

static void AssemblerEmitSleb128(Assembler* assembler, int64_t value) {
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
    AssemblerEmitByte(assembler, assembler->current_section, byte);
  }
}

static void HandleDirective_uleb128(Assembler* assembler) {
  int64_t value = AssemblerEvaluateExpression(assembler);
  if (value < 0) {
    AssemblerError(assembler, "Invalid .uleb128 value %" PRId64 "", value);
    return;
  }
  AssemblerEmitUleb128(assembler, (uint64_t)value);
}

static void HandleDirective_sleb128(Assembler* assembler) {
  int64_t value = AssemblerEvaluateExpression(assembler);
  AssemblerEmitSleb128(assembler, value);
}

static void HandleDirective_string(Assembler* assembler) {
  StringDirective(assembler, true);
}

static void HandleDirective_ascii(Assembler* assembler) {
  StringDirective(assembler, false);
}

static void HandleDirective_byte(Assembler* assembler) {
  HandleDataDirective(assembler, 8);
}

static void HandleDirective_hword(Assembler* assembler) {
  while (!LexEof(&assembler->lex)) {
    if (LexLookingAt(&assembler->lex, TOK(identifier))) {
      int16_t value = (int16_t)SimpleSymbolExpression(assembler, 16);
      AssemblerEmitHalf(assembler, assembler->current_section, value);
    } else {
      int16_t value = (int16_t)AssemblerEvaluateExpression(assembler);
      AssemblerEmitHalf(assembler, assembler->current_section, value);
    }
    if (!LexMatch(&assembler->lex, TOK(comma))) {
      break;
    }
  }}

static void HandleDirective_short(Assembler* assembler) {
  HandleDirective_hword(assembler);
}


// Words can refer to a symbol.  If this is case the lower 32 bits of the
// symbol will be used.  If a relocation is needed, the reloc_types[kRelocSet32]
// will be emitted.
static void HandleDirective_word(Assembler* assembler) {
  while (!LexEof(&assembler->lex)) {
    if (LexLookingAt(&assembler->lex, TOK(identifier))) {
      size_t reloc_count = assembler->relocations.length;
      int32_t value = (int32_t)SimpleSymbolExpression(assembler, 32);
      AssemblerEmitWord(assembler, assembler->current_section, value);
      int32_t field_offset =
          (int32_t)AssemblerCurrentAddress(assembler) - (int32_t)sizeof(int32_t);
      for (size_t i = reloc_count; i < assembler->relocations.length; i++) {
        AssemblerRelocation* reloc =
            (AssemblerRelocation*)assembler->relocations.value.p[i];
        reloc->offset = field_offset;
      }
    } else {
      int32_t value = (int32_t)AssemblerEvaluateExpression(assembler);
      AssemblerEmitWord(assembler, assembler->current_section, value);
    }
    if (!LexMatch(&assembler->lex, TOK(comma))) {
      break;
    }
  }
}

static void HandleDirective_2byte(Assembler* assembler) {
  HandleDirective_short(assembler);
}

static void HandleDirective_4byte(Assembler* assembler) {
  HandleDirective_word(assembler);
}

static void HandleDirective_8byte(Assembler* assembler) {
  HandleDirective_long(assembler);
}

static void HandleDirective_long(Assembler* assembler) {
  while (!LexEof(&assembler->lex)) {
    if (LexLookingAt(&assembler->lex, TOK(identifier))) {
      int64_t value = SimpleSymbolExpression(assembler, 64);
      AssemblerEmitLong(assembler, assembler->current_section, value);
    } else {
      int64_t value = AssemblerEvaluateExpression(assembler);
      AssemblerEmitLong(assembler, assembler->current_section, value);
    }
    if (!LexMatch(&assembler->lex, TOK(comma))) {
      break;
    }
  }
}

static int DefaultSectionAlignment(const Assembler* assembler) {
  // The 6502 can fetch instructions and data at any byte address.  Requiring
  // eight-byte input-section alignment only inserts unreachable zero padding
  // between linked objects.
  return assembler->elf_machine_type == ELF_MACHINE_TYPEW65C02 ? 1 : 8;
}

static void HandleDirective_section(Assembler* assembler) {
  if (LexLookingAt(&assembler->lex, TOK(identifier)) ||
      LexLookingAt(&assembler->lex, TOK(string))) {
    String* name = NewString(assembler->lex.spelling.value);
    LexNextToken(&assembler->lex);

    int32_t flags = 0;
    int32_t type = SHT(null);
    int alignment = DefaultSectionAlignment(assembler);
    if (LexMatch(&assembler->lex, TOK(comma))) {
      if (LexLookingAt(&assembler->lex, TOK(identifier)) ||
          LexLookingAt(&assembler->lex, TOK(string))) {
        for (size_t i = 0; assembler->lex.spelling.value[i] != '\0'; i++) {
          switch (assembler->lex.spelling.value[i]) {
            case 'a':
              flags |= SHF(alloc);
              break;
            case 'w':
              flags |= SHF(write);
              break;
            case 'x':
              flags |= SHF(execinstr);
              break;
            case 'M':
              flags |= SHF(merge);
              break;
            case 'S':
              flags |= SHF(strings);
              break;
            case 'T':
              flags |= SHF(tls);
              break;
            case 'L':
              flags |= SHF(link_order);
              break;
          }
        }
        LexNextToken(&assembler->lex);
        if (LexMatch(&assembler->lex, TOK(comma))) {
          // Type argument
          if (LexLookingAt(&assembler->lex, TOK(identifier))) {
            if (strcmp(assembler->lex.spelling.value, "@progbits") == 0) {
              type = SHT(progbits);
            } else if (strcmp(assembler->lex.spelling.value, "@nobits") == 0) {
              type = SHT(nobits);
            } else if (strcmp(assembler->lex.spelling.value, "@init_array") ==
                       0) {
              type = SHT(init_array);
            } else if (strcmp(assembler->lex.spelling.value, "@fini_array") ==
                       0) {
              type = SHT(fini_array);
            } else if (strcmp(assembler->lex.spelling.value,
                              "@preinit_array") == 0) {
              type = SHT(preinit_array);
            } else if (strcmp(assembler->lex.spelling.value, "@unwind") == 0) {
              type = SHT(ARM_EXIDX);
            }
            LexNextToken(&assembler->lex);
          }
        }
      }
    }
    // Allow alignment (extension, not present in GNU as).
    if (LexMatch(&assembler->lex, TOK(comma))) {
      alignment = (int)AssemblerEvaluateExpression(assembler);
    }
    int section;
    if (assembler->pass == 1) {
      section = AssemblerFindSection(assembler, name);
      if (section == -1) {
        section = AssemblerAddSection(assembler, name, type, flags, alignment);
      } else {
        StringDelete(name);
      }
    } else {
      section = AssemblerFindSection(assembler, name);
      StringDelete(name);
    }
    if (section == -1) {
      AssemblerError(assembler, "Bad .section directive");
    } else {
      assembler->current_section = section;
    }
  } else {
    AssemblerError(assembler, "Missing section name");
  }
}

static void HandleDirective_text(Assembler* assembler) {
  String* name = NewString(".text");
  int section;
  if (assembler->pass == 1) {
    section = AssemblerFindSection(assembler, name);
    if (section == -1) {
      section = AssemblerAddSection(assembler, name, SHT(progbits),
                                    SHF(alloc) | SHF(execinstr),
                                    DefaultSectionAlignment(assembler));
    } else {
      StringDelete(name);
    }
  } else {
    section = AssemblerFindSection(assembler, name);
    StringDelete(name);
  }
  assert(section != -1);
  assembler->current_section = section;
}

static void HandleDirective_data(Assembler* assembler) {
  String* name = NewString(".data");
  int section;
  if (assembler->pass == 1) {
    section = AssemblerFindSection(assembler, name);
    if (section == -1) {
      section = AssemblerAddSection(
          assembler, name, SHT(progbits), SHF(write) | SHF(alloc),
          DefaultSectionAlignment(assembler));
    } else {
      StringDelete(name);
    }
  } else {
    section = AssemblerFindSection(assembler, name);
    StringDelete(name);
  }
  assert(section != -1);
  assembler->current_section = section;
}

static void HandleDirective_file(Assembler* assembler) {
  int index = -1;
  if (LexLookingAt(&assembler->lex, TOK(number))) {
    // The index is optional.  In DWARF output format it will contain the
    // index into the file table in the debug_line section.
    index = (int)assembler->lex.number;
    LexNextToken(&assembler->lex);
  }

  String filename = {0};
  if (LexLookingAt(&assembler->lex, TOK(string))) {
    StringSet(&filename, assembler->lex.spelling.value);
    if (index == -1) {
      StringSet(&assembler->filename, assembler->lex.spelling.value);
    }
    LexNextToken(&assembler->lex);
  } else {
    AssemblerError(assembler, "Missing filename string in .file directive");
  }
  if (assembler->pass == 1) {
    DwarfAddFile(&assembler->dwarf, &filename);
  }
  StringDestruct(&filename);
}

static void HandleDirective_loc(Assembler* assembler) {
  int file = 0;
  int line = 0;
  int col = 0;

  if (LexLookingAt(&assembler->lex, TOK(number))) {
    file = (int)assembler->lex.number;
    LexNextToken(&assembler->lex);
  }

  if (LexLookingAt(&assembler->lex, TOK(number))) {
    line = (int)assembler->lex.number;
    LexNextToken(&assembler->lex);
  }

  if (LexLookingAt(&assembler->lex, TOK(number))) {
    col = (int)assembler->lex.number;
    LexNextToken(&assembler->lex);
  }

  if (assembler->pass == 1) {
    DwarfAddLocation(&assembler->dwarf, file, line, col,
                     AssemblerCurrentAddress(assembler));
  }
}

static void HandleDirective_option(Assembler* assembler) {
  while (LexLookingAt(&assembler->lex, TOK(identifier))) {
    if (StringEqual(&assembler->lex.spelling, "pic")) {
      assembler->pic = true;
    }
    LexNextToken(&assembler->lex);
    if (!LexMatch(&assembler->lex, TOK(comma))) {
      break;
    }
  }
}

static void HandleDirective_set(Assembler* assembler) {
  if (LexLookingAt(&assembler->lex, TOK(identifier))) {
    String name;
    StringInit(&name, assembler->lex.spelling.value);
    LexNextToken(&assembler->lex);
    int64_t value = AssemblerEvaluateExpression(assembler);
    
    AssemblerSymbol* sym = AssemblerFindSymbol(assembler, name.value);
    if (assembler->pass == 1) {
      // Only define symbols in pass 1.
      if (sym != NULL) {
        if (!sym->defined) {
          sym->defined = true;
          sym->is_constant = true;
          sym->section = assembler->current_section;
          sym->value = AssemblerCurrentAddress(assembler);
        } else {
          AssemblerError(assembler, "Duplicate symbol %s", name.value);
        }
      } else {
        // Symbol is new, define it as a local constant.
        sym = NewAssemblerSymbol(name.value, assembler->current_section,
                                 SYM_TYPE(none), SYM_BIND(local),
                                 value);
        sym->defined = true;
        sym->is_constant = true;
        AssemblerInsertSymbol(assembler, sym);
      }
    }
  }
}

static void HandleDirective_chkaddr(Assembler* assembler) {
  if (LexLookingAt(&assembler->lex, TOK(number))) {
    int addr = (int)assembler->lex.number;
    LexNextToken(&assembler->lex);
    int curr = (int)AssemblerCurrentAddress(assembler);
    if (addr != curr) {
      AssemblerError(assembler, "Address mismatch: expected 0x%x(%d); got 0x%x(%d)", addr, addr, curr, curr);
    }
  }
}

