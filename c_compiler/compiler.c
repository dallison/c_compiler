//
//  compiler.c
//  c_compiler
//
//  Created by David Allison on 11/1/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "compiler.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "codegen.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "lex.h"
#include "semantics.h"
#include "syntax.h"
#include "debug.h"

#include "6502_target.h"
#include "p_code_target.h"
#include "risc_v_target.h"
#include "aarch64_target.h"
#include "arm_target.h"
#include "x86_64_target.h"

// This is global to avoid having to pass it around everywhere.
Compiler* compiler;

static CompilerOptionDefinition compiler_options[] = {
    {"-g", kCompilerOptionBool, kOptionDebug, false, "Generate debug info"},
    {"-O", kCompilerOptionString, kOptionOptimize, true, "Optimize with level (-O0, -O1, -O2)"},
    {"-target", kCompilerOptionString, kOptionTarget, false, "Specify one target architecture"},
    {"-c", kCompilerOptionBool, kOptionCompileOnly, false, "Compile only to object file"},
    {"-S", kCompilerOptionBool, kOptionAssemblyOutput, false, "Generate assembly language"},
    {"-o", kCompilerOptionString, kOptionOutputFile, false, "Output filename"},
    {"-isystem", kCompilerOptionString, kOptionSystemIncludePath, false, "Add system include path"},
    {"-I", kCompilerOptionString, kOptionIncludePath, true, "Add a user include path -Ipath"},
    {"-D", kCompilerOptionString, kOptionDefineMacro, true, "Define a macro -Dmacro[=value]"},
    {"-U", kCompilerOptionString, kOptionUndefineMacro, true, "Undefine a macro"},
    {"-fPIC", kCompilerOptionBool, kOptionPic, false, "Generate position independent code"},
    {"-fpic", kCompilerOptionBool, kOptionPic, false, "Generate position independent code"},
    {"-Werror", kCompilerOptionBool, kOptionWerror, false, "Warnings are errors"},
    {"-Wall", kCompilerOptionBool, kOptionWall, false, "All warnings producesd"},
    {"-W", kCompilerOptionString, kOptionWarning, true, "Turn warning on or off"},
    {"-error-limit", kCompilerOptionInt, kOptionErrorLimit, false, "Specify max number of errors"},
    {"-ftls-model", kCompilerOptionString, kOptionTlsModel, false, "Use given Thread Local storage model"},
    {"-chdir", kCompilerOptionString, kOptionChdir, false, "Change to dir before compiling"},
    {"-Xfe-print", kCompilerOptionBool, kOptionPrintFrontend, false, "Print fron end dump"},
    {"-Xbe-print", kCompilerOptionBool, kOptionPrintBackend, false, "Print back end dump"},
    {"-Xpp-print", kCompilerOptionBool, kOptionPrintPreprocessor, false, "Print preprocessor dump"},
    {"-Xkeep-asm", kCompilerOptionBool, kOptionKeepAsmFile, false, "Keep assembly file"},
    {"-Xsave-ir", kCompilerOptionBool, kOptionSaveIR, false, "Save IR to .ir file"},
    {"-Xsave-ast", kCompilerOptionBool, kOptionSaveAST, false, "Save AST to .ast file"},
    {NULL, 0, 0, false, NULL},
};

bool OptLevel0(void) {
  return !compiler->optimize || compiler->opt_level == 0;
}

bool OptLevel1(void) {
  return compiler->optimize && compiler->opt_level >= 1;
}

bool OptLevel2(void) {
  return compiler->optimize && compiler->opt_level >= 2;
}

bool OptLevel3(void) {
  return compiler->optimize && compiler->opt_level >= 3;
}

// Add new targets here.
#define kMaxTargetNames 3
static struct CompilerTargetDefinition{
  const char* canonical_name;
  const char* names[kMaxTargetNames];
  CompilerTarget* (*factory)(void);
  bool static_linkage_only;
} compiler_targets[] = {
  {"pcode", {"pcode", "p-code"}, NewPCodeTarget, false},
  {"riscv", {"riscv", "risc-v"}, NewRVTarget, false},
  {"aarch64", {"aarch64", "armv8"}, NewAARCH64Target, false},
  {"arm", {"arm", "armv7", "armv7-a", "arm32"}, NewARMTarget, false},
  {"x86_64", {"x86_64", "x86-64"}, NewX86_64Target, false},
  {"6502", {"6502"}, New6502Target, true},
  {"65c02", {"65c02", "65C02"}, New65c02Target, true},
};

#define kNumTargets (sizeof(compiler_targets) / sizeof(compiler_targets[0]))

static struct CompilerTargetDefinition* FindTarget(String* name) {
  for (size_t i = 0 ; i < kNumTargets; i++) {
    for (size_t j = 0; j < kMaxTargetNames; j++) {
      if (compiler_targets[i].names[j] == NULL) {
        continue;
      }
      if (StringEqual(name, compiler_targets[i].names[j])) {
        return &compiler_targets[i];
      }
    }
  }
  return NULL;
}

void DeleteCompilerTarget(CompilerTarget* t){
  StringDestruct(&t->name);
  free(t);
}

void PrintCompilerHelp(void) {
  printf("DaveCC compiler options\n");
  PrintAllOptions(compiler_options);
  
  // Print help from all targets.
  for (size_t i = 0; i < kNumTargets; i++) {
    CompilerTarget* t = compiler_targets[i].factory();
    if (t->options != NULL) {
      printf("\nOptions for target '%s'\n", t->name.value);
      PrintAllOptions(t->options);
    }
     DeleteCompilerTarget(t);
  }
}

Vector* ParseOptions(int argc, char** argv, Vector* options) {
  // Temporary vector of parts of args.  Values are copied
  // out into other vectors.  Retains ownership of strings.
  Vector strings = {0};
  for (int i = 1; i < argc; i++) {
    // If arg starts with - check for = and split into two.
    char* equals = argv[i][0] != '-' ? NULL : strchr(argv[i], '=');
    if (equals != NULL) {
      CompilerOptionString* str = NewOptionStringWithValue(argv[i], equals - argv[i], equals+1);
      VectorAppend(&strings, str);
    } else {
      // Not an option or no equals.
      VectorAppend(&strings, NewOptionString(argv[i]));
    }
  }

  return ParseOptionSet(compiler_options, &strings, options);
}





