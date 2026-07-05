//
//  wireformat.c
//  c_compiler
//
//  C implementation of the protobuf wire format.  See wireformat.h.
//

#include "wireformat.h"

#include <stdlib.h>
#include <string.h>

#define WIRE_MIN_SIZE 16

void WireBufferInitOwned(WireBuffer* buf, size_t initial_size) {
  if (initial_size < WIRE_MIN_SIZE) {
    initial_size = WIRE_MIN_SIZE;
  }
  buf->start = (char*)malloc(initial_size);
  if (buf->start == NULL) {
    // Match phaser: allocation failure is fatal for a growable buffer.
    abort();
  }
  // Zero so unwritten regions never read as uninitialized memory.
  memset(buf->start, 0, initial_size);
  buf->addr = buf->start;
  buf->end = buf->start + initial_size;
  buf->size = initial_size;
  buf->owned = true;
  buf->error = false;
}

void WireBufferInitReader(WireBuffer* buf, const void* data, size_t size) {
  buf->start = (char*)data;  // Not modified; readers never write.
  buf->addr = buf->start;
  buf->end = buf->start + size;
  buf->size = size;
  buf->owned = false;
  buf->error = false;
}

void WireBufferDestruct(WireBuffer* buf) {
  if (buf->owned) {
    free(buf->start);
  }
  buf->start = NULL;
  buf->addr = NULL;
  buf->end = NULL;
  buf->size = 0;
}

size_t WireBufferSize(const WireBuffer* buf) {
  return (size_t)(buf->addr - buf->start);
}

const void* WireBufferData(const WireBuffer* buf) { return buf->start; }

bool WireBufferEof(const WireBuffer* buf) { return buf->addr >= buf->end; }

bool WireBufferHasError(const WireBuffer* buf) { return buf->error; }

void WireBufferRewind(WireBuffer* buf) {
  // For a writer, the valid data ends at addr; expose it for reading by moving
  // end to the current write position first.
  if (buf->owned) {
    buf->end = buf->addr;
  }
  buf->addr = buf->start;
}

uint64_t WireZigZag64(int64_t value) {
  uint64_t u = (uint64_t)value;
  return (u << 1) ^ (uint64_t)(-(int64_t)(u >> 63));
}

int64_t WireZagZig64(uint64_t value) {
  return (int64_t)((value >> 1) ^ (uint64_t)(-(int64_t)(value & 1)));
}

size_t WireVarintSize(uint64_t value) {
  size_t size = 1;
  while (value >= 0x80) {
    value >>= 7;
    size++;
  }
  return size;
}

static uint64_t WireMakeTag(int field_number, WireType wire_type) {
  return ((uint64_t)field_number << WIRE_FIELD_SHIFT) | (uint64_t)wire_type;
}

size_t WireTagSize(int field_number, WireType wire_type) {
  return WireVarintSize(WireMakeTag(field_number, wire_type));
}

// Ensures the writer has room for `n` more bytes, growing owned buffers.
// Returns false (and sets error) on a reader or on allocation shrink issues.
static bool WireHasSpaceFor(WireBuffer* buf, size_t n) {
  if (buf->error) {
    return false;
  }
  char* next = buf->addr + n;
  if (next <= buf->end) {
    return true;
  }
  if (!buf->owned) {
    buf->error = true;
    return false;
  }
  size_t new_size = buf->size == 0 ? WIRE_MIN_SIZE : buf->size;
  size_t needed = (size_t)(next - buf->start);
  while (new_size < needed) {
    new_size *= 2;
  }
  char* new_start = (char*)realloc(buf->start, new_size);
  if (new_start == NULL) {
    abort();
  }
  memset(new_start + buf->size, 0, new_size - buf->size);
  size_t offset = (size_t)(buf->addr - buf->start);
  buf->start = new_start;
  buf->addr = new_start + offset;
  buf->end = new_start + new_size;
  buf->size = new_size;
  return true;
}

// Ensures the reader has `n` bytes remaining.
static bool WireCheck(WireBuffer* buf, size_t n) {
  if (buf->error) {
    return false;
  }
  if (buf->addr + n <= buf->end) {
    return true;
  }
  buf->error = true;
  return false;
}

