//
//  serialize.c
//  c_compiler
//
//  Graph-aware serialization framework.  See serialize.h.
//

#include "serialize.h"

#include <stdlib.h>
#include <string.h>

#include "dstring.h"

//
// Global kind vtable / field-metadata registries.  Registration is process
// wide and idempotent, populated by SerializeRegisterAllKinds.
//
static SerialKindVtable g_kind_vtables[kSerialKindCount];
static bool g_kind_registered[kSerialKindCount];
static const WireFieldDesc* g_kind_fields[kSerialKindCount];
static size_t g_kind_field_counts[kSerialKindCount];

void SerializeRegisterKind(SerialKind kind, const SerialKindVtable* vtable) {
  g_kind_vtables[kind] = *vtable;
  g_kind_registered[kind] = true;
}

const SerialKindVtable* SerializeGetKindVtable(SerialKind kind) {
  return g_kind_registered[kind] ? &g_kind_vtables[kind] : NULL;
}

void SerializeRegisterFields(SerialKind kind, const WireFieldDesc* fields,
                             size_t count) {
  g_kind_fields[kind] = fields;
  g_kind_field_counts[kind] = count;
}

const WireFieldDesc* SerializeGetFields(SerialKind kind, size_t* count) {
  if (count != NULL) {
    *count = g_kind_field_counts[kind];
  }
  return g_kind_fields[kind];
}

// Per-object registration entry points, implemented in the *_serialize.c files.
void SerializeRegisterTypeKinds(void);
void SerializeRegisterSymbolKinds(void);
void SerializeRegisterASTKinds(void);

void SerializeRegisterAllKinds(void) {
  SerializeRegisterTypeKinds();
  SerializeRegisterSymbolKinds();
  SerializeRegisterASTKinds();
}

//
// Write side.
//
void SerializeContextInit(SerializeContext* ctx) {
  for (int k = 0; k < kSerialKindCount; k++) {
    MapInitForPointerKeys(&ctx->handle_maps[k]);
    VectorInit(&ctx->objects[k]);
    VectorInit(&ctx->serialized[k]);
    ctx->processed[k] = 0;
  }
  MapInitForCharPointerKeys(&ctx->string_map);
  VectorInit(&ctx->string_pool);
  VectorInit(&ctx->string_lens);
  ctx->writing_module_interface = false;
  ctx->writing_internal_partition = false;
  ctx->error = false;
}

static void FreeWireBuffer(void* p) {
  WireBuffer* buf = (WireBuffer*)p;
  WireBufferDestruct(buf);
}

void SerializeContextDestruct(SerializeContext* ctx) {
  for (int k = 0; k < kSerialKindCount; k++) {
    MapDestruct(&ctx->handle_maps[k]);
    VectorDestruct(&ctx->objects[k]);
    VectorDestructWithContents(&ctx->serialized[k], FreeWireBuffer, true);
  }
  MapDestruct(&ctx->string_map);
  for (size_t i = 0; i < ctx->string_pool.length; i++) {
    free(VectorGet(&ctx->string_pool, i));
  }
  VectorDestruct(&ctx->string_pool);
  VectorDestruct(&ctx->string_lens);
}

SerialHandle SerializeInternStringN(SerializeContext* ctx, const char* str,
                                    size_t len) {
  if (str == NULL) {
    return kSerialNullHandle;
  }
  char* copy = (char*)malloc(len + 1);
  memcpy(copy, str, len);
  copy[len] = '\0';
  void* found = MapFind(&ctx->string_map, (MapKeyType){.p = copy});
  if (found != NULL) {
    free(copy);
    return (SerialHandle)(intptr_t)found;
  }
  SerialHandle handle = (SerialHandle)(ctx->string_pool.length + 1);
  VectorAppend(&ctx->string_pool, copy);
  VectorAppend(&ctx->string_lens, (void*)(intptr_t)len);
  MapInsert(&ctx->string_map,
            (MapKeyValue){.key.p = copy, .value.p = (void*)(intptr_t)handle});
  return handle;
}

