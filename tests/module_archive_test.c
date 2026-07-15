//
//  module_archive_test.c
//  c_compiler
//
//  End-to-end round trip for the C++20 module container: builds a small but
//  representative post-semantic graph (types, a variable symbol with flags,
//  value and attributes, a struct with members, a scoped-style enum with
//  constants, and a function whose body is a real AST), writes it to a ".dcm"
//  archive with ModuleWrite, reads it back with ModuleLoad, and asserts the
//  structure survives.  Returns 0 on success.
//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "compiler.h"
#include "concepts.h"
#include "options.h"
#include "module_archive.h"
#include "preprocessor.h"
#include "symbol.h"
#include "symbol_table.h"
#include "type.h"
#include "type_member.h"
#include "vector.h"

static int g_failures = 0;

#define CHECK(cond)                                                   \
  do {                                                                \
    if (!(cond)) {                                                    \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      g_failures++;                                                   \
    }                                                                 \
  } while (0)

// Builds a temp file path inside the test's writable tmp dir.
static void MakeTempNamedPath(char* out, size_t out_len, const char* name) {
  const char* dir = getenv("TEST_TMPDIR");
  if (dir == NULL || dir[0] == '\0') {
    dir = "/tmp";
  }
  snprintf(out, out_len, "%s/%s", dir, name);
}

static void MakeTempPath(char* out, size_t out_len) {
  MakeTempNamedPath(out, out_len, "roundtrip.dcm");
}

// --- Graph construction -----------------------------------------------------

static Symbol* BuildVariable(TypeRecord* const_int) {
  Symbol* answer = NewSymbol("answer", const_int, STO(static));
  answer->namespace_ = compiler->global_namespace;
  answer->flags.is_defined = true;
  answer->flags.value_set = true;
  answer->value.ivalue = 42;
  answer->alignment = 16;
  Attribute* attr = NewAttribute("aligned");
  AttributeAddArg(attr, "16", 2);
  SymbolAddAttribute(answer, attr);
  return answer;
}

static Symbol* BuildStructVariable(TypeRecord* int_type) {
  Struct* point = NewStruct(false);
  point->tag_name = NewString("Point");

  Symbol* x_sym = NewSymbol("x", int_type, STO(implicit));
  StructMember* x = NewStructMember(x_sym);
  x->byte_offset = 0;
  x->index = 0;
  VectorAppend(&point->members, x);

  Symbol* y_sym = NewSymbol("y", int_type, STO(implicit));
  StructMember* y = NewStructMember(y_sym);
  y->byte_offset = 4;
  y->index = 1;
  VectorAppend(&point->members, y);

  point->size = 8;
  point->alignment = 4;

  TypeRecord* struct_type = NewTypeRecord(kTypeStruct, kQualPlain);
  struct_type->info.struct_info = point;

  Symbol* p = NewSymbol("p", struct_type, STO(static));
  p->namespace_ = compiler->global_namespace;
  return p;
}

static Symbol* BuildEnumVariable(void) {
  Enum* color = NewEnum();
  color->tag_name = NewString("Color");
  color->is_scoped = true;

  Symbol* red = NewEnumConstant("RED", 0);
  Symbol* green = NewEnumConstant("GREEN", 5);
  VectorAppend(&color->constants, red);
  VectorAppend(&color->constants, green);
  color->next_value = 6;

  TypeRecord* enum_type = NewTypeRecord(kTypeEnum | kTypeInt, kQualPlain);
  enum_type->info.enum_info = color;

  Symbol* c = NewSymbol("c", enum_type, STO(static));
  c->namespace_ = compiler->global_namespace;
  return c;
}

