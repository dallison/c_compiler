//
//  common_emitter.c
//  c_compiler_library
//
//  Created by David Allison on 11/28/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "common_emitter.h"
#include "compiler.h"
#include "source.h"
#include "symbol.h"
#include "debug.h"
#include "type_compare.h"
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "target_generator.h"

static COMPILER_UNUSED void FilePrinter(int index, File* file, void* data) {
  FILE* fp = data;
  if (index == 0) {
    // Don't emit index 0 as this is the current file.
    return;
  }
  fprintf(fp, "\t.file %d \"%s\"\n", index, file->name.value);
}


void EmitAssemblyPreamble(String* src_file, FILE* fp) {
  // Emit a .file directive without the file index.  This tells the
  // assembler the name of the current file.
  fprintf(fp, "\t.file   \"%s\"\n", src_file->value);

  // The assembler's compact line table is also consumed by stacktrace
  // metadata generation, independently of full DWARF debug information.
  SourceTraverseFiles(fp, FilePrinter);
  fprintf(fp, "\t.text\n");
  if (compiler->pic) {
    fprintf(fp, "\t.option pic\n");
  }
}

FILE* EmitAssemblyFile(String* src_file, String* asm_file) {
  FILE* fp;
  if (StringEqual(asm_file, "-")) {
    fp = stdout;
  } else {
    fp = fopen(asm_file->value, "w");
  }
  if (fp == NULL) {
    return NULL;
  }
  EmitAssemblyPreamble(src_file, fp);
  return fp;
}


void EmitFunctionSection(FILE* fp, const char* func_name) {
  if (compiler != NULL && compiler->function_sections && func_name != NULL &&
      func_name[0] != '\0') {
    fprintf(fp, "\t.section \".text.%s\", \"ax\", @progbits\n", func_name);
  } else {
    fprintf(fp, "\t.text\n");
  }
}

void EmitFunctionSectionToModule(AsmModule* module, const char* func_name) {
  if (compiler != NULL && compiler->function_sections && func_name != NULL &&
      func_name[0] != '\0') {
    char name[512];
    snprintf(name, sizeof(name), ".text.%s", func_name);
    AsmModuleSection(module, name, SHT(progbits),
                     SHF(alloc) | SHF(execinstr), compiler->alignment);
  } else {
    AsmModuleSection(module, ".text", SHT(progbits),
                     SHF(alloc) | SHF(execinstr), compiler->alignment);
  }
}

void EmitDataStart(FILE* fp) { fprintf(fp, "\t.data\n"); }

static const char* VarName(InitializedStaticVariable* var, char* buf, size_t len) {
  return TargetSymbolName(var->symbol, buf, len);
}

static const char* VarName2(UninitializedStaticVariable* var, char* buf, size_t len) {
  return TargetSymbolName(var->symbol, buf, len);
}

static void EmitBinding(FILE* fp, const char* name, bool is_global,
                        bool is_weak) {
  if (is_weak) {
    fprintf(fp, "\t.weak   %s\n", name);
  } else if (is_global) {
    fprintf(fp, "\t.global %s\n", name);
  } else {
    fprintf(fp, "\t.local  %s\n", name);
  }
}

