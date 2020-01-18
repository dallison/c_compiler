//
//  compiler.h
//  c_compiler
//
//  Created by David Allison on 11/1/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef compiler_h
#define compiler_h

#include <stdlib.h>
#include "assembler.h"
#include "hashtable.h"
#include "type.h"
#include "vector.h"
#include "set.h"
#include "buffer.h"

struct Generator;

typedef enum {
  kOptionInputFile,
  kOptionDebug,
  kOptionOptimize,
  kOptionTarget,
  kOptionCompileOnly,
  kOptionAssemblyOutput,
  kOptionOutputFile,
  kOptionIncludePath,
  kOptionDefineMacro,
  kOptionUndefineMacro,
  kOptionPic,
  kOptionWarning,
  kOptionWerror,
  kOptionWall,
  kOptionErrorLimit,
  kOptionTlsModel,
} CompilerOption;

typedef struct {
  CompilerOption opt;
  union {
    int ivalue;
    String svalue;
    bool bvalue;
  } value;
} CompilerOptionValue;

typedef enum {
  kCompilerOptionString,
  kCompilerOptionInt,
  kCompilerOptionBool,
} CompilerOptionType;

typedef struct {
  const char* name;  // -name ("" means positional arg)
  CompilerOptionType type;
  CompilerOption opt;
  bool is_prefix;
} CompilerOptionDefinition;

// Thread local storage model.
#define TLS(x) kTls_##x
typedef enum {
  TLS(bad),
  TLS(global_dynamic),
  TLS(local_dynamic),
  TLS(initial_exec),
  TLS(local_exec),
} TlsModel;

void ParseOptions(int argc, char** argv, Vector* options);

typedef enum {
  kInitTypeByte,
  kInitTypeHalf,
  kInitTypeWord,
  kInitTypeLong,
  kInitTypeSymbol,
  kInitTypeString,
  kInitTypeMemory,
} InitializerType;

typedef struct {
  InitializerType type;
  int32_t offset;
  int32_t length;
  union {
    uint8_t byte;
    uint16_t half;
    uint32_t word;
    uint64_t _long;
    Symbol* symbol;
    int literal_id;
    Buffer memory;
  } value;
} Initializer;

void InitializerDelete(Initializer* init);

// An initialized static variable.  The values of these are set at compile
// time from the initializer supplied by the user.
typedef struct {
  String name;     // Variable name
  bool is_global;  // Variable is global (can be seen outside of file).
  size_t size;
  int32_t alignment;
  Vector initializers;
  bool is_tls;
} InitializedStaticVariable;

void InitializedStaticVariableDelete(InitializedStaticVariable* var);

// An uninitiaized static variable.  These have just a name and a size.
typedef struct {
  String name;
  bool is_global;  // Variable is global (can be seen outside of file).
  size_t size;
  size_t alignment;
  bool is_tls;
} UnintializedStaticVariable;

// A string literal has an id and a value.
typedef struct {
  int id;
  String value;
  bool disabled;
} StringLiteral;

int CompilerAddStringLiteral(String* value);
void StringLiteralDelete(StringLiteral* literal);

void UninitializedStaticVariableDelete(InitializedStaticVariable* var);

TlsModel ParseTlsModelName(String* name);

// A compiler target back-end.  This contains pointers to
// functions to generate code for a particlar target.
typedef struct {
  String name;  // Name of target.
  int pointer_size;   // Size of pointer.
  
  // Function to generate code.  Returns target specific data.
  void* (*codegen)(struct Generator*);

  // Function to create the assembler file.
  FILE* (*create_asm_file)(String* src_file, String* asm_file);

  // Assembly language emitter.  The 'code' parameter is the return value from
  // the 'codegen' functions.
  // The 'asm_file' parameter is an open file to write to.
  void (*emit_function_assembly)(void* code, FILE* asm_file);

  // Assemble the 'asm_filename' into the 'object_filename'.
  bool (*assemble)(String* asm_filename, String* object_filename);

  // Emit start of data section to asm file.
  void (*emit_data_start)(FILE* asm_file);

  // Emit static data to the assembly file
  void (*emit_static_variable)(InitializedStaticVariable* var, FILE* asm_file);

  // Emit BSS (uninitialized variable) to the assembly file.
  // NOTE: BSS is an old term meaning Block Started by Symbol.
  void (*emit_bss_space)(UnintializedStaticVariable* var, FILE* asm_file);

  // Emit start of tdata section to asm file.
  void (*emit_tdata_start)(FILE* asm_file);
  
  // Emit start of tbss section to asm file.
  void (*emit_tbss_start)(FILE* asm_file);

  // Emit tls data to the assembly file
  void (*emit_tls_variable)(InitializedStaticVariable* var, FILE* asm_file);
  
  // Emit tbss (uninitialized tls variable) to the assembly file.
  void (*emit_tbss_space)(UnintializedStaticVariable* var, FILE* asm_file);

  // Emit start of string literals.
  void (*emit_literals_start)(FILE* asm_file);

  // Emit a string literal.
  void (*emit_string_literal)(StringLiteral* literal, FILE* asm_file);

  // Emit debug information.
  void (*emit_debug)(FILE* asm_file);

  // Clean up all memory used by the target for the given code.
  void (*cleanup)(void* code);
} CompilerTarget;

typedef struct {
  String infile;

  // Front end.
  int num_errors;
  int max_errors;
  bool convert_warnings_to_errors;
  bool enable_all_warnings;
  Set disabled_warnings;
  Preprocessor preprocessor;
  Lex lex;
  Syntax syntax;

  // Global symbol and tag tables created by front end.
  HashTable global_symbol_table;
  HashTable global_tag_table;

  // Back-end, specific to a target.
  CompilerTarget* target;
  String* target_name;

  // Vector containing all the code for all functions.
  Vector functions;

  // Vector containing initialized static variables (InitializedStaticVariable*)
  Vector initialized_static_variables;

  // Vector containing uninitialized static variables
  // (UninitializedStaticVariable*)
  Vector uninitialized_static_variables;

  // String literals: each element is a StringLiteral*.
  Vector string_literals;
  int next_literal_id;

  TypeRecord* current_function;
  bool debug_output;
  bool optimize;
  bool pic;

  TlsModel tls_model;
  
  int pointer_size;     // Size of a pointer.
  size_t current_include_path_index;
} Compiler;

// Globals to avoid passing these around.
extern Compiler* compiler;

bool CompilerInitFromFile(Compiler* compiler, const char* filename,
                          Vector* options);
bool CompilerInitFromString(Compiler* compiler, const char* filename,
                            const char* code, Vector* options);
bool CompilerInitForAssembler(const char* filename, Vector* options);

void CompilerDestruct(Compiler* compiler);
void CompilerDelete(Compiler* compiler);

String* CompileTranslationUnit(const char* filename, Vector* options);
String* CompileTranslationUnitFromString(const char* filename, const char* code,
                                      Vector* options);
int CompilerAddStringLiteral(String* value);
StringLiteral* CompilerFindStringLiteral(int literal_id);

#endif /* compiler_h */
