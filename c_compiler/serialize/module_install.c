//
//  module_install.c
//  c_compiler
//
//  See module_install.h.
//

#include "module_install.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binary_tree.h"
#include "compiler.h"
#include "dstring.h"
#include "module_identity.h"
#include "symbol.h"
#include "symbol_table.h"
#include "type.h"

static char g_module_install_error[512];

const char* ModuleInstallLastError(void) {
  return g_module_install_error[0] != '\0' ? g_module_install_error : NULL;
}

static void ClearInstallError(void) {
  g_module_install_error[0] = '\0';
}

static void SetInstallError(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vsnprintf(g_module_install_error, sizeof(g_module_install_error), format, ap);
  va_end(ap);
}

void ModuleInstallRecordInit(ModuleInstallRecord* record) {
  VectorInit(&record->global_symbols);
  VectorInit(&record->global_tags);
  VectorInit(&record->namespaced_symbols);
  VectorInit(&record->namespaced_symbol_scopes);
  VectorInit(&record->namespaced_tags);
  VectorInit(&record->namespaced_tag_scopes);
  VectorInit(&record->created_namespaces);
  VectorInit(&record->alias_names);
  VectorInit(&record->alias_targets);
  VectorInit(&record->alias_scopes);
  VectorInit(&record->appended_overloads);
  VectorInit(&record->overload_heads);
}

void ModuleInstallRecordDestruct(ModuleInstallRecord* record) {
  for (size_t i = 0; i < record->alias_names.length; i++) {
    StringDestruct((String*)VectorGet(&record->alias_names, i));
    free(VectorGet(&record->alias_names, i));
  }
  VectorDestruct(&record->global_symbols);
  VectorDestruct(&record->global_tags);
  VectorDestruct(&record->namespaced_symbols);
  VectorDestruct(&record->namespaced_symbol_scopes);
  VectorDestruct(&record->namespaced_tags);
  VectorDestruct(&record->namespaced_tag_scopes);
  VectorDestruct(&record->created_namespaces);
  VectorDestruct(&record->alias_names);
  VectorDestruct(&record->alias_targets);
  VectorDestruct(&record->alias_scopes);
  VectorDestruct(&record->appended_overloads);
  VectorDestruct(&record->overload_heads);
}

static void VectorRemovePointer(Vector* vec, void* ptr) {
  for (size_t i = 0; i < vec->length; i++) {
    if (VectorGet(vec, i) == ptr) {
      vec->value.p[i] = vec->value.p[vec->length - 1];
      vec->length--;
      return;
    }
  }
}

static bool NamespaceShellIsEmpty(Namespace* ns) {
  return ns != NULL && ns->symbol_table.root == NULL &&
         ns->tag_table.root == NULL && ns->namespace_aliases.length == 0 &&
         ns->children.length == 0;
}

static void RemoveNamespaceAlias(Namespace* ns, String* name) {
  for (size_t i = 0; i < ns->namespace_aliases.length; i++) {
    NamespaceAlias* alias = (NamespaceAlias*)VectorGet(&ns->namespace_aliases, i);
    if (StringEqualString(&alias->name, name)) {
      StringDestruct(&alias->name);
      if (i + 1 < ns->namespace_aliases.length) {
        ns->namespace_aliases.value.p[i] =
            ns->namespace_aliases.value.p[ns->namespace_aliases.length - 1];
      }
      ns->namespace_aliases.length--;
      return;
    }
  }
}

static void UnlinkAppendedOverload(Symbol* head, Symbol* appended) {
  if (head == NULL || appended == NULL) {
    return;
  }
  if (head == appended) {
    return;
  }
  for (Symbol* prev = head; prev != NULL && prev->overload_next != NULL;
       prev = prev->overload_next) {
    if (prev->overload_next == appended) {
      prev->overload_next = appended->overload_next;
      appended->overload_next = NULL;
      appended->namespace_ = NULL;
      if (head->overload_next == NULL) {
        head->flags.is_overloaded = false;
      }
      appended->flags.is_overloaded = false;
      return;
    }
  }
}