static void InitInteger(ASTNode* expr,
                        Initializer* init_out,
                        int offset,
                        Vector* initializers) {
  int64_t value = 0;
  TypeRecord* type = expr->type;
  if (EvaluateIntegerExpression(expr, &value)) {
    if (TypeIsChar(type) || TypeIsBool(type)) {
      init_out->type = kInitTypeByte;
      init_out->value.byte = (uint8_t)value;
    } else if (TypeIsShort(type)) {
      init_out->type = kInitTypeHalf;
      init_out->value.half = (uint16_t)value;
    } else if (TypeIsInt(type)) {
      if (type->size == 2) {
        init_out->type = kInitTypeHalf;
        init_out->value.half = (uint16_t)value;
      } else {
        init_out->type = kInitTypeWord;
        init_out->value.word = (uint32_t)value;
      }
    } else if (TypeIsLong(type)) {
      if (type->size == 4) {
        init_out->type = kInitTypeWord;
        init_out->value.word = (uint32_t)value;
      } else {
        init_out->type = kInitTypeLong;
        init_out->value._long = (uint64_t)value;
      }
    } else if(TypeIsLongLong(type)) {
      init_out->type = kInitTypeLong;
      init_out->value._long = (uint64_t)value;
    } else {
      assert(false);
    }
    init_out->offset = offset;
    VectorAppend(initializers, init_out);
  } else {
    SemanticError(
        expr, "Invalid static initialization; need a constant expression");
  }

}

static void InitFloatingPoint(ASTNode* expr,
                              Initializer* init_out,
                              int offset,
                              Vector* initializers) {
  double value = 0;
  TypeRecord* type = expr->type;
  if (EvaluateFloatingPointExpression(expr, &value)) {
    if (TypeIsFloat(type)) {
      init_out->type = kInitTypeWord;
      float fvalue = (float)value;
      init_out->value.word = *((int32_t*)&fvalue);
    } else if (TypeIsDouble(type) || TypeIsLongDouble(type)) {
      if (type->size == 4) {
        init_out->type = kInitTypeWord;
        float f = value;
        init_out->value.word = *((int32_t*)&f);
      } else {
        init_out->type = kInitTypeLong ;
        init_out->value._long = *((int64_t*)&value);
      }
    } else {
      assert(false);
    }
    init_out->offset = offset;
    VectorAppend(initializers, init_out);
  } else {
    SemanticError(
        expr, "Invalid static initialization; need a constant expression");
  }
}

static void InitScalar(ASTNode* expr, ASTNode* subinit, int offset,
                       Vector* initializers);
static ASTNode* InitCompoundLiteral(ASTNode* node);

static void InitPointer(ASTNode* expr,
                        ASTNode* subinit,
                        Initializer* init_out,
                        int offset,
                        Vector* initializers) {
  if (expr->op == AST_OP(string) || expr->op == AST_OP(string_wide)) {
    // String literal.
    ConstantASTNode* string_node = (ConstantASTNode*)expr;
    int literal_id = CompilerAddStringLiteral(string_node->value.string, expr->op == AST_OP(string_wide));
    init_out->type = kInitTypeString;
    init_out->value.literal_id = literal_id;
    init_out->offset = offset;
    VectorAppend(initializers, init_out);
    return;
  }
  if (expr->op == AST_OP(number)) {
    // Pointers can be initialized by a number.
    InitInteger(expr, init_out, offset, initializers);
    return;
  }
  ASTNode* var_node;
  if (expr->op == AST_OP(address)) {
    UnaryASTNode* addr_node = (UnaryASTNode*)expr;
    var_node = addr_node->sub;
  } else {
    var_node = expr;
  }

  // Allow cast to pointer.
  if (var_node->op == AST_OP(cast)) {
    CastASTNode* c = (CastASTNode*)var_node;
    InitScalar(c->expr, subinit, offset, initializers);
    return;
  }
  if (var_node->op == AST_OP(identifier)) {
    IdentifierASTNode* id_node = (IdentifierASTNode*)var_node;
    init_out->type = kInitTypeSymbol;
    init_out->value.symbol = id_node->symbol;
    init_out->offset = offset;
    VectorAppend(initializers, init_out);
    return;
  }
  if (var_node->op == AST_OP(compound_literal)) {
    // &(foo){..}
    // The CompoundLiteralASTNode contains a symbol.  We take its address.
    InitCompoundLiteral(var_node);
    
    CompoundLiteralASTNode* c = (CompoundLiteralASTNode*)var_node;
    IdentifierASTNode* id_node = (IdentifierASTNode*)c->sym;
    init_out->type = kInitTypeSymbol;
    init_out->value.symbol = id_node->symbol;
    init_out->offset = offset;
    VectorAppend(initializers, init_out);
    return;
  }
  SemanticError(var_node,
                "Illegal static initializer: need address of variable");
}

static void InitScalar(ASTNode* expr, ASTNode* subinit, int offset,
                       Vector* initializers) {
  TypeRecordCalculateSize(expr->type);
  Initializer* init_out = malloc(sizeof(Initializer));
  TypeRecord* type = subinit->type;
  if (TypeIsIntegral(type)) {
    InitInteger(expr, init_out, offset, initializers);
  } else if (TypeIsFloatingPoint(type)) {
    InitFloatingPoint(expr, init_out, offset, initializers);
#if 0
    
  } else if (TypeIsArray(type)) {
    if (expr->op == AST_OP(identifier)) {
      // Initialization with an exising identifier.
      IdentifierASTNode* id_node = (IdentifierASTNode*)expr;
      init_out->type = kInitTypeSymbol;
      init_out->value.symbol = id_node->symbol;
      init_out->offset = offset;
      VectorAppend(initializers, init_out);
    } else if (expr->op == AST_OP(string) || expr->op == AST_OP(string_wide)) {
      // String literal.
      ConstantASTNode* string_node = (ConstantASTNode*)expr;
      int literal_id = CompilerAddStringLiteral(string_node->value.string, expr->op == AST_OP(string_wide));
      init_out->type = kInitTypeString;
      init_out->value.literal_id = literal_id;
      init_out->offset = offset;
      VectorAppend(initializers, init_out);
    } else {
      SemanticError(subinit, "Invalid static initialization");
    }
#endif
  } else if (TypeIsStructOrUnion(subinit->type)) {
    SemanticError(subinit, "Cannot initialize a static struct/union here");
  } else if (TypeIsFunction(type)) {
    // Functions can be used to initialize a function pointer.
    if (TypeIsFunctionPointer(subinit->type)) {
      init_out->type = kInitTypeSymbol;
      init_out->value.symbol = expr->type->info.function.symbol;
      init_out->offset = offset;
      VectorAppend(initializers, init_out);
    } else {
      SemanticError(subinit, "Invalid use of function in initialization");
    }
  } else if (TypeIsPointer(type)) {
    // Pointers can be initialized to the address of an existing static
    // variable of the same type.
    InitPointer(expr, subinit, init_out, offset, initializers);
  } else {
    SemanticError(subinit, "Invalid static initialization");
  }
}

