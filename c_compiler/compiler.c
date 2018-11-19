//
//  compiler.c
//  c_compiler
//
//  Created by David Allison on 11/1/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "compiler.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "lex.h"
#include "semantics.h"
#include "syntax.h"

#include "p_code_target.h"
#include "risc_v_target.h"

// This is global to avoid having to pass it around everywhere.
Compiler* compiler;

static CompilerOptionDefinition compiler_options[] = {
    {"-g", kCompilerOptionBool, kOptionDebug, false},
    {"-O", kCompilerOptionBool, kOptionOptimize, false},
    {"-target", kCompilerOptionString, kOptionTarget, false},
    {"-c", kCompilerOptionBool, kOptionCompileOnly, false},
    {"-S", kCompilerOptionBool, kOptionAssemblyOutput, false},
    {"-o", kCompilerOptionString, kOptionOutputFile, false},
    {"-I", kCompilerOptionString, kOptionIncludePath, true},
    {"-D", kCompilerOptionString, kOptionDefineMacro, true},
    {"-U", kCompilerOptionString, kOptionUndefineMacro, true},
    {"-fPIC", kCompilerOptionBool, kOptionPic, false},
    {"-fpic", kCompilerOptionBool, kOptionPic, false},
    {"-Werror", kCompilerOptionBool, kOptionWerror, false},
    {"-Wall", kCompilerOptionBool, kOptionWall, false},
    {"-W", kCompilerOptionString, kOptionWarning, true},
    {"-error-limit", kCompilerOptionInt, kOptionErrorLimit, false},
    {NULL, 0, 0, false},
};

void ParseOptions(int argc, char** argv, Vector* options) {
  for (int i = 1; i < argc; i++) {
    String s;
    StringInit(&s, argv[i]);
    if (s.value[0] == '-') {
      for (size_t opt = 0; compiler_options[opt].name != NULL; opt++) {
        if (compiler_options[opt].is_prefix &&
            s.value[1] == compiler_options[opt].name[1]) {
          CompilerOptionValue* o = calloc(sizeof(CompilerOptionValue), 1);
          o->opt = compiler_options[opt].opt;
          // Prefixed options are always a string.
          StringInit(&o->value.svalue, &argv[i][2]);
          VectorAppend(options, o);
        } else if (StringEqual(&s, compiler_options[opt].name)) {
          CompilerOptionValue* o = calloc(sizeof(CompilerOptionValue), 1);
          o->opt = compiler_options[opt].opt;
          switch (compiler_options[opt].type) {
            case kCompilerOptionString:
              i++;
              if (i < argc) {
                StringInit(&o->value.svalue, argv[i]);
              }
              break;
            case kCompilerOptionBool:
              o->value.bvalue = true;
              break;
            case kCompilerOptionInt:
              i++;
              if (i < argc) {
                o->value.ivalue = atoi(argv[i]);
              }
              break;
          }
          VectorAppend(options, o);
        }
      }
    } else {
      CompilerOptionValue* o = calloc(sizeof(CompilerOptionValue), 1);
      o->opt = kOptionInputFile;
      StringInit(&o->value.svalue, s.value);
      VectorAppend(options, o);
    }
  }
}

static String* OptionStringValue(CompilerOption option, Vector* options) {
  for (size_t i = options->length; i > 0; i--) {
    CompilerOptionValue* opt = options->value[i - 1];
    if (opt->opt == option) {
      return &opt->value.svalue;
    }
  }
  return NULL;
}

static int OptionIntValue(CompilerOption option, Vector* options, int def) {
  for (size_t i = options->length; i > 0; i--) {
    CompilerOptionValue* opt = options->value[i - 1];
    if (opt->opt == option) {
      return opt->value.ivalue;
    }
  }
  return def;
}

static bool OptionBoolValue(CompilerOption option, Vector* options, bool def) {
  for (size_t i = options->length; i > 0; i--) {
    CompilerOptionValue* opt = options->value[i - 1];
    if (opt->opt == option) {
      return opt->value.bvalue;
    }
  }
  return def;
}

