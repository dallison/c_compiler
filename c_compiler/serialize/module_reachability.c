//
//  module_reachability.c
//  c_compiler
//

#include "module_reachability.h"

#include <stdio.h>

#include "binary_tree.h"
#include "compiler.h"
#include "symbol.h"
#include "symbol_table.h"

typedef struct {
  ModuleReachability* reachability;
  bool collect_roots;
  bool internal_partition;
  bool ok;
} CollectContext;

void ModuleReachabilityInit(ModuleReachability* reachability) {
  VectorInit(&reachability->exported_roots);
  reachability->error[0] = '\0';
}

void ModuleReachabilityDestruct(ModuleReachability* reachability) {
  VectorDestruct(&reachability->exported_roots);
}

static bool VectorContainsPointer(Vector* vector, void* pointer) {
  for (size_t i = 0; i < vector->length; i++) {
    if (VectorGet(vector, i) == pointer) {
      return true;
    }
  }
  return false;
}

static bool ValidateExportedSymbol(ModuleReachability* reachability,
                                   Symbol* symbol) {
  if (symbol->cxx_linkage == kCXXLinkageInternal) {
    snprintf(reachability->error, sizeof(reachability->error),
             "exported declaration '%s' has internal linkage",
             symbol->name.value);
    return false;
  }
  return true;
}

static void CollectExportedNode(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  CollectContext* context = (CollectContext*)data;
  Symbol* head = ((SymbolNode*)node)->symbol;
  Symbol* first_exported = NULL;
  for (Symbol* symbol = head; symbol != NULL;
       symbol = symbol->overload_next) {
    bool visible = context->internal_partition
                       ? symbol->import_source_module.length == 0 &&
                             symbol->owning_module_name.length != 0 &&
                             symbol->cxx_linkage != kCXXLinkageInternal
                       : symbol->flags.is_exported &&
                             symbol->import_source_module.length == 0;
    if (!visible) {
      continue;
    }
    if (first_exported == NULL) {
      first_exported = symbol;
    }
    if (!ValidateExportedSymbol(context->reachability, symbol)) {
      context->ok = false;
      return;
    }
  }
  if (context->collect_roots && first_exported != NULL &&
      !VectorContainsPointer(&context->reachability->exported_roots,
                             first_exported)) {
    VectorAppend(&context->reachability->exported_roots, first_exported);
  }
}

static void CollectExportedBucket(void* entry, void* data) {
  BinaryTreeTraverse((BinaryTree*)entry, CollectExportedNode, data);
}

static void HideHeaderInternalNode(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  (void)data;
  for (Symbol* symbol = ((SymbolNode*)node)->symbol; symbol != NULL;
       symbol = symbol->overload_next) {
    if (symbol->cxx_linkage == kCXXLinkageInternal) {
      symbol->flags.is_exported = false;
    }
  }
}

static void HideHeaderInternalBucket(void* entry, void* data) {
  (void)data;
  BinaryTreeTraverse((BinaryTree*)entry, HideHeaderInternalNode, NULL);
}

static void HideHeaderNamespaceInternals(Namespace* ns) {
  if (ns == NULL) {
    return;
  }
  BinaryTreeTraverse(&ns->symbol_table, HideHeaderInternalNode, NULL);
  BinaryTreeTraverse(&ns->tag_table, HideHeaderInternalNode, NULL);
  for (size_t i = 0; i < ns->children.length; i++) {
    HideHeaderNamespaceInternals((Namespace*)VectorGet(&ns->children, i));
  }
}

static void ValidateNamespace(Namespace* ns, CollectContext* context) {
  if (ns == NULL || !context->ok) {
    return;
  }
  BinaryTreeTraverse(&ns->symbol_table, CollectExportedNode, context);
  BinaryTreeTraverse(&ns->tag_table, CollectExportedNode, context);
  for (size_t i = 0; i < ns->children.length && context->ok; i++) {
    ValidateNamespace((Namespace*)VectorGet(&ns->children, i), context);
  }
}

bool ModuleReachabilityBuild(ModuleReachability* reachability,
                             bool header_unit) {
  if (compiler == NULL || compiler->global_namespace == NULL) {
    snprintf(reachability->error, sizeof(reachability->error),
             "module reachability requires an initialized compiler");
    return false;
  }

  bool internal_partition =
      compiler->module_unit.kind == kModuleUnitKindInternalPartition;
  if (header_unit) {
    HashTableTraverse(&compiler->global_symbol_table, HideHeaderInternalBucket,
                      NULL);
    HashTableTraverse(&compiler->global_tag_table, HideHeaderInternalBucket,
                      NULL);
    HideHeaderNamespaceInternals(compiler->global_namespace);
  }
  CollectContext global_context = {
      .reachability = reachability,
      .collect_roots = true,
      .internal_partition = internal_partition,
      .ok = true};
  HashTableTraverse(&compiler->global_symbol_table, CollectExportedBucket,
                    &global_context);
  HashTableTraverse(&compiler->global_tag_table, CollectExportedBucket,
                    &global_context);
  if (!global_context.ok) {
    return false;
  }

  CollectContext namespace_context = {
      .reachability = reachability,
      .collect_roots = false,
      .internal_partition = internal_partition,
      .ok = true};
  ValidateNamespace(compiler->global_namespace, &namespace_context);
  return namespace_context.ok;
}