void ModuleInstallRecordRollback(ModuleInstallRecord* record) {
  if (record == NULL || compiler == NULL) {
    return;
  }

  for (size_t i = 0; i < record->appended_overloads.length; i++) {
    Symbol* head = (Symbol*)VectorGet(&record->overload_heads, i);
    Symbol* appended = (Symbol*)VectorGet(&record->appended_overloads, i);
    UnlinkAppendedOverload(head, appended);
  }

  for (size_t i = 0; i < record->alias_scopes.length; i++) {
    Namespace* scope = (Namespace*)VectorGet(&record->alias_scopes, i);
    String* name = (String*)VectorGet(&record->alias_names, i);
    RemoveNamespaceAlias(scope, name);
  }

  for (size_t i = 0; i < record->namespaced_tags.length; i++) {
    Symbol* sym = (Symbol*)VectorGet(&record->namespaced_tags, i);
    Namespace* scope =
        (Namespace*)VectorGet(&record->namespaced_tag_scopes, i);
    UninstallNamespaceSymbol(scope, sym, true);
  }
  for (size_t i = 0; i < record->namespaced_symbols.length; i++) {
    Symbol* sym = (Symbol*)VectorGet(&record->namespaced_symbols, i);
    Namespace* scope =
        (Namespace*)VectorGet(&record->namespaced_symbol_scopes, i);
    UninstallNamespaceSymbol(scope, sym, false);
  }

  for (size_t i = 0; i < record->global_symbols.length; i++) {
    Symbol* sym = (Symbol*)VectorGet(&record->global_symbols, i);
    UninstallGlobalSymbol(sym, false);
  }
  for (size_t i = 0; i < record->global_tags.length; i++) {
    Symbol* sym = (Symbol*)VectorGet(&record->global_tags, i);
    UninstallGlobalSymbol(sym, true);
  }

  for (size_t i = 0; i < record->created_namespaces.length; i++) {
    Namespace* ns = (Namespace*)VectorGet(&record->created_namespaces, i);
    if (NamespaceShellIsEmpty(ns) && ns->parent != NULL) {
      VectorRemovePointer(&ns->parent->children, ns);
      NamespaceDelete(ns);
    }
  }

  ModuleInstallRecordDestruct(record);
  ModuleInstallRecordInit(record);
}

static Symbol* FollowAlias(Symbol* symbol) {
  while (symbol != NULL && symbol->flags.is_using_alias &&
         symbol->alias_target != NULL) {
    symbol = symbol->alias_target;
  }
  return symbol;
}

static bool CanOverloadFunctions(Symbol* a, Symbol* b) {
  return CompilerIsCXX() && a != NULL && b != NULL && a->type != NULL &&
         b->type != NULL && TypeIsFunction(a->type) && TypeIsFunction(b->type);
}

static bool MatchingTemplateKinds(Symbol* existing, Symbol* incoming) {
  if (existing->flags.is_template != incoming->flags.is_template) {
    return false;
  }
  if (existing->flags.is_concept != incoming->flags.is_concept) {
    return false;
  }
  if (TypeIsFunction(existing->type) && TypeIsFunction(incoming->type)) {
    bool existing_template =
        existing->flags.is_template ||
        existing->type->info.function.template_parameter_count > 0;
    bool incoming_template =
        incoming->flags.is_template ||
        incoming->type->info.function.template_parameter_count > 0;
    if (existing_template != incoming_template) {
      return false;
    }
  }
  return true;
}

static Symbol* FindMatchingOverload(Symbol* head, Symbol* incoming) {
  for (Symbol* candidate = head; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!SymbolCompatibleModuleRedeclaration(candidate, incoming)) {
      continue;
    }
    if (!MatchingTemplateKinds(candidate, incoming)) {
      continue;
    }
    if (TypeEqual(candidate->type, incoming->type)) {
      return candidate;
    }
  }
  return NULL;
}

static void AppendOverloadTracked(Symbol* head, Symbol* overload,
                                  ModuleInstallRecord* record) {
  Symbol* tail = head;
  while (tail->overload_next != NULL) {
    tail = tail->overload_next;
  }
  overload->namespace_ = head->namespace_;
  overload->overload_next = NULL;
  tail->overload_next = overload;
  head->flags.is_overloaded = true;
  overload->flags.is_overloaded = true;
  SymbolSetCXXMangledAsmName(overload);
  if (record != NULL) {
    VectorAppend(&record->overload_heads, head);
    VectorAppend(&record->appended_overloads, overload);
  }
}

static bool TagTypesCompatible(Symbol* existing, Symbol* incoming) {
  if (existing == NULL || incoming == NULL || existing->type == NULL ||
      incoming->type == NULL) {
    return false;
  }
  if (TypeEqual(existing->type, incoming->type)) {
    return true;
  }
  if (TypeIsStructOrUnion(existing->type) && TypeIsStructOrUnion(incoming->type) &&
      existing->type->info.struct_info == incoming->type->info.struct_info) {
    return true;
  }
  if (TypeIsEnum(existing->type) && TypeIsEnum(incoming->type) &&
      existing->type->info.enum_info == incoming->type->info.enum_info) {
    return true;
  }
  return false;
}

