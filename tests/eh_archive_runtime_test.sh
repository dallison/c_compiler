#!/usr/bin/env bash
set -euo pipefail

davecc="$1"
archivist="$2"
interpreter="$3"
libc="$4"
target="$5"

work="${TEST_TMPDIR:-$(mktemp -d "${TMPDIR:-/tmp}/davecc-eh-archive.XXXXXX")}"
mkdir -p "$work"

cat >"$work/thrower.cpp" <<'EOF'
struct ArchiveError {
  int code;
};

extern "C" void throw_from_archive() {
  throw ArchiveError{37};
}
EOF

cat >"$work/main.cpp" <<'EOF'
struct ArchiveError {
  int code;
};

extern "C" void throw_from_archive();

int main() {
  try {
    throw_from_archive();
  } catch (const ArchiveError& error) {
    return error.code == 37 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}
EOF

"$davecc" -target "$target" -fexceptions -O1 -c "$work/thrower.cpp" \
  -isystem libc/include -o "$work/thrower.o"
"$archivist" r "$work/libthrower.a" "$work/thrower.o"
"$davecc" -target "$target" -fexceptions -O1 -static -isystem libc/include \
  -Wl,-e -Wl,main "$work/main.cpp" "$work/libthrower.a" "$libc" \
  -o "$work/eh_archive.exe"
"$interpreter" "$work/eh_archive.exe"
