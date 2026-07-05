//
//  wireformat_test.c
//  c_compiler
//
//  Unit tests for the C protobuf wire format (c_compiler/serialize/wireformat).
//  Mirrors the phaser project's wireformat_test.cc, adapted to plain C.  Returns
//  0 on success; a non-zero code identifies the first failing check.
//

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "wireformat.h"

static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      g_failures++;                                                       \
    }                                                                     \
  } while (0)

static void TestSizes(void) {
  CHECK(WireVarintSize(1) == 1);
  CHECK(WireVarintSize(0x80) == 2);
  CHECK(WireVarintSize(0x8000) == 3);
  CHECK(WireVarintSize(UINT64_MAX) == 10);

  CHECK(WireTagSize(1, kWireVarint) == 1);
  CHECK(WireTagSize(0xf, kWireVarint) == 1);
  CHECK(WireTagSize(0x10, kWireVarint) == 2);
}

static void TestZigZagKnownValues(void) {
  CHECK(WireZigZag64(0) == 0u);
  CHECK(WireZigZag64(-1) == 1u);
  CHECK(WireZigZag64(1) == 2u);
  CHECK(WireZigZag64(-2) == 3u);
  CHECK(WireZigZag64(2) == 4u);
  CHECK(WireZigZag64(INT64_MAX) == 0xFFFFFFFFFFFFFFFEull);
  CHECK(WireZigZag64(INT64_MIN) == 0xFFFFFFFFFFFFFFFFull);
  CHECK(WireZigZag64((int64_t)1 << 31) == ((uint64_t)1 << 32));
}

static void TestZigZagRoundTrip(void) {
  int64_t values[] = {INT64_MIN,
                      INT64_MIN + 1,
                      (int64_t)INT32_MIN - 1,
                      INT32_MIN,
                      -((int64_t)1 << 40),
                      -123456,
                      -2,
                      -1,
                      0,
                      1,
                      2,
                      123456,
                      (int64_t)1 << 31,
                      (int64_t)1 << 40,
                      (int64_t)INT32_MAX + 1,
                      INT64_MAX - 1,
                      INT64_MAX};
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
    CHECK(WireZagZig64(WireZigZag64(values[i])) == values[i]);
  }
}

static void TestSignedVarintRoundTrip(void) {
  int64_t values[] = {INT64_MIN, (int64_t)INT32_MIN - 1, -1, 0,
                      1,         (int64_t)1 << 33,        (int64_t)INT32_MAX + 1,
                      INT64_MAX};
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
    WireBuffer out;
    WireBufferInitOwned(&out, 16);
    CHECK(WireWriteSignedVarint(&out, 1, values[i]));

    WireBuffer in;
    WireBufferInitReader(&in, WireBufferData(&out), WireBufferSize(&out));
    int field;
    WireType wt;
    CHECK(WireReadTag(&in, &field, &wt));
    CHECK(field == 1 && wt == kWireVarint);
    int64_t decoded = 0;
    CHECK(WireReadSignedVarint(&in, &decoded));
    CHECK(decoded == values[i]);
    CHECK(!WireBufferHasError(&in));

    WireBufferDestruct(&out);
    WireBufferDestruct(&in);
  }
}