bool WireWriteRawVarint(WireBuffer* buf, uint64_t value) {
  if (!WireHasSpaceFor(buf, WireVarintSize(value))) {
    return false;
  }
  for (;;) {
    if ((value & ~(uint64_t)0x7f) == 0) {
      *buf->addr++ = (char)value;
      break;
    }
    *buf->addr++ = (char)((value & 0x7f) | 0x80);
    value >>= 7;
  }
  return true;
}

bool WireWriteRaw(WireBuffer* buf, const void* data, size_t length) {
  if (!WireHasSpaceFor(buf, length)) {
    return false;
  }
  memcpy(buf->addr, data, length);
  buf->addr += length;
  return true;
}

bool WireWriteTag(WireBuffer* buf, int field_number, WireType wire_type) {
  return WireWriteRawVarint(buf, WireMakeTag(field_number, wire_type));
}

bool WireWriteVarint(WireBuffer* buf, int field_number, uint64_t value) {
  if (!WireWriteTag(buf, field_number, kWireVarint)) {
    return false;
  }
  return WireWriteRawVarint(buf, value);
}

bool WireWriteSignedVarint(WireBuffer* buf, int field_number, int64_t value) {
  if (!WireWriteTag(buf, field_number, kWireVarint)) {
    return false;
  }
  return WireWriteRawVarint(buf, WireZigZag64(value));
}

bool WireWriteBool(WireBuffer* buf, int field_number, bool value) {
  return WireWriteVarint(buf, field_number, value ? 1 : 0);
}

bool WireWriteInt32(WireBuffer* buf, int field_number, int32_t value) {
  // Protobuf int32 sign-extends negatives to 64 bits on the wire.
  return WireWriteVarint(buf, field_number, (uint64_t)(int64_t)value);
}

bool WireWriteUint32(WireBuffer* buf, int field_number, uint32_t value) {
  return WireWriteVarint(buf, field_number, (uint64_t)value);
}

bool WireWriteInt64(WireBuffer* buf, int field_number, int64_t value) {
  return WireWriteVarint(buf, field_number, (uint64_t)value);
}

bool WireWriteUint64(WireBuffer* buf, int field_number, uint64_t value) {
  return WireWriteVarint(buf, field_number, value);
}

bool WireWriteFixed32(WireBuffer* buf, int field_number, uint32_t value) {
  if (!WireWriteTag(buf, field_number, kWireFixed32)) {
    return false;
  }
  return WireWriteRaw(buf, &value, sizeof(value));
}

bool WireWriteFixed64(WireBuffer* buf, int field_number, uint64_t value) {
  if (!WireWriteTag(buf, field_number, kWireFixed64)) {
    return false;
  }
  return WireWriteRaw(buf, &value, sizeof(value));
}

bool WireWriteDouble(WireBuffer* buf, int field_number, double value) {
  uint64_t bits;
  memcpy(&bits, &value, sizeof(bits));
  return WireWriteFixed64(buf, field_number, bits);
}

bool WireWriteFloat(WireBuffer* buf, int field_number, float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  return WireWriteFixed32(buf, field_number, bits);
}

bool WireWriteBytes(WireBuffer* buf, int field_number, const void* data,
                    size_t length) {
  if (!WireWriteTag(buf, field_number, kWireLengthDelimited)) {
    return false;
  }
  if (!WireWriteRawVarint(buf, (uint64_t)length)) {
    return false;
  }
  return WireWriteRaw(buf, data, length);
}

bool WireWriteString(WireBuffer* buf, int field_number, const char* str,
                     size_t length) {
  return WireWriteBytes(buf, field_number, str, length);
}

bool WireReadRawVarint(WireBuffer* buf, uint64_t* out) {
  uint64_t value = 0;
  for (int shift = 0; shift < 64; shift += 7) {
    if (!WireCheck(buf, 1)) {
      return false;
    }
    uint64_t byte = (uint8_t)*buf->addr++;
    value |= (byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      *out = value;
      return true;
    }
  }
  buf->error = true;  // Varint too long.
  return false;
}

