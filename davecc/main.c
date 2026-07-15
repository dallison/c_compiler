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
#include "errors.h"
#include "linker_main.h"
#include "module_archive.h"
#include "module_import.h"
#include "module_install.h"
#include "module_reachability.h"
#include "options.h"
#include "preprocessor.h"
#include "source.h"
#include "symbol_table.h"

Assembler* NewAARCH64Assembler(String* infile, String* outfile);
void AARCH64AssemblerDestruct(Assembler* assembler);
void AssembleAARCH64Instruction(Assembler* assembler, String* word);
static bool DriverImportModule(void* ctx, const char* module_name);

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
    } else if (StringEqual(option, "-Xemit-module") ||
               StringEqual(option, "-Xload-module") ||
               StringEqual(option, "-fprebuilt-module-path") ||
               StringEqual(option, "-fmodule-file") ||
               StringEqual(option, "-fmodule-name") ||
               StringEqual(option, "-fmodule-output") ||
               StringEqual(option, "-fdeps-file") ||
               StringEqual(option, "-fdeps-format")) {
      // These C++20-module options are followed by a path.  The path does not
      // carry a recognized source extension, so route both the flag and its
      // value into compiler_args explicitly; otherwise the path token falls
      // through to the linker and the option ends up swallowing the following
      // source file as its value.
      if (i == argc-1) {
        fprintf(stderr, "%s needs a module path\n", option->value);
        exit(1);
      }
      VectorAppend(compiler_args, argv[i]);
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
        StringEndsWith(&arg, ".ixx") ||
        StringEndsWith(&arg, ".h") ||
        StringEndsWith(&arg, ".hpp") ||
        StringEndsWith(&arg, ".hxx")) {
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

static const char* StringOptionValue(Vector* options, int opt) {
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* option = options->value.p[i];
    if (option->opt == opt) {
      return option->value.svalue.value;
    }
  }
  return NULL;
}

static void AppendUniqueModuleId(Vector* names, const ModuleId* id) {
  String formatted;
  StringInit(&formatted, "");
  ModuleIdFormat(id, &formatted);
  for (size_t i = 0; i < names->length; i++) {
    String* existing = (String*)VectorGet(names, i);
    if (StringEqualString(existing, &formatted)) {
      StringDestruct(&formatted);
      return;
    }
  }
  VectorAppend(names, NewString(formatted.value));
  StringDestruct(&formatted);
}

typedef struct {
  String provided;
  String primary_module;
  Vector required;  // owned String*
  bool provides;
  bool is_interface;
} ModuleDependencyScan;

static void ModuleDependencyScanInit(ModuleDependencyScan* scan) {
  StringInit(&scan->provided, "");
  StringInit(&scan->primary_module, "");
  VectorInit(&scan->required);
  scan->provides = false;
  scan->is_interface = false;
}

static void ModuleDependencyScanDestruct(ModuleDependencyScan* scan) {
  StringDestruct(&scan->provided);
  StringDestruct(&scan->primary_module);
  VectorDestructWithContents(&scan->required,
                             (VectorElementDestructor)StringDelete,
                             /*free_element=*/false);
}

static bool LexAtContextualKeyword(Lex* lex, const char* spelling) {
  return LexLookingAt(lex, TOK(identifier)) &&
         StringEqual(&lex->spelling, spelling);
}

static void AppendUniqueModuleName(Vector* names, const char* name) {
  for (size_t i = 0; i < names->length; i++) {
    if (StringEqual((String*)VectorGet(names, i), name)) {
      return;
    }
  }
  VectorAppend(names, NewString(name));
}

static bool ScanDottedModuleName(Lex* lex, String* out) {
  if (!LexLookingAt(lex, TOK(identifier))) {
    return false;
  }
  StringAppendString(out, &lex->spelling);
  LexNextToken(lex);
  while (LexMatch(lex, TOK(dot))) {
    if (!LexLookingAt(lex, TOK(identifier))) {
      return false;
    }
    StringAppendChar(out, '.');
    StringAppendString(out, &lex->spelling);
    LexNextToken(lex);
  }
  return true;
}

