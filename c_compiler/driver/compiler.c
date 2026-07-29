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
#include "constexpr_pcode.h"
#include "errors.h"
#include "expr_semantics.h"
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "lex.h"
#include "semantics.h"
#include "syntax.h"
#include "debug.h"
#include "member_pointer.h"
#include "type_inheritance.h"
#include "type_compare.h"
#include "type_template.h"

#include "6502_target.h"
#include "p_code_target.h"
#include "risc_v_target.h"
#include "aarch64_target.h"
#include "arm_target.h"
#include "x86_64_target.h"

void EmitInitFiniArrayEntries(Vector* functions, bool is_fini, FILE* fp);

// This is global to avoid having to pass it around everywhere.
Compiler* compiler;

static Vector cxx_init_array_functions;
static Vector cxx_fini_array_functions;

static CompilerOptionDefinition compiler_options[] = {
    {"-g", kCompilerOptionBool, kOptionDebug, false, "Generate debug info"},
    {"-O", kCompilerOptionString, kOptionOptimize, true,
     "Optimize with level (-O0, -O1, -O2, -O3, -Os)"},
    {"-target", kCompilerOptionString, kOptionTarget, false, "Specify one target architecture"},
    {"-c", kCompilerOptionBool, kOptionCompileOnly, false, "Compile only to object file"},
    {"-S", kCompilerOptionBool, kOptionAssemblyOutput, false, "Generate assembly language"},
    {"-o", kCompilerOptionString, kOptionOutputFile, false, "Output filename"},
    {"-isystem", kCompilerOptionString, kOptionSystemIncludePath, false, "Add system include path"},
    {"-I", kCompilerOptionString, kOptionIncludePath, true, "Add a user include path -Ipath"},
    {"-nostdinc", kCompilerOptionBool, kOptionNoStandardIncludes, false,
     "Do not use built-in system include paths"},
    {"-nostdlib", kCompilerOptionBool, kOptionNoStandardLibraries, false,
     "Do not link the target system library"},
    {"-D", kCompilerOptionString, kOptionDefineMacro, true, "Define a macro -Dmacro[=value]"},
    {"-U", kCompilerOptionString, kOptionUndefineMacro, true, "Undefine a macro"},
    {"-fPIC", kCompilerOptionBool, kOptionPic, false, "Generate position independent code"},
    {"-fpic", kCompilerOptionBool, kOptionPic, false, "Generate position independent code"},
    {"-fexceptions", kCompilerOptionBool, kOptionExceptions, false, "Enable C++ exception handling (default)"},
    {"-fno-exceptions", kCompilerOptionBool, kOptionNoExceptions, false, "Disable C++ exception handling"},
    {"-fprintf-specialize", kCompilerOptionBool, kOptionPrintfSpecialize, false,
     "Select smaller printf-family implementations for constant formats"},
    {"-fno-printf-specialize", kCompilerOptionBool,
     kOptionNoPrintfSpecialize, false,
     "Disable constant printf-family format specialization"},
    // All -W* flags are matched by this single prefix entry and interpreted in
    // InitComplexOptions: -W<name>/-Wno-<name> enable/disable, -Wall, -Werror,
    // -Wno-error, and the per-warning -Werror=<name>/-Wno-error=<name>.
    {"-W", kCompilerOptionString, kOptionWarning, true,
     "Control warnings: -W<name>, -Wno-<name>, -Wall, -Werror, -Werror=<name>, -Wno-error[=<name>]"},
    {"-error-limit", kCompilerOptionInt, kOptionErrorLimit, false, "Specify max number of errors"},
    {"-std", kCompilerOptionString, kOptionStandard, false,
     "Select language standard: c89, c99, c11, c17, c++11, c++17, c++20, c++23"},
    {"-ftls-model", kCompilerOptionString, kOptionTlsModel, false, "Use given Thread Local storage model"},
    {"-chdir", kCompilerOptionString, kOptionChdir, false, "Change to dir before compiling"},
    {"-Xfe-print", kCompilerOptionBool, kOptionPrintFrontend, false, "Print fron end dump"},
    {"-Xbe-print", kCompilerOptionBool, kOptionPrintBackend, false, "Print back end dump"},
    {"-Xpp-print", kCompilerOptionBool, kOptionPrintPreprocessor, false, "Print preprocessor dump"},
    {"-Xkeep-asm", kCompilerOptionBool, kOptionKeepAsmFile, false, "Keep assembly file"},
    {"-Xsave-ir", kCompilerOptionBool, kOptionSaveIR, false, "Save IR to .ir file"},
    {"-Xsave-ast", kCompilerOptionBool, kOptionSaveAST, false, "Save AST to .ast file"},
    // Hidden C++20-module hooks: emit/load a post-semantic module (.dcm) file.
    {"-Xemit-module", kCompilerOptionString, kOptionEmitModule, false, "(hidden) Emit a module (.dcm) file"},
    {"-Xload-module", kCompilerOptionString, kOptionLoadModule, false, "(hidden) Load and verify a module (.dcm) file"},
    {"-fprebuilt-module-path", kCompilerOptionString, kOptionPrebuiltModulePath, false, "Search dir for prebuilt .dcm modules"},
    {"-fmodule-file", kCompilerOptionString, kOptionModuleFile, false, "Map module-name=path to a prebuilt .dcm module"},
    {"-fmodule-header", kCompilerOptionBool, kOptionModuleHeader, false, "Compile input as a C++20 header unit"},
    {"-fmodule-name", kCompilerOptionString, kOptionModuleName, false, "Set the logical module or header-unit name"},
    {"-fmodule-output", kCompilerOptionString, kOptionModuleOutput, false, "Emit a .dcm module artifact alongside normal output"},
    {"-fdeps-file", kCompilerOptionString, kOptionDepsFile, false, "Write P1689R5 module dependency information"},
    {"-fdeps-format", kCompilerOptionString, kOptionDepsFormat, false, "Module dependency format (p1689r5)"},
    {"-fdeps-scan-only", kCompilerOptionBool, kOptionDepsScanOnly, false, "Scan module dependencies without compiling"},
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

bool CompilerIsCXX(void) {
  return compiler != NULL &&
         compiler->language_standard >= kLanguageStandardCXX98;
}

bool CompilerCXXAtLeast(LanguageStandard standard) {
  return CompilerIsCXX() && compiler->language_standard >= standard;
}

bool CompilerExceptionsEnabled(void) {
  return compiler != NULL && compiler->exceptions_enabled;
}

// C++20 module import hook, registered by the driver (see SetModuleImportHandler
// in compiler.h for why this indirection exists).
static ModuleImportHandler g_module_import_handler = NULL;
static void* g_module_import_handler_ctx = NULL;
static char g_last_import_error[512];
static const char* g_last_import_error_ptr = NULL;
static void* g_translation_unit_import_state = NULL;
static TranslationUnitImportReleaseFn g_translation_unit_import_release = NULL;

void SetModuleImportHandler(ModuleImportHandler fn, void* ctx) {
  g_module_import_handler = fn;
  g_module_import_handler_ctx = ctx;
}

void CompilerSetImportState(void* state, TranslationUnitImportReleaseFn release) {
  g_translation_unit_import_state = state;
  g_translation_unit_import_release = release;
}

void CompilerSetLastImportError(const char* message) {
  if (message == NULL || message[0] == '\0') {
    g_last_import_error_ptr = NULL;
    g_last_import_error[0] = '\0';
    return;
  }
  snprintf(g_last_import_error, sizeof(g_last_import_error), "%s", message);
  g_last_import_error_ptr = g_last_import_error;
}

const char* CompilerImportLastError(void) {
  return g_last_import_error_ptr;
}

bool CompilerImportModule(const char* module_name) {
  CompilerSetLastImportError(NULL);
  if (g_module_import_handler == NULL) {
    return false;
  }
  bool ok = g_module_import_handler(g_module_import_handler_ctx, module_name);
  return ok;
}

bool CompilerHasModuleImportHandler(void) {
  return g_module_import_handler != NULL;
}

// Add new targets here.
#define kMaxTargetNames 4
static struct CompilerTargetDefinition{
  const char* canonical_name;
  const char* names[kMaxTargetNames];
  CompilerTarget* (*factory)(void);
  bool static_linkage_only;
  int default_opt_level;
} compiler_targets[] = {
  {"pcode", {"pcode", "p-code"}, NewPCodeTarget, false, 0},
  {"riscv", {"riscv", "risc-v"}, NewRVTarget, false, 0},
  {"aarch64", {"aarch64", "armv8"}, NewAARCH64Target, false, 0},
  {"arm", {"arm", "armv7", "armv7-a", "arm32"}, NewARMTarget, false, 0},
  {"x86_64", {"x86_64", "x86-64"}, NewX86_64Target, false, 0},
  {"6502", {"6502"}, New6502Target, true, 2},
  {"65c02", {"65c02", "65C02"}, New65c02Target, true, 2},
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
    if (TypeIsCharFamily(type) || TypeIsBool(type)) {
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
  } else if (TypeIsMemberPointer(subinit->type)) {
    MemberPointerValue pm_value;
    if (expr->op == AST_OP(member_ptr)) {
      UnaryASTNode* unary = (UnaryASTNode*)expr;
      if (unary->sub != NULL && unary->sub->op == AST_OP(structmember)) {
        StructMember* member = ((StructMemberASTNode*)unary->sub)->member;
        Struct* class_info = TypeMemberPointerClass(subinit->type);
        if (member != NULL && class_info != NULL &&
            MemberPointerEncodeFromMember(member, class_info, &pm_value)) {
          free(init_out);
          MemberPointerEmitStaticInitializers(subinit->type, &pm_value, offset,
                                              initializers);
          return;
        }
      }
    }
    if (MemberPointerTryEvaluateConstant(expr, subinit->type, &pm_value)) {
      free(init_out);
      MemberPointerEmitStaticInitializers(subinit->type, &pm_value, offset,
                                          initializers);
      return;
    }
    SemanticError(subinit, "Invalid static pointer-to-member initialization");
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

// Returns the alignment to use for a variable, honoring an explicit
// __attribute__((aligned(N))) override that raises the natural alignment.
static int SymbolEffectiveAlignment(Symbol* sym) {
  int natural = TypeRecordAlignment(sym->type);
  return sym->alignment > natural ? sym->alignment : natural;
}

static ASTNode* InitCompoundLiteral(ASTNode* node) {
  CompoundLiteralASTNode* lit = (CompoundLiteralASTNode*)node;
  IdentifierASTNode* sym_node = (IdentifierASTNode*)lit->sym;
  
  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = sym_node->symbol;
  var->is_global = !StorageIs(sym_node->symbol->storage, STO(static));
  var->is_weak = SymbolHasWeakBinding(sym_node->symbol);
  VectorInit(&var->initializers);
  var->size = sym_node->symbol->type->size;
  var->is_tls = StorageIs(sym_node->symbol->storage, STO(thread));
  var->is_local = sym_node->symbol->flags.is_local;
  var->alignment = SymbolEffectiveAlignment(sym_node->symbol);
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
          case kDesignatorBase:
            offset += d->value.base->byte_offset;
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
  var->is_weak = SymbolHasWeakBinding(decl->symbol);
  VectorInit(&var->initializers);
  var->size = decl->symbol->type->size;
  var->is_tls = StorageIs(decl->symbol->storage, STO(thread));
  var->is_local = decl->symbol->flags.is_local;
  var->alignment = SymbolEffectiveAlignment(decl->symbol);
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

static bool SymbolIsThreadLocal(Symbol* sym) {
  return sym != NULL && StorageIs(sym->storage, STO(thread));
}

static ASTNode* CXXThreadLocalInitializerExpression(ASTNode* initializer) {
  if (initializer == NULL) {
    return NULL;
  }
  if (initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  if (initializer->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  return initializer;
}

static bool CXXThreadLocalInitializerIsDynamic(ASTNode* initializer) {
  ASTNode* expr = CXXThreadLocalInitializerExpression(initializer);
  return expr != NULL && !IsConstantExpression(expr);
}

static ASTNode* CXXThreadLocalDynamicInitStatement(Symbol* sym,
                                                   ASTNode* initializer) {
  SourceLocation location = initializer->location;
  if (initializer->op == AST_OP(init)) {
    return NewExpressionStatementASTNode(initializer, location);
  }
  ASTNode* decl_id = NewIdentifierASTNode(sym, location);
  decl_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* init = NewBinaryASTNode(AST_OP(init), sym->type, location, decl_id,
                                   initializer);
  return NewExpressionStatementASTNode(init, location);
}

static void AddUninitializedLocalStatic(Symbol* symbol) {
  UninitializedStaticVariable* var =
      malloc(sizeof(UninitializedStaticVariable));
  var->symbol = symbol;
  var->is_global = !StorageIs(symbol->storage, STO(static));
  var->is_weak = SymbolHasWeakBinding(symbol);
  var->size = symbol->type->size;
  var->alignment = SymbolEffectiveAlignment(symbol);
  var->is_tls = StorageIs(symbol->storage, STO(thread));
  var->is_local = symbol->flags.is_local || SymbolHasWeakBinding(symbol);
  VectorAppend(&compiler->uninitialized_static_variables, var);
}

static void CollectFunctionLocalStatic(ASTNode* node, void* data, int child_id,
                                       VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(vardecl)) {
    return;
  }
  VariableDeclarationASTNode* declaration =
      (VariableDeclarationASTNode*)node;
  if (declaration->symbol != NULL &&
      StorageIs(declaration->symbol->storage, STO(static)) &&
      !StorageIs(declaration->symbol->storage, STO(thread))) {
    VectorAppend((Vector*)data, declaration);
  }
}

static void FindStaticInitializer(ASTNode* node, void* data, int child_id,
                                  VisitorMode mode) {
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL &&
      node->op == AST_OP(init) && (node->flags & kASTStaticInit) != 0 &&
      *(BinaryASTNode**)data == NULL) {
    *(BinaryASTNode**)data = (BinaryASTNode*)node;
  }
}

// Add all static variable declarations in the current function to the
// initialized or uninitialized static output.  Walking the final body makes
// this work for deferred inline members and instantiated templates too.
static void AddLocalStatics(Syntax* syntax, TypeRecord* function) {
  Vector declarations;
  VectorInit(&declarations);
  ASTNodeVisit(function->info.function.body, CollectFunctionLocalStatic, 0,
               &declarations);
  for (size_t i = 0; i < declarations.length; i++) {
    VariableDeclarationASTNode* decl =
        (VariableDeclarationASTNode*)VectorGet(&declarations, i);
    BinaryASTNode* init_node = NULL;
    if (decl->local_static_init_kind == kLocalStaticInitConstant) {
      ASTNodeVisit(decl->initializer, FindStaticInitializer, 0, &init_node);
    }
    if (init_node != NULL) {
      AddInitializedStaticVariable(decl, init_node->right);
    } else {
      AddUninitializedLocalStatic(decl->symbol);
    }
    if (decl->local_static_guard != NULL) {
      AddUninitializedLocalStatic(decl->local_static_guard);
    }
  }
  VectorDestruct(&declarations);

  // Thread-local block variables and their per-thread guard declarations are
  // still collected during parsing because invented TLS guards do not appear
  // as declarations in the final function body.
  for (size_t i = 0; i < syntax->local_statics.length; i++) {
    VariableDeclarationASTNode* decl =
        (VariableDeclarationASTNode*)syntax->local_statics.value.p[i];
    if (decl->symbol == NULL ||
        !StorageIs(decl->symbol->storage, STO(thread))) {
      continue;
    }
    if (decl->initializer != NULL &&
        decl->initializer->op == AST_OP(init)) {
      AddInitializedStaticVariable(
          decl, ((BinaryASTNode*)decl->initializer)->right);
    } else {
      AddUninitializedLocalStatic(decl->symbol);
    }
  }
  VectorClear(&syntax->local_statics);
}

static bool IsFunctionOrInlineDefinition(Symbol* sym) {
  return TypeIsFunctionDefinition(sym->type) || (TypeIsFunction(sym->type) &&
                                            sym->flags.is_inline_defn);
}

static bool CXXTypeHasNoOpDefaultConstructor(TypeRecord* type) {
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  Struct* str = type->info.struct_info;
  if (str->bases.length != 0 || str->virtual_bases.length != 0 ||
      str->vptr_member != NULL || str->tag_name == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->is_static || member->is_member_function ||
        member->is_using_declaration) {
      continue;
    }
    if (member->symbol != NULL &&
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    return false;
  }
  StructMember* ctor = FindStructMember(str, str->tag_name);
  if (ctor == NULL || !ctor->is_member_function || ctor->symbol == NULL ||
      ctor->symbol->type == NULL || !TypeIsFunction(ctor->symbol->type) ||
      !ctor->symbol->type->info.function.is_constructor ||
      ctor->symbol->type->info.function.is_deleted) {
    return false;
  }
  TypeRecord* func = ctor->symbol->type;
  if (func->info.function.is_defaulted) {
    return true;
  }
  ASTNode* body = func->info.function.body;
  return body != NULL && body->op == AST_OP(compound) &&
         ((CompoundStatementASTNode*)body)->statements != NULL &&
         ((CompoundStatementASTNode*)body)->statements->length == 0;
}

static bool CXXInitializerIsNoOpDefaultConstructor(Symbol* symbol,
                                                   ASTNode* initializer) {
  if (symbol == NULL || initializer == NULL ||
      initializer->op != AST_OP(call) ||
      !CXXTypeHasNoOpDefaultConstructor(symbol->type)) {
    return false;
  }
  ASTNode* callee = ((VectorASTNode*)initializer)->left;
  Symbol* callee_symbol = NULL;
  if (callee != NULL && callee->op == AST_OP(identifier)) {
    callee_symbol = ((IdentifierASTNode*)callee)->symbol;
  } else if (callee != NULL && callee->op == AST_OP(structmember)) {
    StructMember* member = ((StructMemberASTNode*)callee)->member;
    callee_symbol = member != NULL ? member->symbol : NULL;
  }
  TypeRecord* callee_type =
      callee_symbol != NULL ? callee_symbol->type : callee != NULL ? callee->type
                                                                  : NULL;
  return callee_type != NULL && TypeIsFunction(callee_type) &&
         callee_type->info.function.is_constructor;
}

static bool SymbolPointerInVector(Vector* symbols, Symbol* symbol) {
  for (size_t i = 0; i < symbols->length; i++) {
    if (symbols->value.p[i] == symbol) {
      return true;
    }
  }
  return false;
}

static void RecordCXXNoOpInitializedVariable(Symbol* symbol) {
  if (!SymbolPointerInVector(&compiler->cxx_no_op_initialized_variables,
                             symbol)) {
    VectorAppend(&compiler->cxx_no_op_initialized_variables, symbol);
  }
}

static void PruneCXXNoOpGlobalConstructorCalls(void) {
  for (size_t i = 0; i < compiler->cxx_global_constructors.length;) {
    Symbol* object = compiler->cxx_global_constructors.value.p[i];
    if (object != NULL &&
        CXXTypeHasNoOpDefaultConstructor(object->type)) {
      RecordCXXNoOpInitializedVariable(object);
    }
    if (SymbolPointerInVector(&compiler->cxx_no_op_initialized_variables,
                              object)) {
      VectorDeleteElement(&compiler->cxx_global_constructors, i);
    } else {
      i++;
    }
  }
  for (size_t i = 0; i < compiler->cxx_global_constructor_objects.length;) {
    Symbol* object = compiler->cxx_global_constructor_objects.value.p[i];
    if (object != NULL &&
        CXXTypeHasNoOpDefaultConstructor(object->type)) {
      RecordCXXNoOpInitializedVariable(object);
    }
    if (!SymbolPointerInVector(&compiler->cxx_no_op_initialized_variables,
                               object)) {
      i++;
      continue;
    }
    ASTNodeDelete(compiler->cxx_global_constructor_calls.value.p[i]);
    VectorDeleteElement(&compiler->cxx_global_constructor_calls, i);
    VectorDeleteElement(&compiler->cxx_global_constructor_objects, i);
  }
}

static StructMember* FindCXXSpecialMemberForGlobal(Symbol* sym,
                                                   bool destructor) {
  if (!CompilerIsCXX() || sym == NULL || !TypeIsStructOrUnion(sym->type) ||
      sym->type->info.struct_info == NULL ||
      sym->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  StructMember* member = NULL;
  if (destructor) {
    String name;
    StringInit(&name, "~");
    StringAppendString(&name, sym->type->info.struct_info->tag_name);
    member = FindStructMember(sym->type->info.struct_info, &name);
    StringDestruct(&name);
  } else {
    member = FindStructMember(sym->type->info.struct_info,
                              sym->type->info.struct_info->tag_name);
  }
  if (member == NULL || !member->is_member_function) {
    return NULL;
  }
  TypeRecord* func = member->symbol->type;
  if (!TypeIsFunction(func)) {
    return NULL;
  }
  if (destructor && !func->info.function.is_destructor) {
    return NULL;
  }
  if (!destructor && !func->info.function.is_constructor) {
    return NULL;
  }
  // A trivial, implicitly-declared special member performs no work, so a global
  // object never needs a constructor or destructor call for it.  (Such a member
  // is still synthesized as a findable class member once the class has any
  // user-declared special member, so this guard prevents emitting a spurious
  // call that would otherwise be injected into `main`.)
  if (func->info.function.is_implicitly_declared &&
      func->info.function.is_trivial_special_member) {
    return NULL;
  }
  if (!destructor && CXXTypeHasNoOpDefaultConstructor(sym->type)) {
    RecordCXXNoOpInitializedVariable(sym);
    return NULL;
  }
  TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax,
                                             member->symbol);
  return member;
}

static ASTNode* NewCXXGlobalSpecialMemberCall(Symbol* sym, bool destructor) {
  SourceLocation location = sym->location;
  ASTNode* receiver = NewIdentifierASTNode(sym, location);
  String member_name;
  if (destructor) {
    StringInit(&member_name, "~");
    StringAppendString(&member_name, sym->type->info.struct_info->tag_name);
  } else {
    StringInit(&member_name, sym->type->info.struct_info->tag_name->value);
  }
  ASTNode* member =
      NewStringConstantASTNode(NewString(member_name.value), NULL, location);
  StringDestruct(&member_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  Vector* actuals = NewVector();
  // A constructor/destructor of a class with virtual bases takes a hidden
  // `__complete_object` flag as its first explicit argument (the 2nd formal
  // after `this`).  A file-scope object is always the complete/most-derived
  // object, so pass 1.  Locals get this via NewCXXConstructorCall; the global
  // static-init path must supply it too or the callee prototype won't match and
  // the virtual-base pointers are left uninitialized.
  if (TypeIsStructOrUnion(sym->type) && sym->type->info.struct_info != NULL &&
      StructHasVirtualBases(sym->type->info.struct_info)) {
    VectorAppend(actuals,
                 NewIntConstantASTNode(
                     1, NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
  }
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  return NewExpressionStatementASTNode(call, location);
}


static void RegisterCXXThreadLocalObject(Symbol* sym) {
  bool statically_constructed =
      sym != NULL && sym->flags.value_set && sym->value.other != NULL &&
      (TypeIsFixedArray(sym->type) || TypeIsStructOrUnion(sym->type));
  if (!statically_constructed &&
      FindCXXSpecialMemberForGlobal(sym, false) != NULL) {
    VectorAppend(&compiler->cxx_thread_constructor_calls,
                 NewCXXGlobalSpecialMemberCall(sym, false));
  }
  if (FindCXXSpecialMemberForGlobal(sym, true) != NULL) {
    VectorAppend(&compiler->cxx_thread_destructor_calls,
                 NewCXXGlobalSpecialMemberCall(sym, true));
  }
}

static void RegisterCXXGlobalObject(Symbol* sym) {
  if (SymbolIsThreadLocal(sym)) {
    RegisterCXXThreadLocalObject(sym);
    return;
  }
  bool statically_constructed =
      sym != NULL && sym->flags.value_set && sym->value.other != NULL &&
      (TypeIsFixedArray(sym->type) || TypeIsStructOrUnion(sym->type));
  if (!statically_constructed && FindCXXSpecialMemberForGlobal(sym, false) != NULL) {
    VectorAppend(&compiler->cxx_global_constructors, sym);
  }
  if (FindCXXSpecialMemberForGlobal(sym, true) != NULL) {
    VectorAppend(&compiler->cxx_global_destructors, sym);
  }
}

static void RegisterCXXGlobalDestructor(Symbol* sym) {
  if (SymbolIsThreadLocal(sym)) {
    if (FindCXXSpecialMemberForGlobal(sym, true) != NULL) {
      VectorAppend(&compiler->cxx_thread_destructor_calls,
                   NewCXXGlobalSpecialMemberCall(sym, true));
    }
    return;
  }
  if (FindCXXSpecialMemberForGlobal(sym, true) != NULL) {
    VectorAppend(&compiler->cxx_global_destructors, sym);
  }
}

static ASTNode* NewCXXRuntimeVoidCallStatement(const char* name,
                                               SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, void_type);
  Symbol* func_sym = NewSymbol(name, func_type, STO(extern));
  ASTNode* call = NewVectorASTNode(
      AST_OP(call), NULL, location, NewIdentifierASTNode(func_sym, location),
      NewVector());
  return NewExpressionStatementASTNode(call, location);
}

static bool CXXProcessNeedsMainThreadTlsInit(void) {
  return compiler->cxx_thread_constructor_calls.length != 0 ||
         compiler->cxx_tls_block_dtor_thunks.length != 0;
}

static bool CXXProcessNeedsGlobalInitFunction(void) {
  return compiler->cxx_global_constructor_calls.length != 0 ||
         compiler->cxx_global_constructors.length != 0 ||
         compiler->cxx_global_destructors.length != 0 ||
         CXXProcessNeedsMainThreadTlsInit();
}

void CXXInitFiniArrayEntryDelete(CXXInitFiniArrayEntry* entry) {
  (void)entry;
}

static void RegisterInitFiniArrayEntry(Vector* functions, Symbol* function) {
  VectorAppend(functions, function);
}

Vector* CXXInitArrayFunctionsVector(void) { return &cxx_init_array_functions; }

Vector* CXXFiniArrayFunctionsVector(void) { return &cxx_fini_array_functions; }

static bool CXXGlobalObjectAlreadyRegistered(Set* registered, Symbol* sym) {
  if (sym == NULL) {
    return false;
  }
  if (SetContains(registered, sym)) {
    return true;
  }
  SetInsert(registered, sym);
  return false;
}

static void AppendCXXGlobalAtexitRegistration(Vector* statements, Symbol* sym,
                                              Set* registered) {
  if (CXXGlobalObjectAlreadyRegistered(registered, sym)) {
    return;
  }
  ASTNode* registration = SyntaxNewCXXGlobalAtexitStatement(sym, sym->location);
  if (registration != NULL) {
    VectorAppend(statements, registration);
  }
}

static Symbol* CompileCXXThreadLifetimeFunction(Syntax* syntax, const char* name,
                                                Vector* statements,
                                                bool local);

static void BuildCXXProcessInitStatements(Vector* statements) {
  Set registered = {0};
  SetInitForPointers(&registered);

  if (CXXProcessNeedsMainThreadTlsInit()) {
    SourceLocation location = {0};
    VectorAppend(statements,
                 NewCXXRuntimeVoidCallStatement("__davecc_tls_thread_init",
                                                location));
  }

  for (size_t i = 0; i < compiler->cxx_global_constructor_calls.length; i++) {
    VectorAppend(statements,
                 compiler->cxx_global_constructor_calls.value.p[i]);
    Symbol* object = compiler->cxx_global_constructor_objects.value.p[i];
    AppendCXXGlobalAtexitRegistration(statements, object, &registered);
  }

  for (size_t i = 0; i < compiler->cxx_global_constructors.length; i++) {
    Symbol* object = compiler->cxx_global_constructors.value.p[i];
    VectorAppend(statements, NewCXXGlobalSpecialMemberCall(object, false));
    AppendCXXGlobalAtexitRegistration(statements, object, &registered);
  }

  for (size_t i = 0; i < compiler->cxx_global_destructors.length; i++) {
    Symbol* object = compiler->cxx_global_destructors.value.p[i];
    AppendCXXGlobalAtexitRegistration(statements, object, &registered);
  }

  SetDestruct(&registered);
}

static void CompileCXXProcessInitFunction(Syntax* syntax) {
  if (!CompilerIsCXX() || !CXXProcessNeedsGlobalInitFunction()) {
    return;
  }

  Vector* statements = NewVector();
  BuildCXXProcessInitStatements(statements);
  if (statements->length == 0) {
    VectorDelete(statements);
    return;
  }

  Symbol* sym =
      CompileCXXThreadLifetimeFunction(syntax, "__davecc_cxx_global_init",
                                       statements, true);
  if (sym != NULL) {
    RegisterInitFiniArrayEntry(CXXInitArrayFunctionsVector(), sym);
  }
}

static void CheckMainSignature(Syntax* syntax, Symbol* sym) {
  if (!StringEqual(&sym->name, "main")) {
    return;
  }
  TypeRecord* main = sym->type;
  if (!TypeIsInt(main->next)) {
    SyntaxWarning(syntax, "main", "main must return an int");
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
    SyntaxWarning(syntax, "main", "main should have 2 or 3 args");
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


typedef struct {
  bool found;
} UnexpandedPackSearch;

static void FindUnexpandedPackInFunctionBody(ASTNode* node, void* data,
                                             int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  UnexpandedPackSearch* search = data;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
      search->found = true;
    }
    return;
  }
  if (node->op == AST_OP(structmember)) {
    StructMemberASTNode* member = (StructMemberASTNode*)node;
    if (member->member != NULL && member->member->symbol != NULL &&
        member->member->symbol->flags.is_parameter_pack) {
      search->found = true;
    }
  }
}

static bool FunctionBodyContainsUnexpandedPack(ASTNode* body) {
  UnexpandedPackSearch search = {0};
  ASTNodeVisit(body, FindUnexpandedPackInFunctionBody, 0, &search);
  return search.found;
}

static bool AsmNameInVector(Vector* names, const char* asm_name) {
  if (asm_name == NULL || *asm_name == '\0') {
    return false;
  }
  for (size_t i = 0; i < names->length; i++) {
    String* name = names->value.p[i];
    if (name != NULL && strcmp(name->value, asm_name) == 0) {
      return true;
    }
  }
  return false;
}

static bool FunctionAsmNameAlreadyEmitted(const char* asm_name) {
  return AsmNameInVector(&compiler->emitted_function_asm_names, asm_name);
}

static void CompileReferencedInlineFunctions(Syntax* syntax);

void CompilerMarkFunctionReferenced(Symbol* symbol) {
  if (compiler == NULL || symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type)) {
    return;
  }
  // Do not assign an asm name here.  Compiler-invented runtime declarations
  // deliberately have an empty asm_name so they retain their ABI spelling
  // (for example __davecc_throw rather than a C++-mangled name).
  const char* asm_name = symbol->asm_name.length != 0
                             ? symbol->asm_name.value
                             : symbol->name.value;
  if (!AsmNameInVector(&compiler->referenced_function_asm_names, asm_name)) {
    VectorAppend(&compiler->referenced_function_asm_names,
                 NewString(asm_name));
  }
}

void CompilerMarkVariableReferenced(Symbol* symbol) {
  if (compiler == NULL || symbol == NULL || symbol->type == NULL ||
      TypeIsFunction(symbol->type) || symbol->flags.is_local ||
      symbol->flags.is_temp || symbol->flags.is_argument) {
    return;
  }
  const char* asm_name = symbol->asm_name.length != 0
                             ? symbol->asm_name.value
                             : symbol->name.value;
  if (!AsmNameInVector(&compiler->referenced_variable_asm_names, asm_name)) {
    VectorAppend(&compiler->referenced_variable_asm_names,
                 NewString(asm_name));
  }
}

void CompilerRegisterLazyCXXStatic(InitializedStaticVariable* var) {
  if (compiler == NULL || var == NULL) {
    return;
  }
  for (size_t i = 0; i < compiler->cxx_lazy_static_variables.length; i++) {
    if (compiler->cxx_lazy_static_variables.value.p[i] == var) {
      return;
    }
  }
  VectorAppend(&compiler->cxx_lazy_static_variables, var);
}

static const char* SymbolReferenceName(Symbol* symbol) {
  if (symbol == NULL) {
    return NULL;
  }
  return symbol->asm_name.length != 0 ? symbol->asm_name.value
                                      : symbol->name.value;
}

static bool FunctionSymbolIsReferenced(Symbol* symbol) {
  return symbol != NULL &&
         AsmNameInVector(&compiler->referenced_function_asm_names,
                         SymbolReferenceName(symbol));
}

static bool VariableSymbolIsReferenced(Symbol* symbol) {
  return symbol != NULL &&
         AsmNameInVector(&compiler->referenced_variable_asm_names,
                         SymbolReferenceName(symbol));
}

static bool IsLazyCXXStatic(InitializedStaticVariable* var) {
  for (size_t i = 0; i < compiler->cxx_lazy_static_variables.length; i++) {
    if (compiler->cxx_lazy_static_variables.value.p[i] == var) {
      return true;
    }
  }
  return false;
}

// Compiler-generated vtables and RTTI records form a graph through their
// symbol initializers. Follow that graph only from metadata referenced by real
// code. A retained vtable also makes each virtual function in its slots
// reachable; an adjustor thunk in a slot makes its target reachable.
static void MarkReferencedCXXMetadataDependencies(void) {
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (var == NULL || !IsLazyCXXStatic(var) ||
        !VariableSymbolIsReferenced(var->symbol)) {
      continue;
    }
    for (size_t j = 0; j < var->initializers.length; j++) {
      Initializer* init = var->initializers.value.p[j];
      if (init == NULL || init->type != kInitTypeSymbol ||
          init->value.symbol == NULL) {
        continue;
      }
      if (TypeIsFunction(init->value.symbol->type)) {
        CompilerMarkFunctionReferenced(init->value.symbol);
      } else {
        CompilerMarkVariableReferenced(init->value.symbol);
      }
    }
  }

  for (size_t i = 0; i < compiler->cxx_this_adjustor_thunks.length; i++) {
    CXXThisAdjustorThunk* thunk =
        compiler->cxx_this_adjustor_thunks.value.p[i];
    if (thunk != NULL && FunctionSymbolIsReferenced(thunk->thunk)) {
      CompilerMarkFunctionReferenced(thunk->target);
    }
  }
}

static bool FunctionDefinitionIsODRDiscardable(TypeRecord* type) {
  return CompilerIsCXX() && type != NULL && TypeIsFunction(type) &&
         (type->info.function.is_inline ||
          type->info.function.template_origin != NULL);
}

static bool FunctionDefinitionNeedsNativeCode(Symbol* symbol,
                                              TypeRecord* type) {
  if (symbol == NULL || type == NULL || !TypeIsFunction(type)) {
    return false;
  }
  // An explicit specialization is an ordinary externally visible definition,
  // not a discardable implicit instantiation.
  if (symbol->flags.is_explicit_specialization &&
      !StorageIs(symbol->storage, STO(static))) {
    return true;
  }
  // C inline linkage has different rules from C++ ODR emission.  Preserve the
  // existing C behavior; this reachability pass is specifically for C++ inline
  // definitions.
  if (!FunctionDefinitionIsODRDiscardable(type)) {
    return true;
  }
  // A module/header-unit object must provide exported inline definitions for
  // importers.  Their bodies may reference internal-linkage helpers that are
  // intentionally not imported as names.
  if (symbol->flags.is_exported || compiler->module_header) {
    return true;
  }
  const char* asm_name = symbol->asm_name.length != 0
                             ? symbol->asm_name.value
                             : symbol->name.value;
  return AsmNameInVector(&compiler->referenced_function_asm_names, asm_name);
}

static bool VariableDefinitionIsODRDiscardable(Symbol* symbol) {
  if (!CompilerIsCXX() || symbol == NULL || symbol->flags.is_local ||
      symbol->flags.is_exported || compiler->module_header ||
      TypeHasNonTrivialDestructor(symbol->type)) {
    return false;
  }
  if (symbol->flags.is_constexpr && SymbolHasWeakBinding(symbol)) {
    return true;
  }
  bool internal =
      StorageIs(symbol->storage, STO(static)) ||
      ((TypeIsConst(symbol->type) || symbol->flags.is_constexpr) &&
       !StorageIs(symbol->storage, STO(extern)));
  return SymbolPointerInVector(&compiler->cxx_no_op_initialized_variables,
                               symbol) &&
         (internal || SymbolHasWeakBinding(symbol));
}

static bool VariableDefinitionNeedsStorage(Symbol* symbol) {
  if (!VariableDefinitionIsODRDiscardable(symbol)) {
    return true;
  }
  const char* asm_name = symbol->asm_name.length != 0
                             ? symbol->asm_name.value
                             : symbol->name.value;
  return AsmNameInVector(&compiler->referenced_variable_asm_names, asm_name);
}

static void MarkReferencedSymbolInAST(ASTNode* node, void* data,
                                      int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (node == NULL || mode != kVisitPreChildren) {
    return;
  }
  Symbol* symbol = NULL;
  if (node->op == AST_OP(identifier)) {
    symbol = ((IdentifierASTNode*)node)->symbol;
  } else if (node->op == AST_OP(structmember)) {
    StructMember* member = ((StructMemberASTNode*)node)->member;
    symbol = member != NULL ? member->symbol : NULL;
  }
  if (symbol != NULL && symbol->type != NULL &&
      TypeIsFunction(symbol->type)) {
    CompilerMarkFunctionReferenced(symbol);
  } else if (symbol != NULL) {
    CompilerMarkVariableReferenced(symbol);
  }
}

static void MarkReferencesInAST(ASTNode* node) {
  if (node != NULL) {
    ASTNodeVisit(node, MarkReferencedSymbolInAST, 0, NULL);
  }
}

static void MarkFunctionsReferencedByBody(TypeRecord* function) {
  if (function != NULL && TypeIsFunction(function) &&
      function->info.function.body != NULL) {
    MarkReferencesInAST(function->info.function.body);
  }
}

static bool GenerateFunctionDefinition(Syntax* syntax,
                                       VariableDeclarationASTNode* decl) {
  if (FunctionAsmNameAlreadyEmitted(decl->symbol->asm_name.value)) {
    return false;
  }
  bool dependent_function_body =
      TypeContainsTemplateParameter(decl->base.type) ||
      FunctionBodyContainsUnexpandedPack(
          decl->base.type->info.function.body);
  if (NumErrors() != 0 || dependent_function_body ||
      (decl->base.type->info.function.is_inline &&
       !decl->symbol->flags.is_inline_defn) ||
      !FunctionDefinitionNeedsNativeCode(decl->symbol, decl->base.type)) {
    return false;
  }

  TypeRecord* saved_current_function = compiler->current_function;
  Struct* saved_class_access_context =
      compiler->current_class_access_context;
  compiler->current_function = decl->base.type;
  compiler->current_class_access_context =
      decl->base.type->info.function.cxx_member_owner;

  if (compiler->debug_output) {
    decl->symbol->die = BuildDebugInfo(&compiler->debug_builder,
                                      decl->symbol, DW_TAG(subprogram));
  }
  SyntaxPrepareCXXLocalStatics(syntax, decl->base.type);
  Generator codegen;
  GeneratorInit(&codegen, syntax, compiler->current_function);
  void* code = GenerateFunction(&codegen);
  VectorAppend(&compiler->functions, code);
  VectorAppend(&compiler->emitted_function_asm_names,
               NewString(decl->symbol->asm_name.value));

  if (compiler->debug_output) {
    BuildDebugInfoAfterCodegen(&compiler->debug_builder, decl->symbol);
  }
  AddLocalStatics(syntax, decl->base.type);
  GeneratorDestruct(&codegen);
  SyntaxRegisterFunctionInitFiniAttributes(syntax, decl->symbol);
  compiler->current_function = saved_current_function;
  compiler->current_class_access_context = saved_class_access_context;
  return true;
}

static void CompileDeclarationNode(Syntax* syntax, ASTNode* node) {
  if (node != NULL) {
    // Retain the root so the whole AST can be torn down at CompilerDestruct.
    VectorAppend(&compiler->declaration_asts, node);
    // 'node' will be a declaration list containing declarations.
    if (node->op == AST_OP(decl_list)) {
      DeclarationListASTNode* decls = (DeclarationListASTNode*)node;
      size_t num_decls = decls->declarations->length;
      for (size_t i = 0; i < num_decls; i++) {
        VariableDeclarationASTNode* decl =
            (VariableDeclarationASTNode*)decls->declarations->value.p[i];

        if (decl->symbol->flags.is_template) {
          continue;
        }
        if (IsFunctionOrInlineDefinition(decl->symbol)) {
          if (FunctionAsmNameAlreadyEmitted(decl->symbol->asm_name.value)) {
            continue;
          }
          CheckMainSignature(syntax, decl->symbol);
          
          // This is a function definition, generate the code.
          TypeRecord* saved_current_function = compiler->current_function;
          Struct* saved_class_access_context =
              compiler->current_class_access_context;
          compiler->current_function = decl->base.type;
          if (decl->base.type != NULL && TypeIsFunction(decl->base.type)) {
            compiler->current_class_access_context =
                decl->base.type->info.function.cxx_member_owner;
          }
          
          // Run the semantic analyzer.
          SemanticAnalyzeFunction(syntax, (ASTNode*)decl);
          if (compiler->print_front_end) {
            SymbolPrintDetails(decl->symbol, true, compiler->ast_output_file);
          }
          compiler->current_function = saved_current_function;
          compiler->current_class_access_context =
              saved_class_access_context;
          if (!FunctionDefinitionIsODRDiscardable(decl->base.type)) {
            MarkFunctionsReferencedByBody(decl->base.type);
            CompileReferencedInlineFunctions(syntax);
          }
          GenerateFunctionDefinition(syntax, decl);
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
                var->is_weak = SymbolHasWeakBinding(decl->symbol);
                var->size = decl->symbol->type->size;
                var->alignment = SymbolEffectiveAlignment(decl->symbol);
                var->is_tls = StorageIs(decl->symbol->storage, STO(thread));
                var->is_local = decl->symbol->flags.is_local;
                VectorAppend(&compiler->uninitialized_static_variables, var);
                RegisterCXXGlobalObject(decl->symbol);
              } else {
                decl->symbol->flags.is_tentative_decl = false;
                if (TypeContainsAuto(decl->symbol->type)) {
                  decl->initializer = AnalyzeExpression(decl->initializer);
                  if (!SemanticDeduceAutoType(decl->symbol, decl->initializer,
                                              (ASTNode*)decl)) {
                    continue;
                  }
                  ASTNodeSetType((ASTNode*)decl, decl->symbol->type);
                }
                if (CompilerIsCXX() && TypeIsStructOrUnion(decl->symbol->type) &&
                    decl->initializer != NULL &&
                    decl->initializer->op == AST_OP(call)) {
                  UninitializedStaticVariable* var =
                      malloc(sizeof(UninitializedStaticVariable));
                  var->symbol = decl->symbol;
                  var->is_global =
                      !StorageIs(decl->symbol->storage, STO(static));
                  var->is_weak = SymbolHasWeakBinding(decl->symbol);
                  var->size = decl->symbol->type->size;
                  var->alignment = SymbolEffectiveAlignment(decl->symbol);
                  var->is_tls = StorageIs(decl->symbol->storage, STO(thread));
                  var->is_local = decl->symbol->flags.is_local;
                  VectorAppend(&compiler->uninitialized_static_variables, var);

                  if (CXXInitializerIsNoOpDefaultConstructor(
                          decl->symbol, decl->initializer)) {
                    RecordCXXNoOpInitializedVariable(decl->symbol);
                    decl->initializer = NULL;
                    continue;
                  }
                  ASTNode* constructor = decl->initializer;
                  decl->initializer = NULL;
                  if (SymbolIsThreadLocal(decl->symbol)) {
                    VectorAppend(&compiler->cxx_thread_constructor_calls,
                                 NewExpressionStatementASTNode(
                                     constructor, constructor->location));
                    RegisterCXXGlobalDestructor(decl->symbol);
                  } else {
                    VectorAppend(&compiler->cxx_global_constructor_calls,
                                 NewExpressionStatementASTNode(
                                     constructor, constructor->location));
                    VectorAppend(&compiler->cxx_global_constructor_objects,
                                 decl->symbol);
                    RegisterCXXGlobalDestructor(decl->symbol);
                  }
                  continue;
                }
                if (SymbolIsThreadLocal(decl->symbol) &&
                    CXXThreadLocalInitializerIsDynamic(decl->initializer)) {
                  UninitializedStaticVariable* var =
                      malloc(sizeof(UninitializedStaticVariable));
                  var->symbol = decl->symbol;
                  var->is_global =
                      !StorageIs(decl->symbol->storage, STO(static));
                  var->is_weak = SymbolHasWeakBinding(decl->symbol);
                  var->size = decl->symbol->type->size;
                  var->alignment = SymbolEffectiveAlignment(decl->symbol);
                  var->is_tls = StorageIs(decl->symbol->storage, STO(thread));
                  var->is_local = decl->symbol->flags.is_local;
                  VectorAppend(&compiler->uninitialized_static_variables, var);

                  ASTNode* dynamic_init =
                      CXXThreadLocalDynamicInitStatement(decl->symbol,
                                                         decl->initializer);
                  decl->initializer = NULL;
                  VectorAppend(&compiler->cxx_thread_constructor_calls,
                               dynamic_init);
                  RegisterCXXGlobalObject(decl->symbol);
                  continue;
                }
                if (!VariableDefinitionIsODRDiscardable(decl->symbol)) {
                  MarkReferencesInAST(decl->initializer);
                }
                ASTNode* initializer = ConstexprObjectInitializerForSymbol(
                    decl->symbol, decl->initializer->location);
                if (initializer == NULL) {
                  initializer = decl->initializer;
                }
                ASTNode* simplified_init = AnalyzeInitializer(
                    decl->symbol->type, initializer, true);
                // This is an initialized static variable.  The initializer has
                // been simplified to a braced initializer containing only
                // designated initializers.
                if (compiler->print_front_end) {
                  fprintf(compiler->ast_output_file, "Initializer\n");
                  ASTNodePrint(simplified_init, 0, compiler->ast_output_file);
                }
                AddInitializedStaticVariable(decl, simplified_init);
                RegisterCXXGlobalObject(decl->symbol);
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

static void CompilePendingTemplateInstantiations(Syntax* syntax) {
  while (compiler->pending_template_instantiations.length != 0) {
    ASTNode* node = compiler->pending_template_instantiations.value.p[0];
    VectorDeleteElement(&compiler->pending_template_instantiations, 0);
    SyntaxResetForNewDeclaration(syntax);
    CompileDeclarationNode(syntax, node);
  }
}

// Code generation itself records every function address it materializes.  Walk
// the retained inline definitions until that reference set reaches a fixed
// point: emitting one inline body can make further inline callees reachable.
static void CompileReferencedInlineFunctions(Syntax* syntax) {
  bool emitted;
  do {
    // First close the reference graph without generating code.  This lets the
    // subsequent pass emit reachable definitions in their original declaration
    // order, matching eager compilation while omitting unreachable bodies.
    size_t previous_function_reference_count;
    size_t previous_variable_reference_count;
    do {
      previous_function_reference_count =
          compiler->referenced_function_asm_names.length;
      previous_variable_reference_count =
          compiler->referenced_variable_asm_names.length;
      MarkReferencedCXXMetadataDependencies();
      size_t num_roots = compiler->declaration_asts.length;
      for (size_t i = 0; i < num_roots; i++) {
        ASTNode* node = compiler->declaration_asts.value.p[i];
        if (node == NULL || node->op != AST_OP(decl_list)) {
          continue;
        }
        DeclarationListASTNode* decls = (DeclarationListASTNode*)node;
        for (size_t j = 0; j < decls->declarations->length; j++) {
          VariableDeclarationASTNode* decl =
              decls->declarations->value.p[j];
          if (decl == NULL || decl->symbol == NULL) {
            continue;
          }
          if (IsFunctionOrInlineDefinition(decl->symbol) &&
              decl->base.type != NULL &&
              FunctionDefinitionIsODRDiscardable(decl->base.type) &&
              FunctionDefinitionNeedsNativeCode(decl->symbol,
                                                decl->base.type)) {
            MarkFunctionsReferencedByBody(decl->base.type);
          } else if (VariableDefinitionIsODRDiscardable(decl->symbol) &&
                     VariableDefinitionNeedsStorage(decl->symbol)) {
            MarkReferencesInAST(decl->initializer);
          }
        }
      }
    } while (compiler->referenced_function_asm_names.length !=
                 previous_function_reference_count ||
             compiler->referenced_variable_asm_names.length !=
                 previous_variable_reference_count);

    emitted = false;
    size_t num_roots = compiler->declaration_asts.length;
    for (size_t i = 0; i < num_roots; i++) {
      ASTNode* node = compiler->declaration_asts.value.p[i];
      if (node == NULL || node->op != AST_OP(decl_list)) {
        continue;
      }
      DeclarationListASTNode* decls = (DeclarationListASTNode*)node;
      for (size_t j = 0; j < decls->declarations->length; j++) {
        VariableDeclarationASTNode* decl =
            decls->declarations->value.p[j];
        if (decl == NULL || decl->symbol == NULL ||
            !IsFunctionOrInlineDefinition(decl->symbol) ||
            decl->base.type == NULL ||
            !FunctionDefinitionIsODRDiscardable(decl->base.type)) {
          continue;
        }
        if (GenerateFunctionDefinition(syntax, decl)) {
          emitted = true;
        }
      }
    }
  } while (emitted);
}

static void PruneUnreferencedCXXMetadata(void) {
  for (size_t i = 0; i < compiler->initialized_static_variables.length;) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (var == NULL || !IsLazyCXXStatic(var) ||
        VariableSymbolIsReferenced(var->symbol)) {
      i++;
      continue;
    }
    InitializedStaticVariableDelete(var);
    VectorDeleteElement(&compiler->initialized_static_variables, i);
  }

  for (size_t i = 0; i < compiler->cxx_this_adjustor_thunks.length;) {
    CXXThisAdjustorThunk* thunk =
        compiler->cxx_this_adjustor_thunks.value.p[i];
    if (thunk != NULL && FunctionSymbolIsReferenced(thunk->thunk)) {
      i++;
      continue;
    }
    free(thunk);
    VectorDeleteElement(&compiler->cxx_this_adjustor_thunks, i);
  }
}

static void PruneUnreferencedInlineVariables(void) {
  for (size_t i = 0; i < compiler->initialized_static_variables.length;) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (var == NULL || VariableDefinitionNeedsStorage(var->symbol)) {
      i++;
      continue;
    }
    InitializedStaticVariableDelete(var);
    VectorDeleteElement(&compiler->initialized_static_variables, i);
  }
  for (size_t i = 0; i < compiler->uninitialized_static_variables.length;) {
    UninitializedStaticVariable* var =
        compiler->uninitialized_static_variables.value.p[i];
    if (var == NULL || VariableDefinitionNeedsStorage(var->symbol)) {
      i++;
      continue;
    }
    UninitializedStaticVariableDelete(var);
    VectorDeleteElement(&compiler->uninitialized_static_variables, i);
  }
}

// Evaluates and code generates in-class `static constexpr`/`constinit` data
// members whose own type is the enclosing class and whose evaluation the parser
// therefore deferred (the class was incomplete at the point of definition).
// Run after the class's inline member-function bodies (including its
// constructors) have been semantically analyzed by
// CompilePendingTemplateInstantiations, so constant-evaluating the member's
// constructor succeeds.  The initializer is in the scope of the member's class,
// so access control treats it as if written inside that class (the value
// constructor may be private).
static void CompileDeferredCXXStaticMembers(Syntax* syntax) {
  Vector* deferred = &compiler->cxx_deferred_static_member_definitions;
  while (deferred->length != 0) {
    VariableDeclarationASTNode* decl = deferred->value.p[0];
    VectorDeleteElement(deferred, 0);
    if (decl == NULL || decl->symbol == NULL) {
      continue;
    }
    struct Struct* owner =
        TypeIsStructOrUnion(decl->symbol->type)
            ? decl->symbol->type->info.struct_info
            : NULL;
    struct Struct* saved_access = compiler->current_class_access_context;
    compiler->current_class_access_context = owner;
    SemanticAnalyzeVariableDefinition(syntax, decl);
    compiler->current_class_access_context = saved_access;

    Vector* declarations = NewVector();
    VectorAppend(declarations, decl);
    SyntaxResetForNewDeclaration(syntax);
    CompileDeclarationNode(
        syntax, NewDeclarationListASTNode(declarations, decl->base.location));
  }
}

static Symbol* CompileCXXThreadLifetimeFunction(Syntax* syntax, const char* name,
                                                Vector* statements,
                                                bool local) {
  if (statements->length == 0) {
    return NULL;
  }

  SourceLocation location = {0};
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* func_type = NewFunctionTypeRecord();
  Symbol* sym =
      NewSymbol(name, func_type, local ? STO(static) : STO(extern));
  sym->flags.is_defined = true;
  sym->flags.is_local = local;
  TypeRecordChain(func_type, void_type);
  func_type->info.function.symbol = sym;
  func_type->info.function.body =
      NewCompoundStatementASTNode(statements, location);

  ASTNode* decl = NewVariableDeclarationASTNode(sym, NULL, location);
  SemanticAnalyzeFunction(syntax, decl);

  if (NumErrors() != 0) {
    return NULL;
  }

  compiler->current_function = func_type;
  Generator codegen;
  GeneratorInit(&codegen, syntax, func_type);
  void* code = GenerateFunction(&codegen);
  VectorAppend(&compiler->functions, code);
  VectorAppend(&compiler->emitted_function_asm_names, NewString(name));
  GeneratorDestruct(&codegen);
  compiler->current_function = NULL;
  return sym;
}

static void CompileCXXTlsBlockDtorThunk(Syntax* syntax,
                                        CXXTlsBlockDtorThunk* entry) {
  if (entry == NULL || entry->thunk == NULL || entry->thunk->type == NULL ||
      !TypeIsFunction(entry->thunk->type)) {
    return;
  }

  TypeRecord* func_type = entry->thunk->type;
  ASTNode* decl =
      NewVariableDeclarationASTNode(entry->thunk, NULL, entry->thunk->location);
  SemanticAnalyzeFunction(syntax, decl);
  if (NumErrors() != 0) {
    return;
  }

  compiler->current_function = func_type;
  Generator codegen;
  GeneratorInit(&codegen, syntax, func_type);
  void* code = GenerateFunction(&codegen);
  VectorAppend(&compiler->functions, code);
  GeneratorDestruct(&codegen);
  compiler->current_function = NULL;
}

static void CompileCXXTlsBlockDtorThunks(Syntax* syntax) {
  if (!CompilerIsCXX()) {
    return;
  }
  for (size_t i = 0; i < compiler->cxx_tls_block_dtor_thunks.length; i++) {
    CompileCXXTlsBlockDtorThunk(syntax,
                                compiler->cxx_tls_block_dtor_thunks.value.p[i]);
  }
}

static void CompileCXXThreadLifetimeFunctions(Syntax* syntax) {
  if (!CompilerIsCXX()) {
    return;
  }

  CompileCXXTlsBlockDtorThunks(syntax);
  if (NumErrors() != 0) {
    return;
  }

  Vector* init_statements = NewVector();
  for (size_t i = 0; i < compiler->cxx_thread_constructor_calls.length; i++) {
    VectorAppend(init_statements,
                 compiler->cxx_thread_constructor_calls.value.p[i]);
  }
  CompileCXXThreadLifetimeFunction(syntax, "__davecc_tls_thread_init_impl",
                                   init_statements, false);

  Vector* fini_statements = NewVector();
  for (size_t i = compiler->cxx_thread_destructor_calls.length; i > 0; i--) {
    VectorAppend(fini_statements,
                 compiler->cxx_thread_destructor_calls.value.p[i - 1]);
  }
  CompileCXXThreadLifetimeFunction(syntax, "__davecc_tls_thread_fini_impl",
                                   fini_statements, false);
}

static void CompileDeclaration(Syntax* syntax) {
  // Capture the diagnostic state active at the start of this declaration.
  // Parsing reads a lookahead token that can process a following
  // "#pragma diagnostic pop", so we reinstall this snapshot around semantic
  // analysis and codegen (which emit deferred warnings) and then restore the
  // post-parse state for the next declaration.
  void* diag_state = DiagnosticSnapshotState();
  ASTNode* node = SyntaxParseExternalDeclaration(syntax);
  DiagnosticSwapState(diag_state);
  CompileDeclarationNode(syntax, node);
  CompilePendingTemplateInstantiations(syntax);
  CompileDeferredCXXStaticMembers(syntax);
  // Restore the post-parse diagnostic state so the next declaration starts from
  // where the lexer left off.
  DiagnosticSwapState(diag_state);
  DiagnosticFreeState(diag_state);
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
  if (compiler->target != NULL &&
      StringEqual(&compiler->target->name, "aarch64")) {
    // The AArch64 PCS (AAPCS64) va_list.  __stack points at the next
    // stack-passed argument, __gr_top/__vr_top point one past the general /
    // SIMD register save areas, and __gr_offs/__vr_offs are negative byte
    // offsets from those tops to the next unconsumed register argument.
    va_list_typedef =
        "typedef struct __va_list_tag {"
        " void* __stack;"
        " void* __gr_top;"
        " void* __vr_top;"
        " int __gr_offs;"
        " int __vr_offs;"
        " } __builtin_va_list;\n";
  }
  String* code = NewString(va_list_typedef);
  StringAppend(code,
      "#define __asm asm\n"
      "#define __asm__ asm\n"
      "#define __volatile__ volatile\n"
      "#define __attribute __attribute__\n"
      "#define __inline inline\n"
      "#define __signed signed\n"
      "#define __restrict restrict\n"
      "#define __restrict__ restrict\n"
      "\n");
  if (CompilerIsCXX()) {
    StringAppend(code,
                 "extern \"C\" int __davecc_cxa_atexit(void*, void*, unsigned long, "
                 "unsigned long, int);\n"
                 "extern \"C\" void __davecc_tls_thread_init(void);\n"
                 "extern \"C\" void __davecc_tls_thread_fini(void);\n");
  }

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
  // The input filename "-" means standard input; use a plain base name for
  // derived output files (e.g. "stdin.s"/"stdin.o") so they are not mistaken
  // for command-line options (a leading '-') by later tools like the linker.
  StringInit(&compiler->infile, strcmp(filename, "-") == 0 ? "stdin" : filename);
  ModuleUnitInfoInit(&compiler->module_unit);
  VectorInit(&compiler->functions);
  VectorInit(&compiler->emitted_function_asm_names);
  VectorInit(&compiler->referenced_function_asm_names);
  VectorInit(&compiler->referenced_variable_asm_names);
  VectorInit(&compiler->initialized_static_variables);
  VectorInit(&compiler->uninitialized_static_variables);
  VectorInit(&compiler->cxx_deferred_static_member_definitions);
  VectorInit(&compiler->cxx_global_constructors);
  VectorInit(&compiler->cxx_global_destructors);
  VectorInit(&compiler->cxx_no_op_initialized_variables);
  VectorInit(&compiler->cxx_global_constructor_calls);
  VectorInit(&compiler->cxx_global_constructor_objects);
  VectorInit(&compiler->cxx_thread_constructor_calls);
  VectorInit(&compiler->cxx_thread_destructor_calls);
  VectorInit(&compiler->cxx_tls_block_dtor_thunks);
  VectorInit(&compiler->cxx_this_adjustor_thunks);
  VectorInit(&compiler->cxx_lazy_static_variables);
  MapInitForStringKeys(&compiler->rtti_typeinfo_map);
  VectorInit(&compiler->literals);
  VectorInit(&compiler->declaration_asts);
  VectorInit(&compiler->cxx_defined_classes);
  VectorInit(&compiler->functions_being_analyzed);
  VectorInit(&cxx_init_array_functions);
  VectorInit(&cxx_fini_array_functions);
  VectorInit(&compiler->pending_template_instantiations);
  VectorInit(&compiler->orphan_function_symbols);
  SetInit(&compiler->disabled_warnings, CompareWarning);
  SetInit(&compiler->error_warnings, CompareWarning);
  SetInit(&compiler->no_error_warnings, CompareWarning);
  VectorInit(&compiler->diagnostic_stack);
  compiler->diagnostic_suppression_depth = 0;
  compiler->diagnostic_error_trap_depth = 0;
  compiler->diagnostic_error_trapped = false;
  compiler->pack_alignment = 0;
  VectorInit(&compiler->pack_stack);
  compiler->num_errors = 0;
  compiler->constexpr_codegen_recover = false;
  compiler->immediate_function_context_depth = 0;
  compiler->constant_evaluation_required_depth = 0;
  compiler->next_literal_id = 1;
  compiler->next_symbol_id = 1;
  compiler->current_include_path_index = 0;
  compiler->current_function = NULL;
  compiler->current_class_access_context = NULL;
  compiler->global_namespace = NULL;
  compiler->module_header = false;
  
  char dirname[4096];
  char* wd = getcwd(dirname, sizeof(dirname));
  if (wd == NULL) {
    wd = "unknown";
  }
  DebugBuilderInit(&compiler->debug_builder, filename, "davecc", wd);
}

static void ReplaceSourceExtension(String* filename, const char* extension) {
  const char* suffixes[] = {".cpp", ".cxx", ".cc", ".c"};
  for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
    const char* suffix = suffixes[i];
    size_t suffix_len = strlen(suffix);
    if (filename->length >= suffix_len &&
        strcmp(filename->value + filename->length - suffix_len, suffix) == 0) {
      filename->value[filename->length - suffix_len] = '\0';
      filename->length -= suffix_len;
      StringAppend(filename, extension);
      return;
    }
  }
  StringAppend(filename, extension);
}

static void OpenSaveFiles(Compiler* compiler) {
  // Open IR and AST files if required.
  compiler->ir_output_file = stdout;
  if (compiler->save_ir) {
    String ir_filename;
    StringInit(&ir_filename, compiler->infile.value);

    ReplaceSourceExtension(&ir_filename, ".ir");
    compiler->ir_output_file = fopen(ir_filename.value, "w");
    if (compiler->ir_output_file == NULL) {
      compiler->ir_output_file = stdout;
    }
  }
  compiler->ast_output_file = stdout;
  if (compiler->save_ast) {
    String ast_filename;
    StringInit(&ast_filename, compiler->infile.value);

    ReplaceSourceExtension(&ast_filename, ".ast");
    compiler->ast_output_file = fopen(ast_filename.value, "w");
    if (compiler->ast_output_file == NULL) {
      compiler->ast_output_file = stdout;
    }
  }
}

static void ParseOptimizationOption(Compiler* compiler, Vector* options,
                                    int default_opt_level) {
  compiler->optimize = default_opt_level > 0;
  compiler->optimize_for_size = false;
  compiler->opt_level = default_opt_level;
  String* value = OptionStringValue(kOptionOptimize, options);
  if (value == NULL) {
    return;
  }
  compiler->optimize = false;
  compiler->opt_level = 0;
  if (value->length == 0) {
    compiler->optimize = true;
    compiler->opt_level = 2;
  } else {
    if (value->length != 1) {
      fprintf(stderr, "Invalid optimization level -O%s\n", value->value);
      exit(1);
    }
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
      case 's':
        compiler->optimize = true;
        compiler->optimize_for_size = true;
        compiler->opt_level = 2;
        break;
      default:
        fprintf(stderr, "Invalid optimization level -O%c\n", level);
        exit(1);
        break;
    }
  }
  
}

static void ParseStandardOption(Compiler* compiler, Vector* options) {
  compiler->language_standard =
      StringEndsWith(&compiler->infile, ".cc") ||
              StringEndsWith(&compiler->infile, ".cpp")
          ? kLanguageStandardCXX20
          : kLanguageStandardC99;
  String* value = OptionStringValue(kOptionStandard, options);
  if (value == NULL) {
    return;
  }

  if (StringEqual(value, "c89") || StringEqual(value, "c90") ||
      StringEqual(value, "iso9899:1990") || StringEqual(value, "gnu89") ||
      StringEqual(value, "gnu90")) {
    compiler->language_standard = kLanguageStandardC89;
  } else if (StringEqual(value, "c99") || StringEqual(value, "iso9899:1999") ||
             StringEqual(value, "gnu99")) {
    compiler->language_standard = kLanguageStandardC99;
  } else if (StringEqual(value, "c11") || StringEqual(value, "c1x") ||
             StringEqual(value, "iso9899:2011") || StringEqual(value, "gnu11")) {
    compiler->language_standard = kLanguageStandardC11;
  } else if (StringEqual(value, "c17") || StringEqual(value, "c18") ||
             StringEqual(value, "iso9899:2017") || StringEqual(value, "gnu17") ||
             StringEqual(value, "gnu18")) {
    compiler->language_standard = kLanguageStandardC17;
  } else if (StringEqual(value, "c++98") || StringEqual(value, "c++03") ||
             StringEqual(value, "gnu++98") || StringEqual(value, "gnu++03")) {
    compiler->language_standard = StringEqual(value, "c++03") ||
                                          StringEqual(value, "gnu++03")
                                      ? kLanguageStandardCXX03
                                      : kLanguageStandardCXX98;
  } else if (StringEqual(value, "c++11") || StringEqual(value, "c++0x") ||
             StringEqual(value, "gnu++11") || StringEqual(value, "gnu++0x")) {
    compiler->language_standard = kLanguageStandardCXX11;
  } else if (StringEqual(value, "c++14") || StringEqual(value, "c++1y") ||
             StringEqual(value, "gnu++14") || StringEqual(value, "gnu++1y")) {
    compiler->language_standard = kLanguageStandardCXX14;
  } else if (StringEqual(value, "c++17") || StringEqual(value, "c++1z") ||
             StringEqual(value, "gnu++17") || StringEqual(value, "gnu++1z")) {
    compiler->language_standard = kLanguageStandardCXX17;
  } else if (StringEqual(value, "c++20") || StringEqual(value, "c++2a") ||
             StringEqual(value, "gnu++20") || StringEqual(value, "gnu++2a")) {
    compiler->language_standard = kLanguageStandardCXX20;
  } else if (StringEqual(value, "c++23") || StringEqual(value, "c++2b") ||
             StringEqual(value, "gnu++23") || StringEqual(value, "gnu++2b")) {
    compiler->language_standard = kLanguageStandardCXX23;
  } else {
    fprintf(stderr, "Invalid language standard -std=%s\n", value->value);
    exit(1);
  }
}

static void InitBasicOptionsOrDie(Compiler* compiler,
                                  Vector* options, Vector* target_opts) {
  compiler->max_errors = OptionIntValue(kOptionErrorLimit, options, 20);
  compiler->enable_all_warnings = false;
  compiler->convert_warnings_to_errors = false;
  if (options != NULL) {
    for (size_t i = 0; i < options->length; i++) {
      CompilerOptionValue* o = options->value.p[i];
      if (o->opt != kOptionWarning) {
        continue;
      }
      const char* v = o->value.svalue.value;
      if (strcmp(v, "error") == 0) {
        compiler->convert_warnings_to_errors = true;
      } else if (strcmp(v, "no-error") == 0) {
        compiler->convert_warnings_to_errors = false;
      }
    }
  }
  DisableDefaultWarnings();
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
  ParseStandardOption(compiler, options);
  ParseOptimizationOption(compiler, options, target->default_opt_level);
  compiler->pic = OptionBoolValue(kOptionPic, options, false);
  if (target->static_linkage_only && compiler->pic) {
    fprintf(stderr, "-fPIC is not supported on this target");
    exit(1);
  }

  // Exception metadata and unwind support are too large for the 6502 address
  // space, so those targets default to -fno-exceptions.  An explicit option
  // still overrides the target default with last-one-wins semantics.
  compiler->exceptions_enabled =
      strcmp(target->canonical_name, "6502") != 0 &&
      strcmp(target->canonical_name, "65c02") != 0;
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* opt = options->value.p[i];
    if (opt->opt == kOptionExceptions) {
      compiler->exceptions_enabled = true;
    } else if (opt->opt == kOptionNoExceptions) {
      compiler->exceptions_enabled = false;
    }
  }
  compiler->printf_specialize =
      strcmp(target->canonical_name, "6502") == 0 ||
      strcmp(target->canonical_name, "65c02") == 0;
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* opt = options->value.p[i];
    if (opt->opt == kOptionPrintfSpecialize) {
      compiler->printf_specialize = true;
    } else if (opt->opt == kOptionNoPrintfSpecialize) {
      compiler->printf_specialize = false;
    }
  }
  compiler->module_header =
      OptionBoolValue(kOptionModuleHeader, options, false);
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
  if (compiler->optimize_for_size) {
    compiler->code_preference = kCodeForSize;
    compiler->ir_optimizations.code_motion = false;
    compiler->ir_optimizations.dce = true;
    compiler->ir_optimizations.copy_prop = true;
  }
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
  if (OptionBoolValue(kOptionNoStandardIncludes, options, false)) {
    PreprocessorClearSystemIncludePaths(&compiler->preprocessor);
  }
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
                          option_value->value.svalue.length - equals - 1,
                          &macro_value);
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
      case kOptionWarning: {
        const char* v = option_value->value.svalue.value;
        // Global -Werror/-Wno-error are resolved in InitBasicOptionsOrDie.
        if (strcmp(v, "error") == 0 || strcmp(v, "no-error") == 0) {
          // Already handled.
        } else if (WarningGroupExists(v)) {
          EnableWarningGroup(v);
        } else if (strncmp(v, "error=", 6) == 0) {
          const char* warning = v + 6;
          if (WarningExists(warning)) {
            MakeWarningError(warning);
            EnableWarning(warning);
          } else {
            ReportWarning(compiler->infile.value, 0, "unknown-warning-option",
                          "unknown warning option '-W%s'", v);
          }
        } else if (strncmp(v, "no-error=", 9) == 0) {
          const char* warning = v + 9;
          if (WarningExists(warning)) {
            ExemptWarningFromError(warning);
          } else {
            ReportWarning(compiler->infile.value, 0, "unknown-warning-option",
                          "unknown warning option '-W%s'", v);
          }
        } else if (strncmp(v, "no-", 3) == 0) {
          const char* warning = v + 3;
          if (WarningGroupExists(warning)) {
            DisableWarningGroup(warning);
          } else if (WarningExists(warning)) {
            DisableWarning(warning);
          }
        } else {
          if (WarningExists(v)) {
            EnableWarning(v);
          } else {
            ReportWarning(compiler->infile.value, 0, "unknown-warning-option",
                          "unknown warning option '-W%s'", v);
          }
        }
        break;
      }
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

static void FreeRttiTypeInfoKey(MapKeyValue* kv) {
  StringDelete((String*)kv->key.p);
}

void CompilerDestruct(Compiler* compiler) {
  ConstexprPCodeClearImageCache();

  // Tear down the AST forest first, while the symbol table and type records it
  // references are still alive.  Destructing releases each node's non-arena
  // resources (type references, owned strings/vectors); it is idempotent and
  // graph-safe (see ASTNodeDelete).  The node structs themselves live in the
  // AST arena and are reclaimed wholesale afterwards.
  for (size_t i = 0; i < compiler->declaration_asts.length; i++) {
    ASTNodeDelete((ASTNode*)compiler->declaration_asts.value.p[i]);
  }
  VectorDestruct(&compiler->declaration_asts);
  VectorDestruct(&compiler->cxx_defined_classes);
  VectorDestruct(&compiler->functions_being_analyzed);
  for (size_t i = 0; i < compiler->pending_template_instantiations.length; i++) {
    ASTNodeDelete((ASTNode*)compiler->pending_template_instantiations.value.p[i]);
  }
  VectorDestruct(&compiler->pending_template_instantiations);

  // Free function-definition symbols that were superseded by an earlier
  // declaration and so never entered the global symbol table.  Deleting each
  // releases its owned function type (breaking the symbol<->type cycle).
  for (size_t i = 0; i < compiler->orphan_function_symbols.length; i++) {
    SymbolDelete((Symbol*)compiler->orphan_function_symbols.value.p[i]);
  }
  VectorDestruct(&compiler->orphan_function_symbols);

  if (compiler->ir_output_file != stdout) {
    fclose(compiler->ir_output_file);
  }
  if (compiler->ast_output_file != stdout) {
    fclose(compiler->ast_output_file);
  }
  DebugBuilderDestruct(&compiler->debug_builder);

  for (size_t i = 0; i < compiler->functions.length; i++) {
    compiler->target->cleanup(compiler->functions.value.p[i]);
  }
  VectorDestruct(&compiler->functions);
  VectorDestructWithContents(
      &compiler->emitted_function_asm_names,
      (VectorElementDestructor)StringDelete, /*free_element=*/false);
  VectorDestructWithContents(
      &compiler->referenced_function_asm_names,
      (VectorElementDestructor)StringDelete, /*free_element=*/false);
  VectorDestructWithContents(
      &compiler->referenced_variable_asm_names,
      (VectorElementDestructor)StringDelete, /*free_element=*/false);

  // Imported module graphs use heap Symbol/Namespace nodes that must be
  // detached and released after AST/IR teardown has finished using them, but
  // before the AST arena and TU symbol tables disappear.
  if (g_translation_unit_import_release != NULL &&
      g_translation_unit_import_state != NULL) {
    g_translation_unit_import_release(g_translation_unit_import_state);
    g_translation_unit_import_state = NULL;
    g_translation_unit_import_release = NULL;
  }

  ASTArenaRelease();

  DeleteGlobalNamespace();
  ClearSymbolTable(&compiler->global_symbol_table, true);
  ClearSymbolTable(&compiler->global_tag_table, true);
  // ClearSymbolTable only empties the tables; release their bucket arrays too.
  HashTableDestruct(&compiler->global_symbol_table);
  HashTableDestruct(&compiler->global_tag_table);

  StringDestruct(&compiler->infile);
  ModuleUnitInfoDestruct(&compiler->module_unit);

  if (compiler->target != NULL) {
    DeleteCompilerTarget(compiler->target);
    compiler->target = NULL;
  }

  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariableDelete(
        compiler->initialized_static_variables.value.p[i]);
  }
  VectorDestruct(&compiler->initialized_static_variables);

  for (size_t i = 0; i < compiler->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariableDelete(
        compiler->uninitialized_static_variables.value.p[i]);
  }
  VectorDestruct(&compiler->uninitialized_static_variables);

  VectorDestruct(&compiler->cxx_deferred_static_member_definitions);
  VectorDestruct(&compiler->cxx_global_constructors);
  VectorDestruct(&compiler->cxx_global_destructors);
  VectorDestruct(&compiler->cxx_no_op_initialized_variables);
  VectorDestruct(&compiler->cxx_global_constructor_calls);
  VectorDestruct(&compiler->cxx_global_constructor_objects);
  VectorDestruct(&cxx_init_array_functions);
  VectorDestruct(&cxx_fini_array_functions);
  VectorDestruct(&compiler->cxx_thread_constructor_calls);
  VectorDestruct(&compiler->cxx_thread_destructor_calls);
  VectorDestructWithContents(&compiler->cxx_tls_block_dtor_thunks, NULL,
                             /*free_element=*/true);
  VectorDestructWithContents(&compiler->cxx_this_adjustor_thunks, NULL,
                             /*free_element=*/true);
  VectorDestruct(&compiler->cxx_lazy_static_variables);
  MapDestructWithContents(&compiler->rtti_typeinfo_map, FreeRttiTypeInfoKey);

  for (size_t i = 0; i < compiler->literals.length; i++) {
    LiteralDelete(compiler->literals.value.p[i]);
  }
  VectorDestruct(&compiler->literals);

  // Free any unbalanced #pragma diagnostic push snapshots.
  for (size_t i = 0; i < compiler->diagnostic_stack.length; i++) {
    free(compiler->diagnostic_stack.value.p[i]);
  }
  VectorDestruct(&compiler->diagnostic_stack);
  VectorDestruct(&compiler->pack_stack);

  PreprocessorDestruct(&compiler->preprocessor);
  SyntaxDestruct(&compiler->syntax);
  LexDestruct(&compiler->lex);

  // The AST, symbol tables and tags are gone, so struct infos (which can form
  // reference cycles and thus are not freed by refcount) can be freed in one
  // pass.  This deletes member symbols, which decref TypeRecords, so it must
  // run before the type arena is released.
  StructRegistryRelease();

  // Everything that references TypeRecords has now been torn down, so the type
  // arena's struct memory can be reclaimed in one shot.
  TypeRecordArenaRelease();
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
  if (compiler->target->emit_cxx_thunks != NULL) {
    compiler->target->emit_cxx_thunks(asm_file);
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
    if (!var->is_tls &&
        (var->symbol->flags.is_tentative_decl || var->is_local ||
         (CompilerIsCXX() && TypeIsStructOrUnion(var->symbol->type)))) {
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

  EmitInitFiniArrayEntries(CXXInitArrayFunctionsVector(), false, asm_file);
  EmitInitFiniArrayEntries(CXXFiniArrayFunctionsVector(), true, asm_file);

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
    ReplaceSourceExtension(object_filename, ".o");
  }

  // Assemble using target-specific assembler.
  bool ok = compiler->target->assemble(asm_filename, object_filename);
  if (!ok) {
    StringDelete(object_filename);
    return NULL;
  }
  return object_filename;
}

static bool FunctionSymbolWasUsed(Symbol* sym) {
  if (sym->flags.used || sym->flags.address_taken) {
    return true;
  }
  Symbol* table_sym = FindGlobalSymbol(&sym->name);
  return table_sym != NULL &&
         (table_sym->flags.used || table_sym->flags.address_taken);
}

// Emits -Wunused-private-field for each non-static private data member of a
// class defined in this translation unit that is never referenced.  A private
// data member can only be named by the class itself or its friends, so global
// usage tracking (Symbol::flags.used, set wherever a member is named in an
// expression) is sufficient: any reference anywhere clears the warning, while a
// constructor member-initializer does not (matching clang, which reports fields
// that are "initialized but never used").
static void CheckUnusedPrivateFields(void) {
  for (size_t i = 0; i < compiler->cxx_defined_classes.length; i++) {
    Struct* str = compiler->cxx_defined_classes.value.p[i];
    if (str == NULL || str->tag_name == NULL) {
      continue;
    }
    for (size_t j = 0; j < str->members.length; j++) {
      StructMember* member = str->members.value.p[j];
      if (member == NULL || member->symbol == NULL) {
        continue;
      }
      // Only ordinary, named, non-static private data members are candidates.
      if (member->is_member_function || member->is_static || member->is_anon ||
          member->is_using_declaration || member->access != kAccessPrivate) {
        continue;
      }
      Symbol* sym = member->symbol;
      // Skip type members (nested using/typedef) and unnamed bitfields.
      if (sym->name.length == 0 || StorageIs(sym->storage, STO(typedef))) {
        continue;
      }
      if (sym->flags.used || SymbolHasAttribute(sym, "unused")) {
        continue;
      }
      // Match clang: do not diagnose members whose type must be destroyed
      // (RAII members are commonly held for their constructor/destructor side
      // effects rather than being referenced).
      if (TypeHasNonTrivialDestructor(sym->type)) {
        continue;
      }
      SemanticSymbolWarning(sym, "unused-private-field",
                            "private field '%s' is not used in class '%s'",
                            sym->name.value, str->tag_name->value);
    }
  }
}

static void CheckUnusedStaticFunctions(void) {
  for (size_t i = 0; i < compiler->declaration_asts.length; i++) {
    ASTNode* node = compiler->declaration_asts.value.p[i];
    if (node == NULL || node->op != AST_OP(decl_list)) {
      continue;
    }
    DeclarationListASTNode* decls = (DeclarationListASTNode*)node;
    for (size_t j = 0; j < decls->declarations->length; j++) {
      VariableDeclarationASTNode* decl =
          (VariableDeclarationASTNode*)decls->declarations->value.p[j];
      Symbol* sym = decl->symbol;
      if (sym != NULL && TypeIsFunction(sym->type) &&
          sym->flags.is_defined && StorageIs(sym->storage, STO(static)) &&
          !sym->flags.is_inline_defn && !FunctionSymbolWasUsed(sym)) {
        SemanticSymbolWarning(sym, "unused-function",
                              "static function '%s' is not used",
                              sym->name.value);
      }
    }
  }
}

// Emits -Wunused-variable / -Wunused-const-variable for file-scope data objects
// with internal linkage that are never referenced.  Only internal-linkage
// objects are diagnosed: an external-linkage object may be used from another
// translation unit, so it must be left alone.  Internal linkage is recognized
// as either an explicit 'static' or (C++ only) a namespace-scope const/constexpr
// object that is not declared 'extern'.
static void CheckUnusedGlobalVariables(void) {
  for (size_t i = 0; i < compiler->declaration_asts.length; i++) {
    ASTNode* node = compiler->declaration_asts.value.p[i];
    if (node == NULL || node->op != AST_OP(decl_list)) {
      continue;
    }
    DeclarationListASTNode* decls = (DeclarationListASTNode*)node;
    for (size_t j = 0; j < decls->declarations->length; j++) {
      VariableDeclarationASTNode* decl =
          (VariableDeclarationASTNode*)decls->declarations->value.p[j];
      Symbol* sym = decl->symbol;
      if (sym == NULL || sym->name.length == 0) {
        continue;
      }
      // Only ordinary data objects: not functions, typedefs, tags, or template
      // parameters.
      if (TypeIsFunction(sym->type) || StorageIs(sym->storage, STO(typedef))) {
        continue;
      }
      if (sym->flags.used || sym->flags.address_taken ||
          SymbolHasAttribute(sym, "unused")) {
        continue;
      }
      bool is_static = StorageIs(sym->storage, STO(static));
      bool is_extern = StorageIs(sym->storage, STO(extern));
      bool is_const = TypeIsConst(sym->type) || sym->flags.is_constexpr;
      // C++ namespace-scope const/constexpr objects have internal linkage unless
      // explicitly declared extern; C gives them external linkage.
      bool cxx_internal_const =
          CompilerIsCXX() && is_const && !is_static && !is_extern;
      if (!is_static && !cxx_internal_const) {
        continue;
      }
      if (is_const) {
        SemanticSymbolWarning(sym, "unused-const-variable",
                              "variable '%s' is not used", sym->name.value);
      } else {
        SemanticSymbolWarning(sym, "unused-variable",
                              "variable '%s' is not used", sym->name.value);
      }
      // Guard against re-warning the same object across multiple declarators.
      sym->flags.used = true;
    }
  }
}

// Compile a source file, returning name of object file allocated from
// the heap.  Compiler has already been initialized.
// Runs preprocessing, parsing and semantic analysis for the current translation
// unit, leaving the compiler's symbol/type/AST graph populated but NOT running
// code generation.  Returns true if the front end completed without errors.
// Used by the hidden -Xemit-module driver hook (and by module tooling) to obtain
// the post-semantic interface without producing an object file.
bool CompileFrontEndOnly(Compiler* compiler) {
  CreateGlobalSymbolTables();
  DeclarePredefinedTypesAndMacros(&compiler->preprocessor);

  LexNextToken(&compiler->lex);
  while (!LexEof(&compiler->lex)) {
    SyntaxResetForNewDeclaration(&compiler->syntax);
    CompileDeclaration(&compiler->syntax);
  }
  CheckUnusedStaticFunctions();
  CheckUnusedGlobalVariables();
  CheckUnusedPrivateFields();
  return NumErrors() == 0;
}

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
  CheckUnusedStaticFunctions();
  CheckUnusedGlobalVariables();
  CheckUnusedPrivateFields();
  
  if (compiler->print_front_end) {
    HashTablePrintStats(&compiler->global_symbol_table, compiler->ast_output_file);
    HashTablePrintStats(&compiler->global_tag_table, compiler->ast_output_file);
    PreprocessorPrintStats(&compiler->preprocessor, compiler->ast_output_file);
  }

  // Abort if there are any errors.
  if (NumErrors() != 0) {
    return NULL;
  }

  CompileCXXThreadLifetimeFunctions(&compiler->syntax);
  if (NumErrors() != 0) {
    return NULL;
  }

  PruneCXXNoOpGlobalConstructorCalls();
  CompileCXXProcessInitFunction(&compiler->syntax);
  if (NumErrors() != 0) {
    return NULL;
  }
  // Synthetic initialization functions may instantiate or reference further
  // inline functions after the ordinary source declarations have been drained.
  CompilePendingTemplateInstantiations(&compiler->syntax);
  CompileReferencedInlineFunctions(&compiler->syntax);
  if (NumErrors() != 0) {
    return NULL;
  }
  PruneUnreferencedCXXMetadata();
  PruneUnreferencedInlineVariables();
  // Emit the assembly language into a file ending in .s.
  bool output_asm_only = OptionBoolValue(kOptionAssemblyOutput, options, false);
  String asm_filename = {0};
  String* output_filename = OptionStringValue(kOptionOutputFile, options);
  if (output_asm_only && output_filename != NULL) {
    StringInit(&asm_filename, output_filename->value);
  } else {
    StringInit(&asm_filename, compiler->infile.value);
    ReplaceSourceExtension(&asm_filename, ".s");
  }
  bool ok = EmitAssemblyFile(compiler, &asm_filename);
  if (!ok) {
    StringDestruct(&asm_filename);
    return NULL;
  }
  
  // If the user specified -S then we don't assemble the output
  // and we keep the output assembly language file.
  if (output_asm_only) {
    String* result = output_filename;
    if (result == NULL) {
      result = NewString(asm_filename.value);
    }
    StringDestruct(&asm_filename);
    return result;
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
  // Release the global source-file table (file names, line tables and the file
  // map).  Source locations are no longer needed once the object file is built.
  ClearAllFiles();
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
  ClearAllFiles();
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
