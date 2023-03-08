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
#include "buffer.h"
#include "hashtable.h"
#include "set.h"
#include "type.h"
#include "vector.h"
#include "debug.h"
#include "options.h"

struct Generator;

// Thread local storage model.
#define TLS(x) kTls_##x
typedef enum {
  TLS(bad),
  TLS(global_dynamic),
  TLS(local_dynamic),
  TLS(initial_exec),
  TLS(local_exec),
} TlsModel;

typedef enum {
  kInitTypeByte,    // 8-bit constant.
  kInitTypeHalf,    // 16-bit constant.
  kInitTypeWord,    // 32-bit constant.
  kInitTypeLong,    // 64-bit constant
  kInitTypeSymbol,  // Reference to a symbo.
  kInitTypeString,  // String literal reference.
  kInitTypeMemory,  // Fixed memory contents.
} InitializerType;

typedef struct {
  InitializerType type;
  int32_t offset;
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
  Symbol* symbol;
  bool is_global;  // Variable is global (can be seen outside of file).
  size_t size;
  int32_t alignment;
  Vector initializers;
  bool is_tls;
  bool is_local;    // Local (inside a function).
} InitializedStaticVariable;

void InitializedStaticVariableDelete(InitializedStaticVariable* var);

// An uninitiaized static variable.
typedef struct {
  Symbol* symbol;
  bool is_global;  // Variable is global (can be seen outside of file).
  size_t size;
  size_t alignment;
  bool is_tls;
  bool is_local;    // Local (inside a function).
} UninitializedStaticVariable;

// A literal has an id.  The value is in a derived class.
typedef enum {
  kLiteralString,
  kLiteralWideString,
  kLiteralBuffer,
} LiteralType;

typedef struct {
  LiteralType type;
  int id;
  bool disabled;
} Literal;

typedef struct {
  Literal base;
  String value;
} StringLiteral;

void LiteralDelete(Literal* literal);

typedef struct {
  Literal base;
  Buffer value;
} BufferLiteral;

void UninitializedStaticVariableDelete(UninitializedStaticVariable* var);

TlsModel ParseTlsModelName(String* name);

// High level codegen preferences.
typedef enum {
  kCodeForSpeed,      // Prefer speed over size.
  kCodeForSize        // Prefer size over speed.
} CodePreference;

typedef struct {
  int gvn : 1;
  int code_motion : 1;
  int const_prop : 1;
  int tail_call : 1;
} IROptimizations;

// A compiler target back-end.  This contains pointers to
// functions to generate code for a particlar target.
typedef struct {
  String name;       // Name of target.
  int pointer_size;  // Size of pointer.
  int short_size;
  int int_size;
  int long_size;
  int long_long_size;
  int bool_size;
  int float_size;
  int double_size;
  int wchar_size;
  int stack_alignment;      // Power of 2 stack alignment.
  bool plain_char_is_signed;
  CodePreference code_preference;
  bool call_return_fixed_reg;   // Is the value of a call in a fixed register?
  bool keep_ssa;
  int alignment;       // Alignment to apply.
  IROptimizations ir_optimizations;            // IR optimizations.
  bool prepend_underscore;          // Prepend underscore to external symbols.
  int flags;                        // Target specific flags.
  
  // Target supplied options;
  CompilerOptionDefinition* options;
  
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
  void (*emit_bss_space)(UninitializedStaticVariable* var, FILE* asm_file);

  // Emit start of tdata section to asm file.
  void (*emit_tdata_start)(FILE* asm_file);

  // Emit start of tbss section to asm file.
  void (*emit_tbss_start)(FILE* asm_file);

  // Emit tls data to the assembly file
  void (*emit_tls_variable)(InitializedStaticVariable* var, FILE* asm_file);

  // Emit tbss (uninitialized tls variable) to the assembly file.
  void (*emit_tbss_space)(UninitializedStaticVariable* var, FILE* asm_file);

  // Emit start of string literals.
  void (*emit_literals_start)(FILE* asm_file);

  // Emit a literal.
  void (*emit_literal)(Literal* literal, FILE* asm_file);

  // Emit debug information.
  void (*emit_debug)(FILE* asm_file);

  // Clean up all memory used by the target for the given code.
  void (*cleanup)(void* code);
  
  // Handle target specific option values.
  void (*handle_options)(Vector* opts);
} CompilerTarget;

void DeleteCompilerTarget(CompilerTarget* t);

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

  // DWARF debug builder.
  DebugBuilder debug_builder;
  
  // Global symbol and tag tables created by front end.
  HashTable global_symbol_table;
  HashTable global_tag_table;

  int pointer_size;  // Size of a pointer.
  int short_size;  // Size of native short int.
  int int_size;  // Size of native int.
  int bool_size;  // Size of native bool.
  int long_size;  // Size of native long.
  int long_long_size;  // Size of native long long.
  int float_size;  // Size of native float.
  int double_size;  // Size of native double.
  int wchar_size;
  bool plain_char_is_signed;
  
  CodePreference code_preference;
  bool call_return_fixed_reg;   // Is the value of a call in a fixed register?
  bool keep_ssa;             // Keep SSA form for lowering codegen.
  IROptimizations ir_optimizations;                  // Do IR optimizations.
  int alignment;
  bool prepend_underscore;          // Prepend underscore to external symbols.
  bool target_flags;
  
  size_t current_include_path_index;
  TypeRecord* current_function;
  TlsModel tls_model;

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

  Vector literals;     // Literals
  int next_literal_id;

  int next_symbol_id; // Next symbol id.
  
  // Flags.
  bool debug_output;
  bool optimize;
  bool pic;
  bool print_front_end;
  bool print_back_end;
  bool print_preprocessor;
  bool keep_asm_file;
  bool save_ir;
  bool save_ast;
  FILE* ir_output_file;
  FILE* ast_output_file;
  int opt_level;
} Compiler;

// Globals to avoid passing these around.
extern Compiler* compiler;

bool CompilerInitFromFile(Compiler* compiler, const char* filename,
                          Vector* options, Vector* target_opts);
bool CompilerInitFromString(Compiler* compiler, const char* filename,
                            const char* code, Vector* options);
bool CompilerInitForAssembler(const char* filename, Vector* options);

void CompilerDestruct(Compiler* compiler);
void CompilerDelete(Compiler* compiler);

String* CompileTranslationUnit(const char* filename, Vector* options, Vector* target_opts);
String* CompileTranslationUnitFromString(const char* filename, const char* code,
                                         Vector* options);
int CompilerAddStringLiteral(String* value, bool is_wide);
int CompilerAddBufferLiteral(const void* data, size_t length);
StringLiteral* CompilerFindStringLiteral(int literal_id);
BufferLiteral* CompilerFindBufferLiteral(int literal_id);
Literal* CompilerFindLiteral(int literal_id);

bool OptLevel0(void);
bool OptLevel1(void);
bool OptLevel2(void);
bool OptLevel3(void);

int CharSize(void);
int IntSize(void);
int ShortSize(void);
int PointerSize(void);
int BoolSize(void);
int LongSize(void);
int LongLongSize(void);

void PrintCompilerHelp(void);

#endif /* compiler_h */
