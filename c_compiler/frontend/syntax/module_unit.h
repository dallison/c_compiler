//
//  module_unit.h
//  c_compiler
//
//  C++20 module-unit identity and parser state for a translation unit.
//

#ifndef module_unit_h
#define module_unit_h

#include <stdbool.h>
#include <stddef.h>

#include "dstring.h"
#include "source.h"
#include "vector.h"

typedef struct ModuleId {
  String name;       // Dotted module name, e.g. "foo.bar".
  String partition;  // Empty for primary/implementation units.
} ModuleId;

typedef enum {
  kModuleUnitKindNone = 0,
  kModuleUnitKindPrimaryInterface,
  kModuleUnitKindImplementation,
  kModuleUnitKindInterfacePartition,
  kModuleUnitKindInternalPartition,
} ModuleUnitKind;

typedef enum {
  kModuleFragmentNone = 0,
  kModuleFragmentGlobal,
  kModuleFragmentPurview,
  kModuleFragmentPrivate,
} ModuleFragmentState;

typedef enum {
  kModuleParserPhaseStart = 0,
  kModuleParserPhaseGlobalFragment,
  // Import declarations seen before any named module declaration; the TU is
  // treated as a non-module unit and a later module declaration is forbidden.
  kModuleParserPhasePreModule,
  kModuleParserPhaseImportPreamble,
  kModuleParserPhasePurview,
  kModuleParserPhasePrivateFragment,
} ModuleParserPhase;

typedef struct ModuleImportRef {
  ModuleId id;
  bool is_partition_import;  // `import :part;`
  bool is_export_import;     // `export import ...;`
  bool is_header_unit;       // `import "header"` / `import <header>`.
  SourceLocation location;
} ModuleImportRef;

typedef struct ModuleUnitInfo {
  ModuleId id;
  ModuleUnitKind kind;
  ModuleFragmentState fragment;
  ModuleParserPhase phase;
  bool has_module_declaration;
  bool saw_global_module_fragment;
  bool is_module_unit;
  SourceLocation module_declaration_location;
  Vector imports;  // ModuleImportRef* owned by this object.
} ModuleUnitInfo;

void ModuleIdInit(ModuleId* id);
void ModuleIdDestruct(ModuleId* id);
void ModuleIdCopy(ModuleId* dst, const ModuleId* src);
void ModuleIdFormat(const ModuleId* id, String* out);

void ModuleImportRefInit(ModuleImportRef* ref);
void ModuleImportRefDestruct(ModuleImportRef* ref);

void ModuleUnitInfoInit(ModuleUnitInfo* info);
void ModuleUnitInfoDestruct(ModuleUnitInfo* info);

bool ModuleUnitIsInterfaceUnit(const ModuleUnitInfo* info);
bool ModuleUnitIsPrimaryInterfaceUnit(const ModuleUnitInfo* info);
bool ModuleUnitAllowsExportDeclarations(const ModuleUnitInfo* info);
const String* ModuleUnitLogicalName(const ModuleUnitInfo* info);

ModuleImportRef* ModuleUnitAddImport(ModuleUnitInfo* info, const ModuleId* id,
                                     bool is_partition_import,
                                     bool is_export_import,
                                     bool is_header_unit,
                                     SourceLocation location);

#endif /* module_unit_h */