static Symbol* BuildFunction(TypeRecord* int_type) {
  // int add() { 2 + 3; }
  ASTNode* lhs = NewIntConstantASTNode(2, int_type, 0);
  ASTNode* rhs = NewIntConstantASTNode(3, int_type, 0);
  ASTNode* sum = NewBinaryASTNode(AST_OP(plus), int_type, 0, lhs, rhs);
  ASTNode* stmt = NewExpressionStatementASTNode(sum, 0);
  Vector* stmts = NewVector();
  VectorAppend(stmts, stmt);
  ASTNode* body = NewCompoundStatementASTNode(stmts, 0);

  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, int_type);
  func_type->info.function.definition = true;
  func_type->info.function.is_inline = true;
  func_type->info.function.body = body;

  Symbol* add = NewSymbol("add", func_type, STO(implicit));
  add->namespace_ = compiler->global_namespace;
  add->flags.is_defined = true;
  add->flags.is_inline_defn = true;
  func_type->info.function.symbol = add;
  return add;
}

static TemplateParameter* MakeTypeParam(const char* name, int index) {
  TemplateParameter* p = (TemplateParameter*)calloc(1, sizeof(*p));
  StringInit(&p->name, name);
  p->kind = kTemplateParameterType;
  p->index = index;
  p->default_template_parameter_index = -1;
  return p;
}

// A C++20 concept: `template<typename T> concept Integral = <expr>;`.
static Symbol* BuildConceptSymbol(TypeRecord* int_type) {
  Vector* params = NewVector();
  VectorAppend(params, MakeTypeParam("T", 0));
  ConstraintExpr* body =
      NewAtomicConstraint(NewIntConstantASTNode(1, int_type, 0), 0);
  Concept* concept = NewConcept("Integral", params, body, 0);

  Symbol* sym = NewSymbol("Integral", NULL, STO(implicit));
  sym->namespace_ = compiler->global_namespace;
  sym->flags.is_concept = true;
  sym->concept_definition = concept;
  return sym;
}

// A C++ variable template: `template<typename T> constexpr int pi_v = 7;`.
static Symbol* BuildVariableTemplate(TypeRecord* int_type) {
  Symbol* sym = NewSymbol("pi_v", int_type, STO(static));
  sym->namespace_ = compiler->global_namespace;
  sym->flags.is_template = true;

  VariableTemplate* vt = (VariableTemplate*)malloc(sizeof(VariableTemplate));
  vt->initializer = NewIntConstantASTNode(7, int_type, 0);
  vt->associated_constraint = NULL;
  VectorInit(&vt->parameters);
  VectorAppend(&vt->parameters, MakeTypeParam("T", 0));
  sym->variable_template = vt;
  return sym;
}

