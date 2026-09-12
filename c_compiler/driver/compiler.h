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
#include "module_unit.h"

struct Generator;
struct Namespace;
struct ASTNode;
struct Syntax;
struct Struct;
struct ReflectionValue;
struct Compiler;

typedef enum {
  kInjectionTargetNamespace,
  kInjectionTargetClass,
  kInjectionTargetFunction,
} InjectionTargetKind;

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
  kTargetOSNone,
  kTargetOSLinux,
} TargetOS;

typedef struct {
  String architecture;
  String vendor;
  String os_name;
  String environment;
  String canonical;
  TargetOS os;
  bool explicit_triple;
} CompilerTargetTriple;

void CompilerTargetTripleInit(CompilerTargetTriple* triple);
void CompilerTargetTripleDestruct(CompilerTargetTriple* triple);
bool CompilerTargetTripleParse(CompilerTargetTriple* triple, const char* value,
                               char* error, size_t error_size);
bool CompilerTargetTripleIsLinux(const CompilerTargetTriple* triple);

typedef enum {
  kLanguageStandardC89,
  kLanguageStandardC99,
  kLanguageStandardC11,
  kLanguageStandardC17,
  kLanguageStandardC23,
  kLanguageStandardCXX98,
  kLanguageStandardCXX03,
  kLanguageStandardCXX11,
  kLanguageStandardCXX14,
  kLanguageStandardCXX17,
  kLanguageStandardCXX20,
  kLanguageStandardCXX23,
  kLanguageStandardCXX26,
  kLanguageStandardCXX29,
} LanguageStandard;

typedef enum {
  kConstexprEvalAuto,
  kConstexprEvalPCode,
  kConstexprEvalAST,
  kConstexprEvalAudit,
} ConstexprEvalMode;

typedef enum {
  kContractSemanticIgnore = 1,
  kContractSemanticObserve = 2,
  kContractSemanticEnforce = 3,
  kContractSemanticQuickEnforce = 4,
} ContractEvaluationSemantic;

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
  // For kInitTypeSymbol, the byte offset into the symbol that the address
  // designates: an address constant may be reached by selecting a member,
  // subscripting or pointer arithmetic, and each of those only shifts the
  // address, so "&x[18]" is the symbol x with an addend of 72.  Zero for every
  // other initializer type.  Set it through NewSymbolInitializer.
  int64_t symbol_addend;
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

// An initializer holding the address of symbol + addend, placed at offset within
// the object being initialized.
Initializer* NewSymbolInitializer(int32_t offset, Symbol* symbol,
                                  int64_t addend);

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

typedef struct {
  Symbol* thunk;
} CXXTlsBlockDtorThunk;

// A function pointer recorded for ELF .init_array / .fini_array emission.
// Lower priority values run earlier (GNU constructor/destructor convention).
typedef struct CXXInitFiniArrayEntry {
  Symbol* function;
  int priority;
} CXXInitFiniArrayEntry;

void CXXInitFiniArrayEntryDelete(CXXInitFiniArrayEntry* entry);
Vector* CXXInitArrayFunctionsVector(void);
Vector* CXXFiniArrayFunctionsVector(void);

enum {
  kCXXInitFiniPriorityDefault = 65535,
};

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
  int element_size;
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
  int sccp : 1;
  int const_prop : 1;
  int tail_call : 1;
  int dce : 1;
  int copy_prop : 1;
  int loop_preheaders : 1;
  int induction_vars : 1;
  int derived_induction_vars : 1;
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
  // Optional generated-input path that avoids a temporary assembly file.
  void (*emit_assembly_preamble)(String* src_file, FILE* asm_file);
  bool (*assemble_string)(const char* name, String* input,
                          String* object_filename);
  // Optional structured assembler path. It emits either a canonical .s file
  // or an ELF object from the same retained operation stream.
  String* (*emit_program_file)(struct Compiler* compiler, Vector* options,
                               bool assembly_only);
  String* (*emit_object_file)(struct Compiler* compiler, Vector* options);

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

  // DWARF register number of the frame pointer, or -1 if the target has none.
  int dwarf_frame_register;

  // Clean up all memory used by the target for the given code.
  void (*cleanup)(void* code);
  
  // Handle target specific option values.
  void (*handle_options)(Vector* opts);
} CompilerTarget;

void DeleteCompilerTarget(CompilerTarget* t);

typedef struct MetaPromotedStaticEntry {
  String key;
  Symbol* symbol;
} MetaPromotedStaticEntry;

