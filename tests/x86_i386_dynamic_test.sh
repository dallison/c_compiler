#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 davecc" >&2
  exit 2
fi

if [[ "$1" = /* ]]; then
  DAVECC="$1"
else
  ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
  DAVECC="$ROOT/$1"
fi

ROOT_DIR="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-.}"
WORK="${ROOT_DIR}/.x86_i386_dynamic_work"
rm -rf "$WORK"
mkdir -p "$WORK"
trap 'rm -rf "$WORK"' EXIT

if ! command -v docker >/dev/null 2>&1; then
  echo "docker required for i386 dynamic-linking tests" >&2
  exit 1
fi

cat >"$WORK/inner.c" <<'EOF'
int dynamic_value = 57;
int inner(int x) { return x + dynamic_value; }
EOF

cat >"$WORK/outer.c" <<'EOF'
extern int dynamic_value;
int inner(int);
static int local_value = 4;
int outer(int x) {
  const char *literal = "A";
  return inner(x) + dynamic_value + local_value + literal[0];
}
EOF

cat >"$WORK/main.c" <<'EOF'
int outer(int);
int main(void) { return outer(1); }
EOF

cat >"$WORK/gcc_pic.c" <<'EOF'
extern int dynamic_value;
int gcc_pic_read(void) { return dynamic_value; }
EOF

cat >"$WORK/gcc_main.c" <<'EOF'
int gcc_pic_read(void);
int main(void) { return gcc_pic_read(); }
EOF

cat >"$WORK/start.s" <<'EOF'
	.text
	.global _start
_start:
	call main
	movl %eax, %ebx
	movl $1, %eax
	int $0x80
EOF

"$DAVECC" -target x86 -nostdinc -fpic -shared \
  "$WORK/inner.c" -o "$WORK/libinner.so"
"$DAVECC" -target x86 -nostdinc -fpic -shared \
  -rpath "$WORK" -rpath /work \
  "$WORK/outer.c" "$WORK/libinner.so" -o "$WORK/libouter.so"

docker run --rm --platform linux/386 \
  -v "$WORK:/work" -w /work alpine:3.20 sh -c '
    apk add --no-cache gcc musl-dev >/dev/null
    gcc -m32 -c start.s -o start.o
    gcc -m32 -fPIC -c gcc_pic.c -o gcc_pic.o
  '

"$DAVECC" -target x86 -nostdinc -shared \
  -rpath "$WORK" -rpath /work \
  "$WORK/gcc_pic.o" "$WORK/libinner.so" -o "$WORK/libgccpic.so"
"$DAVECC" -target x86 -nostdinc -nostdlib -fpic -rpath /work \
  "$WORK/start.o" "$WORK/main.c" "$WORK/libouter.so" -o "$WORK/program"
"$DAVECC" -target x86 -nostdinc -nostdlib -fpic -rpath /work \
  "$WORK/start.o" "$WORK/gcc_main.c" "$WORK/libgccpic.so" \
  -o "$WORK/gcc_pic_program"

docker run --rm --platform linux/386 \
  -v "$WORK:/work" -w /work alpine:3.20 sh -c '
    set -eu
    apk add --no-cache binutils file >/dev/null
    file libinner.so libouter.so libgccpic.so program gcc_pic_program |
      grep -q "ELF 32-bit"
    readelf -Wr libouter.so | grep -q R_386_JUMP_SLOT
    readelf -Wr libouter.so | grep -q R_386_GLOB_DAT
    readelf -Wr libouter.so | grep -q R_386_RELATIVE
    readelf -d libouter.so | grep -q BIND_NOW
    section_metadata="$(
      readelf -S libinner.so libouter.so libgccpic.so program gcc_pic_program 2>&1
    )"
    if echo "$section_metadata" | grep -Eq "Warning:|Error:"; then
      echo "$section_metadata" >&2
      echo "invalid i386 dynamic section metadata" >&2
      exit 1
    fi
    symbol_metadata="$(
      readelf --dyn-syms \
        libinner.so libouter.so libgccpic.so program gcc_pic_program 2>&1
    )"
    if echo "$symbol_metadata" | grep -Eq "Warning:|Error:"; then
      echo "$symbol_metadata" >&2
      echo "invalid i386 dynamic symbol metadata" >&2
      exit 1
    fi
    if readelf -d libinner.so libouter.so libgccpic.so program gcc_pic_program |
        grep -q TEXTREL; then
      echo "i386 PIC output unexpectedly contains TEXTREL" >&2
      exit 1
    fi
    set +e
    /lib/ld-musl-i386.so.1 --library-path /work ./program
    status=$?
    set -e
    if [ "$status" -ne 184 ]; then
      echo "i386 dynamic executable failed: expected 184, got $status" >&2
      exit 1
    fi
    set +e
    /lib/ld-musl-i386.so.1 --library-path /work ./gcc_pic_program
    status=$?
    set -e
    if [ "$status" -ne 57 ]; then
      echo "GNU i386 PIC object failed: expected 57, got $status" >&2
      exit 1
    fi
  '

docker run --rm --platform linux/386 \
  -v "$WORK:/work" -w /work debian:bookworm-slim sh -c '
    set +e
    /lib/ld-linux.so.2 --library-path /work ./program
    status=$?
    set -e
    if [ "$status" -ne 184 ]; then
      echo "i386 glibc dynamic executable failed: expected 184, got $status" >&2
      exit 1
    fi
    set +e
    /lib/ld-linux.so.2 --library-path /work ./gcc_pic_program
    status=$?
    set -e
    if [ "$status" -ne 57 ]; then
      echo "GNU i386 PIC object under glibc failed: expected 57, got $status" >&2
      exit 1
    fi
  '

echo "x86 i386 dynamic-linking test passed with musl and glibc"
