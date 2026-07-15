//
//  module_identity_test.c
//  c_compiler
//
//  Focused tests for module attachment, linkage, mangling, serialization, and
//  cross-TU redeclaration policy.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "module_archive.h"
#include "module_identity.h"
#include "module_install.h"
#include "module_unit.h"
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

static Vector* TestOptions(void) {
  Vector* options = NewVector();
  CompilerOptionValue* target = calloc(1, sizeof(CompilerOptionValue));
  target->opt = kOptionTarget;
  StringInit(&target->value.svalue, "x86_64");
  VectorAppend(options, target);
  CompilerOptionValue* standard = calloc(1, sizeof(CompilerOptionValue));
  standard->opt = kOptionStandard;
  StringInit(&standard->value.svalue, "c++20");
  VectorAppend(options, standard);
  return options;
}

static void NewCompiler(Vector* options) {
  compiler = malloc(sizeof(Compiler));
  CompilerInitFromString(compiler, "module_identity_test", "", options);
  CreateGlobalSymbolTables();
}

static void MakeTempPath(char* out, size_t out_len, const char* leaf) {
  const char* dir = getenv("TEST_TMPDIR");
  if (dir == NULL || dir[0] == '\0') {
    dir = "/tmp";
  }
  snprintf(out, out_len, "%s/%s", dir, leaf);
}

static Symbol* MakeModuleFunction(const char* name, const char* module_name,
                                  bool exported, CXXLinkageKind linkage) {
  TypeRecord* ft = NewFunctionTypeRecord();
  TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualPlain);
  int_type->size = 4;
  TypeRecordChain(ft, int_type);
  ft->info.function.definition = true;
  Symbol* sym = NewSymbol(name, ft, STO(implicit));
  sym->flags.is_defined = true;
  sym->flags.is_exported = exported;
  sym->cxx_linkage = linkage;
  StringSet(&sym->owning_module_name, module_name);
  ft->info.function.symbol = sym;
  return sym;
}

static void TestLinkageAttachment(void) {
  Vector* options = TestOptions();
  NewCompiler(options);

  compiler->module_unit.is_module_unit = true;
  compiler->module_unit.has_module_declaration = true;
  compiler->module_unit.fragment = kModuleFragmentPurview;
  StringSet(&compiler->module_unit.id.name, "mod_a");

  Symbol* exported = NewSymbol("pub", NewTypeRecord(kTypeInt, kQualPlain),
                               STO(implicit));
  exported->flags.is_exported = true;
  SymbolAttachModuleContext(exported, STO(implicit));
  CHECK(exported->cxx_linkage == kCXXLinkageExternal);
  CHECK(StringEqual(&exported->owning_module_name, "mod_a"));

  Symbol* hidden = NewSymbol("hidden", NewTypeRecord(kTypeInt, kQualPlain),
                             STO(implicit));
  SymbolAttachModuleContext(hidden, STO(implicit));
  CHECK(hidden->cxx_linkage == kCXXLinkageModule);

  Symbol* internal = NewSymbol("local", NewTypeRecord(kTypeInt, kQualPlain),
                              STO(static));
  SymbolAttachModuleContext(internal, STO(static));
  CHECK(internal->cxx_linkage == kCXXLinkageInternal);

  CompilerDelete(compiler);
  compiler = NULL;
  VectorDestructWithContents(options, NULL, /*free_element=*/true);
  VectorDelete(options);
}

static void TestManglingDistinctAcrossModules(void) {
  Vector* options = TestOptions();
  NewCompiler(options);

  Symbol* alpha =
      MakeModuleFunction("helper", "alpha", false, kCXXLinkageModule);
  Symbol* beta = MakeModuleFunction("helper", "beta", false, kCXXLinkageModule);
  SymbolSetCXXMangledAsmName(alpha);
  SymbolSetCXXMangledAsmName(beta);
  CHECK(alpha->asm_name.length > 0);
  CHECK(beta->asm_name.length > 0);
  CHECK(!StringEqualString(&alpha->asm_name, &beta->asm_name));

  Symbol* alpha_exported =
      MakeModuleFunction("helper", "alpha", true, kCXXLinkageExternal);
  Symbol* beta_exported =
      MakeModuleFunction("helper", "beta", true, kCXXLinkageExternal);
  SymbolSetCXXMangledAsmName(alpha_exported);
  SymbolSetCXXMangledAsmName(beta_exported);
  CHECK(!StringEqualString(&alpha_exported->asm_name, &beta_exported->asm_name));

  Symbol* partition_a =
      MakeModuleFunction("shared", "partitioned", false, kCXXLinkageModule);
  Symbol* partition_b =
      MakeModuleFunction("shared", "partitioned", false, kCXXLinkageModule);
  StringSet(&partition_a->owning_module_partition, "a");
  StringSet(&partition_b->owning_module_partition, "b");
  SymbolSetCXXMangledAsmName(partition_a);
  SymbolSetCXXMangledAsmName(partition_b);
  CHECK(StringEqualString(&partition_a->asm_name, &partition_b->asm_name));

  Symbol* dotted =
      MakeModuleFunction("f", "company.math", false, kCXXLinkageModule);
  SymbolSetCXXMangledAsmName(dotted);
  CHECK(strstr(dotted->asm_name.value, "W7company4mathE") != NULL);

  CompilerDelete(compiler);
  compiler = NULL;
  VectorDestructWithContents(options, NULL, /*free_element=*/true);
  VectorDelete(options);
}