static void InitArray(ASTNode* expr, ASTNode* subinit, int offset,
                      Vector* initializers) {
  TypeRecordCalculateSize(expr->type);
  Initializer* init_out = malloc(sizeof(Initializer));
  ConstantASTNode* string_node = (ConstantASTNode*)expr;
  init_out->type = kInitTypeMemory;
  init_out->offset = offset;
  BufferInit(&init_out->value.memory);
  size_t memory_size = expr->type->size;
  size_t length;
  if (TypeIsInt(expr->type)) {
    // Wide string literal, string length includes 0 at end.
    length = string_node->value.string->length;
  } else {
    // String literal, string length doesn't include \0.
    length = string_node->value.string->length + 1;
  }
  if (length > memory_size) {
    length = memory_size;
  }
  BufferAppend(&init_out->value.memory, string_node->value.string->value,
               length);
  memory_size -= length;
  if (memory_size > 0) {
    // Add padding.
    BufferAddSpace(&init_out->value.memory, memory_size);
  }

  VectorAppend(initializers, init_out);
}

static void ExpandBracedInitializer(BracedInitializerASTNode* init,
                                    int dest_offset, Vector* initializers);

static ASTNode* InitCompoundLiteral(ASTNode* node) {
  CompoundLiteralASTNode* lit = (CompoundLiteralASTNode*)node;
  IdentifierASTNode* sym_node = (IdentifierASTNode*)lit->sym;
  
  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = sym_node->symbol;
  var->is_global = !StorageIs(sym_node->symbol->storage, STO(static));
  VectorInit(&var->initializers);
  var->size = sym_node->symbol->type->size;
  var->is_tls = StorageIs(sym_node->symbol->storage, STO(thread));
  var->is_local = sym_node->symbol->flags.is_local;
  var->alignment = TypeRecordAlignment(sym_node->symbol->type);
  ExpandBracedInitializer((BracedInitializerASTNode*)lit->initializer, 0,
                          &var->initializers);
  VectorAppend(&compiler->initialized_static_variables, var);
  return lit->initializer;
}

// Expand a braced initializer into the vector given.
// The initializer has been simplified into a braced initializer
// containing only designated initializers.
static void ExpandBracedInitializer(BracedInitializerASTNode* init,
                                    int dest_offset, Vector* initializers) {
  for (size_t i = 0; i < init->initializers->length; i++) {
    ASTNode* subinit = (ASTNode*)init->initializers->value.p[i];
    assert(subinit->op == AST_OP(designated_init));

    DesignatedInitializerASTNode* designated_init =
        (DesignatedInitializerASTNode*)subinit;
    int offset = dest_offset;
    if (designated_init->designators != NULL) {
      // We have designators.  Need to build up an offset from the
      // designators.
      for (size_t i = 0; i < designated_init->designators->length; i++) {
        Designator* d = (Designator*)designated_init->designators->value.p[i];
        switch (d->designator_type) {
          case kDesignatorArray:
            // Array index.  Add index * size of lower dimensions to offset.
            offset += d->value.array_index * d->type->size;
            break;
          case kDesignatorStruct:
            offset += d->value.struct_member->byte_offset;
            break;
        }
      }
    }
    if (designated_init->init->op == AST_OP(compound_literal)) {
      ASTNode* initval = InitCompoundLiteral(designated_init->init);
      ExpandBracedInitializer((BracedInitializerASTNode*)initval, offset, initializers);
    } else if (TypeIsArray(designated_init->base.type)) {
      InitArray(designated_init->init, subinit, offset, initializers);
    } else {
      // Whole struct is not possible at static level since they
      // are not compile-time constants.
      InitScalar(designated_init->init, subinit, offset, initializers);
    }
  }
}

static void AssignScalarValueToConst(Symbol* symbol, BracedInitializerASTNode* init) {
  TypeRecord* type = symbol->type;
  if (!TypeIsScalar(type)) {
    return;
  }
  if ((type->qualifiers & kQualConst) == 0) {
    return;
  }
  // Take first element of braced initializer and assign it to the value of
  // the symbol.
  if (init->initializers->length != 1) {
    return;
  }
  ASTNode* subinit = (ASTNode*)init->initializers->value.p[0];
  assert(subinit->op == AST_OP(designated_init));

  DesignatedInitializerASTNode* designated_init =
      (DesignatedInitializerASTNode*)subinit;
  if (TypeIsFloatingPoint(type)) {
    symbol->flags.value_set = EvaluateFloatingPointExpression(designated_init->init, &symbol->value.fvalue);
  } else {
    symbol->flags.value_set = EvaluateIntegerExpression(designated_init->init, &symbol->value.ivalue);
  }
}

static void AddInitializedStaticVariable(VariableDeclarationASTNode* decl,
                                         ASTNode* init) {
  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = decl->symbol;
  var->is_global = !StorageIs(decl->symbol->storage, STO(static));
  VectorInit(&var->initializers);
  var->size = decl->symbol->type->size;
  var->is_tls = StorageIs(decl->symbol->storage, STO(thread));
  var->is_local = decl->symbol->flags.is_local;
  var->alignment = TypeRecordAlignment(decl->symbol->type);
  ExpandBracedInitializer((BracedInitializerASTNode*)init, 0,
                          &var->initializers);
  VectorAppend(&compiler->initialized_static_variables, var);
  AssignScalarValueToConst(decl->symbol, (BracedInitializerASTNode*)init);
}

void InitializerDelete(Initializer* init) {
  if (init->type == kInitTypeMemory) {
    BufferDestruct(&init->value.memory);
  }
  free(init);
}

void InitializedStaticVariableDelete(InitializedStaticVariable* var) {
  for (size_t i = 0; i < var->initializers.length; i++) {
    InitializerDelete(var->initializers.value.p[i]);
  }
  VectorDestruct(&var->initializers);
  free(var);
}