typedef struct Compiler {
  String infile;

  // Front end.
  int num_errors;
  int max_errors;
  LanguageStandard language_standard;
  ContractEvaluationSemantic contract_semantic;
  int contract_assertion_depth;
  SourceLocation contract_assertion_location;
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
  // Signature-only template instantiations performed for unevaluated type
  // traits. Such probes may populate instantiation caches, but must not clone,
  // queue, analyze, or emit function bodies.
  int speculative_template_instantiation_depth;
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

  // C++20 module-unit metadata for this translation unit.
  ModuleUnitInfo module_unit;

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
  CompilerTargetTriple target_triple;
  bool target_supports_threads;
  bool target_supports_atomics;
  bool target_supports_c11_atomics;
  bool target_supports_8_byte_atomics;

  // Vector containing all the code for all functions.
  Vector functions;
  // Mangled names whose function bodies have already reached code generation.
  // Retained across draining template-instantiation queues to prevent emitting
  // the same specialization more than once.
  Vector emitted_function_asm_names;
  struct CompilerStringIndex* emitted_function_name_index;
  // Assembly names referenced by real target code generation.  C++ inline
  // definitions are semantically checked when parsed but emitted only after a
  // reachable function or initializer actually materializes their address.
  Vector referenced_function_asm_names;
  // Membership index for the vector above; the vector retains deterministic
  // insertion order while this index avoids repeated linear string scans.
  struct CompilerStringIndex* referenced_function_name_index;
  // Assembly names of namespace/static data referenced by reachable code.
  Vector referenced_variable_asm_names;

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
  // Symbols whose empty defaulted constructor performs no runtime work.
  Vector cxx_no_op_initialized_variables;
  Vector cxx_global_constructor_calls;  // ASTNode*, owned by init function body.
  Vector cxx_global_constructor_objects;  // Symbol* parallel to constructor_calls.
  // Per-thread C++ thread_local construction/destruction (not process-global).
  Vector cxx_thread_constructor_calls;  // ASTNode*, owned by synthetic init body.
  Vector cxx_thread_destructor_calls;   // ASTNode*, owned by synthetic fini body.
  Vector cxx_tls_block_dtor_thunks;  // CXXTlsBlockDtorThunk* entries.
  Vector cxx_this_adjustor_thunks;  // CXXThisAdjustorThunk* entries.
  // InitializedStaticVariable* entries for compiler-generated vtables,
  // virtual-base tables, and RTTI records. These are emitted only when
  // reachable code references them.
  Vector cxx_lazy_static_variables;

  // De-duplication map for RTTI std::type_info objects: String* mangled key ->
  // Symbol* naming the emitted type_info.  Keys are owned by the map.
  Map rtti_typeinfo_map;

  // Compiler-owned C++26 reflection values.  Their entity pointers borrow from
  // the normal symbol/type graph and remain valid for the translation unit.
  Vector reflection_values;

  // Canonical static symbols promoted by define_static_* synthesis calls.
  Vector meta_promoted_statics;

  Vector literals;     // Literals
  int next_literal_id;

  // Roots of every external declaration's AST (ASTNode*).  Retained so the
  // whole AST forest can be destructed once at CompilerDestruct.  The node
  // structs themselves are arena allocated and freed by ASTArenaRelease.
  Vector declaration_asts;

  // Non-template class/struct types defined in this translation unit, in
  // definition order (Struct*, not owned; the structs live in their type
  // records/symbol tables).  Used at end of translation unit to diagnose
  // unused private data members (-Wunused-private-field).
  Vector cxx_defined_classes;

  // Declaration ASTs synthesized while instantiating templates. Function
  // definitions may be stored directly as vardecl nodes; other declarations
  // retain their ordinary declaration-list roots.
  Vector pending_template_instantiations;
  // Shared FIFO cursor and nesting depth. Compilation can recursively request
  // another drain, so all active drains must advance the same cursor and only
  // the outermost drain may clear the vector.
  size_t pending_template_instantiation_head;
  size_t pending_template_instantiation_drain_depth;
  // Owned asm-name index for definitions in the queue above.
  struct CompilerStringIndex* pending_template_instantiation_names;
  // Non-owning ASTNode* entries containing a symbol that had no final asm name
  // or definition body when queued. Late updates are recovered by scanning
  // only this typically empty subset, rather than every pending declaration.
  Vector unindexed_pending_template_instantiations;

  // Function-definition symbols (Symbol*) that are not stored in the global
  // symbol table because the function was previously declared.  Each owns a
  // freshly-parsed function type (forming a symbol<->type cycle), so they are
  // tracked here and freed at CompilerDestruct rather than leaked.
  Vector orphan_function_symbols;

  int next_symbol_id; // Next symbol id.
  
  // Flags.
  bool debug_output;
  bool optimize;
  bool optimize_for_size;
  bool pic;
  bool syntax_only;  // -fsyntax-only: frontend only, no code generation.
  bool lto;  // -flto: emit or consume IR modules instead of per-TU native objects.
  bool lto_ir_only;  // Generate IR and skip target lowering (write a DCCLTO03 object).
  struct LTOModule* lto_module;
  bool exceptions_enabled;  // C++ exception handling enabled (-f[no-]exceptions).
  bool printf_specialize;    // Rewrite constant printf-family calls by profile.
  bool module_header;        // Compiling the object half of a header unit.
  bool print_front_end;
  bool print_back_end;
  bool print_preprocessor;
  bool keep_asm_file;
  bool save_ir;
  bool save_ast;
  ConstexprEvalMode constexpr_eval_mode;
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

  // Set of function types (TypeRecord*) whose bodies are currently being
  // semantically analyzed.  A body is incomplete (has untyped nodes) exactly
  // while its analysis is in flight, so speculative constant folding must not
  // lower any function in this set to pcode -- doing so is what previously
  // crashed code generation on recursive/mutually-recursive constexpr calls.
  // This is the primary guard; the constexpr_codegen_recover longjmp above is
  // kept only as defense in depth.
  Vector functions_being_analyzed;

  // Nonzero while semantically analyzing an immediate-function context, such
  // as the constant-evaluation arm of a C++23 `if consteval`. Calls to
  // immediate functions are permitted to depend on enclosing parameters there.
  int immediate_function_context_depth;

  // Nonzero while analyzing an expression whose value is required at compile
  // time. This distinguishes manifest constant evaluation from speculative
  // runtime folding for context-sensitive constructs such as `if consteval`.
  int constant_evaluation_required_depth;

  // Nonzero while analyzing the unevaluated operand of a noexcept-expression.
  // Keep calls intact so exception-specification analysis sees the selected
  // callee rather than a folded constant result.
  int noexcept_operand_depth;

  // P3294 token-sequence injection frame stack (InjectionFrame*, owned).
  Vector injection_frames;
  // Parsed declaration roots produced by injection; drained at safe driver points.
  Vector pending_injected_declarations;
} Compiler;

