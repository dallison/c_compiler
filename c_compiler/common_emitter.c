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
#include <string.h>
#include <inttypes.h>
#include "target_generator.h"

static COMPILER_UNUSED void FilePrinter(int index, File* file, void* data) {
  FILE* fp = data;
  if (index == 0) {
    // Don't emit index 0 as this is the current file.
    return;
  }
  fprintf(fp, "\t.file %d \"%s\"\n", index, file->name.value);
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

  // Emit a .file directive without the file index.  This tells the
  // assembler the name of the current file.
  fprintf(fp, "\t.file   \"%s\"\n", src_file->value);

  if (compiler->debug_output) {
    // Print all source files.
    SourceTraverseFiles(fp, FilePrinter);
  }
  fprintf(fp, "\t.text\n");
  if (compiler->pic) {
    fprintf(fp, "\t.option pic\n");
  }
  return fp;
}


void EmitDataStart(FILE* fp) { fprintf(fp, "\t.data\n"); }

static const char* VarName(InitializedStaticVariable* var, char* buf, size_t len) {
  if (var->is_local) {
    snprintf(buf, len, ".local.%s.%d", var->symbol->name.value, var->symbol->id);
  } else {
    if (compiler->prepend_underscore) {
      snprintf(buf, len, "_%s", var->symbol->name.value);
    } else {
      snprintf(buf, len, "%s", var->symbol->name.value);
    }
  }
  return buf;
}

static const char* VarName2(UninitializedStaticVariable* var, char* buf, size_t len) {
  if (var->is_local) {
    snprintf(buf, len, ".local.%s.%d", var->symbol->name.value, var->symbol->id);
  } else {
    if (compiler->prepend_underscore) {
      snprintf(buf, len, "_%s", var->symbol->name.value);
    } else {
      snprintf(buf, len, "%s", var->symbol->name.value);
    }
  }
  return buf;
}

void EmitStaticVariable(InitializedStaticVariable* var, FILE* fp) {
  char buf[256];
  const char* ptr_asm =
      compiler->pointer_size == 8 ? ".8byte" : ".word";
  const char* long_asm =
      compiler->pointer_size == 8 ? ".8byte" : ".long";
  EmitP2Align(var->alignment, fp);
  fprintf(fp, "%s:\n", VarName(var, buf, sizeof(buf)));
  fprintf(fp, "\t.type   %s,@object\n", VarName(var, buf, sizeof(buf)));
  if (var->is_global) {
    fprintf(fp, "\t.global %s\n", VarName(var, buf, sizeof(buf)));
  } else {
    fprintf(fp, "\t.local  %s\n", VarName(var, buf, sizeof(buf)));
  }
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
        } else {
          fprintf(fp, "\t.global %s\n", TargetSymbolName(init->value.symbol,  buf, sizeof(buf)));
        }
        fprintf(fp, "\t%s    %s\t\t// offset %d\n", ptr_asm,
                TargetSymbolName(init->value.symbol, buf, sizeof(buf)),
                next_offset);
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
  if (var->is_global) {
    fprintf(fp, "\t.global %s\n", VarName2(var, buf, sizeof(buf)));
  } else {
    fprintf(fp, "\t.local  %s\n", VarName2(var, buf, sizeof(buf)));
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
      // The literal is an object and the size includes the zero
      // at the end.
      fprintf(fp, "\t.type .str.%d, @object\n", literal->id);
      fprintf(fp, "\t.size .str.%d, %zd\n", literal->id,
              slit->value.length + compiler->int_size);
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

      // Print the literal in escaped form. Any non-printable
      // characters are encoded in hex or as their usual
      // ANSI C escape characters.
      StringLiteral* slit = (StringLiteral*)literal;
      String escaped = {0};
      StringEscape(&slit->value, &escaped);
      fprintf(fp, "\t.asciz \"%s\"\n", escaped.value);
      StringDestruct(&escaped);

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
  fprintf(fp, "\t.section \".debug_line\", \"aMS\", @progbits\n");
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
  fprintf(fp, "\t.type   %s,@object\n", var->symbol->name.value);
  if (var->is_global) {
    fprintf(fp, "\t.global %s\n", var->symbol->name.value);
  } else {
    fprintf(fp, "\t.local  %s\n", var->symbol->name.value);
  }
  EmitP2Align((int)var->alignment, fp);
  fprintf(fp, "%s:\n", var->symbol->name.value);
  fprintf(fp, "\t.space   %zd\n", var->size);
  fprintf(fp, "\n");
}

void EmitTlsVariable(InitializedStaticVariable* var, FILE* fp) {
  EmitStaticVariable(var, fp);
}
