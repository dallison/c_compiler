//
//  compiler.c
//  c_compiler
//
//  Created by David Allison on 11/1/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "compiler.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "codegen.h"
#include "constexpr.h"
#include "constexpr_pcode.h"
#include "errors.h"
#include "expr_semantics.h"
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "lex.h"
#include "preprocessor.h"
#include "semantics.h"
#include "syntax.h"
#include "debug.h"
#include "member_pointer.h"
#include "reflection.h"
#include "reflection_semantics.h"
#include "type_inheritance.h"
#include "type_compare.h"
#include "type_template.h"

#include "6502_target.h"
#include "p_code_target.h"
#include "risc_v_target.h"
#include "aarch64_target.h"
#include "arm_target.h"
#include "common_emitter.h"
#include "x86_64_target.h"
#include "x86_target.h"
#include "wasm32_target.h"

void EmitInitFiniArrayEntries(Vector* functions, bool is_fini, FILE* fp);

// This is global to avoid having to pass it around everywhere.
Compiler* compiler;

static Vector cxx_init_array_functions;
static Vector cxx_fini_array_functions;

static bool CompilerStringIndexContains(struct CompilerStringIndex* index,
                                        const char* key);
static bool CompilerStringIndexInsert(struct CompilerStringIndex* index,
                                      const char* key, void* value);

static CompilerOptionDefinition compiler_options[] = {
    {"-g", kCompilerOptionBool, kOptionDebug, false, "Generate debug info"},
    {"-O", kCompilerOptionString, kOptionOptimize, true,
     "Optimize with level (-O0, -O1, -O2, -O3, -Os)"},
    {"-target", kCompilerOptionString, kOptionTarget, false, "Specify one target architecture"},
    {"-c", kCompilerOptionBool, kOptionCompileOnly, false, "Compile only to object file"},
    {"-S", kCompilerOptionBool, kOptionAssemblyOutput, false, "Generate assembly language"},
    {"-fsyntax-only", kCompilerOptionBool, kOptionSyntaxOnly, false,
     "Parse and semantically analyze only; do not generate code"},
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
     "Select language standard: c89, c99, c11, c17, c23, c++11, c++17, c++20, c++23, c++26, c++29"},
    {"-fconstexpr-eval", kCompilerOptionString, kOptionConstexprEval, false,
     "Select constexpr evaluator: auto, pcode, ast, or audit"},
    {"-fcontracts", kCompilerOptionString, kOptionContracts, false,
     "Select contract semantic: ignore, observe, enforce, or quick-enforce"},
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
    {"-flto", kCompilerOptionBool, kOptionLTO, false,
     "Link-time optimization: compile all sources together for cross-TU inlining"},
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

bool CompilerTargetSupportsThreads(void) {
  return compiler != NULL && compiler->target_supports_threads;
}

bool CompilerTargetSupportsAtomics(void) {
  return compiler != NULL && compiler->target_supports_atomics;
}

bool CompilerTargetSupportsC11Atomics(void) {
  return compiler != NULL && compiler->target_supports_c11_atomics;
}

