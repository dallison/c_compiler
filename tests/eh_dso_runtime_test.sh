#!/usr/bin/env bash
set -euo pipefail

davecc="$1"
interpreter="$2"
libc="$3"

work="${TEST_TMPDIR:-$(mktemp -d "${TMPDIR:-/tmp}/davecc-eh-dso.XXXXXX")}"
mkdir -p "$work"

cat >"$work/thrower.cpp" <<'EOF'
struct DSOError {
  int code;
};

extern "C" void throw_from_dso() {
  throw DSOError{41};
}
EOF

cat >"$work/main.cpp" <<'EOF'
struct DSOError {
  int code;
};

extern "C" void throw_from_dso();

int main() {
  try {
    throw_from_dso();
  } catch (const DSOError& error) {
    return error.code == 41 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}
EOF

"$davecc" -target x86_64 -fexceptions -O1 -shared \
  -isystem libc/include "$work/thrower.cpp" -o "$work/libthrower.so"
"$davecc" -target x86_64 -fexceptions -O1 -Wl,-e -Wl,main \
  -rpath "$work" -isystem libc/include "$work/main.cpp" \
  "$work/libthrower.so" "$libc" -o "$work/eh_dso.exe"

if [[ "$(uname -s)" == "Darwin" ]]; then
  echo "skipping dynamic DSO execution on Darwin: fixed-address MAP_FIXED is unsupported here" >&2
  exit 0
fi

(
  cd "$work"
  LD_LIBRARY_PATH="$work:${LD_LIBRARY_PATH:-}" "$interpreter" -i eh_dso.exe
)