static void InitScalar(ASTNode* expr, ASTNode* subinit, int offset,
                       Vector* initializers) {
  TypeRecordCalculateSize(expr->type);
  Initializer* init_out = malloc(sizeof(Initializer));
  TypeRecord* type = expr->type;
  if (TypeIsIntegral(type)) {
    int64_t value = 0;
    if (EvaluateIntegerExpression(expr, &value)) {
      if (TypeIsChar(type) || TypeIsBool(type)) {
        init_out->type = kInitTypeByte;
        init_out->value.byte = (uint8_t)value;
      } else if (TypeIsShort(type)) {
        init_out->type = kInitTypeHalf;
        init_out->value.half = (uint16_t)value;
      } else if (TypeIsInt(type)) {
        init_out->type = kInitTypeWord;
        init_out->value.word = (uint32_t)value;
      } else if (TypeIsLong(type) || TypeIsLongLong(type)) {
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
  } else if (TypeIsFloatingPoint(type)) {
    double value = 0;
    if (EvaluateFloatingPointExpression(expr, &value)) {
      if (TypeIsFloat(type)) {
        init_out->type = kInitTypeWord;
        float fvalue = (float)value;
        init_out->value.word = *((int32_t*)&fvalue);
      } else if (TypeIsDouble(type) || TypeIsLongDouble(type)) {
        init_out->type = kInitTypeLong;
        init_out->value._long = *((int32_t*)&value);
      } else {
        assert(false);
      }
      init_out->offset = offset;
      VectorAppend(initializers, init_out);
    } else {
      SemanticError(
          expr, "Invalid static initialization; need a constant expression");
    }
  } else if (TypeIsArray(type)) {
    if (expr->op == AST_OP(identifier)) {
      // Initialization with an exising identifier.
      IdentifierASTNode* id_node = (IdentifierASTNode*)expr;
      init_out->type = kInitTypeSymbol;
      init_out->value.symbol = id_node->symbol;
      init_out->offset = offset;
      VectorAppend(initializers, init_out);
    } else if (expr->op == AST_OP(string)) {
      // String literal.
      ConstantASTNode* string_node = (ConstantASTNode*)expr;
      int literal_id = CompilerAddStringLiteral(string_node->value.string);
      init_out->type = kInitTypeString;
      init_out->value.literal_id = literal_id;
      init_out->offset = offset;
      VectorAppend(initializers, init_out);
    } else {
      SemanticError(subinit, "Invalid static initialization");
    }
  } else if (TypeIsPointer(type)) {
    // Pointers can be initialize to the address of an existing static
    // variable of the same type.
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
    } else {
      SemanticError(var_node,
                    "Illegal static initializer: need address of variable");
    }
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
  } else {
    SemanticError(subinit, "Invalid static initialization");
  }
}

// Expand a braced initializer into the vector given.
// The initializer has been simplified into a braced initializer
// containing only designated initializers.
static void ExpandBracedInitializer(BracedInitializerASTNode* init,
                                    int dest_offset, Vector* initializers) {
  for (size_t i = 0; i < init->initializers->length; i++) {
    ASTNode* subinit = (ASTNode*)init->initializers->value[i];
    assert(subinit->op == AST_OP(designated_init));

    DesignatedInitializerASTNode* designated_init =
        (DesignatedInitializerASTNode*)subinit;
    int offset = dest_offset;
    if (designated_init->designators != NULL) {
      // We have designators.  Need to build up an offset from the
      // designators.
      for (size_t i = 0; i < designated_init->designators->length; i++) {
        Designator* d = (Designator*)designated_init->designators->value[i];
        switch (d->designator_type) {
          case kDesignatorArray:
            // Array index.  Add index * size of lower dimensions to offset.
            offset += d->value.array_index * d->type->next->size;
            break;
          case kDesignatorStruct:
            offset += d->value.struct_member->byte_offset;
            break;
        }
      }
    }
    switch (designated_init->init->op) {
      // Simple scalar initialization
      case AST_OP(expr_init): {
        ExpressionInitializerASTNode* expr_init =
            (ExpressionInitializerASTNode*)designated_init->init;
        InitScalar(expr_init->expr, subinit, offset, initializers);
        break;
      }
      case AST_OP(braced_init): {
        // TODO: braced init for struct or array.
        BracedInitializerASTNode* braced_init =
            (BracedInitializerASTNode*)designated_init->init;
        ExpandBracedInitializer(braced_init, offset, initializers);
        break;
      }
      default:
        assert(false);
    }
  }
}

static void AddInitializedStaticVariable(VariableDeclarationASTNode* decl,
                                         ASTNode* init) {
  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  StringInit(&var->name, decl->symbol->name.value);
  var->is_global = decl->symbol->storage != kStorageStatic;
  VectorInit(&var->initializers);
  var->size = decl->symbol->type->size;
  var->alignment = TypeRecordAlignment(decl->symbol->type);
  ExpandBracedInitializer((BracedInitializerASTNode*)init, 0,
                          &var->initializers);
  VectorAppend(&compiler->initialized_static_variables, var);
}

void InitializerDelete(Initializer* init) { free(init); }

void InitializedStaticVariableDelete(InitializedStaticVariable* var) {
  StringDestruct(&var->name);
  for (size_t i = 0; i < var->initializers.length; i++) {
    InitializerDelete(var->initializers.value[i]);
  }
  VectorDestruct(&var->initializers);
  free(var);
}

void UninitializedStaticVariableDelete(InitializedStaticVariable* var) {
  StringDestruct(&var->name);
  free(var);
}

int CompilerAddStringLiteral(String* value) {
  StringLiteral* literal = malloc(sizeof(StringLiteral));
  StringInit(&literal->value, value->value);
  literal->id = compiler->next_literal_id++;
  VectorAppend(&compiler->string_literals, literal);
  literal->disabled = false;
  return literal->id;
}

StringLiteral* CompilerFindStringLiteral(int literal_id) {
  for (size_t i = 0; i < compiler->string_literals.length; i++) {
    StringLiteral* literal = compiler->string_literals.value[i];
    if (literal->id == literal_id) {
      return literal;
    }
  }
  return NULL;
}

void StringLiteralDelete(StringLiteral* literal) {
  StringDestruct(&literal->value);
  free(literal);
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
            (VariableDeclarationASTNode*)decls->declarations->value[i];

        if (TypeIsFunctionDefinition(decl->base.type)) {
          compiler->current_function = decl->base.type;
          SemanticAnalyzeFunction(syntax, (ASTNode*)decl);
          SymbolPrintDetails(decl->symbol, true);

          if (NumErrors() == 0) {
            // Generate code for function.
            Generator codegen;
            GeneratorInit(&codegen, syntax, compiler->current_function);

            // Generate code for function with given target.
            void* code = GenerateFunction(&codegen);

            VectorAppend(&compiler->functions, code);
          }
        } else {
          // Declaration is a variable or extern function.
          if (TypeIsFunction(decl->base.type)) {
            // Function declaration, not a definition.
          } else {
            if (decl->symbol->storage == kStorageTypedef) {
              continue;
            }
            // Declaration is a variable.
            if (decl->symbol->storage == kStorageExtern &&
                decl->initializer == NULL) {
              // Extern variable declaration.
              if (TypeIsVoid(decl->symbol->type)) {
                SemanticError((ASTNode*)decl,
                              "Cannot declare a variable with void type");
              }
            } else {
              // Extern or static variable definition.
              if (decl->initializer == NULL) {
                // No initializer.  Add as unitialized static variable.
                UnintializedStaticVariable* var =
                    malloc(sizeof(UnintializedStaticVariable));
                StringInit(&var->name, decl->symbol->name.value);
                var->is_global = decl->symbol->storage != kStorageStatic;
                var->size = decl->symbol->type->size;
                var->alignment = TypeRecordAlignment(decl->symbol->type);
                VectorAppend(&compiler->uninitialized_static_variables, var);
              } else {
                ASTNode* simplified_init = AnalyzeInitializer(
                    syntax, decl->base.type, decl->initializer);
                // This is an initialized static variable.  The initializer has
                // been simplified to a braced initializer containing only
                // designated initializers.
                ASTNodePrint(simplified_init, 0);
                AddInitializedStaticVariable(decl, simplified_init);
              }
            }
          }
        }
      }
    }
  }
}