void UninitializedStaticVariableDelete(UninitializedStaticVariable* var) {
  free(var);
}

static void LiteralInit(Literal* lit, LiteralType type) {
  lit->type = type;
  lit->id = compiler->next_literal_id++;
  lit->disabled = false;
}

int CompilerAddStringLiteral(String* value, bool is_wide) {
  StringLiteral* literal = malloc(sizeof(StringLiteral));
  LiteralInit(&literal->base, is_wide ? kLiteralWideString : kLiteralString);
  StringInitFromSegment(&literal->value, value->value, value->length);
  VectorAppend(&compiler->literals, literal);
  return literal->base.id;
}

Literal* CompilerFindLiteral(int literal_id) {
  for (size_t i = 0; i < compiler->literals.length; i++) {
    Literal* literal = compiler->literals.value.p[i];
    if (literal->id == literal_id) {
      return literal;
    }
  }
  return NULL;
}

StringLiteral* CompilerFindStringLiteral(int literal_id) {
  return (StringLiteral*)CompilerFindLiteral(literal_id);
}

void LiteralDelete(Literal* literal) {
  switch (literal->type) {
    case kLiteralString:
    case kLiteralWideString:
      StringDestruct(&((StringLiteral*)literal)->value);
      break;
    case kLiteralBuffer:
      BufferDestruct(&((BufferLiteral*)literal)->value);
      break;
  }
  free(literal);
}

int CompilerAddBufferLiteral(const void* data, size_t length) {
  BufferLiteral* literal = malloc(sizeof(BufferLiteral));
  LiteralInit(&literal->base, kLiteralBuffer);
  BufferInit(&literal->value);
  literal->base.disabled = true;          // Initially disabled.
  BufferAppend(&literal->value, (char*)data, length);
  VectorAppend(&compiler->literals, literal);
  return literal->base.id;
}

BufferLiteral* CompilerFindBufferLiteral(int literal_id) {
  return (BufferLiteral*)CompilerFindLiteral(literal_id);

}

// Add all static variable declarations in the current function and
// to the initailized or uninitialized static output.
static void AddLocalStatics(Syntax* syntax) {
  // The local statics are held in a vector in the Syntax
  // struct.  The vector contains pointer to NewVariableDeclarationASTNode
  // structs.  The vector does not own these pointers.
  for (size_t i = 0; i < syntax->local_statics.length; i++) {
    VariableDeclarationASTNode* decl =
        (VariableDeclarationASTNode*)syntax->local_statics.value.p[i];
    if (decl->initializer == NULL) {
      // No initializer.  Add as unitialized static variable.
      UninitializedStaticVariable* var =
          malloc(sizeof(UninitializedStaticVariable));
      var->symbol = decl->symbol;
      var->is_global = false;
      var->size = decl->symbol->type->size;
      var->alignment = TypeRecordAlignment(decl->symbol->type);
      var->is_tls = StorageIs(decl->symbol->storage, STO(thread));
      var->is_local = decl->symbol->flags.is_local;
      VectorAppend(&compiler->uninitialized_static_variables, var);
    } else {
      BinaryASTNode* init_node = (BinaryASTNode*)decl->initializer;
      AddInitializedStaticVariable(decl, init_node->right);
    }
  }
  VectorClear(&syntax->local_statics);
}

static bool IsFunctionOrInlineDefinition(Symbol* sym) {
  return TypeIsFunctionDefinition(sym->type) || (TypeIsFunction(sym->type) &&
                                            sym->flags.is_inline_defn);
}

static void CheckMainSignature(Syntax* syntax, Symbol* sym) {
  if (!StringEqual(&sym->name, "main")) {
    return;
  }
  TypeRecord* main = sym->type;
  if (!TypeIsInt(main->next)) {
    SyntaxWarning(syntax, "main-ret-type", "main must return an int");
  }
  Vector* prototype = &main->info.function.prototype;
  // Check main args.
  // First arg must be int.
  // Second arg must be pointer to pointer to char.
  // Third arg (if present) must be a pointer to pointer to char.
  if (prototype->length == 0) {
    return;
  }
  if (prototype->length > 3) {
    SyntaxError(syntax, "main can have a max of 3 args");
  }
  if (prototype->length >= 1) {
    TypeRecord* t = ((Symbol*)prototype->value.p[0])->type;
    if (!TypeIsInt(t)) {
      SyntaxError(syntax, "First arg in main must be int");
    }
  }
  if (prototype->length == 1) {
    SyntaxWarning(syntax, "main-arg-count", "main should have 2 or 3 args");
  }
  if (prototype->length >= 2) {
    TypeRecord* t = ((Symbol*)prototype->value.p[1])->type;
    bool ok = false;
    if (TypeIsPointer(t)) {
      t = t->next;
      if (TypeIsPointer(t)) {
        t = t->next;
        if (TypeIsChar(t)) {
          ok = true;
        }
      }
    }
    if (!ok) {
      SyntaxError(syntax, "Second arg in main must be char**");
    }
  }
  if (prototype->length >= 3) {
    TypeRecord* t = ((Symbol*)prototype->value.p[2])->type;
    bool ok = false;
    if (TypeIsPointer(t)) {
      t = t->next;
      if (TypeIsPointer(t)) {
        t = t->next;
        if (TypeIsChar(t)) {
          ok = true;
        }
      }
    }
    if (!ok) {
      SyntaxError(syntax, "Third arg in main must be char**");
    }
  }
}