void EmitStaticVariable(InitializedStaticVariable* var, FILE* fp) {
  char buf[256];
  // Pick a data directive that emits exactly pointer_size bytes.  The
  // assembler treats .short as 2 bytes, .word as 4 bytes and .8byte as 8
  // bytes, so a 2-byte pointer target (e.g. 6502) must use .short or the
  // pointer would occupy 4 bytes and shift every following field.
  const char* ptr_asm =
      compiler->pointer_size == 8 ? ".8byte" :
      compiler->pointer_size == 2 ? ".short" : ".word";
  const char* long_asm =
      compiler->pointer_size == 8 ? ".8byte" : ".long";
  EmitP2Align(var->alignment, fp);
  fprintf(fp, "%s:\n", VarName(var, buf, sizeof(buf)));
  fprintf(fp, "\t.type   %s,@object\n", VarName(var, buf, sizeof(buf)));
  EmitBinding(fp, VarName(var, buf, sizeof(buf)), var->is_global,
              var->is_weak);
  fprintf(fp, "\t.size   %s,%d\n", VarName(var, buf, sizeof(buf)), var->symbol->type->size);

  int next_offset = 0;
  for (size_t i = 0; i < var->initializers.length; i++) {
    Initializer* init = var->initializers.value.p[i];
    if (init->offset > next_offset) {
      int diff = init->offset - next_offset;
      fprintf(fp, "\t.space  %d\t\t// offset %d\n", diff, next_offset);
      next_offset += diff;
    }
    switch (init->type) {
      case kInitTypeByte:
        fprintf(fp, "\t.byte   %d\t\t// offset %d\n", (int32_t)init->value.byte, next_offset);
        next_offset += 1;
        break;
      case kInitTypeHalf:
        fprintf(fp, "\t.short   %d\t\t// offset %d\n", (int32_t)init->value.half, next_offset);
        next_offset += 2;
        break;
      case kInitTypeWord:
        fprintf(fp, "\t.word   %d\t\t// offset %d\n", init->value.word, next_offset);
        next_offset += 4;
        break;
      case kInitTypeLong:
        fprintf(fp, "\t%s   %" PRId64 "\t\t// offset %d\n", long_asm,
                (int64_t)init->value._long, next_offset);
        next_offset += 8;
        break;
      case kInitTypeSymbol:
        if (init->value.symbol->flags.is_local) {
          fprintf(fp, "\t.local %s\n", TargetSymbolName(init->value.symbol,  buf, sizeof(buf)));
        } else if (SymbolHasWeakBinding(init->value.symbol)) {
          fprintf(fp, "\t.weak %s\n", TargetSymbolName(init->value.symbol,  buf, sizeof(buf)));
        } else {
          fprintf(fp, "\t.global %s\n", TargetSymbolName(init->value.symbol,  buf, sizeof(buf)));
        }
        if (init->symbol_addend != 0) {
          fprintf(fp, "\t%s    %s%+" PRId64 "\t\t// offset %d\n", ptr_asm,
                  TargetSymbolName(init->value.symbol, buf, sizeof(buf)),
                  init->symbol_addend, next_offset);
        } else {
          fprintf(fp, "\t%s    %s\t\t// offset %d\n", ptr_asm,
                  TargetSymbolName(init->value.symbol, buf, sizeof(buf)),
                  next_offset);
        }
        next_offset += compiler->pointer_size;
        break;
      case kInitTypeString:
        fprintf(fp, "\t%s    .str.%d\t\t// offset %d\n", ptr_asm,
                init->value.literal_id, next_offset);
        next_offset += compiler->pointer_size;
        break;
      case kInitTypeMemory: {
        int byte_count = 0;
        const char* sep = "";
        const int kByteLimit = 16;  // 16 bytes per line.
        fprintf(fp, "\t// offset %d\n", next_offset);
        for (size_t i = 0; i < init->value.memory.length; i++) {
          if (byte_count == 0) {
            fprintf(fp, "\t.byte ");
          }
          fprintf(fp, "%s0x%02x", sep, init->value.memory.value[i]);
          sep = ",";
          byte_count++;
          if (byte_count == kByteLimit) {
            byte_count = 0;
            fprintf(fp, "\n");
            sep = "";
          }
        }
        fprintf(fp, "\n");
        next_offset += (int)init->value.memory.length;
        break;
      }
    }
  }

  // Pad to full size.
  ssize_t pad = var->size - next_offset;
  if (pad > 0) {
    fprintf(fp, "\t.space  %zd\n", pad);
  }
  fprintf(fp, "\n");
}


