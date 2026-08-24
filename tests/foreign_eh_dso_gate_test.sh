#!/bin/bash
set -euo pipefail

DAVECC="$1"
INTERPRETER="$2"
LIBC="$3"

WORK="${TEST_TMPDIR:-$(mktemp -d "${TMPDIR:-/tmp}/foreign-eh-dso.XXXXXX")}"
mkdir -p "$WORK"

CLANG_CXX="${CLANG_CXX:-clang++}"
if ! command -v "$CLANG_CXX" >/dev/null 2>&1; then
  echo "missing host clang++ for foreign EH DSO gate" >&2
  exit 1
fi

cat >"$WORK/foreign_throw.cpp" <<'EOF'
extern "C" void foreign_throw_int() {
  throw 42;
}
EOF

cat >"$WORK/davecc_catch.cpp" <<'EOF'
extern "C" void foreign_throw_int();

int main() {
  try {
    foreign_throw_int();
  } catch (int value) {
    return value == 42 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}
EOF

cat >"$WORK/davecc_throw.cpp" <<'EOF'
extern "C" void davecc_throw_int() {
  throw 41;
}
EOF

cat >"$WORK/foreign_catch.cpp" <<'EOF'
extern "C" void davecc_throw_int();

int main() {
  try {
    davecc_throw_int();
  } catch (int value) {
    return value == 41 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}
EOF

"$CLANG_CXX" --target=x86_64-unknown-linux-gnu -std=c++20 -O1 \
  -fexceptions -fPIC -c "$WORK/foreign_throw.cpp" -o "$WORK/foreign_throw.o"
"$DAVECC" -target x86_64 -shared -fexceptions \
  "$WORK/foreign_throw.o" -o "$WORK/libforeign_throw.so"
"$DAVECC" -target x86_64 -std=c++20 -O1 -fexceptions \
  -isystem libc/include -Wl,-e -Wl,main -rpath "$WORK" \
  "$WORK/davecc_catch.cpp" "$WORK/libforeign_throw.so" "$LIBC" \
  -o "$WORK/davecc_catches_foreign.exe"

"$DAVECC" -target x86_64 -std=c++20 -O1 -fexceptions -shared \
  -isystem libc/include "$WORK/davecc_throw.cpp" \
  -o "$WORK/libdavecc_throw.so"
"$CLANG_CXX" --target=x86_64-unknown-linux-gnu -std=c++20 -O1 \
  -fexceptions -fno-pic -fno-omit-frame-pointer -mcmodel=large \
  -c "$WORK/foreign_catch.cpp" -o "$WORK/foreign_catch.o"
"$DAVECC" -target x86_64 -std=c++20 -O1 -fexceptions \
  -isystem libc/include -Wl,-e -Wl,main -rpath "$WORK" \
  "$WORK/foreign_catch.o" "$WORK/libdavecc_throw.so" "$LIBC" \
  -o "$WORK/foreign_catches_davecc.exe"

if [[ "$(uname -s)" == "Darwin" ]]; then
  echo "skipping dynamic foreign EH execution on Darwin: fixed-address MAP_FIXED is unsupported here" >&2
  exit 0
fi

(
  cd "$WORK"
  LD_LIBRARY_PATH="$WORK:${LD_LIBRARY_PATH:-}" \
    "$INTERPRETER" -i davecc_catches_foreign.exe
  LD_LIBRARY_PATH="$WORK:${LD_LIBRARY_PATH:-}" \
    "$INTERPRETER" -i foreign_catches_davecc.exe
)