static void CompileDeclaration(Syntax* syntax) {
  ASTNode* node = SyntaxParseExternalDeclaration(syntax);
  if (node != NULL) {
    // 'node' will be a declaration list containing declarations.
    if (node->op == AST_OP(decl_list)) {
      DeclarationListASTNode* decls = (DeclarationListASTNode*)node;
      size_t num_decls = decls->declarations->length;
      for (size_t i = 0; i < num_decls; i++) {
        VariableDeclarationASTNode* decl =
            (VariableDeclarationASTNode*)decls->declarations->value.p[i];

        if (IsFunctionOrInlineDefinition(decl->symbol)) {
          CheckMainSignature(syntax, decl->symbol);
          
          // This is a function definition, generate the code.
          compiler->current_function = decl->base.type;
          
          // Run the semantic analyzer.
          SemanticAnalyzeFunction(syntax, (ASTNode*)decl);
          if (compiler->print_front_end) {
            SymbolPrintDetails(decl->symbol, true, compiler->ast_output_file);
          }

          // Any semantic errors?
          if (NumErrors() == 0 &&
              (!decl->base.type->info.function.is_inline ||
               decl->symbol->flags.is_inline_defn)) {
            if (compiler->debug_output) {
              decl->symbol->die = BuildDebugInfo(&compiler->debug_builder,
                                           decl->symbol,
                                           DW_TAG(subprogram));
            }
             // Generate code for function.
            Generator codegen;
            GeneratorInit(&codegen, syntax, compiler->current_function);

            // Generate code for function with given target.
            void* code = GenerateFunction(&codegen);

            VectorAppend(&compiler->functions, code);

            if (compiler->debug_output) {
               BuildDebugInfoAfterCodegen(&compiler->debug_builder,
                                            decl->symbol);
             }

            // Handle local static variables.
            AddLocalStatics(syntax);
            GeneratorDestruct(&codegen);
          }
        } else {
          // Declaration is a variable or extern function.
          if (TypeIsFunction(decl->base.type)) {
            // Function declaration, not a definition.
          } else {
            if (StorageIs(decl->symbol->storage, STO(typedef))) {
              if (compiler->debug_output) {
                decl->symbol->die = BuildDebugInfo(&compiler->debug_builder,
                                             decl->symbol,
                                             DW_TAG(typedef));
                BuildDebugInfoAfterCodegen(&compiler->debug_builder,
                                             decl->symbol);
              }
              continue;
            }
            // Declaration is a variable.
            if (TypeIsVoid(decl->symbol->type)) {
              SemanticError((ASTNode*)decl,
                            "Cannot declare a variable with void type");
            }
            if (decl->symbol->flags.is_defined || decl->symbol->flags.is_tentative_decl) {
              // Extern or static variable definition.
              if (decl->initializer == NULL) {
                // No initializer.  Add as unitialized static variable.
                UninitializedStaticVariable* var =
                    malloc(sizeof(UninitializedStaticVariable));
                var->symbol = decl->symbol;
                var->is_global = !StorageIs(decl->symbol->storage, STO(static));
                var->size = decl->symbol->type->size;
                var->alignment = TypeRecordAlignment(decl->symbol->type);
                var->is_tls = StorageIs(decl->symbol->storage, STO(thread));
                var->is_local = decl->symbol->flags.is_local;
                VectorAppend(&compiler->uninitialized_static_variables, var);
              } else {
                decl->symbol->flags.is_tentative_decl = false;
                ASTNode* simplified_init = AnalyzeInitializer(
                    decl->base.type, decl->initializer, true);
                // This is an initialized static variable.  The initializer has
                // been simplified to a braced initializer containing only
                // designated initializers.
                if (compiler->print_front_end) {
                  fprintf(compiler->ast_output_file, "Initializer\n");
                  ASTNodePrint(simplified_init, 0, compiler->ast_output_file);
                }
                AddInitializedStaticVariable(decl, simplified_init);
              }
            }
            if (compiler->debug_output) {
              decl->symbol->die = BuildDebugInfo(&compiler->debug_builder,
                                           decl->symbol,
                                           DW_TAG(variable));
              VariableDIESetStatic(decl->symbol->die, decl->symbol->name.value);
              BuildDebugInfoAfterCodegen(&compiler->debug_builder,
                                           decl->symbol);
            }

          }
        }
      }
    }
  }
}

static void DeclarePredefinedTypesAndMacros(Preprocessor* preprocessor) {
  const char* va_list_typedef =
      "typedef void* __builtin_va_list;\n";
  if (compiler->target != NULL &&
      StringEqual(&compiler->target->name, "x86-64")) {
    va_list_typedef =
        "typedef struct __va_list_tag {"
        " unsigned int __gp_offset;"
        " unsigned int __fp_offset;"
        " void* __overflow_arg_area;"
        " void* __reg_save_area;"
        " } __builtin_va_list;\n";
  }
  String* code = NewString(va_list_typedef);
  StringAppend(code,
      "#define __asm asm\n"
      "#define __asm__ asm\n"
      "#define __attribute __attribute__\n"
      "#define __inline inline\n"
      "#define __signed signed\n"
      "#define __restrict restrict\n"
      "#define __restrict__ restrict\n"
      "\n");

  Lex* old_lex = preprocessor->lex;
  Lex lex;
  LexInitFromString(&lex, "builtin", code, preprocessor);
  LexNextToken(&lex);
  Syntax syntax;
  SyntaxInit(&syntax, &lex);

  while (!LexEof(&lex)) {
    CompileDeclaration(&syntax);
  }

  LexDestruct(&lex);
  preprocessor->lex = old_lex;
  SyntaxDestruct(&syntax);
}

static int CompareWarning(const void* a, const void* b) {
  const char** s1 = (const char**)a;
  const char** s2 = (const char**)b;
  return strcmp(*s1, *s2);
}

// Parse a tls model name to a tls model.
TlsModel ParseTlsModelName(String* tls_model) {
  if (StringEqual(tls_model, "global-dynamic")) {
    return TLS(global_dynamic);
  } else if (StringEqual(tls_model, "local-dynamic")) {
    return TLS(local_dynamic);
  } else if (StringEqual(tls_model, "initial-exec")) {
    return TLS(initial_exec);
  } else if (StringEqual(tls_model, "local-exec")) {
    return TLS(local_exec);
  }
  return TLS(bad);
}

static void InitBasic(Compiler* compiler, const char* filename) {
  StringInit(&compiler->infile, filename);
  VectorInit(&compiler->functions);
  VectorInit(&compiler->initialized_static_variables);
  VectorInit(&compiler->uninitialized_static_variables);
  VectorInit(&compiler->literals);
  SetInit(&compiler->disabled_warnings, CompareWarning);
  compiler->num_errors = 0;
  compiler->next_literal_id = 1;
  compiler->next_symbol_id = 1;
  compiler->current_include_path_index = 0;
  
  char dirname[4096];
  char* wd = getcwd(dirname, sizeof(dirname));
  if (wd == NULL) {
    wd = "unknown";
  }
  DebugBuilderInit(&compiler->debug_builder, filename, "davecc", wd);
}

