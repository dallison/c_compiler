//
//  module_archive.c
//  c_compiler
//
//  AR-based container for serialized C++20 module interfaces.  See
//  module_archive.h.
//

#include "module_archive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ar.h"
#include "preprocessor.h"
#include "serialize.h"
#include "symbol.h"
#include "symbol_table.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_internal.h"
#include "type_member.h"

// MODULE-member field numbers.
enum {
  kModule_format_version = 1,
  kModule_module_name = 2,
  kModule_target_triple = 3,
  kModule_compiler_version = 4,
  kModule_flags = 5,
  kModule_root_count = 6,
  kModule_root_handle = 7,  // Repeated.
  kModule_ns_root_count = 8,
  kModule_ns_root_handle = 9,  // Repeated.
  kModule_dependency = 10,    // Repeated logical module name.
  kModule_reexport = 11,      // Repeated logical module name.
  kModule_header_macro = 12,  // Repeated nested macro definition.
};

enum {
  kMacro_name = 1,
  kMacro_replacement = 2,
  kMacro_is_function_like = 3,
  kMacro_varargs = 4,
  kMacro_arg = 5,
};

// Maps each serializable pool kind to its archive member name.  The string pool
// is handled separately (STRINGS).  Order matters for read: strings first, then
// pools in dependency-tolerant order (two-pass read resolves cross-references,
// so ordering only affects allocation, not correctness).
typedef struct {
  SerialKind kind;
  const char* member;
} PoolMember;

static const PoolMember kPoolMembers[] = {
    {kSerialKindType, "TYPES"},        {kSerialKindSymbol, "SYMBOLS"},
    {kSerialKindStruct, "STRUCTS"},    {kSerialKindEnum, "ENUMS"},
    {kSerialKindStructMember, "MEMBERS"}, {kSerialKindNamespace, "NAMESPACES"},
    {kSerialKindAST, "AST"},
};
#define kNumPoolMembers (sizeof(kPoolMembers) / sizeof(kPoolMembers[0]))

static char g_module_write_error[256];

const char* ModuleWriteLastError(void) {
  return g_module_write_error[0] != '\0' ? g_module_write_error : NULL;
}

