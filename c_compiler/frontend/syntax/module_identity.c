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
    StringSet(&sym->owning_module_name, unit->id.name.value);
    if (unit->id.partition.length > 0) {
      StringSet(&sym->owning_module_partition, unit->id.partition.value);
    }
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
  return strcmp(a->owning_module_name.value, b->owning_module_name.value) == 0 &&
         strcmp(a->owning_module_partition.value,
                b->owning_module_partition.value) == 0;
}

static bool ImportedPrimaryInterfaceBinding(const Symbol* imported,
                                            const Symbol* local) {
  if (imported == NULL || local == NULL ||
      imported->import_source_module.length == 0 ||
      local->owning_module_name.length == 0) {
    return false;
  }
  return strcmp(imported->owning_module_name.value,
                local->owning_module_name.value) == 0 &&
         strcmp(imported->import_source_module.value,
                local->owning_module_name.value) == 0 &&
         local->owning_module_partition.length == 0;
}

bool SymbolCompatibleModuleRedeclaration(const Symbol* existing,
                                           const Symbol* incoming) {
  if (existing == NULL || incoming == NULL) {
    return true;
  }

  if (SymbolSameOwningModule(existing, incoming) &&
      (existing->owning_module_name.length > 0 ||
       incoming->owning_module_name.length > 0)) {
    return true;
  }

  if (ImportedPrimaryInterfaceBinding(existing, incoming) ||
      ImportedPrimaryInterfaceBinding(incoming, existing)) {
    return true;
  }

  if (existing->import_source_module.length > 0 &&
      incoming->owning_module_name.length == 0 && existing->flags.is_exported) {
    return true;
  }
  if (incoming->import_source_module.length > 0 &&
      existing->owning_module_name.length == 0 && incoming->flags.is_exported) {
    return true;
  }

  if (existing->owning_module_name.length == 0 &&
      incoming->owning_module_name.length == 0) {
    return true;
  }

  return false;
}

void SymbolSetImportProvenance(Symbol* sym, const char* source_module) {
  if (sym == NULL || source_module == NULL || source_module[0] == '\0') {
    return;
  }
  StringSet(&sym->import_source_module, source_module);
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
      symbol->owning_module_name.length == 0) {
    return;
  }

  // Every C++ entity attached to a named module needs a distinct ABI identity,
  // whether or not its name is exported.  C-linkage entities are excluded
  // above because their language linkage fixes the external name.
  // Itanium's module-name introducer keeps module attachment distinct from a
  // same-spelled namespace nested-name.  Partitions share the primary module's
  // attachment and therefore deliberately use the same ABI module name.
  StringAppendChar(out, 'W');
  const char* component = symbol->owning_module_name.value;
  while (component != NULL && *component != '\0') {
    const char* dot = strchr(component, '.');
    size_t len = dot != NULL ? (size_t)(dot - component) : strlen(component);
    char lenbuf[32];
    snprintf(lenbuf, sizeof(lenbuf), "%zu", len);
    StringAppend(out, lenbuf);
    StringAppendSegment(out, component, len);
    component = dot != NULL ? dot + 1 : NULL;
  }
  StringAppendChar(out, 'E');
}
