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
#include "value_state.h"

struct TypeRecord;
struct Struct;
struct DIE;
struct Namespace;
struct ASTNode;
struct Concept;
struct TemplateArgument;

typedef enum {
  kTemplateTemplateParameterType,
  kTemplateTemplateParameterVariable,
  kTemplateTemplateParameterConcept,
} TemplateTemplateParameterKind;

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
  // The defining type-id names one of this alias's template-template
  // parameters (directly or through another alias). Such an alias is not a
  // deducible template for CTAD.
  bool ctad_names_template_template_parameter;                // @wire 2
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
  String name;   // Normalized semantic name.                // @wire 1
  Vector args;   // Vector of String* argument tokens.       // @wire 2
  // Unevaluated operands for a dependent C++ alignas specifier. Exactly one
  // is non-NULL; template instantiation substitutes it and writes the
  // resulting concrete alignment to the owning Symbol.
  struct TypeRecord* dependent_alignas_type;                  // @wire 3
  struct ASTNode* dependent_alignas_expr;                     // @wire 4
  struct ASTNode* annotation_expr;                            // @wire 5
  struct ReflectionValue* annotation_value;                     // @wire 6
  // P3385 identity. `name` remains normalized for existing semantic queries,
  // while these preserve the source-level attribute-token.
  String attribute_namespace;                                 // @wire 7
  String token;                                               // @wire 8
  // Location of the attribute-token, used for follow-up diagnostics.
  SourceLocation location;                                    // @wire 9
  // `alignas(Ts...)` / `alignas(Ns...)`: the dependent operand is expanded
  // over its pack and the strictest element alignment applies.
  bool dependent_alignas_is_pack_expansion;                   // @wire 10
} Attribute;

Attribute* NewAttribute(const char* name);
void AttributeSetCXXIdentity(Attribute* attr, const char* namespace_name,
                             const char* token);
void AttributeDestruct(Attribute* attr);
void AttributeDelete(Attribute* attr);
Attribute* AttributeClone(Attribute* attr);
const char* AttributeIdentifier(const Attribute* attr);
bool AttributeIdentityEqual(const Attribute* left, const Attribute* right,
                            bool ignore_namespace, bool ignore_arguments);

// Append an argument token (a copy is made).
void AttributeAddArg(Attribute* attr, const char* arg, size_t length);
size_t AttributeArgCount(Attribute* attr);
// Returns the raw argument token at index, or NULL if out of bounds.
const char* AttributeArgString(Attribute* attr, size_t index);
// Parses the argument at index as a base-10 integer.  Returns false (and
// leaves *value unchanged) if missing or not a valid integer.
bool AttributeArgInt(Attribute* attr, size_t index, long* value);
// The strictest alignment requested by the non-dependent `aligned` attributes
// (alignas, [[gnu::aligned]], __attribute__((aligned))) in `attrs`, 0 if
// none.  An `aligned` without an argument requests `bare_alignment`.
int AttributeListAlignment(const Vector* attrs, int bare_alignment);

// Operations on a Vector of Attribute*.
Attribute* AttributeListFind(const Vector* attrs, const char* name);
bool AttributeListHas(const Vector* attrs, const char* name);
void AttributeListDestruct(Vector* attrs);          // Frees contained Attribute*.
void AttributeListClone(Vector* dest,
                        const Vector* src);  // dest is initialized.

// A symbol's named-module attachment.  Records are interned and immutable, so
// symbols share them; set one with SymbolSetModuleIdentity.  Each field is ""
// when absent, never NULL.
typedef struct SymbolModuleIdentity {
  const char* owning_module_name;       // Named-module purview owner.
  const char* owning_module_partition;  // Owning partition, if any.
  const char* import_source_module;     // Import provenance for importers.
} SymbolModuleIdentity;