static bool ValidateReachableSymbols(SerializeContext* ctx,
                                     bool allow_internal_reachability) {
  Vector* symbols = &ctx->objects[kSerialKindSymbol];
  for (size_t i = 0; i < symbols->length; i++) {
    Symbol* symbol = (Symbol*)VectorGet(symbols, i);
    if (symbol == NULL || symbol->flags.is_local ||
        symbol->flags.is_argument || symbol->flags.is_template_parameter) {
      continue;
    }
    if (symbol->flags.is_module_private) {
      snprintf(g_module_write_error, sizeof(g_module_write_error),
               "exported interface reaches private-fragment declaration '%s'",
               symbol->name.value);
      return false;
    }
    if (!allow_internal_reachability && !symbol->flags.is_exported &&
        symbol->cxx_linkage == kCXXLinkageInternal) {
      snprintf(g_module_write_error, sizeof(g_module_write_error),
               "exported interface exposes internal-linkage declaration '%s'",
               symbol->name.value);
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
// Write.
// ---------------------------------------------------------------------------

static void WriteStringField(WireBuffer* buf, int field, const char* s) {
  if (s == NULL) {
    return;
  }
  WireWriteString(buf, field, s, strlen(s));
}

static void WriteStringVector(WireBuffer* buf, int field, Vector* strings) {
  if (strings == NULL) {
    return;
  }
  for (size_t i = 0; i < strings->length; i++) {
    String* value = (String*)VectorGet(strings, i);
    if (value != NULL) {
      WriteStringField(buf, field, value->value);
    }
  }
}

static void WriteHeaderMacros(WireBuffer* buf, Vector* macros) {
  if (macros == NULL) {
    return;
  }
  for (size_t i = 0; i < macros->length; i++) {
    Macro* macro = (Macro*)VectorGet(macros, i);
    if (macro == NULL || macro->undefined) {
      continue;
    }
    WireBuffer record;
    WireBufferInitOwned(&record, 32);
    WriteStringField(&record, kMacro_name, macro->name.value);
    WireWriteBytes(&record, kMacro_replacement,
                   macro->replacement_text.value,
                   macro->replacement_text.length);
    WireWriteBool(&record, kMacro_is_function_like, macro->is_function_like);
    WireWriteBool(&record, kMacro_varargs, macro->varargs);
    for (size_t j = 0; j < macro->args.length; j++) {
      String* arg = (String*)VectorGet(&macro->args, j);
      WriteStringField(&record, kMacro_arg, arg->value);
    }
    WireWriteBytes(buf, kModule_header_macro, WireBufferData(&record),
                   WireBufferSize(&record));
    WireBufferDestruct(&record);
  }
}

bool ModuleWrite(const char* path, const ModuleWriteRequest* req) {
  g_module_write_error[0] = '\0';
  SerializeRegisterAllKinds();

  SerializeContext ctx;
  SerializeContextInit(&ctx);
  ctx.writing_module_interface = true;
  ctx.writing_internal_partition =
      (req->flags & kModuleArchiveInternalPartition) != 0;

  // Intern the exported roots, remembering their pool handles.
  Vector root_handles;
  VectorInit(&root_handles);
  if (req->root_symbols != NULL) {
    for (size_t i = 0; i < req->root_symbols->length; i++) {
      void* sym = VectorGet(req->root_symbols, i);
      SerialHandle h = SerializeIntern(&ctx, kSerialKindSymbol, sym);
      VectorAppend(&root_handles, (void*)(intptr_t)h);
    }
  }
  Vector ns_root_handles;
  VectorInit(&ns_root_handles);
  if (req->root_namespaces != NULL) {
    for (size_t i = 0; i < req->root_namespaces->length; i++) {
      void* ns = VectorGet(req->root_namespaces, i);
      SerialHandle h = SerializeIntern(&ctx, kSerialKindNamespace, ns);
      VectorAppend(&ns_root_handles, (void*)(intptr_t)h);
    }
  }

  // Serialize the whole graph reachable from the roots.
  if (!SerializeContextDrain(&ctx)) {
    VectorDestruct(&root_handles);
    VectorDestruct(&ns_root_handles);
    SerializeContextDestruct(&ctx);
    return false;
  }
  if (!ValidateReachableSymbols(
          &ctx, (req->flags & kModuleArchiveHeaderUnit) != 0)) {
    VectorDestruct(&root_handles);
    VectorDestruct(&ns_root_handles);
    SerializeContextDestruct(&ctx);
    return false;
  }

  // Build the MODULE header member.
  WireBuffer module_buf;
  WireBufferInitOwned(&module_buf, 64);
  WireWriteRaw(&module_buf, MODULE_MAGIC, MODULE_MAGIC_LEN);
  WireWriteVarint(&module_buf, kModule_format_version, MODULE_FORMAT_VERSION);
  WriteStringField(&module_buf, kModule_module_name, req->module_name);
  WriteStringField(&module_buf, kModule_target_triple, req->target_triple);
  WriteStringField(&module_buf, kModule_compiler_version, req->compiler_version);
  WireWriteVarint(&module_buf, kModule_flags, req->flags);
  WireWriteVarint(&module_buf, kModule_root_count,
                  (uint64_t)root_handles.length);
  for (size_t i = 0; i < root_handles.length; i++) {
    SerialHandle h = (SerialHandle)(intptr_t)VectorGet(&root_handles, i);
    WireWriteVarint(&module_buf, kModule_root_handle, h);
  }
  WireWriteVarint(&module_buf, kModule_ns_root_count,
                  (uint64_t)ns_root_handles.length);
  for (size_t i = 0; i < ns_root_handles.length; i++) {
    SerialHandle h = (SerialHandle)(intptr_t)VectorGet(&ns_root_handles, i);
    WireWriteVarint(&module_buf, kModule_ns_root_handle, h);
  }
  WriteStringVector(&module_buf, kModule_dependency, req->dependencies);
  WriteStringVector(&module_buf, kModule_reexport, req->reexports);
  WriteHeaderMacros(&module_buf, req->header_macros);

  // Build the STRINGS member and every pool member.  Kept alive until the
  // archive is written (the builder stores content pointers, not copies).
  WireBuffer strings_buf;
  WireBufferInitOwned(&strings_buf, 64);
  SerializeWriteStringPool(&ctx, &strings_buf);

  WireBuffer pool_bufs[kNumPoolMembers];
  for (size_t i = 0; i < kNumPoolMembers; i++) {
    WireBufferInitOwned(&pool_bufs[i], 64);
    SerializeWritePool(&ctx, kPoolMembers[i].kind, &pool_bufs[i]);
  }

  bool ok = !ctx.error && !WireBufferHasError(&module_buf) &&
            !WireBufferHasError(&strings_buf);
  for (size_t i = 0; i < kNumPoolMembers; i++) {
    ok = ok && !WireBufferHasError(&pool_bufs[i]);
  }

  if (ok) {
    ARArchiveBuilder builder;
    ARArchiveBuilderInit(&builder, path);
    ARFile* module_file = ARArchiveBuilderAddFile(
        &builder, "MODULE", WireBufferSize(&module_buf), 0, 0, 0644, 0,
        (void*)WireBufferData(&module_buf));
    ARArchiveBuilderAddFile(&builder, "STRINGS", WireBufferSize(&strings_buf), 0,
                            0, 0644, 0, (void*)WireBufferData(&strings_buf));
    for (size_t i = 0; i < kNumPoolMembers; i++) {
      ARArchiveBuilderAddFile(&builder, kPoolMembers[i].member,
                              WireBufferSize(&pool_bufs[i]), 0, 0, 0644, 0,
                              (void*)WireBufferData(&pool_bufs[i]));
    }
    // At least one symbol is required so the AR writer emits a symbol table and
    // its precomputed file offsets stay consistent.
    ARArchiveBuilderAddSymbol(&builder, module_file, "__dcm_module__");
    ok = ARArchiveBuilderWrite(&builder);
    ARArchiveBuilderDestruct(&builder);
  }

  WireBufferDestruct(&module_buf);
  WireBufferDestruct(&strings_buf);
  for (size_t i = 0; i < kNumPoolMembers; i++) {
    WireBufferDestruct(&pool_bufs[i]);
  }
  VectorDestruct(&root_handles);
  VectorDestruct(&ns_root_handles);
  SerializeContextDestruct(&ctx);
  return ok;
}

// ---------------------------------------------------------------------------
// Read.
// ---------------------------------------------------------------------------

// Reads the raw bytes of the named member into a freshly malloc'd buffer.
// Returns NULL if the member is absent.  *out_len receives the length.
static void* ReadMemberBytes(ARArchive* archive, FILE* fp, const char* name,
                             size_t* out_len) {
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = (ARFile*)VectorGet(&archive->files, i);
    if (StringEqual(&file->filename, name)) {
      void* data = malloc((size_t)file->size);
      fseek(fp, file->file_offset, SEEK_SET);
      if (file->size > 0 &&
          fread(data, 1, (size_t)file->size, fp) != (size_t)file->size) {
        free(data);
        return NULL;
      }
      *out_len = (size_t)file->size;
      return data;
    }
  }
  return NULL;
}

// `dest` must already be initialized (ModuleLoad initializes the header strings
// up front so cleanup is always safe); this overwrites its contents in place.
static void ReadStringFieldInto(WireBuffer* buf, String* dest) {
  const void* data;
  size_t len;
  StringClear(dest);
  if (WireReadBytes(buf, &data, &len)) {
    StringAppendSegment(dest, (const char*)data, len);
  }
}

static void DestructOwnedStringVector(Vector* strings) {
  VectorDestructWithContents(strings, (VectorElementDestructor)StringDelete,
                             /*free_element=*/false);
}

static void DestructOwnedMacroVector(Vector* macros) {
  for (size_t i = 0; i < macros->length; i++) {
    Macro* macro = (Macro*)VectorGet(macros, i);
    MacroDestruct(macro);
    free(macro);
  }
  VectorDestruct(macros);
}

static Macro* ReadHeaderMacro(const void* data, size_t len) {
  WireBuffer in;
  WireBufferInitReader(&in, data, len);
  String name;
  String replacement;
  StringInit(&name, "");
  StringInit(&replacement, "");
  Vector args;
  VectorInit(&args);
  bool is_function_like = false;
  bool varargs = false;
  while (!WireBufferEof(&in) && !WireBufferHasError(&in)) {
    int field;
    WireType wt;
    if (!WireReadTag(&in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kMacro_name:
        ReadStringFieldInto(&in, &name);
        break;
      case kMacro_replacement:
        ReadStringFieldInto(&in, &replacement);
        break;
      case kMacro_is_function_like:
        WireReadBool(&in, &is_function_like);
        break;
      case kMacro_varargs:
        WireReadBool(&in, &varargs);
        break;
      case kMacro_arg: {
        String* arg = NewString("");
        ReadStringFieldInto(&in, arg);
        VectorAppend(&args, arg);
        break;
      }
      default:
        WireSkip(&in, wt);
        break;
    }
  }
  Macro* macro = NULL;
  if (!WireBufferHasError(&in) && name.length > 0) {
    macro = NewMacro(name.value, is_function_like, varargs, &args,
                     &replacement, SOURCE_LOCATION_MISSING);
  }
  if (macro == NULL) {
    for (size_t i = 0; i < args.length; i++) {
      StringDelete((String*)VectorGet(&args, i));
    }
  }
  // NewMacro takes ownership of the argument strings on success.
  VectorDestruct(&args);
  StringDestruct(&name);
  StringDestruct(&replacement);
  return macro;
}

// Parses the MODULE member: validates magic + version, fills header fields, and
// collects the exported root handles.
static bool ParseModuleHeader(LoadedModule* out, const void* blob, size_t len,
                              Vector* root_handles, Vector* ns_root_handles) {
  WireBuffer in;
  WireBufferInitReader(&in, blob, len);

  // Header strings are pre-initialized by ModuleLoad, so cleanup is safe even
  // if we bail out here.
  char magic[MODULE_MAGIC_LEN];
  if (!WireReadRaw(&in, magic, MODULE_MAGIC_LEN) ||
      memcmp(magic, MODULE_MAGIC, MODULE_MAGIC_LEN) != 0) {
    return false;
  }

  while (!WireBufferEof(&in) && !WireBufferHasError(&in)) {
    int field;
    WireType wt;
    if (!WireReadTag(&in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kModule_format_version: {
        uint64_t v;
        WireReadVarint(&in, &v);
        out->format_version = (uint32_t)v;
        break;
      }
      case kModule_module_name:
        ReadStringFieldInto(&in, &out->module_name);
        break;
      case kModule_target_triple:
        ReadStringFieldInto(&in, &out->target_triple);
        break;
      case kModule_compiler_version:
        ReadStringFieldInto(&in, &out->compiler_version);
        break;
      case kModule_flags: {
        uint64_t v;
        WireReadVarint(&in, &v);
        out->flags = (uint32_t)v;
        break;
      }
      case kModule_root_count: {
        uint64_t v;
        WireReadVarint(&in, &v);  // Advisory; roots come from repeated field 7.
        break;
      }
      case kModule_root_handle: {
        uint64_t v;
        WireReadVarint(&in, &v);
        VectorAppend(root_handles, (void*)(intptr_t)(SerialHandle)v);
        break;
      }
      case kModule_ns_root_count: {
        uint64_t v;
        WireReadVarint(&in, &v);  // Advisory; roots come from repeated field 9.
        break;
      }
      case kModule_ns_root_handle: {
        uint64_t v;
        WireReadVarint(&in, &v);
        VectorAppend(ns_root_handles, (void*)(intptr_t)(SerialHandle)v);
        break;
      }
      case kModule_dependency: {
        String* dependency = NewString("");
        ReadStringFieldInto(&in, dependency);
        VectorAppend(&out->dependencies, dependency);
        break;
      }
      case kModule_reexport: {
        String* reexport = NewString("");
        ReadStringFieldInto(&in, reexport);
        VectorAppend(&out->reexports, reexport);
        break;
      }
      case kModule_header_macro: {
        const void* data;
        size_t len;
        if (WireReadBytes(&in, &data, &len)) {
          Macro* macro = ReadHeaderMacro(data, len);
          if (macro == NULL) {
            return false;
          }
          VectorAppend(&out->header_macros, macro);
        }
        break;
      }
      default:
        WireSkip(&in, wt);
        break;
    }
  }

  if (WireBufferHasError(&in) ||
      out->format_version != MODULE_FORMAT_VERSION) {
    return false;
  }
  return true;
}

static void DetachImportedTemplateParameterVector(Vector* vec) {
  if (vec == NULL) {
    return;
  }
  VectorDestruct(vec);
}

static void RepairImportedTemplateParameterVector(Vector* vec) {
  if (vec == NULL || vec->length == 0) {
    return;
  }
  Vector owned;
  VectorInit(&owned);
  for (size_t i = 0; i < vec->length; i++) {
    TemplateParameter* param = (TemplateParameter*)VectorGet(vec, i);
    if (param != NULL) {
      VectorAppend(&owned, TemplateParameterCopy(param));
    }
  }
  DetachImportedTemplateParameterVector(vec);
  *vec = owned;
}

static void RepairImportedFunctionTemplateParameters(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return;
  }
  RepairImportedTemplateParameterVector(&func->info.function.template_parameters);
  if (func->info.function.template_parameters.length > 0) {
    func->info.function.template_parameter_count =
        (int)func->info.function.template_parameters.length;
  }
}

static void RepairImportedStructTemplateParameters(Struct* st) {
  if (st == NULL) {
    return;
  }
  RepairImportedTemplateParameterVector(&st->template_parameters);
  if (st->template_parameters.length > 0) {
    st->template_parameter_count = (int)st->template_parameters.length;
  }
}

static void RepairImportedPartialSpecializationParameters(Vector* partials) {
  if (partials == NULL) {
    return;
  }
  for (size_t i = 0; i < partials->length; i++) {
    ClassTemplatePartialSpecialization* partial =
        (ClassTemplatePartialSpecialization*)VectorGet(partials, i);
    if (partial != NULL) {
      RepairImportedTemplateParameterVector(&partial->template_parameters);
    }
  }
}

static void RepairDeserializedModuleGraph(DeserializeContext* ctx) {
  if (ctx == NULL) {
    return;
  }
  Vector* struct_pool = &ctx->objects[kSerialKindStruct];
  for (size_t i = 0; i < struct_pool->length; i++) {
    Struct* st = (Struct*)VectorGet(struct_pool, i);
    if (st != NULL) {
      StructRebuildMemberLookupTables(st);
      CollectCXXVirtualBases(st);
      // A type record that named this class was read before the class itself
      // and so took a size of zero from it.  Nothing lays out an imported
      // class, which is where a parsed one propagates its finished size, so
      // push it out here now that the whole graph is in.
      TypeRecordSyncStructSizes(st);
      if (st->is_template) {
        RepairImportedStructTemplateParameters(st);
      }
      RepairImportedPartialSpecializationParameters(
          &st->partial_specializations);
    }
  }
  Vector* sym_pool = &ctx->objects[kSerialKindSymbol];
  for (size_t i = 0; i < sym_pool->length; i++) {
    Symbol* sym = (Symbol*)VectorGet(sym_pool, i);
    if (sym != NULL && sym->alias_template != NULL) {
      RepairImportedTemplateParameterVector(
          &sym->alias_template->parameters);
    }
    if (sym != NULL && sym->variable_template != NULL) {
      RepairImportedTemplateParameterVector(
          &sym->variable_template->parameters);
      RepairImportedPartialSpecializationParameters(
          &sym->variable_template->partial_specializations);
    }
    if (sym != NULL && sym->type != NULL && TypeIsFunction(sym->type)) {
      if (sym->type->info.function.symbol == NULL) {
        sym->type->info.function.symbol = sym;
      }
      if (sym->type->info.function.body != NULL && sym->value.func_defn == NULL) {
        sym->value.func_defn = sym;
      }
      if (sym->flags.is_template ||
          sym->type->info.function.template_parameters.length > 0) {
        RepairImportedFunctionTemplateParameters(sym->type);
      }
    }
  }
}

bool ModuleLoad(const char* path, LoadedModule* out) {
  SerializeRegisterAllKinds();

  // Initialize the header fields up front so every early-exit cleanup path can
  // safely destruct them.
  StringInit(&out->module_name, "");
  StringInit(&out->target_triple, "");
  StringInit(&out->compiler_version, "");
  VectorInit(&out->dependencies);
  VectorInit(&out->reexports);
  VectorInit(&out->header_macros);
  out->format_version = 0;
  out->flags = 0;
  out->graph_released = false;

  FILE* fp = fopen(path, "r");
  if (fp == NULL) {
    StringDestruct(&out->module_name);
    StringDestruct(&out->target_triple);
    StringDestruct(&out->compiler_version);
    DestructOwnedStringVector(&out->dependencies);
    DestructOwnedStringVector(&out->reexports);
    DestructOwnedMacroVector(&out->header_macros);
    return false;
  }

  ARArchive archive;
  ARArchiveInit(&archive, path);
  if (!ARArchiveOpen(&archive, fp)) {
    ARArchiveDestruct(&archive);
    fclose(fp);
    StringDestruct(&out->module_name);
    StringDestruct(&out->target_triple);
    StringDestruct(&out->compiler_version);
    DestructOwnedStringVector(&out->dependencies);
    DestructOwnedStringVector(&out->reexports);
    DestructOwnedMacroVector(&out->header_macros);
    return false;
  }

  // Read every member we need into owned buffers.  They must all stay alive
  // through pass 2 (the deserializer holds pointers into them).
  Vector owned_buffers;  // void* content blobs to free on the way out.
  VectorInit(&owned_buffers);

  bool ok = true;
  Vector root_handles;
  VectorInit(&root_handles);
  Vector ns_root_handles;
  VectorInit(&ns_root_handles);

  size_t module_len = 0;
  void* module_bytes = ReadMemberBytes(&archive, fp, "MODULE", &module_len);
  if (module_bytes == NULL) {
    ok = false;
  } else {
    VectorAppend(&owned_buffers, module_bytes);
    ok = ParseModuleHeader(out, module_bytes, module_len, &root_handles,
                           &ns_root_handles);
  }

  DeserializeContext* ctx = &out->ctx;
  if (ok) {
    DeserializeContextInit(ctx);

    // Pass 1: strings, then every pool (allocate empty objects).
    size_t strings_len = 0;
    void* strings_bytes =
        ReadMemberBytes(&archive, fp, "STRINGS", &strings_len);
    if (strings_bytes == NULL) {
      ok = false;
    } else {
      VectorAppend(&owned_buffers, strings_bytes);
      WireBuffer sb;
      WireBufferInitReader(&sb, strings_bytes, strings_len);
      ok = SerializeReadStringPool(ctx, &sb);
    }

    for (size_t i = 0; i < kNumPoolMembers && ok; i++) {
      size_t plen = 0;
      void* pbytes =
          ReadMemberBytes(&archive, fp, kPoolMembers[i].member, &plen);
      if (pbytes == NULL) {
        ok = false;
        break;
      }
      VectorAppend(&owned_buffers, pbytes);
      WireBuffer pb;
      WireBufferInitReader(&pb, pbytes, plen);
      ok = SerializeReadPool(ctx, kPoolMembers[i].kind, &pb);
    }

    // Pass 2: fill every object, resolving handle references.
    if (ok) {
      ok = DeserializeContextResolve(ctx);
    }
    if (ok) {
      RepairDeserializedModuleGraph(ctx);
    }

    // Resolve the exported roots.
    if (ok) {
      VectorInit(&out->root_symbols);
      for (size_t i = 0; i < root_handles.length; i++) {
        SerialHandle h = (SerialHandle)(intptr_t)VectorGet(&root_handles, i);
        void* sym = DeserializeResolve(ctx, kSerialKindSymbol, h);
        VectorAppend(&out->root_symbols, sym);
      }
      VectorInit(&out->root_namespaces);
      for (size_t i = 0; i < ns_root_handles.length; i++) {
        SerialHandle h = (SerialHandle)(intptr_t)VectorGet(&ns_root_handles, i);
        void* ns = DeserializeResolve(ctx, kSerialKindNamespace, h);
        VectorAppend(&out->root_namespaces, ns);
      }
      ok = !ctx->error;
    }

    if (!ok) {
      DeserializeContextDestruct(ctx);
    }
  }

  // Keep the content buffers alive in the LoadedModule on success; free them
  // on failure.
  if (ok) {
    out->owned_buffers = owned_buffers;
  } else {
    for (size_t i = 0; i < owned_buffers.length; i++) {
      free(VectorGet(&owned_buffers, i));
    }
    VectorDestruct(&owned_buffers);
    StringDestruct(&out->module_name);
    StringDestruct(&out->target_triple);
    StringDestruct(&out->compiler_version);
    DestructOwnedStringVector(&out->dependencies);
    DestructOwnedStringVector(&out->reexports);
    DestructOwnedMacroVector(&out->header_macros);
  }

  VectorDestruct(&root_handles);
  VectorDestruct(&ns_root_handles);
  ARArchiveDestruct(&archive);
  fclose(fp);
  return ok;
}

void LoadedModuleReleaseGraph(LoadedModule* out) {
  if (out == NULL || out->graph_released) {
    return;
  }

  DeserializeContext* ctx = &out->ctx;
  // Imported deduction guides are also entries in the symbol pool.  Structs
  // normally own and delete their guide symbols, so detach these reference
  // vectors before pool-owned symbols are destructed individually below.
  Vector* struct_pool = &ctx->objects[kSerialKindStruct];
  for (size_t i = 0; i < struct_pool->length; i++) {
    Struct* str = (Struct*)VectorGet(struct_pool, i);
    if (str != NULL) {
      VectorDestruct(&str->deduction_guides);
      VectorInit(&str->deduction_guides);
    }
  }

  Vector* ns_pool = &ctx->objects[kSerialKindNamespace];
  for (size_t i = 0; i < ns_pool->length; i++) {
    Namespace* ns = (Namespace*)VectorGet(ns_pool, i);
    if (ns == NULL) {
      continue;
    }
    BinaryTreeDestruct(&ns->symbol_table, NULL);
    BinaryTreeDestruct(&ns->tag_table, NULL);
    for (size_t j = 0; j < ns->namespace_aliases.length; j++) {
      NamespaceAlias* alias = (NamespaceAlias*)VectorGet(&ns->namespace_aliases, j);
      StringDestruct(&alias->name);
      free(alias);
    }
    VectorDestruct(&ns->namespace_aliases);
    VectorDestruct(&ns->children);
  }

  Vector* sym_pool = &ctx->objects[kSerialKindSymbol];
  // Every symbol in an overload chain has its own pool entry.  SymbolDelete
  // normally owns and recursively deletes overload_next, so sever those links
  // before deleting pool entries individually.
  for (size_t i = 0; i < sym_pool->length; i++) {
    Symbol* sym = (Symbol*)VectorGet(sym_pool, i);
    if (sym != NULL) {
      sym->overload_next = NULL;
    }
  }
  for (size_t i = 0; i < sym_pool->length; i++) {
    Symbol* sym = (Symbol*)VectorGet(sym_pool, i);
    if (sym != NULL) {
      SymbolDestruct(sym);
    }
  }

  for (size_t i = 0; i < ns_pool->length; i++) {
    Namespace* ns = (Namespace*)VectorGet(ns_pool, i);
    if (ns == NULL) {
      continue;
    }
    StringDestruct(&ns->name);
    StringDestruct(&ns->qualified_name);
    free(ns);
    ns_pool->value.p[i] = NULL;
  }

  for (int k = 0; k < kSerialKindCount; k++) {
    if (k == kSerialKindSymbol) {
      continue;
    }
    VectorDestruct(&ctx->objects[k]);
    VectorInit(&ctx->objects[k]);
  }
  VectorDestruct(&out->root_symbols);
  VectorInit(&out->root_symbols);
  VectorDestruct(&out->root_namespaces);
  VectorInit(&out->root_namespaces);
  out->graph_released = true;
}

void LoadedModuleDestruct(LoadedModule* out) {
  if (out == NULL) {
    return;
  }
  StringDestruct(&out->module_name);
  StringDestruct(&out->target_triple);
  StringDestruct(&out->compiler_version);
  DestructOwnedStringVector(&out->dependencies);
  DestructOwnedStringVector(&out->reexports);
  DestructOwnedMacroVector(&out->header_macros);
  VectorDestruct(&out->root_symbols);
  VectorDestruct(&out->root_namespaces);
  if (out->graph_released) {
    Vector* sym_pool = &out->ctx.objects[kSerialKindSymbol];
    for (size_t i = 0; i < sym_pool->length; i++) {
      free(VectorGet(sym_pool, i));
    }
  }
  DeserializeContextDestruct(&out->ctx);
  for (size_t i = 0; i < out->owned_buffers.length; i++) {
    free(VectorGet(&out->owned_buffers, i));
  }
  VectorDestruct(&out->owned_buffers);
}
