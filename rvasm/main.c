//
//  main.c
//  rvasm
//
//  Created by David Allison on 10/9/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

// This is RISC-V assembler that generates ELF object files.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dstring.h"
#include "vector.h"
#include "risc_v_assembler.h"
#include "compiler.h"

int main(int argc, char * argv[]) {
  Vector asm_filenames;
  String object_filename;
  Vector options;
  
  VectorInit(&asm_filenames);
  StringInit(&object_filename, NULL);
  VectorInit(&options);
  ParseOptions(argc, argv, &options);
  
  // Add -target risc-v to options
  CompilerOptionValue* target = calloc(sizeof(CompilerOptionValue), 1);
  target->opt = kOptionTarget;
  StringInit(&target->value.svalue, "risc-v");
  VectorAppend(&options, target);
  
  for (size_t i = 0; i < options.length; i++) {
    CompilerOptionValue* option = options.value[i];
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
  
  // If the user has specified -o, make sure there's only one .s file.
  if (asm_filenames.length > 1 && object_filename.length != 0) {
    fprintf(stderr, "Can only use -o with one input file\n");
    exit(1);
  }

  for (size_t i = 0; i < asm_filenames.length; i++) {
    String* asm_filename = asm_filenames.value[i];
    String output_filename;
    StringInit(&output_filename, object_filename.value);
    
    if (object_filename.length == 0) {
      StringSet(&output_filename, asm_filenames.value[i]);
      // No output file specified (no -o) so work it out.
      // If the file ends in ".s", make it ".o", otherwise append ".o".
      char* suffix = strstr(output_filename.value, ".s");
      if (suffix == NULL) {
        StringAppend(&output_filename, ".o");
      } else {
        // Overwrite 's' with 'o'.
        suffix[1] = 'o';
      }
    }
    
    // Create and run the assembler.
    // We need to initialize the compiler because we use it for expressions.  This
    // also handles options like -D, -I, etc.
    bool ok = CompilerInitForAssembler(asm_filename->value, &options);
    if (!ok) {
      exit(1);
    }
    // Create the global symbol tables.
    CreateGlobalSymbolTables();
    
    RVAssembler assembler;
    RVAssemblerInit(&assembler, asm_filename, &output_filename);
    AssemblerRun(&assembler.base, AssembleRVInstruction);
  
    int num_errors = assembler.base.num_errors;
    RVAssemblerDestruct(&assembler);
    if (num_errors != 0) {
      exit(1);
    }
    CompilerDelete(compiler);
  }
  VectorDestruct(&asm_filenames);
}
