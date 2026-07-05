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
#include "options.h"
#include "module_archive.h"
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

// Builds a temp file path inside the test's writable tmp dir.
static void MakeTempPath(char* out, size_t out_len) {
  const char* dir = getenv("TEST_TMPDIR");
  if (dir == NULL || dir[0] == '\0') {
    dir = "/tmp";
  }
  snprintf(out, out_len, "%s/roundtrip.dcm", dir);
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

  Vector roots;
  VectorInit(&roots);
  VectorAppend(&roots, BuildVariable(const_int));
  VectorAppend(&roots, BuildStructVariable(int_type));
  VectorAppend(&roots, BuildEnumVariable());
  VectorAppend(&roots, BuildFunction(int_type));

  char path[4096];
  MakeTempPath(path, sizeof(path));

  ModuleWriteRequest req = {
      .module_name = "roundtrip_module",
      .target_triple = "x86_64-unknown-none",
      .compiler_version = "davecc-test",
      .flags = 0,
      .root_symbols = &roots,
  };
  CHECK(ModuleWrite(path, &req));

  LoadedModule loaded;
  CHECK(ModuleLoad(path, &loaded));
  if (g_failures == 0) {
    CHECK(loaded.format_version == MODULE_FORMAT_VERSION);
    CHECK(StringEqual(&loaded.module_name, "roundtrip_module"));
    CHECK(StringEqual(&loaded.target_triple, "x86_64-unknown-none"));
    CHECK(loaded.root_symbols.length == 4);
    if (loaded.root_symbols.length == 4) {
      CheckVariable((Symbol*)VectorGet(&loaded.root_symbols, 0));
      CheckStructVariable((Symbol*)VectorGet(&loaded.root_symbols, 1));
      CheckEnumVariable((Symbol*)VectorGet(&loaded.root_symbols, 2));
      CheckFunction((Symbol*)VectorGet(&loaded.root_symbols, 3));
    }
    // Shared namespace round-trips to a single shared object.
    if (loaded.root_symbols.length == 4) {
      Symbol* a = (Symbol*)VectorGet(&loaded.root_symbols, 0);
      Symbol* p = (Symbol*)VectorGet(&loaded.root_symbols, 1);
      CHECK(a->namespace_ != NULL && a->namespace_ == p->namespace_);
    }
    LoadedModuleDestruct(&loaded);
  }

  VectorDestruct(&roots);

  if (g_failures != 0) {
    fprintf(stderr, "%d module_archive check(s) failed\n", g_failures);
    return 1;
  }
  printf("module_archive_test: all checks passed\n");
  return 0;
}