void EmitBSSVariable(UninitializedStaticVariable* var, FILE* fp) {
  char buf[256];
  fprintf(fp, "\t.type   %s,@object\n", VarName2(var, buf, sizeof(buf)));
  EmitBinding(fp, VarName2(var, buf, sizeof(buf)), var->is_global,
              var->is_weak);
  if (var->is_weak) {
    fprintf(fp, "\t.size   %s,%zd\n", VarName2(var, buf, sizeof(buf)),
            var->size);
    EmitP2Align((int)var->alignment, fp);
    fprintf(fp, "%s:\n", VarName2(var, buf, sizeof(buf)));
    fprintf(fp, "\t.space  %zd\n", var->size);
    fprintf(fp, "\n");
    return;
  }
  fprintf(fp, "\t.comm   %s,%zd,%zd\n", VarName2(var, buf, sizeof(buf)), var->size,
          var->alignment);
  fprintf(fp, "\n");
}

void EmitStringLiteralSection(FILE* fp) {
  // String literals are in their own section.  The flags
  // mean:
  //  a: allocated in program
  //  M: can be merged with other rodata sections.
  //  S: contains strings.
  // These are used by the linker.
  fprintf(fp, "\t.section \".rodata\", \"aMS\", @progbits\n");
}

void EmitLiteral(Literal* literal, FILE* fp) {
  // A string literal might be disabled if it's used in an
  // asm statement and has alrady been emitted as assembly
  // language.
  if (literal->disabled) {
    return;
  }
  switch (literal->type) {
    case kLiteralWideString: {
      fprintf(fp, ".str.%d:\n", literal->id);
      StringLiteral* slit = (StringLiteral*)literal;
      for (size_t i = 0; i < slit->value.length; i++) {
        fprintf(fp, "\t.byte 0x%02x\n", slit->value.value[i] & 0xff);
      }
      for (int i = 0; i < slit->element_size; i++) {
        fprintf(fp, "\t.byte 0x00\n");
      }
      // The literal is an object and the size includes the zero
      // at the end.
      fprintf(fp, "\t.type .str.%d, @object\n", literal->id);
      fprintf(fp, "\t.size .str.%d, %zd\n", literal->id,
              slit->value.length + slit->element_size);
      fprintf(fp, "\n");
      break;
    }
    case kLiteralString: {
      // Each string literal has its own symbol.  The symbol
      // is of the form ".str.xx" where 'xx' is the literal
      // id (assigned when the string literal is compiled).
      // A reference to this symbol is output to a movxc
      // instruction with a relocation so that the linker
      // can set the address.
      fprintf(fp, ".str.%d:\n", literal->id);

      StringLiteral* slit = (StringLiteral*)literal;
      for (size_t i = 0; i < slit->value.length; i++) {
        fprintf(fp, "\t.byte 0x%02x\n", slit->value.value[i] & 0xff);
      }
      fprintf(fp, "\t.byte 0x00\n");

      // The literal is an object and the size includes the zero
      // at the end.
      fprintf(fp, "\t.type .str.%d, @object\n", literal->id);
      fprintf(fp, "\t.size .str.%d, %zd\n", literal->id,
              slit->value.length + 1);
      fprintf(fp, "\n");
      break;
    }
    case kLiteralBuffer: {
      fprintf(fp, ".lit.%d:\n", literal->id);
      BufferLiteral* buf = (BufferLiteral*)literal;
      for (size_t i = 0; i < buf->value.length; i++) {
        fprintf(fp, "\t.byte 0x%02x\n", buf->value.value[i] & 0xff);
      }
      fprintf(fp, "\t.type .lit.%d, @object\n", literal->id);
      fprintf(fp, "\t.size .lit.%d, %zd\n", literal->id, buf->value.length);
      fprintf(fp, "\n");
      break;
    }
  }
}

void EmitDebug(FILE* fp) {
  fprintf(fp, "\t.section \".debug_line\", \"\", @progbits, 1\n");
}

void EmitP2Align(int alignment, FILE* fp) {
  alignment = alignment - 1;
  // The p2align directive takes, as its first argument the number
  // of bits to align to.  We caluclate this by counting the
  // lower order 1 bits in the alignment.
  int p2align_arg = 0;
  for (int i = 0; i < 16; i++) {
    if ((alignment & (1 << i)) != 0) {
      p2align_arg++;
    } else {
      break;
    }
  }
  fprintf(fp, "\t.p2align  %d\n", p2align_arg);
}

