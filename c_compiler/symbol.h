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

struct TypeRecord;

// Storage for symbol (where it is located in memory).
typedef enum {
  kStorageImplicit,      // Implied.
  kStorageAuto,          // Automatic (on stack).
  kStorageStatic,        // Static (in data segment)
  kStorageTypedef,       // Type definition.
  kStorageExtern,        // External (not defined in this program).
  kStorageRegister,      // In a register (not used in modern C).
  kStorageAssembler,     // Assembler label.
} Storage;

// A symbol.  This is a variable, function or type used in a program.
typedef struct Symbol {
  String name;                // Symbol name.
  struct TypeRecord* type;    // Type.
  Storage storage;            // Storage (static, typedef, etc.)
  bool is_defined;            // Symbol is defined.
  bool is_forward_declared;   // Symbol is forward declared.
  bool is_local;              // Local symbol.
  bool is_argument;           // Defined in function prototype.
  bool is_temp;               // Temporary (invented).
  bool address_taken;         // The address has been taken in the program.
  bool used;                  // The symbol has been used.
  // Symbol value, one of these.
  union {
    int64_t ivalue;         // Integer value for constants.
    double fvalue;          // Double value for constants.
    int32_t arg_number;     // Argument number in prototype.
    void* other;            // Something else.
  } value;
  
  int32_t stack_offset;     // Stack offset if local.
} Symbol;

void SymbolInit(Symbol* sym, const char* name, struct TypeRecord* type,
                Storage storage);
Symbol* NewSymbol(const char* name, struct TypeRecord* type, Storage storage);
void SymbolDelete(Symbol* symbol);
void SymbolDestruct(Symbol* symbol);

void SymbolSetType(Symbol* symbol, struct TypeRecord* type);

void SymbolPrintDetails(Symbol* sym, bool with_function_body);
void SymbolPrint(Symbol* sym);

#endif /* symbol_h */
