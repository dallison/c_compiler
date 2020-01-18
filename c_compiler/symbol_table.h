//
//  symbol_table.h
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

// Symbol tables.

#ifndef symbol_table_h
#define symbol_table_h

#include "hashtable.h"
#include "symbol.h"
#include "binary_tree.h"

// A SymbolNode is a symbol held in a symbol table.  Symbol
// tables are simple binary trees of SymbolNodes.  The
// nodes to the left of a particular parent node are lexically
// less than the parent node based on symbol name.
typedef struct SymbolNode {
  BinaryTreeNode header;
  Symbol* symbol;
} SymbolNode;

// Local symbol tables are arranged in a stack where the top
// of the stack is the innermost scope.  The LocalSymbolTable
// structs are pushed onto the stack when a new scope is entered
// and popped off when it exits.
typedef struct LocalSymbolTable {
  BinaryTree table;
  struct LocalSymbolTable* prev;
} LocalSymbolTable;

// Create the global symbol tables.
void CreateGlobalSymbolTables(void);

// Create a new local symbol table.
LocalSymbolTable* NewLocalSymbolTable(void);
void LocalSymbolTableDelete(LocalSymbolTable* table);

void ClearSymbolTable(HashTable* table, bool delete_symbols);

// Allcoates a new symbol node with the given symbol
SymbolNode* NewSymbolNode(Symbol* symbol);

// Deletes a symbol node and all its children.
void SymbolNodeDelete(SymbolNode* node);

// Inserts a symbol into the given symbol table.  Returns true
// if the insertion was successful.
bool InsertSymbol(BinaryTree* table, SymbolNode* node, SymbolNode** parent);

// Finds a symbol name in the given symbol table.  Returns NULL if it can't
// be found.
Symbol* FindSymbol(BinaryTree* table, String* name);

// Inserts a symbol into the global symbol table.  Returns true if insertion
// was successful.
bool InsertGlobalSymbol(Symbol* symbol);

// Inserts a tag into the global tag table.  Returns true if insertion
// was successful.
bool InsertGlobalTag(Symbol* symbol);

// Finds a global symbol.  Returns NULL if not found.
Symbol* FindGlobalSymbol(String* name);

// Finds a global tag.  Returns NULL if not found.
Symbol* FindGlobalTag(String* name);

// Inserts a symbol into the local symbol table.  Returns true if the
// insertion was successful.
bool InsertLocalSymbol(LocalSymbolTable* table, Symbol* symbol);

// Finds a local symbol by searching all tables in the local symbol
// table stack.  Returns NULL if not found.
Symbol* FindLocalSymbol(LocalSymbolTable* table, String* name);

// Finds a local symbol by searching only the top scope local symbol
// table.  Returns NULL if not found.
Symbol* FindTopLocalSymbol(LocalSymbolTable* table, String* name);

#endif /* symbol_table_h */
