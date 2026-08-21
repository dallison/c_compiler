//
//  module_import_test.c
//  c_compiler
//
//  Focused tests for per-translation-unit module import ownership, duplicate
//  import idempotency, archive metadata validation, and retry-stable failures.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "module_archive.h"
#include "module_import.h"
#include "module_install.h"
#include "options.h"
#include "symbol.h"
#include "symbol_table.h"
#include "type.h"
#include "vector.h"

static int g_failures = 0;

#define CHECK(cond)                                                   \
  do {                                                                \
    if (!(cond)) {                                                    \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      g_failures++;                                                   \
    }                                                                 \
  } while (0)

static void NewCompiler(Vector* options) {
  compiler = malloc(sizeof(Compiler));
  CompilerInitFromString(compiler, "module_import_test", "", options);
  CreateGlobalSymbolTables();
}

static Symbol* MakeFn(const char* name, TypeRecord* int_type) {
  TypeRecord* ft = NewFunctionTypeRecord();
  TypeRecordChain(ft, int_type);
  ft->info.function.definition = true;
  Symbol* s = NewSymbol(name, ft, STO(implicit));
  s->flags.is_defined = true;
  s->flags.is_exported = true;
  ft->info.function.symbol = s;
  return s;
}

static bool WriteModuleWithDependencies(const char* path,
                                        const char* module_name,
                                        const char* target,
                                        Symbol* exported_fn,
                                        Vector* dependencies) {
  Vector roots;
  VectorInit(&roots);
  VectorAppend(&roots, exported_fn);
  Vector ns_roots;
  VectorInit(&ns_roots);
  VectorAppend(&ns_roots, compiler->global_namespace);

  ModuleWriteRequest req = {
      .module_name = module_name,
      .target_triple = target,
      .compiler_version = "davecc-test",
      .flags = 0,
      .root_symbols = &roots,
      .root_namespaces = &ns_roots,
      .dependencies = dependencies,
  };
  bool ok = ModuleWrite(path, &req);
  VectorDestruct(&roots);
  VectorDestruct(&ns_roots);
  return ok;
}

static bool WriteModule(const char* path, const char* module_name,
                        const char* target, Symbol* exported_fn) {
  return WriteModuleWithDependencies(path, module_name, target, exported_fn,
                                     NULL);
}

static void AddSearchPath(Vector* options, const char* dir) {
  CompilerOptionValue* opt = calloc(1, sizeof(CompilerOptionValue));
  opt->opt = kOptionPrebuiltModulePath;
  StringInit(&opt->value.svalue, dir);
  VectorAppend(options, opt);
}

