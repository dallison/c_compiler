//
//  symbol.h
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

// Symbols.

#ifndef symbol_h
#define symbol_h

#include "buffer.h"
#include "dstring.h"
#include "vector.h"
#include "source.h"

struct TypeRecord;
struct Struct;
struct DIE;
struct Namespace;
struct ASTNode;
struct Concept;

typedef enum {
  kCXXLinkageExternal = 0,
  kCXXLinkageInternal = 1,
  kCXXLinkageModule = 2,
} CXXLinkageKind;

// Storage for symbol (where it is located in memory).
#define STO(x) kStorage_##x
typedef enum {
  STO(implicit) = 0,         // Implied.
  STO(auto) = 1<<0,          // Automatic (on stack).
  STO(static) = 1<<1,        // Static (in data segment)
  STO(typedef) = 1<<2,       // Type definition.
  STO(extern) = 1<<3,        // External (not defined in this program).
  STO(register) = 1<<4,      // In a register (not used in modern C).
  STO(assembler) = 1<<5,     // Assembler label.
  STO(thread) = 1<<6,        // Thread local.
} Storage;

bool StorageIs(Storage storage, Storage value);

// A C++ variable template (`template <class T> constexpr U name = <init>;`).
// The initializer is kept unanalyzed at parse time and re-cloned, substituted,
// and folded per use with concrete template arguments (see
// TypeInstantiateVariableTemplateConstant).
typedef struct VariableTemplate {
  struct ASTNode* initializer;  // Unanalyzed initializer expression (owned).
  Vector parameters;            // TemplateParameter* entries (owned).
  struct ConstraintExpr* associated_constraint;  // C++20 requires-clause. // @wire 3
  Vector partial_specializations;  // ClassTemplatePartialSpecialization* (owned).
} VariableTemplate;

// Parameter list for an alias template whose RHS does not carry a template-id
// pattern (e.g. `template<typename T> using X = int;`).
typedef struct AliasTemplate {
  Vector parameters;  // TemplateParameter* entries (owned). // @wire 1
} AliasTemplate;

// A parsed __attribute__((...)) clause: a name with optional argument tokens.
// e.g. "aligned(16)"          -> name "aligned", args ["16"]
//      "format(printf, 1, 2)" -> name "format",  args ["printf", "1", "2"]
//      "packed"               -> name "packed",  args []
// The name is normalized by stripping a surrounding "__" pair so that
// "__packed__" and "packed" compare equal (as GCC does).
// Serialized as an inline sub-message (see WriteAttributeVector); the field
// numbers below are local to that sub-message.
typedef struct Attribute {
  String name;   // Normalized attribute name.               // @wire 1
  Vector args;   // Vector of String* argument tokens.       // @wire 2
} Attribute;

Attribute* NewAttribute(const char* name);
void AttributeDestruct(Attribute* attr);
void AttributeDelete(Attribute* attr);
Attribute* AttributeClone(Attribute* attr);

// Append an argument token (a copy is made).
void AttributeAddArg(Attribute* attr, const char* arg, size_t length);
size_t AttributeArgCount(Attribute* attr);
// Returns the raw argument token at index, or NULL if out of bounds.
const char* AttributeArgString(Attribute* attr, size_t index);
// Parses the argument at index as a base-10 integer.  Returns false (and
// leaves *value unchanged) if missing or not a valid integer.
bool AttributeArgInt(Attribute* attr, size_t index, long* value);

// Operations on a Vector of Attribute*.
Attribute* AttributeListFind(Vector* attrs, const char* name);
bool AttributeListHas(Vector* attrs, const char* name);
void AttributeListDestruct(Vector* attrs);          // Frees contained Attribute*.
void AttributeListClone(Vector* dest, Vector* src); // dest is initialized.

