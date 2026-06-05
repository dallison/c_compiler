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
struct DIE;

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

// A symbol.  This is a variable, function or type used in a program.
typedef struct Symbol {
  String name;                // Symbol name.
  int id;
  struct TypeRecord* type;    // Type.
  Storage storage;            // Storage (static, typedef, etc.)
  struct {
    bool is_defined: 1;            // Symbol is defined.
    bool is_tentative_decl: 1;     // Tentative declaration.
    bool is_forward_declared: 1;   // Symbol is forward declared.
    bool is_local: 1;              // Local symbol.
    bool is_argument: 1;           // Defined in function prototype.
    bool is_temp: 1;               // Temporary (invented).
    bool address_taken: 1;         // The address has been taken in the program.
    bool used: 1;                  // The symbol has been used.
    bool invented: 1;
    bool is_inline_defn: 1;        // Is an inline function definition.
    bool value_set : 1;            // Value has been set (for const).
  } flags;
  
  struct {
    int used_as_arg;              // Times used as function arg.
    int used_in_loop;             // Times used in loop.
    int reads;                    // Number of reads.
  } usage_info;
  
  Vector attributes;          // Attributes (owns String*).
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
  struct DIE* die;
} Symbol;


void SymbolInit(Symbol* sym, const char* name, struct TypeRecord* type,
                Storage storage);
Symbol* NewSymbol(const char* name, struct TypeRecord* type, Storage storage);
void SymbolDelete(Symbol* symbol);
void SymbolDestruct(Symbol* symbol);

Symbol* SymbolClone(Symbol* sym);

void SymbolSetType(Symbol* symbol, struct TypeRecord* type);

// Adds attribute and takes ownership of the String.
void SymbolAddAttribute(Symbol* symbol, String* attribute);
bool SymbolHasAttribute(Symbol* symbol, const char* attribute);

void SymbolPrintDetails(Symbol* sym, bool with_function_body, FILE* fp);
void SymbolPrint(Symbol* sym, FILE* fp);

#endif /* symbol_h */
