//
//  module_unit.c
//  c_compiler
//

#include "module_unit.h"

#include <stdlib.h>
#include <string.h>

void ModuleIdInit(ModuleId* id) {
  StringInit(&id->name, "");
  StringInit(&id->partition, "");
}

void ModuleIdDestruct(ModuleId* id) {
  StringDestruct(&id->name);
  StringDestruct(&id->partition);
}

void ModuleIdCopy(ModuleId* dst, const ModuleId* src) {
  StringSet(&dst->name, src->name.value);
  StringSet(&dst->partition, src->partition.value);
}

void ModuleIdFormat(const ModuleId* id, String* out) {
  StringSet(out, id->name.value);
  if (id->partition.length > 0) {
    StringAppendChar(out, ':');
    StringAppend(out, id->partition.value);
  }
}

void ModuleImportRefInit(ModuleImportRef* ref) {
  ModuleIdInit(&ref->id);
  ref->is_partition_import = false;
  ref->is_export_import = false;
  ref->is_header_unit = false;
  ref->location = SOURCE_LOCATION_MISSING;
}

void ModuleImportRefDestruct(ModuleImportRef* ref) {
  ModuleIdDestruct(&ref->id);
}

static void DestroyModuleImportRef(void* element) {
  if (element == NULL) {
    return;
  }
  ModuleImportRefDestruct((ModuleImportRef*)element);
}

void ModuleUnitInfoInit(ModuleUnitInfo* info) {
  ModuleIdInit(&info->id);
  info->kind = kModuleUnitKindNone;
  info->fragment = kModuleFragmentNone;
  info->phase = kModuleParserPhaseStart;
  info->has_module_declaration = false;
  info->saw_global_module_fragment = false;
  info->is_module_unit = false;
  info->module_declaration_location = SOURCE_LOCATION_MISSING;
  VectorInit(&info->imports);
}

void ModuleUnitInfoDestruct(ModuleUnitInfo* info) {
  VectorDestructWithContents(&info->imports, DestroyModuleImportRef,
                             /*free_element=*/true);
  ModuleIdDestruct(&info->id);
}

bool ModuleUnitIsInterfaceUnit(const ModuleUnitInfo* info) {
  return info->kind == kModuleUnitKindPrimaryInterface ||
         info->kind == kModuleUnitKindInterfacePartition;
}

bool ModuleUnitIsPrimaryInterfaceUnit(const ModuleUnitInfo* info) {
  return info->kind == kModuleUnitKindPrimaryInterface;
}

bool ModuleUnitAllowsExportDeclarations(const ModuleUnitInfo* info) {
  if (!ModuleUnitIsInterfaceUnit(info)) {
    return false;
  }
  return info->fragment == kModuleFragmentPurview;
}

const String* ModuleUnitLogicalName(const ModuleUnitInfo* info) {
  return &info->id.name;
}

ModuleImportRef* ModuleUnitAddImport(ModuleUnitInfo* info, const ModuleId* id,
                                     bool is_partition_import,
                                     bool is_export_import,
                                     bool is_header_unit,
                                     SourceLocation location) {
  ModuleImportRef* ref = (ModuleImportRef*)calloc(1, sizeof(*ref));
  ModuleImportRefInit(ref);
  ModuleIdCopy(&ref->id, id);
  ref->is_partition_import = is_partition_import;
  ref->is_export_import = is_export_import;
  ref->is_header_unit = is_header_unit;
  ref->location = location;
  VectorAppend(&info->imports, ref);
  return ref;
}