// Globals to avoid passing these around.
extern Compiler* compiler;

void CompilerQueuePendingTemplateInstantiation(struct ASTNode* declaration);
void CompilerQueuePendingFunctionDefinition(struct Symbol* symbol);
bool CompilerPendingTemplateInstantiationHasAsmName(const char* asm_name);

bool CompilerInitFromFile(Compiler* compiler, const char* filename,
                          Vector* options, Vector* target_opts);
bool CompilerInitFromString(Compiler* compiler, const char* filename,
                            const char* code, Vector* options);
bool CompilerInitForLTOLink(Compiler* compiler, const char* filename,
                            Vector* options, Vector* target_opts);
bool CompilerInitForAssembler(const char* filename, Vector* options);

void CompilerDestruct(Compiler* compiler);
void CompilerDelete(Compiler* compiler);

// Runs the front end (preprocess/parse/semantic) for the current translation
// unit without code generation, leaving the symbol/type/AST graph populated.
// Returns true on success (no errors).  Used by the hidden -Xemit-module hook.
bool CompileFrontEndOnly(Compiler* compiler);

// C++20 module import hook.  The parser cannot depend on the serialize library
// (which depends on the compiler), so the driver registers a handler that
// locates and installs a module's interface when the parser encounters an
// `import foo;` directive.  `ctx` is opaque state owned by the driver.
typedef bool (*ModuleImportHandler)(void* ctx, const char* module_name);
void SetModuleImportHandler(ModuleImportHandler fn, void* ctx);

// Invoked by the parser for `import foo;`.  Dispatches to the registered
// handler; returns false if no handler is installed or the import fails.
bool CompilerImportModule(const char* module_name);

// True when a driver has registered a module import handler for this compile.
bool CompilerHasModuleImportHandler(void);

// Optional detail for the most recent failed CompilerImportModule call on the
// active translation unit.  Owned by the driver/import store; valid only until
// the next import attempt.
const char* CompilerImportLastError(void);

// Records a driver-provided detail string for the next import diagnostic.
void CompilerSetLastImportError(const char* message);

// Attaches per-translation-unit import state released from CompilerDestruct
// before lookup tables are torn down.  `release` may be NULL only when clearing.
typedef void (*TranslationUnitImportReleaseFn)(void* state);
void CompilerSetImportState(void* state, TranslationUnitImportReleaseFn release);