static bool SymbolsCompatibleRedeclaration(Symbol* existing, Symbol* incoming) {
  if (existing == NULL || incoming == NULL) {
    return false;
  }
  if (!SymbolCompatibleModuleRedeclaration(existing, incoming)) {
    return false;
  }
  if (existing->flags.is_using_alias || incoming->flags.is_using_alias) {
    return FollowAlias(existing) == FollowAlias(incoming);
  }
  if (existing->flags.is_concept != incoming->flags.is_concept) {
    return false;
  }
  if (existing->flags.is_template != incoming->flags.is_template) {
    return false;
  }
  if (SymbolIsTagSymbol(existing) || SymbolIsTagSymbol(incoming)) {
    return TagTypesCompatible(existing, incoming);
  }
  return TypeEqual(existing->type, incoming->type);
}

static Symbol* FindExistingSymbol(Namespace* dest, String* name, bool is_tag) {
  if (dest == compiler->global_namespace) {
    return is_tag ? FindGlobalTag(name) : FindGlobalSymbol(name);
  }
  return is_tag ? NamespaceFindTag(dest, name) : NamespaceFindSymbol(dest, name);
}

static bool TrackInsertedSymbol(Namespace* dest, Symbol* sym, bool is_tag,
                                ModuleInstallRecord* record) {
  if (record == NULL) {
    return true;
  }
  if (dest == compiler->global_namespace) {
    VectorAppend(is_tag ? &record->global_tags : &record->global_symbols, sym);
    return true;
  }
  if (is_tag) {
    VectorAppend(&record->namespaced_tags, sym);
    VectorAppend(&record->namespaced_tag_scopes, dest);
  } else {
    VectorAppend(&record->namespaced_symbols, sym);
    VectorAppend(&record->namespaced_symbol_scopes, dest);
  }
  return true;
}

static bool InsertFreshSymbol(Namespace* dest, Symbol* sym, bool is_tag,
                              ModuleInstallRecord* record) {
  if (dest == compiler->global_namespace) {
    sym->namespace_ = NULL;
    bool ok = is_tag ? InsertGlobalTag(sym) : InsertGlobalSymbol(sym);
    if (!ok) {
      SetInstallError("failed to insert exported symbol '%s'", sym->name.value);
      return false;
    }
    if (!is_tag && sym->is_imported_module_symbol && sym->flags.is_template &&
        sym->type != NULL && TypeIsFunction(sym->type)) {
      SymbolBackupImportedFunctionTemplateParameters(sym);
    }
    return TrackInsertedSymbol(dest, sym, is_tag, record);
  }

  bool ok = is_tag ? NamespaceInsertTag(dest, sym)
                   : NamespaceInsertSymbol(dest, sym);
  if (!ok) {
    SetInstallError("failed to insert exported symbol '%s' into namespace",
                    sym->name.value);
    return false;
  }
  if (!is_tag && sym->is_imported_module_symbol && sym->flags.is_template &&
      sym->type != NULL && TypeIsFunction(sym->type)) {
    SymbolBackupImportedFunctionTemplateParameters(sym);
  }
  return TrackInsertedSymbol(dest, sym, is_tag, record);
}

static Symbol* ExportedTypeTagSymbol(Symbol* sym) {
  if (sym == NULL || sym->type == NULL || SymbolIsTagSymbol(sym)) {
    return NULL;
  }
  if (TypeIsStructOrUnion(sym->type) && sym->type->info.struct_info != NULL) {
    return sym->type->info.struct_info->tag_symbol;
  }
  if (TypeIsEnum(sym->type) && sym->type->info.enum_info != NULL) {
    return sym->type->info.enum_info->tag_symbol;
  }
  return NULL;
}

static bool SymbolInstallAsTag(Symbol* sym, bool is_tag) {
  if (is_tag || sym == NULL) {
    return is_tag;
  }
  if (SymbolIsTagSymbol(sym)) {
    return true;
  }
  if (sym->type != NULL && StorageIs(sym->storage, STO(typedef)) &&
      (TypeIsStructOrUnion(sym->type) || TypeIsEnum(sym->type))) {
    Symbol* real_tag = ExportedTypeTagSymbol(sym);
    if (real_tag != NULL && real_tag != sym) {
      return false;
    }
    return true;
  }
  return false;
}

