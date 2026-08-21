//
//  module_import.c
//  c_compiler
//
//  See module_import.h.
//

#include "module_import.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "dstring.h"
#include "module_archive.h"
#include "module_install.h"
#include "options.h"
#include "preprocessor.h"

typedef enum {
  kModuleImportAbsent = 0,
  kModuleImportLoading,
  kModuleImportLoaded,
  kModuleImportFailed,
} ModuleImportStatus;

typedef struct {
  String name;
  ModuleImportStatus status;
  LoadedModule* module;  // Owned while status is kModuleImportLoaded.
  ModuleInstallRecord install_record;
  bool installed;
  Vector installed_macro_names;  // owned String*
  char error[256];
} ImportEntry;

typedef struct {
  String name;
  String path;
} ModulePathMapping;

struct TranslationUnitImportState {
  Vector search_paths;  // const char* directories borrowed from options.
  Vector mappings;      // ModulePathMapping*
  Vector entries;       // ImportEntry*
  char last_error[512];
  bool released;
};

static void SetLastError(TranslationUnitImportState* state, const char* msg) {
  if (state == NULL) {
    return;
  }
  if (msg == NULL || msg[0] == '\0') {
    state->last_error[0] = '\0';
    return;
  }
  snprintf(state->last_error, sizeof(state->last_error), "%s", msg);
}

static ImportEntry* FindEntry(TranslationUnitImportState* state,
                              const char* module_name) {
  for (size_t i = 0; i < state->entries.length; i++) {
    ImportEntry* entry = (ImportEntry*)VectorGet(&state->entries, i);
    if (StringEqual(&entry->name, module_name)) {
      return entry;
    }
  }
  return NULL;
}

static ImportEntry* FindOrCreateEntry(TranslationUnitImportState* state,
                                      const char* module_name) {
  ImportEntry* entry = FindEntry(state, module_name);
  if (entry != NULL) {
    return entry;
  }
  entry = calloc(1, sizeof(ImportEntry));
  StringInit(&entry->name, module_name);
  ModuleInstallRecordInit(&entry->install_record);
  VectorInit(&entry->installed_macro_names);
  VectorAppend(&state->entries, entry);
  return entry;
}

static bool FileExists(const char* path) {
  FILE* f = fopen(path, "r");
  if (f == NULL) {
    return false;
  }
  fclose(f);
  return true;
}

static bool ModuleArtifactFilename(const char* module_name, char* filename,
                                   size_t filename_len) {
  if (module_name == NULL || filename_len < 5) {
    return false;
  }
  size_t input_length = strlen(module_name);
  size_t begin = 0;
  size_t end = input_length;
  if (input_length >= 2 &&
      ((module_name[0] == '"' && module_name[input_length - 1] == '"') ||
       (module_name[0] == '<' && module_name[input_length - 1] == '>'))) {
    begin = 1;
    end--;
  }
  size_t out = 0;
  for (size_t i = begin; i < end; i++) {
    if (out + 5 >= filename_len) {
      return false;
    }
    char ch = module_name[i];
    filename[out++] =
        ch == ':' || ch == '/' || ch == '\\' ? '-' : ch;
  }
  memcpy(filename + out, ".dcm", 5);
  return true;
}

static bool ResolveModulePath(TranslationUnitImportState* state,
                              const char* module_name, char* path,
                              size_t path_len) {
  for (size_t i = 0; i < state->mappings.length; i++) {
    ModulePathMapping* mapping =
        (ModulePathMapping*)VectorGet(&state->mappings, i);
    if (StringEqual(&mapping->name, module_name)) {
      snprintf(path, path_len, "%s", mapping->path.value);
      return FileExists(path);
    }
  }
  char filename[4096];
  if (!ModuleArtifactFilename(module_name, filename, sizeof(filename))) {
    return false;
  }
  for (size_t i = 0; i < state->search_paths.length; i++) {
    const char* dir = (const char*)VectorGet(&state->search_paths, i);
    snprintf(path, path_len, "%s/%s", dir, filename);
    if (FileExists(path)) {
      return true;
    }
    // Compatibility with the original literal-name lookup.
    snprintf(path, path_len, "%s/%s.dcm", dir, module_name);
    if (FileExists(path)) {
      return true;
    }
  }
  snprintf(path, path_len, "%s", filename);
  if (FileExists(path)) {
    return true;
  }
  snprintf(path, path_len, "%s.dcm", module_name);
  return FileExists(path);
}