int main(void) {
  Vector options;
  VectorInit(&options);
  CompilerOptionValue* target = calloc(1, sizeof(CompilerOptionValue));
  target->opt = kOptionTarget;
  StringInit(&target->value.svalue, "x86_64");
  VectorAppend(&options, target);

  char validate_err[256];

  CHECK(ModuleValidateLoadedForImport("hello", "x86_64", "hello", "x86_64",
                                      validate_err, sizeof(validate_err)));
  CHECK(!ModuleValidateLoadedForImport("hello", "x86_64", "other", "x86_64",
                                         validate_err, sizeof(validate_err)));
  CHECK(strstr(validate_err, "other") != NULL);
  CHECK(!ModuleValidateLoadedForImport("hello", "x86_64", "hello", "aarch64",
                                       validate_err, sizeof(validate_err)));
  CHECK(strstr(validate_err, "aarch64") != NULL);
  CHECK(ModuleValidateLoadedForImport(
      "hello", "aarch64-unknown-linux-davecc", "hello",
      "aarch64-unknown-linux-davecc", validate_err, sizeof(validate_err)));
  CHECK(!ModuleValidateLoadedForImport(
      "hello", "aarch64-unknown-linux-davecc", "hello",
      "aarch64-unknown-none-davecc", validate_err, sizeof(validate_err)));
  CHECK(strstr(validate_err, "aarch64-unknown-none-davecc") != NULL);

  const char* dir = getenv("TEST_TMPDIR");
  if (dir == NULL || dir[0] == '\0') {
    dir = "/tmp";
  }

  char good_path[4096];
  char bad_name_path[4096];
  char bad_target_path[4096];
  char cycle_a_path[4096];
  char cycle_b_path[4096];
  snprintf(good_path, sizeof(good_path), "%s/good.dcm", dir);
  snprintf(bad_name_path, sizeof(bad_name_path), "%s/hello.dcm", dir);
  snprintf(bad_target_path, sizeof(bad_target_path), "%s/bad_target.dcm", dir);
  snprintf(cycle_a_path, sizeof(cycle_a_path), "%s/cycle_a.dcm", dir);
  snprintf(cycle_b_path, sizeof(cycle_b_path), "%s/cycle_b.dcm", dir);

  NewCompiler(&options);
  TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualPlain);
  int_type->size = 4;
  Symbol* exported_fn = MakeFn("dup_fn", int_type);
  InsertGlobalSymbol(exported_fn);
  CHECK(WriteModule(good_path, "good", "x86_64", exported_fn));
  CHECK(WriteModule(bad_name_path, "wrong_name", "x86_64", exported_fn));
  CHECK(WriteModule(bad_target_path, "bad_target", "aarch64", exported_fn));
  String cycle_a_dependency;
  String cycle_b_dependency;
  StringInit(&cycle_a_dependency, "cycle_b");
  StringInit(&cycle_b_dependency, "cycle_a");
  Vector cycle_a_dependencies;
  Vector cycle_b_dependencies;
  VectorInit(&cycle_a_dependencies);
  VectorInit(&cycle_b_dependencies);
  VectorAppend(&cycle_a_dependencies, &cycle_a_dependency);
  VectorAppend(&cycle_b_dependencies, &cycle_b_dependency);
  CHECK(WriteModuleWithDependencies(cycle_a_path, "cycle_a", "x86_64",
                                    exported_fn, &cycle_a_dependencies));
  CHECK(WriteModuleWithDependencies(cycle_b_path, "cycle_b", "x86_64",
                                    exported_fn, &cycle_b_dependencies));
  VectorDestruct(&cycle_a_dependencies);
  VectorDestruct(&cycle_b_dependencies);
  StringDestruct(&cycle_a_dependency);
  StringDestruct(&cycle_b_dependency);
  CompilerDelete(compiler);
  compiler = NULL;

  Vector import_options;
  VectorInit(&import_options);
  VectorAppend(&import_options, target);
  AddSearchPath(&import_options, dir);

  NewCompiler(&import_options);
  TranslationUnitImportState* state =
      TranslationUnitImportStateCreate(&import_options);
  CHECK(TranslationUnitImportStateImport(state, "good"));
  CHECK(TranslationUnitImportStateImport(state, "good"));
  String name;
  StringInit(&name, "dup_fn");
  CHECK(FindGlobalSymbol(&name) != NULL);
  StringDestruct(&name);

  CHECK(!TranslationUnitImportStateImport(state, "hello"));
  CHECK(TranslationUnitImportStateLastError(state) != NULL);
  CHECK(strstr(TranslationUnitImportStateLastError(state), "wrong_name") !=
        NULL);
  CHECK(!TranslationUnitImportStateImport(state, "hello"));
  CHECK(strstr(TranslationUnitImportStateLastError(state), "wrong_name") !=
        NULL);

  CHECK(!TranslationUnitImportStateImport(state, "bad_target"));
  CHECK(TranslationUnitImportStateLastError(state) != NULL);
  CHECK(strstr(TranslationUnitImportStateLastError(state), "aarch64") != NULL);

  CHECK(!TranslationUnitImportStateImport(state, "cycle_a"));
  CHECK(TranslationUnitImportStateLastError(state) != NULL);
  CHECK(strstr(TranslationUnitImportStateLastError(state),
               "circular module import") != NULL);
  CHECK(!TranslationUnitImportStateImport(state, "bad_target"));
  CHECK(strstr(TranslationUnitImportStateLastError(state), "aarch64") != NULL);

  TranslationUnitImportStateMarkLoadingForTest(state, "cycle_mod");
  CHECK(!TranslationUnitImportStateImport(state, "cycle_mod"));
  CHECK(strstr(TranslationUnitImportStateLastError(state),
                "circular module import") != NULL);

  TranslationUnitImportStateRelease(state);
  StringInit(&name, "dup_fn");
  CHECK(FindGlobalSymbol(&name) == NULL);
  StringDestruct(&name);
  TranslationUnitImportStateDelete(state);
  CompilerDelete(compiler);
  compiler = NULL;

  // A fresh translation unit must not reuse the previous store's loaded graph.
  NewCompiler(&import_options);
  TranslationUnitImportState* state2 =
      TranslationUnitImportStateCreate(&import_options);
  CHECK(TranslationUnitImportStateImport(state2, "good"));
  StringInit(&name, "dup_fn");
  CHECK(FindGlobalSymbol(&name) != NULL);
  StringDestruct(&name);
  TranslationUnitImportStateRelease(state2);
  StringInit(&name, "dup_fn");
  CHECK(FindGlobalSymbol(&name) == NULL);
  StringDestruct(&name);
  TranslationUnitImportStateDelete(state2);
  CompilerDelete(compiler);
  compiler = NULL;

  // Explicit canonical OS-neutral targets retain compatibility with archives
  // produced before module metadata used canonical target triples.
  StringSet(&target->value.svalue, "x86_64-unknown-none-davecc");
  NewCompiler(&import_options);
  TranslationUnitImportState* state3 =
      TranslationUnitImportStateCreate(&import_options);
  CHECK(TranslationUnitImportStateImport(state3, "good"));
  TranslationUnitImportStateRelease(state3);
  TranslationUnitImportStateDelete(state3);
  CompilerDelete(compiler);
  compiler = NULL;

  for (size_t i = 0; i < import_options.length; i++) {
    CompilerOptionValue* opt = import_options.value.p[i];
    if (opt->opt == kOptionPrebuiltModulePath) {
      StringDestruct(&opt->value.svalue);
      free(opt);
    }
  }
  VectorDestruct(&import_options);
  StringDestruct(&target->value.svalue);
  free(target);
  VectorDestruct(&options);

  if (g_failures != 0) {
    fprintf(stderr, "%d module_import check(s) failed\n", g_failures);
    return 1;
  }
  printf("module_import_test: all checks passed\n");
  return 0;
}
