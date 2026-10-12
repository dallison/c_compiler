#!/bin/bash
set -euo pipefail

DAVECC="$1"
X86_64="$2"
LIBC="$3"
ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/explicit-specialization.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

cat > "$WORK/caller.cpp" <<'EOF'
struct Value {
  template <class T>
  int get(const T&) {
    return 1;
  }
};

template <>
int Value::get<int>(const int&);

int main() {
  Value value;
  int argument = 0;
  return value.get(argument) == 42 ? 0 : 1;
}
EOF

cat > "$WORK/definition.cpp" <<'EOF'
struct Value {
  template <class T>
  int get(const T&) {
    return 1;
  }
};

template <>
int Value::get<int>(const int&) {
  return 42;
}
EOF

"$ROOT/$DAVECC" -target x86_64 -static -std=c++17 -isystem "$ROOT/libc/include" \
  -c "$WORK/caller.cpp" -o "$WORK/caller.o"
"$ROOT/$DAVECC" -target x86_64 -static -std=c++17 -isystem "$ROOT/libc/include" \
  -c "$WORK/definition.cpp" -o "$WORK/definition.o"
"$ROOT/$DAVECC" -target x86_64 -static -Wl,-e -Wl,main \
  "$WORK/caller.o" "$WORK/definition.o" "$ROOT/$LIBC" -o "$WORK/test.exe"
"$ROOT/$X86_64" -i "$WORK/test.exe"
