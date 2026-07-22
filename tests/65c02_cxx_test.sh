#!/bin/bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 davecc interpreter rom source" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
ROM="$ROOT/$3"
SOURCE="$ROOT/$4"
WORK="${TEST_TMPDIR:-/tmp}/65c02-cxx"
mkdir -p "$WORK"

# The source dispatches through a secondary base and therefore requires a
# non-zero `this`-adjustor thunk. The driver must also infer static linkage for
# this static-only target.
"$DAVECC" -target 65c02 "$SOURCE" -o "$WORK/test.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/test.exe"
if grep -aq '__davecc_tls_' "$WORK/test.exe"; then
  echo "thread-local runtime linked into a threadless 65C02 program" >&2
  exit 1
fi
for symbol in AlignSize ExpandHeap InitFreeList TakeStartOfFreeBlock \
              exit_functions exit_lock; do
  if strings "$WORK/test.exe" | grep -x "$symbol" >/dev/null; then
    echo "internal runtime symbol exported: $symbol" >&2
    exit 1
  fi
done

cat >"$WORK/atoi_vector.cpp" <<'SRC'
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "need an arg\n";
    exit(1);
  }
  if (argc != 2 || atoi(argv[1]) != 5) {
    return 1;
  }
  if (!((unsigned char)'5' <= (unsigned char)'9') ||
      ((unsigned char)'9' <= (unsigned char)'5') || !(-1 <= 0) || (1 <= 0)) {
    return 2;
  }
  std::vector<std::string> values;
  for (int i = 0; i < 10; i++) {
    values.push_back("hello world " + std::to_string(i));
  }
  for (size_t i = 0; i < values.size(); i++) {
    std::cout << values[i] << std::endl;
  }
  return 0;
}
SRC

"$DAVECC" -target 65c02 "$WORK/atoi_vector.cpp" -o "$WORK/atoi_vector.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/atoi_vector.exe" 5

cat >"$WORK/map_stream.cpp" <<'SRC'
#include <iostream>
#include <map>
#include <string>

int main() {
  std::map<std::string, int> values;
  auto it = values.begin();
  std::cout << it->second << std::endl;
}
SRC

# The nested operator-> call and member load route the loaded value into a
# forward temporary. Verify that 65C02 lowering handles that destination.
"$DAVECC" -target 65c02 -S "$WORK/map_stream.cpp" -o "$WORK/map_stream.s"
if grep -q 'bad_exception' "$WORK/map_stream.s"; then
  echo "unused std::bad_exception RTTI/vtable emitted for <map>" >&2
  exit 1
fi
