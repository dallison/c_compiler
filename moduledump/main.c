//
//  main.c
//  moduledump
//
//  A pretty-printer for C++20 module interface files (".dcm").  Unlike the
//  generic `archivist` tool (which only understands the outer `ar` container),
//  moduledump understands the module payload: it deserializes the full typed
//  object graph via ModuleLoad and dumps it in a user-friendly format.
//
//  The file layout is documented in docs/module_file_format.md.  Each dump
//  section corresponds to one interned object pool (types, symbols, structs,
//  enums, struct members, namespaces, AST nodes) plus the module header, the
//  string pool, and the raw `ar` member table.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ar.h"
#include "ast.h"
#include "compiler.h"
#include "concepts.h"
#include "dstring.h"
#include "module_archive.h"
#include "options.h"
#include "serialize.h"
#include "symbol.h"
#include "symbol_table.h"
#include "type.h"
#include "vector.h"

// ---------------------------------------------------------------------------
// Selected output sections.
// ---------------------------------------------------------------------------
typedef struct {
  bool header;
  bool summary;
  bool archive;     // Raw `ar` member table + symbol table.
  bool strings;
  bool types;
  bool symbols;
  bool structs;
  bool enums;
  bool members;
  bool namespaces;
  bool ast;
} Sections;

static bool AnySectionSelected(const Sections* s) {
  return s->header || s->summary || s->archive || s->strings || s->types ||
         s->symbols || s->structs || s->enums || s->members || s->namespaces ||
         s->ast;
}

static void SelectAll(Sections* s) {
  s->header = s->summary = s->archive = s->strings = s->types = s->symbols =
      s->structs = s->enums = s->members = s->namespaces = s->ast = true;
}

// ---------------------------------------------------------------------------
// Small rendering helpers.
// ---------------------------------------------------------------------------

// Renders a type into `out` (already initialized), guarding against NULL.
static void RenderType(TypeRecord* t, String* out) {
  StringClear(out);
  if (t == NULL) {
    StringAppend(out, "<none>");
    return;
  }
  TypeRecordToString(t, out);
  if (out->length == 0) {
    StringAppend(out, "<unnamed>");
  }
}

static const char* AccessName(CXXAccess a) {
  switch (a) {
    case kAccessPublic: return "public";
    case kAccessProtected: return "protected";
    case kAccessPrivate: return "private";
  }
  return "?";
}

static const char* ConstraintKindName(ConstraintExprKind k) {
  switch (k) {
    case kConstraintAtomic: return "atomic";
    case kConstraintConjunction: return "conjunction(&&)";
    case kConstraintDisjunction: return "disjunction(||)";
    case kConstraintConceptId: return "concept-id";
    case kConstraintRequires: return "requires-expr";
  }
  return "?";
}

static const char* TemplateParameterKindName(TemplateParameterKind k) {
  switch (k) {
    case kTemplateParameterType: return "type";
    case kTemplateParameterNonType: return "non-type";
    case kTemplateParameterTemplate: return "template";
  }
  return "?";
}

// Appends a comma-separated list of the storage bits that are set.
static void RenderStorage(Storage storage, String* out) {
  StringClear(out);
  if (storage == STO(implicit)) {
    StringAppend(out, "implicit");
    return;
  }
  const char* sep = "";
  struct {
    Storage bit;
    const char* name;
  } bits[] = {
      {STO(auto), "auto"},         {STO(static), "static"},
      {STO(typedef), "typedef"},   {STO(extern), "extern"},
      {STO(register), "register"}, {STO(assembler), "assembler"},
      {STO(thread), "thread"},
  };
  for (size_t i = 0; i < sizeof(bits) / sizeof(bits[0]); i++) {
    if ((storage & bits[i].bit) != 0) {
      StringAppend(out, sep);
      StringAppend(out, bits[i].name);
      sep = "|";
    }
  }
  if (out->length == 0) {
    StringAppend(out, "implicit");
  }
}

