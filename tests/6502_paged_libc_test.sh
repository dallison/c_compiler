#!/bin/bash
# Build the sideways libc and check the language-ROM header, the fixed
# vector table, the bank-switch shim, and --gc-sections on unused stubs.
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 davecc elfdump archivist" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$1"
ELFDUMP="$2"
ARCHIVIST="$3"
if [[ "$DAVECC" != /* ]]; then
  DAVECC="$ROOT/$DAVECC"
fi
if [[ "$ELFDUMP" != /* ]]; then
  ELFDUMP="$ROOT/$ELFDUMP"
fi
if [[ "$ARCHIVIST" != /* ]]; then
  ARCHIVIST="$ROOT/$ARCHIVIST"
fi

WORK="${TEST_TMPDIR:-/tmp}/6502-paged-libc"
mkdir -p "$(dirname "$WORK")"
"$ROOT/6502 support/paged/build_paged_libc.sh" \
  "$DAVECC" "$ARCHIVIST" "$ELFDUMP" "$WORK"

python3 - "$WORK/libc6502_paged.bin" <<'PY'
import sys
image = open(sys.argv[1], "rb").read()
if len(image) != 0x4000:
    raise SystemExit("sideways image is %d bytes" % len(image))
if image[0] != 0x4C or image[3] != 0x4C:
    raise SystemExit("header is not two JMP instructions")
if image[6] != 0xC0:
    raise SystemExit("ROM type is 0x%02x, expected language+service" % image[6])
offset = image[7]
text = image[offset:offset + 32]
if b"(C)" not in text:
    raise SystemExit("copyright string at offset %d does not contain (C)" % offset)
if image[0x100] != 0x4C:
    raise SystemExit("vector 0 at 0x8100 is not a JMP")
print("header ok, copyright at +0x%02x" % offset)
PY

# strlen only. printf's stub must disappear; the bank-switch stub must stay.
cat >"$WORK/strlen_only.c" <<'EOF'
#include <string.h>
int main(void) {
  return (int)strlen("paged");
}
EOF

"$DAVECC" -target 6502 -c -isystem "$ROOT/libc/include" \
  "$WORK/strlen_only.c" -o "$WORK/strlen_only.o"

runtime=()
for obj in "$WORK/runtime"/*.o; do
  base="$(basename "$obj")"
  if [[ "$base" == "bbc_syscall.o" || "$base" == "bbc_mos.o" ]]; then
    continue
  fi
  runtime+=("$obj")
done

"$DAVECC" -target 6502 -static -nostdlib --gc-sections \
  -Wl,-T -Wl,"$ROOT/6502 support/bbc.ld" \
  "$WORK/strlen_only.o" "${runtime[@]}" \
  "$WORK/libc6502_paged_shim.a" \
  -o "$WORK/strlen_only.elf"

if "$ELFDUMP" -s "$WORK/strlen_only.elf" | grep -Eq ' malloc$'; then
  echo "malloc shim survived --gc-sections" >&2
  exit 1
fi
if ! "$ELFDUMP" -s "$WORK/strlen_only.elf" | grep -Eq ' strlen$'; then
  echo "strlen shim was removed" >&2
  exit 1
fi
if "$ELFDUMP" -s "$WORK/strlen_only.elf" | grep -Eq ' sin$'; then
  echo "sin shim survived a strlen-only link" >&2
  exit 1
fi
if ! "$ELFDUMP" -s "$WORK/strlen_only.elf" | grep -q __libc_paged_enter; then
  echo "bank-switch enter was removed" >&2
  exit 1
fi

"$ELFDUMP" -c "$WORK/strlen_only.elf" >"$WORK/strlen_only.dis"
python3 - "$WORK/strlen_only.dis" <<'PY'
import sys
text = open(sys.argv[1], encoding="utf-8", errors="replace").read().lower()
if "0xfe30" not in text:
    raise SystemExit("shim does not write the ROM select latch at &FE30")
if "0xf4" not in text:
    raise SystemExit("shim does not update the MOS ROM id at &F4")
print("bank switch ok")
PY

for extra in libm math_a math_b cxx stdio float; do
  python3 - "$WORK/libc6502_paged_$extra.bin" "$extra" <<'PY'
import sys
image = open(sys.argv[1], "rb").read()
label = sys.argv[2]
if len(image) != 0x4000 or image[0] != 0x4C or image[6] != 0x80:
    raise SystemExit("%s is not a 16 KiB service ROM" % label)
if image[0x100] != 0x4C:
    raise SystemExit("%s vector 0 is not a JMP" % label)
print("%s service rom ok" % label)
PY
done

cat >"$WORK/sin_only.c" <<'EOF'
#include <math.h>
int main(void) {
  return (int)sin(0.0);
}
EOF
"$DAVECC" -target 6502 -c -isystem "$ROOT/libc/include" \
  "$WORK/sin_only.c" -o "$WORK/sin_only.o"
"$DAVECC" -target 6502 -static -nostdlib --gc-sections \
  -Wl,-T -Wl,"$ROOT/6502 support/bbc.ld" \
  "$WORK/sin_only.o" "${runtime[@]}" \
  "$WORK/libc6502_paged_shim.a" \
  -o "$WORK/sin_only.elf"
if "$ELFDUMP" -s "$WORK/sin_only.elf" | grep -Eq ' malloc$'; then
  echo "sin link kept malloc" >&2
  exit 1
fi
if ! "$ELFDUMP" -s "$WORK/sin_only.elf" | grep -Eq ' sin$'; then
  echo "sin link dropped sin" >&2
  exit 1
fi
"$ELFDUMP" -s "$WORK/sin_only.elf" | python3 -c '
import sys
ok = False
for line in sys.stdin:
    parts = line.split()
    if len(parts) >= 7 and parts[-1] == "__libc_paged_gate" and parts[1] == "00001f00":
        ok = True
if not ok:
    raise SystemExit("paged gate is not at &1F00")
print("gate at &1F00")
'
"$ELFDUMP" -c "$WORK/sin_only.elf" >"$WORK/sin_only.dis"
python3 - "$WORK/sin_only.dis" <<'PY'
import sys
text = open(sys.argv[1], encoding="utf-8", errors="replace").read().lower()
if "lda #1" not in text and "lda #0x1" not in text and "lda #0x01" not in text:
    raise SystemExit("sin stub does not select image 1")
print("sin selects libm image")
PY

cat >"$WORK/printf_only.c" <<'EOF'
#include <stdio.h>
int main(int argc, char** argv) {
  return printf(argv[0], argc);
}
EOF
"$DAVECC" -target 6502 -c -isystem "$ROOT/libc/include" -DDAVECC_PAGED_LIBC \
  "$WORK/printf_only.c" -o "$WORK/printf_only.o"
"$DAVECC" -target 6502 -static -nostdlib --gc-sections \
  -Wl,-T -Wl,"$ROOT/6502 support/bbc.ld" \
  "$WORK/printf_only.o" "${runtime[@]}" \
  "$WORK/libc6502_paged_shim.a" \
  -o "$WORK/printf_only.elf"
if "$ELFDUMP" -s "$WORK/printf_only.elf" | grep -Eq ' sin$'; then
  echo "printf link kept sin" >&2
  exit 1
fi
if ! "$ELFDUMP" -s "$WORK/printf_only.elf" | grep -Eq ' printf$'; then
  echo "printf link dropped printf" >&2
  exit 1
fi
"$ELFDUMP" -c "$WORK/printf_only.elf" >"$WORK/printf_only.dis"
python3 - "$WORK/printf_only.dis" <<'PY'
import sys
text = open(sys.argv[1], encoding="utf-8", errors="replace").read().lower()
if "lda #5" not in text and "lda #0x5" not in text and "lda #0x05" not in text:
    raise SystemExit("printf stub does not select image 5")
print("printf selects the stdio image")
PY

if ! "$ELFDUMP" -s "$WORK/libc6502_paged.elf" | grep -q __paged_init_stdio; then
  echo "primary ROM does not build the cassette FILE objects" >&2
  exit 1
fi
cat >"$WORK/stdio_main.c" <<'EOF'
#include <stdio.h>
int main(void) {
  return fputc('A', stdout);
}
EOF
"$DAVECC" -target 6502 -c -isystem "$ROOT/libc/include" -DDAVECC_PAGED_LIBC \
  "$WORK/stdio_main.c" -o "$WORK/stdio_main.o"
"$DAVECC" -target 6502 -static -nostdlib --gc-sections \
  -Wl,-T -Wl,"$ROOT/6502 support/bbc.ld" \
  "$WORK/stdio_main.o" "${runtime[@]}" \
  "$WORK/libc6502_paged_shim.a" \
  -o "$WORK/stdio_main.elf"
"$ELFDUMP" -c "$WORK/stdio_main.elf" >"$WORK/stdio_main.dis"
python3 - "$WORK/stdio_main.dis" <<'PY'
import sys
text = open(sys.argv[1], encoding="utf-8", errors="replace").read().lower()
if "lda #0xe2" not in text or "lda #0x09" not in text:
    raise SystemExit("stdout is not the cassette pointer at &09E2")
print("stdout is the cassette pointer")
PY

echo "paged libc test ok"

# The target tuple is the whole command: no -nostdlib, no -T, no archive list.
cat >"$WORK/paged_main.c" <<'EOF'
#ifndef __DAVECC_BBC__
#error __DAVECC_BBC__ was not defined
#endif
#ifndef __DAVECC_PAGED_LIBC__
#error __DAVECC_PAGED_LIBC__ was not defined
#endif
#include <string.h>
int main(void) {
  return (int)strlen("paged");
}
EOF

cat >"$WORK/not_bbc.c" <<'EOF'
#ifdef __DAVECC_BBC__
#error plain 6502 must not select the paged BBC runtime
#endif
int not_bbc;
EOF

"$DAVECC" -target 6502 -c "$WORK/not_bbc.c" -o "$WORK/not_bbc.o"

if "$DAVECC" -target 6502-acorn-paged-davecc -fsyntax-only \
    "$WORK/not_bbc.c" >/dev/null 2>"$WORK/bad_target.txt"; then
  echo "OS paged was accepted" >&2
  exit 1
fi

DAVECC_LIB_DIR="$WORK" "$DAVECC" -target 6502-acorn-bbc-davecc \
  "$WORK/paged_main.c" -o "$WORK/paged_main.elf"

if "$ELFDUMP" -s "$WORK/paged_main.elf" | grep -Eq ' malloc$'; then
  echo "driver link kept the malloc shim" >&2
  exit 1
fi
if ! "$ELFDUMP" -s "$WORK/paged_main.elf" | grep -Eq ' strlen$'; then
  echo "driver link dropped strlen" >&2
  exit 1
fi
if ! "$ELFDUMP" -s "$WORK/paged_main.elf" | grep -q '00001f00'; then
  echo "driver link did not place code at bbc.ld origin &1F00" >&2
  "$ELFDUMP" -s "$WORK/paged_main.elf" >&2
  exit 1
fi

cat >"$WORK/oswrch_main.c" <<'EOF'
#include <bbc.h>
int main(void) {
  oswrch('A');
  return 0;
}
EOF
DAVECC_LIB_DIR="$WORK" "$DAVECC" -target 65c02-acorn-bbc-davecc \
  "$WORK/oswrch_main.c" -o "$WORK/oswrch_main.elf"
if ! "$ELFDUMP" -s "$WORK/oswrch_main.elf" | grep -Eq ' oswrch$'; then
  echo "driver link dropped oswrch" >&2
  exit 1
fi

echo "paged target tuple ok"