// Fields that few symbols ever set.  A symbol has no SymbolExtra until the
// first write through SymbolMutableExtra; until then SymbolExtraOf reads every
// field as its kEmptySymbolExtra default, which is zero unless noted.  Owned by
// the symbol.
typedef struct SymbolExtra {
  struct Symbol* alias_target;  // C++ using-declaration target. // @wire 38
  struct ASTNode* default_argument;  // C++ default arg, if any.  // @wire 40
  // Portable initializer for serialized constexpr aggregate values. The live
  // ConstexprObject cache in value.other is process-local and cannot be wired.
  struct ASTNode* constexpr_initializer;  // @wire 58
  struct VariableTemplate*
      variable_template;  // C++ variable template body. // @wire 44
  struct AliasTemplate*
      alias_template;  // C++ alias template parameters. // @wire 47
  // C++20 requires-clause for alias templates (`template<...> using A = ...`).
  // Function/class/variable templates store constraints on their type bodies;
  // alias templates have no other durable owner.
  struct ConstraintExpr* associated_constraint;  // @wire 46
  // Original entity represented by an implementation-generated lambda
  // capture field. Transient: closure fields never cross module boundaries.
  struct Symbol* lambda_capture_source;
  // Block-scope anonymous union/struct: injected member names alias storage in
  // `anonymous_union_host` at `anonymous_union_member_offset`.  Transient.
  struct Symbol* anonymous_union_host;
  int anonymous_union_member_offset;
  int alignment;  // aligned(N) override; 0 = natural.              // @wire 32
  // Template parameter index of a deferred dependent value; -1 if none.
  int dependent_value_template_parameter_index;  // @wire 34
  TemplateTemplateParameterKind template_template_parameter_kind;  // @wire 57
  // Structured-binding dependence marker and, for a pack, known cardinality:
  // -2 (the default) means materialized/not a binding, -1 means still
  // dependent, and a nonnegative value is the known pack size.  Transient.
  int structured_binding_pack_size;
  // See is_constexpr_representable.
  struct TypeRecord* constexpr_reference_scope;
  // Owned signature copy for a template-template parameter placeholder.
  Vector* template_template_parameters;  // TemplateParameter*.     // @wire 56
  struct Concept* concept_definition;  // Body when flags.is_concept. // @wire 45
  // Class that owns this static data member. Its out-of-line definition,
  // including the implicit constructor and destructor calls, is in that
  // class's scope. Transient: rebuilt when the class member is declared.
  struct Struct* static_data_member_class;  // @wire - (transient)
  // Static data member of a class template specialization: the template's
  // member it was instantiated from and the (owned) arguments used, so its
  // definition can be instantiated when it is first odr-used.  Transient.
  struct Symbol* static_data_member_pattern;  // @wire - (transient)
  Vector* static_data_member_template_arguments;  // @wire - (transient)
  // A class template's static data member: its out-of-class definition
  // (`template <class T> const T C<T>::k[] = {...};`), owned by the
  // translation unit's declaration list.  Transient.
  struct ASTNode* static_data_member_template_definition;  // @wire - (transient)
  // Names of the local classes defined in this function's body, in order of
  // definition (owned String*), for their mangling discriminators.  Transient.
  Vector* local_class_names;  // @wire - (transient)
  // Compiler-owned copy of a deserialized function template's parameter list.
  // Imported module symbols can have their live parameter vector cleared while
  // pending instantiations are compiled; this backup restores completion.
  Vector imported_function_template_parameters_backup;  // transient
} SymbolExtra;

// State only emitted symbols have.  Owned by the symbol.
typedef struct SymbolCodegen {
  struct DIE* die;  // Debug-info entry.
  // Lazily computed, full (untruncated) target assembly symbol name, reused
  // across every emission site so that long mangled names (e.g. deeply nested
  // template specializations) are never truncated to a caller's fixed-size
  // scratch buffer, which would let distinct symbols collide.
  char* target_symbol_name;
} SymbolCodegen;

