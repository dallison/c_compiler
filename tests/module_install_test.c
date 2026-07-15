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
#include "type_template.h"
#include "vector.h"

static TemplateParameter* MakeTypeParam(const char* name, int index) {
  TemplateParameter* p = (TemplateParameter*)calloc(1, sizeof(*p));
  StringInit(&p->name, name);
  p->kind = kTemplateParameterType;
  p->index = index;
  p->default_template_parameter_index = -1;
  return p;
}

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

static Symbol* MakeFnWithParam(const char* name, TypeRecord* int_type,
                               bool exported) {
  TypeRecord* ft = NewFunctionTypeRecord();
  TypeRecordChain(ft, int_type);
  ft->info.function.definition = true;
  Symbol* param = NewSymbol("x", int_type, STO(auto));
  param->flags.is_argument = true;
  param->flags.is_defined = true;
  VectorAppend(&ft->info.function.prototype, param);
  Symbol* s = NewSymbol(name, ft, STO(implicit));
  s->flags.is_defined = true;
  s->flags.is_exported = exported;
  ft->info.function.symbol = s;
  return s;
}

static TypeRecord* MakeTemplatePlaceholderType(const char* name, int index) {
  TypeRecord* t = NewTypeRecord(kTypeUnknown, kQualPlain);
  t->template_parameter_index = index;
  t->template_parameter_name = NewString(name);
  return t;
}