static void TestRedeclarationPolicy(void) {
  Vector* options = TestOptions();
  NewCompiler(options);

  Symbol* iface = MakeModuleFunction("f", "iface_mod", true, kCXXLinkageExternal);
  Symbol* impl = MakeModuleFunction("f", "iface_mod", false, kCXXLinkageModule);
  SymbolSetImportProvenance(iface, "iface_mod");
  CHECK(SymbolCompatibleModuleRedeclaration(iface, impl));

  Symbol* alpha = MakeModuleFunction("secret", "alpha", false, kCXXLinkageModule);
  Symbol* beta = MakeModuleFunction("secret", "beta", false, kCXXLinkageModule);
  CHECK(!SymbolCompatibleModuleRedeclaration(alpha, beta));

  CompilerDelete(compiler);
  compiler = NULL;
  VectorDestructWithContents(options, NULL, /*free_element=*/true);
  VectorDelete(options);
}

static void TestSerializeRoundTrip(void) {
  Vector* options = TestOptions();

  char path[4096];
  MakeTempPath(path, sizeof(path), "identity.dcm");

  NewCompiler(options);
  Symbol* hidden =
      MakeModuleFunction("hidden", "roundtrip", false, kCXXLinkageModule);
  StringSet(&hidden->owning_module_partition, "part");
  Vector roots;
  VectorInit(&roots);
  VectorAppend(&roots, hidden);
  ModuleWriteRequest req = {
      .module_name = "roundtrip",
      .target_triple = "x86_64",
      .compiler_version = "test",
      .root_symbols = &roots,
      .root_namespaces = NULL,
  };
  CHECK(ModuleWrite(path, &req));
  CompilerDelete(compiler);
  compiler = NULL;

  NewCompiler(options);
  LoadedModule loaded;
  CHECK(ModuleLoad(path, &loaded));
  CHECK(loaded.root_symbols.length == 1);
  Symbol* loaded_sym = (Symbol*)VectorGet(&loaded.root_symbols, 0);
  CHECK(loaded_sym->cxx_linkage == kCXXLinkageModule);
  CHECK(StringEqual(&loaded_sym->owning_module_name, "roundtrip"));
  CHECK(StringEqual(&loaded_sym->owning_module_partition, "part"));
  LoadedModuleReleaseGraph(&loaded);
  LoadedModuleDestruct(&loaded);
  CompilerDelete(compiler);
  compiler = NULL;
  VectorDestructWithContents(options, NULL, /*free_element=*/true);
  VectorDelete(options);
}

static void TestInstallProvenance(void) {
  Vector* options = TestOptions();

  char path[4096];
  MakeTempPath(path, sizeof(path), "prov.dcm");

  NewCompiler(options);
  Symbol* pub = MakeModuleFunction("pub", "prov_mod", true, kCXXLinkageExternal);
  Vector roots;
  VectorInit(&roots);
  VectorAppend(&roots, pub);
  ModuleWriteRequest req = {
      .module_name = "prov_mod",
      .target_triple = "x86_64",
      .compiler_version = "test",
      .root_symbols = &roots,
      .root_namespaces = NULL,
  };
  CHECK(ModuleWrite(path, &req));
  CompilerDelete(compiler);
  compiler = NULL;

  NewCompiler(options);
  LoadedModule loaded;
  CHECK(ModuleLoad(path, &loaded));
  CHECK(ModuleInstallLoaded(&loaded));
  String name;
  StringInit(&name, "pub");
  Symbol* installed = FindGlobalSymbol(&name);
  StringDestruct(&name);
  CHECK(installed != NULL);
  CHECK(StringEqual(&installed->import_source_module, "prov_mod"));
  UninstallGlobalSymbol(installed, false);
  LoadedModuleReleaseGraph(&loaded);
  LoadedModuleDestruct(&loaded);
  CompilerDelete(compiler);
  compiler = NULL;
  VectorDestructWithContents(options, NULL, /*free_element=*/true);
  VectorDelete(options);
}

int main(void) {
  TestLinkageAttachment();
  TestManglingDistinctAcrossModules();
  TestRedeclarationPolicy();
  TestSerializeRoundTrip();
  TestInstallProvenance();

  if (g_failures != 0) {
    fprintf(stderr, "%d module_identity_test failure(s)\n", g_failures);
    return 1;
  }
  puts("ok module_identity_test");
  return 0;
}