// Appends a space-separated list of the symbol flags that are set.
static void RenderSymbolFlags(Symbol* s, String* out) {
  StringClear(out);
  const char* sep = "";
#define FLAG(cond, name)         \
  if (cond) {                    \
    StringAppend(out, sep);      \
    StringAppend(out, name);     \
    sep = " ";                   \
  }
  FLAG(s->flags.is_defined, "defined");
  FLAG(s->flags.is_tentative_decl, "tentative");
  FLAG(s->flags.is_forward_declared, "forward-declared");
  FLAG(s->flags.is_local, "local");
  FLAG(s->flags.is_block_scope, "block-scope");
  FLAG(s->flags.is_argument, "argument");
  FLAG(s->flags.is_temp, "temp");
  FLAG(s->flags.address_taken, "address-taken");
  FLAG(s->flags.used, "used");
  FLAG(s->flags.invented, "invented");
  FLAG(s->flags.is_inline_defn, "inline-defn");
  FLAG(s->flags.value_set, "value-set");
  FLAG(s->flags.noreturn, "noreturn");
  FLAG(s->flags.always_inline, "always-inline");
  FLAG(s->flags.noinline, "noinline");
  FLAG(s->flags.is_using_alias, "using-alias");
  FLAG(s->flags.is_overloaded, "overloaded");
  FLAG(s->flags.is_template, "template");
  FLAG(s->flags.is_template_parameter, "template-parameter");
  FLAG(s->flags.is_template_type_parameter, "template-type-parameter");
  FLAG(s->flags.is_parameter_pack, "parameter-pack");
  FLAG(s->flags.is_constexpr, "constexpr");
  FLAG(s->flags.is_constinit, "constinit");
  FLAG(s->flags.is_weak, "weak");
  FLAG(s->flags.is_c_linkage, "c-linkage");
  FLAG(s->flags.is_exported, "exported");
  FLAG(s->flags.is_concept, "concept");
#undef FLAG
  if (out->length == 0) {
    StringAppend(out, "-");
  }
}

// ---------------------------------------------------------------------------
// Pool accessors.  Each pool is DeserializeContext.objects[kind]; handle N maps
// to index N-1.
// ---------------------------------------------------------------------------
static Vector* Pool(LoadedModule* m, SerialKind kind) {
  return &m->ctx.objects[kind];
}

// ---------------------------------------------------------------------------
// Sections.
// ---------------------------------------------------------------------------

static void DumpHeader(LoadedModule* m) {
  printf("== Module header ==\n");
  printf("  module name       : %s\n",
         m->module_name.length ? m->module_name.value : "(anonymous)");
  printf("  target triple     : %s\n",
         m->target_triple.length ? m->target_triple.value : "(none)");
  printf("  compiler version  : %s\n",
         m->compiler_version.length ? m->compiler_version.value : "(none)");
  printf("  format version    : %u\n", m->format_version);
  printf("  flags             : 0x%x\n", m->flags);
  printf("  dependencies      : %zu\n", m->dependencies.length);
  for (size_t i = 0; i < m->dependencies.length; i++) {
    String* dependency = (String*)VectorGet(&m->dependencies, i);
    printf("      - %s\n", dependency->value);
  }
  printf("  re-exports        : %zu\n", m->reexports.length);
  for (size_t i = 0; i < m->reexports.length; i++) {
    String* reexport = (String*)VectorGet(&m->reexports, i);
    printf("      - %s\n", reexport->value);
  }
  printf("  exported symbols  : %zu\n", m->root_symbols.length);
  for (size_t i = 0; i < m->root_symbols.length; i++) {
    Symbol* s = (Symbol*)VectorGet(&m->root_symbols, i);
    printf("      - %s\n", s != NULL ? s->name.value : "<null>");
  }
  printf("  exported namespaces: %zu\n", m->root_namespaces.length);
  for (size_t i = 0; i < m->root_namespaces.length; i++) {
    Namespace* ns = (Namespace*)VectorGet(&m->root_namespaces, i);
    const char* name = ns == NULL ? "<null>"
                       : ns->qualified_name.length ? ns->qualified_name.value
                                                   : "(global)";
    printf("      - %s\n", name);
  }
  printf("\n");
}