void EmitTlsDataStart(FILE* fp) { fprintf(fp, "\t.section \".tdata\", \"awT\", @progbits\n"); }
void EmitTlsBSSStart(FILE* fp) { fprintf(fp, "\t.section \".tbss\", \"awT\", @nobits\n"); }

void EmitTlsBSSVariable(UninitializedStaticVariable* var, FILE* fp) {
  char buf[256];
  fprintf(fp, "\t.type   %s,@object\n", VarName2(var, buf, sizeof(buf)));
  EmitBinding(fp, VarName2(var, buf, sizeof(buf)), var->is_global,
              var->is_weak);
  EmitP2Align((int)var->alignment, fp);
  fprintf(fp, "%s:\n", VarName2(var, buf, sizeof(buf)));
  fprintf(fp, "\t.space   %zd\n", var->size);
  fprintf(fp, "\n");
}

void EmitTlsVariable(InitializedStaticVariable* var, FILE* fp) {
  EmitStaticVariable(var, fp);
}

static void EmitFunctionPointer(Symbol* function, FILE* fp) {
  char buf[256];
  const char* ptr_asm =
      compiler->pointer_size == 8 ? ".8byte" :
      compiler->pointer_size == 2 ? ".short" : ".word";
  if (function->flags.is_local) {
    fprintf(fp, "\t.local %s\n",
            TargetSymbolName(function, buf, sizeof(buf)));
  } else if (SymbolHasWeakBinding(function)) {
    fprintf(fp, "\t.weak %s\n",
            TargetSymbolName(function, buf, sizeof(buf)));
  } else {
    fprintf(fp, "\t.global %s\n",
            TargetSymbolName(function, buf, sizeof(buf)));
  }
  fprintf(fp, "\t%s    %s\n", ptr_asm,
          TargetSymbolName(function, buf, sizeof(buf)));
}

static int InitFiniPriorityForSymbol(Symbol* function, bool is_fini) {
  if (function == NULL) {
    return kCXXInitFiniPriorityDefault;
  }
  const char* name = is_fini ? "destructor" : "constructor";
  Attribute* attr = AttributeListFind(&function->attributes, name);
  if (attr == NULL) {
    return kCXXInitFiniPriorityDefault;
  }
  long value = 0;
  if (!AttributeArgInt(attr, 0, &value) || value < 0 || value > 65535) {
    return kCXXInitFiniPriorityDefault;
  }
  return (int)value;
}

static void StableSortInitFiniFunctions(Symbol** functions, size_t count,
                                        bool is_fini) {
  for (size_t i = 1; i < count; i++) {
    Symbol* function = functions[i];
    int priority = InitFiniPriorityForSymbol(function, is_fini);
    size_t j = i;
    while (j != 0 &&
           InitFiniPriorityForSymbol(functions[j - 1], is_fini) > priority) {
      functions[j] = functions[j - 1];
      j--;
    }
    functions[j] = function;
  }
}

static void EmitInitFiniSectionStart(int priority, bool is_fini, FILE* fp) {
  const char* section = is_fini ? ".fini_array" : ".init_array";
  const char* type = is_fini ? "fini_array" : "init_array";
  if (priority == kCXXInitFiniPriorityDefault) {
    fprintf(fp, "\t.section \"%s\", \"aw\", @%s, %d\n", section, type,
            compiler->pointer_size);
  } else {
    fprintf(fp, "\t.section \"%s.%05d\", \"aw\", @%s, %d\n", section,
            priority, type, compiler->pointer_size);
  }
  EmitP2Align(compiler->pointer_size, fp);
}

