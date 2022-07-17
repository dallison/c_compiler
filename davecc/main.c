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

static int ParseArg(int i, int argc, char** argv,
                    Vector* compiler_args,
                    Vector* linker_args,
                    Vector* object_files,
                    Vector* asm_files, Vector* args_from_file,
                    bool* run_compiler, bool* compile_only) {
  if (argv[i][0] == '-') {
    // Option.
    String* option = NewString(argv[i]);
    if (StringStartsWith(option, "-Wl,")) {
      // Arg passed through to linker.
      VectorAppend(linker_args, argv[i]+4);
    } else if (StringEqual(option, "-c") || StringEqual(option, "-S")) {
      // Compile only flag.
      *compile_only = true;
      VectorAppend(compiler_args, argv[i]);
    } else if (StringEqual(option, "-o")) {
      // -o option is followed by a filename
      if (i == argc-1) {
        fprintf(stderr, "-o needs the name of a file");
        exit(1);
      }
      if (*compile_only) {
        // Pass through to compiler.
        VectorAppend(compiler_args, argv[i]);
        // Get next arg into compiler_args too.
        VectorAppend(compiler_args, argv[i+1]);
      } else {
        // Linker option
        VectorAppend(linker_args, argv[i]);
        // Get next arg into linker_args too.
        VectorAppend(linker_args, argv[i+1]);
      }
      i++;
    } else if (StringEqual(option, "-target")) {
      // -target option is followed by a target name
      if (i == argc-1) {
        fprintf(stderr, "-target needs a target name");
        exit(1);
      }
      // Pass through to compiler.
      VectorAppend(compiler_args, argv[i]);
      // Get next arg into compiler_args too.
      VectorAppend(compiler_args, argv[i+1]);
      i++;
    } else if (StringEqual(option, "-isystem")) {
      // -isystem option is followed by an include dir
      if (i == argc-1) {
        fprintf(stderr, "-isystem needs a directory");
        exit(1);
      }
      // Pass through to compiler.
      VectorAppend(compiler_args, argv[i]);
      // Get next arg into compiler_args too.
      VectorAppend(compiler_args, argv[i+1]);
        i++;
    } else if (StringEqual(option, "-rpath")) {
      // -rpath option is followed by an include dir
      if (i == argc-1) {
        fprintf(stderr, "-rpath needs a directory");
        exit(1);
      }
      // Pass through to linker.
      VectorAppend(linker_args, argv[i]);
      // Get next arg into linker_args too.
      VectorAppend(linker_args, argv[i+1]);
      i++;
    } else if (StringEqual(option, "-chdir")) {
      // -chdir option is followed by an include dir
      if (i == argc-1) {
        fprintf(stderr, "-chdir needs a directory");
        exit(1);
      }
      // Pass through to compiler and linker.
      VectorAppend(compiler_args, argv[i]);
      VectorAppend(linker_args, argv[i]);
      // Get next arg into compiler_args too.
      VectorAppend(compiler_args, argv[i+1]);
      VectorAppend(linker_args, argv[i+1]);
      i++;
          
    } else if (StringEqual(option, "-origin")) {
      // -origin option is followed by an address
      if (i == argc-1) {
        fprintf(stderr, "-origin needs a value");
        exit(1);
      }
      // Pass through to linker.
      VectorAppend(linker_args, argv[i]);
      // Get next arg into linker too.
      VectorAppend(linker_args, argv[i+1]);
      i++;
    } else if (StringEqual(option, "-static")) {
      VectorAppend(linker_args, argv[i]);
    } else if (StringEqual(option, "-shared")) {
      VectorAppend(linker_args, argv[i]);
    } else if (StringStartsWith(option, "-l")) {
      VectorAppend(linker_args, argv[i]);
    } else if (StringStartsWith(option, "-L")) {
      VectorAppend(linker_args, argv[i]);
    } else {
      VectorAppend(compiler_args, argv[i]);
    }
  } else if (argv[i][0] == '@') {
    char* arg = &argv[i][1];
    // Args from file.
    FILE* fp = fopen(arg, "r");
    if (fp == NULL) {
      fprintf(stderr, "Unable to open compiler args file %s\n", argv[i]);
    } else {
      char buf[1024];
      while (fgets(buf, sizeof(buf), fp) != NULL) {
        String s;
        StringInit(&s, buf);
        StringTrim(&s);     // Contains newline.
        if (s.length == 0) {
          continue;
        }
        if (s.value[0] == '#') {
          continue;
        }
        // Split string into parts separated by space.  Each element
        // of the vector will be a String pointer which will be added
        // to the args_from_file vector.
        Vector parts = {0};
        StringSplit(&s, ' ', &parts);
        for (size_t i = 0; i < parts.length; i++) {
          String* part = parts.value.p[i];
          while (StringEndsWith(part, "\\")) {
            // If it ends in \ then append next part.
            StringReplace(part, part->length - 1, 1, "", 0);
            StringAppend(part, " ");
            i++;
            String* tail = parts.value.p[i];
            StringAppend(part, tail->value);
          }
          VectorAppend(args_from_file, part);
        }
        StringDestruct(&s);
        VectorDestruct(&parts);
      }
      fclose(fp);
      
      // Build a new argv vector pointing to the strings in args_from_file.
      int new_argc = (int)args_from_file->length;
      char** new_argv = malloc(sizeof(char*) * new_argc);
      char** p = new_argv;
      for (size_t i = 0; i < new_argc; i++) {
        String* s = args_from_file->value.p[i];
        *p++ = s->value;
      }
      int j = 0;
      while (j < new_argc) {
        j = ParseArg(j,
                     new_argc, new_argv, compiler_args,
                     linker_args, object_files, asm_files,
                     args_from_file, run_compiler, compile_only);
      }
      free(new_argv);
    }
  } else {
    String arg;
    StringInit(&arg, argv[i]);
    if (StringEndsWith(&arg, ".c")) {
      VectorAppend(compiler_args, argv[i]);
      *run_compiler = true;
    } else if (StringEndsWith(&arg, ".s")) {
      VectorAppend(asm_files, NewString(argv[i]));
    } else if (StringEndsWith(&arg, ".o")) {
      VectorAppend(linker_args, argv[i]);
    } else {
      // Unknown extension, add to linker args.
      VectorAppend(linker_args, argv[i]);
    }
  }
  return i + 1;
}

