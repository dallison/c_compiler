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
#include "x86_64_assembler.h"
#include "arm_assembler.h"
#include "compiler.h"
#include "linker_main.h"
#include "module_archive.h"
#include "module_install.h"
#include "options.h"
#include "source.h"
#include "symbol_table.h"

Assembler* NewAARCH64Assembler(String* infile, String* outfile);
void AARCH64AssemblerDestruct(Assembler* assembler);
void AssembleAARCH64Instruction(Assembler* assembler, String* word);

static int ParseArg(int i, int argc, char** argv,
                    Vector* compiler_args,
                    Vector* linker_args,
                    Vector* object_files,
                    Vector* asm_files, Vector* args_from_file,
                    bool* run_compiler, bool* compile_only,
                    bool* read_stdin, int* num_inputs) {
  if (strcmp(argv[i], "-") == 0) {
    // A lone "-" means: read the translation unit from standard input.  It is
    // only valid as the sole input file (enforced by the caller).
    VectorAppend(compiler_args, argv[i]);
    *run_compiler = true;
    *read_stdin = true;
    (*num_inputs)++;
    return i + 1;
  }
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
    StringDelete(option);
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
                     args_from_file, run_compiler, compile_only,
                     read_stdin, num_inputs);
      }
      free(new_argv);
    }
  } else {
    String arg;
    StringInit(&arg, argv[i]);
    if (StringEndsWith(&arg, ".c") ||
        StringEndsWith(&arg, ".cc") ||
        StringEndsWith(&arg, ".cpp") ||
        StringEndsWith(&arg, ".cxx") ||
        StringEndsWith(&arg, ".cppm") ||
        StringEndsWith(&arg, ".ixx")) {
      VectorAppend(compiler_args, argv[i]);
      *run_compiler = true;
      (*num_inputs)++;
    } else if (StringEndsWith(&arg, ".s")) {
      VectorAppend(asm_files, NewString(argv[i]));
      (*num_inputs)++;
    } else if (StringEndsWith(&arg, ".o")) {
      VectorAppend(linker_args, argv[i]);
      (*num_inputs)++;
    } else {
      // Unknown extension, add to linker args.
      VectorAppend(linker_args, argv[i]);
    }
    StringDestruct(&arg);
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

// Collects exported symbols from a global-symbol-table bucket (a BinaryTree of
// SymbolNodes) into the Vector passed as `data`.
static void CollectExportedFromNode(BinaryTreeNode* node, int depth,
                                    void* data) {
  (void)depth;
  Symbol* sym = ((SymbolNode*)node)->symbol;
  if (sym != NULL && sym->flags.is_exported) {
    VectorAppend((Vector*)data, sym);
  }
}

static void CollectExportedFromBucket(void* entry, void* data) {
  BinaryTreeTraverse((BinaryTree*)entry, CollectExportedFromNode, data);
}

// Hidden -Xemit-module hook: compile the front end of `input` and serialize the
// resulting module interface (the global namespace tree plus exported
// file-scope symbols) to a module archive at `module_path`.  Returns true on
// success.
static bool EmitModule(const char* input, Vector* options, Vector* target_opts,
                       const char* module_path) {
  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromFile(compiler, input, options, target_opts)) {
    fprintf(stderr, "Cannot open file %s\n", input);
    return false;
  }
  bool ok = CompileFrontEndOnly(compiler);
  if (ok) {
    Vector ns_roots;
    VectorInit(&ns_roots);
    VectorAppend(&ns_roots, compiler->global_namespace);

    // File-scope C/C++ symbols live in the global hash table (not the
    // namespace tree), so gather the exported ones explicitly.
    Vector root_syms;
    VectorInit(&root_syms);
    HashTableTraverse(&compiler->global_symbol_table, CollectExportedFromBucket,
                      &root_syms);

    // Prefer the module name declared by `[export] module foo;`; otherwise
    // fall back to the input filename.
    const char* module_name = compiler->module_name.length > 0
                                  ? compiler->module_name.value
                                  : input;
    ModuleWriteRequest req = {
        .module_name = module_name,
        .target_triple =
            compiler->target_name != NULL ? compiler->target_name->value : "",
        .compiler_version = "davecc",
        .flags = 0,
        .root_symbols = &root_syms,
        .root_namespaces = &ns_roots,
    };
    ok = ModuleWrite(module_path, &req);
    if (!ok) {
      fprintf(stderr, "Failed to write module %s\n", module_path);
    }
    VectorDestruct(&root_syms);
    VectorDestruct(&ns_roots);
  } else {
    fprintf(stderr, "Front end failed for %s\n", input);
  }
  CompilerDelete(compiler);
  compiler = NULL;
  ClearAllFiles();
  return ok;
}

