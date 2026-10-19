//
//  module_identity.c
//  c_compiler
//

#include "module_identity.h"

#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "module_unit.h"
#include "type.h"

bool ModuleIdentityIsNamed(const String* module_name) {
  return module_name != NULL && module_name->length > 0;
}

void ModuleIdentityFormat(const String* module_name, const String* partition,
                          String* out) {
  StringInit(out, "");
  if (!ModuleIdentityIsNamed(module_name)) {
    return;
  }
  StringAppend(out, module_name->value);
  if (partition != NULL && partition->length > 0) {
    StringAppendChar(out, ':');
    StringAppend(out, partition->value);
  }
}

static bool ModuleUnitInPurviewOrPrivate(const ModuleUnitInfo* info) {
  return info->fragment == kModuleFragmentPurview ||
         info->fragment == kModuleFragmentPrivate;
}

void SymbolAttachModuleContext(Symbol* sym, Storage storage) {
  if (sym == NULL) {
    return;
  }

  const ModuleUnitInfo* unit = &compiler->module_unit;
  if (!unit->is_module_unit || !unit->has_module_declaration ||
      !ModuleUnitInPurviewOrPrivate(unit)) {
    sym->cxx_linkage = StorageIs(storage, STO(static)) ? kCXXLinkageInternal
                                                        : kCXXLinkageExternal;
    return;
  }

  if (unit->id.name.length > 0) {
    SymbolSetModuleIdentity(sym, unit->id.name.value,
                            unit->id.partition.length > 0
                                ? unit->id.partition.value
                                : SymbolOwningModulePartition(sym),
                            SymbolImportSourceModule(sym));
  }
  sym->flags.is_module_private =
      unit->fragment == kModuleFragmentPrivate;

  if (StorageIs(storage, STO(static))) {
    sym->cxx_linkage = kCXXLinkageInternal;
  } else if (sym->flags.is_exported || sym->flags.is_c_linkage) {
    sym->cxx_linkage = kCXXLinkageExternal;
  } else {
    sym->cxx_linkage = kCXXLinkageModule;
  }
}

bool SymbolSameOwningModule(const Symbol* a, const Symbol* b) {
  if (a == NULL || b == NULL) {
    return false;
  }
  return strcmp(SymbolOwningModuleName(a), SymbolOwningModuleName(b)) == 0 &&
         strcmp(SymbolOwningModulePartition(a),
                SymbolOwningModulePartition(b)) == 0;
}

static bool ImportedPrimaryInterfaceBinding(const Symbol* imported,
                                            const Symbol* local) {
  if (imported == NULL || local == NULL ||
      !SymbolIsImportedFromModule(imported) || !SymbolHasOwningModule(local)) {
    return false;
  }
  return strcmp(SymbolOwningModuleName(imported),
                SymbolOwningModuleName(local)) == 0 &&
         strcmp(SymbolImportSourceModule(imported),
                SymbolOwningModuleName(local)) == 0 &&
         SymbolOwningModulePartition(local)[0] == '\0';
}

bool SymbolCompatibleModuleRedeclaration(const Symbol* existing,
                                           const Symbol* incoming) {
  if (existing == NULL || incoming == NULL) {
    return true;
  }

  if (SymbolSameOwningModule(existing, incoming) &&
      (SymbolHasOwningModule(existing) || SymbolHasOwningModule(incoming))) {
    return true;
  }

  if (ImportedPrimaryInterfaceBinding(existing, incoming) ||
      ImportedPrimaryInterfaceBinding(incoming, existing)) {
    return true;
  }

  if (SymbolIsImportedFromModule(existing) &&
      !SymbolHasOwningModule(incoming) && existing->flags.is_exported) {
    return true;
  }
  if (SymbolIsImportedFromModule(incoming) &&
      !SymbolHasOwningModule(existing) && incoming->flags.is_exported) {
    return true;
  }

  if (!SymbolHasOwningModule(existing) && !SymbolHasOwningModule(incoming)) {
    return true;
  }

  return false;
}

void SymbolSetImportProvenance(Symbol* sym, const char* source_module) {
  if (sym == NULL || source_module == NULL || source_module[0] == '\0') {
    return;
  }
  SymbolSetModuleIdentity(sym, SymbolOwningModuleName(sym),
                          SymbolOwningModulePartition(sym), source_module);
}

Symbol* SymbolFindModuleCompatibleOverload(Symbol* head, Symbol* incoming) {
  for (Symbol* candidate = head; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!SymbolCompatibleModuleRedeclaration(candidate, incoming)) {
      continue;
    }
    return candidate;
  }
  return NULL;
}

void AppendCXXModuleIdentityMangling(String* out, const Symbol* symbol) {
  if (symbol == NULL || symbol->flags.is_c_linkage ||
      !SymbolHasOwningModule(symbol)) {
    return;
  }

  // Every C++ entity attached to a named module needs a distinct ABI identity,
  // whether or not its name is exported.  C-linkage entities are excluded
  // above because their language linkage fixes the external name.
  // Itanium's module-name introducer keeps module attachment distinct from a
  // same-spelled namespace nested-name.  Partitions share the primary module's
  // attachment and therefore deliberately use the same ABI module name.
  StringAppendChar(out, 'W');
  const char* component = SymbolOwningModuleName(symbol);
  while (component != NULL && *component != '\0') {
    const char* dot = strchr(component, '.');
    size_t len = dot != NULL ? (size_t)(dot - component) : strlen(component);
    StringAppendUInt64(out, len);
    StringAppendSegment(out, component, len);
    component = dot != NULL ? dot + 1 : NULL;
  }
  StringAppendChar(out, 'E');
}
