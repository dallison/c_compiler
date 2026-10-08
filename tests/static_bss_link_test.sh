#!/bin/sh
# Uninitialized `static` variables of the same name in two translation units
# are distinct objects: each is a local .bss symbol (not a global common the
# linker merges), and every object's .bss gets its own space when linked.
# Statics named like registers (si, x0, r1, a0, ...) are symbols, not
# registers.
set -eu

root="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
davecc="$root/$1"
shift
work="$(mktemp -d "${TMPDIR:-/tmp}/static-bss-link.XXXXXX")"
if [ -z "${DAVECC_KEEP_TEST_WORK:-}" ]; then
  trap 'rm -rf "$work"' EXIT
else
  echo "test work directory: $work"
fi

cat >"$work/one.c" <<'EOF'
static long sv;
static long si = 3, di, x0 = 4, w1, r1 = 5, a0, sp = 6, lr, pc = 7, ra;
long get(void) { return sv + si + di + x0 + w1 + r1 + a0 + sp + lr + pc + ra; }
void set(long v) { sv = v; di = 1; w1 = 1; a0 = 1; lr = 1; ra = 1; }
EOF

cat >"$work/two.c" <<'EOF'
static long sv;
static long ra;
long get(void);
void set(long);
int main(void) {
  sv = 100;
  ra = 200;
  set(5);
  if (sv != 100 || ra != 200) return 1;
  return get() == 5 + 3 + 4 + 5 + 6 + 7 + 5 ? 0 : 2;
}
EOF

status=0
while [ "$#" -ge 3 ]; do
  target=$1
  libc="$root/$2"
  interpreter="$root/$3"
  shift 3
  for opt in -O0 -O2; do
    "$davecc" -target "$target" "$opt" -nostdinc -c "$work/one.c" \
      -o "$work/one.o"
    "$davecc" -target "$target" "$opt" -nostdinc -c "$work/two.c" \
      -o "$work/two.o"
    "$davecc" -target "$target" -static -nostdlib -Wl,-e -Wl,main \
      "$work/one.o" "$work/two.o" "$libc" -o "$work/prog"
    rc=0
    "$interpreter" -i "$work/prog" || rc=$?
    if [ "$rc" -ne 0 ]; then
      echo "FAIL: $target $opt exited $rc"
      status=1
    fi
  done
done
exit "$status"
