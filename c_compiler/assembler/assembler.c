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
#include <stdint.h>
#include <inttypes.h>
#include "elf_writer.h"
#include "asm_expr.h"
#include "errors.h"

#define ASM_OBJECT_FINAL_PASS 3

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

int64_t AssemblerEvaluateExpression(Assembler* assembler) {
  return AssemblerEvaluateKnownExpression(assembler, NULL);
}

int64_t AssemblerEvaluateKnownExpression(Assembler* assembler, bool* known) {
  int64_t value = 0;
  bool ok = AssemblerEvaluateExpressionInternal(assembler, &value);
  if (known != NULL) {
    *known = ok;
  }
  if (!ok && assembler->object.pass == ASM_OBJECT_FINAL_PASS) {
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
  } else if (LexLookingAt(&assembler->lex, TOK(identifier))) {
    const char* name = assembler->lex.spelling.value;
    union {
      uint64_t bits;
      double value;
    } converted;
    if (strcmp(name, "inf") == 0 || strcmp(name, "infinity") == 0) {
      converted.bits = 0x7ff0000000000000ULL;
      v = converted.value;
      LexNextToken(&assembler->lex);
    } else if (strcmp(name, "nan") == 0) {
      converted.bits = 0x7ff8000000000000ULL;
      v = converted.value;
      LexNextToken(&assembler->lex);
    } else {
      AssemblerError(assembler, "Floating point constant expected");
    }
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
  assembler->out = fopen(outfile->value, "w");
  if (assembler->out == NULL) {
    fprintf(stderr, "Cannot open object file %s\n", outfile->value);
    return false;
  }

  PreprocessorInit(&assembler->preprocessor);
  AsmObjectInit(&assembler->object, elf_machine_type, elf_flags, reloc_types);
  MapInit(&assembler->directives, CompareCharPointer);
  InitializeDirectives(&assembler->directives);
  assembler->num_errors = 0;
  assembler->parsing_layout_expression = false;
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
    AsmObjectDestruct(&assembler->object);
    PreprocessorDestruct(&assembler->preprocessor);
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
    AsmObjectDestruct(&assembler->object);
    PreprocessorDestruct(&assembler->preprocessor);
    fclose(assembler->out);
    return false;
  }
  StringInitFromSegment(owned_input, input->value, input->length);
  if (!LexInitFromString(&assembler->lex, name, owned_input,
                         &assembler->preprocessor)) {
    StringDestruct(owned_input);
    free(owned_input);
    MapDestruct(&assembler->directives);
    AsmObjectDestruct(&assembler->object);
    PreprocessorDestruct(&assembler->preprocessor);
    fclose(assembler->out);
    return false;
  }
  assembler->lex.assembler_mode = true;
  SyntaxInit(&assembler->syntax, &assembler->lex);
  SyntaxOpenScope(&assembler->syntax);
  return true;
}

void AssemblerDestruct(Assembler* assembler) {
  fclose(assembler->out);
  SyntaxCloseScope(&assembler->syntax);
  SyntaxDestruct(&assembler->syntax);
  LexDestruct(&assembler->lex);
  PreprocessorDestruct(&assembler->preprocessor);
  MapDestruct(&assembler->directives);
  AsmObjectDestruct(&assembler->object);
}

AssemblerSymbol* AssemblerFindSymbol(Assembler* assembler, const char* name) {
  return AsmObjectFindSymbol(&assembler->object, name);
}

void AssemblerTrackOrphanSymbol(Assembler* assembler, AssemblerSymbol* sym) {
  AsmObjectTrackOrphanSymbol(&assembler->object, sym);
}

void AssemblerInsertSymbol(Assembler* assembler, AssemblerSymbol* sym) {
  AsmObjectInsertSymbol(&assembler->object, sym);
}

void AssemblerClearSymbols(Assembler* assembler) {
  AsmObjectClearSymbols(&assembler->object);
}

void AssemblerEmitWord(Assembler* assembler, int section, int32_t word) {
  AsmObjectEmitWord(&assembler->object, section, word);
}

void AssemblerEmitByte(Assembler* assembler, int section, uint8_t byte) {
  AsmObjectEmitByte(&assembler->object, section, byte);
}

void AssemblerEmitHalf(Assembler* assembler, int section, uint16_t half) {
  AsmObjectEmitHalf(&assembler->object, section, half);
}

void AssemblerEmitLong(Assembler* assembler, int section, uint64_t l) {
  AsmObjectEmitLong(&assembler->object, section, l);
}