static void DumpSummary(LoadedModule* m) {
  printf("== Pool summary ==\n");
  printf("  strings     : %zu\n", m->ctx.string_pool.length);
  printf("  types       : %zu\n", Pool(m, kSerialKindType)->length);
  printf("  symbols     : %zu\n", Pool(m, kSerialKindSymbol)->length);
  printf("  structs     : %zu\n", Pool(m, kSerialKindStruct)->length);
  printf("  enums       : %zu\n", Pool(m, kSerialKindEnum)->length);
  printf("  members     : %zu\n", Pool(m, kSerialKindStructMember)->length);
  printf("  namespaces  : %zu\n", Pool(m, kSerialKindNamespace)->length);
  printf("  ast nodes   : %zu\n", Pool(m, kSerialKindAST)->length);
  printf("\n");
}

static void DumpArchive(const char* path) {
  printf("== AR container ==\n");
  ARArchive archive;
  ARArchiveInit(&archive, path);
  FILE* fp = fopen(path, "r");
  if (fp == NULL || !ARArchiveOpen(&archive, fp)) {
    printf("  <unable to open archive>\n\n");
    if (fp != NULL) fclose(fp);
    ARArchiveDestruct(&archive);
    return;
  }
  printf("  %-12s %10s\n", "member", "size");
  for (size_t i = 0; i < archive.files.length; i++) {
    ARFile* f = (ARFile*)VectorGet(&archive.files, i);
    if (f->filename.value[0] == '/') {
      continue;  // Skip the ar symbol/string tables.
    }
    printf("  %-12s %10lld\n", f->filename.value, (long long)f->size);
  }
  if (archive.symbol_table_file != NULL) {
    printf("  ar symbol table:\n");
    ARArchivePrintSymbolTable(&archive);
  }
  fclose(fp);
  ARArchiveDestruct(&archive);
  printf("\n");
}

static void DumpStrings(LoadedModule* m) {
  printf("== String pool (%zu) ==\n", m->ctx.string_pool.length);
  for (size_t i = 0; i < m->ctx.string_pool.length; i++) {
    const char* s = (const char*)VectorGet(&m->ctx.string_pool, i);
    printf("  [%zu] \"%s\"\n", i + 1, s != NULL ? s : "");
  }
  printf("\n");
}

static void DumpTypes(LoadedModule* m) {
  Vector* pool = Pool(m, kSerialKindType);
  printf("== Types (%zu) ==\n", pool->length);
  String s;
  StringInit(&s, "");
  for (size_t i = 0; i < pool->length; i++) {
    TypeRecord* t = (TypeRecord*)VectorGet(pool, i);
    RenderType(t, &s);
    printf("  [#%zu] %s", i + 1, s.value);
    if (t != NULL) {
      printf("   (id=%d size=%d)", t->id, t->size);
    }
    printf("\n");
  }
  StringDestruct(&s);
  printf("\n");
}

static void DumpConstraint(ConstraintExpr* c, int indent) {
  for (int i = 0; i < indent; i++) printf("  ");
  if (c == NULL) {
    printf("<none>\n");
    return;
  }
  printf("constraint: %s\n", ConstraintKindName(c->kind));
  switch (c->kind) {
    case kConstraintAtomic:
      if (c->as.atomic.expr != NULL) {
        ASTNodePrint(c->as.atomic.expr, indent + 1, stdout);
      }
      break;
    case kConstraintConjunction:
    case kConstraintDisjunction:
      DumpConstraint(c->as.binary.left, indent + 1);
      DumpConstraint(c->as.binary.right, indent + 1);
      break;
    case kConstraintConceptId: {
      Symbol* cs = c->as.concept_id.concept_symbol;
      for (int i = 0; i < indent + 1; i++) printf("  ");
      printf("concept %s, %zu argument(s)\n",
             cs != NULL ? cs->name.value : "<null>",
             c->as.concept_id.arguments ? c->as.concept_id.arguments->length
                                        : 0);
      break;
    }
    case kConstraintRequires:
      for (int i = 0; i < indent + 1; i++) printf("  ");
      printf("(requires-expression)\n");
      break;
  }
}