// A symbol.  This is a variable, function or type used in a program.
// Module-serialization field numbers (see
// c_compiler/serialize/symbol_serialize.c).  Transient/codegen fields
// (usage_info, die) are recomputed on load and carry no wire number.
typedef struct Symbol {
  String name;                // Symbol name.                     // @wire 1
  String asm_name;            // Optional asm-visible name.       // @wire 2
  struct Namespace* namespace_;  // Owning C++ namespace, if any. // @wire 3
  int id;                                                         // @wire 4
  struct TypeRecord* type;    // Type.                            // @wire 5
  Storage storage;            // Storage (static, typedef, etc.)  // @wire 6
  struct {
    bool is_defined: 1;            // Symbol is defined.              // @wire 7
    bool is_tentative_decl: 1;     // Tentative declaration.          // @wire 8
    bool is_forward_declared: 1;   // Symbol is forward declared.     // @wire 9
    bool is_local: 1;              // Local symbol.                   // @wire 10
    bool is_block_scope: 1;        // Declared at block scope.        // @wire 11
    bool is_argument: 1;           // Defined in function prototype.  // @wire 12
    bool is_temp: 1;               // Temporary (invented).           // @wire 13
    bool address_taken: 1;         // Address has been taken.         // @wire 14
    bool used: 1;                  // The symbol has been used.       // @wire 15
    bool invented: 1;                                                 // @wire 16
    bool is_inline_defn: 1;        // Is an inline function defn.     // @wire 17
    bool value_set : 1;            // Value has been set (const).     // @wire 18
    bool noreturn: 1;              // noreturn / _Noreturn.            // @wire 19
    bool always_inline: 1;         // __attribute__((always_inline)). // @wire 20
    bool noinline: 1;              // __attribute__((noinline)).      // @wire 21
    bool is_using_alias: 1;        // C++ using-declaration alias.    // @wire 22
    bool is_overloaded: 1;         // Has C++ overload alternatives.  // @wire 23
    bool is_template: 1;           // C++ template declaration.       // @wire 24
    bool is_template_parameter: 1; // C++ template parameter.         // @wire 25
    bool is_template_type_parameter: 1; // typename/class parameter.  // @wire 26
    bool is_template_template_parameter: 1; // template<...> class.   // @wire 55
    bool is_parameter_pack: 1;     // Template/function param pack.   // @wire 27
    bool is_constexpr: 1;          // C++ constexpr variable.         // @wire 28
    bool is_constinit: 1;          // C++ constinit variable.         // @wire 29
    bool is_weak: 1;               // Emits ELF weak binding.         // @wire 30
    bool is_c_linkage: 1;          // C language linkage (extern "C").// @wire 31
    bool is_exported: 1;           // C++20 module export.            // @wire 41
    bool is_concept: 1;            // C++20 concept definition.       // @wire 43
    bool is_module_private: 1;     // Declared in private fragment.    // @wire 52
    bool is_explicit_specialization: 1;  // `template <>` function.    // @wire 54
  } flags;
  
  struct {
    int used_as_arg;              // Times used as function arg.
    int used_in_loop;             // Times used in loop.
    int reads;                    // Number of reads.
  } usage_info;                   // @wire - (transient, not serialized)
  
  Vector attributes;          // Attributes (owns Attribute*).    // @wire 42
  int alignment;              // aligned(N) override; 0 = natural. // @wire 32
  int template_parameter_index;  // Template parameter index.      // @wire 33
  int dependent_value_template_parameter_index;  // Deferred value. // @wire 34
  SourceLocation location;                                         // @wire 35
  
  // Symbol value, one of these.  Serialized as the raw 64-bit slot.
  union {
    int64_t ivalue;         // Integer value for constants.      // @wire 36
    double fvalue;          // Double value for constants.       // (via 36)
    int32_t arg_number;     // Argument number in prototype.     // (via 36)
    void* other;            // Something else.                   // @wire -
    struct Symbol* func_defn;      // Defn of this func decl.     // @wire -
  } value;
  int32_t stack_offset;     // Stack offset if local.            // @wire 37
  struct Symbol* alias_target;  // C++ using-declaration target. // @wire 38
  struct Symbol* overload_next; // Next overload, same name.     // @wire 39
  struct ASTNode* default_argument; // C++ default arg, if any.  // @wire 40
  struct VariableTemplate* variable_template;  // C++ variable template body. // @wire 44
  struct AliasTemplate* alias_template;  // C++ alias template parameters. // @wire 47
  // Owned signature copy for a template-template parameter placeholder.
  Vector* template_template_parameters;  // TemplateParameter*.          // @wire 56
  struct Concept* concept_definition;  // C++20 concept body when flags.is_concept. // @wire 45
  // C++20 requires-clause for alias templates (`template<...> using A = ...`).
  // Function/class/variable templates store constraints on their type bodies;
  // alias templates have no other durable owner.
  struct ConstraintExpr* associated_constraint;  // @wire 46
  CXXLinkageKind cxx_linkage;        // C++20 external/internal/module. // @wire 48
  String owning_module_name;         // Named-module purview owner.      // @wire 49
  String owning_module_partition;    // Owning partition, if any.       // @wire 50
  String import_source_module;       // Import provenance for importers. // @wire 51
  struct DIE* die;          // @wire - (debug info, not serialized)
  // Deserialized module symbols are graph-owned until the importing compiler
  // has finished tearing down structs/types that may still point at them.
  bool is_imported_module_symbol;  // @wire - (transient)
  bool destruction_complete;      // @wire - (transient)
  // Set when the symbol's value is read (used in any context other than as the
  // pure left-hand side of a plain assignment).  Combined with flags.used this
  // distinguishes "referenced but only written" objects for
  // -Wunused-but-set-variable.  Recomputed during analysis, never serialized.
  bool is_read;                   // @wire - (transient)
  // Local object selected for named return-value optimization.  Semantic
  // analysis sets this before IR generation so every reference to the pooled
  // variable uses the hidden struct-return address.
  bool is_nrvo;                   // @wire - (transient)
  // Compiler-owned copy of a deserialized function template's parameter list.
  // Imported module symbols can have their live parameter vector cleared while
  // pending instantiations are compiled; this backup restores completion.
  Vector imported_function_template_parameters_backup;  // transient
  // Lazily computed, full (untruncated) target assembly symbol name.  Owned by
  // the symbol and reused across every emission site so that long mangled names
  // (e.g. deeply nested template specializations) are never truncated to a
  // caller's fixed-size scratch buffer, which would let distinct symbols
  // collide.  @wire - (transient)
  char* cached_target_symbol_name;
} Symbol;