static void AppendScannedHeaderToken(Lex* lex, String* out) {
  if (LexLookingAt(lex, TOK(identifier))) {
    StringAppendString(out, &lex->spelling);
  } else if (LexLookingAt(lex, TOK(number)) ||
             LexLookingAt(lex, TOK(fnumber))) {
    StringAppendString(out, &lex->literal_spelling);
  } else {
    StringAppend(out, TokenName(lex->current_token));
  }
}

static bool ScanModuleName(Lex* lex, String* out, bool allow_header_name,
                           bool* relative_partition) {
  StringInit(out, "");
  if (relative_partition != NULL) {
    *relative_partition = false;
  }
  if (allow_header_name && LexLookingAt(lex, TOK(string))) {
    StringAppendChar(out, '"');
    StringAppendString(out, &lex->spelling);
    StringAppendChar(out, '"');
    LexNextToken(lex);
    return true;
  }
  if (allow_header_name && LexMatch(lex, TOK(less))) {
    StringAppendChar(out, '<');
    while (!LexEof(lex) && !LexLookingAt(lex, TOK(greater))) {
      AppendScannedHeaderToken(lex, out);
      LexNextToken(lex);
    }
    if (!LexMatch(lex, TOK(greater))) {
      return false;
    }
    StringAppendChar(out, '>');
    return true;
  }
  if (LexMatch(lex, TOK(colon))) {
    if (relative_partition != NULL) {
      *relative_partition = true;
    }
    StringAppendChar(out, ':');
    if (!LexLookingAt(lex, TOK(identifier))) {
      return false;
    }
    StringAppendString(out, &lex->spelling);
    LexNextToken(lex);
    return true;
  }
  if (!ScanDottedModuleName(lex, out)) {
    return false;
  }
  if (LexMatch(lex, TOK(colon))) {
    if (!LexLookingAt(lex, TOK(identifier))) {
      return false;
    }
    StringAppendChar(out, ':');
    StringAppendString(out, &lex->spelling);
    LexNextToken(lex);
  }
  return true;
}

static void SetPrimaryModuleName(ModuleDependencyScan* scan,
                                 const String* module_name) {
  const char* colon = strchr(module_name->value, ':');
  if (colon == NULL) {
    StringSetString(&scan->primary_module, (String*)module_name);
    return;
  }
  StringClear(&scan->primary_module);
  StringAppendSegment(&scan->primary_module, module_name->value,
                      (size_t)(colon - module_name->value));
}

static bool ScanModuleDependencies(const char* input, Vector* options,
                                   Vector* target_opts,
                                   ModuleDependencyScan* scan) {
  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromFile(compiler, input, options, target_opts)) {
    fprintf(stderr, "Cannot open file %s\n", input);
    free(compiler);
    compiler = NULL;
    ClearAllFiles();
    return false;
  }

  bool header_unit = BoolOptionValue(options, kOptionModuleHeader, false);
  const char* requested_name = StringOptionValue(options, kOptionModuleName);
  if (header_unit) {
    if (requested_name == NULL || requested_name[0] == '\0') {
      fprintf(stderr,
              "-fmodule-header requires -fmodule-name with the header name\n");
      CompilerDelete(compiler);
      compiler = NULL;
      ClearAllFiles();
      return false;
    }
    StringSet(&scan->provided, requested_name);
    scan->provides = true;
    scan->is_interface = true;
  }

  Lex* lex = &compiler->lex;
  LexNextToken(lex);
  while (!LexEof(lex)) {
    bool exported = false;
    if (LexLookingAt(lex, TOK(export))) {
      exported = true;
      LexNextToken(lex);
    }

    if (LexAtContextualKeyword(lex, "module")) {
      LexNextToken(lex);
      if (LexMatch(lex, TOK(semicolon))) {
        continue;  // Global module fragment.
      }
      if (LexLookingAt(lex, TOK(colon))) {
        LexNextToken(lex);
        if (LexLookingAt(lex, TOK(private))) {
          LexNextToken(lex);
          LexMatch(lex, TOK(semicolon));
          continue;
        }
      }
      String name;
      bool relative = false;
      if (ScanModuleName(lex, &name, false, &relative) &&
          LexMatch(lex, TOK(semicolon))) {
        bool partition = strchr(name.value, ':') != NULL;
        StringSetString(&scan->provided, &name);
        SetPrimaryModuleName(scan, &name);
        scan->provides = exported || partition;
        scan->is_interface = exported;
        if (!exported && !partition) {
          AppendUniqueModuleName(&scan->required, name.value);
        }
      }
      StringDestruct(&name);
      continue;
    }

    if (LexAtContextualKeyword(lex, "import")) {
      LexNextToken(lex);
      String name;
      bool relative = false;
      if (ScanModuleName(lex, &name, true, &relative) &&
          LexMatch(lex, TOK(semicolon))) {
        if (relative && scan->primary_module.length > 0) {
          String resolved;
          StringInit(&resolved, scan->primary_module.value);
          StringAppendString(&resolved, &name);
          AppendUniqueModuleName(&scan->required, resolved.value);
          StringDestruct(&resolved);
        } else {
          AppendUniqueModuleName(&scan->required, name.value);
        }
      }
      StringDestruct(&name);
      continue;
    }

    if (!exported) {
      LexNextToken(lex);
    }
  }

  bool ok = NumErrors() == 0;
  CompilerDelete(compiler);
  compiler = NULL;
  ClearAllFiles();
  return ok;
}