static void DumpTemplateParameters(Vector* params, int indent) {
  if (params == NULL || params->length == 0) {
    return;
  }
  for (int i = 0; i < indent; i++) printf("  ");
  printf("template parameters (%zu):\n", params->length);
  for (size_t i = 0; i < params->length; i++) {
    TemplateParameter* p = (TemplateParameter*)VectorGet(params, i);
    for (int j = 0; j < indent + 1; j++) printf("  ");
    printf("[%d] %s %s%s\n", p->index, TemplateParameterKindName(p->kind),
           p->name.value, p->is_parameter_pack ? " ..." : "");
    if (p->associated_constraint != NULL) {
      DumpConstraint(p->associated_constraint, indent + 2);
    }
  }
}

static void DumpSymbol(Symbol* s, size_t handle, String* scratch) {
  if (s == NULL) {
    printf("  [#%zu] <null>\n", handle);
    return;
  }
  printf("  [#%zu] %s\n", handle, s->name.length ? s->name.value : "(unnamed)");

  String store;
  StringInit(&store, "");
  RenderStorage(s->storage, &store);
  printf("        storage : %s\n", store.value);
  StringDestruct(&store);

  RenderType(s->type, scratch);
  printf("        type    : %s\n", scratch->value);

  RenderSymbolFlags(s, scratch);
  printf("        flags   : %s\n", scratch->value);

  if (s->asm_name.length) {
    printf("        asm name: %s\n", s->asm_name.value);
  }
  if (s->namespace_ != NULL && s->namespace_->qualified_name.length) {
    printf("        in ns   : %s\n", s->namespace_->qualified_name.value);
  }
  if (s->flags.value_set) {
    printf("        value   : %lld\n", (long long)s->value.ivalue);
  }
  if (s->alignment != 0) {
    printf("        align   : %d\n", s->alignment);
  }
  for (size_t i = 0; i < s->attributes.length; i++) {
    Attribute* a = (Attribute*)VectorGet(&s->attributes, i);
    printf("        attr    : %s", a->name.value);
    if (AttributeArgCount(a) > 0) {
      printf("(");
      for (size_t j = 0; j < AttributeArgCount(a); j++) {
        printf("%s%s", j ? ", " : "", AttributeArgString(a, j));
      }
      printf(")");
    }
    printf("\n");
  }
  if (s->alias_target != NULL) {
    printf("        alias   : %s\n", s->alias_target->name.value);
  }
  if (s->overload_next != NULL) {
    printf("        overload-> %s\n", s->overload_next->name.value);
  }
  if (s->default_argument != NULL) {
    printf("        default argument:\n");
    ASTNodePrint(s->default_argument, 5, stdout);
  }
  if (s->type != NULL && TypeIsFunction(s->type)) {
    printf("        fn template params: count=%d vec=%zu\n",
           s->type->info.function.template_parameter_count,
           s->type->info.function.template_parameters.length);
  }
  if (s->variable_template != NULL) {
    printf("        variable template:\n");
    DumpTemplateParameters(&s->variable_template->parameters, 5);
    if (s->variable_template->initializer != NULL) {
      for (int i = 0; i < 5; i++) printf("  ");
      printf("initializer:\n");
      ASTNodePrint(s->variable_template->initializer, 6, stdout);
    }
  }
  if (s->concept_definition != NULL) {
    Concept* c = s->concept_definition;
    printf("        concept definition: %s\n", c->name.value);
    DumpTemplateParameters(c->template_parameters, 5);
    DumpConstraint(c->constraint, 5);
  }
}

static void DumpSymbols(LoadedModule* m) {
  Vector* pool = Pool(m, kSerialKindSymbol);
  printf("== Symbols (%zu) ==\n", pool->length);
  String scratch;
  StringInit(&scratch, "");
  for (size_t i = 0; i < pool->length; i++) {
    DumpSymbol((Symbol*)VectorGet(pool, i), i + 1, &scratch);
  }
  StringDestruct(&scratch);
  printf("\n");
}

static void DumpStructMember(StructMember* mem, int indent, String* scratch) {
  for (int i = 0; i < indent; i++) printf("  ");
  if (mem == NULL) {
    printf("<null member>\n");
    return;
  }
  const char* name =
      mem->symbol != NULL ? mem->symbol->name.value : "(anonymous)";
  RenderType(mem->symbol != NULL ? mem->symbol->type : NULL, scratch);
  printf("[%zu] %s : %s @+%d", mem->index, name, scratch->value,
         mem->byte_offset);
  if (mem->bit_size != 0) {
    printf(" bit[%d:%d]", mem->bit_offset, mem->bit_size);
  }
  printf(" %s", AccessName(mem->access));
  if (mem->is_static) printf(" static");
  if (mem->is_mutable) printf(" mutable");
  if (mem->is_member_function) printf(" fn");
  if (mem->is_using_declaration) printf(" using");
  printf("\n");
}

