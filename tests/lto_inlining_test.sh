#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc x86_64" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/lto-inlining"
mkdir -p "$WORK"

cat >"$WORK/add.c" <<'EOF'
int add2(int x) { return x + 2; }
EOF

cat >"$WORK/main.c" <<'EOF'
int add2(int);
__attribute__((noinline)) int caller(void) { return add2(3); }
int main(void) { return caller(); }
EOF

cat >"$WORK/static_a.c" <<'EOF'
static int helper(void) { return 1; }
int from_a(void) { return helper(); }
EOF

cat >"$WORK/static_b.c" <<'EOF'
static int helper(void) { return 2; }
int from_b(void) { return helper(); }
int main(void) { return from_a() + from_b(); }
EOF

common=(
  "$DAVECC" -target x86_64 -O2 -nostdinc -nostdlib -static
)

# Without LTO the callee body is not visible, so the call remains.
"${common[@]}" -S "$WORK/main.c" -o "$WORK/main.s"
if ! grep -Eq '[[:space:]]call[[:space:]]+add2' "$WORK/main.s"; then
  echo "expected a call to add2 without LTO" >&2
  cat "$WORK/main.s" >&2
  exit 1
fi

# Combined -flto compile inlines add2 regardless of file order.
for order in add_first main_first; do
  local_s="$WORK/combined_${order}.s"
  if [[ "$order" == add_first ]]; then
    "${common[@]}" -flto -S "$WORK/add.c" "$WORK/main.c" -o "$local_s"
  else
    "${common[@]}" -flto -S "$WORK/main.c" "$WORK/add.c" -o "$local_s"
  fi
  if grep -Eq '[[:space:]]call[[:space:]]+add2' "$local_s"; then
    echo "LTO $order retained a call to add2" >&2
    cat "$local_s" >&2
    exit 1
  fi
done

"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/add.c" "$WORK/main.c" \
  -o "$WORK/lto.exe"
set +e
"$INTERPRETER" -i "$WORK/lto.exe"
rc=$?
set -e
if [[ "$rc" -ne 5 ]]; then
  echo "LTO executable returned $rc; expected 5" >&2
  exit 1
fi

# File-scope statics with the same source name must not collide.
"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/static_a.c" "$WORK/static_b.c" \
  -o "$WORK/static.exe"
set +e
"$INTERPRETER" -i "$WORK/static.exe"
rc=$?
set -e
if [[ "$rc" -ne 3 ]]; then
  echo "LTO static-collision executable returned $rc; expected 3" >&2
  exit 1
fi

# -c -flto writes relocatable source stubs; linking them recompiles together.
"${common[@]}" -c -flto "$WORK/add.c" -o "$WORK/add.o"
"${common[@]}" -c -flto "$WORK/main.c" -o "$WORK/main.o"
"${common[@]}" -O2 -Wl,-e -Wl,main "$WORK/add.o" "$WORK/main.o" \
  -o "$WORK/stub.exe"
set +e
"$INTERPRETER" -i "$WORK/stub.exe"
rc=$?
set -e
if [[ "$rc" -ne 5 ]]; then
  echo "LTO stub link returned $rc; expected 5" >&2
  exit 1
fi

# Include guards in a shared header must survive the second TU.
cat >"$WORK/h.h" <<'EOF'
#ifndef H_H
#define H_H
struct Point { int x, y; };
static inline int sum(struct Point p) { return p.x + p.y; }
#endif
EOF

cat >"$WORK/hdr_a.c" <<'EOF'
#include "h.h"
int from_a(void) {
  struct Point p;
  p.x = 3;
  p.y = 4;
  return sum(p);
}
EOF

cat >"$WORK/hdr_b.c" <<'EOF'
#include "h.h"
int from_a(void);
int main(void) {
  struct Point p;
  p.x = 1;
  p.y = 2;
  return from_a() + sum(p);
}
EOF

"${common[@]}" -flto -I"$WORK" -Wl,-e -Wl,main "$WORK/hdr_a.c" "$WORK/hdr_b.c" \
  -o "$WORK/hdr.exe"
set +e
"$INTERPRETER" -i "$WORK/hdr.exe"
rc=$?
set -e
if [[ "$rc" -ne 10 ]]; then
  echo "LTO include-guard executable returned $rc; expected 10" >&2
  exit 1
fi

# Same-name file-scope typedefs and tags in two .c files.
cat >"$WORK/type_a.c" <<'EOF'
typedef int T;
struct S { int x; };
enum { K = 1 };
int from_a(void) {
  T v = K;
  struct S s;
  s.x = v;
  return s.x;
}
EOF

cat >"$WORK/type_b.c" <<'EOF'
typedef int T;
struct S { int x; };
enum { K = 2 };
int from_a(void);
int main(void) {
  T v = K;
  struct S s;
  s.x = v;
  return from_a() + s.x;
}
EOF

"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/type_a.c" "$WORK/type_b.c" \
  -o "$WORK/type.exe"
set +e
"$INTERPRETER" -i "$WORK/type.exe"
rc=$?
set -e
if [[ "$rc" -ne 3 ]]; then
  echo "LTO file-scope type executable returned $rc; expected 3" >&2
  exit 1
fi

# Tentative globals in two TUs are one object.
cat >"$WORK/tent_a.c" <<'EOF'
int g;
int bump(void) { return ++g; }
EOF

cat >"$WORK/tent_b.c" <<'EOF'
int g;
int bump(void);
int main(void) { return bump() + bump(); }
EOF

"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/tent_a.c" "$WORK/tent_b.c" \
  -o "$WORK/tent.exe"
set +e
"$INTERPRETER" -i "$WORK/tent.exe"
rc=$?
set -e
if [[ "$rc" -ne 3 ]]; then
  echo "LTO tentative executable returned $rc; expected 3" >&2
  exit 1
fi

# -c -flto must restore -D from the stub at link time.
cat >"$WORK/def_a.c" <<'EOF'
int value(void) { return VALUE; }
EOF

cat >"$WORK/def_b.c" <<'EOF'
int value(void);
int main(void) { return value(); }
EOF

"${common[@]}" -c -flto -DVALUE=7 "$WORK/def_a.c" -o "$WORK/def_a.o"
"${common[@]}" -c -flto "$WORK/def_b.c" -o "$WORK/def_b.o"
"${common[@]}" -Wl,-e -Wl,main "$WORK/def_a.o" "$WORK/def_b.o" \
  -o "$WORK/def.exe"
set +e
"$INTERPRETER" -i "$WORK/def.exe"
rc=$?
set -e
if [[ "$rc" -ne 7 ]]; then
  echo "LTO stub -D link returned $rc; expected 7" >&2
  exit 1
fi

# -c -flto must restore -I from the stub at link time.
"${common[@]}" -c -flto -I"$WORK" "$WORK/hdr_a.c" -o "$WORK/hdr_a.o"
"${common[@]}" -c -flto -I"$WORK" "$WORK/hdr_b.c" -o "$WORK/hdr_b.o"
"${common[@]}" -Wl,-e -Wl,main "$WORK/hdr_a.o" "$WORK/hdr_b.o" \
  -o "$WORK/hdr_stub.exe"
set +e
"$INTERPRETER" -i "$WORK/hdr_stub.exe"
rc=$?
set -e
if [[ "$rc" -ne 10 ]]; then
  echo "LTO stub -I link returned $rc; expected 10" >&2
  exit 1
fi