void CollectInitFiniArrayFunctions(Vector* functions, bool is_fini,
                                   Vector* out) {
  if (functions == NULL || functions->length == 0) {
    return;
  }
  Symbol** sorted = malloc(functions->length * sizeof(Symbol*));
  if (sorted == NULL) {
    return;
  }
  for (size_t i = 0; i < functions->length; i++) {
    sorted[i] = functions->value.p[i];
  }
  StableSortInitFiniFunctions(sorted, functions->length, is_fini);
  for (size_t i = 0; i < functions->length; i++) {
    VectorAppend(out, sorted[i]);
  }
  free(sorted);
}

void EmitInitFiniArrayEntries(Vector* functions, bool is_fini, FILE* fp) {
  if (functions == NULL || functions->length == 0) {
    return;
  }

  Symbol** sorted = malloc(functions->length * sizeof(Symbol*));
  if (sorted == NULL) {
    return;
  }
  for (size_t i = 0; i < functions->length; i++) {
    sorted[i] = functions->value.p[i];
  }
  StableSortInitFiniFunctions(sorted, functions->length, is_fini);

  int current_priority = -1;
  for (size_t i = 0; i < functions->length; i++) {
    int priority = InitFiniPriorityForSymbol(sorted[i], is_fini);
    if (priority != current_priority) {
      if (current_priority >= 0) {
        fprintf(fp, "\n");
      }
      EmitInitFiniSectionStart(priority, is_fini, fp);
      current_priority = priority;
    }
    EmitFunctionPointer(sorted[i], fp);
  }
  fprintf(fp, "\n");
  free(sorted);
}

bool EmitTranslationUnitRemainder(struct Compiler* compiler, FILE* asm_file) {
  if (compiler->target->emit_cxx_thunks != NULL) {
    compiler->target->emit_cxx_thunks(asm_file);
  }
  if (compiler->debug_output) {
    fprintf(asm_file, ".PCend:\n");
  }
  compiler->target->emit_data_start(asm_file);

  bool contains_tls_vars = false;

  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (!var->is_tls) {
      compiler->target->emit_static_variable(var, asm_file);
    }
    contains_tls_vars |= var->is_tls;
  }

  for (size_t i = 0; i < compiler->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        compiler->uninitialized_static_variables.value.p[i];
    if (!var->is_tls &&
        (var->symbol->flags.is_tentative_decl || var->is_local ||
         (CompilerIsCXX() && TypeIsStructOrUnion(var->symbol->type)))) {
      compiler->target->emit_bss_space(var, asm_file);
    }
    contains_tls_vars |= var->is_tls;
  }

  compiler->target->emit_literals_start(asm_file);

  for (size_t i = 0; i < compiler->literals.length; i++) {
    compiler->target->emit_literal(compiler->literals.value.p[i], asm_file);
  }

  if (contains_tls_vars) {
    compiler->target->emit_tdata_start(asm_file);
    for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
      InitializedStaticVariable* var =
          compiler->initialized_static_variables.value.p[i];
      if (var->is_tls) {
        compiler->target->emit_tls_variable(var, asm_file);
      }
    }

    compiler->target->emit_tbss_start(asm_file);

    for (size_t i = 0; i < compiler->uninitialized_static_variables.length;
         i++) {
      UninitializedStaticVariable* var =
          compiler->uninitialized_static_variables.value.p[i];
      if (var->is_tls) {
        compiler->target->emit_tbss_space(var, asm_file);
      }
    }
  }

  compiler->target->emit_debug(asm_file);
  if (compiler->debug_output) {
    compiler->debug_builder.fp = asm_file;
    DebugBuilderEmitDebugInfo(&compiler->debug_builder);
    DebugBuilderEmitAbbreviations(&compiler->debug_builder);
  }

  EmitInitFiniArrayEntries(CXXInitArrayFunctionsVector(), false, asm_file);
  EmitInitFiniArrayEntries(CXXFiniArrayFunctionsVector(), true, asm_file);
  return true;
}

bool EmitTranslationUnitContents(struct Compiler* compiler, FILE* asm_file) {
  for (size_t i = 0; i < compiler->functions.length; i++) {
    if (compiler->print_back_end) {
      compiler->target->emit_function_assembly(compiler->functions.value.p[i],
                                               stdout);
    }
    compiler->target->emit_function_assembly(compiler->functions.value.p[i],
                                             asm_file);
  }
  return EmitTranslationUnitRemainder(compiler, asm_file);
}

