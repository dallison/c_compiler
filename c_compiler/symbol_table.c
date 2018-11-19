//
//  symbol_table.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "symbol_table.h"
#include "compiler.h"

#include <stdlib.h>

// Global symbol tables are fixed size hash tables.  These are their
// sizes.
#define GLOBAL_SYMBOL_TABLE_SIZE 1009
#define GLOBAL_TAG_TABLE_SIZE 101

void DeleteSymbolTree(SymbolNode* tree, bool delete_symbols) {
  if (tree == NULL) {
    return;
  }
  DeleteSymbolTree(tree->left, delete_symbols);
  DeleteSymbolTree(tree->right, delete_symbols);
  if (delete_symbols) {
    SymbolDelete(tree->symbol);
  }
  free(tree);
}

// Clear a symbol table, optionally deleting the symbols.
void ClearSymbolTable(HashTable* table, bool delete_symbols) {
  for (size_t i = 0; i < table->size; i++) {
    DeleteSymbolTree((SymbolNode*)table->entries[i], delete_symbols);
  }
  HashTableClear(table);
}

LocalSymbolTable* NewLocalSymbolTable() {
  LocalSymbolTable* table = malloc(sizeof(LocalSymbolTable));
  table->table = NULL;
  table->prev = NULL;
  return table;
}

void LocalSymbolTableDelete(LocalSymbolTable* table) {
  // Clear the symbol table but don't delete the symbols.
  DeleteSymbolTree(table->table, false);
  free(table);
}

bool InsertGlobalSymbol(Symbol* symbol) {
  SymbolNode* node = NewSymbolNode(symbol);
  bool ok = HashTableInsert(&compiler->global_symbol_table, node);
  if (!ok) {
    // Insertion unsuccessful.  Don't need the SymbolNode any more.
    free(node);
  }
  return ok;
}

bool InsertGlobalTag(Symbol* symbol) {
  SymbolNode* node = NewSymbolNode(symbol);
  bool ok = HashTableInsert(&compiler->global_tag_table, node);
  if (!ok) {
    // Insertion unsuccessful.  Don't need the SymbolNode any more.
    free(node);
  }
  return ok;
}

Symbol* FindGlobalSymbol(String* name) {
  return HashTableSearch(&compiler->global_symbol_table, name);
}

Symbol* FindGlobalTag(String* name) {
  return HashTableSearch(&compiler->global_tag_table, name);
}

bool InsertLocalSymbol(LocalSymbolTable* table, Symbol* symbol) {
  SymbolNode* node = NewSymbolNode(symbol);
  bool ok = InsertSymbol(table->table, node, &table->table);
  if (!ok) {
    free(node);
  }
  return ok;
}

Symbol* FindLocalSymbol(LocalSymbolTable* table, String* name) {
  while (table != NULL) {
    Symbol* symbol = FindSymbol(table->table, name);
    if (symbol != NULL) {
      return symbol;
    }
    table = table->prev;
  }
  return NULL;
}

Symbol* FindTopLocalSymbol(LocalSymbolTable* table, String* name) {
  return FindSymbol(table->table, name);
}

SymbolNode* NewSymbolNode(Symbol* symbol) {
  SymbolNode* node = malloc(sizeof(SymbolNode));
  node->symbol = symbol;
  node->left = NULL;
  node->right = NULL;
  return node;
}

void SymbolNodeDelete(SymbolNode* node) {
  if (node->left != NULL) {
    SymbolNodeDelete(node->left);
  }
  if (node->right != NULL) {
    SymbolNodeDelete(node->right);
  }
  free(node->symbol);
}

// Inserts a symbol in the given table using a recursive binary
// insertion algorithm.  The SymbolNode is inserted either to the
// left or right of a parent depending on is lexographical order.  If an
// existing symbol is found the insertion is aborted and the function
// returns false.   The 'parent' pointer is a pointer to the address of the
// left or right pointers inside the node.  This says where to insert the
// new node when we reach the leaf node.
bool InsertSymbol(SymbolNode* table, SymbolNode* node, SymbolNode** parent) {
  if (table == NULL) {
    *parent = node;
    return true;
  } else {
    int comp = StringCompareString(&node->symbol->name, &table->symbol->name);
    if (comp == 0) {
      // Duplicate symbol.
      return false;
    }
    if (comp < 0) {
      return InsertSymbol(table->left, node, &table->left);
    }
    return InsertSymbol(table->right, node, &table->right);
  }
}

// Mapping function for hash table inserter.
static bool InsertSymbolIntoHashTable(void* table, void* node, void** parent) {
  return InsertSymbol(table, node, (SymbolNode**)parent);
}

// Find a symbol given its name in the given symbol table.  This
// searches the binary tree using a recursive algorithm.
Symbol* FindSymbol(SymbolNode* table, String* name) {
  if (table == NULL) {
    return NULL;
  }
  int comp = StringCompareString(name, &table->symbol->name);
  if (comp == 0) {
    return table->symbol;
  }
  if (comp < 0) {
    return FindSymbol(table->left, name);
  }
  return FindSymbol(table->right, name);
}

// Mapping function for hash table searcher.
static void* FindSymbolInHashTable(void* table, void* value) {
  return FindSymbol(table, value);
}

// Create a hash value from a given symbol node (passed as void* from
// hash table inserter and searcher functions.
static size_t HashSymbol(void* value, HashTable* table, HashMode mode) {
  String* name;
  switch (mode) {
    case kHashInsert:
      // For insertion we have a pointer to symbol node.
      name = &((SymbolNode*)value)->symbol->name;
      break;
    case kHashSearch:
      // For search we have pointer to a String containing the name
      // to find.
      name = (String*)value;
      break;
  }
  size_t hash = 0;
  for (size_t i = 0; i < name->length; i++) {
    hash = (hash << 1) ^ name->value[i];
  }
  return hash;
}

void CreateGlobalSymbolTables() {
  HashTableInit(&compiler->global_symbol_table, "global-symbol-table",
                GLOBAL_SYMBOL_TABLE_SIZE, HashSymbol, InsertSymbolIntoHashTable,
                FindSymbolInHashTable);
  HashTableInit(&compiler->global_tag_table, "global-tag-table",
                GLOBAL_TAG_TABLE_SIZE, HashSymbol, InsertSymbolIntoHashTable,
                FindSymbolInHashTable);
}