static void OpenSaveFiles(Compiler* compiler) {
  // Open IR and AST files if required.
  compiler->ir_output_file = stdout;
  if (compiler->save_ir) {
    String ir_filename;
    StringInit(&ir_filename, compiler->infile.value);

    // If the file ends in ".c", make it ".ir", otherwise append ".ir".
    char* suffix = strstr(ir_filename.value, ".c");
    if (suffix == NULL) {
      StringAppend(&ir_filename, ".ir");
    } else {
      // Overwrite 'c' with 'ir'.
      suffix[1] = 'i';
      StringAppendChar(&ir_filename, 'r');
    }
    compiler->ir_output_file = fopen(ir_filename.value, "w");
    if (compiler->ir_output_file == NULL) {
      compiler->ir_output_file = stdout;
    }
  }
  compiler->ast_output_file = stdout;
  if (compiler->save_ast) {
    String ast_filename;
    StringInit(&ast_filename, compiler->infile.value);

    // If the file ends in ".c", make it ".ast", otherwise append ".ast".
    char* suffix = strstr(ast_filename.value, ".c");
    if (suffix == NULL) {
      StringAppend(&ast_filename, ".ast");
    } else {
      // Overwrite 'c' with 'ir'.
      suffix[1] = 'a';
      StringAppend(&ast_filename, "st");
    }
    compiler->ast_output_file = fopen(ast_filename.value, "w");
    if (compiler->ast_output_file == NULL) {
      compiler->ast_output_file = stdout;
    }
  }
}

static void ParseOptimizationOption(Compiler* compiler, Vector* options) {
  compiler->optimize = false;
  String* value = OptionStringValue(kOptionOptimize, options);
  if (value == NULL) {
    return;
  }
  if (value->length == 0) {
    compiler->optimize = true;
    compiler->opt_level = 2;
  } else {
    char level = value->value[0];
    switch (level) {
      case '0':
        break;
      case '1':
      case '2':
      case '3':
        compiler->optimize = true;
        compiler->opt_level = level - '0';
        break;
      default:
        fprintf(stderr, "Invalid optimization level -O%c\n", level);
        exit(1);
        break;
    }
  }
  
}

static void InitBasicOptionsOrDie(Compiler* compiler,
                                  Vector* options, Vector* target_opts) {
  compiler->max_errors = OptionIntValue(kOptionErrorLimit, options, 20);
  compiler->convert_warnings_to_errors =
      OptionBoolValue(kOptionWerror, options, false);
  compiler->enable_all_warnings = OptionBoolValue(kOptionWall, options, false);
  compiler->target_name = OptionStringValue(kOptionTarget, options);
  if (compiler->target_name == NULL) {
    fprintf(stderr, "No target specified; please specify -target option\n");
    exit(1);
  }
  struct CompilerTargetDefinition* target = FindTarget(compiler->target_name);
  if (target == NULL) {
    fprintf(stderr, "Unknown -target architecture %s\n", compiler->target_name->value);
    exit(1);
  }
  compiler->target = target->factory();
  StringSet(compiler->target_name, target->canonical_name);

  compiler->debug_output = OptionBoolValue(kOptionDebug, options, false);
  ParseOptimizationOption(compiler, options);
  compiler->pic = OptionBoolValue(kOptionPic, options, false);
  if (target->static_linkage_only && compiler->pic) {
    fprintf(stderr, "-fPIC is not supported on this target");
    exit(1);
  }
  compiler->tls_model = compiler->pic ? TLS(global_dynamic) : TLS(local_exec);

  compiler->pointer_size = compiler->target->pointer_size;
  compiler->short_size = compiler->target->short_size;
  compiler->bool_size = compiler->target->bool_size;
  compiler->int_size = compiler->target->int_size;
  compiler->long_size = compiler->target->long_size;
  compiler->long_long_size = compiler->target->long_long_size;
  compiler->float_size = compiler->target->float_size;
  compiler->double_size = compiler->target->double_size;
  compiler->wchar_size = compiler->target->wchar_size;
  compiler->code_preference = compiler->target->code_preference;
  compiler->call_return_fixed_reg = compiler->target->call_return_fixed_reg;
  compiler->keep_ssa = compiler->target->keep_ssa;
  compiler->ir_optimizations = compiler->target->ir_optimizations;
  compiler->prepend_underscore = compiler->target->prepend_underscore;
  compiler->plain_char_is_signed = compiler->target->plain_char_is_signed;
  compiler->target_flags = compiler->target->flags;
  
  compiler->alignment = compiler->target->alignment;

  compiler->print_front_end =
      OptionBoolValue(kOptionPrintFrontend, options, false);
  compiler->print_back_end =
      OptionBoolValue(kOptionPrintBackend, options, false);
  compiler->save_ir =
      OptionBoolValue(kOptionSaveIR, options, false);
  compiler->save_ast =
      OptionBoolValue(kOptionSaveAST, options, false);
  compiler->print_preprocessor =
      OptionBoolValue(kOptionPrintPreprocessor, options, false);
  compiler->keep_asm_file = OptionBoolValue(kOptionKeepAsmFile, options, false);
  if (compiler->save_ast) {
    compiler->print_front_end = true;
  }
  if (compiler->save_ir) {
    compiler->print_back_end = true;
  }
  // Parse target-specific options.
  
  // TLS model.
  String* tls_model = OptionStringValue(kOptionTlsModel, options);
  if (tls_model != NULL) {
    compiler->tls_model = ParseTlsModelName(tls_model);
    if (compiler->tls_model == TLS(bad)) {
      fprintf(stderr, "Invalid TLS model value: %s\n", tls_model->value);
      exit(1);
    }
  }
  
  Vector target_options = {0};
  if (target_opts != NULL && compiler->target->options != NULL) {
    // NOTE: this will destruct the target_opts vector and return a new one
    // containing unknown options.
    target_opts = ParseOptionSet(compiler->target->options, target_opts, &target_options);
  }

  if (target_opts != NULL) {
    // Print out unknown options,
    if (target_opts->length != 0) {
      for (size_t i = 0; i < target_opts->length; i++) {
        String* s = target_opts->value.p[i];
        fprintf(stderr, "Unknown option %s\n", s->value);
      }
      exit(1);
    }
    
    VectorDestructWithContents(target_opts, (VectorElementDestructor)StringDestruct, /*free_element=*/true);
  }

  compiler->target->handle_options(&target_options);
  VectorDestructWithContents(&target_options, (VectorElementDestructor)StringDestruct, /*free_element=*/true);
  
  PreprocessorInit(&compiler->preprocessor);
  SyntaxInit(&compiler->syntax, &compiler->lex);
}