static void ModuleFilePrinter(int index, File* file, void* data) {
  if (index != 0) {
    AsmModuleFile(data, index, file->name.value);
  }
}

void EmitAssemblyPreambleToModule(struct Compiler* compiler,
                                  AsmModule* module) {
  AsmModuleFile(module, -1, compiler->infile.value);
  SourceTraverseFiles(module, ModuleFilePrinter);
  AsmModuleSection(module, ".text", SHT(progbits),
                   SHF(alloc) | SHF(execinstr), compiler->alignment);
}

static AssemblerSymbolBinding ModuleBinding(bool is_global, bool is_weak) {
  if (is_weak) {
    return SYM_BIND(weak);
  }
  return is_global ? SYM_BIND(global) : SYM_BIND(local);
}

static void EmitInitializedVariableToModule(InitializedStaticVariable* var,
                                            AsmModule* module) {
  char buf[256];
  const char* name = VarName(var, buf, sizeof(buf));
  AsmModuleAlign(module, var->alignment);
  AsmModuleSymbol(module, name, var->is_tls ? SYM_TYPE(tls)
                                            : SYM_TYPE(object),
                  ModuleBinding(var->is_global, var->is_weak), 0,
                  var->alignment, false, true, false);
  AsmModuleLabel(module, name);
  AsmExpr size;
  AsmExprInitConstant(&size, (int64_t)var->size);
  AsmModuleSymbolSize(module, name, &size);
  AsmExprDestruct(&size);

  int next_offset = 0;
  for (size_t i = 0; i < var->initializers.length; i++) {
    Initializer* init = var->initializers.value.p[i];
    if (init->offset > next_offset) {
      AsmModuleFill(module, init->offset - next_offset, 0);
      next_offset = init->offset;
    }
    AsmExpr value;
    switch (init->type) {
      case kInitTypeByte:
        AsmExprInitConstant(&value, init->value.byte);
        AsmModuleInteger(module, 1, &value);
        AsmExprDestruct(&value);
        next_offset++;
        break;
      case kInitTypeHalf:
        AsmExprInitConstant(&value, init->value.half);
        AsmModuleInteger(module, 2, &value);
        AsmExprDestruct(&value);
        next_offset += 2;
        break;
      case kInitTypeWord:
        AsmExprInitConstant(&value, init->value.word);
        AsmModuleInteger(module, 4, &value);
        AsmExprDestruct(&value);
        next_offset += 4;
        break;
      case kInitTypeLong:
        AsmExprInitConstant(&value, (int64_t)init->value._long);
        AsmModuleInteger(module, 8, &value);
        AsmExprDestruct(&value);
        next_offset += 8;
        break;
      case kInitTypeSymbol: {
        char symbol_buf[256];
        const char* symbol_name =
            TargetSymbolName(init->value.symbol, symbol_buf,
                             sizeof(symbol_buf));
        AsmExprInitSymbol(&value, symbol_name, init->symbol_addend);
        AsmModuleSymbol(
            module, value.symbol.value, SYM_TYPE(none),
            init->value.symbol->flags.is_local
                ? SYM_BIND(local)
                : SymbolHasWeakBinding(init->value.symbol) ? SYM_BIND(weak)
                                                           : SYM_BIND(global),
            0, 1, false, true, false);
        AsmModuleInteger(module, compiler->pointer_size, &value);
        AsmExprDestruct(&value);
        next_offset += compiler->pointer_size;
        break;
      }
      case kInitTypeString: {
        char literal_name[64];
        snprintf(literal_name, sizeof(literal_name), ".str.%d",
                 init->value.literal_id);
        AsmExprInitSymbol(&value, literal_name, 0);
        AsmModuleInteger(module, compiler->pointer_size, &value);
        AsmExprDestruct(&value);
        next_offset += compiler->pointer_size;
        break;
      }
      case kInitTypeMemory:
        AsmModuleBytes(module, init->value.memory.value,
                       init->value.memory.length);
        next_offset += (int)init->value.memory.length;
        break;
    }
  }
  if ((size_t)next_offset < var->size) {
    AsmModuleFill(module, (int64_t)var->size - next_offset, 0);
  }
}

