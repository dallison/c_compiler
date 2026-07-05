//
//  module_archive.c
//  c_compiler
//
//  AR-based container for serialized C++20 module interfaces.  See
//  module_archive.h.
//

#include "module_archive.h"

#include <stdlib.h>
#include <string.h>

#include "ar.h"

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

// ---------------------------------------------------------------------------
// Write.
// ---------------------------------------------------------------------------

static void WriteStringField(WireBuffer* buf, int field, const char* s) {
  if (s == NULL) {
    return;
  }
  WireWriteString(buf, field, s, strlen(s));
}

bool ModuleWrite(const char* path, const ModuleWriteRequest* req) {
  SerializeRegisterAllKinds();

  SerializeContext ctx;
  SerializeContextInit(&ctx);

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

bool ModuleLoad(const char* path, LoadedModule* out) {
  SerializeRegisterAllKinds();

  // Initialize the header fields up front so every early-exit cleanup path can
  // safely destruct them.
  StringInit(&out->module_name, "");
  StringInit(&out->target_triple, "");
  StringInit(&out->compiler_version, "");
  out->format_version = 0;
  out->flags = 0;

  FILE* fp = fopen(path, "r");
  if (fp == NULL) {
    StringDestruct(&out->module_name);
    StringDestruct(&out->target_triple);
    StringDestruct(&out->compiler_version);
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
  }

  VectorDestruct(&root_handles);
  VectorDestruct(&ns_root_handles);
  ARArchiveDestruct(&archive);
  fclose(fp);
  return ok;
}

void LoadedModuleDestruct(LoadedModule* out) {
  StringDestruct(&out->module_name);
  StringDestruct(&out->target_triple);
  StringDestruct(&out->compiler_version);
  VectorDestruct(&out->root_symbols);
  VectorDestruct(&out->root_namespaces);
  DeserializeContextDestruct(&out->ctx);
  for (size_t i = 0; i < out->owned_buffers.length; i++) {
    free(VectorGet(&out->owned_buffers, i));
  }
  VectorDestruct(&out->owned_buffers);
}
