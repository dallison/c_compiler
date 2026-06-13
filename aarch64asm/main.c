//
//  main.c
//  aarch64asm
//

// Standalone AArch64 assembler that generates ELF object files.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aarch64_assembler.h"
#include "compiler.h"
#include "dstring.h"
#include "vector.h"

int main(int argc, char* argv[]) {
  Vector asm_filenames;
  String object_filename;
  Vector options;

  VectorInit(&asm_filenames);
  StringInit(&object_filename, NULL);
  VectorInit(&options);
  ParseOptions(argc, argv, &options);

  CompilerOptionValue* target = calloc(sizeof(CompilerOptionValue), 1);
  target->opt = kOptionTarget;
  StringInit(&target->value.svalue, "aarch64");
  VectorAppend(&options, target);

  for (size_t i = 0; i < options.length; i++) {
    CompilerOptionValue* option = options.value.p[i];
    if (option->opt == kOptionInputFile) {
      VectorAppend(&asm_filenames, &option->value.svalue);
    } else if (option->opt == kOptionOutputFile) {
      if (object_filename.length != 0) {
        fprintf(stderr, "Only one -o on command line please\n");
        exit(1);
      }
      StringSetString(&object_filename, &option->value.svalue);
    }
  }

  if (asm_filenames.length == 0) {
    fprintf(stderr, "Need at least one assembly language file\n");
    exit(1);
  }

  if (asm_filenames.length > 1 && object_filename.length != 0) {
    fprintf(stderr, "Can only use -o with one input file\n");
    exit(1);
  }

  for (size_t i = 0; i < asm_filenames.length; i++) {
    String* asm_filename = asm_filenames.value.p[i];
    String output_filename;
    StringInit(&output_filename, object_filename.value);

    if (object_filename.length == 0) {
      StringSet(&output_filename, asm_filename->value);
      char* suffix = strstr(output_filename.value, ".s");
      if (suffix == NULL) {
        StringAppend(&output_filename, ".o");
      } else {
        suffix[1] = 'o';
      }
    }

    bool ok = CompilerInitForAssembler(asm_filename->value, &options);
    if (!ok) {
      exit(1);
    }
    CreateGlobalSymbolTables();

    AARCH64Assembler* assembler =
        NewAARCH64Assembler(asm_filename, &output_filename);
    AssemblerRun(&assembler->base, AssembleAARCH64Instruction);

    int num_errors = assembler->base.num_errors;
    AARCH64AssemblerDestruct(assembler);
    if (num_errors != 0) {
      exit(1);
    }
    CompilerDelete(compiler);
  }
  VectorDestruct(&asm_filenames);
  return 0;
}
