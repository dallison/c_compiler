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

// C++ namespace scope.  Namespace scopes form a tree rooted at the translation
// unit; the root represents global scope and does not own global C symbols.
// Module-serialization field numbers (see
// c_compiler/serialize/symbol_serialize.c).  The symbol_table/tag_table trees
// are flattened to symbol-handle lists on the wire (@wire 7 / @wire 8).
typedef struct Namespace {
  String name;                          // @wire 1
  String qualified_name;                // @wire 2
  bool is_anonymous;                    // @wire 3
  BinaryTree symbol_table;              // flattened -> @wire 7 (symbols)
  BinaryTree tag_table;                 // flattened -> @wire 8 (tags)
  Vector children;  // Namespace*, owned by this namespace.   // @wire 4
  struct Namespace* parent;             // @wire 5
  struct Namespace* anonymous_child;    // @wire 6
  bool is_inline;                       // @wire 9
} Namespace;

// Create the global symbol tables.
void CreateGlobalSymbolTables(void);
void DeleteGlobalNamespace(void);

Namespace* NewNamespace(const char* name, Namespace* parent, bool is_anonymous);
void NamespaceDelete(Namespace* ns);
Namespace* NamespaceFindChild(Namespace* parent, String* name);
// Direct named-child lookup for namespace definitions and reopening.  Does not
// treat inline namespaces differently from ordinary children.
Namespace* NamespaceFindDirectChild(Namespace* parent, String* name);
Namespace* NamespaceFindOrCreateChild(Namespace* parent, String* name);
// Opens or reopens a direct child for a namespace-definition.  Inline status
// is sticky across reopenings.  Sets *inline_conflict when `is_inline` is
// requested for an existing non-inline namespace in this translation unit.
Namespace* NamespaceFindOrReopenChild(Namespace* parent, String* name,
                                      bool is_inline, bool* inline_conflict);
Namespace* NamespaceFindOrCreateAnonymousChild(Namespace* parent);
// Opens or reopens the translation unit's reusable anonymous namespace child.
// Inline status is sticky; sets *inline_conflict when adding inline to an
// existing non-inline anonymous namespace in this translation unit.
Namespace* NamespaceFindOrReopenAnonymousChild(Namespace* parent, bool is_inline,
                                               bool* inline_conflict);
void NamespaceForEachInlineChild(Namespace* parent,
                                 void (*visit)(Namespace* child, void* ctx),
                                 void* ctx);
// Collect associated namespaces for ADL, including transitive inline children
// and enclosing-namespace relationships in both directions.
void NamespaceCollectADLAssociatedNamespaces(Namespace* ns, Vector* namespaces);
// Qualified namespace-name lookup: direct child, then descendants reachable
// through inline namespace members (transitive).  Not for namespace definitions.
Namespace* NamespaceFindChildForQualifiedLookup(Namespace* parent, String* name);
typedef enum {
  kInlineLookupNotFound,
  kInlineLookupUnique,
  kInlineLookupAmbiguous,
} InlineLookupStatus;

typedef struct {
  InlineLookupStatus status;
  Symbol* symbol;
} NamespaceInlineSymbolLookup;

typedef struct {
  InlineLookupStatus status;
  Symbol* tag;
} NamespaceInlineTagLookup;

typedef struct {
  InlineLookupStatus status;
  Namespace* child;
} NamespaceInlineChildLookup;

void NamespaceCollectSymbolHeadsInInlineSet(Namespace* ns, String* name,
                                            Vector* heads);
void NamespaceCollectTagHeadsInInlineSet(Namespace* ns, String* name,
                                           Vector* tags);
void NamespaceCollectFunctionSymbolsInInlineSet(Namespace* ns, String* name,
                                                Vector* functions);
NamespaceInlineSymbolLookup NamespaceResolveSymbolInInlineSet(Namespace* ns,
                                                              String* name);
NamespaceInlineTagLookup NamespaceResolveTagInInlineSet(Namespace* ns,
                                                        String* name);
NamespaceInlineChildLookup NamespaceResolveChildInInlineSet(Namespace* parent,
                                                            String* name);
Namespace* NamespaceFindStdNamespace(void);
// The namespace whose inline-transparent lookup set contains symbols declared in
// `declaring_ns` (walks out of inline/anonymous nested namespaces).
Namespace* NamespaceParentForInlineTransparentLookup(Namespace* declaring_ns);
// Unqualified member lookup treating inline namespace members as members of the
// enclosing namespace (transitive).  Returns the unique match when present.
Symbol* NamespaceLookupUnqualifiedSymbol(Namespace* ns, String* name);
Symbol* NamespaceLookupUnqualifiedTag(Namespace* ns, String* name);
// Walk enclosing namespaces from `ns`, using inline-transparent lookup at each
// level.  Does not search C file-scope global hash tables.
Symbol* NamespaceLookupSymbolInEnclosingScopes(Namespace* ns, String* name);
Symbol* NamespaceLookupTagInEnclosingScopes(Namespace* ns, String* name);
bool NamespaceInsertSymbol(Namespace* ns, Symbol* symbol);
bool NamespaceInsertTag(Namespace* ns, Symbol* symbol);
Symbol* NamespaceFindSymbol(Namespace* ns, String* name);
Symbol* NamespaceFindTag(Namespace* ns, String* name);
Symbol* NamespaceFindSymbolInScope(Namespace* ns, String* name);
Symbol* NamespaceFindTagInScope(Namespace* ns, String* name);

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
