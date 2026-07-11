//
//  module_install.c
//  c_compiler
//
//  See module_install.h.
//

#include "module_install.h"

#include "binary_tree.h"
#include "compiler.h"
#include "symbol.h"
#include "symbol_table.h"

// Installs one exported symbol into a destination namespace.  The compiler's
// root namespace does not own file-scope C/C++ symbols (those live in the
// global hash tables), so the root is special-cased.  Existing names are left
// untouched (existing declaration wins for this phase).
static void InstallSymbolInto(Namespace* dest, Symbol* sym, bool is_tag) {
  if (sym == NULL) {
    return;
  }
  if (dest == compiler->global_namespace) {
    Symbol* existing =
        is_tag ? FindGlobalTag(&sym->name) : FindGlobalSymbol(&sym->name);
    if (existing != NULL) {
      return;
    }
    sym->namespace_ = NULL;  // File/global scope.
    if (is_tag) {
      InsertGlobalTag(sym);
    } else {
      InsertGlobalSymbol(sym);
    }
    return;
  }

  Symbol* existing = is_tag ? NamespaceFindTagInScope(dest, &sym->name)
                            : NamespaceFindSymbolInScope(dest, &sym->name);
  if (existing != NULL) {
    return;
  }
  if (is_tag) {
    NamespaceInsertTag(dest, sym);  // Sets sym->namespace_ = dest.
  } else {
    NamespaceInsertSymbol(dest, sym);
  }
}

// BinaryTreeTraverse context: where to install and whether this is the tag
// table (vs the ordinary symbol table).
typedef struct {
  Namespace* dest;
  bool is_tag;
} MergeContext;

static void MergeSymbolNode(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  MergeContext* ctx = (MergeContext*)data;
  Symbol* sym = ((SymbolNode*)node)->symbol;
  if (sym != NULL && sym->flags.is_exported) {
    InstallSymbolInto(ctx->dest, sym, ctx->is_tag);
  }
}

// Recursively merges the exported contents of loaded namespace `src` into the
// live compiler namespace `dest`.  Returns false on inline/non-inline conflict.
static bool MergeNamespace(Namespace* dest, Namespace* src) {
  if (dest == NULL || src == NULL) {
    return false;
  }

  MergeContext sym_ctx = {dest, false};
  BinaryTreeTraverse(&src->symbol_table, MergeSymbolNode, &sym_ctx);
  MergeContext tag_ctx = {dest, true};
  BinaryTreeTraverse(&src->tag_table, MergeSymbolNode, &tag_ctx);

  for (size_t i = 0; i < src->children.length; i++) {
    Namespace* child = (Namespace*)VectorGet(&src->children, i);
    if (child == NULL || child->is_anonymous) {
      continue;
    }
    bool inline_conflict = false;
    Namespace* child_dest =
        NamespaceFindOrReopenChild(dest, &child->name, child->is_inline,
                                   &inline_conflict);
    if (inline_conflict) {
      return false;
    }
    if (!MergeNamespace(child_dest, child)) {
      return false;
    }
  }
  return true;
}

bool ModuleInstallLoaded(LoadedModule* m) {
  if (compiler == NULL || compiler->global_namespace == NULL || m == NULL) {
    return false;
  }

  // File-scope exported symbols (functions/variables live in the producer's
  // global symbol hash table, so they are carried as explicit root symbols
  // rather than in the namespace graph).  Exported global tags (struct/enum)
  // are not yet installed here; that is a documented limitation for this phase.
  for (size_t i = 0; i < m->root_symbols.length; i++) {
    Symbol* sym = (Symbol*)VectorGet(&m->root_symbols, i);
    if (sym != NULL && sym->flags.is_exported) {
      InstallSymbolInto(compiler->global_namespace, sym, /*is_tag=*/false);
    }
  }

  // Namespaced exports (including nested namespaces).
  for (size_t i = 0; i < m->root_namespaces.length; i++) {
    Namespace* ns = (Namespace*)VectorGet(&m->root_namespaces, i);
    if (!MergeNamespace(compiler->global_namespace, ns)) {
      return false;
    }
  }

  return true;
}