// Process macro definition and include path options and process warning
// options
static void InitComplexOptions(Compiler* compiler, Vector* options) {
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* option_value = options->value.p[i];
    switch (option_value->opt) {
      case kOptionIncludePath:
        PreprocessorAddUserIncludePath(&compiler->preprocessor,
                                       option_value->value.svalue.value);
        break;
      case kOptionSystemIncludePath:
        PreprocessorInsertSystemIncludePath(&compiler->preprocessor, 0,
                                            option_value->value.svalue.value);
        break;
      case kOptionDefineMacro: {
        ssize_t equals = StringIndexOf(&option_value->value.svalue, "=");
        String macro_name = {0};
        String macro_value = {0};

        if (equals < 0) {
          StringSet(&macro_name, option_value->value.svalue.value);
          StringSet(&macro_value, "");
        } else {
          StringSubstring(&option_value->value.svalue, 0, equals, &macro_name);
          StringSubstring(&option_value->value.svalue, equals + 1,
                          option_value->value.svalue.length - equals,
                          &macro_name);
        }
        PreprocessorDefineMacro(&compiler->preprocessor, macro_name.value,
                                macro_value.value);
        StringDestruct(&macro_name);
        StringDestruct(&macro_value);
        break;
      }
      case kOptionUndefineMacro:
        PreprocessorUndefineMacro(&compiler->preprocessor,
                                  &option_value->value.svalue);
        break;
      case kOptionWarning:
        if (StringStartsWith(&option_value->value.svalue, "no-")) {
          DisableWarning(option_value->value.svalue.value + 3);
        } else {
          EnableWarning(option_value->value.svalue.value);
        }
        break;
      default:
        break;
    }
  }
  OpenSaveFiles(compiler);
}

static bool CompilerInitCommon(Compiler* compiler, const char* filename,
                               Vector* options, Vector* target_opts) {
  InitBasic(compiler, filename);

  // Basic option initialization.
  InitBasicOptionsOrDie(compiler, options, target_opts);

  // Chdir if asked.
  String* dir = OptionStringValue(kOptionChdir, options);
  if (dir != NULL) {
    int e = chdir(dir->value);
    if (e == -1) {
      fprintf(stderr, "Failed to change directory to %s: %s\n", dir->value,
              strerror(errno));
      exit(1);
    }
  }

  // Define the preprocessor architecture-specific macros.
  PreprocessorDefineArchitectureMacros(&compiler->preprocessor);

  InitComplexOptions(compiler, options);
  return true;
}

void CompilerDestruct(Compiler* compiler) {
  if (compiler->ir_output_file != stdout) {
    fclose(compiler->ir_output_file);
  }
  if (compiler->ast_output_file != stdout) {
    fclose(compiler->ast_output_file);
  }
  DebugBuilderDestruct(&compiler->debug_builder);
  
  ClearSymbolTable(&compiler->global_symbol_table, true);
  ClearSymbolTable(&compiler->global_tag_table, true);

  StringDestruct(&compiler->infile);

  for (size_t i = 0; i < compiler->functions.length; i++) {
    compiler->target->cleanup(compiler->functions.value.p[i]);
  }
  VectorDestruct(&compiler->functions);

  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariableDelete(
        compiler->initialized_static_variables.value.p[i]);
  }
  VectorDestruct(&compiler->initialized_static_variables);

  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    UninitializedStaticVariableDelete(
        compiler->uninitialized_static_variables.value.p[i]);
  }
  VectorDestruct(&compiler->uninitialized_static_variables);

  for (size_t i = 0; i < compiler->literals.length; i++) {
    LiteralDelete(compiler->literals.value.p[i]);
  }
  VectorDestruct(&compiler->literals);

  PreprocessorDestruct(&compiler->preprocessor);
  SyntaxDestruct(&compiler->syntax);
  LexDestruct(&compiler->lex);
}

void CompilerDelete(Compiler* compiler) {
  CompilerDestruct(compiler);
  free(compiler);
}

bool CompilerInitFromFile(Compiler* compiler, const char* filename,
                          Vector* options, Vector* target_opts) {
  bool ok = CompilerInitCommon(compiler, filename, options, target_opts);
  if (!ok) {
    return false;
  }
  ok = LexInitFromFile(&compiler->lex, filename, &compiler->preprocessor);
  if (!ok) {
    return false;
  }
  return true;
}

bool CompilerInitFromString(Compiler* compiler, const char* filename,
                            const char* code, Vector* options) {
  bool ok = CompilerInitCommon(compiler, filename, options, NULL);
  if (!ok) {
    return false;
  }
  String* code_string = NewString(code);
  LexInitFromString(&compiler->lex, filename, code_string,
                    &compiler->preprocessor);

  return true;
}

bool CompilerInitForAssembler(const char* filename, Vector* options) {
  compiler = malloc(sizeof(Compiler));
  bool ok = CompilerInitCommon(compiler, filename, options, NULL);
  if (!ok) {
    return false;
  }
  ok = LexInitFromFile(&compiler->lex, filename, &compiler->preprocessor);
  if (!ok) {
    return false;
  }
  return true;
}

