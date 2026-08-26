#!/bin/bash
# GCC and Clang accept the value of -I, -D and -U either joined to the flag or as
# the argument that follows it.  Each check below compiles a file that only
# succeeds when the value arrived intact, so a value that was dropped or truncated
# fails the test instead of passing quietly.
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"

WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/davecc-option-forms.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

mkdir incdir
printf '%s\n' 'int from_header;' > incdir/hdr.h
cat > uses-header.c <<'SRC'
#include "hdr.h"
int main(void) { return from_header; }
SRC
cat > checks-macro.c <<'SRC'
int probe[MYVAL == 7 ? 1 : -1];
int main(void) { return sizeof probe; }
SRC
cat > undefines-macro.c <<'SRC'
#ifdef MYVAL
#error MYVAL survived -U
#endif
int main(void) { return 0; }
SRC

expect_ok() {
  local what="$1"
  shift
  if ! "$DAVECC" -target aarch64 -fsyntax-only "$@" >"$WORK/out" 2>&1; then
    echo "$what" >&2
    cat "$WORK/out" >&2
    exit 1
  fi
}

for spelling in joined separate; do
  if [[ "$spelling" == joined ]]; then
    include=("-Iincdir")
    define=("-DMYVAL=7")
    undefine=("-DMYVAL=7" "-UMYVAL")
  else
    include=("-I" "incdir")
    define=("-D" "MYVAL=7")
    undefine=("-D" "MYVAL=7" "-U" "MYVAL")
  fi
  expect_ok "$spelling -I did not add the include directory" \
    "${include[@]}" uses-header.c
  expect_ok "$spelling -D did not define the macro with its value" \
    "${define[@]}" checks-macro.c
  expect_ok "$spelling -U did not undefine the macro" \
    "${undefine[@]}" undefines-macro.c
done

# The relative path above is resolved against the working directory, so an
# absolute one exercises a separate branch of the include search.
expect_ok "separate -I did not accept an absolute directory" \
  -I "$WORK/incdir" uses-header.c

# A flag left with no value at all must be reported rather than silently taken to
# have an empty one.
if "$DAVECC" -target aarch64 -fsyntax-only -Iincdir uses-header.c -I \
    >trailing.out 2>&1; then
  echo "a trailing -I with no directory was accepted" >&2
  exit 1
fi
if ! grep -q "needs a value" trailing.out; then
  echo "a trailing -I was not diagnosed:" >&2
  cat trailing.out >&2
  exit 1
fi

# A bare -O selects an optimization level and a bare -W the default warning set,
# so neither may swallow the argument that follows it.
expect_ok "a bare -O consumed the file that followed it" \
  -O -Iincdir uses-header.c
expect_ok "a bare -W consumed the file that followed it" \
  -W -Iincdir uses-header.c
