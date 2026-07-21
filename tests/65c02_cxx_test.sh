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
