//
//  symbol_table.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "symbol_table.h"
#include "compiler.h"
#include "type.h"

#include <stdlib.h>

// Global symbol tables are fixed size hash tables.  These are their
// sizes.
#define GLOBAL_SYMBOL_TABLE_SIZE 1009
#define GLOBAL_TAG_TABLE_SIZE 101

static int anonymous_namespace_id = 0;

static NamespaceAlias* NewNamespaceAlias(String* name, Namespace* target) {
  NamespaceAlias* alias = malloc(sizeof(NamespaceAlias));
  StringInit(&alias->name, name->value);
  alias->target = target;
  return alias;
}

static void NamespaceAliasDelete(NamespaceAlias* alias) {
  if (alias == NULL) {
    return;
  }
  StringDestruct(&alias->name);
  free(alias);
}

static void NamespaceAliasVectorDestruct(Vector* aliases) {
  for (size_t i = 0; i < aliases->length; i++) {
    NamespaceAliasDelete((NamespaceAlias*)VectorGet(aliases, i));
  }
  VectorDestruct(aliases);
}

static Namespace* FindNamespaceAliasInVector(Vector* aliases, String* name) {
  for (size_t i = 0; i < aliases->length; i++) {
    NamespaceAlias* alias = (NamespaceAlias*)VectorGet(aliases, i);
    if (alias != NULL && StringEqualString(&alias->name, name)) {
      return alias->target;
    }
  }
  return NULL;
}

static NamespaceAliasInsertResult InsertNamespaceAliasInVector(
    Vector* aliases, String* name, Namespace* target) {
  Namespace* existing = FindNamespaceAliasInVector(aliases, name);
  if (existing != NULL) {
    return existing == target ? kNamespaceAliasRedeclared
                              : kNamespaceAliasConflict;
  }
  VectorAppend(aliases, NewNamespaceAlias(name, target));
  return kNamespaceAliasInserted;
}

static int SymbolNodeInsertCompare(BinaryTreeNode* node1,
                                   BinaryTreeNode* node2) {
  SymbolNode* sym1 = (SymbolNode*)node1;
  SymbolNode* sym2 = (SymbolNode*)node2;
  return StringCompareString(&sym1->name, &sym2->name);
}

static int SymbolNodeSearchCompare(BinaryTreeNode* node, void* name) {
  SymbolNode* sym = (SymbolNode*)node;
  return StringCompareString(&sym->name, name);
}

static void SymbolNodeDestructor(BinaryTreeNode* node, void* delete_symbols) {
  SymbolNode* sym = (SymbolNode*)node;
  if (delete_symbols != NULL) {
    SymbolDelete(sym->symbol);
  }
  StringDestruct(&sym->name);
}

void SymbolNodeDelete(SymbolNode* node) {
  if (node == NULL) {
    return;
  }
  StringDestruct(&node->name);
  free(node);
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
  VectorInit(&table->namespace_aliases);
  table->prev = NULL;
  return table;
}

static void PrintSymbolNode(SymbolNode* node, int indent) {
  for (int i = 0; i < indent * 2; i++) {
    printf("%s", " ");
  }
  SymbolNode* parent = (SymbolNode*)node->header.parent;
  printf("%s: %s (%s)\n", node->name.value,
         node->header.color == kBinaryTreeNodeRed ? "RED" : "BLACK",
         parent == NULL ? "" : parent->name.value);
}

static COMPILER_UNUSED void Printer(BinaryTreeNode* node, int depth, void* data) {
  PrintSymbolNode((SymbolNode*)node, depth);
}


void LocalSymbolTableDelete(LocalSymbolTable* table) {
  // Clear the symbol table but don't delete the symbols.
  // BinaryTreeTraverse(&table->table, Printer, NULL);

  BinaryTreeDestruct(&table->table, NULL);
  NamespaceAliasVectorDestruct(&table->namespace_aliases);
  free(table);
}