// Hidden -Xload-module hook: load and verify a module archive, printing a short
// summary.  Requires a compiler global for the deserialized objects' arenas.
static bool LoadModule(const char* module_path, Vector* options) {
  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromString(compiler, module_path, "", options)) {
    return false;
  }
  CreateGlobalSymbolTables();

  LoadedModule loaded;
  bool ok = ModuleLoad(module_path, &loaded);
  if (ok) {
    printf("module %s: format v%u, target %s, %zu root symbol(s), "
           "%zu root namespace(s)\n",
           loaded.module_name.value, loaded.format_version,
           loaded.target_triple.value, loaded.root_symbols.length,
           loaded.root_namespaces.length);
    LoadedModuleDestruct(&loaded);
  } else {
    fprintf(stderr, "Failed to load module %s\n", module_path);
  }
  CompilerDelete(compiler);
  compiler = NULL;
  ClearAllFiles();
  return ok;
}

// C++20 module import support for normal compilation.  The compiler front end
// invokes the registered handler (via CompilerImportModule) when it parses an
// `import foo;` directive; we resolve `<foo>.dcm` across the prebuilt-module
// search paths, load it, and install its exported names into the active
// compiler's symbol tables.
typedef struct {
  Vector search_paths;  // const char* directories (borrowed from options).
  Vector loaded;        // LoadedModule* loaded during compilation (owned).
} DriverImportState;

static bool FileExists(const char* path) {
  FILE* f = fopen(path, "r");
  if (f == NULL) {
    return false;
  }
  fclose(f);
  return true;
}

static bool DriverImportModule(void* ctx, const char* module_name) {
  DriverImportState* state = (DriverImportState*)ctx;
  char path[4096];
  const char* found = NULL;
  for (size_t i = 0; i < state->search_paths.length && found == NULL; i++) {
    const char* dir = (const char*)VectorGet(&state->search_paths, i);
    snprintf(path, sizeof(path), "%s/%s.dcm", dir, module_name);
    if (FileExists(path)) {
      found = path;
    }
  }
  if (found == NULL) {
    // Fall back to the current directory.
    snprintf(path, sizeof(path), "%s.dcm", module_name);
    if (FileExists(path)) {
      found = path;
    }
  }
  if (found == NULL) {
    return false;
  }

  LoadedModule* m = calloc(1, sizeof(LoadedModule));
  if (!ModuleLoad(found, m)) {
    free(m);
    return false;
  }
  bool ok = ModuleInstallLoaded(m);
  VectorAppend(&state->loaded, m);  // Keep alive for the rest of the compile.
  return ok;
}

static void DriverImportStateInit(DriverImportState* state, Vector* options) {
  VectorInit(&state->search_paths);
  VectorInit(&state->loaded);
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* opt = options->value.p[i];
    if (opt->opt == kOptionPrebuiltModulePath) {
      VectorAppend(&state->search_paths, opt->value.svalue.value);
    }
  }
}