// Emit the assembly language into the filename given, returning true
// if it worked.
static bool EmitAssemblyFile(Compiler* compiler, String* asm_filename) {
  FILE* asm_file = compiler->target->create_asm_file(&compiler->infile, asm_filename);
  
  if (asm_file == NULL) {
    fprintf(stderr, "Unable to open assembler file %s\n",
            asm_filename->value);
    return false;
  }
  for (size_t i = 0; i < compiler->functions.length; i++) {
    // Debug, print to stdout.
    if (compiler->print_back_end) {
      compiler->target->emit_function_assembly(compiler->functions.value.p[i],
                                               stdout);
    }

    compiler->target->emit_function_assembly(compiler->functions.value.p[i],
                                             asm_file);
  }

  // Now emit the data to the assembly file.
  compiler->target->emit_data_start(asm_file);

  bool contains_tls_vars = false;

  // Initialized variables.
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (!var->is_tls) {
      compiler->target->emit_static_variable(var, asm_file);
    }
    contains_tls_vars |= var->is_tls;
  }

  // Uninitialized variables.
  for (size_t i = 0; i < compiler->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        compiler->uninitialized_static_variables.value.p[i];
    if (!var->is_tls && var->symbol->flags.is_tentative_decl) {
      compiler->target->emit_bss_space(var, asm_file);
    }
    contains_tls_vars |= var->is_tls;
  }

  // Emit string literals start.
  compiler->target->emit_literals_start(asm_file);

  // Now the string literals.
  for (size_t i = 0; i < compiler->literals.length; i++) {
    compiler->target->emit_literal(compiler->literals.value.p[i],
                                          asm_file);
  }

  // Emit TLS sections if there is any TLS data.
  if (contains_tls_vars) {
    // .tdata section.
    compiler->target->emit_tdata_start(asm_file);
    for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
      InitializedStaticVariable* var =
          compiler->initialized_static_variables.value.p[i];
      if (var->is_tls) {
        compiler->target->emit_tls_variable(var, asm_file);
      }
    }

    // .tbss section.
    compiler->target->emit_tbss_start(asm_file);

    for (size_t i = 0; i < compiler->uninitialized_static_variables.length;
         i++) {
      UninitializedStaticVariable* var =
          compiler->uninitialized_static_variables.value.p[i];
      if (var->is_tls) {
        compiler->target->emit_tbss_space(var, asm_file);
      }
    }
  }

  if (compiler->debug_output) {
    compiler->target->emit_debug(asm_file);
    compiler->debug_builder.fp = asm_file;
    DebugBuilderEmitDebugInfo(&compiler->debug_builder);
    DebugBuilderEmitAbbreviations(&compiler->debug_builder);
  }

  if (asm_file != stdout) {
    fclose(asm_file);
  }
  return true;
}

// Assemble the asm_filename into an object file and return the name
// of the file generated, in a String allocated from the heap.
static String* Assemble(Compiler* compiler, String* asm_filename, Vector* options) {
  String* object_filename;
  String* output_filename = OptionStringValue(kOptionOutputFile, options);
  // The object file name is either specified as an option or is the
  // name of the source file with .c replaced by .o.
  if (output_filename != NULL) {
    object_filename = NewString(output_filename->value);
  } else {
    object_filename = NewString(compiler->infile.value);

    // If the file ends in ".c", make it ".o", otherwise append ".o".
    char* suffix = strstr(object_filename->value, ".c");
    if (suffix == NULL) {
      StringAppend(object_filename, ".o");
    } else {
      // Overwrite 'c' with 'o'.
      suffix[1] = 'o';
    }
  }

  // Assemble using target-specific assembler.
  bool ok = compiler->target->assemble(asm_filename, object_filename);
  if (!ok) {
    StringDelete(object_filename);
    return NULL;
  }
  return object_filename;
}

// Compile a source file, returning name of object file allocated from
// the heap.  Compiler has already been initialized.
static String* Compile(Compiler* compiler, Vector* options) {
  // Create global symbol tables and predefine internal types.
  CreateGlobalSymbolTables();
  DeclarePredefinedTypesAndMacros(&compiler->preprocessor);
  
  // Main loop.
  LexNextToken(&compiler->lex);
  while (!LexEof(&compiler->lex)) {
    SyntaxResetForNewDeclaration(&compiler->syntax);
    CompileDeclaration(&compiler->syntax);
  }
  
  if (compiler->print_front_end) {
    HashTablePrintStats(&compiler->global_symbol_table, compiler->ast_output_file);
    HashTablePrintStats(&compiler->global_tag_table, compiler->ast_output_file);
    PreprocessorPrintStats(&compiler->preprocessor, compiler->ast_output_file);
  }

  // Abort if there are any errors.
  if (NumErrors() != 0) {
    return NULL;
  }
  
  // Emit the assembly language into a file ending in .s.
  bool output_asm_only = OptionBoolValue(kOptionAssemblyOutput, options, false);
  String asm_filename = {0};
  String* output_filename = OptionStringValue(kOptionOutputFile, options);
  if (output_asm_only && output_filename != NULL) {
    StringInit(&asm_filename, output_filename->value);
  } else {
    StringInit(&asm_filename, compiler->infile.value);

    // If the file ends in ".c", make it ".s", otherwise append ".s".
    char* suffix = strstr(asm_filename.value, ".c");
    if (suffix == NULL) {
      StringAppend(&asm_filename, ".s");
    } else {
      // Overwrite 'c' with 's'.
      suffix[1] = 's';
    }
  }
  bool ok = EmitAssemblyFile(compiler, &asm_filename);
  if (!ok) {
    StringDestruct(&asm_filename);
    return NULL;
  }
  
  // If the user specified -S then we don't assemble the output
  // and we keep the output assembly language file.
  if (output_asm_only) {
    StringDestruct(&asm_filename);
    return output_filename;
  }

  // Run the assembler to assemble into the object file.  Return the
  // name of rhe object file or NULL if something went wrong.
  String* object_filename = Assemble(compiler, &asm_filename, options);
  
  // Remove .s file unless told not to.
  if (!compiler->keep_asm_file) {
    remove(asm_filename.value);
  }
  
  StringDestruct(&asm_filename);
  return object_filename;
}

String* CompileTranslationUnit(const char* filename, Vector* options, Vector* target_opts) {
  ClearAllFiles();

  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromFile(compiler, filename, options, target_opts)) {
    fprintf(stderr, "Cannot open file %s\n", filename);
    return NULL;
  }

  String* object_file = Compile(compiler, options);
  CompilerDelete(compiler);
  compiler = NULL;
  return object_file;
}

String* CompileTranslationUnitFromString(const char* filename, const char* code,
                                         Vector* options) {
  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  CompilerInitFromString(compiler, filename, code, options);
  String* object_file = Compile(compiler, options);
  CompilerDelete(compiler);
  compiler = NULL;
  return object_file;
}

int CharSize() {
  return 1;
}

int IntSize(void) {
  return compiler->int_size;
}
int ShortSize(void) {
  return compiler->short_size;

}

int PointerSize(void) {
  return compiler->pointer_size;

}

int BoolSize(void) {
  return compiler->bool_size;

}

int LongSize(void) {
  return compiler->long_size;

}

int LongLongSize(void) {
  return compiler->long_long_size;
}

int FloatSize(void) {
  return compiler->float_size;
}

int DoubleSize(void) {
  return compiler->double_size;
}