bool WireReadRaw(WireBuffer* buf, void* dest, size_t length) {
  if (!WireCheck(buf, length)) {
    return false;
  }
  memcpy(dest, buf->addr, length);
  buf->addr += length;
  return true;
}

bool WireReadTag(WireBuffer* buf, int* field_number, WireType* wire_type) {
  uint64_t tag;
  if (!WireReadRawVarint(buf, &tag)) {
    return false;
  }
  *wire_type = (WireType)(tag & WIRE_TYPE_MASK);
  *field_number = (int)(tag >> WIRE_FIELD_SHIFT);
  return true;
}

bool WireReadVarint(WireBuffer* buf, uint64_t* out) {
  return WireReadRawVarint(buf, out);
}

bool WireReadSignedVarint(WireBuffer* buf, int64_t* out) {
  uint64_t value;
  if (!WireReadRawVarint(buf, &value)) {
    return false;
  }
  *out = WireZagZig64(value);
  return true;
}

bool WireReadBool(WireBuffer* buf, bool* out) {
  uint64_t value;
  if (!WireReadRawVarint(buf, &value)) {
    return false;
  }
  *out = value != 0;
  return true;
}

bool WireReadInt32(WireBuffer* buf, int32_t* out) {
  uint64_t value;
  if (!WireReadRawVarint(buf, &value)) {
    return false;
  }
  *out = (int32_t)value;
  return true;
}

bool WireReadUint32(WireBuffer* buf, uint32_t* out) {
  uint64_t value;
  if (!WireReadRawVarint(buf, &value)) {
    return false;
  }
  *out = (uint32_t)value;
  return true;
}

bool WireReadInt64(WireBuffer* buf, int64_t* out) {
  uint64_t value;
  if (!WireReadRawVarint(buf, &value)) {
    return false;
  }
  *out = (int64_t)value;
  return true;
}

bool WireReadUint64(WireBuffer* buf, uint64_t* out) {
  return WireReadRawVarint(buf, out);
}

bool WireReadFixed32(WireBuffer* buf, uint32_t* out) {
  return WireReadRaw(buf, out, sizeof(*out));
}

bool WireReadFixed64(WireBuffer* buf, uint64_t* out) {
  return WireReadRaw(buf, out, sizeof(*out));
}

bool WireReadDouble(WireBuffer* buf, double* out) {
  uint64_t bits;
  if (!WireReadFixed64(buf, &bits)) {
    return false;
  }
  memcpy(out, &bits, sizeof(*out));
  return true;
}

bool WireReadFloat(WireBuffer* buf, float* out) {
  uint32_t bits;
  if (!WireReadFixed32(buf, &bits)) {
    return false;
  }
  memcpy(out, &bits, sizeof(*out));
  return true;
}

bool WireReadBytes(WireBuffer* buf, const void** data, size_t* length) {
  uint64_t len;
  if (!WireReadRawVarint(buf, &len)) {
    return false;
  }
  if (!WireCheck(buf, (size_t)len)) {
    return false;
  }
  *data = buf->addr;
  *length = (size_t)len;
  buf->addr += len;
  return true;
}

bool WireSkip(WireBuffer* buf, WireType wire_type) {
  switch (wire_type) {
    case kWireVarint: {
      uint64_t value;
      return WireReadRawVarint(buf, &value);
    }
    case kWireFixed64:
      if (!WireCheck(buf, 8)) {
        return false;
      }
      buf->addr += 8;
      return true;
    case kWireLengthDelimited: {
      uint64_t len;
      if (!WireReadRawVarint(buf, &len)) {
        return false;
      }
      if (!WireCheck(buf, (size_t)len)) {
        return false;
      }
      buf->addr += len;
      return true;
    }
    case kWireFixed32:
      if (!WireCheck(buf, 4)) {
        return false;
      }
      buf->addr += 4;
      return true;
    case kWireStartGroup:
    case kWireEndGroup:
    default:
      buf->error = true;
      return false;
  }
}
