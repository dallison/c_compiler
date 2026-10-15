#!/bin/bash
set -euo pipefail

DAVECC="$1"
X86_64="$2"
LIBC="$3"
ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/unnamed-namespace.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

# Both translation units define the same unnamed-namespace names (abseil's
# poison.cc and low_level_alloc.cc each have `(anonymous)::GetPageSize()`),
# and the same `static` class objects, constant-initialized and named by
# another static initializer.  They have internal linkage, so the link must
# keep both.
cat > "$WORK/first.cpp" <<'EOF'
struct Registry {
  constexpr Registry(int v) : value(v) {}
  virtual int Get() const { return value; }
  int value;
};
static Registry registry(3);
static const Registry* const registry_ptr = &registry;

namespace base {
namespace {
int counter = 1;
int GetPageSize() { return 4096 + counter; }
}  // namespace
int First() { return GetPageSize() + registry_ptr->Get(); }
}  // namespace base

namespace base {
int Second();
}

int main() {
  return base::First() == 4100 && base::Second() == 16391 ? 0 : 1;
}
EOF

cat > "$WORK/second.cpp" <<'EOF'
struct Registry {
  constexpr Registry(int v) : value(v) {}
  virtual int Get() const { return value; }
  int value;
};
static Registry registry(5);
static const Registry* const registry_ptr = &registry;

namespace base {
namespace {
int counter = 2;
int GetPageSize() { return 16384 + counter; }
}  // namespace
int Second() { return GetPageSize() + registry_ptr->Get(); }
}  // namespace base
EOF

"$ROOT/$DAVECC" -target x86_64 -static -std=c++17 -isystem "$ROOT/libc/include" \
  -c "$WORK/first.cpp" -o "$WORK/first.o"
"$ROOT/$DAVECC" -target x86_64 -static -std=c++17 -isystem "$ROOT/libc/include" \
  -c "$WORK/second.cpp" -o "$WORK/second.o"
"$ROOT/$DAVECC" -target x86_64 -static -Wl,-e -Wl,main \
  "$WORK/first.o" "$WORK/second.o" "$ROOT/$LIBC" -o "$WORK/test.exe"
"$ROOT/$X86_64" -i "$WORK/test.exe"