bool CompilerTargetSupportsAtomicSize(int size) {
  if (!CompilerTargetSupportsAtomics()) {
    return false;
  }
  return size == 1 || size == 2 || size == 4 ||
         (size == 8 && compiler->target_supports_8_byte_atomics);
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
#define kMaxTargetNames 6

enum {
  kTargetSupportsThreads = 1u << 0,
  kTargetSupportsAtomics = 1u << 1,
  // A complete C11 profile includes atomic_llong even when it is not lock-free.
  // ARM32 lacks 64-bit lowering; AArch64 lacks the required bit-preserving
  // floating-point transfer; pcode and wasm do not yet implement the profile.
  kTargetSupportsC11Atomics = 1u << 2,
  kTargetSupports8ByteAtomics = 1u << 3,
};

static struct CompilerTargetDefinition{
  const char* canonical_name;
  const char* names[kMaxTargetNames];
  CompilerTarget* (*factory)(void);
  bool static_linkage_only;
  int default_opt_level;
  unsigned capabilities;
} compiler_targets[] = {
  {"pcode", {"pcode", "p-code"}, NewPCodeTarget, false, 0,
   kTargetSupportsAtomics | kTargetSupports8ByteAtomics},
  {"riscv", {"riscv", "risc-v"}, NewRVTarget, false, 0,
   kTargetSupportsThreads | kTargetSupportsAtomics |
       kTargetSupportsC11Atomics | kTargetSupports8ByteAtomics},
  {"aarch64", {"aarch64", "armv8"}, NewAARCH64Target, false, 0,
   kTargetSupportsThreads | kTargetSupportsAtomics |
       kTargetSupports8ByteAtomics},
  {"arm", {"arm", "armv7", "armv7-a", "arm32"}, NewARMTarget, false, 0,
   kTargetSupportsThreads | kTargetSupportsAtomics},
  {"x86_64", {"x86_64", "x86-64"}, NewX86_64Target, false, 0,
   kTargetSupportsThreads | kTargetSupportsAtomics |
       kTargetSupportsC11Atomics | kTargetSupports8ByteAtomics},
  {"x86", {"x86", "i386", "i486", "i586", "i686", "x86-32"}, NewX86Target,
   false, 0, kTargetSupportsThreads | kTargetSupportsAtomics},
  {"6502", {"6502"}, New6502Target, true, 2, 0},
  {"65c02", {"65c02", "65C02"}, New65c02Target, true, 2, 0},
  {"wasm32", {"wasm32", "wasm"}, NewWasm32Target, true, 0,
   kTargetSupportsAtomics | kTargetSupports8ByteAtomics},
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

void CompilerTargetTripleInit(CompilerTargetTriple* triple) {
  StringInit(&triple->architecture, "");
  StringInit(&triple->vendor, "");
  StringInit(&triple->os_name, "");
  StringInit(&triple->environment, "");
  StringInit(&triple->canonical, "");
  triple->os = kTargetOSNone;
  triple->explicit_triple = false;
}

void CompilerTargetTripleDestruct(CompilerTargetTriple* triple) {
  StringDestruct(&triple->architecture);
  StringDestruct(&triple->vendor);
  StringDestruct(&triple->os_name);
  StringDestruct(&triple->environment);
  StringDestruct(&triple->canonical);
}

static bool SetTargetTripleError(char* error, size_t error_size,
                                 const char* format, const char* value) {
  if (error != NULL && error_size != 0) {
    snprintf(error, error_size, format, value);
  }
  return false;
}

bool CompilerTargetTripleParse(CompilerTargetTriple* triple, const char* value,
                               char* error, size_t error_size) {
  if (value == NULL || value[0] == '\0') {
    return SetTargetTripleError(error, error_size, "empty target '%s'",
                                value == NULL ? "" : value);
  }

  String whole;
  StringInit(&whole, value);
  struct CompilerTargetDefinition* definition = FindTarget(&whole);
  if (definition != NULL) {
    StringSet(&triple->architecture, definition->canonical_name);
    StringSet(&triple->vendor, "unknown");
    StringSet(&triple->os_name, "none");
    StringSet(&triple->environment, "davecc");
    triple->os = kTargetOSNone;
    triple->explicit_triple = false;
    StringClear(&triple->canonical);
    StringPrintf(&triple->canonical, "%s-unknown-none-davecc",
                 definition->canonical_name);
    StringDestruct(&whole);
    return true;
  }
  StringDestruct(&whole);

  const char* first = strchr(value, '-');
  const char* second = first == NULL ? NULL : strchr(first + 1, '-');
  const char* third = second == NULL ? NULL : strchr(second + 1, '-');
  if (first == NULL || second == NULL || third == NULL ||
      strchr(third + 1, '-') != NULL || first == value ||
      second == first + 1 || third == second + 1 || third[1] == '\0') {
    return SetTargetTripleError(
        error, error_size,
        "target '%s' must be an architecture or arch-vendor-os-environment",
        value);
  }

  String architecture;
  StringInitFromSegment(&architecture, value, (size_t)(first - value));
  definition = FindTarget(&architecture);
  StringDestruct(&architecture);
  if (definition == NULL) {
    return SetTargetTripleError(error, error_size,
                                "unknown target architecture in '%s'", value);
  }

  StringSet(&triple->architecture, definition->canonical_name);
  StringClear(&triple->vendor);
  StringAppendSegment(&triple->vendor, first + 1,
                      (size_t)(second - first - 1));
  StringClear(&triple->os_name);
  StringAppendSegment(&triple->os_name, second + 1,
                      (size_t)(third - second - 1));
  StringSet(&triple->environment, third + 1);
  triple->explicit_triple = true;
  if (StringEqual(&triple->os_name, "linux")) {
    triple->os = kTargetOSLinux;
  } else if (StringEqual(&triple->os_name, "none")) {
    triple->os = kTargetOSNone;
  } else {
    return SetTargetTripleError(error, error_size,
                                "unsupported target OS in '%s'", value);
  }
  if (!StringEqual(&triple->environment, "davecc")) {
    return SetTargetTripleError(error, error_size,
                                "unsupported target environment in '%s'",
                                value);
  }
  if (triple->os == kTargetOSLinux &&
      (strcmp(definition->canonical_name, "pcode") == 0 ||
       strcmp(definition->canonical_name, "6502") == 0 ||
       strcmp(definition->canonical_name, "65c02") == 0)) {
    return SetTargetTripleError(error, error_size,
                                "Linux is not supported by target '%s'", value);
  }
  StringClear(&triple->canonical);
  StringPrintf(&triple->canonical, "%s-%s-%s-%s",
               definition->canonical_name, triple->vendor.value,
               triple->os_name.value, triple->environment.value);
  return true;
}

bool CompilerTargetTripleIsLinux(const CompilerTargetTriple* triple) {
  return triple != NULL && triple->os == kTargetOSLinux;
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





// The initializer occupies the storage of the object being initialized, so its
// encoding follows the declared type rather than the type of the expression;
// after a diagnostic the two need not even be in the same family.
static void InitInteger(ASTNode* expr,
                        TypeRecord* type,
                        Initializer* init_out,
                        int offset,
                        Vector* initializers) {
  int64_t value = 0;
  if (EvaluateIntegerExpression(expr, &value)) {
    if (TypeIsBitInt(type)) {
      switch (type->size) {
        case 1:
          init_out->type = kInitTypeByte;
          init_out->value.byte = (uint8_t)value;
          break;
        case 2:
          init_out->type = kInitTypeHalf;
          init_out->value.half = (uint16_t)value;
          break;
        case 4:
          init_out->type = kInitTypeWord;
          init_out->value.word = (uint32_t)value;
          break;
        case 8:
          init_out->type = kInitTypeLong;
          init_out->value._long = (uint64_t)value;
          break;
        default:
          SemanticError(expr, "Unsupported _BitInt storage size");
          free(init_out);
          return;
      }
    } else if (TypeIsCharFamily(type) || TypeIsBool(type)) {
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
      SemanticError(expr, "Unsupported type for static initialization");
      free(init_out);
      return;
    }
    init_out->offset = offset;
    VectorAppend(initializers, init_out);
  } else {
    SemanticError(
        expr, "Invalid static initialization; need a constant expression");
  }

}

static void InitFloatingPoint(ASTNode* expr,
                              TypeRecord* type,
                              Initializer* init_out,
                              int offset,
                              Vector* initializers) {
  double value = 0;
  if (EvaluateFloatingPointExpression(expr, &value)) {
    if (TypeUsesFloat32Representation(type)) {
      init_out->type = kInitTypeWord;
      float fvalue = (float)value;
      init_out->value.word = *((int32_t*)&fvalue);
    } else if (TypeUsesFloat64Representation(type)) {
      if (type->size == 4) {
        init_out->type = kInitTypeWord;
        float f = value;
        init_out->value.word = *((int32_t*)&f);
      } else {
        init_out->type = kInitTypeLong ;
        init_out->value._long = *((int64_t*)&value);
      }
    } else {
      SemanticError(expr, "Unsupported type for static initialization");
      free(init_out);
      return;
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

static int StringLiteralElementSize(ASTNode* expr) {
  if (expr != NULL && TypeIsArray(expr->type) && expr->type->next != NULL) {
    TypeRecordCalculateSize(expr->type->next);
    return expr->type->next->size;
  }
  return expr != NULL && expr->op == AST_OP(string_wide)
             ? compiler->wchar_size
             : 1;
}

// The address of a static object shifted by a byte offset, which is what an
// address constant is: C reaches one by selecting a member, subscripting or
// adding an integer to a pointer, and each of those only moves the offset.
typedef struct {
  Symbol* symbol;
  int64_t addend;
} AddressConstant;

// Size of the object a pointer or array expression addresses, for scaling the
// integer in pointer arithmetic.  Zero when it cannot be determined, which makes
// the caller give up rather than scale by a guess.
static int64_t InitAddressedSize(TypeRecord* type) {
  if (type == NULL || type->next == NULL) {
    return 0;
  }
  TypeRecordCalculateSize(type->next);
  return type->next->size;
}

// Does the symbol live at an address the linker can name?  An object with
// automatic storage does not: it has no address until its frame exists, so
// "&local" and everything reached from it is not an address constant even though
// it is spelled like one.
static bool SymbolHasLinkTimeAddress(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  if (TypeIsFunction(symbol->type) ||
      StorageIs(symbol->storage, STO(static) | STO(extern)) ||
      CompilerSymbolIsMetaPromotedStatic(symbol)) {
    return true;
  }
  return !symbol->flags.is_block_scope && !symbol->flags.is_argument;
}

// Evaluate expr, whose value is an address, as an address constant.
static bool InitAddressConstant(ASTNode* expr, AddressConstant* out);

// Evaluate the object expr designates, for an expr that is an lvalue rather than
// a value: the operand of '&', or the base of a member selection.
static bool InitDesignatedObject(ASTNode* expr, AddressConstant* out) {
  if (expr == NULL) {
    return false;
  }
  switch (expr->op) {
    case AST_OP(identifier): {
      Symbol* symbol = ((IdentifierASTNode*)expr)->symbol;
      if (!SymbolHasLinkTimeAddress(symbol)) {
        return false;
      }
      out->symbol = symbol;
      out->addend = 0;
      return true;
    }
    case AST_OP(structmember): {
      // A static data member or a member function stands for a symbol of its
      // own rather than a place inside an enclosing object.
      StructMember* member = ((StructMemberASTNode*)expr)->member;
      if (member == NULL || member->symbol == NULL ||
          !(member->is_static || member->is_member_function)) {
        return false;
      }
      out->symbol = member->symbol;
      out->addend = 0;
      return true;
    }
    case AST_OP(dot):
    case AST_OP(arrow): {
      if (ASTNodeGetShape(expr) != kASTShapeBinary) {
        return false;
      }
      BinaryASTNode* access = (BinaryASTNode*)expr;
      if (access->right == NULL ||
          access->right->op != AST_OP(structmember)) {
        return false;
      }
      StructMemberASTNode* member = (StructMemberASTNode*)access->right;
      if (member->member != NULL && member->member->is_bit_field) {
        // A bitfield has no address of its own.
        return false;
      }
      // The left of a '.' designates an object; the left of a '->' is a pointer
      // value, so it is an address constant in its own right.
      bool ok = expr->op == AST_OP(dot)
                    ? InitDesignatedObject(access->left, out)
                    : InitAddressConstant(access->left, out);
      if (!ok) {
        return false;
      }
      out->addend += member->byte_offset;
      return true;
    }
    case AST_OP(subscript): {
      if (ASTNodeGetShape(expr) != kASTShapeBinary) {
        return false;
      }
      BinaryASTNode* subscript = (BinaryASTNode*)expr;
      int64_t index;
      if (subscript->left == NULL ||
          !EvaluateIntegerExpression(subscript->right, &index)) {
        return false;
      }
      int64_t element_size = InitAddressedSize(subscript->left->type);
      if (element_size == 0 ||
          index < INT32_MIN / element_size || index > INT32_MAX / element_size) {
        // An offset that does not fit an address is either a bad subscript or an
        // overflow in the multiplication below.
        return false;
      }
      // Subscripting an array designates a place inside it; subscripting a
      // pointer starts from the address that pointer holds.
      bool ok = TypeIsArray(subscript->left->type)
                    ? InitDesignatedObject(subscript->left, out)
                    : InitAddressConstant(subscript->left, out);
      if (!ok) {
        return false;
      }
      out->addend += index * element_size;
      return true;
    }
    case AST_OP(contents):
      // *p designates whatever p addresses.
      if (ASTNodeGetShape(expr) != kASTShapeUnary) {
        return false;
      }
      return InitAddressConstant(((UnaryASTNode*)expr)->sub, out);
    default:
      return false;
  }
}

static bool InitAddressConstant(ASTNode* expr, AddressConstant* out) {
  if (expr == NULL) {
    return false;
  }
  switch (expr->op) {
    case AST_OP(cast):
      // A cast between pointer types keeps the address; one that converts to or
      // from something else is not an address constant we can encode.
      if (!TypeIsPointer(expr->type) && !TypeIsArray(expr->type)) {
        return false;
      }
      return InitAddressConstant(((CastASTNode*)expr)->expr, out);
    case AST_OP(address):
      if (ASTNodeGetShape(expr) != kASTShapeUnary) {
        return false;
      }
      return InitDesignatedObject(((UnaryASTNode*)expr)->sub, out);
    case AST_OP(identifier):
      // An array or function name used as a value is the address of the object
      // itself.
      return InitDesignatedObject(expr, out);
    case AST_OP(structmember):
      return InitDesignatedObject(expr, out);
    case AST_OP(plus):
    case AST_OP(minus): {
      if (ASTNodeGetShape(expr) != kASTShapeBinary) {
        return false;
      }
      BinaryASTNode* arithmetic = (BinaryASTNode*)expr;
      ASTNode* base = arithmetic->left;
      ASTNode* count = arithmetic->right;
      if (base == NULL || count == NULL) {
        return false;
      }
      if (!TypeIsPointer(base->type) && !TypeIsArray(base->type)) {
        // "n + p" is the same address as "p + n", but subtracting from an
        // integer is not an address at all.
        if (expr->op == AST_OP(minus)) {
          return false;
        }
        ASTNode* swap = base;
        base = count;
        count = swap;
      }
      // Semantic analysis has already multiplied the operand by the size of the
      // pointed-to type (see AnalyzePlusOperator), so it is a count of bytes and
      // is added as it stands.
      int64_t byte_offset;
      if (!EvaluateIntegerExpression(count, &byte_offset) ||
          byte_offset < INT32_MIN || byte_offset > INT32_MAX ||
          !InitAddressConstant(base, out)) {
        return false;
      }
      out->addend +=
          expr->op == AST_OP(minus) ? -byte_offset : byte_offset;
      return true;
    }
    default:
      return false;
  }
}

static void InitPointer(ASTNode* expr,
                        ASTNode* subinit,
                        Initializer* init_out,
                        int offset,
                        Vector* initializers) {
  if (expr->op == AST_OP(string) || expr->op == AST_OP(string_wide)) {
    // String literal.
    ConstantASTNode* string_node = (ConstantASTNode*)expr;
    int literal_id = CompilerAddStringLiteral(
        string_node->value.string, StringLiteralElementSize(expr));
    init_out->type = kInitTypeString;
    init_out->value.literal_id = literal_id;
    init_out->offset = offset;
    VectorAppend(initializers, init_out);
    return;
  }
  if (expr->op == AST_OP(number)) {
    // A null pointer constant is represented by an integer-valued AST node,
    // including the dedicated std::nullptr_t node.  Encode it using the
    // destination pointer width rather than the source expression's type.
    int64_t value = 0;
    TypeRecordCalculateSize(subinit->type);
    bool is_nullptr = TypeIsNullPointer(expr->type);
    bool is_zero_literal =
        ((ConstantASTNode*)expr)->value.ivalue == 0;
    if ((!is_nullptr && !is_zero_literal &&
         !EvaluateIntegerExpression(expr, &value)) ||
        value != 0) {
      SemanticError(expr,
                    "Invalid static pointer initialization; expected null");
      free(init_out);
      return;
    }
    if (subinit->type->size == 2) {
      init_out->type = kInitTypeHalf;
      init_out->value.half = 0;
    } else if (subinit->type->size == 4) {
      init_out->type = kInitTypeWord;
      init_out->value.word = 0;
    } else {
      init_out->type = kInitTypeLong;
      init_out->value._long = 0;
    }
    init_out->offset = offset;
    VectorAppend(initializers, init_out);
    return;
  }
  AddressConstant address;
  if (InitAddressConstant(expr, &address)) {
    init_out->type = kInitTypeSymbol;
    init_out->value.symbol = address.symbol;
    init_out->symbol_addend = address.addend;
    init_out->offset = offset;
    VectorAppend(initializers, init_out);
    return;
  }

  ASTNode* var_node;
  if (expr->op == AST_OP(address)) {
    UnaryASTNode* addr_node = (UnaryASTNode*)expr;
    var_node = addr_node->sub;
  } else {
    var_node = expr;
  }

  // A cast of something that is not an address, such as an integer used as a
  // pointer.
  if (var_node->op == AST_OP(cast)) {
    CastASTNode* c = (CastASTNode*)var_node;
    InitScalar(c->expr, subinit, offset, initializers);
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
  Initializer* init_out = calloc(1, sizeof(Initializer));
  TypeRecord* type = subinit->type;
  TypeRecordCalculateSize(type);
  if (TypeIsIntegral(type)) {
    InitInteger(expr, type, init_out, offset, initializers);
  } else if (TypeIsFloatingPoint(type)) {
    InitFloatingPoint(expr, type, init_out, offset, initializers);
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
      int literal_id = CompilerAddStringLiteral(
          string_node->value.string, StringLiteralElementSize(expr));
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
  Initializer* init_out = calloc(1, sizeof(Initializer));
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
  if (sym == NULL || sym->type == NULL) {
    return 1;
  }
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
            offset += d->value.base != NULL
                          ? d->value.base->byte_offset
                          : d->base_byte_offset;
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
  if (!TypeIsIntegral(type) && !TypeIsFloatingPoint(type) &&
      !TypeIsPointer(type) && !TypeIsEnum(type) &&
      !TypeIsMemberPointer(type)) {
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

static bool InitializedStaticAlreadyRegistered(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* existing =
        compiler->initialized_static_variables.value.p[i];
    if (existing != NULL && existing->symbol == symbol) {
      return true;
    }
  }
  return false;
}

static void CompilerRegisterConstexprStatic(Symbol* symbol,
                                            ASTNode* initializer) {
  if (compiler == NULL || symbol == NULL || initializer == NULL) {
    return;
  }
  if (InitializedStaticAlreadyRegistered(symbol)) {
    return;
  }
  SyntaxAddSymbol(&compiler->syntax, symbol);
  ASTNode* init = initializer;
  if (init->op == AST_OP(expr_init)) {
    init = ((ExpressionInitializerASTNode*)init)->expr;
  }
  ASTNode* simplified = AnalyzeInitializer(symbol->type, init, true);
  if (simplified == NULL || simplified->op != AST_OP(braced_init)) {
    return;
  }
  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = symbol;
  var->is_global = !StorageIs(symbol->storage, STO(static));
  var->is_weak = SymbolHasWeakBinding(symbol);
  VectorInit(&var->initializers);
  var->size = symbol->type->size;
  var->is_tls = StorageIs(symbol->storage, STO(thread));
  var->is_local = symbol->flags.is_local;
  var->alignment = SymbolEffectiveAlignment(symbol);
  ExpandBracedInitializer((BracedInitializerASTNode*)simplified, 0,
                          &var->initializers);
  VectorAppend(&compiler->initialized_static_variables, var);
  AssignScalarValueToConst(symbol, (BracedInitializerASTNode*)simplified);
  CompilerMarkVariableReferenced(symbol);
}

void CompilerRegisterMetaPromotedStatic(Symbol* symbol, ASTNode* initializer) {
  if (symbol != NULL && !SymbolHasAttribute(symbol, "meta_promoted_static")) {
    SymbolAddAttribute(symbol, NewAttribute("meta_promoted_static"));
  }
  CompilerRegisterConstexprStatic(symbol, initializer);
}

void CompilerRegisterTemplateParameterObject(Symbol* symbol,
                                             ASTNode* initializer) {
  CompilerRegisterConstexprStatic(symbol, initializer);
}

bool CompilerSymbolIsMetaPromotedStatic(Symbol* symbol) {
  return symbol != NULL && SymbolHasAttribute(symbol, "meta_promoted_static");
}

bool CompilerSymbolIsMetaPromotedString(Symbol* symbol) {
  if (!CompilerSymbolIsMetaPromotedStatic(symbol)) {
    return false;
  }
  for (size_t i = 0; i < compiler->meta_promoted_statics.length; i++) {
    MetaPromotedStaticEntry* entry = compiler->meta_promoted_statics.value.p[i];
    if (entry != NULL && entry->symbol == symbol && entry->key.value != NULL &&
        strncmp(entry->key.value, "str:", 4) == 0) {
      return true;
    }
  }
  return false;
}

Symbol* CompilerMetaPromotedPointerTarget(Symbol* pointer_symbol) {
  if (pointer_symbol == NULL || pointer_symbol->type == NULL ||
      !TypeIsPointer(pointer_symbol->type) || pointer_symbol->value.other == NULL ||
      (!pointer_symbol->flags.value_set &&
       !pointer_symbol->flags.is_constexpr &&
       !pointer_symbol->flags.is_constinit)) {
    return NULL;
  }
  Symbol* target = (Symbol*)pointer_symbol->value.other;
  return CompilerSymbolIsMetaPromotedStatic(target) ? target : NULL;
}

Symbol* CompilerConstantFunctionPointerTarget(Symbol* pointer_symbol) {
  if (pointer_symbol == NULL || pointer_symbol->type == NULL ||
      !TypeIsPointer(pointer_symbol->type) || pointer_symbol->value.other == NULL ||
      (!pointer_symbol->flags.value_set &&
       !pointer_symbol->flags.is_constexpr &&
       !pointer_symbol->flags.is_constinit)) {
    return NULL;
  }
  Symbol* target = (Symbol*)pointer_symbol->value.other;
  return target->type != NULL && TypeIsFunction(target->type) ? target : NULL;
}

bool ConstexprEnsureMetaPromotedStaticObject(Symbol* promoted) {
  if (!CompilerSymbolIsMetaPromotedStatic(promoted)) {
    return false;
  }
  if (promoted->flags.value_set && promoted->value.other != NULL &&
      !TypeIsPointer(promoted->type)) {
    return true;
  }
  if (promoted->constexpr_initializer == NULL) {
    return false;
  }
  return ConstexprEvaluateObjectConstantForSymbol(promoted,
                                                  promoted->constexpr_initializer);
}

void InitializerDelete(Initializer* init) {
  if (init->type == kInitTypeMemory) {
    BufferDestruct(&init->value.memory);
  }
  free(init);
}

Initializer* NewSymbolInitializer(int32_t offset, Symbol* symbol,
                                  int64_t addend) {
  Initializer* init = calloc(1, sizeof(Initializer));
  init->type = kInitTypeSymbol;
  init->offset = offset;
  init->value.symbol = symbol;
  init->symbol_addend = addend;
  return init;
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

int CompilerAddStringLiteral(String* value, int element_size) {
  StringLiteral* literal = malloc(sizeof(StringLiteral));
  LiteralInit(&literal->base,
              element_size > 1 ? kLiteralWideString : kLiteralString);
  StringInitFromSegment(&literal->value, value->value, value->length);
  literal->element_size = element_size;
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
static bool LocalStaticAlreadyRegistered(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* existing =
        compiler->initialized_static_variables.value.p[i];
    if (existing != NULL && existing->symbol == symbol) {
      return true;
    }
  }
  for (size_t i = 0; i < compiler->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* existing =
        compiler->uninitialized_static_variables.value.p[i];
    if (existing != NULL && existing->symbol == symbol) {
      return true;
    }
  }
  return false;
}

static void RemoveUninitializedStaticForSymbol(Symbol* symbol) {
  if (symbol == NULL) {
    return;
  }
  for (size_t i = 0; i < compiler->uninitialized_static_variables.length;) {
    UninitializedStaticVariable* existing =
        compiler->uninitialized_static_variables.value.p[i];
    if (existing != NULL && existing->symbol == symbol) {
      UninitializedStaticVariableDelete(existing);
      VectorDeleteElement(&compiler->uninitialized_static_variables, i);
      continue;
    }
    i++;
  }
}

static void AddLocalStatics(Syntax* syntax, TypeRecord* function) {
  Vector declarations;
  VectorInit(&declarations);
  ASTNodeVisit(function->info.function.body, CollectFunctionLocalStatic, 0,
               &declarations);
  for (size_t i = 0; i < declarations.length; i++) {
    VariableDeclarationASTNode* decl =
        (VariableDeclarationASTNode*)VectorGet(&declarations, i);
    if (decl->symbol != NULL && LocalStaticAlreadyRegistered(decl->symbol)) {
      continue;
    }
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

static const char* FunctionEmitName(Symbol* symbol) {
  if (symbol == NULL) {
    return "";
  }
  if (symbol->asm_name.length != 0) {
    return symbol->asm_name.value;
  }
  return symbol->name.value;
}

static bool FunctionAsmNameAlreadyEmitted(const char* asm_name) {
  return CompilerStringIndexContains(
      compiler->emitted_function_name_index, asm_name);
}

static void CompileReferencedInlineFunctions(Syntax* syntax);

void CompilerMarkFunctionReferenced(Symbol* symbol) {
  if (compiler == NULL || symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type)) {
    return;
  }
  TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, symbol);
  // Do not assign an asm name here.  Compiler-invented runtime declarations
  // deliberately have an empty asm_name so they retain their ABI spelling
  // (for example __davecc_throw rather than a C++-mangled name).
  const char* asm_name = symbol->asm_name.length != 0
                             ? symbol->asm_name.value
                             : symbol->name.value;
  bool first_reference =
      !CompilerStringIndexContains(
          compiler->referenced_function_name_index, asm_name);
  if (first_reference && symbol->is_imported_module_symbol &&
      StorageIs(symbol->storage, STO(static)) &&
      symbol->type->info.function.template_origin != NULL &&
      symbol->type->info.function.body != NULL) {
    // An internal-linkage template specialization imported with a module is
    // local to the importing translation unit.  Its serialized body cannot be
    // satisfied by the module object's private definition, so emit a local
    // copy when this importer first references it.
    CompilerQueuePendingFunctionDefinition(symbol);
  }
  if (first_reference &&
      CompilerStringIndexInsert(compiler->referenced_function_name_index,
                                asm_name, symbol)) {
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
         CompilerStringIndexContains(
             compiler->referenced_function_name_index,
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
    if (var == NULL ||
        (IsLazyCXXStatic(var) &&
         !VariableSymbolIsReferenced(var->symbol))) {
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
  return CompilerStringIndexContains(
      compiler->referenced_function_name_index, asm_name);
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
      function->info.function.body != NULL &&
      !function->info.function.references_marked) {
    // The global reference sets only grow.  Once this body's outgoing edges
    // have been added, rescanning it in later fixed-point iterations cannot
    // discover anything new.
    function->info.function.references_marked = true;
    MarkReferencesInAST(function->info.function.body);
  }
}

static bool TypeContainsUninstantiatedClassTemplate(TypeRecord* type);

static bool TemplateArgumentContainsUninstantiatedClassTemplate(
    TemplateArgument* argument) {
  if (argument == NULL) {
    return false;
  }
  if (TypeContainsUninstantiatedClassTemplate(argument->type)) {
    return true;
  }
  for (size_t i = 0; argument->pack_arguments != NULL &&
                     i < argument->pack_arguments->length; i++) {
    if (TemplateArgumentContainsUninstantiatedClassTemplate(
            argument->pack_arguments->value.p[i])) {
      return true;
    }
  }
  return false;
}

static bool TypeContainsUninstantiatedClassTemplate(TypeRecord* type) {
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (TypeIsStructOrUnion(current) && current->info.struct_info != NULL &&
        current->info.struct_info->is_template &&
        current->template_arguments == NULL &&
        (current->info.struct_info->tag_symbol == NULL ||
         current->info.struct_info->tag_symbol->type == NULL ||
         current->info.struct_info->tag_symbol->type->template_arguments ==
             NULL)) {
      return true;
    }
    for (size_t i = 0; current->template_arguments != NULL &&
                       i < current->template_arguments->length; i++) {
      if (TemplateArgumentContainsUninstantiatedClassTemplate(
              current->template_arguments->value.p[i])) {
        return true;
      }
    }
    if (TypeIsFunction(current)) {
      for (size_t i = 0; i < current->info.function.prototype.length; i++) {
        Symbol* formal = current->info.function.prototype.value.p[i];
        if (formal != NULL &&
            TypeContainsUninstantiatedClassTemplate(formal->type)) {
          return true;
        }
      }
    }
  }
  return false;
}

static bool GenerateFunctionDefinition(Syntax* syntax,
                                       VariableDeclarationASTNode* decl) {
  if (compiler->syntax_only || NumErrors() != 0) {
    return false;
  }
  if (FunctionAsmNameAlreadyEmitted(FunctionEmitName(decl->symbol))) {
    return false;
  }
  bool dependent_function_body =
      TypeContainsTemplateParameter(decl->base.type) ||
      TypeContainsUninstantiatedClassTemplate(decl->base.type) ||
      FunctionBodyContainsUnexpandedPack(
          decl->base.type->info.function.body);
  bool uninstantiated_function_template =
      decl->symbol->flags.is_template &&
      decl->base.type->info.function.template_origin == NULL;
  bool unnamed_cxx_function =
      CompilerIsCXX() && decl->symbol->asm_name.length == 0 &&
      !decl->symbol->flags.invented && !decl->symbol->flags.is_c_linkage &&
      strcmp(decl->symbol->name.value, "main") != 0 &&
      strncmp(decl->symbol->name.value, "__", 2) != 0;
  if (dependent_function_body || uninstantiated_function_template ||
      unnamed_cxx_function ||
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
  if (compiler->lto_ir_only) {
    GenerateFunctionIR(&codegen);
    if (compiler->lto_module == NULL) {
      compiler->lto_module = LTOModuleCreate();
    }
    LTOModuleAddFunction(compiler->lto_module, &codegen);
    const char* emit_name = FunctionEmitName(decl->symbol);
    VectorAppend(&compiler->emitted_function_asm_names, NewString(emit_name));
    CompilerStringIndexInsert(compiler->emitted_function_name_index, emit_name,
                              decl->symbol);
    AddLocalStatics(syntax, decl->base.type);
    GeneratorDestruct(&codegen);
  } else {
    void* code = GenerateFunction(&codegen);
    if (code != NULL) {
      VectorAppend(&compiler->functions, code);
      const char* emit_name = FunctionEmitName(decl->symbol);
      VectorAppend(&compiler->emitted_function_asm_names, NewString(emit_name));
      CompilerStringIndexInsert(compiler->emitted_function_name_index, emit_name,
                                decl->symbol);

      if (compiler->debug_output) {
        BuildDebugInfoAfterCodegen(&compiler->debug_builder, decl->symbol);
      }
    }
    AddLocalStatics(syntax, decl->base.type);
    GeneratorDestruct(&codegen);
  }
  SyntaxRegisterFunctionInitFiniAttributes(syntax, decl->symbol);
  compiler->current_function = saved_current_function;
  compiler->current_class_access_context = saved_class_access_context;
  return true;
}

static void CompileFunctionDefinitionNode(
    Syntax* syntax, VariableDeclarationASTNode* decl) {
  if (FunctionAsmNameAlreadyEmitted(FunctionEmitName(decl->symbol))) {
    return;
  }
  CheckMainSignature(syntax, decl->symbol);

  TypeRecord* saved_current_function = compiler->current_function;
  Struct* saved_class_access_context =
      compiler->current_class_access_context;
  compiler->current_function = decl->base.type;
  if (decl->base.type != NULL && TypeIsFunction(decl->base.type)) {
    compiler->current_class_access_context =
        decl->base.type->info.function.cxx_member_owner;
  }

  SemanticAnalyzeFunction(syntax, (ASTNode*)decl);
  if (compiler->print_front_end) {
    SymbolPrintDetails(decl->symbol, true, compiler->ast_output_file);
  }
  compiler->current_function = saved_current_function;
  compiler->current_class_access_context = saved_class_access_context;
  if (decl->base.type == NULL || !TypeIsFunction(decl->base.type)) {
    return;
  }
  // Reference closure only controls which ODR-discardable definitions reach
  // code generation. A syntax-only compilation returns before that phase.
  if (!compiler->syntax_only &&
      !FunctionDefinitionIsODRDiscardable(decl->base.type)) {
    MarkFunctionsReferencedByBody(decl->base.type);
  }
  if (!(decl->base.type->info.function.is_consteval ||
        (decl->base.type->info.function.is_constexpr &&
         TypeIsConstevalOnly(decl->base.type)))) {
    GenerateFunctionDefinition(syntax, decl);
  }
}

static void CompileDeclarationNode(Syntax* syntax, ASTNode* node) {
  if (node != NULL) {
    // Retain the root so the whole AST can be torn down at CompilerDestruct.
    VectorAppend(&compiler->declaration_asts, node);
    if (node->op == AST_OP(consteval_block)) {
      SemanticAnalyzeConstevalBlock((ConstevalBlockASTNode*)node);
      return;
    }
    // 'node' will be a declaration list containing declarations.
    if (node->op == AST_OP(decl_list)) {
      DeclarationListASTNode* decls = (DeclarationListASTNode*)node;
      size_t num_decls = decls->declarations->length;
      for (size_t i = 0; i < num_decls; i++) {
        ASTNode* decl_node = decls->declarations->value.p[i];
        if (decl_node != NULL && decl_node->op == AST_OP(consteval_block)) {
          SemanticAnalyzeConstevalBlock((ConstevalBlockASTNode*)decl_node);
          continue;
        }
        if (decl_node == NULL || decl_node->op != AST_OP(vardecl)) {
          continue;
        }
        VariableDeclarationASTNode* decl =
            (VariableDeclarationASTNode*)decl_node;
        if (decl->symbol == NULL) {
          continue;
        }

        if (decl->symbol->flags.is_template) {
          continue;
        }
        if (IsFunctionOrInlineDefinition(decl->symbol)) {
          CompileFunctionDefinitionNode(syntax, decl);
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
            if (decl->symbol->type != NULL) {
              decl->symbol->type = SemanticResolveDependentSpliceType(
                  decl->symbol->type, (ASTNode*)decl);
            }
            if (TypeIsVoid(decl->symbol->type)) {
              SemanticError((ASTNode*)decl,
                            "Cannot declare a variable with void type");
            }
            if (decl->symbol->flags.is_defined || decl->symbol->flags.is_tentative_decl) {
              // Extern or static variable definition.
              if (decl->initializer == NULL) {
                // No initializer.  Add as unitialized static variable.
                // Tentative `int x;` in a later LTO TU reuses the same
                // Symbol*, so skip a second BSS entry.
                if (!LocalStaticAlreadyRegistered(decl->symbol)) {
                  UninitializedStaticVariable* var =
                      malloc(sizeof(UninitializedStaticVariable));
                  var->symbol = decl->symbol;
                  var->is_global =
                      !StorageIs(decl->symbol->storage, STO(static));
                  var->is_weak = SymbolHasWeakBinding(decl->symbol);
                  var->size = decl->symbol->type != NULL
                                  ? decl->symbol->type->size
                                  : 0;
                  var->alignment = SymbolEffectiveAlignment(decl->symbol);
                  var->is_tls = StorageIs(decl->symbol->storage, STO(thread));
                  var->is_local = decl->symbol->flags.is_local;
                  VectorAppend(&compiler->uninitialized_static_variables, var);
                  RegisterCXXGlobalObject(decl->symbol);
                }
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
                if (TypeIsConstevalOnly(decl->symbol->type)) {
                  if (!decl->symbol->flags.is_constexpr) {
                    SemanticError(
                        (ASTNode*)decl,
                        "A variable of consteval-only reflection type must be "
                        "declared constexpr");
                  }
                  ASTNode* reflection_expr =
                      ConstexprInitializerExpression(decl->initializer);
                  reflection_expr = AnalyzeExpression(reflection_expr);
                  ReflectionValue* reflection =
                      TypeIsReflection(decl->symbol->type)
                          ? SemanticEvaluateReflection(reflection_expr)
                          : SemanticReflectionValueFromExpression(
                                reflection_expr);
                  if (TypeIsReflection(decl->symbol->type) &&
                      reflection != NULL) {
                    decl->symbol->value.other = reflection;
                    decl->symbol->flags.value_set = true;
                  } else if (!TypeIsReflection(decl->symbol->type) &&
                             ConstexprEvaluateObjectConstantForSymbol(
                                 decl->symbol, decl->initializer)) {
                    decl->symbol->flags.value_set = true;
                  } else if (!ExpressionIsTemplateDependent(reflection_expr)) {
                    SemanticError(
                        (ASTNode*)decl,
                        "consteval-only reflection initializer is not a "
                        "constant expression");
                  }
                  // Reflection values have no runtime representation.
                  continue;
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
                RemoveUninitializedStaticForSymbol(decl->symbol);
                if (!InitializedStaticAlreadyRegistered(decl->symbol)) {
                  AddInitializedStaticVariable(decl, simplified_init);
                }
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

void CompilerCompileQueuedDeclaration(Syntax* syntax, ASTNode* node) {
  if (node == NULL) {
    return;
  }
  CompileDeclarationNode(syntax, node);
  (void)syntax;
}

static void ForgetUnindexedPendingTemplateInstantiation(ASTNode* node) {
  Vector* unindexed =
      &compiler->unindexed_pending_template_instantiations;
  for (size_t i = 0; i < unindexed->length; i++) {
    if (unindexed->value.p[i] == node) {
      VectorDeleteElement(unindexed, i);
      return;
    }
  }
}

static void CompilePendingTemplateInstantiations(Syntax* syntax) {
  Vector* pending = &compiler->pending_template_instantiations;
  compiler->pending_template_instantiation_drain_depth++;
  while (compiler->pending_template_instantiation_head < pending->length) {
    ASTNode* node =
        pending->value.p[compiler->pending_template_instantiation_head++];
    ForgetUnindexedPendingTemplateInstantiation(node);
    SyntaxResetForNewDeclaration(syntax);
    if (node != NULL && node->op == AST_OP(vardecl)) {
      // Function instantiations are queued directly, avoiding a one-element
      // declaration vector and the generic declaration classifier.
      VectorAppend(&compiler->declaration_asts, node);
      CompileFunctionDefinitionNode(
          syntax, (VariableDeclarationASTNode*)node);
    } else {
      CompileDeclarationNode(syntax, node);
    }
  }
  compiler->pending_template_instantiation_drain_depth--;
  if (compiler->pending_template_instantiation_drain_depth == 0) {
    VectorClear(pending);
    compiler->pending_template_instantiation_head = 0;
  }
}

static size_t DeclarationRootLength(ASTNode* root) {
  if (root == NULL) {
    return 0;
  }
  if (root->op == AST_OP(vardecl)) {
    return 1;
  }
  if (root->op != AST_OP(decl_list)) {
    return 0;
  }
  DeclarationListASTNode* declarations = (DeclarationListASTNode*)root;
  return declarations->declarations != NULL
             ? declarations->declarations->length
             : 0;
}

static VariableDeclarationASTNode* DeclarationRootAt(ASTNode* root,
                                                      size_t index) {
  if (root->op == AST_OP(vardecl)) {
    return index == 0 ? (VariableDeclarationASTNode*)root : NULL;
  }
  DeclarationListASTNode* declarations = (DeclarationListASTNode*)root;
  ASTNode* declaration = declarations->declarations->value.p[index];
  return declaration != NULL && declaration->op == AST_OP(vardecl)
             ? (VariableDeclarationASTNode*)declaration
             : NULL;
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
        ASTNode* root = compiler->declaration_asts.value.p[i];
        size_t num_declarations = DeclarationRootLength(root);
        for (size_t j = 0; j < num_declarations; j++) {
          VariableDeclarationASTNode* decl = DeclarationRootAt(root, j);
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

    CompilePendingTemplateInstantiations(syntax);
    if (NumErrors() != 0) {
      return;
    }

    emitted = false;
    size_t num_roots = compiler->declaration_asts.length;
    for (size_t i = 0; i < num_roots; i++) {
      ASTNode* root = compiler->declaration_asts.value.p[i];
      size_t num_declarations = DeclarationRootLength(root);
      for (size_t j = 0; j < num_declarations; j++) {
        VariableDeclarationASTNode* decl = DeclarationRootAt(root, j);
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

    Vector* declarations = NewVector();
    VectorAppend(declarations, decl);
    SyntaxResetForNewDeclaration(syntax);
    CompileDeclarationNode(
        syntax, NewDeclarationListASTNode(declarations, decl->base.location));
    compiler->current_class_access_context = saved_access;
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
  CompilerStringIndexInsert(
      compiler->emitted_function_name_index, name, sym);
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
  int errors_before = NumErrors();
  Token token_before = syntax->lex->current_token;
  SourceLocation location_before = syntax->lex->current_token_location;
  ASTNode* node = SyntaxParseExternalDeclaration(syntax);
  DiagnosticSwapState(diag_state);
  SyntaxEnsureProgress(syntax, token_before, location_before, errors_before,
                       0);
  CompileDeclarationNode(syntax, node);
  CompilePendingTemplateInstantiations(syntax);
  CompilerDrainPendingInjectedDeclarations(syntax);
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
                 "extern \"C\" int __cxa_guard_acquire(unsigned long long*);\n"
                 "extern \"C\" void __cxa_guard_release(unsigned long long*);\n"
                 "extern \"C\" void __cxa_guard_abort(unsigned long long*);\n"
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

typedef struct CompilerStringIndexEntry {
  char* key;
  void* value;
  uint64_t hash;
  struct CompilerStringIndexEntry* next;
} CompilerStringIndexEntry;

struct CompilerStringIndex {
  size_t bucket_count;
  size_t entry_count;
  CompilerStringIndexEntry** buckets;
};

#define COMPILER_STRING_INDEX_INITIAL_BUCKETS 16

static uint64_t HashCompilerStringIndexKey(const char* key) {
  uint64_t hash = UINT64_C(1469598103934665603);
  const unsigned char* current = (const unsigned char*)key;
  while (*current != '\0') {
    hash ^= *current++;
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

static struct CompilerStringIndex* NewCompilerStringIndex(void) {
  struct CompilerStringIndex* index =
      malloc(sizeof(*index));
  index->bucket_count = COMPILER_STRING_INDEX_INITIAL_BUCKETS;
  index->entry_count = 0;
  index->buckets = calloc(index->bucket_count, sizeof(*index->buckets));
  return index;
}

static void CompilerStringIndexDelete(struct CompilerStringIndex* index) {
  if (index == NULL) {
    return;
  }
  for (size_t i = 0; i < index->bucket_count; i++) {
    CompilerStringIndexEntry* entry = index->buckets[i];
    while (entry != NULL) {
      CompilerStringIndexEntry* next = entry->next;
      free(entry->key);
      free(entry);
      entry = next;
    }
  }
  free(index->buckets);
  free(index);
}

static CompilerStringIndexEntry* CompilerStringIndexFindEntry(
    struct CompilerStringIndex* index, const char* key) {
  uint64_t hash = HashCompilerStringIndexKey(key);
  size_t bucket = hash % index->bucket_count;
  for (CompilerStringIndexEntry* entry = index->buckets[bucket];
       entry != NULL; entry = entry->next) {
    if (entry->hash == hash && strcmp(entry->key, key) == 0) {
      return entry;
    }
  }
  return NULL;
}

static void* CompilerStringIndexFind(struct CompilerStringIndex* index,
                                     const char* key) {
  if (index == NULL || key == NULL || *key == '\0') {
    return NULL;
  }
  CompilerStringIndexEntry* entry = CompilerStringIndexFindEntry(index, key);
  return entry != NULL ? entry->value : NULL;
}

static bool CompilerStringIndexContains(struct CompilerStringIndex* index,
                                        const char* key) {
  if (index == NULL || key == NULL || *key == '\0') {
    return false;
  }
  return CompilerStringIndexFindEntry(index, key) != NULL;
}

static void ResizeCompilerStringIndex(struct CompilerStringIndex* index) {
  size_t new_bucket_count = index->bucket_count * 2;
  CompilerStringIndexEntry** buckets =
      calloc(new_bucket_count, sizeof(*buckets));
  for (size_t i = 0; i < index->bucket_count; i++) {
    CompilerStringIndexEntry* entry = index->buckets[i];
    while (entry != NULL) {
      CompilerStringIndexEntry* next = entry->next;
      size_t bucket = entry->hash % new_bucket_count;
      entry->next = buckets[bucket];
      buckets[bucket] = entry;
      entry = next;
    }
  }
  free(index->buckets);
  index->bucket_count = new_bucket_count;
  index->buckets = buckets;
}

static bool CompilerStringIndexInsert(struct CompilerStringIndex* index,
                                      const char* key, void* value) {
  if (index == NULL || key == NULL || *key == '\0') {
    return false;
  }
  uint64_t hash = HashCompilerStringIndexKey(key);
  size_t bucket = hash % index->bucket_count;
  for (CompilerStringIndexEntry* entry = index->buckets[bucket];
       entry != NULL; entry = entry->next) {
    if (entry->hash == hash && strcmp(entry->key, key) == 0) {
      return false;
    }
  }
  if ((index->entry_count + 1) * 4 > index->bucket_count * 3) {
    ResizeCompilerStringIndex(index);
    bucket = hash % index->bucket_count;
  }
  CompilerStringIndexEntry* entry = malloc(sizeof(*entry));
  entry->key = strdup(key);
  entry->value = value;
  entry->hash = hash;
  entry->next = index->buckets[bucket];
  index->buckets[bucket] = entry;
  index->entry_count++;
  return true;
}

static bool PendingSymbolIsDefinition(Symbol* symbol, const char* asm_name) {
  if (symbol == NULL || symbol->type == NULL || asm_name == NULL ||
      symbol->asm_name.value == NULL ||
      strcmp(symbol->asm_name.value, asm_name) != 0) {
    return false;
  }
  return !TypeIsFunction(symbol->type) ||
         (!symbol->flags.is_template &&
          symbol->type->info.function.body != NULL);
}

static bool IndexPendingTemplateInstantiationSymbol(Symbol* symbol) {
  const char* asm_name = symbol != NULL ? symbol->asm_name.value : NULL;
  if (asm_name == NULL || *asm_name == '\0' ||
      !PendingSymbolIsDefinition(symbol, asm_name)) {
    return false;
  }
  CompilerStringIndexInsert(
      compiler->pending_template_instantiation_names, asm_name, symbol);
  return true;
}

static bool IndexPendingTemplateInstantiation(ASTNode* declaration) {
  size_t num_declarations = DeclarationRootLength(declaration);
  if (num_declarations == 0) {
    return true;
  }
  bool fully_indexed = true;
  for (size_t i = 0; i < num_declarations; i++) {
    VariableDeclarationASTNode* decl =
        DeclarationRootAt(declaration, i);
    Symbol* symbol = decl != NULL ? decl->symbol : NULL;
    if (symbol != NULL &&
        !IndexPendingTemplateInstantiationSymbol(symbol)) {
      fully_indexed = false;
    }
  }
  return fully_indexed;
}

void CompilerQueuePendingTemplateInstantiation(ASTNode* declaration) {
  if (compiler == NULL || declaration == NULL) {
    return;
  }
  VectorAppend(&compiler->pending_template_instantiations, declaration);
  if (!IndexPendingTemplateInstantiation(declaration)) {
    VectorAppend(&compiler->unindexed_pending_template_instantiations,
                 declaration);
  }
}

void CompilerQueuePendingFunctionDefinition(Symbol* symbol) {
  if (compiler == NULL || symbol == NULL || symbol->flags.is_template ||
      !IsFunctionOrInlineDefinition(symbol)) {
    return;
  }
  CompilerQueuePendingTemplateInstantiation(
      NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
}

bool CompilerPendingTemplateInstantiationHasAsmName(const char* asm_name) {
  if (compiler == NULL || asm_name == NULL || *asm_name == '\0') {
    return false;
  }
  Symbol* indexed =
      CompilerStringIndexFind(
          compiler->pending_template_instantiation_names, asm_name);
  if (PendingSymbolIsDefinition(indexed, asm_name)) {
    return true;
  }
  // A symbol can receive its final mangled name or definition body after it is
  // queued. Preserve that behavior by revisiting only declarations that could
  // not be fully indexed at queue time.
  Vector* unindexed =
      &compiler->unindexed_pending_template_instantiations;
  for (size_t i = 0; i < unindexed->length;) {
    ASTNode* declaration = unindexed->value.p[i];
    if (!IndexPendingTemplateInstantiation(declaration)) {
      i++;
    } else {
      VectorDeleteElement(unindexed, i);
    }
    indexed = CompilerStringIndexFind(
        compiler->pending_template_instantiation_names, asm_name);
    if (PendingSymbolIsDefinition(indexed, asm_name)) {
      return true;
    }
  }
  return false;
}

static void InitBasic(Compiler* compiler, const char* filename) {
  // The input filename "-" means standard input; use a plain base name for
  // derived output files (e.g. "stdin.s"/"stdin.o") so they are not mistaken
  // for command-line options (a leading '-') by later tools like the linker.
  StringInit(&compiler->infile, strcmp(filename, "-") == 0 ? "stdin" : filename);
  CompilerTargetTripleInit(&compiler->target_triple);
  ModuleUnitInfoInit(&compiler->module_unit);
  VectorInit(&compiler->functions);
  VectorInit(&compiler->emitted_function_asm_names);
  compiler->emitted_function_name_index = NewCompilerStringIndex();
  VectorInit(&compiler->referenced_function_asm_names);
  compiler->referenced_function_name_index = NewCompilerStringIndex();
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
  VectorInit(&compiler->reflection_values);
  VectorInit(&compiler->meta_promoted_statics);
  VectorInit(&compiler->literals);
  VectorInit(&compiler->declaration_asts);
  VectorInit(&compiler->cxx_defined_classes);
  VectorInit(&compiler->functions_being_analyzed);
  VectorInit(&cxx_init_array_functions);
  VectorInit(&cxx_fini_array_functions);
  VectorInit(&compiler->pending_template_instantiations);
  compiler->pending_template_instantiation_head = 0;
  compiler->pending_template_instantiation_drain_depth = 0;
  compiler->pending_template_instantiation_names =
      NewCompilerStringIndex();
  VectorInit(&compiler->unindexed_pending_template_instantiations);
  VectorInit(&compiler->injection_frames);
  VectorInit(&compiler->pending_injected_declarations);
  VectorInit(&compiler->orphan_function_symbols);
  SetInit(&compiler->disabled_warnings, CompareWarning);
  SetInit(&compiler->error_warnings, CompareWarning);
  SetInit(&compiler->no_error_warnings, CompareWarning);
  VectorInit(&compiler->diagnostic_stack);
  compiler->diagnostic_suppression_depth = 0;
  compiler->diagnostic_error_trap_depth = 0;
  compiler->diagnostic_error_trapped = false;
  compiler->contract_assertion_depth = 0;
  compiler->contract_assertion_location = 0;
  compiler->pack_alignment = 0;
  VectorInit(&compiler->pack_stack);
  compiler->num_errors = 0;
  compiler->syntax_only = false;
  compiler->lto = false;
  compiler->lto_ir_only = false;
  compiler->lto_module = NULL;
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
  compiler->target_supports_threads = false;
  compiler->target_supports_atomics = false;
  compiler->target_supports_c11_atomics = false;
  compiler->target_supports_8_byte_atomics = false;
  
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
  } else if (StringEqual(value, "c23") || StringEqual(value, "c2x") ||
             StringEqual(value, "iso9899:2024") || StringEqual(value, "gnu23") ||
             StringEqual(value, "gnu2x")) {
    compiler->language_standard = kLanguageStandardC23;
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
  } else if (StringEqual(value, "c++26") || StringEqual(value, "c++2c") ||
             StringEqual(value, "gnu++26") || StringEqual(value, "gnu++2c")) {
    compiler->language_standard = kLanguageStandardCXX26;
  } else if (StringEqual(value, "c++29") || StringEqual(value, "c++2d") ||
             StringEqual(value, "gnu++29") || StringEqual(value, "gnu++2d")) {
    compiler->language_standard = kLanguageStandardCXX29;
  } else {
    fprintf(stderr, "Invalid language standard -std=%s\n", value->value);
    exit(1);
  }
}

static void InitBasicOptionsOrDie(Compiler* compiler,
                                  Vector* options, Vector* target_opts) {
  int max_errors = OptionIntValue(kOptionErrorLimit, options, 20);
  // 0 means unlimited, matching Clang's -ferror-limit=0.
  compiler->max_errors = max_errors <= 0 ? INT_MAX : max_errors;
  compiler->syntax_only = OptionBoolValue(kOptionSyntaxOnly, options, false);
  compiler->lto = OptionBoolValue(kOptionLTO, options, false);
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
  char target_error[256];
  if (!CompilerTargetTripleParse(&compiler->target_triple,
                                 compiler->target_name->value, target_error,
                                 sizeof(target_error))) {
    fprintf(stderr, "Invalid -target: %s\n", target_error);
    exit(1);
  }
  compiler->target_name = &compiler->target_triple.architecture;
  struct CompilerTargetDefinition* target = FindTarget(compiler->target_name);
  if (target == NULL) {
    fprintf(stderr, "Unknown -target architecture %s\n", compiler->target_name->value);
    exit(1);
  }
  compiler->target = target->factory();
  StringSet(compiler->target_name, target->canonical_name);
  compiler->target_supports_threads =
      (target->capabilities & kTargetSupportsThreads) != 0;
  compiler->target_supports_atomics =
      (target->capabilities & kTargetSupportsAtomics) != 0;
  compiler->target_supports_c11_atomics =
      (target->capabilities & kTargetSupportsC11Atomics) != 0;
  compiler->target_supports_8_byte_atomics =
      (target->capabilities & kTargetSupports8ByteAtomics) != 0;

  compiler->debug_output = OptionBoolValue(kOptionDebug, options, false);
  ParseStandardOption(compiler, options);
  compiler->constexpr_eval_mode = kConstexprEvalAuto;
  String* constexpr_eval = OptionStringValue(kOptionConstexprEval, options);
  if (constexpr_eval != NULL) {
    if (StringEqual(constexpr_eval, "auto")) {
      compiler->constexpr_eval_mode = kConstexprEvalAuto;
    } else if (StringEqual(constexpr_eval, "pcode")) {
      compiler->constexpr_eval_mode = kConstexprEvalPCode;
    } else if (StringEqual(constexpr_eval, "ast")) {
      compiler->constexpr_eval_mode = kConstexprEvalAST;
    } else if (StringEqual(constexpr_eval, "audit")) {
      compiler->constexpr_eval_mode = kConstexprEvalAudit;
    } else {
      fprintf(stderr, "Invalid constexpr evaluator: %s\n",
              constexpr_eval->value);
      exit(1);
    }
  }
  compiler->contract_semantic = kContractSemanticEnforce;
  String* contracts = OptionStringValue(kOptionContracts, options);
  if (contracts != NULL) {
    if (StringEqual(contracts, "ignore")) {
      compiler->contract_semantic = kContractSemanticIgnore;
    } else if (StringEqual(contracts, "observe")) {
      compiler->contract_semantic = kContractSemanticObserve;
    } else if (StringEqual(contracts, "enforce")) {
      compiler->contract_semantic = kContractSemanticEnforce;
    } else if (StringEqual(contracts, "quick-enforce")) {
      compiler->contract_semantic = kContractSemanticQuickEnforce;
    } else {
      fprintf(stderr, "Invalid contract semantic: %s\n", contracts->value);
      exit(1);
    }
  }
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

static void ApplyPreprocessorCommandLineOptions(Compiler* compiler,
                                                Vector* options) {
  if (options == NULL) {
    return;
  }
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
      default:
        break;
    }
  }
}

// Process macro definition and include path options and process warning
// options
static void InitComplexOptions(Compiler* compiler, Vector* options) {
  ApplyPreprocessorCommandLineOptions(compiler, options);
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* option_value = options->value.p[i];
    switch (option_value->opt) {
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
  for (size_t i = compiler->pending_template_instantiation_head;
       i < compiler->pending_template_instantiations.length; i++) {
    ASTNodeDelete((ASTNode*)compiler->pending_template_instantiations.value.p[i]);
  }
  VectorDestruct(&compiler->pending_template_instantiations);
  CompilerStringIndexDelete(
      compiler->pending_template_instantiation_names);
  compiler->pending_template_instantiation_names = NULL;
  VectorDestruct(
      &compiler->unindexed_pending_template_instantiations);
  CompilerInjectionFramesTeardown();

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
  if (compiler->lto_module != NULL) {
    LTOModuleDelete(compiler->lto_module);
    compiler->lto_module = NULL;
  }
  VectorDestructWithContents(
      &compiler->emitted_function_asm_names,
      (VectorElementDestructor)StringDelete, /*free_element=*/false);
  CompilerStringIndexDelete(compiler->emitted_function_name_index);
  compiler->emitted_function_name_index = NULL;
  VectorDestructWithContents(
      &compiler->referenced_function_asm_names,
      (VectorElementDestructor)StringDelete, /*free_element=*/false);
  CompilerStringIndexDelete(compiler->referenced_function_name_index);
  compiler->referenced_function_name_index = NULL;
  VectorDestructWithContents(
      &compiler->referenced_variable_asm_names,
      (VectorElementDestructor)StringDelete, /*free_element=*/false);

  VectorDestructWithContents(
      &compiler->reflection_values,
      (VectorElementDestructor)ReflectionValueDelete,
      /*free_element=*/false);
  for (size_t i = 0; i < compiler->meta_promoted_statics.length; i++) {
    MetaPromotedStaticEntry* entry = compiler->meta_promoted_statics.value.p[i];
    if (entry != NULL) {
      StringDestruct(&entry->key);
      free(entry);
    }
  }
  VectorDestruct(&compiler->meta_promoted_statics);

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
  CompilerTargetTripleDestruct(&compiler->target_triple);
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

  // Template arguments can own TypeRecord references, so release their recycled
  // shells only after all argument payloads have been destructed and before the
  // type arena itself is reclaimed.
  TemplateArgumentArenaRelease();

  // Everything that references TypeRecords has now been torn down, so the
  // type arena's struct memory can be reclaimed in one shot.
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

bool CompilerInitForLTOLink(Compiler* compiler, const char* filename,
                            Vector* options, Vector* target_opts) {
  if (!CompilerInitCommon(compiler, filename, options, target_opts)) {
    return false;
  }
  String* code_string = NewString("/* lto */\n");
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
static bool EmitCompilerAssemblyFile(Compiler* compiler, String* asm_filename,
                                     String* generated_output) {
  char* generated_buffer = NULL;
  size_t generated_size = 0;
  FILE* asm_file;
  if (generated_output != NULL) {
    asm_file = open_memstream(&generated_buffer, &generated_size);
    if (asm_file != NULL) {
      compiler->target->emit_assembly_preamble(&compiler->infile, asm_file);
    }
  } else {
    asm_file =
        compiler->target->create_asm_file(&compiler->infile, asm_filename);
  }

  if (asm_file == NULL) {
    fprintf(stderr, "Unable to open assembler file %s\n",
            asm_filename->value);
    return false;
  }

  if (!EmitTranslationUnitContents(compiler, asm_file)) {
    if (asm_file != stdout) {
      fclose(asm_file);
    }
    return false;
  }

  if (asm_file != stdout) {
    fclose(asm_file);
  }
  if (generated_output != NULL) {
    StringAppendSegment(generated_output, generated_buffer, generated_size);
    free(generated_buffer);
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

static String* AssembleGeneratedString(Compiler* compiler, String* input,
                                       Vector* options) {
  String* output_filename = OptionStringValue(kOptionOutputFile, options);
  String* object_filename;
  if (output_filename != NULL) {
    object_filename = NewString(output_filename->value);
  } else {
    object_filename = NewString(compiler->infile.value);
    ReplaceSourceExtension(object_filename, ".o");
  }

  if (!compiler->target->assemble_string(compiler->infile.value, input,
                                         object_filename)) {
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
      if (sym->flags.used || sym->flags.is_name_independent ||
          SymbolHasAttribute(sym, "unused")) {
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

static void ParseCurrentTranslationUnit(Compiler* compiler) {
  LexNextToken(&compiler->lex);
  while (!LexEof(&compiler->lex)) {
    SyntaxResetForNewDeclaration(&compiler->syntax);
    CompileDeclaration(&compiler->syntax);
  }
}

bool CompileFrontEndOnly(Compiler* compiler) {
  // Runs preprocessing, parsing and semantic analysis for the current
  // translation unit without code generation.  Used by -Xemit-module.
  CreateGlobalSymbolTables();
  DeclarePredefinedTypesAndMacros(&compiler->preprocessor);

  ParseCurrentTranslationUnit(compiler);
  CheckUnusedStaticFunctions();
  CheckUnusedGlobalVariables();
  CheckUnusedPrivateFields();
  return NumErrors() == 0;
}

static String* CompileAfterParse(Compiler* compiler, Vector* options) {
  CheckUnusedStaticFunctions();
  CheckUnusedGlobalVariables();
  CheckUnusedPrivateFields();

  if (compiler->print_front_end) {
    HashTablePrintStats(&compiler->global_symbol_table, compiler->ast_output_file);
    HashTablePrintStats(&compiler->global_tag_table, compiler->ast_output_file);
    PreprocessorPrintStats(&compiler->preprocessor, compiler->ast_output_file);
  }

  if (compiler->syntax_only) {
    return NumErrors() == 0 ? NewString("") : NULL;
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
  if (compiler->lto_ir_only) {
    if (compiler->lto_module == NULL) {
      compiler->lto_module = LTOModuleCreate();
    }
    LTOModuleCaptureCompilerState(compiler->lto_module, compiler);
    char tu_id[17];
    LTOMakeTUId(compiler->infile.value, tu_id);
    LTOModuleRenameInternalSymbols(compiler->lto_module, tu_id);
    return NumErrors() == 0 ? NewString("") : NULL;
  }
  return CompilerEmitTranslationUnit(compiler, options);
}

String* CompilerEmitTranslationUnit(Compiler* compiler, Vector* options) {
  bool output_asm_only = OptionBoolValue(kOptionAssemblyOutput, options, false);
  if (compiler->target->emit_program_file != NULL) {
    return compiler->target->emit_program_file(compiler, options,
                                                output_asm_only);
  }
  if (!output_asm_only && !compiler->keep_asm_file &&
      compiler->target->emit_object_file != NULL) {
    return compiler->target->emit_object_file(compiler, options);
  }

  String asm_filename = {0};
  String* output_filename = OptionStringValue(kOptionOutputFile, options);
  if (output_asm_only && output_filename != NULL) {
    StringInit(&asm_filename, output_filename->value);
  } else {
    StringInit(&asm_filename, compiler->infile.value);
    ReplaceSourceExtension(&asm_filename, ".s");
  }
  bool use_in_memory_assembly =
      !output_asm_only && !compiler->keep_asm_file &&
      compiler->target->emit_assembly_preamble != NULL &&
      compiler->target->assemble_string != NULL;
  String generated_assembly = {0};
  bool ok = EmitCompilerAssemblyFile(
      compiler, &asm_filename,
      use_in_memory_assembly ? &generated_assembly : NULL);
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
  String* object_filename =
      use_in_memory_assembly
          ? AssembleGeneratedString(compiler, &generated_assembly, options)
          : Assemble(compiler, &asm_filename, options);
  StringDestruct(&generated_assembly);
  
  // Remove .s file unless told not to.
  if (!compiler->keep_asm_file) {
    remove(asm_filename.value);
  }
  
  StringDestruct(&asm_filename);
  return object_filename;
}

static String* Compile(Compiler* compiler, Vector* options) {
  CreateGlobalSymbolTables();
  DeclarePredefinedTypesAndMacros(&compiler->preprocessor);
  ParseCurrentTranslationUnit(compiler);
  return CompileAfterParse(compiler, options);
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

bool CompileTranslationUnitToLTOIR(const char* filename, Vector* options,
                                   Vector* target_opts) {
  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromFile(compiler, filename, options, target_opts)) {
    fprintf(stderr, "Cannot open file %s\n", filename);
    free(compiler);
    compiler = NULL;
    ClearAllFiles();
    return false;
  }
  compiler->lto = true;
  compiler->lto_ir_only = true;
  String* result = Compile(compiler, options);
  bool ok = result != NULL && NumErrors() == 0;
  if (result != NULL) {
    StringDelete(result);
  }
  if (!ok) {
    CompilerDelete(compiler);
    compiler = NULL;
    ClearAllFiles();
    return false;
  }
  return true;
}

void CompilerPrepareForIRLoad(void) {
  CreateGlobalSymbolTables();
  DeclarePredefinedTypesAndMacros(&compiler->preprocessor);
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
