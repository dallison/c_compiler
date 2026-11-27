#!/usr/bin/env bash
# Cross-compile DaveCC tools for aarch64-unknown-daveos-davecc using host davecc.
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
usage: build_daveos_tools.sh --davecc PATH --archivist PATH --libc PATH
                             --crt PATH --lds PATH --outdir DIR [--jobs N]
EOF
  exit 2
}

DAVECC=
ARCHIVIST=
LIBC=
CRT=
LDS=
OUTDIR=
JOBS=4
TARGET=aarch64-unknown-daveos-davecc

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --archivist) ARCHIVIST=$2; shift 2 ;;
    --libc) LIBC=$2; shift 2 ;;
    --crt) CRT=$2; shift 2 ;;
    --lds) LDS=$2; shift 2 ;;
    --outdir) OUTDIR=$2; shift 2 ;;
    --jobs) JOBS=$2; shift 2 ;;
    -h|--help) usage ;;
    *) echo "unknown option: $1" >&2; usage ;;
  esac
done

[ -n "$DAVECC" ] && [ -n "$ARCHIVIST" ] && [ -n "$LIBC" ] && \
  [ -n "$CRT" ] && [ -n "$LDS" ] && [ -n "$OUTDIR" ] || usage
[ -x "$DAVECC" ] || { echo "davecc not executable: $DAVECC" >&2; exit 1; }
[ -x "$ARCHIVIST" ] || { echo "archivist not executable: $ARCHIVIST" >&2; exit 1; }
[ -f "$LIBC" ] || { echo "missing libc: $LIBC" >&2; exit 1; }
[ -f "$CRT" ] || { echo "missing crt: $CRT" >&2; exit 1; }
[ -f "$LDS" ] || { echo "missing lds: $LDS" >&2; exit 1; }

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)
repo_root=$(cd "$script_dir/.." >/dev/null 2>&1 && pwd)
cd "$repo_root"

SRC_LIST="$script_dir/daveos_tool_lib_srcs.txt"
INC_LIST="$script_dir/daveos_tool_includes.txt"
[ -f "$SRC_LIST" ] && [ -f "$INC_LIST" ] || {
  echo "missing source/include lists under tools/" >&2
  exit 1
}

work="$OUTDIR/.build"
rm -rf "$work"
mkdir -p "$work/obj" "$OUTDIR/bin" "$OUTDIR/lib"

INC_ARGS="-isystem libc/include -Ilibc"
while IFS= read -r inc; do
  [ -n "$inc" ] || continue
  INC_ARGS="$INC_ARGS -I$inc"
done <"$INC_LIST"

export DAVECC TARGET INC_ARGS work
compile_one() {
  src=$1
  obj=$2
  mkdir -p "$(dirname "$obj")"
  # shellcheck disable=SC2086
  # C11: compiler sources use _Static_assert / _Alignof.
  # Some TUs miscompile at -O2 for aarch64-daveos (0xffffffff text pads /
  # bad ELF objects). Keep those at -O1 until the optimizer is fixed.
  opt=O2
  case "$src" in
    Linker/linker_script.c|c_compiler/ELF/elf_writer.c|c_compiler/ELF/elf_reader.c) opt=O1 ;;
  esac
  "$DAVECC" -target "$TARGET" -std=c11 -$opt -c \
    -DDAVECC_SYSROOT_HDRS=libc/include \
    $INC_ARGS \
    "$src" -o "$obj"
}
export -f compile_one

# Build compile job list for xargs.
jobfile="$work/compile_jobs.txt"
: >"$jobfile"
while IFS= read -r src; do
  [ -n "$src" ] || continue
  [ -f "$src" ] || { echo "missing source $src" >&2; exit 1; }
  obj="$work/obj/${src%.c}.o"
  printf '%s\t%s\n' "$src" "$obj" >>"$jobfile"
done <"$SRC_LIST"

echo "compiling $(wc -l <"$jobfile") library sources with -j$JOBS..."
if command -v xargs >/dev/null 2>&1; then
  cat "$jobfile" | tr '\t' '\n' | xargs -n 2 -P "$JOBS" bash -c 'compile_one "$0" "$1"'
else
  while IFS="$(printf '\t')" read -r src obj; do
    compile_one "$src" "$obj"
  done <"$jobfile"
fi

OBJ_DIR="$work/obj"
( cd "$OBJ_DIR" && find . -name '*.o' -print ) >"$work/obj_list.txt"
[ -s "$work/obj_list.txt" ] || { echo "no library objects" >&2; exit 1; }
LIBA="$work/libdaveos_tools.a"
rm -f "$LIBA"
# shellcheck disable=SC2046
( cd "$OBJ_DIR" && "$ARCHIVIST" r "$LIBA" $(cat "$work/obj_list.txt") )
echo "archived $(wc -l <"$work/obj_list.txt") objects → $LIBA"

crt_obj="$work/crt0.o"
"$DAVECC" -target "$TARGET" -O2 -c "$CRT" -o "$crt_obj"

link_tool() {
  name=$1
  main=$2
  main_obj="$work/main_${name}.o"
  compile_one "$main" "$main_obj"
  "$DAVECC" -target "$TARGET" -static -nostdlib -O2 \
    -Wl,-e -Wl,_start -Wl,-T -Wl,"$LDS" \
    "$crt_obj" "$main_obj" "$LIBA" "$LIBC" \
    -o "$OUTDIR/bin/$name"
  echo "built $OUTDIR/bin/$name ($(wc -c <"$OUTDIR/bin/$name") bytes)"
}

link_tool davecc davecc/main.c
link_tool daveld daveld/main.c
link_tool archivist archivist/main.c
link_tool elfdump elfdump/main.c
link_tool aarch64asm aarch64asm/main.c
link_tool aarch64dasm aarch64dasm/main.c
link_tool 6502dasm 6502dasm/main.c
link_tool armdasm armdasm/main.c
link_tool riscvdasm riscvdasm/main.c
link_tool x86_64dasm x86_64dasm/main.c
link_tool xtensadasm xtensadasm/main.c
link_tool bpfdasm bpfdasm/main.c
link_tool pcodedasm pcodedasm/main.c
link_tool wasmdasm wasmdasm/main.c

# CRT + linker script for on-box compiles against /lib/libcdaveos.a.
cp "$crt_obj" "$OUTDIR/lib/crt0.o"
cp "$LDS" "$OUTDIR/lib/link_libc.ld"

rm -rf "$work"
echo "DaveOS tools ready in $OUTDIR/bin (crt in $OUTDIR/lib)"