Namespace* FindDirectLocalNamespaceAlias(LocalSymbolTable* table, String* name) {
  if (table == NULL) {
    return NULL;
  }
  return FindNamespaceAliasInVector(&table->namespace_aliases, name);
}

NamespaceAliasInsertResult InsertLocalNamespaceAlias(LocalSymbolTable* table,
                                                     String* name,
                                                     Namespace* target) {
  if (table == NULL || target == NULL) {
    return kNamespaceAliasConflict;
  }
  return InsertNamespaceAliasInVector(&table->namespace_aliases, name, target);
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
  ns->is_inline = false;
  InitSymbolTree(&ns->symbol_table);
  InitSymbolTree(&ns->tag_table);
  VectorInit(&ns->children);
  VectorInit(&ns->namespace_aliases);
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
  NamespaceAliasVectorDestruct(&ns->namespace_aliases);
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

Namespace* NamespaceFindDirectChild(Namespace* parent, String* name) {
  return NamespaceFindChild(parent, name);
}

Namespace* NamespaceFindDirectAlias(Namespace* ns, String* name) {
  if (ns == NULL) {
    return NULL;
  }
  return FindNamespaceAliasInVector(&ns->namespace_aliases, name);
}

NamespaceAliasInsertResult NamespaceInsertAlias(Namespace* ns, String* name,
                                                Namespace* target) {
  if (ns == NULL || target == NULL || NamespaceFindDirectChild(ns, name) != NULL) {
    return kNamespaceAliasConflict;
  }
  return InsertNamespaceAliasInVector(&ns->namespace_aliases, name, target);
}

Namespace* NamespaceFindOrReopenChild(Namespace* parent, String* name,
                                      bool is_inline, bool* inline_conflict) {
  if (inline_conflict != NULL) {
    *inline_conflict = false;
  }
  Namespace* child = NamespaceFindChild(parent, name);
  if (child == NULL) {
    child = NewNamespace(name->value, parent, false);
    child->is_inline = is_inline;
    VectorAppend(&parent->children, child);
    return child;
  }
  if (is_inline && !child->is_inline) {
    if (inline_conflict != NULL) {
      *inline_conflict = true;
    }
  }
  return child;
}

void NamespaceForEachInlineChild(Namespace* parent,
                                 void (*visit)(Namespace* child, void* ctx),
                                 void* ctx) {
  if (parent == NULL || visit == NULL) {
    return;
  }
  for (size_t i = 0; i < parent->children.length; i++) {
    Namespace* child = parent->children.value.p[i];
    if (child != NULL && child->is_inline) {
      visit(child, ctx);
    }
  }
}

static bool NamespaceVectorContains(Vector* namespaces, Namespace* ns) {
  for (size_t i = 0; i < namespaces->length; i++) {
    if (namespaces->value.p[i] == ns) {
      return true;
    }
  }
  return false;
}

static void NamespaceVectorAdd(Vector* namespaces, Namespace* ns) {
  if (ns != NULL && !NamespaceVectorContains(namespaces, ns)) {
    VectorAppend(namespaces, ns);
  }
}

static void NamespaceAddInlineDescendants(Vector* namespaces, Namespace* ns) {
  if (ns == NULL) {
    return;
  }
  for (size_t i = 0; i < ns->children.length; i++) {
    Namespace* child = ns->children.value.p[i];
    if (child == NULL || !child->is_inline) {
      continue;
    }
    NamespaceVectorAdd(namespaces, child);
    NamespaceAddInlineDescendants(namespaces, child);
  }
}

void NamespaceCollectADLAssociatedNamespaces(Namespace* ns, Vector* namespaces) {
  if (ns == NULL || namespaces == NULL) {
    return;
  }

  Namespace* inline_chain = ns;
  while (inline_chain != NULL) {
    NamespaceVectorAdd(namespaces, inline_chain);
    if (inline_chain->is_anonymous) {
      if (inline_chain->parent != NULL) {
        NamespaceVectorAdd(namespaces, inline_chain->parent);
      }
      break;
    }
    if (!inline_chain->is_inline) {
      break;
    }
    inline_chain = inline_chain->parent;
  }

  Namespace* search_root = ns;
  while (search_root != NULL &&
         (search_root->is_inline || search_root->is_anonymous)) {
    search_root = search_root->parent;
  }
  if (search_root == NULL) {
    search_root = ns;
  }
  NamespaceAddInlineDescendants(namespaces, search_root);
  if (search_root->anonymous_child != NULL) {
    NamespaceVectorAdd(namespaces, search_root->anonymous_child);
    NamespaceAddInlineDescendants(namespaces, search_root->anonymous_child);
  }
}

static bool SymbolVectorContains(Vector* symbols, Symbol* symbol) {
  for (size_t i = 0; i < symbols->length; i++) {
    if (symbols->value.p[i] == symbol) {
      return true;
    }
  }
  return false;
}

static void AppendSymbolHeadIfPresent(BinaryTree* table, String* name,
                                      Vector* heads) {
  Symbol* symbol = FindSymbol(table, name);
  if (symbol != NULL && !SymbolVectorContains(heads, symbol)) {
    VectorAppend(heads, symbol);
  }
}

static void CollectSymbolHeadsFromInlineNamespace(Namespace* ns, String* name,
                                                  Vector* heads) {
  if (ns == NULL) {
    return;
  }
  AppendSymbolHeadIfPresent(&ns->symbol_table, name, heads);
  if (ns->anonymous_child != NULL) {
    AppendSymbolHeadIfPresent(&ns->anonymous_child->symbol_table, name, heads);
  }
  for (size_t i = 0; i < ns->children.length; i++) {
    Namespace* child = ns->children.value.p[i];
    if (child != NULL && child->is_inline) {
      CollectSymbolHeadsFromInlineNamespace(child, name, heads);
    }
  }
}

void NamespaceCollectSymbolHeadsInInlineSet(Namespace* ns, String* name,
                                            Vector* heads) {
  if (ns == NULL || heads == NULL) {
    return;
  }
  AppendSymbolHeadIfPresent(&ns->symbol_table, name, heads);
  if (ns->anonymous_child != NULL) {
    AppendSymbolHeadIfPresent(&ns->anonymous_child->symbol_table, name, heads);
    for (size_t i = 0; i < ns->anonymous_child->children.length; i++) {
      Namespace* child = ns->anonymous_child->children.value.p[i];
      if (child != NULL && child->is_inline) {
        CollectSymbolHeadsFromInlineNamespace(child, name, heads);
      }
    }
  }
  for (size_t i = 0; i < ns->children.length; i++) {
    Namespace* child = ns->children.value.p[i];
    if (child != NULL && child->is_inline) {
      CollectSymbolHeadsFromInlineNamespace(child, name, heads);
    }
  }
}

static void AppendTagHeadIfPresent(BinaryTree* table, String* name,
                                   Vector* tags) {
  Symbol* tag = FindSymbol(table, name);
  if (tag != NULL && !SymbolVectorContains(tags, tag)) {
    VectorAppend(tags, tag);
  }
}

static void CollectTagHeadsFromInlineNamespace(Namespace* ns, String* name,
                                               Vector* tags) {
  if (ns == NULL) {
    return;
  }
  AppendTagHeadIfPresent(&ns->tag_table, name, tags);
  if (ns->anonymous_child != NULL) {
    AppendTagHeadIfPresent(&ns->anonymous_child->tag_table, name, tags);
  }
  for (size_t i = 0; i < ns->children.length; i++) {
    Namespace* child = ns->children.value.p[i];
    if (child != NULL && child->is_inline) {
      CollectTagHeadsFromInlineNamespace(child, name, tags);
    }
  }
}

void NamespaceCollectTagHeadsInInlineSet(Namespace* ns, String* name,
                                         Vector* tags) {
  if (ns == NULL || tags == NULL) {
    return;
  }
  AppendTagHeadIfPresent(&ns->tag_table, name, tags);
  if (ns->anonymous_child != NULL) {
    AppendTagHeadIfPresent(&ns->anonymous_child->tag_table, name, tags);
    for (size_t i = 0; i < ns->anonymous_child->children.length; i++) {
      Namespace* child = ns->anonymous_child->children.value.p[i];
      if (child != NULL && child->is_inline) {
        CollectTagHeadsFromInlineNamespace(child, name, tags);
      }
    }
  }
  for (size_t i = 0; i < ns->children.length; i++) {
    Namespace* child = ns->children.value.p[i];
    if (child != NULL && child->is_inline) {
      CollectTagHeadsFromInlineNamespace(child, name, tags);
    }
  }
}

void NamespaceCollectFunctionSymbolsInInlineSet(Namespace* ns, String* name,
                                                Vector* functions) {
  if (ns == NULL || functions == NULL) {
    return;
  }
  Vector heads;
  VectorInit(&heads);
  NamespaceCollectSymbolHeadsInInlineSet(ns, name, &heads);
  for (size_t i = 0; i < heads.length; i++) {
    Symbol* head = (Symbol*)heads.value.p[i];
    for (Symbol* candidate = head; candidate != NULL;
         candidate = candidate->overload_next) {
      if (candidate->type != NULL && TypeIsFunction(candidate->type) &&
          !SymbolVectorContains(functions, candidate)) {
        VectorAppend(functions, candidate);
      }
    }
  }
  VectorDestruct(&heads);
}

static NamespaceInlineSymbolLookup
ResolveCollectedSymbolHeads(Vector* heads) {
  NamespaceInlineSymbolLookup result = {kInlineLookupNotFound, NULL};
  Vector non_functions;
  VectorInit(&non_functions);
  Symbol* first_function = NULL;
  for (size_t i = 0; i < heads->length; i++) {
    Symbol* symbol = (Symbol*)heads->value.p[i];
    if (symbol->type != NULL && TypeIsFunction(symbol->type)) {
      if (first_function == NULL) {
        first_function = symbol;
      }
      continue;
    }
    if (!SymbolVectorContains(&non_functions, symbol)) {
      VectorAppend(&non_functions, symbol);
    }
  }
  if (non_functions.length > 1) {
    result.status = kInlineLookupAmbiguous;
    VectorDestruct(&non_functions);
    return result;
  }
  if (non_functions.length == 1) {
    result.status = kInlineLookupUnique;
    result.symbol = (Symbol*)non_functions.value.p[0];
    VectorDestruct(&non_functions);
    return result;
  }
  VectorDestruct(&non_functions);
  if (first_function != NULL) {
    result.status = kInlineLookupUnique;
    result.symbol = first_function;
  }
  return result;
}

NamespaceInlineSymbolLookup NamespaceResolveSymbolInInlineSet(Namespace* ns,
                                                              String* name) {
  Vector heads;
  VectorInit(&heads);
  NamespaceCollectSymbolHeadsInInlineSet(ns, name, &heads);
  NamespaceInlineSymbolLookup result = ResolveCollectedSymbolHeads(&heads);
  VectorDestruct(&heads);
  return result;
}

NamespaceInlineTagLookup NamespaceResolveTagInInlineSet(Namespace* ns,
                                                        String* name) {
  NamespaceInlineTagLookup result = {kInlineLookupNotFound, NULL};
  Vector tags;
  VectorInit(&tags);
  NamespaceCollectTagHeadsInInlineSet(ns, name, &tags);
  if (tags.length > 1) {
    result.status = kInlineLookupAmbiguous;
  } else if (tags.length == 1) {
    result.status = kInlineLookupUnique;
    result.tag = (Symbol*)tags.value.p[0];
  }
  VectorDestruct(&tags);
  return result;
}

static void CollectNamespaceChildrenFromInlineNamespace(Namespace* ns,
                                                        String* name,
                                                        Vector* children);

static void CollectNamespaceChildrenInInlineSet(Namespace* parent, String* name,
                                                Vector* children) {
  if (parent == NULL || children == NULL) {
    return;
  }
  Namespace* direct = NamespaceFindDirectChild(parent, name);
  if (direct != NULL && !NamespaceVectorContains(children, direct)) {
    VectorAppend(children, direct);
  }
  Namespace* alias = NamespaceFindDirectAlias(parent, name);
  if (alias != NULL && !NamespaceVectorContains(children, alias)) {
    VectorAppend(children, alias);
  }
  for (size_t i = 0; i < parent->children.length; i++) {
    Namespace* child = parent->children.value.p[i];
    if (child != NULL && child->is_inline) {
      CollectNamespaceChildrenFromInlineNamespace(child, name, children);
    }
  }
}

static void CollectNamespaceChildrenFromInlineNamespace(Namespace* ns,
                                                        String* name,
                                                        Vector* children) {
  if (ns == NULL) {
    return;
  }
  Namespace* direct = NamespaceFindDirectChild(ns, name);
  if (direct != NULL && !NamespaceVectorContains(children, direct)) {
    VectorAppend(children, direct);
  }
  Namespace* alias = NamespaceFindDirectAlias(ns, name);
  if (alias != NULL && !NamespaceVectorContains(children, alias)) {
    VectorAppend(children, alias);
  }
  for (size_t i = 0; i < ns->children.length; i++) {
    Namespace* child = ns->children.value.p[i];
    if (child != NULL && child->is_inline) {
      CollectNamespaceChildrenFromInlineNamespace(child, name, children);
    }
  }
}

NamespaceInlineChildLookup NamespaceResolveChildInInlineSet(Namespace* parent,
                                                          String* name) {
  NamespaceInlineChildLookup result = {kInlineLookupNotFound, NULL};
  Vector children;
  VectorInit(&children);
  CollectNamespaceChildrenInInlineSet(parent, name, &children);
  if (children.length > 1) {
    result.status = kInlineLookupAmbiguous;
  } else if (children.length == 1) {
    result.status = kInlineLookupUnique;
    result.child = (Namespace*)children.value.p[0];
  }
  VectorDestruct(&children);
  return result;
}

Symbol* NamespaceLookupUnqualifiedSymbol(Namespace* ns, String* name) {
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(ns, name);
  if (result.status == kInlineLookupUnique) {
    return result.symbol;
  }
  return NULL;
}

Symbol* NamespaceLookupUnqualifiedTag(Namespace* ns, String* name) {
  NamespaceInlineTagLookup result = NamespaceResolveTagInInlineSet(ns, name);
  if (result.status == kInlineLookupUnique) {
    return result.tag;
  }
  return NULL;
}

Namespace* NamespaceFindChildForQualifiedLookup(Namespace* parent, String* name) {
  NamespaceInlineChildLookup result =
      NamespaceResolveChildInInlineSet(parent, name);
  if (result.status == kInlineLookupUnique) {
    return result.child;
  }
  return NULL;
}

Symbol* NamespaceLookupSymbolInEnclosingScopes(Namespace* ns, String* name) {
  while (ns != NULL) {
    NamespaceInlineSymbolLookup result =
        NamespaceResolveSymbolInInlineSet(ns, name);
    if (result.status == kInlineLookupUnique) {
      return result.symbol;
    }
    if (result.status == kInlineLookupAmbiguous) {
      return NULL;
    }
    ns = ns->parent;
  }
  return NULL;
}

Symbol* NamespaceLookupTagInEnclosingScopes(Namespace* ns, String* name) {
  while (ns != NULL) {
    NamespaceInlineTagLookup result = NamespaceResolveTagInInlineSet(ns, name);
    if (result.status == kInlineLookupUnique) {
      return result.tag;
    }
    if (result.status == kInlineLookupAmbiguous) {
      return NULL;
    }
    ns = ns->parent;
  }
  return NULL;
}

Namespace* NamespaceFindStdNamespace(void) {
  if (compiler == NULL || compiler->global_namespace == NULL) {
    return NULL;
  }
  String std_name;
  StringInit(&std_name, "std");
  Namespace* std_ns =
      NamespaceFindDirectChild(compiler->global_namespace, &std_name);
  StringDestruct(&std_name);
  return std_ns;
}

Namespace* NamespaceParentForInlineTransparentLookup(Namespace* declaring_ns) {
  if (declaring_ns == NULL) {
    return compiler != NULL ? compiler->global_namespace : NULL;
  }
  Namespace* ns = declaring_ns;
  while (ns->parent != NULL && (ns->is_inline || ns->is_anonymous)) {
    ns = ns->parent;
  }
  return ns;
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
  bool inline_conflict = false;
  return NamespaceFindOrReopenAnonymousChild(parent, false, &inline_conflict);
}

Namespace* NamespaceFindOrReopenAnonymousChild(Namespace* parent, bool is_inline,
                                               bool* inline_conflict) {
  if (inline_conflict != NULL) {
    *inline_conflict = false;
  }
  if (parent->anonymous_child != NULL) {
    if (is_inline && !parent->anonymous_child->is_inline) {
      if (inline_conflict != NULL) {
        *inline_conflict = true;
      }
    }
    return parent->anonymous_child;
  }
  String name;
  StringInit(&name, NULL);
  StringPrintf(&name, "__anonymous_namespace_%d", anonymous_namespace_id++);
  Namespace* child = NewNamespace(name.value, parent, true);
  child->is_inline = is_inline;
  StringDestruct(&name);
  parent->anonymous_child = child;
  VectorAppend(&parent->children, child);
  return child;
}

bool NamespaceInsertSymbol(Namespace* ns, Symbol* symbol) {
  if (NamespaceFindDirectAlias(ns, &symbol->name) != NULL) {
    return false;
  }
  symbol->namespace_ = ns;
  SymbolNode* node = NewSymbolNode(symbol);
  bool ok = BinaryTreeInsert(&ns->symbol_table, &node->header);
  if (!ok) {
    SymbolNodeDelete(node);
    symbol->namespace_ = NULL;
  }
  return ok;
}

bool NamespaceInsertTag(Namespace* ns, Symbol* symbol) {
  if (NamespaceFindDirectAlias(ns, &symbol->name) != NULL) {
    return false;
  }
  symbol->namespace_ = ns;
  SymbolNode* node = NewSymbolNode(symbol);
  bool ok = BinaryTreeInsert(&ns->tag_table, &node->header);
  if (!ok) {
    SymbolNodeDelete(node);
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
    SymbolNodeDelete(node);
  }
  return ok;
}

bool InsertGlobalTag(Symbol* symbol) {
  SymbolNode* node = NewSymbolNode(symbol);
  bool ok = HashTableInsert(&compiler->global_tag_table, node);
  if (!ok) {
    // Insertion unsuccessful.  Don't need the SymbolNode any more.
    SymbolNodeDelete(node);
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
  if (FindDirectLocalNamespaceAlias(table, &symbol->name) != NULL) {
    return false;
  }
  SymbolNode* node = NewSymbolNode(symbol);
  bool ok = BinaryTreeInsert(&table->table, &node->header);
  if (!ok) {
    SymbolNodeDelete(node);
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
  StringInit(&node->name, symbol->name.value);
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
  if (table == NULL) {
    return NULL;
  }
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
      name = &((SymbolNode*)value)->name;
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