SerialHandle SerializeInternString(SerializeContext* ctx, String* s) {
  if (s == NULL) {
    return kSerialNullHandle;
  }
  return SerializeInternStringN(ctx, s->value, s->length);
}

SerialHandle SerializeIntern(SerializeContext* ctx, SerialKind kind,
                             void* ptr) {
  if (ptr == NULL) {
    return kSerialNullHandle;
  }
  const SerialKindVtable* vt = SerializeGetKindVtable(kind);
  if (vt != NULL && vt->can_intern != NULL && !vt->can_intern(ptr)) {
    return kSerialNullHandle;
  }
  void* found = MapFindPointerKey(&ctx->handle_maps[kind], ptr);
  if (found != NULL) {
    return (SerialHandle)(intptr_t)found;
  }
  SerialHandle handle = (SerialHandle)(ctx->objects[kind].length + 1);
  VectorAppend(&ctx->objects[kind], ptr);
  MapInsert(&ctx->handle_maps[kind],
            (MapKeyValue){.key.p = ptr, .value.p = (void*)(intptr_t)handle});
  return handle;
}

bool SerializeContextDrain(SerializeContext* ctx) {
  bool progress = true;
  while (progress && !ctx->error) {
    progress = false;
    for (int k = 0; k < kSerialKindCount; k++) {
      if (k == kSerialKindString) {
        continue;  // Strings carry no fields; nothing to drain.
      }
      const SerialKindVtable* vt = SerializeGetKindVtable((SerialKind)k);
      while (ctx->processed[k] < ctx->objects[k].length) {
        size_t i = ctx->processed[k]++;
        void* obj = VectorGet(&ctx->objects[k], i);
        WireBuffer* buf = (WireBuffer*)malloc(sizeof(WireBuffer));
        WireBufferInitOwned(buf, 32);
        if (vt == NULL || vt->write == NULL || !vt->write(ctx, buf, obj)) {
          ctx->error = true;
        }
        // Keep the buffer even on error so destruct frees it uniformly.
        VectorAppend(&ctx->serialized[k], buf);
        progress = true;
      }
    }
  }
  return !ctx->error;
}

bool SerializeWritePool(SerializeContext* ctx, SerialKind kind,
                        WireBuffer* out) {
  Vector* blobs = &ctx->serialized[kind];
  if (!WireWriteRawVarint(out, (uint64_t)blobs->length)) {
    return false;
  }
  for (size_t i = 0; i < blobs->length; i++) {
    WireBuffer* blob = (WireBuffer*)VectorGet(blobs, i);
    size_t len = WireBufferSize(blob);
    if (!WireWriteRawVarint(out, (uint64_t)len)) {
      return false;
    }
    if (!WireWriteRaw(out, WireBufferData(blob), len)) {
      return false;
    }
  }
  return true;
}

bool SerializeWriteStringPool(SerializeContext* ctx, WireBuffer* out) {
  size_t count = ctx->string_pool.length;
  if (!WireWriteRawVarint(out, (uint64_t)count)) {
    return false;
  }
  for (size_t i = 0; i < count; i++) {
    char* str = (char*)VectorGet(&ctx->string_pool, i);
    size_t len = (size_t)(intptr_t)VectorGet(&ctx->string_lens, i);
    if (!WireWriteRawVarint(out, (uint64_t)len)) {
      return false;
    }
    if (!WireWriteRaw(out, str, len)) {
      return false;
    }
  }
  return true;
}

//
// Read side.
//
void DeserializeContextInit(DeserializeContext* ctx) {
  for (int k = 0; k < kSerialKindCount; k++) {
    VectorInit(&ctx->objects[k]);
    VectorInit(&ctx->blob_ptrs[k]);
    VectorInit(&ctx->blob_lens[k]);
  }
  VectorInit(&ctx->string_pool);
  VectorInit(&ctx->string_lens);
  ctx->error = false;
}