// A symbol.  This is a variable, function or type used in a program.
// Module-serialization field numbers (see
// c_compiler/serialize/symbol_serialize.c).  Transient/codegen fields
// (usage_info, codegen) are recomputed on load and carry no wire number.
// Fields are ordered to avoid padding; there are hundreds of thousands of
// symbols in a large C++ translation unit.
typedef struct Symbol {
  String name;                // Symbol name.                     // @wire 1
  // Optional asm-visible name.  Read through SymbolAsmName and write through
  // SymbolSetAsmName, which handle deferred C++ mangling.
  String asm_name_;              // @wire 2
  struct Namespace* namespace_;  // Owning C++ namespace, if any. // @wire 3
  struct TypeRecord* type;    // Type.                            // @wire 5
  int id;                                                         // @wire 4
  uint8_t storage;  // Storage (static, typedef, etc.)  // @wire 6
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
    // C++26 [basic.scope.scope] name-independent declaration (`_`).
    bool is_name_independent: 1;                                    // @wire 59
    // Set on every declaration that ordinary lookup may select after a second
    // name-independent declaration of the same name enters the same scope.
    bool name_independent_lookup_ambiguous: 1;                       // @wire 60
    // A member-pointer constant's value.other is the referenced StructMember
    // rather than value.ivalue being its encoding.
    bool value_is_pointed_member: 1;                                 // @wire -
  } flags;
  // Transient: none of these cross a module boundary.
  // The implementation-generated lambda capture field (lambda_capture_source)
  // captures by reference.
  bool lambda_capture_by_reference : 1;
  // C++26 symbolic constexpr reference/pointer whose initializer designates an
  // object relative to the current function frame (constexpr_reference_scope).
  // Automatic-storage identities cannot cross a module boundary.
  bool is_constexpr_representable : 1;
  bool requires_ast_constexpr : 1;
  // Deserialized module symbols are graph-owned until the importing compiler
  // has finished tearing down structs/types that may still point at them.
  bool is_imported_module_symbol : 1;
  bool destruction_complete : 1;
  // Set when the symbol's value is read (used in any context other than as the
  // pure left-hand side of a plain assignment).  Combined with flags.used this
  // distinguishes "referenced but only written" objects for
  // -Wunused-but-set-variable.  Recomputed during analysis.
  bool is_read : 1;
  // The hidden object a structured binding declaration decomposes.
  bool is_structured_binding_object : 1;
  // Local object selected for named return-value optimization.  Semantic
  // analysis sets this before IR generation so every reference to the pooled
  // variable uses the hidden struct-return address.
  bool is_nrvo : 1;
  // SymbolAsmName mangles the name on first read
  // (SymbolDeferCXXMangledAsmName).
  bool asm_name_deferred : 1;
  // A CXXLinkageKind: C++20 external/internal/module.  // @wire 48
  unsigned cxx_linkage : 2;

  struct {
    int used_in_loop;             // Times used in loop.
    int reads;                    // Number of reads.
  } usage_info;                   // @wire - (transient, not serialized)
  int template_parameter_index;  // Template parameter index.      // @wire 33

  // Owned Attribute*; NULL until one is added.  Read through SymbolAttributes
  // and add through SymbolAddAttribute.
  Vector* attributes_;                                             // @wire 42
  SourceLocation location;                                         // @wire 35

  // Symbol value, one of these.  Serialized as the raw 64-bit slot.
  union {
    int64_t ivalue;         // Integer value for constants.      // @wire 36
    double fvalue;          // Double value for constants.       // (via 36)
    int32_t arg_number;     // Argument number in prototype.     // (via 36)
    void* other;            // Something else.                   // @wire -
    struct Symbol* func_defn;      // Defn of this func decl.     // @wire -
  } value;
  int32_t stack_offset;  // Stack offset if local.            // @wire 37
  // For a function template specialization, the template instantiation depth
  // that requested it.  Its deferred body is analyzed at that depth, so
  // instantiations it triggers nest under it.  Transient.
  int instantiation_depth;        // @wire - (transient)
  struct Symbol* overload_next;   // Next overload, same name.     // @wire 39
  // Interned and shared; NULL when attached to no module and not imported.
  const SymbolModuleIdentity* module_identity;  // @wire 49, 50, 51
  // Class whose member list holds this function's overload set.  Usually the
  // same as the function type's cxx_member_owner.  A using-declaration clone
  // keeps the base function's type (and therefore the base as
  // cxx_member_owner) while the derived class may add further overloads
  // (`using CordRep::Unref` plus `Unref(Span)`).  Transient.
  struct Struct* member_lookup_class;  // @wire - (transient)
  // Code generation and debug-info state; NULL until one is set.  Transient.
  SymbolCodegen* codegen;  // @wire - (transient)
  // Rarely set fields; NULL until one is written.  Use SymbolExtraOf to read
  // and SymbolMutableExtra to write.
  SymbolExtra* extra;
} Symbol;