static void WriteJsonString(FILE* out, const char* value) {
  fputc('"', out);
  for (const unsigned char* p = (const unsigned char*)value; *p != '\0'; p++) {
    switch (*p) {
      case '"':
        fputs("\\\"", out);
        break;
      case '\\':
        fputs("\\\\", out);
        break;
      case '\n':
        fputs("\\n", out);
        break;
      case '\r':
        fputs("\\r", out);
        break;
      case '\t':
        fputs("\\t", out);
        break;
      default:
        if (*p < 0x20) {
          fprintf(out, "\\u%04x", *p);
        } else {
          fputc(*p, out);
        }
        break;
    }
  }
  fputc('"', out);
}

static void DefaultObjectPath(const char* input, String* output) {
  StringInit(output, input);
  char* slash = strrchr(output->value, '/');
  char* dot = strrchr(output->value, '.');
  if (dot != NULL && (slash == NULL || dot > slash)) {
    StringReplace(output, (size_t)(dot - output->value),
                  output->length - (size_t)(dot - output->value), ".o", 2);
  } else {
    StringAppend(output, ".o");
  }
}

static bool WriteModuleDependencies(const char* path, const char* input,
                                    const char* primary_output,
                                    const ModuleDependencyScan* scan) {
  FILE* out = fopen(path, "w");
  if (out == NULL) {
    fprintf(stderr, "Cannot write dependency file %s\n", path);
    return false;
  }
  fputs("{\n  \"version\": 1,\n  \"revision\": 0,\n  \"rules\": [\n"
        "    {\n      \"primary-output\": ",
        out);
  WriteJsonString(out, primary_output);
  if (scan->provides) {
    fputs(",\n      \"provides\": [\n        {\n"
          "          \"logical-name\": ",
          out);
    WriteJsonString(out, scan->provided.value);
    fputs(",\n          \"source-path\": ", out);
    WriteJsonString(out, input);
    fprintf(out, ",\n          \"is-interface\": %s\n        }\n      ]",
            scan->is_interface ? "true" : "false");
  }
  if (scan->required.length > 0) {
    fputs(",\n      \"requires\": [\n", out);
    for (size_t i = 0; i < scan->required.length; i++) {
      fputs("        { \"logical-name\": ", out);
      WriteJsonString(out,
                      ((String*)VectorGet((Vector*)&scan->required, i))->value);
      fputs(i + 1 == scan->required.length ? " }\n" : " },\n", out);
    }
    fputs("      ]", out);
  }
  fputs("\n    }\n  ]\n}\n", out);
  bool ok = fclose(out) == 0;
  if (!ok) {
    fprintf(stderr, "Failed to finish dependency file %s\n", path);
  }
  return ok;
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
    free(compiler);
    compiler = NULL;
    ClearAllFiles();
    return false;
  }
  TranslationUnitImportState* import_state =
      TranslationUnitImportStateCreate(options);
  CompilerSetImportState(
      import_state,
      (TranslationUnitImportReleaseFn)TranslationUnitImportStateRelease);
  SetModuleImportHandler(DriverImportModule, import_state);
  bool header_unit =
      BoolOptionValue(options, kOptionModuleHeader, false);
  if (header_unit) {
    compiler->syntax.export_depth = 1;
  }
  bool ok = CompileFrontEndOnly(compiler);
  if (ok) {
    if (!header_unit &&
        !ModuleUnitIsInterfaceUnit(&compiler->module_unit) &&
        compiler->module_unit.kind != kModuleUnitKindInternalPartition) {
      fprintf(stderr,
              "Cannot emit module artifact: input is not an importable module "
              "unit\n");
      ok = false;
    }
  }
  if (ok) {
    ModuleReachability reachability;
    ModuleReachabilityInit(&reachability);
    if (!ModuleReachabilityBuild(&reachability, header_unit)) {
      fprintf(stderr, "Cannot emit module interface: %s\n",
              reachability.error);
      ok = false;
    }

    Vector ns_roots;
    VectorInit(&ns_roots);
    VectorAppend(&ns_roots, compiler->global_namespace);

    Vector dependencies;
    Vector reexports;
    Vector header_macros;
    VectorInit(&dependencies);
    VectorInit(&reexports);
    VectorInit(&header_macros);
    for (size_t i = 0; i < compiler->module_unit.imports.length; i++) {
      ModuleImportRef* import =
          (ModuleImportRef*)VectorGet(&compiler->module_unit.imports, i);
      AppendUniqueModuleId(&dependencies, &import->id);
      if (import->is_export_import) {
        AppendUniqueModuleId(&reexports, &import->id);
      }
    }

    String module_name;
    StringInit(&module_name, input);
    const char* requested_module_name =
        StringOptionValue(options, kOptionModuleName);
    if (requested_module_name != NULL) {
      StringSet(&module_name, requested_module_name);
    } else if (compiler->module_unit.id.name.length > 0) {
      StringClear(&module_name);
      ModuleIdFormat(&compiler->module_unit.id, &module_name);
    }
    if (header_unit && compiler->syntax.lex != NULL &&
        compiler->syntax.lex->preprocessor != NULL) {
      PreprocessorCollectHeaderUnitMacros(
          compiler->syntax.lex->preprocessor, &header_macros);
    }
    ModuleWriteRequest req = {
        .module_name = module_name.value,
        .target_triple =
            compiler->target_name != NULL ? compiler->target_name->value : "",
        .compiler_version = "davecc",
        .flags = header_unit
            ? kModuleArchiveHeaderUnit
            : compiler->module_unit.kind == kModuleUnitKindPrimaryInterface
                ? kModuleArchivePrimaryInterface
                : compiler->module_unit.kind ==
                          kModuleUnitKindInterfacePartition
                      ? kModuleArchiveInterfacePartition
                      : kModuleArchiveInternalPartition,
        .root_symbols = &reachability.exported_roots,
        .root_namespaces = &ns_roots,
        .dependencies = &dependencies,
        .reexports = &reexports,
        .header_macros = &header_macros,
    };
    if (ok) {
      ok = ModuleWrite(module_path, &req);
    }
    if (!ok) {
      const char* detail = ModuleWriteLastError();
      if (detail != NULL) {
        fprintf(stderr, "Failed to write module %s: %s\n", module_path,
                detail);
      } else {
        fprintf(stderr, "Failed to write module %s\n", module_path);
      }
    }
    VectorDestruct(&ns_roots);
    VectorDestructWithContents(
        &dependencies, (VectorElementDestructor)StringDelete,
        /*free_element=*/false);
    VectorDestructWithContents(&reexports,
                               (VectorElementDestructor)StringDelete,
                               /*free_element=*/false);
    VectorDestruct(&header_macros);
    StringDestruct(&module_name);
    ModuleReachabilityDestruct(&reachability);
  } else {
    fprintf(stderr, "Front end failed for %s\n", input);
  }
  CompilerDelete(compiler);
  compiler = NULL;
  SetModuleImportHandler(NULL, NULL);
  CompilerSetImportState(NULL, NULL);
  TranslationUnitImportStateDelete(import_state);
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
    LoadedModuleReleaseGraph(&loaded);
  } else {
    fprintf(stderr, "Failed to load module %s\n", module_path);
  }
  CompilerDelete(compiler);
  compiler = NULL;
  if (ok) {
    LoadedModuleDestruct(&loaded);
  }
  ClearAllFiles();
  return ok;
}

