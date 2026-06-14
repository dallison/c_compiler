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

static int anonymous_namespace_id = 0;

static int SymbolNodeInsertCompare(BinaryTreeNode* node1,
                                   BinaryTreeNode* node2) {
  SymbolNode* sym1 = (SymbolNode*)node1;
  SymbolNode* sym2 = (SymbolNode*)node2;
  return StringCompareString(&sym1->symbol->name, &sym2->symbol->name);
}

static int SymbolNodeSearchCompare(BinaryTreeNode* node, void* name) {
  SymbolNode* sym = (SymbolNode*)node;
  return StringCompareString(&sym->symbol->name, name);
}

static void SymbolNodeDestructor(BinaryTreeNode* node, void* delete_symbols) {
  if (delete_symbols != NULL) {
    SymbolNode* sym = (SymbolNode*)node;
    SymbolDelete(sym->symbol);
  }
}

static void DeleteSymbolTable(void* table, void* data) {
  BinaryTreeDestruct(table, data);
  free(table);
}

// Clear a symbol table, optionally deleting the symbols.
void ClearSymbolTable(HashTable* table, bool delete_symbols) {
  HashTableTraverse(table, DeleteSymbolTable, (void*)delete_symbols);
  HashTableClear(table);
}

LocalSymbolTable* NewLocalSymbolTable() {
  LocalSymbolTable* table = malloc(sizeof(LocalSymbolTable));
  BinaryTreeInit(&table->table,
                 SymbolNodeInsertCompare,
                 SymbolNodeSearchCompare,
                 SymbolNodeDestructor);
  table->prev = NULL;
  return table;
}

static void PrintSymbolNode(SymbolNode* node, int indent) {
  for (int i = 0; i < indent * 2; i++) {
    printf("%s", " ");
  }
  SymbolNode* parent = (SymbolNode*)node->header.parent;
  printf("%s: %s (%s)\n", node->symbol->name.value,
         node->header.color == kBinaryTreeNodeRed ? "RED" : "BLACK",
         parent == NULL ? "" : parent->symbol->name.value);
}

static COMPILER_UNUSED void Printer(BinaryTreeNode* node, int depth, void* data) {
  PrintSymbolNode((SymbolNode*)node, depth);
}


void LocalSymbolTableDelete(LocalSymbolTable* table) {
  // Clear the symbol table but don't delete the symbols.
  // BinaryTreeTraverse(&table->table, Printer, NULL);

  BinaryTreeDestruct(&table->table, NULL);
  free(table);
}

static void InitSymbolTree(BinaryTree* tree) {
  BinaryTreeInit(tree,
                 SymbolNodeInsertCompare,
                 SymbolNodeSearchCompare,
                 SymbolNodeDestructor);
}

Namespace* NewNamespace(const char* name, Namespace* parent, bool is_anonymous) {
  Namespace* ns = malloc(sizeof(Namespace));
  StringInit(&ns->name, name);
  StringInit(&ns->qualified_name, NULL);
  ns->is_anonymous = is_anonymous;
  InitSymbolTree(&ns->symbol_table);
  InitSymbolTree(&ns->tag_table);
  VectorInit(&ns->children);
  ns->parent = parent;
  ns->anonymous_child = NULL;

  if (parent == NULL || parent->qualified_name.length == 0) {
    StringSet(&ns->qualified_name, name);
  } else if (name[0] == '\0') {
    StringSetString(&ns->qualified_name, &parent->qualified_name);
  } else {
    StringPrintf(&ns->qualified_name, "%s::%s",
                 parent->qualified_name.value, name);
  }
  return ns;
}

void NamespaceDelete(Namespace* ns) {
  if (ns == NULL) {
    return;
  }
  for (size_t i = 0; i < ns->children.length; i++) {
    NamespaceDelete((Namespace*)ns->children.value.p[i]);
  }
  VectorDestruct(&ns->children);
  BinaryTreeDestruct(&ns->symbol_table, (void*)true);
  BinaryTreeDestruct(&ns->tag_table, (void*)true);
  StringDestruct(&ns->name);
  StringDestruct(&ns->qualified_name);
  free(ns);
}

Namespace* NamespaceFindChild(Namespace* parent, String* name) {
  for (size_t i = 0; i < parent->children.length; i++) {
    Namespace* child = parent->children.value.p[i];
    if (!child->is_anonymous && StringEqualString(&child->name, name)) {
      return child;
    }
  }
  return NULL;
}

Namespace* NamespaceFindOrCreateChild(Namespace* parent, String* name) {
  Namespace* child = NamespaceFindChild(parent, name);
  if (child != NULL) {
    return child;
  }
  child = NewNamespace(name->value, parent, false);
  VectorAppend(&parent->children, child);
  return child;
}