void DeserializeContextDestruct(DeserializeContext* ctx) {
  // Note: objects[] are the freshly-allocated compiler objects handed to the
  // loader; they are intentionally NOT freed here.
  for (int k = 0; k < kSerialKindCount; k++) {
    VectorDestruct(&ctx->objects[k]);
    VectorDestruct(&ctx->blob_ptrs[k]);
    VectorDestruct(&ctx->blob_lens[k]);
  }
  for (size_t i = 0; i < ctx->string_pool.length; i++) {
    free(VectorGet(&ctx->string_pool, i));
  }
  VectorDestruct(&ctx->string_pool);
  VectorDestruct(&ctx->string_lens);
}

void* DeserializeResolve(DeserializeContext* ctx, SerialKind kind,
                         SerialHandle handle) {
  if (handle == kSerialNullHandle) {
    return NULL;
  }
  size_t index = (size_t)handle - 1;
  if (index >= ctx->objects[kind].length) {
    ctx->error = true;
    return NULL;
  }
  return VectorGet(&ctx->objects[kind], index);
}

const char* DeserializeResolveString(DeserializeContext* ctx,
                                     SerialHandle handle, size_t* len) {
  if (handle == kSerialNullHandle) {
    if (len != NULL) {
      *len = 0;
    }
    return NULL;
  }
  size_t index = (size_t)handle - 1;
  if (index >= ctx->string_pool.length) {
    ctx->error = true;
    if (len != NULL) {
      *len = 0;
    }
    return NULL;
  }
  if (len != NULL) {
    *len = (size_t)(intptr_t)VectorGet(&ctx->string_lens, index);
  }
  return (const char*)VectorGet(&ctx->string_pool, index);
}

bool SerializeReadPool(DeserializeContext* ctx, SerialKind kind,
                       WireBuffer* in) {
  const SerialKindVtable* vt = SerializeGetKindVtable(kind);
  uint64_t count;
  if (!WireReadRawVarint(in, &count)) {
    ctx->error = true;
    return false;
  }
  for (uint64_t i = 0; i < count; i++) {
    const void* blob;
    size_t len;
    if (!WireReadBytes(in, &blob, &len)) {
      ctx->error = true;
      return false;
    }
    if (vt == NULL || vt->alloc == NULL) {
      ctx->error = true;
      return false;
    }
    void* obj = vt->alloc(ctx, blob, len);
    VectorAppend(&ctx->objects[kind], obj);
    VectorAppend(&ctx->blob_ptrs[kind], (void*)blob);
    VectorAppend(&ctx->blob_lens[kind], (void*)(intptr_t)len);
  }
  return true;
}

bool SerializeReadStringPool(DeserializeContext* ctx, WireBuffer* in) {
  uint64_t count;
  if (!WireReadRawVarint(in, &count)) {
    ctx->error = true;
    return false;
  }
  for (uint64_t i = 0; i < count; i++) {
    const void* data;
    size_t len;
    if (!WireReadBytes(in, &data, &len)) {
      ctx->error = true;
      return false;
    }
    char* copy = (char*)malloc(len + 1);
    memcpy(copy, data, len);
    copy[len] = '\0';
    VectorAppend(&ctx->string_pool, copy);
    VectorAppend(&ctx->string_lens, (void*)(intptr_t)len);
  }
  return true;
}

bool DeserializeContextResolve(DeserializeContext* ctx) {
  for (int k = 0; k < kSerialKindCount && !ctx->error; k++) {
    if (k == kSerialKindString) {
      continue;
    }
    const SerialKindVtable* vt = SerializeGetKindVtable((SerialKind)k);
    for (size_t i = 0; i < ctx->objects[k].length && !ctx->error; i++) {
      void* obj = VectorGet(&ctx->objects[k], i);
      const void* blob = VectorGet(&ctx->blob_ptrs[k], i);
      size_t len = (size_t)(intptr_t)VectorGet(&ctx->blob_lens[k], i);
      WireBuffer buf;
      WireBufferInitReader(&buf, blob, len);
      if (vt == NULL || vt->read == NULL || !vt->read(ctx, &buf, obj)) {
        ctx->error = true;
      }
    }
  }
  return !ctx->error;
}