static void DeclarePredefinedTypes(Preprocessor* preprocessor) {
  String* code = NewString(
      "typedef void* __builtin_va_list;\n"
      "#define __asm asm\n"
      "#define __asm__ asm\n"
      "#define __attribute __attribute__\n"
      "#define __inline inline\n"
      "#define __signed signed\n"
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
}

static int CompareWarning(const void* a, const void* b) {
  const char** s1 = (const char**)a;
  const char** s2 = (const char**)b;
  return strcmp(*s1, *s2);
}

static bool CompilerInitCommon(Compiler* compiler, const char* filename,
                               Vector* options) {
  StringInit(&compiler->infile, filename);
  VectorInit(&compiler->functions);
  VectorInit(&compiler->initialized_static_variables);
  VectorInit(&compiler->uninitialized_static_variables);
  VectorInit(&compiler->string_literals);
  SetInit(&compiler->disabled_warnings, CompareWarning);
  compiler->num_errors = 0;
  compiler->max_errors = OptionIntValue(kOptionErrorLimit, options, 20);
  compiler->convert_warnings_to_errors = OptionBoolValue(kOptionWerror, options, false);
  compiler->enable_all_warnings = OptionBoolValue(kOptionWall, options, false);
  compiler->next_literal_id = 1;
  compiler->current_include_path_index = 0;
  PreprocessorInit(&compiler->preprocessor);
  SyntaxInit(&compiler->syntax, &compiler->lex);

  compiler->target_name = OptionStringValue(kOptionTarget, options);
  if (compiler->target_name == NULL) {
    fprintf(stderr, "No target specified; please specify -target option\n");
    return false;
  }
  if (StringEqual(compiler->target_name, "pcode") ||
      StringEqual(compiler->target_name, "p-code")) {
    compiler->target = NewPCodeTarget();
  } else if (StringEqual(compiler->target_name, "riscv") ||
             StringEqual(compiler->target_name, "risc-v")) {
    compiler->target = NewRVTarget();
  } else {
    fprintf(stderr, "Unknown -target value %s\n", compiler->target_name->value);
    return false;
  }
  compiler->debug_output = OptionBoolValue(kOptionDebug, options, false);
  compiler->optimize = OptionBoolValue(kOptionOptimize, options, false);
  compiler->pic = OptionBoolValue(kOptionPic, options, false);
  
  compiler->pointer_size = compiler->target->pointer_size;
  
  PreprocessorDefineArchitectureMacros(&compiler->preprocessor);

  // Process macro definition and include path options and process warning
  // options
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* option_value = options->value[i];
    switch (option_value->opt) {
      case kOptionIncludePath:
        PreprocessorAddUserIncludePath(&compiler->preprocessor,
                                       option_value->value.svalue.value);
        break;
      case kOptionDefineMacro: {
        ssize_t equals = StringIndexOf(&option_value->value.svalue, "=");
        String macro_name;
        String macro_value;
        StringInit(&macro_name, "");
        StringInit(&macro_value, "");

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
  
  return true;
}

void CompilerDestruct(Compiler* compiler) {
  ClearSymbolTable(&compiler->global_symbol_table, true);
  ClearSymbolTable(&compiler->global_tag_table, true);

  StringDestruct(&compiler->infile);

  for (size_t i = 0; i < compiler->functions.length; i++) {
    compiler->target->cleanup(compiler->functions.value[i]);
  }
  VectorDestruct(&compiler->functions);

  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariableDelete(
        compiler->initialized_static_variables.value[i]);
  }
  VectorDestruct(&compiler->initialized_static_variables);

  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    UninitializedStaticVariableDelete(
        compiler->uninitialized_static_variables.value[i]);
  }
  VectorDestruct(&compiler->uninitialized_static_variables);

  for (size_t i = 0; i < compiler->string_literals.length; i++) {
    StringLiteralDelete(compiler->string_literals.value[i]);
  }
  VectorDestruct(&compiler->string_literals);

  PreprocessorDestruct(&compiler->preprocessor);
  SyntaxDestruct(&compiler->syntax);
  LexDestruct(&compiler->lex);
}

void CompilerDelete(Compiler* compiler) {
  CompilerDestruct(compiler);
  free(compiler);
}

bool CompilerInitFromFile(Compiler* compiler, const char* filename,
                          Vector* options) {
  bool ok = CompilerInitCommon(compiler, filename, options);
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
  bool ok = CompilerInitCommon(compiler, filename, options);
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
  bool ok = CompilerInitCommon(compiler, filename, options);
  if (!ok) {
    return false;
  }
  ok = LexInitFromFile(&compiler->lex, filename, &compiler->preprocessor);
  if (!ok) {
    return false;
  }
  return true;
}

// Compile a source file.
static void Compile(Compiler* compiler, Vector* options) {
  CreateGlobalSymbolTables();
  DeclarePredefinedTypes(&compiler->preprocessor);
  LexNextToken(&compiler->lex);
  while (!LexEof(&compiler->lex)) {
    CompileDeclaration(&compiler->syntax);
  }
  HashTablePrintStats(&compiler->global_symbol_table);
  HashTablePrintStats(&compiler->global_tag_table);
  PreprocessorPrintStats(&compiler->preprocessor);

  String asm_filename;
  StringInit(&asm_filename, compiler->infile.value);
  // If the file ends in ".c", make it ".s", otherwise append ".s".
  char* suffix = strstr(asm_filename.value, ".c");
  if (suffix == NULL) {
    StringAppend(&asm_filename, ".s");
  } else {
    // Overwrite 'c' with 's'.
    suffix[1] = 's';
  }

  FILE* asm_file =
      compiler->target->create_asm_file(&compiler->infile, &asm_filename);
  if (asm_file == NULL) {
    fprintf(stderr, "Unable to open assembler file %s\n", asm_filename.value);
    return;
  }
  for (size_t i = 0; i < compiler->functions.length; i++) {
    // Debug, print to stdout.
    compiler->target->emit_function_assembly(compiler->functions.value[i],
                                             stdout);

    compiler->target->emit_function_assembly(compiler->functions.value[i],
                                             asm_file);
  }

  // Now emit the data to the assembly file.
  compiler->target->emit_data_start(asm_file);

  // Initialized variables.
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    compiler->target->emit_static_variable(
        compiler->initialized_static_variables.value[i], asm_file);
  }

  // Uninitialized variables.
  for (size_t i = 0; i < compiler->uninitialized_static_variables.length; i++) {
    compiler->target->emit_bss_space(
        compiler->uninitialized_static_variables.value[i], asm_file);
  }

  // Emit string literals start.
  compiler->target->emit_literals_start(asm_file);

  // Now the string literals.
  for (size_t i = 0; i < compiler->string_literals.length; i++) {
    compiler->target->emit_string_literal(compiler->string_literals.value[i],
                                          asm_file);
  }

  if (compiler->debug_output) {
    compiler->target->emit_debug(asm_file);
  }

  fclose(asm_file);

  // If the user specified -S then we don't assemble the output
  // and we keep the output file.
  if (OptionBoolValue(kOptionAssemblyOutput, options, false)) {
    return;
  }

  String object_filename;
  String* output_filename = OptionStringValue(kOptionOutputFile, options);
  if (output_filename != NULL) {
    StringInit(&object_filename, output_filename->value);
  } else {
    StringInit(&object_filename, compiler->infile.value);

    // If the file ends in ".c", make it ".o", otherwise append ".o".
    suffix = strstr(object_filename.value, ".c");
    if (suffix == NULL) {
      StringAppend(&object_filename, ".o");
    } else {
      // Overwrite 'c' with 'o'.
      suffix[1] = 'o';
    }
  }

  bool ok = compiler->target->assemble(&asm_filename, &object_filename);
  if (!ok) {
    fprintf(stderr, "Failed to assemble\n");
  }

  // Remove .s file.
  // remove(asm_filename.value);
}

void CompileTranslationUnit(const char* filename, Vector* options) {
  ClearAllFiles();

  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromFile(compiler, filename, options)) {
    fprintf(stderr, "Cannot open file %s\n", filename);
    return;
  }

  Compile(compiler, options);
  CompilerDelete(compiler);
  compiler = NULL;
}

void CompileTranslationUnitFromString(const char* filename, const char* code,
                                      Vector* options) {
  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  CompilerInitFromString(compiler, filename, code, options);
  Compile(compiler, options);
  CompilerDelete(compiler);
  compiler = NULL;
}
