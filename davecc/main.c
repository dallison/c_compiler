//
//  main.c
//  davecc
//
//  Created by David Allison on 6/27/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

// This is a driver for the C compiler, assembler, linker and interpreter.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dstring.h"
#include "vector.h"
#include "risc_v_assembler.h"
#include "6502_assembler.h"
#include "p_code_assembler.h"
#include "compiler.h"
#include "linker_main.h"

int main(int argc, char * argv[]) {
  Vector asm_files;
  
  Vector compiler_args;
  Vector linker_args;
  Vector object_files;

  VectorInit(&asm_files);
  VectorInit(&compiler_args);
  VectorInit(&linker_args);
  VectorInit(&object_files);
  VectorAppend(&compiler_args, "");   // argv[0]
  VectorAppend(&linker_args, "");   // argv[0]

  bool compile_only = false;
  bool run_compiler = false;
  
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // Option.
      String* option = NewString(argv[i]);
      if (StringStartsWith(option, "-Wl,")) {
        // Arg passed through to linker.
        VectorAppend(&linker_args, argv[i]+4);
      } else if (StringEqual(option, "-c")) {
        // Compile only flag.
        compile_only = true;
      } else if (StringEqual(option, "-o")) {
        // -o option is followed by a filename
        if (i == argc-1) {
          fprintf(stderr, "-o needs the name of a file");
          exit(1);
        }
        if (compile_only) {
          // Pass through to compiler.
          VectorAppend(&compiler_args, argv[i]);
          // Get next arg into compiler_args too.
          VectorAppend(&compiler_args, argv[i+1]);
        } else {
          // Linker option
          VectorAppend(&linker_args, argv[i]);
          // Get next arg into linker_args too.
          VectorAppend(&linker_args, argv[i+1]);
        }
        i++;
      } else if (StringEqual(option, "-target")) {
        // -target option is followed by a target name
        if (i == argc-1) {
          fprintf(stderr, "-target needs a target name");
          exit(1);
        }
        // Pass through to compiler.
        VectorAppend(&compiler_args, argv[i]);
        // Get next arg into compiler_args too.
        VectorAppend(&compiler_args, argv[i+1]);
        i++;
      } else if (StringEqual(option, "-origin")) {
        // -origin option is followed by an address
        if (i == argc-1) {
          fprintf(stderr, "-origin needs a value");
          exit(1);
        }
        // Pass through to linker.
        VectorAppend(&linker_args, argv[i]);
        // Get next arg into linker too.
        VectorAppend(&linker_args, argv[i+1]);
        i++;
      } else if (StringEqual(option, "-static")) {
        VectorAppend(&linker_args, argv[i]);
      } else if (StringEqual(option, "-shared")) {
        VectorAppend(&linker_args, argv[i]);
      } else if (StringStartsWith(option, "-l")) {
        VectorAppend(&linker_args, argv[i]);
      } else if (StringStartsWith(option, "-L")) {
        VectorAppend(&linker_args, argv[i]);
      } else {
        VectorAppend(&compiler_args, argv[i]);
      }
    } else {
      String arg;
      StringInit(&arg, argv[i]);
      if (StringEndsWith(&arg, ".c")) {
        VectorAppend(&compiler_args, argv[i]);
        run_compiler = true;
      } else if (StringEndsWith(&arg, ".s")) {
        VectorAppend(&asm_files, NewString(argv[i]));
      } else if (StringEndsWith(&arg, ".o")) {
        VectorAppend(&linker_args, argv[i]);
      }
    }
  }
  
  // Parse compiler options for C and asm files.
  Vector compiler_options;
  if (run_compiler || asm_files.length > 0) {
    ParseOptions((int)compiler_args.length,
                 (char**)compiler_args.value.p,
                 &compiler_options);
  }
  
  // Any C files to compile?
  if (run_compiler) {
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionInputFile) {
        String* object_file = CompileTranslationUnit(opt->value.svalue.value, &compiler_options);
        if (object_file != NULL) {
          VectorAppend(&linker_args, object_file->value);
        }
      }
    }
  }
  
  if (asm_files.length > 0) {
    String object_filename;
    StringInit(&object_filename, NULL);
    String target;
    StringInit(&target, NULL);
    
    // See if we've been given an output filename as a compiler option.
    // Get the target from the options too.
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionOutputFile) {
        StringSet(&object_filename, opt->value.svalue.value);
      } else if (opt->opt == kOptionTarget) {
        StringSet(&target, opt->value.svalue.value);
      }
    }
    
    for (size_t i = 0; i < asm_files.length; i++) {
      String* asm_filename = asm_files.value.p[i];
      String output_filename;
      StringInit(&output_filename, object_filename.value);
      
      if (object_filename.length == 0) {
        StringSet(&output_filename, asm_filename->value);
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
      bool ok = CompilerInitForAssembler(asm_filename->value, &compiler_options);
      if (!ok) {
        fprintf(stderr, "Failed to assemble\n");
        exit(1);
      }
      // Create the global symbol tables.
      CreateGlobalSymbolTables();
      
      Assembler* assembler = NULL;
      void (*asm_run)(Assembler*, String*);
      typedef void (*AssemblerDestructor)(Assembler*);
      AssemblerDestructor destructor;
      
      if (StringEqual(&target, "6502")) {
        assembler = (Assembler*)New6502Assembler(asm_filename, &output_filename);
        asm_run = Assemble6502Instruction;
        destructor = (AssemblerDestructor)_6502AssemblerDestruct;
      } else if (StringEqual(&target, "risc-v")) {
        assembler = (Assembler*)NewRVAssembler(asm_filename, &output_filename);
        asm_run = AssembleRVInstruction;
        destructor = (AssemblerDestructor)RVAssemblerDestruct;
      } else if (StringEqual(&target, "pcode")) {
        assembler = (Assembler*)NewPCodeAssembler(asm_filename, &output_filename);
        asm_run = AssemblePCodeInstruction;
        destructor = (AssemblerDestructor)PCodeAssemblerDestruct;
      } else {
        fprintf(stderr, "Unknown assembler architecture %s\n", target.value);
        exit(1);
      }
      AssemblerRun(assembler, asm_run);
      int num_errors = assembler->num_errors;
      destructor(assembler);
      if (num_errors != 0) {
        exit(1);
      }
      CompilerDelete(compiler);
      String* object_file = NewString(output_filename.value);
      VectorAppend(&object_files, object_file);
      VectorAppend(&linker_args, object_file->value);
    }
  }
  
  if (!compile_only) {
     Link((int)linker_args.length, (char**)linker_args.value.p);
  }
  
  // TODO: tidyup
}