extern const SymbolExtra kEmptySymbolExtra;

// The symbol's rarely set fields, all zero if none has been written.
static inline const SymbolExtra* SymbolExtraOf(const Symbol* symbol) {
  return symbol->extra != NULL ? symbol->extra : &kEmptySymbolExtra;
}

// The symbol's rarely set fields for writing, allocated on first use.
SymbolExtra* SymbolMutableExtra(Symbol* symbol);

// The symbol's aligned(N) override; 0 for natural alignment.
static inline int SymbolAlignment(const Symbol* symbol) {
  return SymbolExtraOf(symbol)->alignment;
}
void SymbolSetAlignment(Symbol* symbol, int alignment);

static inline struct DIE* SymbolDIE(const Symbol* symbol) {
  return symbol->codegen != NULL ? symbol->codegen->die : NULL;
}
void SymbolSetDIE(Symbol* symbol, struct DIE* die);

// The cached full target assembly name, or NULL if not computed yet.
static inline const char* SymbolCachedTargetName(const Symbol* symbol) {
  return symbol->codegen != NULL ? symbol->codegen->target_symbol_name : NULL;
}
// Takes ownership of `name` (heap-allocated) and frees the previous one; NULL
// clears the cache.
void SymbolSetCachedTargetName(Symbol* symbol, char* name);

extern const Vector kNoSymbolAttributes;

// The symbol's attributes (Attribute*), empty if it has none.
static inline const Vector* SymbolAttributes(const Symbol* symbol) {
  return symbol->attributes_ != NULL ? symbol->attributes_
                                     : &kNoSymbolAttributes;
}

// Gives `to` the lambda capture source of `from` without allocating extra
// fields for `to` when there is nothing to copy.
static inline void SymbolCopyLambdaCaptureSource(Symbol* to,
                                                 const Symbol* from) {
  Symbol* source = SymbolExtraOf(from)->lambda_capture_source;
  if (source != NULL || to->extra != NULL) {
    SymbolMutableExtra(to)->lambda_capture_source = source;
  }
}

// Gives `to` the using-declaration target of `from`, likewise.
static inline void SymbolCopyAliasTarget(Symbol* to, const Symbol* from) {
  Symbol* target = SymbolExtraOf(from)->alias_target;
  if (target != NULL || to->extra != NULL) {
    SymbolMutableExtra(to)->alias_target = target;
  }
}

static inline int SymbolDependentValueTemplateParameterIndex(
    const Symbol* symbol) {
  return SymbolExtraOf(symbol)->dependent_value_template_parameter_index;
}
void SymbolSetDependentValueTemplateParameterIndex(Symbol* symbol, int index);

static inline int SymbolStructuredBindingPackSize(const Symbol* symbol) {
  return SymbolExtraOf(symbol)->structured_binding_pack_size;
}
void SymbolSetStructuredBindingPackSize(Symbol* symbol, int size);

// Gives a new `to` a clone of `from`'s default argument, likewise.
void SymbolCloneDefaultArgument(Symbol* to, const Symbol* from);

void SymbolInit(Symbol* sym, const char* name, struct TypeRecord* type,
                Storage storage);
Symbol* NewSymbol(const char* name, struct TypeRecord* type, Storage storage);
void SymbolDelete(Symbol* symbol);
void SymbolDestruct(Symbol* symbol);

Symbol* SymbolClone(Symbol* sym);