static bool BoolOptionValue(Vector* options, int opt, bool def) {
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* option = options->value.p[i];
    if (option->opt == opt) {
      return option->value.bvalue;
    }
  }
  return def;
}

int main(int argc, char * argv[]) {
  Vector asm_files = {0};
  Vector compiler_args = {0};
  Vector linker_args = {0};
  Vector object_files = {0};

  VectorAppend(&compiler_args, "");   // argv[0]
  VectorAppend(&linker_args, "");   // argv[0]

  // Storage for strings read from @file.
  Vector args_from_file = {0};
  
  bool compile_only = false;
  bool run_compiler = false;
  
  int i = 1;
  while (i < argc) {
    i = ParseArg(i, argc, argv, &compiler_args, &linker_args, &object_files,
                 &asm_files, &args_from_file, &run_compiler, &compile_only);
  }
  
  // Parse compiler options for C and asm files.
  Vector compiler_options;
  VectorInit(&compiler_options);
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
        } else {
          // If -S was specified we won't have an output file.
          if (!BoolOptionValue(&compiler_options, kOptionAssemblyOutput, false)) {
            fprintf(stderr, "Failed to compile\n");
            exit(1);
          }
        }
      }
    }
  }
  
  if (asm_files.length > 0) {
    String object_filename = {0};
    String target = {0};
    
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
      typedef void (*AssemblerFinalizer)(Assembler*);
      AssemblerFinalizer finalizer = NULL;
      AssemblerDestructor destructor;

      if (StringEqual(&target, "6502") || StringEqual(&target, "65c02")) {
        assembler = (Assembler*)New6502Assembler(asm_filename, &output_filename);
        asm_run = Assemble6502Instruction;
        destructor = (AssemblerDestructor)W65C02AssemblerDestruct;
        finalizer = (AssemblerFinalizer)W65C02AssemblerFinalize;
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
      PreprocessorCopyOptions(&assembler->preprocessor, &compiler->preprocessor);
      AssemblerRun(assembler, asm_run);
      if (finalizer != NULL) {
        finalizer(assembler);
      }
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
  
  int status = 0;
  if (!compile_only) {
    String* output = Link((int)linker_args.length, (char**)linker_args.value.p);
    if (output == NULL) {
      status = 1;
    } else {
      StringDelete(output);
    }
  }
  
  // TODO: tidyup
  exit(status);
}