static void StampImportProvenance(Symbol* sym, const char* source_module) {
  for (Symbol* chain = sym; chain != NULL; chain = chain->overload_next) {
    SymbolSetImportProvenance(chain, source_module);
  }
}

static bool InstallSymbolIntoTracked(Namespace* dest, Symbol* sym, bool is_tag,
                                     ModuleInstallRecord* record,
                                     const char* source_module) {
  if (sym == NULL) {
    return true;
  }

  is_tag = SymbolInstallAsTag(sym, is_tag);
  Symbol* existing = FindExistingSymbol(dest, &sym->name, is_tag);
  if (existing == NULL) {
    for (Symbol* incoming = sym; incoming != NULL;
         incoming = incoming->overload_next) {
      if (incoming->flags.is_exported) {
        StampImportProvenance(incoming, source_module);
      }
    }
    return InsertFreshSymbol(dest, sym, is_tag, record);
  }

  for (Symbol* incoming = sym; incoming != NULL;
       incoming = incoming->overload_next) {
    if (!incoming->flags.is_exported) {
      continue;
    }
    StampImportProvenance(incoming, source_module);
    if (existing == incoming) {
      continue;
    }
    if (CanOverloadFunctions(existing, incoming)) {
      if (FindMatchingOverload(existing, incoming) != NULL) {
        continue;
      }
      AppendOverloadTracked(existing, incoming, record);
      continue;
    }
    if (SymbolsCompatibleRedeclaration(existing, incoming)) {
      continue;
    }
    SetInstallError("conflicting export '%s' in namespace '%s' from module '%s'",
                    sym->name.value,
                    dest->qualified_name.length > 0
                        ? dest->qualified_name.value
                        : "::",
                    source_module != NULL ? source_module : "");
    return false;
  }
  return true;
}

typedef struct {
  Namespace* dest;
  bool is_tag;
  ModuleInstallRecord* record;
  const char* source_module;
} MergeContext;

static void MergeSymbolNode(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  MergeContext* ctx = (MergeContext*)data;
  Symbol* sym = ((SymbolNode*)node)->symbol;
  if (sym != NULL && sym->flags.is_exported) {
    InstallSymbolIntoTracked(ctx->dest, sym, ctx->is_tag, ctx->record,
                             ctx->source_module);
  }
}

static bool MergeNamespaceTracked(Namespace* dest, Namespace* src,
                                  ModuleInstallRecord* record,
                                  const char* source_module) {
  if (dest == NULL || src == NULL) {
    return false;
  }

  MergeContext tag_ctx = {dest, true, record, source_module};
  BinaryTreeTraverse(&src->tag_table, MergeSymbolNode, &tag_ctx);
  if (ModuleInstallLastError() != NULL) {
    return false;
  }
  MergeContext sym_ctx = {dest, false, record, source_module};
  BinaryTreeTraverse(&src->symbol_table, MergeSymbolNode, &sym_ctx);
  if (ModuleInstallLastError() != NULL) {
    return false;
  }

  for (size_t i = 0; i < src->children.length; i++) {
    Namespace* child = (Namespace*)VectorGet(&src->children, i);
    if (child == NULL || child->is_anonymous) {
      continue;
    }
    bool inline_conflict = false;
    bool created = NamespaceFindChild(dest, &child->name) == NULL;
    Namespace* child_dest =
        NamespaceFindOrReopenChild(dest, &child->name, child->is_inline,
                                   &inline_conflict);
    if (inline_conflict) {
      SetInstallError("inline namespace conflict for '%s'", child->name.value);
      return false;
    }
    if (created && record != NULL) {
      VectorAppend(&record->created_namespaces, child_dest);
    }
    if (!MergeNamespaceTracked(child_dest, child, record, source_module)) {
      return false;
    }
  }
  return true;
}

static Namespace* FindInstalledNamespace(Namespace* loaded) {
  if (loaded == NULL || compiler == NULL || compiler->global_namespace == NULL) {
    return NULL;
  }
  if (loaded->parent == NULL) {
    return compiler->global_namespace;
  }

  Vector components;
  VectorInit(&components);
  for (Namespace* ns = loaded; ns != NULL && ns->parent != NULL;
       ns = ns->parent) {
    VectorAppend(&components, &ns->name);
  }

  Namespace* installed = compiler->global_namespace;
  for (size_t i = components.length; i > 0 && installed != NULL; i--) {
    installed = NamespaceFindDirectChild(
        installed, (String*)VectorGet(&components, i - 1));
  }
  VectorDestruct(&components);
  return installed;
}