// A constrained variable template whose requires-clause is a concept-id.
static Symbol* BuildConstrainedVariableTemplate(TypeRecord* int_type,
                                                Symbol* concept_sym) {
  Symbol* sym = NewSymbol("constrained_pi_v", int_type, STO(static));
  sym->namespace_ = compiler->global_namespace;
  sym->flags.is_template = true;

  VariableTemplate* vt = (VariableTemplate*)malloc(sizeof(VariableTemplate));
  vt->initializer = NewIntConstantASTNode(3, int_type, 0);
  vt->associated_constraint = NULL;
  VectorInit(&vt->parameters);
  TemplateParameter* tp = MakeTypeParam("T", 0);
  VectorAppend(&vt->parameters, tp);

  Vector* args = NewVector();
  TemplateArgument* arg = (TemplateArgument*)calloc(1, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->type = int_type;
  arg->template_parameter_index = 0;
  VectorAppend(args, arg);
  vt->associated_constraint = NewConceptIdConstraint(concept_sym, args, 0);
  sym->variable_template = vt;
  return sym;
}

// A constrained class template with a requires-clause on the Struct body.
static Symbol* BuildConstrainedClassTemplate(TypeRecord* int_type,
                                             Symbol* concept_sym) {
  Struct* box = NewStruct(false);
  box->is_class = true;
  box->is_template = true;
  box->tag_name = NewString("ConstrainedBox");
  box->template_parameter_count = 1;
  VectorAppend(&box->template_parameters, MakeTypeParam("T", 0));

  Vector* args = NewVector();
  TemplateArgument* arg = (TemplateArgument*)calloc(1, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->type = int_type;
  arg->template_parameter_index = 0;
  VectorAppend(args, arg);
  box->associated_constraint = NewConceptIdConstraint(concept_sym, args, 0);

  TypeRecord* struct_type = NewTypeRecord(kTypeStruct, kQualPlain);
  struct_type->info.struct_info = box;

  Symbol* tag = NewSymbol("ConstrainedBox", struct_type, STO(implicit));
  tag->namespace_ = compiler->global_namespace;
  tag->flags.is_template = true;
  tag->flags.is_defined = true;
  box->tag_symbol = tag;
  return tag;
}

// A constrained function template whose requires-clause is a concept-id
// referencing `concept_sym`, whose template parameter carries its own atomic
// constraint, and whose instantiation cache holds one entry.
static Symbol* BuildConstrainedFunction(TypeRecord* int_type,
                                        Symbol* concept_sym) {
  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, int_type);
  FunctionInfo* fn = &func_type->info.function;

  TemplateParameter* tp = MakeTypeParam("T", 0);
  tp->associated_constraint =
      NewAtomicConstraint(NewIntConstantASTNode(1, int_type, 0), 0);
  VectorAppend(&fn->template_parameters, tp);
  fn->template_parameter_count = 1;

  Vector* args = NewVector();
  TemplateArgument* arg = (TemplateArgument*)calloc(1, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->type = int_type;
  arg->template_parameter_index = 0;
  VectorAppend(args, arg);
  fn->associated_constraint = NewConceptIdConstraint(concept_sym, args, 0);

  Symbol* sym = NewSymbol("constrained_fn", func_type, STO(implicit));
  sym->namespace_ = compiler->global_namespace;
  fn->symbol = sym;

  // Cache entry (a Symbol reference, shared with the concept root here).
  VectorAppend(&fn->template_instantiations, concept_sym);
  return sym;
}

// --- Assertions -------------------------------------------------------------

static StructMember* FindMember(Struct* s, const char* name) {
  for (size_t i = 0; i < s->members.length; i++) {
    StructMember* m = (StructMember*)VectorGet(&s->members, i);
    if (m->symbol != NULL && StringEqual(&m->symbol->name, name)) {
      return m;
    }
  }
  return NULL;
}

static void CheckVariable(Symbol* answer) {
  CHECK(answer != NULL);
  if (answer == NULL) return;
  CHECK(StringEqual(&answer->name, "answer"));
  CHECK(answer->storage == STO(static));
  CHECK(answer->flags.is_defined);
  CHECK(answer->flags.value_set);
  CHECK(answer->value.ivalue == 42);
  CHECK(answer->alignment == 16);
  CHECK(answer->type != NULL);
  if (answer->type != NULL) {
    CHECK(answer->type->declarator == kDeclPrimitive);
    CHECK((answer->type->type & kTypeInt) != 0);
    CHECK((answer->type->qualifiers & kQualConst) != 0);
  }
  CHECK(answer->attributes.length == 1);
  if (answer->attributes.length == 1) {
    Attribute* attr = (Attribute*)VectorGet(&answer->attributes, 0);
    CHECK(StringEqual(&attr->name, "aligned"));
    CHECK(AttributeArgCount(attr) == 1);
    const char* arg = AttributeArgString(attr, 0);
    CHECK(arg != NULL && strcmp(arg, "16") == 0);
  }
}

static void CheckStructVariable(Symbol* p) {
  CHECK(p != NULL);
  if (p == NULL) return;
  CHECK(StringEqual(&p->name, "p"));
  CHECK(p->type != NULL && (p->type->type & kTypeStruct) != 0);
  if (p->type == NULL) return;
  Struct* point = p->type->info.struct_info;
  CHECK(point != NULL);
  if (point == NULL) return;
  CHECK(point->tag_name != NULL && StringEqual(point->tag_name, "Point"));
  CHECK(point->size == 8);
  CHECK(point->members.length == 2);
  StructMember* x = FindMember(point, "x");
  StructMember* y = FindMember(point, "y");
  CHECK(x != NULL && x->byte_offset == 0);
  CHECK(y != NULL && y->byte_offset == 4);
  CHECK(FindStructMemberByName(point, "x") != NULL);
  CHECK(FindStructMemberByName(point, "y") != NULL);
}

static void CheckEnumVariable(Symbol* c) {
  CHECK(c != NULL);
  if (c == NULL) return;
  CHECK(StringEqual(&c->name, "c"));
  CHECK(c->type != NULL && (c->type->type & kTypeEnum) != 0);
  if (c->type == NULL) return;
  Enum* color = c->type->info.enum_info;
  CHECK(color != NULL);
  if (color == NULL) return;
  CHECK(color->tag_name != NULL && StringEqual(color->tag_name, "Color"));
  CHECK(color->is_scoped);
  CHECK(color->constants.length == 2);
  if (color->constants.length == 2) {
    Symbol* red = (Symbol*)VectorGet(&color->constants, 0);
    Symbol* green = (Symbol*)VectorGet(&color->constants, 1);
    CHECK(StringEqual(&red->name, "RED"));
    CHECK(StringEqual(&green->name, "GREEN"));
    CHECK(green->value.ivalue == 5);
  }
}

static void CheckFunction(Symbol* add) {
  CHECK(add != NULL);
  if (add == NULL) return;
  CHECK(StringEqual(&add->name, "add"));
  CHECK(add->flags.is_inline_defn);
  CHECK(add->type != NULL && add->type->declarator == kDeclFunction);
  if (add->type == NULL) return;
  FunctionInfo* fn = &add->type->info.function;
  CHECK(fn->definition);
  CHECK(fn->is_inline);
  // Return type chained.
  CHECK(add->type->next != NULL && (add->type->next->type & kTypeInt) != 0);
  // Body: compound { exprstmt(2 + 3); }
  ASTNode* body = fn->body;
  CHECK(body != NULL && body->op == AST_OP(compound));
  if (body == NULL || body->op != AST_OP(compound)) return;
  CompoundStatementASTNode* cs = (CompoundStatementASTNode*)body;
  CHECK(cs->statements != NULL && cs->statements->length == 1);
  if (cs->statements == NULL || cs->statements->length != 1) return;
  ASTNode* stmt = (ASTNode*)VectorGet(cs->statements, 0);
  CHECK(stmt->op == AST_OP(expr));
  ExpressionStatementASTNode* es = (ExpressionStatementASTNode*)stmt;
  ASTNode* sum = es->expr;
  CHECK(sum != NULL && sum->op == AST_OP(plus));
  if (sum == NULL || sum->op != AST_OP(plus)) return;
  BinaryASTNode* bin = (BinaryASTNode*)sum;
  CHECK(bin->left != NULL && bin->left->op == AST_OP(number));
  CHECK(bin->right != NULL && bin->right->op == AST_OP(number));
  if (bin->left != NULL && bin->left->op == AST_OP(number)) {
    CHECK(((ConstantASTNode*)bin->left)->value.ivalue == 2);
  }
  if (bin->right != NULL && bin->right->op == AST_OP(number)) {
    CHECK(((ConstantASTNode*)bin->right)->value.ivalue == 3);
  }
}

static void CheckConceptSymbol(Symbol* sym) {
  CHECK(sym != NULL);
  if (sym == NULL) return;
  CHECK(StringEqual(&sym->name, "Integral"));
  CHECK(sym->flags.is_concept);
  Concept* concept = sym->concept_definition;
  CHECK(concept != NULL);
  if (concept == NULL) return;
  CHECK(StringEqual(&concept->name, "Integral"));
  CHECK(concept->template_parameters != NULL &&
        concept->template_parameters->length == 1);
  if (concept->template_parameters != NULL &&
      concept->template_parameters->length == 1) {
    TemplateParameter* p =
        (TemplateParameter*)VectorGet(concept->template_parameters, 0);
    CHECK(StringEqual(&p->name, "T"));
    CHECK(p->kind == kTemplateParameterType);
  }
  CHECK(concept->constraint != NULL &&
        concept->constraint->kind == kConstraintAtomic);
  if (concept->constraint != NULL &&
      concept->constraint->kind == kConstraintAtomic) {
    CHECK(concept->constraint->as.atomic.expr != NULL);
  }
}

static void CheckVariableTemplate(Symbol* sym) {
  CHECK(sym != NULL);
  if (sym == NULL) return;
  CHECK(StringEqual(&sym->name, "pi_v"));
  CHECK(sym->flags.is_template);
  VariableTemplate* vt = sym->variable_template;
  CHECK(vt != NULL);
  if (vt == NULL) return;
  CHECK(vt->initializer != NULL &&
        vt->initializer->op == AST_OP(number));
  CHECK(vt->parameters.length == 1);
  if (vt->parameters.length == 1) {
    TemplateParameter* p = (TemplateParameter*)VectorGet(&vt->parameters, 0);
    CHECK(StringEqual(&p->name, "T"));
  }
}

static void CheckConstrainedVariableTemplate(Symbol* sym, Symbol* concept_sym) {
  CHECK(sym != NULL);
  if (sym == NULL) return;
  CHECK(StringEqual(&sym->name, "constrained_pi_v"));
  CHECK(sym->flags.is_template);
  VariableTemplate* vt = sym->variable_template;
  CHECK(vt != NULL);
  if (vt == NULL) return;
  CHECK(vt->associated_constraint != NULL &&
        vt->associated_constraint->kind == kConstraintConceptId);
  if (vt->associated_constraint != NULL &&
      vt->associated_constraint->kind == kConstraintConceptId) {
    CHECK(vt->associated_constraint->as.concept_id.concept_symbol ==
          concept_sym);
  }
}

static void CheckConstrainedClassTemplate(Symbol* sym, Symbol* concept_sym) {
  CHECK(sym != NULL);
  if (sym == NULL) return;
  CHECK(StringEqual(&sym->name, "ConstrainedBox"));
  CHECK(sym->flags.is_template);
  CHECK(sym->type != NULL && (sym->type->type & kTypeStruct) != 0);
  if (sym->type == NULL) return;
  Struct* box = sym->type->info.struct_info;
  CHECK(box != NULL);
  if (box == NULL) return;
  CHECK(box->is_template);
  CHECK(box->template_parameters.length == 1);
  CHECK(box->associated_constraint != NULL &&
        box->associated_constraint->kind == kConstraintConceptId);
  if (box->associated_constraint != NULL &&
      box->associated_constraint->kind == kConstraintConceptId) {
    CHECK(box->associated_constraint->as.concept_id.concept_symbol ==
          concept_sym);
  }
}

static void CheckConstrainedFunction(Symbol* sym, Symbol* concept_sym) {
  CHECK(sym != NULL);
  if (sym == NULL) return;
  CHECK(StringEqual(&sym->name, "constrained_fn"));
  CHECK(sym->type != NULL && sym->type->declarator == kDeclFunction);
  if (sym->type == NULL) return;
  FunctionInfo* fn = &sym->type->info.function;

  // Template parameter with its own atomic constraint.
  CHECK(fn->template_parameters.length == 1);
  if (fn->template_parameters.length == 1) {
    TemplateParameter* p =
        (TemplateParameter*)VectorGet(&fn->template_parameters, 0);
    CHECK(p->associated_constraint != NULL &&
          p->associated_constraint->kind == kConstraintAtomic);
  }

  // Requires-clause: a concept-id referencing the shared concept symbol.
  CHECK(fn->associated_constraint != NULL &&
        fn->associated_constraint->kind == kConstraintConceptId);
  if (fn->associated_constraint != NULL &&
      fn->associated_constraint->kind == kConstraintConceptId) {
    CHECK(fn->associated_constraint->as.concept_id.concept_symbol ==
          concept_sym);
    Vector* args = fn->associated_constraint->as.concept_id.arguments;
    CHECK(args != NULL && args->length == 1);
    if (args != NULL && args->length == 1) {
      TemplateArgument* a = (TemplateArgument*)VectorGet(args, 0);
      CHECK(a->kind == kTemplateParameterType);
      CHECK(a->template_parameter_index == 0);
    }
  }

  // Instantiation cache entry shared with the concept root.
  CHECK(fn->template_instantiations.length == 1);
  if (fn->template_instantiations.length == 1) {
    CHECK((Symbol*)VectorGet(&fn->template_instantiations, 0) == concept_sym);
  }
}

int main(void) {
  Vector options;
  VectorInit(&options);
  CompilerOptionValue* target = calloc(1, sizeof(CompilerOptionValue));
  target->opt = kOptionTarget;
  StringInit(&target->value.svalue, "x86_64");
  VectorAppend(&options, target);

  compiler = malloc(sizeof(Compiler));
  CompilerInitFromString(compiler, "roundtrip", "", &options);
  CreateGlobalSymbolTables();

  TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualPlain);
  int_type->size = 4;
  TypeRecord* const_int = NewTypeRecord(kTypeInt, kQualConst);
  const_int->size = 4;

  Symbol* concept_sym = BuildConceptSymbol(int_type);

  Vector roots;
  VectorInit(&roots);
  VectorAppend(&roots, BuildVariable(const_int));
  VectorAppend(&roots, BuildStructVariable(int_type));
  VectorAppend(&roots, BuildEnumVariable());
  VectorAppend(&roots, BuildFunction(int_type));
  VectorAppend(&roots, concept_sym);
  VectorAppend(&roots, BuildVariableTemplate(int_type));
  VectorAppend(&roots, BuildConstrainedFunction(int_type, concept_sym));
  VectorAppend(&roots, BuildConstrainedVariableTemplate(int_type, concept_sym));
  VectorAppend(&roots, BuildConstrainedClassTemplate(int_type, concept_sym));

  char path[4096];
  MakeTempPath(path, sizeof(path));

  String dependency;
  String reexport;
  StringInit(&dependency, "roundtrip_dependency");
  StringInit(&reexport, "roundtrip_reexport");
  Vector dependencies;
  Vector reexports;
  Vector header_macros;
  VectorInit(&dependencies);
  VectorInit(&reexports);
  VectorInit(&header_macros);
  VectorAppend(&dependencies, &dependency);
  VectorAppend(&dependencies, &reexport);
  VectorAppend(&reexports, &reexport);
  Vector macro_args;
  VectorInit(&macro_args);
  VectorAppend(&macro_args, NewString("x"));
  String macro_replacement;
  StringInit(&macro_replacement, "tokenized replacement");
  Macro* header_macro =
      NewMacro("ROUNDTRIP_MACRO", true, false, &macro_args,
               &macro_replacement, SOURCE_LOCATION_MISSING);
  VectorDestruct(&macro_args);
  VectorAppend(&header_macros, header_macro);

  ModuleWriteRequest req = {
      .module_name = "roundtrip_module",
      .target_triple = "x86_64-unknown-none",
      .compiler_version = "davecc-test",
      .flags = kModuleArchiveHeaderUnit,
      .root_symbols = &roots,
      .dependencies = &dependencies,
      .reexports = &reexports,
      .header_macros = &header_macros,
  };
  CHECK(ModuleWrite(path, &req));

  char malformed_path[4096];
  MakeTempNamedPath(malformed_path, sizeof(malformed_path), "malformed.dcm");
  FILE* malformed = fopen(malformed_path, "wb");
  CHECK(malformed != NULL);
  if (malformed != NULL) {
    static const char bad_archive[] = "not a DaveCC module archive";
    CHECK(fwrite(bad_archive, 1, sizeof(bad_archive), malformed) ==
          sizeof(bad_archive));
    CHECK(fclose(malformed) == 0);
    LoadedModule rejected;
    CHECK(!ModuleLoad(malformed_path, &rejected));
  }

  char truncated_path[4096];
  MakeTempNamedPath(truncated_path, sizeof(truncated_path), "truncated.dcm");
  FILE* valid_file = fopen(path, "rb");
  FILE* truncated_file = fopen(truncated_path, "wb");
  CHECK(valid_file != NULL);
  CHECK(truncated_file != NULL);
  if (valid_file != NULL && truncated_file != NULL) {
    unsigned char prefix[96];
    size_t prefix_len = fread(prefix, 1, sizeof(prefix), valid_file);
    CHECK(prefix_len > 0);
    CHECK(fwrite(prefix, 1, prefix_len, truncated_file) == prefix_len);
  }
  if (valid_file != NULL) {
    CHECK(fclose(valid_file) == 0);
  }
  if (truncated_file != NULL) {
    CHECK(fclose(truncated_file) == 0);
    LoadedModule rejected;
    CHECK(!ModuleLoad(truncated_path, &rejected));
  }

  LoadedModule loaded;
  CHECK(ModuleLoad(path, &loaded));
  if (g_failures == 0) {
    CHECK(loaded.format_version == MODULE_FORMAT_VERSION);
    CHECK(StringEqual(&loaded.module_name, "roundtrip_module"));
    CHECK(StringEqual(&loaded.target_triple, "x86_64-unknown-none"));
    CHECK((loaded.flags & kModuleArchiveHeaderUnit) != 0);
    CHECK(loaded.dependencies.length == 2);
    CHECK(loaded.reexports.length == 1);
    if (loaded.dependencies.length == 2 && loaded.reexports.length == 1) {
      CHECK(StringEqual((String*)VectorGet(&loaded.dependencies, 0),
                        "roundtrip_dependency"));
      CHECK(StringEqual((String*)VectorGet(&loaded.dependencies, 1),
                        "roundtrip_reexport"));
      CHECK(StringEqual((String*)VectorGet(&loaded.reexports, 0),
                        "roundtrip_reexport"));
    }
    CHECK(loaded.header_macros.length == 1);
    if (loaded.header_macros.length == 1) {
      Macro* loaded_macro =
          (Macro*)VectorGet(&loaded.header_macros, 0);
      CHECK(StringEqual(&loaded_macro->name, "ROUNDTRIP_MACRO"));
      CHECK(loaded_macro->is_function_like);
      CHECK(!loaded_macro->varargs);
      CHECK(loaded_macro->args.length == 1);
      if (loaded_macro->args.length == 1) {
        CHECK(StringEqual((String*)VectorGet(&loaded_macro->args, 0), "x"));
      }
      CHECK(StringEqual(&loaded_macro->replacement_text,
                        "tokenized replacement"));
    }
    CHECK(loaded.root_symbols.length == 9);
    if (loaded.root_symbols.length == 9) {
      CheckVariable((Symbol*)VectorGet(&loaded.root_symbols, 0));
      CheckStructVariable((Symbol*)VectorGet(&loaded.root_symbols, 1));
      CheckEnumVariable((Symbol*)VectorGet(&loaded.root_symbols, 2));
      CheckFunction((Symbol*)VectorGet(&loaded.root_symbols, 3));
      Symbol* loaded_concept = (Symbol*)VectorGet(&loaded.root_symbols, 4);
      CheckConceptSymbol(loaded_concept);
      CheckVariableTemplate((Symbol*)VectorGet(&loaded.root_symbols, 5));
      CheckConstrainedFunction((Symbol*)VectorGet(&loaded.root_symbols, 6),
                               loaded_concept);
      CheckConstrainedVariableTemplate(
          (Symbol*)VectorGet(&loaded.root_symbols, 7), loaded_concept);
      CheckConstrainedClassTemplate(
          (Symbol*)VectorGet(&loaded.root_symbols, 8), loaded_concept);
    }
    // Shared namespace round-trips to a single shared object.
    if (loaded.root_symbols.length == 9) {
      Symbol* a = (Symbol*)VectorGet(&loaded.root_symbols, 0);
      Symbol* p = (Symbol*)VectorGet(&loaded.root_symbols, 1);
      CHECK(a->namespace_ != NULL && a->namespace_ == p->namespace_);
    }
    LoadedModuleReleaseGraph(&loaded);
    CompilerDelete(compiler);
    compiler = NULL;
    LoadedModuleDestruct(&loaded);
    VectorDestruct(&dependencies);
    VectorDestruct(&reexports);
    MacroDestruct(header_macro);
    free(header_macro);
    VectorDestruct(&header_macros);
    StringDestruct(&macro_replacement);
    StringDestruct(&dependency);
    StringDestruct(&reexport);
  }

  VectorDestruct(&roots);

  if (g_failures != 0) {
    fprintf(stderr, "%d module_archive check(s) failed\n", g_failures);
    return 1;
  }
  printf("module_archive_test: all checks passed\n");
  return 0;
}
