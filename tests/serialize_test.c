//
//  serialize_test.c
//  c_compiler
//
//  Exercises the graph-aware serialization framework (serialize.{c,h}) with a
//  small dummy object kind: string interning/de-duplication, integer-handle
//  interning, cyclic references, pool emission, and the two-pass
//  allocate-then-resolve read path.  Returns 0 on success.
//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "serialize.h"
#include "wireformat.h"

static int g_failures = 0;

#define CHECK(cond)                                                   \
  do {                                                                \
    if (!(cond)) {                                                    \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      g_failures++;                                                   \
    }                                                                 \
  } while (0)

// A tiny graph node used as a stand-in for a real AST/type object.  We reuse
// kSerialKindType as the pool for these test nodes.
typedef struct TestNode {
  int value;
  struct TestNode* next;  // May form a cycle.
  const char* name;
} TestNode;

// Field numbers.
enum {
  kTestNodeValue = 1,
  kTestNodeNext = 2,
  kTestNodeName = 3,
};

static bool TestNodeWrite(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  TestNode* node = (TestNode*)obj;
  WireWriteInt32(buf, kTestNodeValue, node->value);
  SerialHandle next = SerializeIntern(ctx, kSerialKindType, node->next);
  WireWriteVarint(buf, kTestNodeNext, next);
  SerialHandle name = node->name == NULL
                          ? kSerialNullHandle
                          : SerializeInternStringN(ctx, node->name,
                                                   strlen(node->name));
  WireWriteVarint(buf, kTestNodeName, name);
  return !WireBufferHasError(buf);
}

static void* TestNodeAlloc(DeserializeContext* ctx, const void* blob,
                           size_t len) {
  (void)ctx;
  (void)blob;
  (void)len;
  return calloc(1, sizeof(TestNode));
}

// Read pass stores handles temporarily so pointers can be resolved after all
// objects are allocated.  Because DeserializeResolve works during pass 2 (all
// objects already allocated), we can resolve inline here.
static bool TestNodeRead(DeserializeContext* ctx, WireBuffer* buf, void* obj) {
  TestNode* node = (TestNode*)obj;
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    switch (field) {
      case kTestNodeValue:
        WireReadInt32(buf, &node->value);
        break;
      case kTestNodeNext: {
        uint64_t h;
        WireReadVarint(buf, &h);
        node->next =
            (TestNode*)DeserializeResolve(ctx, kSerialKindType, (SerialHandle)h);
        break;
      }
      case kTestNodeName: {
        uint64_t h;
        WireReadVarint(buf, &h);
        node->name = DeserializeResolveString(ctx, (SerialHandle)h, NULL);
        break;
      }
      default:
        WireSkip(buf, wt);
        break;
    }
  }
  return !WireBufferHasError(buf);
}

static void TestStringDedup(void) {
  SerializeContext ctx;
  SerializeContextInit(&ctx);
  SerialHandle a = SerializeInternStringN(&ctx, "hello", 5);
  SerialHandle b = SerializeInternStringN(&ctx, "world", 5);
  SerialHandle c = SerializeInternStringN(&ctx, "hello", 5);
  CHECK(a == 1);
  CHECK(b == 2);
  CHECK(c == a);  // De-duplicated.
  CHECK(SerializeInternStringN(&ctx, NULL, 0) == kSerialNullHandle);
  CHECK(ctx.string_pool.length == 2);
  SerializeContextDestruct(&ctx);
}

static void TestGraphRoundTrip(void) {
  SerialKindVtable vt = {TestNodeWrite, TestNodeAlloc, TestNodeRead, "TestNode"};
  SerializeRegisterKind(kSerialKindType, &vt);

  // Build a cycle: a -> b -> a, with a shared name string.
  TestNode a = {10, NULL, "shared"};
  TestNode b = {20, NULL, "shared"};
  a.next = &b;
  b.next = &a;

  SerializeContext sctx;
  SerializeContextInit(&sctx);
  SerialHandle root = SerializeIntern(&sctx, kSerialKindType, &a);
  CHECK(root == 1);
  CHECK(SerializeContextDrain(&sctx));
  CHECK(sctx.objects[kSerialKindType].length == 2);   // a and b.
  CHECK(sctx.string_pool.length == 1);                // "shared" de-duplicated.

  WireBuffer out;
  WireBufferInitOwned(&out, 64);
  CHECK(SerializeWriteStringPool(&sctx, &out));
  CHECK(SerializeWritePool(&sctx, kSerialKindType, &out));
  CHECK(!WireBufferHasError(&out));

  // Read it back.
  DeserializeContext dctx;
  DeserializeContextInit(&dctx);
  WireBuffer in;
  WireBufferInitReader(&in, WireBufferData(&out), WireBufferSize(&out));
  CHECK(SerializeReadStringPool(&dctx, &in));
  CHECK(SerializeReadPool(&dctx, kSerialKindType, &in));
  CHECK(DeserializeContextResolve(&dctx));
  CHECK(!dctx.error);

  TestNode* ra = (TestNode*)DeserializeResolve(&dctx, kSerialKindType, 1);
  TestNode* rb = (TestNode*)DeserializeResolve(&dctx, kSerialKindType, 2);
  CHECK(ra != NULL && rb != NULL);
  CHECK(ra->value == 10 && rb->value == 20);
  CHECK(ra->next == rb);   // Cycle preserved.
  CHECK(rb->next == ra);
  CHECK(ra->name != NULL && strcmp(ra->name, "shared") == 0);
  CHECK(ra->name == rb->name);  // Shared string.

  free(ra);
  free(rb);
  WireBufferDestruct(&out);
  WireBufferDestruct(&in);
  SerializeContextDestruct(&sctx);
  DeserializeContextDestruct(&dctx);
}

int main(void) {
  TestStringDedup();
  TestGraphRoundTrip();
  if (g_failures != 0) {
    fprintf(stderr, "%d serialize check(s) failed\n", g_failures);
    return 1;
  }
  printf("serialize_test: all checks passed\n");
  return 0;
}