static bool MergeNamespaceAliasesTracked(Namespace* dest, Namespace* src,
                                         ModuleInstallRecord* record) {
  if (dest == NULL || src == NULL) {
    return false;
  }
  for (size_t i = 0; i < src->namespace_aliases.length; i++) {
    NamespaceAlias* alias =
        (NamespaceAlias*)VectorGet(&src->namespace_aliases, i);
    if (alias == NULL) {
      continue;
    }
    Namespace* target = FindInstalledNamespace(alias->target);
    if (target == NULL) {
      SetInstallError("failed to resolve namespace alias target '%s'",
                      alias->name.value);
      return false;
    }
    NamespaceAliasInsertResult result =
        NamespaceInsertAlias(dest, &alias->name, target);
    if (result == kNamespaceAliasConflict) {
      SetInstallError("conflicting namespace alias '%s'", alias->name.value);
      return false;
    }
    if (result == kNamespaceAliasInserted &&
        dest->namespace_aliases.length > 0) {
      NamespaceAlias* installed = (NamespaceAlias*)VectorGet(
          &dest->namespace_aliases, dest->namespace_aliases.length - 1);
      installed->is_imported_module_alias = true;
    }
    if (result == kNamespaceAliasInserted && record != NULL) {
      String* name_copy = malloc(sizeof(String));
      StringInit(name_copy, alias->name.value);
      VectorAppend(&record->alias_names, name_copy);
      VectorAppend(&record->alias_targets, target);
      VectorAppend(&record->alias_scopes, dest);
    }
  }

  for (size_t i = 0; i < src->children.length; i++) {
    Namespace* child = (Namespace*)VectorGet(&src->children, i);
    if (child == NULL || child->is_anonymous) {
      continue;
    }
    Namespace* child_dest = NamespaceFindDirectChild(dest, &child->name);
    if (!MergeNamespaceAliasesTracked(child_dest, child, record)) {
      return false;
    }
  }
  return true;
}

static bool InstallExportedRootSymbol(Symbol* sym, ModuleInstallRecord* record,
                                      const char* source_module) {
  if (sym == NULL) {
    return true;
  }
  Symbol* tag = ExportedTypeTagSymbol(sym);
  if (tag != NULL) {
    if (!InstallSymbolIntoTracked(compiler->global_namespace, tag, true, record,
                                  source_module)) {
      return false;
    }
  }
  return InstallSymbolIntoTracked(compiler->global_namespace, sym,
                                  SymbolIsTagSymbol(sym), record,
                                  source_module);
}

bool ModuleInstallLoadedTracked(LoadedModule* m, ModuleInstallRecord* record) {
  ClearInstallError();
  if (compiler == NULL || compiler->global_namespace == NULL || m == NULL) {
    SetInstallError("no active compiler");
    return false;
  }

  const char* source_module = m->module_name.value;

  for (size_t i = 0; i < m->root_symbols.length; i++) {
    Symbol* sym = (Symbol*)VectorGet(&m->root_symbols, i);
    if (sym != NULL &&
        (sym->flags.is_exported ||
         (m->flags & kModuleArchiveInternalPartition) != 0)) {
      if (!InstallExportedRootSymbol(sym, record, source_module)) {
        if (record != NULL) {
          ModuleInstallRecordRollback(record);
        }
        return false;
      }
    }
  }

  for (size_t i = 0; i < m->root_namespaces.length; i++) {
    Namespace* ns = (Namespace*)VectorGet(&m->root_namespaces, i);
    if (!MergeNamespaceTracked(compiler->global_namespace, ns, record,
                               source_module)) {
      if (record != NULL) {
        ModuleInstallRecordRollback(record);
      }
      return false;
    }
  }
  for (size_t i = 0; i < m->root_namespaces.length; i++) {
    Namespace* ns = (Namespace*)VectorGet(&m->root_namespaces, i);
    if (!MergeNamespaceAliasesTracked(compiler->global_namespace, ns, record)) {
      if (record != NULL) {
        ModuleInstallRecordRollback(record);
      }
      return false;
    }
  }

  return true;
}

bool ModuleInstallLoaded(LoadedModule* m) {
  return ModuleInstallLoadedTracked(m, NULL);
}
