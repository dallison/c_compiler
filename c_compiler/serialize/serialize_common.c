//
//  serialize_common.c
//  c_compiler
//
//  Implementation of the shared per-object serialization helpers.
//

#include "serialize_common.h"

#include <stdlib.h>

void SWriteRef(SerializeContext* ctx, WireBuffer* buf, int field,
               SerialKind kind, void* ptr) {
  WireWriteVarint(buf, field, SerializeIntern(ctx, kind, ptr));
}

void SWriteStringPtr(SerializeContext* ctx, WireBuffer* buf, int field,
                     String* s) {
  WireWriteVarint(buf, field, SerializeInternString(ctx, s));
}

void SWriteStringVal(SerializeContext* ctx, WireBuffer* buf, int field,
                     String* s) {
  WireWriteVarint(buf, field, SerializeInternString(ctx, s));
}

void SWriteRefVector(SerializeContext* ctx, WireBuffer* buf, int field,
                     SerialKind kind, Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  size_t length = v == NULL ? 0 : v->length;
  WireWriteRawVarint(&tmp, (uint64_t)length);
  for (size_t i = 0; i < length; i++) {
    WireWriteRawVarint(&tmp, SerializeIntern(ctx, kind, VectorGet(v, i)));
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

void SWriteStringVector(SerializeContext* ctx, WireBuffer* buf, int field,
                        Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  size_t length = v == NULL ? 0 : v->length;
  WireWriteRawVarint(&tmp, (uint64_t)length);
  for (size_t i = 0; i < length; i++) {
    WireWriteRawVarint(&tmp, SerializeInternString(ctx, (String*)VectorGet(v, i)));
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

void* SReadRef(DeserializeContext* ctx, WireBuffer* buf, SerialKind kind) {
  uint64_t handle;
  if (!WireReadVarint(buf, &handle)) {
    return NULL;
  }
  return DeserializeResolve(ctx, kind, (SerialHandle)handle);
}

String* SReadStringPtr(DeserializeContext* ctx, WireBuffer* buf) {
  uint64_t handle;
  if (!WireReadVarint(buf, &handle)) {
    return NULL;
  }
  size_t len;
  const char* str = DeserializeResolveString(ctx, (SerialHandle)handle, &len);
  if (str == NULL) {
    return NULL;
  }
  return NewStringWithLength(str, len);
}

void SReadStringVal(DeserializeContext* ctx, WireBuffer* buf, String* dest) {
  uint64_t handle;
  if (!WireReadVarint(buf, &handle)) {
    return;
  }
  size_t len;
  const char* str = DeserializeResolveString(ctx, (SerialHandle)handle, &len);
  if (str == NULL) {
    StringSet(dest, "");
  } else {
    // StringSet copies up to a NUL; module strings never contain NUL bytes.
    StringSet(dest, str);
  }
}

void SReadRefVector(DeserializeContext* ctx, WireBuffer* buf, SerialKind kind,
                    Vector* out) {
  const void* data;
  size_t len;
  if (!WireReadBytes(buf, &data, &len)) {
    return;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  uint64_t count;
  if (!WireReadRawVarint(&sub, &count)) {
    return;
  }
  for (uint64_t i = 0; i < count; i++) {
    uint64_t handle;
    if (!WireReadRawVarint(&sub, &handle)) {
      return;
    }
    VectorAppend(out, DeserializeResolve(ctx, kind, (SerialHandle)handle));
  }
}

void SReadStringVector(DeserializeContext* ctx, WireBuffer* buf, Vector* out) {
  const void* data;
  size_t len;
  if (!WireReadBytes(buf, &data, &len)) {
    return;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  uint64_t count;
  if (!WireReadRawVarint(&sub, &count)) {
    return;
  }
  for (uint64_t i = 0; i < count; i++) {
    uint64_t handle;
    if (!WireReadRawVarint(&sub, &handle)) {
      return;
    }
    size_t slen;
    const char* str =
        DeserializeResolveString(ctx, (SerialHandle)handle, &slen);
    VectorAppend(out, str == NULL ? NULL : NewStringWithLength(str, slen));
  }
}
