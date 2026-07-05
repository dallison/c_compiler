//
//  wireformat.h
//  c_compiler
//
//  Protobuf wire-format primitives, ported to C from the phaser project's
//  runtime/wireformat.h (which is C++).  This provides varint / zigzag / tag /
//  fixed / length-delimited encoding and decoding on top of a growable byte
//  buffer.  No .proto files or protobuf compiler are involved; callers assign
//  their own field numbers and drive (de)serialization by hand.
//
//  A WireBuffer is either:
//    * owned  (writer): allocates and grows its own memory as data is written.
//    * reader:          wraps caller-provided memory and only reads from it.
//
//  Errors are sticky: once any operation fails the `error` flag is set and all
//  subsequent operations become no-ops returning false.  Callers may check the
//  flag once at the end of a batch rather than after every call.  (C has no
//  absl::Status, so this replaces phaser's StatusOr-based error handling.)
//

#ifndef wireformat_h
#define wireformat_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  kWireVarint = 0,
  kWireFixed64 = 1,
  kWireLengthDelimited = 2,
  kWireStartGroup = 3,  // Groups are unsupported; present for completeness.
  kWireEndGroup = 4,
  kWireFixed32 = 5,
} WireType;

#define WIRE_FIELD_SHIFT 3
#define WIRE_TYPE_MASK 0x7

typedef struct {
  char* start;   // Beginning of the buffer.
  char* addr;    // Current read/write cursor.
  char* end;     // One past the last valid byte.
  size_t size;   // Total capacity of `start`.
  bool owned;    // True if this buffer owns (and may grow/free) `start`.
  bool error;    // Sticky error flag.
} WireBuffer;

// Initialize a growable writer buffer.  `initial_size` is clamped to a
// reasonable minimum.
void WireBufferInitOwned(WireBuffer* buf, size_t initial_size);

// Initialize a reader over caller-owned memory.  The memory must outlive the
// buffer; it is never freed by WireBufferDestruct.
void WireBufferInitReader(WireBuffer* buf, const void* data, size_t size);

// Releases owned memory (no-op for readers).
void WireBufferDestruct(WireBuffer* buf);

// Number of bytes written (writer) or consumed (reader): addr - start.
size_t WireBufferSize(const WireBuffer* buf);

// Pointer to the start of the buffer's data.
const void* WireBufferData(const WireBuffer* buf);

// True when the read cursor has reached the end of a reader buffer.
bool WireBufferEof(const WireBuffer* buf);

// True when the sticky error flag is set.
bool WireBufferHasError(const WireBuffer* buf);

// Rewind the cursor to the start (keeps contents/capacity).  Useful to read a
// writer buffer back, or to re-read a reader.
void WireBufferRewind(WireBuffer* buf);

// ZigZag maps a signed integer to an unsigned one where small magnitudes map to
// small values (protobuf sint64 encoding).  ZagZig is the inverse.
uint64_t WireZigZag64(int64_t value);
int64_t WireZagZig64(uint64_t value);

// Number of bytes a value would occupy as a varint.
size_t WireVarintSize(uint64_t value);
// Number of bytes the tag for (field, wire_type) would occupy.
size_t WireTagSize(int field_number, WireType wire_type);

//
// Writing.  All writers return false (and set the error flag) on failure.
//
bool WireWriteTag(WireBuffer* buf, int field_number, WireType wire_type);
bool WireWriteRawVarint(WireBuffer* buf, uint64_t value);
bool WireWriteRaw(WireBuffer* buf, const void* data, size_t length);

// Tagged scalar writers.
bool WireWriteVarint(WireBuffer* buf, int field_number, uint64_t value);
bool WireWriteSignedVarint(WireBuffer* buf, int field_number, int64_t value);
bool WireWriteBool(WireBuffer* buf, int field_number, bool value);
bool WireWriteInt32(WireBuffer* buf, int field_number, int32_t value);
bool WireWriteUint32(WireBuffer* buf, int field_number, uint32_t value);
bool WireWriteInt64(WireBuffer* buf, int field_number, int64_t value);
bool WireWriteUint64(WireBuffer* buf, int field_number, uint64_t value);
bool WireWriteFixed32(WireBuffer* buf, int field_number, uint32_t value);
bool WireWriteFixed64(WireBuffer* buf, int field_number, uint64_t value);
bool WireWriteDouble(WireBuffer* buf, int field_number, double value);
bool WireWriteFloat(WireBuffer* buf, int field_number, float value);

// Tagged length-delimited writers.
bool WireWriteBytes(WireBuffer* buf, int field_number, const void* data,
                    size_t length);
bool WireWriteString(WireBuffer* buf, int field_number, const char* str,
                     size_t length);

//
// Reading.  All readers return false (and set the error flag) on failure.
//

// Reads a tag, returning the field number and wire type.
bool WireReadTag(WireBuffer* buf, int* field_number, WireType* wire_type);

bool WireReadRawVarint(WireBuffer* buf, uint64_t* out);
bool WireReadRaw(WireBuffer* buf, void* dest, size_t length);

bool WireReadVarint(WireBuffer* buf, uint64_t* out);
bool WireReadSignedVarint(WireBuffer* buf, int64_t* out);
bool WireReadBool(WireBuffer* buf, bool* out);
bool WireReadInt32(WireBuffer* buf, int32_t* out);
bool WireReadUint32(WireBuffer* buf, uint32_t* out);
bool WireReadInt64(WireBuffer* buf, int64_t* out);
bool WireReadUint64(WireBuffer* buf, uint64_t* out);
bool WireReadFixed32(WireBuffer* buf, uint32_t* out);
bool WireReadFixed64(WireBuffer* buf, uint64_t* out);
bool WireReadDouble(WireBuffer* buf, double* out);
bool WireReadFloat(WireBuffer* buf, float* out);

// Reads a length-delimited field.  On success, *data points into the buffer's
// own memory (no copy) and *length is the payload length.  The pointer is only
// valid for the lifetime of the buffer's backing store.
bool WireReadBytes(WireBuffer* buf, const void** data, size_t* length);

// Skips the payload of a field whose tag (wire_type) has already been read.
// Used to ignore unknown field numbers for forward/backward compatibility.
bool WireSkip(WireBuffer* buf, WireType wire_type);

#endif /* wireformat_h */