bool ModuleValidateLoadedForImport(const char* requested_module_name,
                                   const char* requested_target,
                                   const char* archive_module_name,
                                   const char* archive_target, char* err,
                                   size_t err_len) {
  if (requested_module_name == NULL || requested_module_name[0] == '\0') {
    if (err != NULL && err_len > 0) {
      snprintf(err, err_len, "missing requested module name");
    }
    return false;
  }

  if (archive_module_name != NULL && archive_module_name[0] != '\0' &&
      strcmp(archive_module_name, requested_module_name) != 0) {
    if (err != NULL && err_len > 0) {
      snprintf(err, err_len,
               "module file declares '%s' but import requested '%s'",
               archive_module_name, requested_module_name);
    }
    return false;
  }

  if (requested_target != NULL && requested_target[0] != '\0' &&
      archive_target != NULL && archive_target[0] != '\0' &&
      strcmp(archive_target, requested_target) != 0) {
    if (err != NULL && err_len > 0) {
      snprintf(err, err_len,
               "module file target '%s' does not match compiler target '%s'",
               archive_target, requested_target);
    }
    return false;
  }

  return true;
}

TranslationUnitImportState* TranslationUnitImportStateCreate(Vector* options) {
  TranslationUnitImportState* state =
      calloc(1, sizeof(TranslationUnitImportState));
  VectorInit(&state->search_paths);
  VectorInit(&state->mappings);
  VectorInit(&state->entries);
  if (options != NULL) {
    for (size_t i = 0; i < options->length; i++) {
      CompilerOptionValue* opt = options->value.p[i];
      if (opt->opt == kOptionPrebuiltModulePath) {
        VectorAppend(&state->search_paths, opt->value.svalue.value);
      } else if (opt->opt == kOptionModuleFile) {
        const char* equals = strchr(opt->value.svalue.value, '=');
        if (equals != NULL && equals != opt->value.svalue.value &&
            equals[1] != '\0') {
          ModulePathMapping* mapping = malloc(sizeof(ModulePathMapping));
          StringInit(&mapping->name, "");
          StringAppendSegment(&mapping->name, opt->value.svalue.value,
                              (size_t)(equals - opt->value.svalue.value));
          StringInit(&mapping->path, equals + 1);
          VectorAppend(&state->mappings, mapping);
        }
      }
    }
  }
  return state;
}

void TranslationUnitImportStateRelease(TranslationUnitImportState* state) {
  if (state == NULL || state->released) {
    return;
  }
  for (size_t i = 0; i < state->entries.length; i++) {
    ImportEntry* entry = (ImportEntry*)VectorGet(&state->entries, i);
    if (compiler != NULL && compiler->syntax.lex != NULL &&
        compiler->syntax.lex->preprocessor != NULL) {
      for (size_t j = 0; j < entry->installed_macro_names.length; j++) {
        String* name =
            (String*)VectorGet(&entry->installed_macro_names, j);
        PreprocessorUndefineMacro(compiler->syntax.lex->preprocessor, name);
      }
    }
    if (entry->module != NULL) {
      if (entry->installed) {
        ModuleInstallRecordRollback(&entry->install_record);
        entry->installed = false;
      }
      LoadedModuleReleaseGraph(entry->module);
    }
  }
  state->released = true;
}

void TranslationUnitImportStateDelete(TranslationUnitImportState* state) {
  if (state == NULL) {
    return;
  }
  if (!state->released) {
    TranslationUnitImportStateRelease(state);
  }
  for (size_t i = 0; i < state->entries.length; i++) {
    ImportEntry* entry = (ImportEntry*)VectorGet(&state->entries, i);
    if (entry->module != NULL) {
      LoadedModuleDestruct(entry->module);
      free(entry->module);
      entry->module = NULL;
    }
    ModuleInstallRecordDestruct(&entry->install_record);
    for (size_t j = 0; j < entry->installed_macro_names.length; j++) {
      StringDelete((String*)VectorGet(&entry->installed_macro_names, j));
    }
    VectorDestruct(&entry->installed_macro_names);
    StringDestruct(&entry->name);
    free(entry);
  }
  VectorDestruct(&state->entries);
  for (size_t i = 0; i < state->mappings.length; i++) {
    ModulePathMapping* mapping =
        (ModulePathMapping*)VectorGet(&state->mappings, i);
    StringDestruct(&mapping->name);
    StringDestruct(&mapping->path);
    free(mapping);
  }
  VectorDestruct(&state->mappings);
  VectorDestruct(&state->search_paths);
  free(state);
}

const char* TranslationUnitImportStateLastError(
    TranslationUnitImportState* state) {
  if (state == NULL || state->last_error[0] == '\0') {
    return NULL;
  }
  return state->last_error;
}

static bool CurrentTranslationUnitOwnsPartition(const char* module_name) {
  const char* colon = module_name != NULL ? strchr(module_name, ':') : NULL;
  if (colon == NULL || compiler == NULL ||
      !compiler->module_unit.is_module_unit) {
    return false;
  }
  size_t primary_length = (size_t)(colon - module_name);
  return compiler->module_unit.id.name.length == primary_length &&
         strncmp(compiler->module_unit.id.name.value, module_name,
                 primary_length) == 0;
}