static Symbol* BuildIdFunctionTemplate(const char* name, bool exported) {
  TypeRecord* t_type = MakeTemplatePlaceholderType("T", 0);
  TypeRecord* ft = NewFunctionTypeRecord();
  TypeRecordChain(ft, TypeRecordCopy(t_type));
  ft->info.function.definition = true;
  VectorAppend(&ft->info.function.template_parameters, MakeTypeParam("T", 0));
  ft->info.function.template_parameter_count = 1;

  Symbol* param = NewSymbol("x", TypeRecordCopy(t_type), STO(auto));
  param->flags.is_argument = true;
  param->flags.is_defined = true;
  VectorAppend(&ft->info.function.prototype, param);

  ASTNode* ret_expr = NewIdentifierASTNode(param, 0);
  ASTNode* ret_stmt =
      NewCombinedStatementASTNode(AST_OP(return), ret_expr, NULL, 0);
  Vector* stmts = NewVector();
  VectorAppend(stmts, ret_stmt);
  ft->info.function.body = NewCompoundStatementASTNode(stmts, 0);

  Symbol* id = NewSymbol(name, ft, STO(implicit));
  id->flags.is_template = true;
  id->flags.is_defined = true;
  id->flags.is_exported = exported;
  ft->info.function.symbol = id;
  id->value.func_defn = id;
  return id;
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

  Struct* mod_box = NewStruct(false);
  mod_box->tag_name = NewString("NsBox");
  Symbol* box_value = NewSymbol("value", int_type, STO(implicit));
  VectorAppend(&mod_box->members, NewStructMember(box_value));
  mod_box->size = 4;
  mod_box->alignment = 4;
  TypeRecord* mod_box_type = NewTypeRecord(kTypeStruct, kQualPlain);
  mod_box_type->info.struct_info = mod_box;
  Symbol* mod_box_tag = NewSymbol("NsBox", mod_box_type, STO(implicit));
  mod_box_tag->flags.is_exported = true;
  mod_box_tag->flags.is_defined = true;
  mod_box->tag_symbol = mod_box_tag;
  NamespaceInsertTag(mymod, mod_box_tag);

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
    ModuleInstallRecord record;
    ModuleInstallRecordInit(&record);
    CHECK(ModuleInstallLoadedTracked(&loaded, &record));

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
      StringInit(&name, "NsBox");
      Symbol* imported_box = NamespaceFindTagInScope(imported, &name);
      CHECK(imported_box != NULL);
      if (imported_box != NULL) {
        CHECK(imported_box->type != NULL &&
              imported_box->type->info.struct_info != NULL);
        CHECK(imported_box->type->info.struct_info->members.length >= 1);
      }
      StringDestruct(&name);
    }

    ModuleInstallRecordRollback(&record);
    ModuleInstallRecordDestruct(&record);
    LoadedModuleReleaseGraph(&loaded);
    CompilerDelete(compiler);
    compiler = NULL;
    LoadedModuleDestruct(&loaded);
  }

  // --- Exported tags, overloads, variables, and install conflicts. -----------
  {
    char surface_path[4096];
    const char* dir = getenv("TEST_TMPDIR");
    if (dir == NULL || dir[0] == '\0') {
      dir = "/tmp";
    }
    snprintf(surface_path, sizeof(surface_path), "%s/install_surface.dcm", dir);

    NewCompiler(&options);
    TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualPlain);
    int_type->size = 4;

    Struct* box = NewStruct(false);
    box->tag_name = NewString("InstallBox");
    Symbol* x_sym = NewSymbol("value", int_type, STO(implicit));
    StructMember* x = NewStructMember(x_sym);
    VectorAppend(&box->members, x);
    box->size = 4;
    box->alignment = 4;
    TypeRecord* box_type = NewTypeRecord(kTypeStruct, kQualPlain);
    box_type->info.struct_info = box;
    Symbol* box_tag = NewSymbol("InstallBox", box_type, STO(implicit));
    box_tag->flags.is_exported = true;
    box_tag->flags.is_defined = true;
    box->tag_symbol = box_tag;
    InsertGlobalTag(box_tag);

    Struct* point = NewStruct(false);
    point->tag_name = NewString("GlobalPoint");
    Symbol* px = NewSymbol("x", int_type, STO(implicit));
    Symbol* py = NewSymbol("y", int_type, STO(implicit));
    VectorAppend(&point->members, NewStructMember(px));
    VectorAppend(&point->members, NewStructMember(py));
    point->size = 8;
    point->alignment = 4;
    TypeRecord* point_type = NewTypeRecord(kTypeStruct, kQualPlain);
    point_type->info.struct_info = point;
    Symbol* point_typedef = NewSymbol("GlobalPoint", point_type, STO(typedef));
    point_typedef->flags.is_exported = true;
    point->tag_symbol = NewSymbol("GlobalPoint", point_type, STO(implicit));
    point->tag_symbol->flags.is_exported = true;
    point->tag_symbol->flags.is_defined = true;
    InsertGlobalTag(point->tag_symbol);

    Symbol* overload_a = MakeFn("install_overload", int_type, true);
    TypeRecord* double_type = NewTypeRecord(kTypeDouble, kQualPlain);
    double_type->size = 8;
    Symbol* overload_b = MakeFn("install_overload", double_type, true);
    overload_b->overload_next = NULL;
    InsertGlobalSymbol(overload_a);
    overload_a->overload_next = overload_b;
    overload_a->flags.is_overloaded = true;
    overload_b->flags.is_overloaded = true;

    Symbol* exported_var = NewSymbol("install_answer", int_type, STO(static));
    exported_var->flags.is_exported = true;
    exported_var->flags.is_defined = true;
    exported_var->flags.value_set = true;
    exported_var->value.ivalue = 42;
    InsertGlobalSymbol(exported_var);

    Vector roots;
    VectorInit(&roots);
    VectorAppend(&roots, box_tag);
    VectorAppend(&roots, point_typedef);
    VectorAppend(&roots, overload_a);
    VectorAppend(&roots, exported_var);

    Vector ns_roots;
    VectorInit(&ns_roots);
    VectorAppend(&ns_roots, compiler->global_namespace);

    ModuleWriteRequest req = {
        .module_name = "install_surface",
        .target_triple = "x86_64-unknown-none",
        .compiler_version = "davecc-test",
        .flags = 0,
        .root_symbols = &roots,
        .root_namespaces = &ns_roots,
    };
    CHECK(ModuleWrite(surface_path, &req));
    VectorDestruct(&roots);
    VectorDestruct(&ns_roots);
    CompilerDelete(compiler);
    compiler = NULL;

    NewCompiler(&options);
    LoadedModule surface_loaded;
    CHECK(ModuleLoad(surface_path, &surface_loaded));
    Symbol* loaded_global_point = NULL;
    for (size_t i = 0; i < surface_loaded.root_symbols.length; i++) {
      Symbol* root = (Symbol*)VectorGet(&surface_loaded.root_symbols, i);
      if (root != NULL && StringEqual(&root->name, "GlobalPoint")) {
        loaded_global_point = root;
        break;
      }
    }
    CHECK(loaded_global_point != NULL);
    if (loaded_global_point != NULL && loaded_global_point->type != NULL &&
        loaded_global_point->type->info.struct_info != NULL) {
      Symbol* tag = loaded_global_point->type->info.struct_info->tag_symbol;
      CHECK(tag != NULL);
      if (tag != NULL) {
        CHECK(tag->type != NULL && tag->type->info.struct_info != NULL);
        CHECK(tag->type->info.struct_info->members.length >= 2);
      }
    }
    ModuleInstallRecord surface_record;
    ModuleInstallRecordInit(&surface_record);
    CHECK(ModuleInstallLoadedTracked(&surface_loaded, &surface_record));

    String name;
    StringInit(&name, "InstallBox");
    CHECK(FindGlobalTag(&name) != NULL);
    StringDestruct(&name);

    StringInit(&name, "GlobalPoint");
    Symbol* installed_global_point = FindGlobalTag(&name);
    CHECK(installed_global_point != NULL);
    if (installed_global_point != NULL) {
      CHECK(installed_global_point->type != NULL &&
            installed_global_point->type->info.struct_info != NULL);
      CHECK(installed_global_point->type->info.struct_info->members.length >= 2);
    }
    StringDestruct(&name);

    StringInit(&name, "install_overload");
    Symbol* installed_overload = FindGlobalSymbol(&name);
    CHECK(installed_overload != NULL);
    CHECK(installed_overload->overload_next != NULL);
    StringDestruct(&name);

    StringInit(&name, "install_answer");
    CHECK(FindGlobalSymbol(&name) != NULL);
    CHECK(FindGlobalSymbol(&name)->value.ivalue == 42);
    StringDestruct(&name);

    ModuleInstallRecordDestruct(&surface_record);
    LoadedModuleReleaseGraph(&surface_loaded);
    CompilerDelete(compiler);
    compiler = NULL;
    LoadedModuleDestruct(&surface_loaded);

    NewCompiler(&options);
    TypeRecord* conflict_int_type = NewTypeRecord(kTypeInt, kQualPlain);
    conflict_int_type->size = 4;
    TypeRecord* conflict_double_type = NewTypeRecord(kTypeDouble, kQualPlain);
    conflict_double_type->size = 8;
    Symbol* conflicting =
        NewSymbol("install_answer", conflict_double_type, STO(static));
    conflicting->flags.is_defined = true;
    InsertGlobalSymbol(conflicting);
    LoadedModule conflict_surface;
    CHECK(ModuleLoad(surface_path, &conflict_surface));
    ModuleInstallRecord conflict_surface_record;
    ModuleInstallRecordInit(&conflict_surface_record);
    CHECK(!ModuleInstallLoadedTracked(&conflict_surface, &conflict_surface_record));
    CHECK(ModuleInstallLastError() != NULL);
    ModuleInstallRecordDestruct(&conflict_surface_record);
    LoadedModuleReleaseGraph(&conflict_surface);
    CompilerDelete(compiler);
    compiler = NULL;
    LoadedModuleDestruct(&conflict_surface);
  }

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
    ModuleInstallRecord conflict_record;
    ModuleInstallRecordInit(&conflict_record);
    CHECK(!ModuleInstallLoadedTracked(&conflict_loaded, &conflict_record));
    StringInit(&conflict_name, "conflict_fn");
    CHECK(NamespaceFindSymbolInScope(existing, &conflict_name) == NULL);
    StringDestruct(&conflict_name);
    ModuleInstallRecordDestruct(&conflict_record);
    LoadedModuleReleaseGraph(&conflict_loaded);
    CompilerDelete(compiler);
    compiler = NULL;
    LoadedModuleDestruct(&conflict_loaded);
  }

  // --- Function prototype parameter round-trip. ------------------------------
  {
    char param_path[4096];
    const char* dir = getenv("TEST_TMPDIR");
    if (dir == NULL || dir[0] == '\0') {
      dir = "/tmp";
    }
    snprintf(param_path, sizeof(param_path), "%s/install_param.dcm", dir);

    NewCompiler(&options);
    TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualPlain);
    int_type->size = 4;
    Symbol* param_fn = MakeFnWithParam("param_fn", int_type, true);
    InsertGlobalSymbol(param_fn);

    Vector roots;
    VectorInit(&roots);
    VectorAppend(&roots, param_fn);
    Vector ns_roots;
    VectorInit(&ns_roots);
    VectorAppend(&ns_roots, compiler->global_namespace);

    ModuleWriteRequest req = {
        .module_name = "param_mod",
        .target_triple = "x86_64-unknown-none",
        .compiler_version = "davecc-test",
        .flags = 0,
        .root_symbols = &roots,
        .root_namespaces = &ns_roots,
    };
    CHECK(ModuleWrite(param_path, &req));
    VectorDestruct(&roots);
    VectorDestruct(&ns_roots);
    CompilerDelete(compiler);
    compiler = NULL;

    NewCompiler(&options);
    LoadedModule param_loaded;
    CHECK(ModuleLoad(param_path, &param_loaded));
    Symbol* loaded_fn = NULL;
    for (size_t i = 0; i < param_loaded.root_symbols.length; i++) {
      Symbol* root = (Symbol*)VectorGet(&param_loaded.root_symbols, i);
      if (root != NULL && StringEqual(&root->name, "param_fn")) {
        loaded_fn = root;
        break;
      }
    }
    CHECK(loaded_fn != NULL);
    if (loaded_fn != NULL && loaded_fn->type != NULL &&
        TypeIsFunction(loaded_fn->type)) {
      CHECK(loaded_fn->type->info.function.prototype.length == 1);
      Symbol* formal =
          (Symbol*)loaded_fn->type->info.function.prototype.value.p[0];
      CHECK(formal != NULL);
      if (formal != NULL) {
        CHECK(StringEqual(&formal->name, "x"));
        CHECK(formal->type != NULL && TypeIsInt(formal->type));
        CHECK(formal->is_imported_module_symbol);
      }
    }
    ModuleInstallRecord param_record;
    ModuleInstallRecordInit(&param_record);
    CHECK(ModuleInstallLoadedTracked(&param_loaded, &param_record));
    String name;
    StringInit(&name, "param_fn");
    Symbol* installed = FindGlobalSymbol(&name);
    CHECK(installed != NULL);
    if (installed != NULL && installed->type != NULL &&
        TypeIsFunction(installed->type)) {
      CHECK(installed->type->info.function.prototype.length == 1);
      Symbol* formal =
          (Symbol*)installed->type->info.function.prototype.value.p[0];
      CHECK(formal != NULL && formal->type != NULL);
    }
    StringDestruct(&name);
    ModuleInstallRecordDestruct(&param_record);
    LoadedModuleReleaseGraph(&param_loaded);
    CompilerDelete(compiler);
    compiler = NULL;
    LoadedModuleDestruct(&param_loaded);
  }

  // --- Function template install preserves template parameters. --------------
  {
    char tmpl_path[4096];
    const char* dir = getenv("TEST_TMPDIR");
    if (dir == NULL || dir[0] == '\0') {
      dir = "/tmp";
    }
    snprintf(tmpl_path, sizeof(tmpl_path), "%s/install_tmpl.dcm", dir);

    NewCompiler(&options);
    TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualPlain);
    int_type->size = 4;
    TypeRecord* ft = NewFunctionTypeRecord();
    TypeRecordChain(ft, int_type);
    ft->info.function.definition = true;
    VectorAppend(&ft->info.function.template_parameters, MakeTypeParam("T", 0));
    ft->info.function.template_parameter_count = 1;

    Symbol* tmpl_fn = NewSymbol("tmpl_fn", ft, STO(implicit));
    tmpl_fn->flags.is_template = true;
    tmpl_fn->flags.is_defined = true;
    tmpl_fn->flags.is_exported = true;
    ft->info.function.symbol = tmpl_fn;
    InsertGlobalSymbol(tmpl_fn);

    Vector roots;
    VectorInit(&roots);
    VectorAppend(&roots, tmpl_fn);
    Vector ns_roots;
    VectorInit(&ns_roots);
    VectorAppend(&ns_roots, compiler->global_namespace);

    ModuleWriteRequest req = {
        .module_name = "install_tmpl",
        .target_triple = "x86_64-unknown-none",
        .compiler_version = "davecc-test",
        .flags = 0,
        .root_symbols = &roots,
        .root_namespaces = &ns_roots,
    };
    CHECK(ModuleWrite(tmpl_path, &req));
    VectorDestruct(&roots);
    VectorDestruct(&ns_roots);
    CompilerDelete(compiler);
    compiler = NULL;

    NewCompiler(&options);
    LoadedModule tmpl_loaded;
    CHECK(ModuleLoad(tmpl_path, &tmpl_loaded));
    ModuleInstallRecord tmpl_record;
    ModuleInstallRecordInit(&tmpl_record);
    CHECK(ModuleInstallLoadedTracked(&tmpl_loaded, &tmpl_record));
    Symbol* loaded_tmpl = NULL;
    for (size_t i = 0; i < tmpl_loaded.root_symbols.length; i++) {
      Symbol* root = (Symbol*)VectorGet(&tmpl_loaded.root_symbols, i);
      if (root != NULL && StringEqual(&root->name, "tmpl_fn")) {
        loaded_tmpl = root;
        break;
      }
    }
    CHECK(loaded_tmpl != NULL);
    String name;
    StringInit(&name, "tmpl_fn");
    Symbol* installed_tmpl = FindGlobalSymbol(&name);
    CHECK(installed_tmpl != NULL);
    CHECK(installed_tmpl == loaded_tmpl);
    if (installed_tmpl != NULL && installed_tmpl->type != NULL &&
        TypeIsFunction(installed_tmpl->type)) {
      CHECK(installed_tmpl->type->info.function.template_parameters.length == 1);
      CHECK(installed_tmpl->type->info.function.template_parameter_count == 1);
    }
    StringDestruct(&name);
    ModuleInstallRecordDestruct(&tmpl_record);
    LoadedModuleReleaseGraph(&tmpl_loaded);
    CompilerDelete(compiler);
    compiler = NULL;
    LoadedModuleDestruct(&tmpl_loaded);
  }

  // --- Function template with body + placeholder formal survives install. ---
  {
    char id_path[4096];
    const char* dir = getenv("TEST_TMPDIR");
    if (dir == NULL || dir[0] == '\0') {
      dir = "/tmp";
    }
    snprintf(id_path, sizeof(id_path), "%s/install_id.dcm", dir);

    NewCompiler(&options);
    Symbol* id_fn = BuildIdFunctionTemplate("id", true);
    InsertGlobalSymbol(id_fn);

    Vector roots;
    VectorInit(&roots);
    VectorAppend(&roots, id_fn);
    Vector ns_roots;
    VectorInit(&ns_roots);
    VectorAppend(&ns_roots, compiler->global_namespace);

    ModuleWriteRequest req = {
        .module_name = "install_id",
        .target_triple = "x86_64-unknown-none",
        .compiler_version = "davecc-test",
        .flags = 0,
        .root_symbols = &roots,
        .root_namespaces = &ns_roots,
    };
    CHECK(ModuleWrite(id_path, &req));
    VectorDestruct(&roots);
    VectorDestruct(&ns_roots);
    CompilerDelete(compiler);
    compiler = NULL;

    NewCompiler(&options);
    TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualPlain);
    int_type->size = 4;
    LoadedModule id_loaded;
    CHECK(ModuleLoad(id_path, &id_loaded));
    Symbol* loaded_id = NULL;
    TemplateParameter* loaded_param = NULL;
    for (size_t i = 0; i < id_loaded.root_symbols.length; i++) {
      Symbol* root = (Symbol*)VectorGet(&id_loaded.root_symbols, i);
      if (root != NULL && StringEqual(&root->name, "id")) {
        loaded_id = root;
        if (root->type != NULL && TypeIsFunction(root->type) &&
            root->type->info.function.template_parameters.length == 1) {
          loaded_param = (TemplateParameter*)VectorGet(
              &root->type->info.function.template_parameters, 0);
        }
        break;
      }
    }
    CHECK(loaded_id != NULL);
    CHECK(loaded_param != NULL);

    ModuleInstallRecord id_record;
    ModuleInstallRecordInit(&id_record);
    CHECK(ModuleInstallLoadedTracked(&id_loaded, &id_record));
    String name;
    StringInit(&name, "id");
    Symbol* installed_id = FindGlobalSymbol(&name);
    CHECK(installed_id != NULL);
    CHECK(installed_id == loaded_id);
    if (installed_id != NULL && installed_id->type != NULL &&
        TypeIsFunction(installed_id->type)) {
      CHECK(installed_id->type->info.function.template_parameters.length == 1);
      TemplateParameter* installed_param = (TemplateParameter*)VectorGet(
          &installed_id->type->info.function.template_parameters, 0);
      CHECK(installed_param != NULL);
      Vector actuals;
      VectorInit(&actuals);
      VectorAppend(&actuals, NewIntConstantASTNode(5, int_type, 0));
      Symbol* instantiated = TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
          &compiler->syntax, installed_id, NULL, &actuals);
      VectorDestruct(&actuals);
      CHECK(instantiated != NULL);
      CHECK(instantiated != installed_id);
      CHECK(instantiated->type != NULL && TypeIsFunction(instantiated->type));
      CHECK(instantiated->type->next != NULL);
      CHECK(!instantiated->flags.is_template);
    }
    StringDestruct(&name);
    ModuleInstallRecordDestruct(&id_record);
    LoadedModuleReleaseGraph(&id_loaded);
    CompilerDelete(compiler);
    compiler = NULL;
    LoadedModuleDestruct(&id_loaded);
  }

  // --- Source-emitted surface.dcm: function template id instantiates. ---
  {
    const char* tmp = getenv("TEST_TMPDIR");
    if (tmp == NULL || tmp[0] == '\0') {
      tmp = "/tmp";
    }
    const char* srcdir = getenv("TEST_SRCDIR");
    const char* workspace = getenv("TEST_WORKSPACE");
    const char* davecc = getenv("DAVECC_BIN");
    if (davecc == NULL || davecc[0] == '\0') {
      davecc = "bazel-bin/davecc";
    }
    char surface_cppm[4096];
    if (srcdir != NULL && workspace != NULL) {
      snprintf(surface_cppm, sizeof(surface_cppm), "%s/%s/cxx_testsuite/tests/modules/surface.cppm",
               srcdir, workspace);
    } else {
      snprintf(surface_cppm, sizeof(surface_cppm),
               "cxx_testsuite/tests/modules/surface.cppm");
    }
    char surface_dcm[4096];
    snprintf(surface_dcm, sizeof(surface_dcm), "%s/surface_install_test.dcm", tmp);
    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
             "%s -target x86_64 -std=c++20 -Xemit-module %s %s >/dev/null 2>&1",
             davecc, surface_dcm, surface_cppm);
    if (system(cmd) == 0) {
      NewCompiler(&options);
      TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualPlain);
      int_type->size = 4;
      LoadedModule surface_loaded;
      CHECK(ModuleLoad(surface_dcm, &surface_loaded));
      Symbol* loaded_id = NULL;
      for (size_t i = 0; i < surface_loaded.root_symbols.length; i++) {
        Symbol* root = (Symbol*)surface_loaded.root_symbols.value.p[i];
        if (root != NULL && StringEqual(&root->name, "id")) {
          loaded_id = root;
          break;
        }
      }
      CHECK(loaded_id != NULL);
      ModuleInstallRecord surface_record;
      ModuleInstallRecordInit(&surface_record);
      CHECK(ModuleInstallLoadedTracked(&surface_loaded, &surface_record));
      String name;
      StringInit(&name, "id");
      Symbol* installed_id = FindGlobalSymbol(&name);
      CHECK(installed_id != NULL);
      CHECK(installed_id == loaded_id);
      if (installed_id != NULL && installed_id->type != NULL &&
          TypeIsFunction(installed_id->type)) {
        size_t type_params =
            installed_id->type->info.function.template_parameters.length;
        size_t defn_params = 0;
        if (installed_id->value.func_defn != NULL &&
            installed_id->value.func_defn->type != NULL &&
            TypeIsFunction(installed_id->value.func_defn->type)) {
          defn_params =
              installed_id->value.func_defn->type->info.function
                  .template_parameters.length;
        }
        CHECK(type_params > 0 || defn_params > 0);
        Vector actuals;
        VectorInit(&actuals);
        VectorAppend(&actuals, NewIntConstantASTNode(5, int_type, 0));
        Symbol* instantiated = TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
            &compiler->syntax, installed_id, NULL, &actuals);
        VectorDestruct(&actuals);
        CHECK(instantiated != NULL);
        CHECK(instantiated != installed_id);
      CHECK(instantiated->type != NULL && TypeIsFunction(instantiated->type));
      CHECK(instantiated->type->next != NULL);
      CHECK(!instantiated->flags.is_template);
    }
    StringDestruct(&name);
    ModuleInstallRecordDestruct(&surface_record);
      LoadedModuleReleaseGraph(&surface_loaded);
      CompilerDelete(compiler);
      compiler = NULL;
      LoadedModuleDestruct(&surface_loaded);
    }
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