static void TestScalarRoundTrip(void) {
  WireBuffer out;
  WireBufferInitOwned(&out, 16);
  CHECK(WireWriteVarint(&out, 1, 300));
  CHECK(WireWriteBool(&out, 2, true));
  CHECK(WireWriteInt32(&out, 3, -7));
  CHECK(WireWriteUint64(&out, 4, 0xdeadbeefcafebabeull));
  CHECK(WireWriteFixed32(&out, 5, 0x12345678u));
  CHECK(WireWriteFixed64(&out, 6, 0x1122334455667788ull));
  CHECK(WireWriteDouble(&out, 7, 3.5));
  CHECK(WireWriteFloat(&out, 8, 2.25f));
  CHECK(WireWriteString(&out, 9, "hello", 5));
  CHECK(!WireBufferHasError(&out));

  WireBuffer in;
  WireBufferInitReader(&in, WireBufferData(&out), WireBufferSize(&out));

  int field;
  WireType wt;
  uint64_t u64;
  bool b;
  int32_t i32;
  uint32_t u32;
  uint64_t f64;
  double d;
  float f;
  const void* bytes;
  size_t len;

  CHECK(WireReadTag(&in, &field, &wt) && field == 1 && wt == kWireVarint);
  CHECK(WireReadVarint(&in, &u64) && u64 == 300);
  CHECK(WireReadTag(&in, &field, &wt) && field == 2);
  CHECK(WireReadBool(&in, &b) && b == true);
  CHECK(WireReadTag(&in, &field, &wt) && field == 3);
  CHECK(WireReadInt32(&in, &i32) && i32 == -7);
  CHECK(WireReadTag(&in, &field, &wt) && field == 4);
  CHECK(WireReadUint64(&in, &u64) && u64 == 0xdeadbeefcafebabeull);
  CHECK(WireReadTag(&in, &field, &wt) && field == 5 && wt == kWireFixed32);
  CHECK(WireReadFixed32(&in, &u32) && u32 == 0x12345678u);
  CHECK(WireReadTag(&in, &field, &wt) && field == 6 && wt == kWireFixed64);
  CHECK(WireReadFixed64(&in, &f64) && f64 == 0x1122334455667788ull);
  CHECK(WireReadTag(&in, &field, &wt) && field == 7);
  CHECK(WireReadDouble(&in, &d) && d == 3.5);
  CHECK(WireReadTag(&in, &field, &wt) && field == 8);
  CHECK(WireReadFloat(&in, &f) && f == 2.25f);
  CHECK(WireReadTag(&in, &field, &wt) && field == 9 && wt == kWireLengthDelimited);
  CHECK(WireReadBytes(&in, &bytes, &len) && len == 5 &&
        memcmp(bytes, "hello", 5) == 0);
  CHECK(WireBufferEof(&in));
  CHECK(!WireBufferHasError(&in));

  WireBufferDestruct(&out);
  WireBufferDestruct(&in);
}

static void TestSkipUnknownFields(void) {
  // Writer emits fields 1, 2, 3; reader only understands field 2 and skips the
  // rest, exercising WireSkip for every wire type.
  WireBuffer out;
  WireBufferInitOwned(&out, 16);
  CHECK(WireWriteDouble(&out, 1, 9.0));       // fixed64
  CHECK(WireWriteVarint(&out, 2, 42));        // varint (the one we read)
  CHECK(WireWriteString(&out, 3, "skip", 4)); // length-delimited
  CHECK(WireWriteFixed32(&out, 4, 5u));       // fixed32

  WireBuffer in;
  WireBufferInitReader(&in, WireBufferData(&out), WireBufferSize(&out));
  int found = -1;
  while (!WireBufferEof(&in) && !WireBufferHasError(&in)) {
    int field;
    WireType wt;
    if (!WireReadTag(&in, &field, &wt)) {
      break;
    }
    if (field == 2) {
      uint64_t v;
      CHECK(WireReadVarint(&in, &v));
      found = (int)v;
    } else {
      CHECK(WireSkip(&in, wt));
    }
  }
  CHECK(found == 42);
  CHECK(!WireBufferHasError(&in));

  WireBufferDestruct(&out);
  WireBufferDestruct(&in);
}

static void TestGrowth(void) {
  // Force many reallocations starting from the minimum size.
  WireBuffer out;
  WireBufferInitOwned(&out, 16);
  for (int i = 0; i < 1000; i++) {
    CHECK(WireWriteVarint(&out, 1, (uint64_t)i));
  }
  CHECK(!WireBufferHasError(&out));

  WireBuffer in;
  WireBufferInitReader(&in, WireBufferData(&out), WireBufferSize(&out));
  for (int i = 0; i < 1000; i++) {
    int field;
    WireType wt;
    uint64_t v;
    CHECK(WireReadTag(&in, &field, &wt) && field == 1);
    CHECK(WireReadVarint(&in, &v) && v == (uint64_t)i);
  }
  CHECK(WireBufferEof(&in));

  WireBufferDestruct(&out);
  WireBufferDestruct(&in);
}

static void TestTruncatedInputErrors(void) {
  // A varint with the continuation bit set but no following byte must error.
  char bad[] = {(char)0x80};
  WireBuffer in;
  WireBufferInitReader(&in, bad, sizeof(bad));
  uint64_t v;
  CHECK(!WireReadRawVarint(&in, &v));
  CHECK(WireBufferHasError(&in));
}

int main(void) {
  TestSizes();
  TestZigZagKnownValues();
  TestZigZagRoundTrip();
  TestSignedVarintRoundTrip();
  TestScalarRoundTrip();
  TestSkipUnknownFields();
  TestGrowth();
  TestTruncatedInputErrors();

  if (g_failures != 0) {
    fprintf(stderr, "%d wire-format check(s) failed\n", g_failures);
    return 1;
  }
  printf("wireformat_test: all checks passed\n");
  return 0;
}