String* CompileTranslationUnit(const char* filename, Vector* options, Vector* target_opts);
String* CompileTranslationUnitFromString(const char* filename, const char* code,
                                         Vector* options);
String* CompilerEmitTranslationUnit(struct Compiler* compiler, Vector* options);
bool CompileTranslationUnitToLTOIR(const char* filename, Vector* options,
                                   Vector* target_opts);
void CompilerPrepareForIRLoad(void);
#include "lto_module.h"
int CompilerAddStringLiteral(String* value, int element_size);
int CompilerAddBufferLiteral(const void* data, size_t length);
StringLiteral* CompilerFindStringLiteral(int literal_id);
BufferLiteral* CompilerFindBufferLiteral(int literal_id);
Literal* CompilerFindLiteral(int literal_id);

bool OptLevel0(void);
bool OptLevel1(void);
bool OptLevel2(void);
bool OptLevel3(void);
static inline bool CompilerIsCXX(void) {
  return compiler != NULL &&
         compiler->language_standard >= kLanguageStandardCXX98;
}
static inline bool CompilerCAtLeast(LanguageStandard standard) {
  return compiler != NULL &&
         compiler->language_standard < kLanguageStandardCXX98 &&
         compiler->language_standard >= standard;
}
static inline bool CompilerCXXAtLeast(LanguageStandard standard) {
  return compiler != NULL &&
         compiler->language_standard >= kLanguageStandardCXX98 &&
         compiler->language_standard >= standard;
}
bool CompilerTargetSupportsThreads(void);
bool CompilerTargetSupportsAtomics(void);
bool CompilerTargetSupportsC11Atomics(void);
bool CompilerTargetSupportsAtomicSize(int size);
bool CompilerExceptionsEnabled(void);
void CompilerMarkFunctionReferenced(struct Symbol* symbol);
void CompilerMarkVariableReferenced(struct Symbol* symbol);
void CompilerRegisterMetaPromotedStatic(struct Symbol* symbol, struct ASTNode* initializer);
void CompilerRegisterTemplateParameterObject(struct Symbol* symbol,
                                             struct ASTNode* initializer);
bool CompilerSymbolIsMetaPromotedStatic(struct Symbol* symbol);
bool CompilerSymbolIsMetaPromotedString(struct Symbol* symbol);
struct Symbol* CompilerMetaPromotedPointerTarget(struct Symbol* pointer_symbol);
struct Symbol* CompilerConstantFunctionPointerTarget(
    struct Symbol* pointer_symbol);
bool ConstexprEnsureMetaPromotedStaticObject(struct Symbol* promoted);
void CompilerRegisterLazyCXXStatic(InitializedStaticVariable* var);

int CharSize(void);
int IntSize(void);
int ShortSize(void);
int PointerSize(void);
int BoolSize(void);
int LongSize(void);
int LongLongSize(void);

void PrintCompilerHelp(void);

bool CompilerInjectionFrameActive(void);
void CompilerRecordSynthesizedEnum(Enum* enumeration, TypeRecord* enum_type,
                                   struct Symbol* tag);
bool CompilerBeginInjectionFrame(InjectionTargetKind kind,
                                 struct Namespace* target_ns,
                                 struct Struct* target_class,
                                 int target_class_access,
                                 struct ASTNode* target_function_body,
                                 size_t function_insert_index, bool temporary);
bool CompilerBeginInjectionFrameForSyntax(struct Syntax* syntax,
                                          int target_class_access);
bool CompilerBeginFunctionInjectionFrame(Vector* statements,
                                         size_t function_insert_index);
void CompilerRollbackInjectionFrame(void);
bool CompilerCommitInjectionFrame(struct Syntax* syntax);
bool CompilerQueueInjection(struct ReflectionValue* sequence,
                            SourceLocation location, struct ASTNode* diagnostic);
bool CompilerNamespaceInject(struct ReflectionValue* ns_value,
                             struct ReflectionValue* sequence,
                             SourceLocation location, struct ASTNode* diagnostic);
void CompilerQueueInjectedDeclaration(struct ASTNode* declaration_root);
void CompilerRecordInjectedSymbol(struct Namespace* namespace_,
                                  struct LocalSymbolTable* local_table,
                                  struct Symbol* symbol, bool is_tag,
                                  bool is_global);
struct ASTNode* CompilerPopPendingInjectedDeclaration(void);
void CompilerDrainPendingInjectedDeclarations(struct Syntax* syntax);
void CompilerCompileQueuedDeclaration(struct Syntax* syntax,
                                      struct ASTNode* node);
void CompilerInjectionFramesTeardown(void);

#endif /* compiler_h */