void EmitStaticVariableToModule(InitializedStaticVariable* var,
                                AsmModule* module) {
  EmitInitializedVariableToModule(var, module);
}

void EmitBSSVariableToModule(UninitializedStaticVariable* var,
                             AsmModule* module) {
  char buf[256];
  const char* name = VarName2(var, buf, sizeof(buf));
  if (!var->is_weak) {
    AsmModuleSymbol(module, name, SYM_TYPE(common), SYM_BIND(global),
                    (int32_t)var->size,
                    (int32_t)var->alignment, true, true, true);
    return;
  }
  AsmModuleSymbol(module, name, SYM_TYPE(object), SYM_BIND(weak),
                  (int32_t)var->size, (int32_t)var->alignment, false, true,
                  false);
  AsmModuleAlign(module, (int)var->alignment);
  AsmModuleLabel(module, name);
  AsmModuleFill(module, (int64_t)var->size, 0);
}

void EmitLiteralToModule(Literal* literal, AsmModule* module) {
  if (literal->disabled) {
    return;
  }
  char name[64];
  snprintf(name, sizeof(name),
           literal->type == kLiteralBuffer ? ".lit.%d" : ".str.%d",
           literal->id);
  AsmModuleSymbol(module, name, SYM_TYPE(object), SYM_BIND(local), 0, 1,
                  false, true, false);
  AsmModuleLabel(module, name);
  size_t size = 0;
  if (literal->type == kLiteralBuffer) {
    BufferLiteral* value = (BufferLiteral*)literal;
    AsmModuleBytes(module, value->value.value, value->value.length);
    size = value->value.length;
  } else {
    StringLiteral* value = (StringLiteral*)literal;
    AsmModuleBytes(module, value->value.value, value->value.length);
    int terminator_size =
        literal->type == kLiteralWideString ? value->element_size : 1;
    AsmModuleFill(module, terminator_size, 0);
    size = value->value.length + (size_t)terminator_size;
  }
  AsmExpr size_expr;
  AsmExprInitConstant(&size_expr, (int64_t)size);
  AsmModuleSymbolSize(module, name, &size_expr);
  AsmExprDestruct(&size_expr);
}

void EmitTlsVariableToModule(InitializedStaticVariable* var,
                             AsmModule* module) {
  EmitInitializedVariableToModule(var, module);
}

void EmitTlsBSSVariableToModule(UninitializedStaticVariable* var,
                                AsmModule* module) {
  char buf[256];
  const char* name = VarName2(var, buf, sizeof(buf));
  AsmModuleSymbol(module, name, SYM_TYPE(tls),
                  ModuleBinding(var->is_global, var->is_weak),
                  (int32_t)var->size, (int32_t)var->alignment, false, true,
                  false);
  AsmModuleAlign(module, (int)var->alignment);
  AsmModuleLabel(module, name);
  AsmModuleFill(module, (int64_t)var->size, 0);
}