static bool LoadModuleEntry(TranslationUnitImportState* state,
                            const char* module_name, bool dependency_load,
                            ImportEntry** result) {
  if (state == NULL || state->released || module_name == NULL ||
      module_name[0] == '\0') {
    return false;
  }
  if (compiler == NULL) {
    SetLastError(state, "no active compiler");
    return false;
  }

  ImportEntry* entry = FindEntry(state, module_name);
  if (entry != NULL) {
    switch (entry->status) {
      case kModuleImportLoaded:
        *result = entry;
        return true;
      case kModuleImportLoading:
        SetLastError(state, "circular module import");
        return false;
      case kModuleImportFailed:
        SetLastError(state, entry->error[0] != '\0' ? entry->error
                                                    : "module import failed");
        return false;
      case kModuleImportAbsent:
        break;
    }
  }

  entry = FindOrCreateEntry(state, module_name);
  entry->status = kModuleImportLoading;
  entry->installed = false;
  entry->error[0] = '\0';

  char path[4096];
  if (!ResolveModulePath(state, module_name, path, sizeof(path))) {
    entry->status = kModuleImportFailed;
    snprintf(entry->error, sizeof(entry->error),
             "no prebuilt module file found for '%s'", module_name);
    SetLastError(state, entry->error);
    return false;
  }

  LoadedModule* loaded = calloc(1, sizeof(LoadedModule));
  if (!ModuleLoad(path, loaded)) {
    free(loaded);
    entry->status = kModuleImportFailed;
    snprintf(entry->error, sizeof(entry->error),
             "failed to read module archive '%s'", path);
    SetLastError(state, entry->error);
    return false;
  }

  const char* target = compiler->target_triple.canonical.value;
  if (compiler->target_triple.os == kTargetOSNone &&
      strcmp(loaded->target_triple.value,
             compiler->target_triple.architecture.value) == 0) {
    // Accept archives produced before canonical triples were serialized for
    // OS-neutral interpreter profiles. Native OS profiles always require
    // their full canonical triple.
    target = compiler->target_triple.architecture.value;
  }
  char validate_err[256];
  if (!ModuleValidateLoadedForImport(
          module_name, target, loaded->module_name.value,
          loaded->target_triple.value, validate_err, sizeof(validate_err))) {
    entry->module = loaded;
    entry->status = kModuleImportFailed;
    snprintf(entry->error, sizeof(entry->error), "%s", validate_err);
    SetLastError(state, entry->error);
    return false;
  }
  if (!dependency_load &&
      (loaded->flags & (kModuleArchiveInterfacePartition |
                        kModuleArchiveInternalPartition)) != 0 &&
      !CurrentTranslationUnitOwnsPartition(module_name)) {
    entry->module = loaded;
    entry->status = kModuleImportFailed;
    snprintf(entry->error, sizeof(entry->error),
             "module partition '%s' can only be imported by a unit of module "
             "'%.*s'",
             module_name, (int)(strchr(module_name, ':') - module_name),
             module_name);
    SetLastError(state, entry->error);
    return false;
  }

  entry->module = loaded;
  for (size_t i = 0; i < loaded->dependencies.length; i++) {
    String* dependency = (String*)VectorGet(&loaded->dependencies, i);
    ImportEntry* dependency_entry = NULL;
    if (!LoadModuleEntry(state, dependency->value, true, &dependency_entry)) {
      const char* detail = TranslationUnitImportStateLastError(state);
      entry->status = kModuleImportFailed;
      snprintf(entry->error, sizeof(entry->error),
               "dependency '%s' failed: %s", dependency->value,
               detail != NULL ? detail : "module import failed");
      SetLastError(state, entry->error);
      return false;
    }
  }

  entry->status = kModuleImportLoaded;
  *result = entry;
  return true;
}

static bool InstallEntryWithReexports(TranslationUnitImportState* state,
                                      ImportEntry* entry,
                                      Vector* newly_installed) {
  if (entry == NULL || entry->module == NULL ||
      entry->status != kModuleImportLoaded) {
    SetLastError(state, "module graph is not loaded");
    return false;
  }
  if (entry->installed) {
    return true;
  }

  for (size_t i = 0; i < entry->module->reexports.length; i++) {
    String* name = (String*)VectorGet(&entry->module->reexports, i);
    ImportEntry* dependency = FindEntry(state, name->value);
    if (dependency != NULL && dependency->module != NULL &&
        (dependency->module->flags & kModuleArchiveInternalPartition) != 0) {
      SetLastError(state, "an internal module partition cannot be re-exported");
      return false;
    }
    if (dependency == NULL ||
        !InstallEntryWithReexports(state, dependency, newly_installed)) {
      if (TranslationUnitImportStateLastError(state) == NULL) {
        SetLastError(state, "re-exported module is not loaded");
      }
      return false;
    }
  }

  if (!ModuleInstallLoadedTracked(entry->module, &entry->install_record)) {
    entry->status = kModuleImportFailed;
    const char* detail = ModuleInstallLastError();
    snprintf(entry->error, sizeof(entry->error), "%s",
             detail != NULL ? detail : "failed to install module");
    SetLastError(state, entry->error);
    return false;
  }

  entry->installed = true;
  VectorAppend(newly_installed, entry);
  return true;
}