static void DumpStructs(LoadedModule* m) {
  Vector* pool = Pool(m, kSerialKindStruct);
  printf("== Structs / unions / classes (%zu) ==\n", pool->length);
  String scratch;
  StringInit(&scratch, "");
  for (size_t i = 0; i < pool->length; i++) {
    Struct* st = (Struct*)VectorGet(pool, i);
    if (st == NULL) {
      printf("  [#%zu] <null>\n", i + 1);
      continue;
    }
    const char* kind = st->is_union ? "union" : st->is_class ? "class" : "struct";
    printf("  [#%zu] %s %s\n", i + 1, kind,
           st->tag_name != NULL ? st->tag_name->value : "(anonymous)");
    printf("        size=%d align=%d%s%s%s%s\n", st->size, st->alignment,
           st->packed ? " packed" : "", st->is_final ? " final" : "",
           st->is_abstract ? " abstract" : "",
           st->is_template ? " template" : "");
    DumpTemplateParameters(&st->template_parameters, 4);
    for (size_t b = 0; b < st->bases.length; b++) {
      CXXBaseSpecifier* base = (CXXBaseSpecifier*)VectorGet(&st->bases, b);
      RenderType(base->type, &scratch);
      printf("        base: %s %s @+%d%s\n", AccessName(base->access),
             scratch.value, base->byte_offset,
             base->is_virtual ? " virtual" : "");
    }
    printf("        members (%zu):\n", st->members.length);
    for (size_t mi = 0; mi < st->members.length; mi++) {
      DumpStructMember((StructMember*)VectorGet(&st->members, mi), 5, &scratch);
    }
  }
  StringDestruct(&scratch);
  printf("\n");
}

static void DumpEnums(LoadedModule* m) {
  Vector* pool = Pool(m, kSerialKindEnum);
  printf("== Enums (%zu) ==\n", pool->length);
  for (size_t i = 0; i < pool->length; i++) {
    Enum* e = (Enum*)VectorGet(pool, i);
    if (e == NULL) {
      printf("  [#%zu] <null>\n", i + 1);
      continue;
    }
    printf("  [#%zu] %senum %s", i + 1, e->is_scoped ? "scoped " : "",
           e->tag_name != NULL ? e->tag_name->value : "(anonymous)");
    if (e->has_fixed_underlying) {
      printf(" : (fixed underlying, %d bytes)", e->fixed_underlying_size);
    }
    printf("\n");
    printf("        constants (%zu):\n", e->constants.length);
    for (size_t c = 0; c < e->constants.length; c++) {
      Symbol* k = (Symbol*)VectorGet(&e->constants, c);
      printf("          %s = %lld\n", k != NULL ? k->name.value : "<null>",
             k != NULL ? (long long)k->value.ivalue : 0);
    }
  }
  printf("\n");
}

static void DumpMembers(LoadedModule* m) {
  Vector* pool = Pool(m, kSerialKindStructMember);
  printf("== Struct members pool (%zu) ==\n", pool->length);
  String scratch;
  StringInit(&scratch, "");
  for (size_t i = 0; i < pool->length; i++) {
    printf("  [#%zu]", i + 1);
    DumpStructMember((StructMember*)VectorGet(pool, i), 1, &scratch);
  }
  StringDestruct(&scratch);
  printf("\n");
}

// BinaryTree traversal callback: prints "name (type)".
static void PrintNamespaceEntry(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  SymbolNode* sn = (SymbolNode*)node;
  String* scratch = (String*)data;
  Symbol* s = sn->symbol;
  if (s == NULL) {
    printf("          <null>\n");
    return;
  }
  RenderType(s->type, scratch);
  printf("          %s : %s\n", s->name.value, scratch->value);
}