Namespace* NamespaceFindOrCreateAnonymousChild(Namespace* parent) {
  if (parent->anonymous_child != NULL) {
    return parent->anonymous_child;
  }
  String name;
  StringInit(&name, NULL);
  StringPrintf(&name, "__anonymous_namespace_%d", anonymous_namespace_id++);
  Namespace* child = NewNamespace(name.value, parent, true);
  StringDestruct(&name);
  parent->anonymous_child = child;
  VectorAppend(&parent->children, child);
  return child;
}

bool NamespaceInsertSymbol(Namespace* ns, Symbol* symbol) {
  symbol->namespace_ = ns;
  SymbolNode* node = NewSymbolNode(symbol);
  bool ok = BinaryTreeInsert(&ns->symbol_table, &node->header);
  if (!ok) {
    free(node);
    symbol->namespace_ = NULL;
  }
  return ok;
}

bool NamespaceInsertTag(Namespace* ns, Symbol* symbol) {
  symbol->namespace_ = ns;
  SymbolNode* node = NewSymbolNode(symbol);
  bool ok = BinaryTreeInsert(&ns->tag_table, &node->header);
  if (!ok) {
    free(node);
    symbol->namespace_ = NULL;
  }
  return ok;
}

Symbol* NamespaceFindSymbol(Namespace* ns, String* name) {
  if (ns == NULL) {
    return NULL;
  }
  return FindSymbol(&ns->symbol_table, name);
}

Symbol* NamespaceFindTag(Namespace* ns, String* name) {
  if (ns == NULL) {
    return NULL;
  }
  return FindSymbol(&ns->tag_table, name);
}

Symbol* NamespaceFindSymbolInScope(Namespace* ns, String* name) {
  while (ns != NULL) {
    Symbol* symbol = NamespaceFindSymbol(ns, name);
    if (symbol != NULL) {
      return symbol;
    }
    ns = ns->parent;
  }
  return NULL;
}

Symbol* NamespaceFindTagInScope(Namespace* ns, String* name) {
  while (ns != NULL) {
    Symbol* symbol = NamespaceFindTag(ns, name);
    if (symbol != NULL) {
      return symbol;
    }
    ns = ns->parent;
  }
  return NULL;
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
  bool ok = BinaryTreeInsert(&table->table, &node->header);
  if (!ok) {
    free(node);
  }
  return ok;
}

Symbol* FindLocalSymbol(LocalSymbolTable* table, String* name) {
  while (table != NULL) {
    Symbol* symbol = FindSymbol(&table->table, name);
    if (symbol != NULL) {
      return symbol;
    }
    table = table->prev;
  }
  return NULL;
}

Symbol* FindTopLocalSymbol(LocalSymbolTable* table, String* name) {
  return FindSymbol(&table->table, name);
}

SymbolNode* NewSymbolNode(Symbol* symbol) {
  SymbolNode* node = malloc(sizeof(SymbolNode));
  BinaryTreeNodeInit(&node->header);
  node->symbol = symbol;
  return node;
}

// Mapping function for hash table inserter.
static bool InsertSymbolIntoHashTable(void* table, void* node, void** parent) {
  BinaryTree* tree = table;
  if (table == NULL) {
    tree = NewBinaryTree(
                         SymbolNodeInsertCompare,
                         SymbolNodeSearchCompare,
                         SymbolNodeDestructor);

    *parent = tree;
  }
  return BinaryTreeInsert(tree, node);
}

// Find a symbol given its name in the given symbol table.  This
// searches the binary tree using a recursive algorithm.
Symbol* FindSymbol(BinaryTree* table, String* name) {
  SymbolNode* node = (SymbolNode*)BinaryTreeSearch(table, name);
  if (node == NULL) {
    return NULL;
  }
  return node->symbol;
}

// Mapping function for hash table searcher.
static void* FindSymbolInHashTable(void* table, void* value) {
  BinaryTree* tree = table;
  return FindSymbol(tree, value);
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
  const char* nm = name->value;
  uint32_t hash = 5381;
  while (*nm != '\0') {
    hash = (hash << 5) + hash + *nm++;
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
  anonymous_namespace_id = 0;
  compiler->global_namespace = NewNamespace("", NULL, false);
  compiler->syntax.current_namespace = compiler->global_namespace;
}

void DeleteGlobalNamespace() {
  NamespaceDelete(compiler->global_namespace);
  compiler->global_namespace = NULL;
  compiler->syntax.current_namespace = NULL;
}