void SymbolSetType(Symbol* symbol, struct TypeRecord* type);
void SymbolSetCXXMangledAsmName(Symbol* symbol);
// Like SymbolSetCXXMangledAsmName, but mangles only when the name is first
// read.  Most implicit special members are never odr-used, so most of their
// long mangled names are never needed.
void SymbolDeferCXXMangledAsmName(Symbol* symbol);
void SymbolMaterializeDeferredAsmName(Symbol* symbol);

// The asm-visible name; empty when the symbol has none.  Edits through the
// result are safe since any deferred name has already been mangled.
static inline String* SymbolAsmName(Symbol* symbol) {
  if (symbol->asm_name_deferred) {
    SymbolMaterializeDeferredAsmName(symbol);
  }
  return &symbol->asm_name_;
}
static inline bool SymbolHasAsmName(Symbol* symbol) {
  return SymbolAsmName(symbol)->length != 0;
}
// The asm-visible name if there is one, otherwise the source name.
static inline const char* SymbolAsmNameOrName(Symbol* symbol) {
  return SymbolHasAsmName(symbol) ? symbol->asm_name_.value
                                  : symbol->name.value;
}
// These replace any name, including a deferred one, without mangling it.
void SymbolSetAsmName(Symbol* symbol, String* name);
void SymbolSetAsmNameCString(Symbol* symbol, const char* name);
void SymbolClearAsmName(Symbol* symbol);
void SymbolBackupImportedFunctionTemplateParameters(Symbol* symbol);
void SymbolRestoreImportedFunctionTemplateParameters(Symbol* symbol);
// Appends the Itanium-style mangled encoding of `type` to `out`.  Used to form
// canonical, stable keys/symbol names for RTTI type_info objects.
void AppendCXXMangledTypeName(String* out, struct TypeRecord* type);
void SymbolSetCXXDataAsmName(Symbol* symbol, struct Struct* owner);
Symbol* GetCXXTemplateParameterObject(struct TemplateArgument* argument);

// Adds attribute and takes ownership of the Attribute.
void SymbolAddAttribute(Symbol* symbol, Attribute* attribute);
// Adds the Attribute* entries of `attributes`, taking ownership of the entries
// but not of the vector.
void SymbolAppendAttributes(Symbol* symbol, const Vector* attributes);
// Replaces the symbol's attributes with copies of `attributes`.
void SymbolSetAttributeCopies(Symbol* symbol, const Vector* attributes);
bool SymbolHasAttribute(Symbol* symbol, const char* attribute);
// Returns the attribute with the given (normalized) name, or NULL.
Attribute* SymbolFindAttribute(Symbol* symbol, const char* attribute);
bool SymbolHasWeakBinding(Symbol* symbol);
ValueState SymbolInitialValueState(Symbol* symbol);

void SymbolPrintDetails(Symbol* sym, bool with_function_body, FILE* fp);
void SymbolPrint(Symbol* sym, FILE* fp);

// Sets the module identity fields.  NULL or "" means absent.
void SymbolSetModuleIdentity(Symbol* symbol, const char* owning_module_name,
                             const char* owning_module_partition,
                             const char* import_source_module);

static inline const char* SymbolOwningModuleName(const Symbol* symbol) {
  return symbol->module_identity != NULL
             ? symbol->module_identity->owning_module_name
             : "";
}

static inline const char* SymbolOwningModulePartition(const Symbol* symbol) {
  return symbol->module_identity != NULL
             ? symbol->module_identity->owning_module_partition
             : "";
}

static inline const char* SymbolImportSourceModule(const Symbol* symbol) {
  return symbol->module_identity != NULL
             ? symbol->module_identity->import_source_module
             : "";
}

static inline bool SymbolHasOwningModule(const Symbol* symbol) {
  return SymbolOwningModuleName(symbol)[0] != '\0';
}

static inline bool SymbolIsImportedFromModule(const Symbol* symbol) {
  return SymbolImportSourceModule(symbol)[0] != '\0';
}

#endif /* symbol_h */