static void DumpNamespaces(LoadedModule* m) {
  Vector* pool = Pool(m, kSerialKindNamespace);
  printf("== Namespaces (%zu) ==\n", pool->length);
  String scratch;
  StringInit(&scratch, "");
  for (size_t i = 0; i < pool->length; i++) {
    Namespace* ns = (Namespace*)VectorGet(pool, i);
    if (ns == NULL) {
      printf("  [#%zu] <null>\n", i + 1);
      continue;
    }
    const char* qname = ns->qualified_name.length ? ns->qualified_name.value
                                                   : "(global)";
    printf("  [#%zu] %s%s\n", i + 1, qname,
           ns->is_anonymous ? " (anonymous)" : "");
    if (ns->parent != NULL) {
      printf("        parent: %s\n",
             ns->parent->qualified_name.length ? ns->parent->qualified_name.value
                                               : "(global)");
    }
    for (size_t c = 0; c < ns->children.length; c++) {
      Namespace* child = (Namespace*)VectorGet(&ns->children, c);
      printf("        child : %s\n",
             child != NULL && child->name.length ? child->name.value
                                                  : "(anonymous)");
    }
    printf("        symbols:\n");
    BinaryTreeTraverse(&ns->symbol_table, PrintNamespaceEntry, &scratch);
    printf("        tags:\n");
    BinaryTreeTraverse(&ns->tag_table, PrintNamespaceEntry, &scratch);
  }
  StringDestruct(&scratch);
  printf("\n");
}

// The AST pool is a flat set of interconnected nodes; dumping every node would
// print shared subtrees many times.  Instead we print each AST *root* anchored
// to its owner (function body, default argument, variable-template initializer,
// or struct-member default initializer), which is what a reader wants to see.
static void DumpAST(LoadedModule* m) {
  Vector* symbols = Pool(m, kSerialKindSymbol);
  Vector* members = Pool(m, kSerialKindStructMember);
  printf("== AST bodies ==\n");
  bool any = false;

  for (size_t i = 0; i < symbols->length; i++) {
    Symbol* s = (Symbol*)VectorGet(symbols, i);
    if (s == NULL) {
      continue;
    }
    if (s->type != NULL && s->type->declarator == kDeclFunction &&
        s->type->info.function.body != NULL) {
      printf("  function %s body:\n", s->name.value);
      ASTNodePrint(s->type->info.function.body, 2, stdout);
      any = true;
    }
    if (s->default_argument != NULL) {
      printf("  default argument of %s:\n", s->name.value);
      ASTNodePrint(s->default_argument, 2, stdout);
      any = true;
    }
    if (s->variable_template != NULL &&
        s->variable_template->initializer != NULL) {
      printf("  variable-template initializer of %s:\n", s->name.value);
      ASTNodePrint(s->variable_template->initializer, 2, stdout);
      any = true;
    }
    if (s->concept_definition != NULL &&
        s->concept_definition->constraint != NULL) {
      printf("  concept %s constraint:\n", s->concept_definition->name.value);
      DumpConstraint(s->concept_definition->constraint, 2);
      any = true;
    }
  }
  for (size_t i = 0; i < members->length; i++) {
    StructMember* mem = (StructMember*)VectorGet(members, i);
    if (mem != NULL && mem->default_initializer != NULL) {
      printf("  default member initializer of %s:\n",
             mem->symbol != NULL ? mem->symbol->name.value : "(anonymous)");
      ASTNodePrint(mem->default_initializer, 2, stdout);
      any = true;
    }
  }
  if (!any) {
    printf("  (no AST bodies)\n");
  }
  printf("\n");
}

