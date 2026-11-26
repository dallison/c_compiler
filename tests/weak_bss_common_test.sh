#!/bin/sh
# Weak uninitialized globals must be ELF commons (not .data zero images) so:
#   1) object files stay small
#   2) a larger strong common merges with the weak one (same address)
set -eu

root="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
davecc="$root/$1"
shift
work="$(mktemp -d "${TMPDIR:-/tmp}/weak-bss-common.XXXXXX")"
if [ -z "${DAVECC_KEEP_TEST_WORK:-}" ]; then
  trap 'rm -rf "$work"' EXIT
else
  echo "test work directory: $work"
fi

cat >"$work/weak.c" <<'EOF'
__attribute__((weak)) unsigned long long arena[(256 * 1024) / 8];
unsigned long long* get_weak_arena(void) { return arena; }
EOF

cat >"$work/strong.c" <<'EOF'
unsigned long long arena[(1024 * 1024) / 8];
unsigned long long* get_weak_arena(void);
int main(void) {
  unsigned long long* w = get_weak_arena();
  if (w != arena) return 2;
  arena[0] = 11;
  if (w[0] != 11) return 3;
  arena[(1024 * 1024) / 8 - 1] = 22;
  if (w[(1024 * 1024) / 8 - 1] != 22) return 4;
  return 0;
}
EOF

status=0
while [ "$#" -ge 3 ]; do
  target=$1
  libc="$root/$2"
  interpreter="$root/$3"
  shift 3

  "$davecc" -target "$target" -O0 -nostdinc -c "$work/weak.c" -o "$work/weak.o"
  # Weak arena must not bloat the object with a 256KiB .data image.
  weak_bytes=$(wc -c <"$work/weak.o")
  if [ "$weak_bytes" -gt 8192 ]; then
    echo "FAIL: $target weak.o is $weak_bytes bytes (expected COMMON, not .data)"
    status=1
    continue
  fi

  "$davecc" -target "$target" -O0 -nostdinc -c "$work/strong.c" -o "$work/strong.o"
  "$davecc" -target "$target" -static -nostdlib -Wl,-e -Wl,main \
    "$work/strong.o" "$work/weak.o" "$libc" -o "$work/prog"
  rc=0
  "$interpreter" -i "$work/prog" || rc=$?
  if [ "$rc" -ne 0 ]; then
    echo "FAIL: $target linked prog exited $rc"
    status=1
  fi
done
exit "$status"
