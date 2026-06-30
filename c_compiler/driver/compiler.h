//
//  compiler.h
//  c_compiler
//
//  Created by David Allison on 11/1/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef compiler_h
#define compiler_h

#include <setjmp.h>
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
struct Namespace;

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
  kLanguageStandardC89,
  kLanguageStandardC99,
  kLanguageStandardC11,
  kLanguageStandardC17,
  kLanguageStandardCXX98,
  kLanguageStandardCXX03,
  kLanguageStandardCXX11,
  kLanguageStandardCXX14,
  kLanguageStandardCXX17,
  kLanguageStandardCXX20,
} LanguageStandard;

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
  bool is_weak;    // Variable has weak external binding.
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
  bool is_weak;    // Variable has weak external binding.
  size_t size;
  size_t alignment;
  bool is_tls;
  bool is_local;    // Local (inside a function).
} UninitializedStaticVariable;

typedef struct {
  Symbol* thunk;
  Symbol* target;
  int this_adjustment;
} CXXThisAdjustorThunk;

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

  // Emit target-specific C++ helper thunks.
  void (*emit_cxx_thunks)(FILE* asm_file);

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
  LanguageStandard language_standard;
  bool convert_warnings_to_errors;
  bool enable_all_warnings;
  Set disabled_warnings;
  // Warnings promoted to errors by -Werror=<name>.
  Set error_warnings;
  // Warnings exempted from -Werror by -Wno-error=<name>.
  Set no_error_warnings;
  // Stack of saved warning states for #pragma diagnostic push/pop.
  Vector diagnostic_stack;
  int diagnostic_suppression_depth;
  // Tentative-parse error trap: while the depth is non-zero, errors are
  // swallowed (neither printed nor counted) and `diagnostic_error_trapped` is
  // set, so a speculative parse can be rolled back without user-visible output.
  int diagnostic_error_trap_depth;
  bool diagnostic_error_trapped;
  // Current #pragma pack(n) member alignment cap (0 = no packing in effect).
  int pack_alignment;
  // Stack of saved pack values for #pragma pack(push[,n]) / pack(pop).
  Vector pack_stack;
  Preprocessor preprocessor;
  Lex lex;
  Syntax syntax;

  // DWARF debug builder.
  DebugBuilder debug_builder;
  
  // Global symbol and tag tables created by front end.
  HashTable global_symbol_table;
  HashTable global_tag_table;
  struct Namespace* global_namespace;

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
  // While analyzing a class's own static data member initializer (which is not
  // inside any member function), this names the enclosing class so member
  // access control treats the initializer as if it were inside a member of that
  // class.  NULL when not analyzing such an initializer.
  struct Struct* current_class_access_context;
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

  // In-class `static constexpr`/`constinit` data members whose own type is the
  // (then-incomplete) enclosing class, so their constant evaluation was deferred
  // by the parser until the whole translation unit is parsed and the class --
  // including its constructors' inline bodies -- is complete.  Elements are
  // VariableDeclarationASTNode* owned by the declaration ASTs.
  Vector cxx_deferred_static_member_definitions;

  // Namespace-scope C++ objects that need dynamic construction/destruction.
  // Elements are Symbol* owned by the normal symbol tables.
  Vector cxx_global_constructors;
  Vector cxx_global_destructors;
  Vector cxx_global_destructor_calls;  // ASTNode*, owned by declaration ASTs.
  Vector cxx_this_adjustor_thunks;  // CXXThisAdjustorThunk* entries.

  // De-duplication map for RTTI std::type_info objects: String* mangled key ->
  // Symbol* naming the emitted type_info.  Keys are owned by the map.
  Map rtti_typeinfo_map;

  Vector literals;     // Literals
  int next_literal_id;

  // Roots of every external declaration's AST (ASTNode*).  Retained so the
  // whole AST forest can be destructed once at CompilerDestruct.  The node
  // structs themselves are arena allocated and freed by ASTArenaRelease.
  Vector declaration_asts;

  // Declaration ASTs synthesized while instantiating templates.  Drained by
  // the driver through the normal semantic/codegen path.
  Vector pending_template_instantiations;

  // Function-definition symbols (Symbol*) that are not stored in the global
  // symbol table because the function was previously declared.  Each owns a
  // freshly-parsed function type (forming a symbol<->type cycle), so they are
  // tracked here and freed at CompilerDestruct rather than leaked.
  Vector orphan_function_symbols;

  int next_symbol_id; // Next symbol id.
  
  // Flags.
  bool debug_output;
  bool optimize;
  bool pic;
  bool exceptions_enabled;  // C++ exception handling enabled (-f[no-]exceptions).
  bool print_front_end;
  bool print_back_end;
  bool print_preprocessor;
  bool keep_asm_file;
  bool save_ir;
  bool save_ast;
  FILE* ir_output_file;
  FILE* ast_output_file;
  int opt_level;

  // Recovery point for soft failures while lowering a function to pcode for
  // constant evaluation.  Constant evaluation is speculative: it may try to
  // fold a call whose callee's inline body has not yet been semantically
  // analyzed (and so still has untyped nodes).  When `constexpr_codegen_recover`
  // is set, code-generation paths that would otherwise assert on such a node
  // longjmp to `constexpr_codegen_abort` instead, so the fold fails gracefully
  // and the caller falls back to the AST interpreter / a runtime expression.
  // Outside constant evaluation the flag is clear and those asserts still fire
  // on genuine compiler bugs.
  jmp_buf constexpr_codegen_abort;
  bool constexpr_codegen_recover;
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
bool CompilerIsCXX(void);
bool CompilerCXXAtLeast(LanguageStandard standard);
bool CompilerExceptionsEnabled(void);

int CharSize(void);
int IntSize(void);
int ShortSize(void);
int PointerSize(void);
int BoolSize(void);
int LongSize(void);
int LongLongSize(void);

void PrintCompilerHelp(void);

#endif /* compiler_h */