static void DriverImportStateDestruct(DriverImportState* state) {
  for (size_t i = 0; i < state->loaded.length; i++) {
    LoadedModule* m = (LoadedModule*)VectorGet(&state->loaded, i);
    LoadedModuleDestruct(m);
    free(m);
  }
  VectorDestruct(&state->loaded);
  VectorDestruct(&state->search_paths);
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
  bool read_stdin = false;
  int num_inputs = 0;
  
  int i = 1;
  bool help = false;
  while (i < argc) {
    if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
      help = true;
      break;
    }
    i = ParseArg(i, argc, argv, &compiler_args, &linker_args, &object_files,
                 &asm_files, &args_from_file, &run_compiler, &compile_only,
                 &read_stdin, &num_inputs);
  }
  
  if (help) {
    PrintCompilerHelp();
    exit(0);
  }

  if (read_stdin && num_inputs > 1) {
    fprintf(stderr,
            "'-' (standard input) must be the only input file\n");
    exit(1);
  }
  // Parse compiler options for C and asm files.
  Vector compiler_options;
  VectorInit(&compiler_options);
  Vector* target_opts = NULL;
  // Parse compiler options whenever any were supplied (compiler_args always
  // holds argv[0]); this also covers standalone module hooks like
  // -Xload-module that have no C input file.
  if (run_compiler || asm_files.length > 0 || compiler_args.length > 1) {
    target_opts = ParseOptions((int)compiler_args.length,
                 (char**)compiler_args.value.p,
                 &compiler_options);
  }
  
  // Hidden C++20-module hooks.  -Xemit-module compiles the front end and writes
  // a module instead of an object file; -Xload-module loads and verifies one.
  String* emit_module = OptionStringValue(kOptionEmitModule, &compiler_options);
  String* load_module = OptionStringValue(kOptionLoadModule, &compiler_options);
  if (emit_module != NULL) {
    bool ok = true;
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionInputFile) {
        ok = EmitModule(opt->value.svalue.value, &compiler_options, target_opts,
                        emit_module->value) &&
             ok;
      }
    }
    exit(ok ? 0 : 1);
  }
  if (load_module != NULL) {
    exit(LoadModule(load_module->value, &compiler_options) ? 0 : 1);
  }

  // Any C files to compile?
  DriverImportState import_state;
  if (run_compiler) {
    // Register the module import hook so `import foo;` resolves and installs a
    // prebuilt module during parsing.
    DriverImportStateInit(&import_state, &compiler_options);
    SetModuleImportHandler(DriverImportModule, &import_state);

    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionInputFile) {
        String* object_file = CompileTranslationUnit(opt->value.svalue.value, &compiler_options, target_opts);
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

    SetModuleImportHandler(NULL, NULL);
    DriverImportStateDestruct(&import_state);
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
      } else if (StringEqual(&target, "riscv") || StringEqual(&target, "risc-v")) {
        assembler = (Assembler*)NewRVAssembler(asm_filename, &output_filename);
        asm_run = AssembleRVInstruction;
        destructor = (AssemblerDestructor)RVAssemblerDestruct;
      } else if (StringEqual(&target, "aarch64")) {
        assembler = NewAARCH64Assembler(asm_filename, &output_filename);
        asm_run = AssembleAARCH64Instruction;
        destructor = (AssemblerDestructor)AARCH64AssemblerDestruct;
      } else if (StringEqual(&target, "arm") || StringEqual(&target, "armv7") ||
                 StringEqual(&target, "armv7-a") || StringEqual(&target, "arm32")) {
        assembler = (Assembler*)NewARMAssembler(asm_filename, &output_filename);
        asm_run = AssembleARMInstruction;
        destructor = (AssemblerDestructor)ARMAssemblerDestruct;
      } else if (StringEqual(&target, "x86_64") || StringEqual(&target, "x86-64")) {
        assembler = (Assembler*)NewX86_64Assembler(asm_filename, &output_filename);
        asm_run = AssembleX86_64Instruction;
        destructor = (AssemblerDestructor)X86_64AssemblerDestruct;
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


