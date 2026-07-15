//
//  module_syntax_test.c
//  c_compiler
//
//  Targeted tests for the C++20 module-unit model and parser state machine.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "errors.h"
#include "module_unit.h"
#include "options.h"
#include "symbol_table.h"
#include "vector.h"

static int g_failures = 0;

#define CHECK(cond)                                                   \
  do {                                                                \
    if (!(cond)) {                                                    \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      g_failures++;                                                   \
    }                                                                 \
  } while (0)

typedef struct {
  int num_errors;
  ModuleUnitInfo unit;
} ParseResult;

static Vector* Cxx20Options(void) {
  Vector* options = NewVector();
  CompilerOptionValue* target = calloc(1, sizeof(CompilerOptionValue));
  target->opt = kOptionTarget;
  StringInit(&target->value.svalue, "pcode");
  VectorAppend(options, target);
  CompilerOptionValue* standard = calloc(1, sizeof(CompilerOptionValue));
  standard->opt = kOptionStandard;
  StringInit(&standard->value.svalue, "c++20");
  VectorAppend(options, standard);
  return options;
}

static ParseResult ParseSource(const char* name, const char* code) {
  ParseResult result;
  ModuleUnitInfoInit(&result.unit);
  result.num_errors = 0;

  Vector* options = Cxx20Options();
  compiler = malloc(sizeof(Compiler));
  CompilerInitFromString(compiler, name, code, options);
  (void)CompileFrontEndOnly(compiler);
  result.num_errors = NumErrors();
  ModuleIdCopy(&result.unit.id, &compiler->module_unit.id);
  result.unit.kind = compiler->module_unit.kind;
  result.unit.fragment = compiler->module_unit.fragment;
  result.unit.phase = compiler->module_unit.phase;
  result.unit.has_module_declaration = compiler->module_unit.has_module_declaration;
  result.unit.saw_global_module_fragment =
      compiler->module_unit.saw_global_module_fragment;
  result.unit.is_module_unit = compiler->module_unit.is_module_unit;
  for (size_t i = 0; i < compiler->module_unit.imports.length; i++) {
    ModuleImportRef* src = compiler->module_unit.imports.value.p[i];
    ModuleImportRef* copy = (ModuleImportRef*)calloc(1, sizeof(*copy));
    ModuleImportRefInit(copy);
    ModuleIdCopy(&copy->id, &src->id);
    copy->is_partition_import = src->is_partition_import;
    copy->is_export_import = src->is_export_import;
    copy->location = src->location;
    VectorAppend(&result.unit.imports, copy);
  }

  CompilerDelete(compiler);
  compiler = NULL;
  VectorDestructWithContents(options, NULL, /*free_element=*/true);
  VectorDelete(options);
  return result;
}

static void FreeParseResult(ParseResult* result) {
  ModuleUnitInfoDestruct(&result->unit);
}

static void TestPrimaryInterfaceModel(void) {
  const char* code =
      "export module unit_test_primary;\n"
      "export int exported_fn() { return 1; }\n";
  ParseResult result = ParseSource("primary.cppm", code);
  CHECK(result.num_errors == 0);
  CHECK(result.unit.has_module_declaration);
  CHECK(result.unit.kind == kModuleUnitKindPrimaryInterface);
  CHECK(result.unit.fragment == kModuleFragmentPurview);
  CHECK(StringEqual(&result.unit.id.name, "unit_test_primary"));
  CHECK(result.unit.id.partition.length == 0);
  CHECK(ModuleUnitIsInterfaceUnit(&result.unit));
  FreeParseResult(&result);
}

static void TestImplementationModel(void) {
  const char* code =
      "module unit_test_impl;\n"
      "int impl_only() { return 2; }\n";
  ParseResult result = ParseSource("impl.cpp", code);
  CHECK(result.num_errors == 0);
  CHECK(result.unit.kind == kModuleUnitKindImplementation);
  CHECK(!ModuleUnitIsInterfaceUnit(&result.unit));
  CHECK(result.unit.imports.length == 1);
  ModuleImportRef* ref = result.unit.imports.value.p[0];
  CHECK(ref != NULL);
  CHECK(StringEqual(&ref->id.name, "unit_test_impl"));
  CHECK(!ref->is_partition_import);
  FreeParseResult(&result);
}

static void TestPartitionModels(void) {
  const char* iface_code =
      "export module unit_test_part:iface;\n"
      "export int iface_fn() { return 3; }\n";
  ParseResult iface = ParseSource("iface_part.cppm", iface_code);
  CHECK(iface.num_errors == 0);
  CHECK(iface.unit.kind == kModuleUnitKindInterfacePartition);
  CHECK(StringEqual(&iface.unit.id.name, "unit_test_part"));
  CHECK(StringEqual(&iface.unit.id.partition, "iface"));
  FreeParseResult(&iface);

  const char* internal_code =
      "module unit_test_part:impl;\n"
      "int internal_fn() { return 4; }\n";
  ParseResult internal = ParseSource("internal_part.cpp", internal_code);
  CHECK(internal.num_errors == 0);
  CHECK(internal.unit.kind == kModuleUnitKindInternalPartition);
  CHECK(StringEqual(&internal.unit.id.partition, "impl"));
  FreeParseResult(&internal);
}

