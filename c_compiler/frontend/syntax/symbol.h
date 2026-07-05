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
} VariableTemplate;

// A parsed __attribute__((...)) clause: a name with optional argument tokens.
// e.g. "aligned(16)"          -> name "aligned", args ["16"]
//      "format(printf, 1, 2)" -> name "format",  args ["printf", "1", "2"]
//      "packed"               -> name "packed",  args []
// The name is normalized by stripping a surrounding "__" pair so that
// "__packed__" and "packed" compare equal (as GCC does).
typedef struct Attribute {
  String name;   // Normalized attribute name.
  Vector args;   // Vector of String* argument tokens (owns String*).
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
typedef struct Symbol {
  String name;                // Symbol name.
  String asm_name;            // Optional assembler-visible name.
  struct Namespace* namespace_;  // C++ namespace owning the symbol, if any.
  int id;
  struct TypeRecord* type;    // Type.
  Storage storage;            // Storage (static, typedef, etc.)
  struct {
    bool is_defined: 1;            // Symbol is defined.
    bool is_tentative_decl: 1;     // Tentative declaration.
    bool is_forward_declared: 1;   // Symbol is forward declared.
    bool is_local: 1;              // Local symbol.
    bool is_block_scope: 1;        // Declared at block (local) scope.
    bool is_argument: 1;           // Defined in function prototype.
    bool is_temp: 1;               // Temporary (invented).
    bool address_taken: 1;         // The address has been taken in the program.
    bool used: 1;                  // The symbol has been used.
    bool invented: 1;
    bool is_inline_defn: 1;        // Is an inline function definition.
    bool value_set : 1;            // Value has been set (for const).
    bool noreturn: 1;              // __attribute__((noreturn)) / _Noreturn.
    bool always_inline: 1;         // __attribute__((always_inline)).
    bool noinline: 1;              // __attribute__((noinline)).
    bool is_using_alias: 1;        // C++ using-declaration alias.
    bool is_overloaded: 1;         // Has C++ overload alternatives.
    bool is_template: 1;           // C++ template declaration.
    bool is_template_parameter: 1; // C++ template parameter.
    bool is_template_type_parameter: 1; // `typename`/`class` parameter.
    bool is_parameter_pack: 1;     // C++ template or function parameter pack.
    bool is_constexpr: 1;          // C++ constexpr variable.
    bool is_constinit: 1;          // C++ constinit variable.
    bool is_weak: 1;               // Emits ELF weak binding.
    bool is_c_linkage: 1;          // Declared with C language linkage (extern "C").
    bool is_concept: 1;            // C++20 concept definition (see concept_definition).
  } flags;
  
  struct {
    int used_as_arg;              // Times used as function arg.
    int used_in_loop;             // Times used in loop.
    int reads;                    // Number of reads.
  } usage_info;
  
  Vector attributes;          // Attributes (owns Attribute*).
  int alignment;              // __attribute__((aligned(N))) override; 0 = natural.
  int template_parameter_index;  // Index for template parameter symbols.
  int dependent_value_template_parameter_index;  // Deferred non-type arg value.
  SourceLocation location;
  
  // Symbol value, one of these.
  union {
    int64_t ivalue;         // Integer value for constants.
    double fvalue;          // Double value for constants.
    int32_t arg_number;     // Argument number in prototype.
    void* other;            // Something else.
    struct Symbol* func_defn;      // Defintion of this func declaration.
  } value;
  int32_t stack_offset;     // Stack offset if local.
  struct Symbol* alias_target;  // Target for a C++ using-declaration alias.
  struct Symbol* overload_next;  // Next C++ overload with the same source name.
  struct ASTNode* default_argument;  // C++ default function argument, if any.
  struct VariableTemplate* variable_template;  // C++ variable template body.
  struct Concept* concept_definition;  // C++20 concept body when flags.is_concept.
  struct DIE* die;
} Symbol;


void SymbolInit(Symbol* sym, const char* name, struct TypeRecord* type,
                Storage storage);
Symbol* NewSymbol(const char* name, struct TypeRecord* type, Storage storage);
void SymbolDelete(Symbol* symbol);
void SymbolDestruct(Symbol* symbol);

Symbol* SymbolClone(Symbol* sym);

void SymbolSetType(Symbol* symbol, struct TypeRecord* type);
void SymbolSetCXXMangledAsmName(Symbol* symbol);
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
