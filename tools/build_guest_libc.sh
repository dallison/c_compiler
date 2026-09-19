#!/usr/bin/env bash
# Build a DaveCC guest libc archive or shared object the same way the Bazel
# genrules do: assemble the target runtime, compile libc C/C++ sources with
# davecc, then either archive with archivist or link -shared.
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
usage: build_guest_libc.sh --davecc PATH --output PATH
                           --target TRIPLE [--archivist PATH]
                           [--cflags FLAG ...] [--runtime FILE ...]
                           [--source FILE ...] [--cxx-source FILE ...]
                           [--exclude GLOB ...] [--no-cxx] [--shared]
EOF
  exit 2
}

davecc=
archivist=
output=
target=
no_cxx=0
shared=0
cflags=()
runtime_srcs=()
c_srcs=()
cxx_srcs=()
excludes=()

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) davecc=$2; shift 2 ;;
    --archivist) archivist=$2; shift 2 ;;
    --output) output=$2; shift 2 ;;
    --target) target=$2; shift 2 ;;
    --cflags) cflags+=("$2"); shift 2 ;;
    --runtime) runtime_srcs+=("$2"); shift 2 ;;
    --source) c_srcs+=("$2"); shift 2 ;;
    --cxx-source) cxx_srcs+=("$2"); shift 2 ;;
    --exclude) excludes+=("$2"); shift 2 ;;
    --no-cxx) no_cxx=1; shift ;;
    --shared) shared=1; shift ;;
    -h|--help) usage ;;
    *) echo "unknown option: $1" >&2; usage ;;
  esac
done

[ -n "$davecc" ] && [ -n "$output" ] && [ -n "$target" ] || usage
if [ "$shared" -eq 0 ]; then
  [ -n "$archivist" ] || usage
fi

workspace=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." >/dev/null 2>&1 && pwd)
cd "$workspace"

work=$(mktemp -d "${TMPDIR:-/tmp}/davecc-libc.XXXXXX")
trap 'rm -rf "$work"' EXIT

compile_flags=(-target "$target" -c -isystem libc/include -Ilibc "${cflags[@]+"${cflags[@]}"}")
if [ "$shared" -eq 1 ]; then
  compile_flags+=(-fPIC)
fi

excluded() {
  local src=$1
  local pattern
  for pattern in "${excludes[@]+"${excludes[@]}"}"; do
    # shellcheck disable=SC2254
    case "$src" in
      $pattern) return 0 ;;
    esac
  done
  return 1
}

for src in "${runtime_srcs[@]+"${runtime_srcs[@]}"}"; do
  obj="$work/rt_$(basename "${src%.*}").o"
  "$davecc" "${compile_flags[@]}" "$src" -o "$obj"
done

if [ "${#c_srcs[@]}" -eq 0 ]; then
  while IFS= read -r src; do
    c_srcs+=("$src")
  done < <(find libc -maxdepth 1 -name '*.c' | sort)
fi

for src in "${c_srcs[@]}"; do
  excluded "$src" && continue
  case "$src" in
    *.c) ;;
    *) continue ;;
  esac
  case "$(basename "$src")" in
    libc_test.c) continue ;;
  esac
  extra=()
  if [ "$src" = "libc/printf.c" ] && [ "$target" = "esp32" ]; then
    extra+=(-DPRINTF_DISABLE_FLOAT=1)
  fi
  obj="$work/$(basename "${src%.c}").o"
  "$davecc" "${compile_flags[@]}" ${extra[@]+"${extra[@]}"} "$src" -o "$obj"
done

if [ "$no_cxx" -eq 0 ]; then
  if [ "${#cxx_srcs[@]}" -eq 0 ]; then
    while IFS= read -r src; do
      cxx_srcs+=("$src")
    done < <(find libc -maxdepth 1 -name '*.cc' | sort)
  fi
  for src in "${cxx_srcs[@]+"${cxx_srcs[@]}"}"; do
    excluded "$src" && continue
    case "$src" in
      *.cc) ;;
      *) continue ;;
    esac
    obj="$work/cxx_$(basename "${src%.cc}").o"
    "$davecc" "${compile_flags[@]}" -std=c++20 "$src" -o "$obj"
  done
fi

mkdir -p "$(dirname "$output")"
case "$output" in
  /*) out=$output ;;
  *) out=$workspace/$output ;;
esac

if [ "$shared" -eq 1 ]; then
  (
    cd "$work"
    "$davecc" -target "$target" -nostdlib -shared *.o -o "$out"
  )
else
  (
    cd "$work"
    "$archivist" r "$out" *.o
  )
fi
