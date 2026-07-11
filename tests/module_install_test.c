//
//  module_install_test.c
//  c_compiler
//
//  Verifies ModuleInstallLoaded: a module is produced with a mix of exported
//  and non-exported entities (both at file scope and inside a namespace),
//  written to a ".dcm", then loaded into a *fresh* compiler and installed.
//  Only the exported names must become visible in the importer's global symbol
//  tables / namespace tree.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "compiler.h"
#include "module_archive.h"
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

static void MakeTempPath(char* out, size_t out_len) {
  const char* dir = getenv("TEST_TMPDIR");
  if (dir == NULL || dir[0] == '\0') {
    dir = "/tmp";
  }
  snprintf(out, out_len, "%s/install.dcm", dir);
}

static void NewCompiler(Vector* options) {
  compiler = malloc(sizeof(Compiler));
  CompilerInitFromString(compiler, "install_test", "", options);
  CreateGlobalSymbolTables();
}

// A simple defined function symbol `int name()` with the given export flag.
static Symbol* MakeFn(const char* name, TypeRecord* int_type, bool exported) {
  TypeRecord* ft = NewFunctionTypeRecord();
  TypeRecordChain(ft, int_type);
  ft->info.function.definition = true;
  Symbol* s = NewSymbol(name, ft, STO(implicit));
  s->flags.is_defined = true;
  s->flags.is_exported = exported;
  ft->info.function.symbol = s;
  return s;
}

static Namespace* FindChild(Namespace* parent, const char* name) {
  String s;
  StringInit(&s, name);
  Namespace* child = NamespaceFindChild(parent, &s);
  StringDestruct(&s);
  return child;
}