// ---------------------------------------------------------------------------
// CLI.
// ---------------------------------------------------------------------------
static void Usage(const char* prog) {
  fprintf(stderr,
          "Usage: %s [options] <module.dcm>\n"
          "\n"
          "Dumps the contents of a C++20 module interface (.dcm) file.\n"
          "With no section options, prints the header and a pool summary.\n"
          "\n"
          "Section options:\n"
          "  --header        Module header (name, target, roots).\n"
          "  --summary       Object-pool element counts.\n"
          "  --archive       Raw ar container member table + symbol table.\n"
          "  --strings       Interned string pool.\n"
          "  --types         Type records (rendered).\n"
          "  --symbols       Symbols (flags, type, value, attrs, templates).\n"
          "  --structs       Structs/unions/classes (bases, members, offsets).\n"
          "  --enums         Enums (constants and values).\n"
          "  --members       Flat struct-member pool.\n"
          "  --namespaces    Namespace tree (children, symbols, tags).\n"
          "  --ast           AST bodies (function bodies, initializers, ...).\n"
          "  -a, --all       Every section.\n"
          "\n"
          "Other options:\n"
          "  --target <t>    Target triple for the loader (default x86_64).\n"
          "  -h, --help      This help.\n",
          prog);
}

int main(int argc, char* argv[]) {
  Sections sections;
  memset(&sections, 0, sizeof(sections));
  const char* target = "x86_64";
  const char* path = NULL;

  for (int i = 1; i < argc; i++) {
    const char* a = argv[i];
    if (strcmp(a, "--header") == 0) {
      sections.header = true;
    } else if (strcmp(a, "--summary") == 0) {
      sections.summary = true;
    } else if (strcmp(a, "--archive") == 0) {
      sections.archive = true;
    } else if (strcmp(a, "--strings") == 0) {
      sections.strings = true;
    } else if (strcmp(a, "--types") == 0) {
      sections.types = true;
    } else if (strcmp(a, "--symbols") == 0) {
      sections.symbols = true;
    } else if (strcmp(a, "--structs") == 0) {
      sections.structs = true;
    } else if (strcmp(a, "--enums") == 0) {
      sections.enums = true;
    } else if (strcmp(a, "--members") == 0) {
      sections.members = true;
    } else if (strcmp(a, "--namespaces") == 0) {
      sections.namespaces = true;
    } else if (strcmp(a, "--ast") == 0) {
      sections.ast = true;
    } else if (strcmp(a, "-a") == 0 || strcmp(a, "--all") == 0) {
      SelectAll(&sections);
    } else if (strcmp(a, "--target") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "moduledump: --target needs a value\n");
        return 1;
      }
      target = argv[++i];
    } else if (strncmp(a, "--target=", 9) == 0) {
      target = a + 9;
    } else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      Usage(argv[0]);
      return 0;
    } else if (a[0] == '-' && a[1] != '\0') {
      fprintf(stderr, "moduledump: unknown option %s\n", a);
      Usage(argv[0]);
      return 1;
    } else {
      path = a;
    }
  }

  if (path == NULL) {
    Usage(argv[0]);
    return 1;
  }

  if (!AnySectionSelected(&sections)) {
    sections.header = true;
    sections.summary = true;
  }

  // The module loader allocates deserialized objects from the compiler's
  // arenas, so a compiler global must exist (see module_archive.h).
  Vector options;
  VectorInit(&options);
  CompilerOptionValue* target_opt = calloc(1, sizeof(CompilerOptionValue));
  target_opt->opt = kOptionTarget;
  StringInit(&target_opt->value.svalue, target);
  VectorAppend(&options, target_opt);

  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromString(compiler, path, "", &options)) {
    fprintf(stderr, "moduledump: failed to initialize compiler for %s\n", path);
    return 1;
  }
  CreateGlobalSymbolTables();

  LoadedModule loaded;
  if (!ModuleLoad(path, &loaded)) {
    fprintf(stderr, "moduledump: failed to load module %s\n", path);
    return 1;
  }

  if (sections.header) DumpHeader(&loaded);
  if (sections.summary) DumpSummary(&loaded);
  if (sections.archive) DumpArchive(path);
  if (sections.strings) DumpStrings(&loaded);
  if (sections.namespaces) DumpNamespaces(&loaded);
  if (sections.types) DumpTypes(&loaded);
  if (sections.structs) DumpStructs(&loaded);
  if (sections.enums) DumpEnums(&loaded);
  if (sections.members) DumpMembers(&loaded);
  if (sections.symbols) DumpSymbols(&loaded);
  if (sections.ast) DumpAST(&loaded);

  LoadedModuleReleaseGraph(&loaded);
  CompilerDelete(compiler);
  compiler = NULL;
  LoadedModuleDestruct(&loaded);
  return 0;
}