static void RollbackInstallTransaction(Vector* newly_installed) {
  for (size_t i = newly_installed->length; i > 0; i--) {
    ImportEntry* entry = (ImportEntry*)VectorGet(newly_installed, i - 1);
    ModuleInstallRecordRollback(&entry->install_record);
    ModuleInstallRecordDestruct(&entry->install_record);
    ModuleInstallRecordInit(&entry->install_record);
    entry->installed = false;
    if (entry->status == kModuleImportFailed) {
      entry->status = kModuleImportLoaded;
    }
  }
}

static void RollbackImportedMacros(Vector* newly_installed) {
  if (compiler == NULL || compiler->syntax.lex == NULL ||
      compiler->syntax.lex->preprocessor == NULL) {
    return;
  }
  Preprocessor* preprocessor = compiler->syntax.lex->preprocessor;
  for (size_t i = newly_installed->length; i > 0; i--) {
    ImportEntry* entry = (ImportEntry*)VectorGet(newly_installed, i - 1);
    for (size_t j = entry->installed_macro_names.length; j > 0; j--) {
      String* name =
          (String*)VectorGet(&entry->installed_macro_names, j - 1);
      PreprocessorUndefineMacro(preprocessor, name);
      StringDelete(name);
    }
    VectorClear(&entry->installed_macro_names);
  }
}

static bool ImportTransactionMacros(TranslationUnitImportState* state,
                                    Vector* newly_installed) {
  if (compiler == NULL || compiler->syntax.lex == NULL ||
      compiler->syntax.lex->preprocessor == NULL) {
    return true;
  }
  Preprocessor* preprocessor = compiler->syntax.lex->preprocessor;
  for (size_t i = 0; i < newly_installed->length; i++) {
    ImportEntry* entry = (ImportEntry*)VectorGet(newly_installed, i);
    for (size_t j = 0; j < entry->module->header_macros.length; j++) {
      Macro* macro = (Macro*)VectorGet(&entry->module->header_macros, j);
      bool inserted = false;
      if (!PreprocessorImportMacro(preprocessor, macro, &inserted)) {
        snprintf(entry->error, sizeof(entry->error),
                 "conflicting imported macro definition for '%s'",
                 macro->name.value);
        SetLastError(state, entry->error);
        return false;
      }
      if (inserted) {
        VectorAppend(&entry->installed_macro_names,
                     NewString(macro->name.value));
      }
    }
  }
  return true;
}

bool TranslationUnitImportStateImport(TranslationUnitImportState* state,
                                      const char* module_name) {
  ImportEntry* entry = NULL;
  if (!LoadModuleEntry(state, module_name, false, &entry)) {
    return false;
  }
  if ((entry->module->flags & (kModuleArchiveInterfacePartition |
                               kModuleArchiveInternalPartition)) != 0 &&
      !CurrentTranslationUnitOwnsPartition(module_name)) {
    char error[256];
    const char* colon = strchr(module_name, ':');
    snprintf(error, sizeof(error),
             "module partition '%s' can only be imported by a unit of module "
             "'%.*s'",
             module_name, (int)(colon - module_name), module_name);
    SetLastError(state, error);
    return false;
  }

  Vector newly_installed;
  VectorInit(&newly_installed);
  if (!InstallEntryWithReexports(state, entry, &newly_installed)) {
    RollbackInstallTransaction(&newly_installed);
    VectorDestruct(&newly_installed);
    return false;
  }
  if (!ImportTransactionMacros(state, &newly_installed)) {
    RollbackImportedMacros(&newly_installed);
    RollbackInstallTransaction(&newly_installed);
    VectorDestruct(&newly_installed);
    return false;
  }
  VectorDestruct(&newly_installed);
  SetLastError(state, NULL);
  return true;
}

void TranslationUnitImportStateMarkLoadingForTest(
    TranslationUnitImportState* state, const char* module_name) {
  ImportEntry* entry = FindOrCreateEntry(state, module_name);
  entry->status = kModuleImportLoading;
  entry->installed = false;
  entry->error[0] = '\0';
}