int main(void) {
  Vector options;
  VectorInit(&options);
  CompilerOptionValue* target = calloc(1, sizeof(CompilerOptionValue));
  target->opt = kOptionTarget;
  StringInit(&target->value.svalue, "x86_64");
  VectorAppend(&options, target);

  char path[4096];
  MakeTempPath(path, sizeof(path));

  // --- Producer: build graph and write the module. -------------------------
  NewCompiler(&options);

  TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualPlain);
  int_type->size = 4;

  // File-scope: one exported, one not.  These live in the global symbol table
  // (like real file-scope declarations) and are also passed as root symbols.
  Symbol* exported_fn = MakeFn("exported_fn", int_type, true);
  InsertGlobalSymbol(exported_fn);
  Symbol* hidden_fn = MakeFn("hidden_fn", int_type, false);
  InsertGlobalSymbol(hidden_fn);

  Vector roots;
  VectorInit(&roots);
  VectorAppend(&roots, exported_fn);
  VectorAppend(&roots, hidden_fn);

  // Namespace `mymod` with one exported and one non-exported symbol.
  String nsname;
  StringInit(&nsname, "mymod");
  Namespace* mymod =
      NamespaceFindOrCreateChild(compiler->global_namespace, &nsname);
  StringDestruct(&nsname);
  NamespaceInsertSymbol(mymod, MakeFn("ns_exported", int_type, true));
  NamespaceInsertSymbol(mymod, MakeFn("ns_hidden", int_type, false));

  String inlname;
  StringInit(&inlname, "inline_ns");
  bool inline_conflict = false;
  Namespace* inline_ns =
      NamespaceFindOrReopenChild(mymod, &inlname, true, &inline_conflict);
  StringDestruct(&inlname);
  CHECK(inline_ns != NULL);
  CHECK(!inline_conflict);
  CHECK(inline_ns->is_inline);
  NamespaceInsertSymbol(inline_ns, MakeFn("inline_exported", int_type, true));

  Vector ns_roots;
  VectorInit(&ns_roots);
  VectorAppend(&ns_roots, compiler->global_namespace);

  ModuleWriteRequest req = {
      .module_name = "mymod",
      .target_triple = "x86_64-unknown-none",
      .compiler_version = "davecc-test",
      .flags = 0,
      .root_symbols = &roots,
      .root_namespaces = &ns_roots,
  };
  CHECK(ModuleWrite(path, &req));

  VectorDestruct(&roots);
  VectorDestruct(&ns_roots);
  CompilerDelete(compiler);
  compiler = NULL;

  // --- Importer: fresh compiler, load + install, then check visibility. ----
  NewCompiler(&options);

  LoadedModule loaded;
  CHECK(ModuleLoad(path, &loaded));
  if (g_failures == 0) {
    CHECK(ModuleInstallLoaded(&loaded));

    // File-scope: only the exported symbol is visible.
    String name;
    StringInit(&name, "exported_fn");
    CHECK(FindGlobalSymbol(&name) != NULL);
    StringDestruct(&name);
    StringInit(&name, "hidden_fn");
    CHECK(FindGlobalSymbol(&name) == NULL);
    StringDestruct(&name);

    // Namespace was created and only its exported member is visible.
    Namespace* imported = FindChild(compiler->global_namespace, "mymod");
    CHECK(imported != NULL);
    if (imported != NULL) {
      StringInit(&name, "ns_exported");
      CHECK(NamespaceFindSymbolInScope(imported, &name) != NULL);
      StringDestruct(&name);
      StringInit(&name, "ns_hidden");
      CHECK(NamespaceFindSymbolInScope(imported, &name) == NULL);
      StringDestruct(&name);

      Namespace* imported_inline = FindChild(imported, "inline_ns");
      CHECK(imported_inline != NULL);
      CHECK(imported_inline->is_inline);
      if (imported_inline != NULL) {
        StringInit(&name, "inline_exported");
        CHECK(NamespaceFindSymbolInScope(imported_inline, &name) != NULL);
        StringDestruct(&name);
      }
    }

    LoadedModuleDestruct(&loaded);
  }

  CompilerDelete(compiler);
  compiler = NULL;

  // --- Inline conflict: importer has non-inline child, module merges inline. --
  {
    char conflict_path[4096];
    const char* dir = getenv("TEST_TMPDIR");
    if (dir == NULL || dir[0] == '\0') {
      dir = "/tmp";
    }
    snprintf(conflict_path, sizeof(conflict_path), "%s/install_conflict.dcm",
             dir);

    NewCompiler(&options);
    TypeRecord* conflict_int_type = NewTypeRecord(kTypeInt, kQualPlain);
    conflict_int_type->size = 4;

    String conflict_name;
    StringInit(&conflict_name, "conflict_ns");
    Namespace* inline_src =
        NamespaceFindOrReopenChild(compiler->global_namespace, &conflict_name,
                                   true, NULL);
    StringDestruct(&conflict_name);
    CHECK(inline_src != NULL);
    CHECK(inline_src->is_inline);
    NamespaceInsertSymbol(inline_src,
                          MakeFn("conflict_fn", conflict_int_type, true));

    Vector conflict_roots;
    VectorInit(&conflict_roots);
    VectorAppend(&conflict_roots, compiler->global_namespace);

    ModuleWriteRequest conflict_req = {
        .module_name = "conflict_mod",
        .target_triple = "x86_64-unknown-none",
        .compiler_version = "davecc-test",
        .flags = 0,
        .root_symbols = NULL,
        .root_namespaces = &conflict_roots,
    };
    CHECK(ModuleWrite(conflict_path, &conflict_req));
    VectorDestruct(&conflict_roots);
    CompilerDelete(compiler);
    compiler = NULL;

    NewCompiler(&options);
    StringInit(&conflict_name, "conflict_ns");
    Namespace* existing =
        NamespaceFindOrReopenChild(compiler->global_namespace, &conflict_name,
                                   false, NULL);
    StringDestruct(&conflict_name);
    CHECK(existing != NULL);
    CHECK(!existing->is_inline);

    LoadedModule conflict_loaded;
    CHECK(ModuleLoad(conflict_path, &conflict_loaded));
    CHECK(!ModuleInstallLoaded(&conflict_loaded));
    LoadedModuleDestruct(&conflict_loaded);
    CompilerDelete(compiler);
    compiler = NULL;
  }

  // options cleanup
  StringDestruct(&target->value.svalue);
  free(target);
  VectorDestruct(&options);

  if (g_failures != 0) {
    fprintf(stderr, "%d module_install check(s) failed\n", g_failures);
    return 1;
  }
  printf("module_install_test: all checks passed\n");
  return 0;
}