// C++20 module import support for normal compilation.  Each translation unit
// gets its own TranslationUnitImportState so loaded module graphs are never
// reused after CompilerDelete.
static bool DriverImportModule(void* ctx, const char* module_name) {
  TranslationUnitImportState* state = (TranslationUnitImportState*)ctx;
  bool ok = TranslationUnitImportStateImport(state, module_name);
  if (!ok) {
    CompilerSetLastImportError(TranslationUnitImportStateLastError(state));
  }
  return ok;
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
  
  String* deps_file = OptionStringValue(kOptionDepsFile, &compiler_options);
  String* deps_format = OptionStringValue(kOptionDepsFormat, &compiler_options);
  bool deps_scan_only =
      BoolOptionValue(&compiler_options, kOptionDepsScanOnly, false);
  if (deps_format != NULL && !StringEqual(deps_format, "p1689r5")) {
    fprintf(stderr, "Unsupported dependency format '%s'; use p1689r5\n",
            deps_format->value);
    exit(1);
  }
  if (deps_scan_only && deps_file == NULL) {
    fprintf(stderr, "-fdeps-scan-only requires -fdeps-file\n");
    exit(1);
  }
  if (deps_file != NULL) {
    const char* scan_input = NULL;
    size_t input_count = 0;
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionInputFile) {
        scan_input = opt->value.svalue.value;
        input_count++;
      }
    }
    if (input_count != 1) {
      fprintf(stderr, "-fdeps-file requires exactly one source input\n");
      exit(1);
    }
    ModuleDependencyScan scan;
    ModuleDependencyScanInit(&scan);
    bool ok =
        ScanModuleDependencies(scan_input, &compiler_options, target_opts, &scan);
    String default_output;
    String* output = OptionStringValue(kOptionOutputFile, &compiler_options);
    if (output == NULL) {
      DefaultObjectPath(scan_input, &default_output);
      output = &default_output;
    }
    if (ok) {
      ok = WriteModuleDependencies(deps_file->value, scan_input, output->value,
                                   &scan);
    }
    if (OptionStringValue(kOptionOutputFile, &compiler_options) == NULL) {
      StringDestruct(&default_output);
    }
    ModuleDependencyScanDestruct(&scan);
    if (!ok || deps_scan_only) {
      exit(ok ? 0 : 1);
    }
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

  // Public coordinated mode: emit the module artifact and then continue with
  // normal object generation in the same driver invocation.
  String* module_output =
      OptionStringValue(kOptionModuleOutput, &compiler_options);
  if (module_output != NULL) {
    const char* module_input = NULL;
    size_t input_count = 0;
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionInputFile) {
        module_input = opt->value.svalue.value;
        input_count++;
      }
    }
    if (input_count != 1) {
      fprintf(stderr, "-fmodule-output requires exactly one source input\n");
      exit(1);
    }
    if (!EmitModule(module_input, &compiler_options, target_opts,
                    module_output->value)) {
      exit(1);
    }
  }

  // Any C files to compile?
  if (run_compiler) {
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionInputFile) {
        // One import store per translation unit: LoadedModule graphs must stay
        // alive through that compile and be released before CompilerDelete.
        TranslationUnitImportState* import_state =
            TranslationUnitImportStateCreate(&compiler_options);
        CompilerSetImportState(import_state,
                               (TranslationUnitImportReleaseFn)
                                   TranslationUnitImportStateRelease);
        SetModuleImportHandler(DriverImportModule, import_state);

        String* object_file = CompileTranslationUnit(opt->value.svalue.value,
                                                     &compiler_options,
                                                     target_opts);
        SetModuleImportHandler(NULL, NULL);
        CompilerSetImportState(NULL, NULL);
        TranslationUnitImportStateDelete(import_state);

        if (object_file != NULL) {
          VectorAppend(&linker_args, object_file->value);
        } else {
          fprintf(stderr, "Failed to compile\n");
          exit(1);
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