void EmitInitFiniArrayEntriesToModule(Vector* functions, bool is_fini,
                                      AsmModule* module) {
  if (functions == NULL || functions->length == 0) {
    return;
  }
  Symbol** sorted = malloc(functions->length * sizeof(Symbol*));
  if (sorted == NULL) {
    module->failed = true;
    return;
  }
  for (size_t i = 0; i < functions->length; i++) {
    sorted[i] = functions->value.p[i];
  }
  StableSortInitFiniFunctions(sorted, functions->length, is_fini);
  int current_priority = -1;
  for (size_t i = 0; i < functions->length; i++) {
    int priority = InitFiniPriorityForSymbol(sorted[i], is_fini);
    if (priority != current_priority) {
      char section[64];
      const char* base = is_fini ? ".fini_array" : ".init_array";
      if (priority == kCXXInitFiniPriorityDefault) {
        snprintf(section, sizeof(section), "%s", base);
      } else {
        snprintf(section, sizeof(section), "%s.%05d", base, priority);
      }
      AsmModuleSection(module, section,
                       is_fini ? SHT(fini_array) : SHT(init_array),
                       SHF(alloc) | SHF(write), compiler->pointer_size);
      AsmModuleAlign(module, compiler->pointer_size);
      current_priority = priority;
    }
    char buf[256];
    const char* name =
        TargetSymbolName(sorted[i], buf, sizeof(buf));
    AsmModuleSymbol(
        module, name, SYM_TYPE(func),
        sorted[i]->flags.is_local
            ? SYM_BIND(local)
            : SymbolHasWeakBinding(sorted[i]) ? SYM_BIND(weak)
                                              : SYM_BIND(global),
        0, 1, false, true, false);
    AsmExpr value;
    AsmExprInitSymbol(&value, name, 0);
    AsmModuleInteger(module, compiler->pointer_size, &value);
    AsmExprDestruct(&value);
  }
  free(sorted);
}

bool EmitTranslationUnitRemainderToModule(struct Compiler* compiler,
                                          AsmModule* module) {
  if (compiler->debug_output) {
    AsmModuleSection(module, ".text", SHT(progbits),
                     SHF(alloc) | SHF(execinstr), compiler->alignment);
    AsmModuleLabel(module, ".PCend");
  }
  AsmModuleSection(module, ".data", SHT(progbits),
                   SHF(alloc) | SHF(write), compiler->alignment);
  bool contains_tls_vars = false;
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (!var->is_tls) {
      EmitStaticVariableToModule(var, module);
    }
    contains_tls_vars |= var->is_tls;
  }
  for (size_t i = 0; i < compiler->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        compiler->uninitialized_static_variables.value.p[i];
    if (!var->is_tls &&
        (var->symbol->flags.is_tentative_decl || var->is_local ||
         (CompilerIsCXX() && TypeIsStructOrUnion(var->symbol->type)))) {
      EmitBSSVariableToModule(var, module);
    }
    contains_tls_vars |= var->is_tls;
  }
  AsmModuleSection(module, ".rodata", SHT(progbits),
                   SHF(alloc) | SHF(merge) | SHF(strings),
                   compiler->alignment);
  for (size_t i = 0; i < compiler->literals.length; i++) {
    EmitLiteralToModule(compiler->literals.value.p[i], module);
  }
  if (contains_tls_vars) {
    AsmModuleSection(module, ".tdata", SHT(progbits),
                     SHF(alloc) | SHF(write) | SHF(tls),
                     compiler->alignment);
    for (size_t i = 0; i < compiler->initialized_static_variables.length;
         i++) {
      InitializedStaticVariable* var =
          compiler->initialized_static_variables.value.p[i];
      if (var->is_tls) {
        EmitTlsVariableToModule(var, module);
      }
    }
    AsmModuleSection(module, ".tbss", SHT(nobits),
                     SHF(alloc) | SHF(write) | SHF(tls),
                     compiler->alignment);
    for (size_t i = 0; i < compiler->uninitialized_static_variables.length;
         i++) {
      UninitializedStaticVariable* var =
          compiler->uninitialized_static_variables.value.p[i];
      if (var->is_tls) {
        EmitTlsBSSVariableToModule(var, module);
      }
    }
  }
  AsmModuleSection(module, ".debug_line", SHT(progbits), 0, 1);
  if (compiler->debug_output) {
    compiler->debug_builder.module = module;
    DebugBuilderEmitDebugInfo(&compiler->debug_builder);
    DebugBuilderEmitAbbreviations(&compiler->debug_builder);
    compiler->debug_builder.module = NULL;
  }
  EmitInitFiniArrayEntriesToModule(CXXInitArrayFunctionsVector(), false,
                                   module);
  EmitInitFiniArrayEntriesToModule(CXXFiniArrayFunctionsVector(), true,
                                   module);
  return !module->failed;
}