void AssemblerAddRelocation(Assembler* assembler, AssemblerRelocation* reloc) {
  AsmObjectAddRelocation(&assembler->object, reloc);
}

void AssemblerAddRelocationForSymbol(Assembler* assembler,
                                     AssemblerSymbol* symbol, int32_t type,
                                     int32_t section, int32_t offset,
                                     int32_t addend) {
  AsmObjectAddRelocationForSymbol(&assembler->object, symbol, type, section,
                                  offset, addend);
}

int AssemblerAddSection(Assembler* assembler, String* name, int32_t type,
                        int32_t flags, int32_t alignment) {
  return AsmObjectAddSection(&assembler->object, name, type, flags, alignment);
}

int AssemblerFindSection(Assembler* assembler, String* name) {
  return AsmObjectFindSection(&assembler->object, name);
}

void AssemblerSetSectionSize(Assembler* assembler, size_t index, size_t size) {
  AsmObjectSetSectionSize(&assembler->object, index, size);
}

int64_t AssemblerCurrentAddress(Assembler* assembler) {
  return AsmObjectCurrentAddress(&assembler->object);
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

static AssemblerSymbol* DefineLabel(Assembler* assembler, String* spelling) {
  return AsmObjectDefineLabel(&assembler->object, assembler, spelling);
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
        if (assembler->object.pass == 1) {
          assembler->define_label(assembler, &word);
        } else {
          // Variable-length directives can make pass-two offsets differ from
          // the provisional pass-one layout. Refresh labels as they are
          // encountered so following relocations use the emitted location.
          AssemblerSymbol* symbol =
              AssemblerFindSymbol(assembler, word.value);
          if (symbol != NULL) {
            symbol->section = assembler->object.current_section;
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

void AssemblerReset(Assembler* assembler, bool clear_symbols) {
  AsmObjectReset(&assembler->object, clear_symbols);
  LexRewind(&assembler->lex);
  LexNextToken(&assembler->lex);
  PreprocessorReset(&assembler->preprocessor);
}

static void AssemblerAdvancePass(Assembler* assembler) {
  if (assembler->object.pass == 1 && assembler->object.allow_layout_pass_skip &&
      !assembler->object.requires_layout_pass) {
    assembler->object.pass = ASM_OBJECT_FINAL_PASS;
  } else {
    assembler->object.pass++;
  }
}

void AssemblerAssembleInput(Assembler* assembler, const char* name, String* input,
                            void (*run_func)(Assembler*, String*)) {
  if (input == NULL || input->length == 0) {
    return;
  }

  String owned;
  StringInitFromSegment(&owned, input->value, input->length);
  LexCheckpoint checkpoint;
  LexCheckpointSave(&assembler->lex, &checkpoint);
  LexInitFromString(&assembler->lex, name, &owned, &assembler->preprocessor);
  assembler->lex.assembler_mode = true;
  assembler->lex.suppress_preprocessing = true;
  LexNextToken(&assembler->lex);
  Assemble(assembler, run_func);
  LexCheckpointRestore(&assembler->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  StringDestruct(&owned);
}

void AssemblerRunRecordedOperations(Assembler* assembler, Vector* inputs,
                                    void (*run_func)(Assembler*, String*)) {
  assembler->object.pass = 1;
  while (assembler->num_errors == 0 &&
         assembler->object.pass <= ASM_OBJECT_FINAL_PASS) {
    for (size_t i = 0; i < inputs->length; i++) {
      AssemblerRecordedInput* input = inputs->value.p[i];
      if (input->kind == kAssemblerRecordedText) {
        AssemblerAssembleInput(
            assembler, input->name != NULL ? input->name : "<generated>",
            input->value.text, run_func);
      } else {
        input->value.emitter.emit(assembler, input->value.emitter.context);
      }
    }
    AssemblerAdvancePass(assembler);
    if (assembler->object.pass <= ASM_OBJECT_FINAL_PASS) {
      AssemblerReset(assembler, false);
    }
  }

  AsmObjectWriteELF(&assembler->object, assembler->out);
}

void AssemblerRun(Assembler* assembler, void (*run_func)(Assembler*, String*)) {
  // Pass 1 discovers symbols, pass 2 converges variable-length directive
  // offsets, and pass 3 emits the final contents.
  assembler->object.pass = 1;
  LexNextToken(&assembler->lex);
  while (assembler->num_errors == 0 &&
         assembler->object.pass <= ASM_OBJECT_FINAL_PASS) {
    Assemble(assembler, run_func);  // Run the assembly pass.
    AssemblerAdvancePass(assembler);
    if (assembler->object.pass <= ASM_OBJECT_FINAL_PASS) {
      AssemblerReset(assembler, false);
    }
  }

  AsmObjectWriteELF(&assembler->object, assembler->out);
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

static void HandleDirective_p2align(Assembler* assembler);

static void HandleDirective_align(Assembler* assembler) {
  // GNU `as` treats `.align n` as 2^n-byte alignment on ARM.
  HandleDirective_p2align(assembler);
}

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
                               assembler->object.current_section, SYM_TYPE(none),
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
      if (assembler->object.pass == 1 && sym->defined && sym->section != SHN_COM) {
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
        sym = NewAssemblerSymbol(name.value, assembler->object.current_section,
                                 SYM_TYPE(none), SYM_BIND(local), 0);
        AssemblerInsertSymbol(assembler, sym);
      }

      // If the symbol has a .type then it is exported to the object file.
      sym->exported = true;
      if (LexLookingAt(&assembler->lex, TOK(identifier))) {
        AssemblerSection* sect = sym->section == SHN_COM ? NULL :
          assembler->object.sections.value.p[sym->section];
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
    if (assembler->object.pass == 1 && assembler->parsing_layout_expression) {
      assembler->object.requires_layout_pass = true;
    }
    String spelling = assembler->lex.spelling;
    AssemblerSymbol* sym = AssemblerFindSymbol(assembler, spelling.value);
    LexNextToken(&assembler->lex);
    if (sym == NULL) {
      sym = NewAssemblerSymbol(spelling.value, 0, SYM_TYPE(none),
                               SYM_BIND(global), 0);
      if (strcmp(spelling.value, ".") == 0) {
        sym->is_label = true;
        sym->defined = true;
        sym->section = assembler->object.current_section;
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
  switch (assembler->object.elf_machine_type) {
    case ELF_MACHINE_TYPE_X86_64:
      return R_X86_64_PC32;
    case ELF_MACHINE_TYPE_AARCH64:
      return R_AARCH64_PREL32;
    default:
      return assembler->object.reloc_types[kRelocSet32];
  }
}

static bool IsEhTableSection(const Assembler* assembler) {
  if (assembler->object.current_section < 0 ||
      (size_t)assembler->object.current_section >= assembler->object.sections.length) {
    return false;
  }
  AssemblerSection* section =
      assembler->object.sections.value.p[assembler->object.current_section];
  if (section->name == NULL) {
    return false;
  }
  if (assembler->object.elf_machine_type == ELF_MACHINE_TYPE_ARM &&
      (strcmp(section->name->value, ".ARM.exidx") == 0 ||
       strcmp(section->name->value, ".ARM.extab") == 0)) {
    return true;
  }
  return strcmp(section->name->value, ".gcc_except_table") == 0 ||
         strcmp(section->name->value, ".eh_frame") == 0;
}

static int RelocTypeForWord(Assembler* assembler) {
  if (IsEhTableSection(assembler)) {
    if (assembler->object.elf_machine_type == ELF_MACHINE_TYPE_ARM) {
      AssemblerSection* section =
          assembler->object.sections.value.p[assembler->object.current_section];
      if (strcmp(section->name->value, ".ARM.exidx") == 0 ||
          strcmp(section->name->value, ".ARM.extab") == 0) {
        return R_ARM_PREL31;
      }
      return R_ARM_REL32;
    }
    return EhTableWordRelocType(assembler);
  }
  return assembler->object.reloc_types[kRelocSet32];
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
  if (assembler->object.pass != ASM_OBJECT_FINAL_PASS) {
    while (LexLookingAt(&assembler->lex, TOK(plus)) ||
           LexLookingAt(&assembler->lex, TOK(minus))) {
      LexNextToken(&assembler->lex);
      if (LexLookingAt(&assembler->lex, TOK(number))) {
        LexNextToken(&assembler->lex);
      } else if (ExpressionPrimary(assembler) == NULL) {
        break;
      }
    }
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
                 : assembler->object.reloc_types[reloc_index];
  AssemblerRelocation* reloc = NewAssemblerRelocation(
      left, initial_reloc_type, assembler->object.current_section,
      (int32_t)AssemblerCurrentAddress(assembler), 0);
  
  VectorAppend(&relocations, reloc);
  bool known_values = left->is_label && left->type == SYM_TYPE(none) &&
      assembler->object.current_section == left->section && !assembler->object.absolute;
  // The section base address cancels out of an expression only when its
  // additive and subtractive same-section terms balance (e.g. a `label2 -
  // label1` difference).  Such a value is position-independent and can be
  // resolved here with no relocation.  A lone `label` (or otherwise unbalanced
  // expression) is section-base-relative, so it MUST keep its relocation for
  // the linker to patch in the final address; folding it to the assembly-time
  // section offset would leave a bogus small value at run time.
  int additive_terms = 1;
  int subtractive_terms = 0;
  int64_t constant_addend = 0;

  while (LexLookingAt(&assembler->lex, TOK(plus)) ||
         LexLookingAt(&assembler->lex, TOK(minus))) {
    Token tok = assembler->lex.current_token;
    LexNextToken(&assembler->lex);
    if (LexLookingAt(&assembler->lex, TOK(number))) {
      int64_t value = assembler->lex.number;
      LexNextToken(&assembler->lex);
      constant_addend += tok == TOK(plus) ? value : -value;
      continue;
    }
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
                right, assembler->object.reloc_types[reloc_type], assembler->object.current_section,
                            (int32_t)AssemblerCurrentAddress(assembler), 0);
    known_values &= right->is_label && right->type == SYM_TYPE(none) &&
        assembler->object.current_section == right->section;
    if (tok == TOK(plus)) {
      additive_terms++;
    } else {
      subtractive_terms++;
    }
    VectorAppend(&relocations, reloc);
  }
  ((AssemblerRelocation*)relocations.value.p[0])->addend += constant_addend;
  int64_t value = constant_addend;
  if (known_values && additive_terms == subtractive_terms &&
      !(bits == 32 && IsEhTableSection(assembler) &&
        RelocTypeForWord(assembler) == R_ARM_PREL31)) {
    value += left->value;
    for (size_t i = 1; i < relocations.length; i++) {
      AssemblerRelocation* reloc = relocations.value.p[i];
      if (reloc->type == assembler->object.reloc_types[kRelocAdd64] ||
          reloc->type == assembler->object.reloc_types[kRelocAdd32] ||
          reloc->type == assembler->object.reloc_types[kRelocAdd16]) {
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
        assembler->object.elf_machine_type == ELF_MACHINE_TYPE_X86_64 ||
        assembler->object.elf_machine_type == ELF_MACHINE_TYPE_AARCH64 ||
        assembler->object.elf_machine_type == ELF_MACHINE_TYPE_ARM;
    if (direct_pcrel32 &&
        (sub_is_dot ||
         (sub_reloc->symbol->is_label && sub_reloc->symbol->defined &&
          sub_reloc->symbol->section == assembler->object.current_section &&
          sub_reloc->type == assembler->object.reloc_types[kRelocSub32]))) {
      int64_t addend = add_reloc->addend;
      if (!sub_is_dot) {
        addend += (int64_t)AssemblerCurrentAddress(assembler) -
                  sub_reloc->symbol->value;
      }
      AssemblerRelocation* merged = NewAssemblerRelocation(
          add_reloc->symbol, RelocTypeForWord(assembler),
          assembler->object.current_section,
          (int32_t)AssemblerCurrentAddress(assembler), addend);
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

    AsmObjectAlignCurrentSection(&assembler->object, alignment + 1);
  } else {
    AssemblerError(assembler, "Alignment expected");
  }
}

static int64_t EvaluateDataDirectiveValue(Assembler* assembler) {
  if (LexLookingAt(&assembler->lex, TOK(number))) {
    int64_t value = assembler->lex.number;
    LexNextToken(&assembler->lex);
    return value;
  }
  return AssemblerEvaluateExpression(assembler);
}

static void HandleDataDirective(Assembler* assembler, int bits) {
  while (!LexEof(&assembler->lex)) {
    int64_t value = EvaluateDataDirectiveValue(assembler);
    switch (bits) {
      case 8:
        AssemblerEmitByte(assembler, assembler->object.current_section,
                          (uint8_t)value);
        break;
      case 16:
        AssemblerEmitHalf(assembler, assembler->object.current_section,
                          (uint16_t)value);
        break;
      case 32:
        AssemblerEmitWord(assembler, assembler->object.current_section,
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
  assembler->parsing_layout_expression = true;
  int64_t num_bytes = AssemblerEvaluateExpression(assembler);
  assembler->parsing_layout_expression = false;
  if (num_bytes < 0) {
    AssemblerError(assembler, "Invalid .space size %" PRId64 "", num_bytes);
    return;
  }
  int value = 0;
  if (LexMatch(&assembler->lex, TOK(comma))) {
    value = (int)AssemblerEvaluateExpression(assembler);
  }
  AsmObjectEmitFill(&assembler->object, assembler->object.current_section,
                    num_bytes, value);
}

// A string output with optional \0 after the characters.
static void StringDirective(Assembler* assembler, bool append_zero) {
  for (;;) {
    if (LexLookingAt(&assembler->lex, TOK(string))) {
      String str;
      StringInit(&str, assembler->lex.spelling.value);
      for (size_t i = 0; i < str.length + append_zero; i++) {
        AssemblerEmitByte(assembler, assembler->object.current_section, str.value[i]);
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
    AssemblerEmitByte(assembler, assembler->object.current_section, byte);
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
    AssemblerEmitByte(assembler, assembler->object.current_section, byte);
  }
}

static void HandleDirective_uleb128(Assembler* assembler) {
  assembler->parsing_layout_expression = true;
  int64_t value = AssemblerEvaluateExpression(assembler);
  assembler->parsing_layout_expression = false;
  if (value < 0) {
    AssemblerError(assembler, "Invalid .uleb128 value %" PRId64 "", value);
    return;
  }
  AssemblerEmitUleb128(assembler, (uint64_t)value);
}

static void HandleDirective_sleb128(Assembler* assembler) {
  assembler->parsing_layout_expression = true;
  int64_t value = AssemblerEvaluateExpression(assembler);
  assembler->parsing_layout_expression = false;
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
      AssemblerEmitHalf(assembler, assembler->object.current_section, value);
    } else {
      int16_t value = (int16_t)EvaluateDataDirectiveValue(assembler);
      AssemblerEmitHalf(assembler, assembler->object.current_section, value);
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
      size_t reloc_count = assembler->object.relocations.length;
      int32_t value = (int32_t)SimpleSymbolExpression(assembler, 32);
      AssemblerEmitWord(assembler, assembler->object.current_section, value);
      int32_t field_offset =
          (int32_t)AssemblerCurrentAddress(assembler) - (int32_t)sizeof(int32_t);
      for (size_t i = reloc_count; i < assembler->object.relocations.length; i++) {
        AssemblerRelocation* reloc =
            (AssemblerRelocation*)assembler->object.relocations.value.p[i];
        reloc->offset = field_offset;
      }
    } else {
      int32_t value = (int32_t)EvaluateDataDirectiveValue(assembler);
      AssemblerEmitWord(assembler, assembler->object.current_section, value);
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
      AssemblerEmitLong(assembler, assembler->object.current_section, value);
    } else {
      int64_t value = EvaluateDataDirectiveValue(assembler);
      AssemblerEmitLong(assembler, assembler->object.current_section, value);
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
  return assembler->object.elf_machine_type == ELF_MACHINE_TYPEW65C02 ? 1 : 8;
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
    if (assembler->object.pass == 1) {
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
      assembler->object.current_section = section;
    }
  } else {
    AssemblerError(assembler, "Missing section name");
  }
}

static void HandleDirective_text(Assembler* assembler) {
  String* name = NewString(".text");
  int section;
  if (assembler->object.pass == 1) {
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
  assembler->object.current_section = section;
}

static void HandleDirective_data(Assembler* assembler) {
  String* name = NewString(".data");
  int section;
  if (assembler->object.pass == 1) {
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
  assembler->object.current_section = section;
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
      StringSet(&assembler->object.filename, assembler->lex.spelling.value);
    }
    LexNextToken(&assembler->lex);
  } else {
    AssemblerError(assembler, "Missing filename string in .file directive");
  }
  if (assembler->object.pass == 1) {
    DwarfAddFile(&assembler->object.dwarf, &filename);
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

  if (assembler->object.pass == 1) {
    DwarfAddLocation(&assembler->object.dwarf, file, line, col,
                     AssemblerCurrentAddress(assembler));
  }
}

static void HandleDirective_option(Assembler* assembler) {
  while (LexLookingAt(&assembler->lex, TOK(identifier))) {
    if (StringEqual(&assembler->lex.spelling, "pic")) {
      assembler->object.pic = true;
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
    if (assembler->object.pass == 1) {
      // Only define symbols in pass 1.
      if (sym != NULL) {
        if (!sym->defined) {
          sym->defined = true;
          sym->is_constant = true;
          sym->section = assembler->object.current_section;
          sym->value = AssemblerCurrentAddress(assembler);
        } else {
          AssemblerError(assembler, "Duplicate symbol %s", name.value);
        }
      } else {
        // Symbol is new, define it as a local constant.
        sym = NewAssemblerSymbol(name.value, assembler->object.current_section,
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