static void TestGlobalAndPrivateFragments(void) {
  const char* code =
      "module;\n"
      "int gmf = 1;\n"
      "export module unit_test_fragments;\n"
      "export int iface() { return gmf; }\n"
      "module :private;\n"
      "int helper() { return iface() + 1; }\n";
  ParseResult result = ParseSource("fragments.cppm", code);
  CHECK(result.num_errors == 0);
  CHECK(result.unit.saw_global_module_fragment);
  CHECK(result.unit.phase == kModuleParserPhasePrivateFragment);
  CHECK(result.unit.fragment == kModuleFragmentPrivate);
  FreeParseResult(&result);
}

static void TestImportRecorded(void) {
  const char* code =
      "export module unit_test_import;\n"
      "import missing.module.name;\n"
      "export int x() { return 0; }\n";
  ParseResult result = ParseSource("import.cppm", code);
  CHECK(result.unit.imports.length == 1);
  ModuleImportRef* ref = result.unit.imports.value.p[0];
  CHECK(ref != NULL);
  CHECK(StringEqual(&ref->id.name, "missing.module.name"));
  CHECK(!ref->is_partition_import);
  CHECK(!ref->is_export_import);
  FreeParseResult(&result);
}

static void TestPartitionImportModeled(void) {
  const char* code =
      "export module unit_test_rel:part;\n"
      "import :other;\n"
      "export int x() { return 0; }\n";
  ParseResult result = ParseSource("rel_import.cppm", code);
  CHECK(result.unit.imports.length == 1);
  ModuleImportRef* ref = result.unit.imports.value.p[0];
  CHECK(ref != NULL);
  CHECK(ref->is_partition_import);
  CHECK(StringEqual(&ref->id.partition, "other"));
  FreeParseResult(&result);
}

static void TestStandardsDiagnostics(void) {
  ParseResult impl_private = ParseSource(
      "impl_private.cpp",
      "module a;\nmodule :private;\nint x;\n");
  CHECK(impl_private.num_errors > 0);
  FreeParseResult(&impl_private);

  ParseResult part_private = ParseSource(
      "part_private.cppm",
      "export module a:p;\nmodule :private;\nint x;\n");
  CHECK(part_private.num_errors > 0);
  FreeParseResult(&part_private);

  ParseResult gmf_import = ParseSource(
      "gmf_import.cppm",
      "module;\nimport a;\nexport module a;\n");
  CHECK(gmf_import.num_errors > 0);
  FreeParseResult(&gmf_import);

  ParseResult nonmod_module = ParseSource(
      "nonmod_module.cpp",
      "import a;\nint x;\nmodule a;\n");
  CHECK(nonmod_module.num_errors > 0);
  FreeParseResult(&nonmod_module);

  ParseResult import_then_module = ParseSource(
      "import_then_module.cpp",
      "import a;\nmodule a;\n");
  CHECK(import_then_module.num_errors > 0);
  CHECK(!import_then_module.unit.has_module_declaration);
  FreeParseResult(&import_then_module);

  ParseResult export_import_impl = ParseSource(
      "export_import_impl.cpp",
      "module a;\nexport import a;\n");
  CHECK(export_import_impl.num_errors > 0);
  FreeParseResult(&export_import_impl);
}

static void TestDiagnostics(void) {
  ParseResult multi = ParseSource("multi.cppm",
                                  "export module a;\nmodule a;\nint x;\n");
  CHECK(multi.num_errors > 0);
  FreeParseResult(&multi);

  ParseResult late_import = ParseSource("late_import.cppm",
                                        "export module a;\nint x;\nimport a;\n");
  CHECK(late_import.num_errors > 0);
  FreeParseResult(&late_import);

  ParseResult impl_export =
      ParseSource("impl_export.cpp", "module a;\nexport int x();\n");
  CHECK(impl_export.num_errors > 0);
  FreeParseResult(&impl_export);

  ParseResult private_export = ParseSource(
      "private_export.cppm",
      "export module a;\nmodule :private;\nexport int x();\n");
  CHECK(private_export.num_errors > 0);
  FreeParseResult(&private_export);
}

int main(void) {
  TestPrimaryInterfaceModel();
  TestImplementationModel();
  TestPartitionModels();
  TestGlobalAndPrivateFragments();
  TestImportRecorded();
  TestPartitionImportModeled();
  TestStandardsDiagnostics();
  TestDiagnostics();

  if (g_failures != 0) {
    fprintf(stderr, "%d module_syntax_test failure(s)\n", g_failures);
    return 1;
  }
  puts("ok module_syntax_test");
  return 0;
}