void SymbolInit(Symbol* sym, const char* name, struct TypeRecord* type,
                Storage storage);
Symbol* NewSymbol(const char* name, struct TypeRecord* type, Storage storage);
void SymbolDelete(Symbol* symbol);
void SymbolDestruct(Symbol* symbol);

Symbol* SymbolClone(Symbol* sym);

void SymbolSetType(Symbol* symbol, struct TypeRecord* type);
void SymbolSetCXXMangledAsmName(Symbol* symbol);
void SymbolBackupImportedFunctionTemplateParameters(Symbol* symbol);
void SymbolRestoreImportedFunctionTemplateParameters(Symbol* symbol);
// Appends the Itanium-style mangled encoding of `type` to `out`.  Used to form
// canonical, stable keys/symbol names for RTTI type_info objects.
void AppendCXXMangledTypeName(String* out, struct TypeRecord* type);
void SymbolSetCXXDataAsmName(Symbol* symbol, struct Struct* owner);

// Adds attribute and takes ownership of the Attribute.
void SymbolAddAttribute(Symbol* symbol, Attribute* attribute);
bool SymbolHasAttribute(Symbol* symbol, const char* attribute);
// Returns the attribute with the given (normalized) name, or NULL.
Attribute* SymbolFindAttribute(Symbol* symbol, const char* attribute);
bool SymbolHasWeakBinding(Symbol* symbol);

void SymbolPrintDetails(Symbol* sym, bool with_function_body, FILE* fp);
void SymbolPrint(Symbol* sym, FILE* fp);

#endif /* symbol_h */
