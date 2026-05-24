#!/bin/bash
# Full libc test driver for DaveCC targets.
set -uo pipefail

usage() {
  echo "usage: $0 [--compile-all] [--compiler] [--runtime] <davecc> <target> [interpreter]" >&2
  echo "  default: run all selected suites (all if none specified)" >&2
  exit 2
}

do_compile_all=0
do_compiler=0
do_runtime=0

while [ "$#" -gt 0 ]; do
  case "$1" in
    --compile-all) do_compile_all=1; shift ;;
    --compiler) do_compiler=1; shift ;;
    --runtime) do_runtime=1; shift ;;
    -h|--help) usage ;;
    --) shift; break ;;
    -*) echo "unknown option: $1" >&2; usage ;;
    *) break ;;
  esac
done

if [ "$#" -lt 2 ]; then
  usage
fi

if [ "$do_compile_all" -eq 0 ] && [ "$do_compiler" -eq 0 ] && [ "$do_runtime" -eq 0 ]; then
  do_compile_all=1
  do_compiler=1
  do_runtime=1
fi

davecc=$1
target=$2
interpreter=${3:-}

if [ -f "libc/tests/run_tests.sh" ]; then
  :
elif [ -n "${BUILD_WORKSPACE_DIRECTORY:-}" ]; then
  cd "$BUILD_WORKSPACE_DIRECTORY"
else
  cd "$(dirname "$0")/../.."
fi

failed=0

run_step() {
  local name=$1
  shift
  echo "== $name =="
  if "$@"; then
    echo "== $name: ok =="
  else
    echo "== $name: FAILED =="
    failed=1
  fi
  echo
}

if [ "$do_compile_all" -eq 1 ]; then
  run_step "compile all libc sources" \
    bash libc/tests/compile_all.sh "$davecc" "$target"
fi

if [ "$do_compiler" -eq 1 ]; then
  run_step "compiler regression (-O0)" \
    bash libc/tests/compiler_regression.sh "$davecc" "$target" -O0

  run_step "compiler regression (-O1)" \
    bash libc/tests/compiler_regression.sh "$davecc" "$target" -O1
fi

if [ "$do_runtime" -eq 1 ] && { [ "$target" = "x86_64" ] || [ "$target" = "riscv" ] || [ "$target" = "risc-v" ]; } && [ -n "$interpreter" ]; then
  work=$(mktemp -d "${TMPDIR:-/tmp}/libc_runtime_test.XXXXXX")
  trap 'rm -rf "$work"' EXIT

  case "$target" in
    risc-v) target=riscv ;;
  esac

  case "$target" in
    x86_64)
      rt_cflags=(-target x86_64 -O1 -c -isystem libc/include -Ilibc)
      link_cflags=(-target x86_64 -O1 -static -Wl,-e -Wl,main)
      ;;
    riscv)
      rt_cflags=(-target riscv -O1 -c -isystem libc/include -Ilibc)
      link_cflags=(-target riscv -O1 -static -Wl,-e -Wl,main)
      ;;
  esac

  smoke_exe="$work/smoke.exe"
  smoke_obj="$work/smoke.o"
  "$davecc" "${rt_cflags[@]}" libc/tests/runtime/smoke.c -o "$smoke_obj"
  "$davecc" "${link_cflags[@]}" "$smoke_obj" -o "$smoke_exe"
  run_step "runtime smoke test" env INTERP="$interpreter" EXE="$smoke_exe" TARGET="$target" bash -c '
    if [ "$TARGET" = x86_64 ]; then
      "$INTERP" -i "$EXE"
    else
      "$INTERP" "$EXE"
    fi
    test $? -eq 0
  '

  test_objs=()
  for src in \
      libc/tests/runtime/main.c \
      libc/tests/runtime/test_core.c; do
    obj="$work/$(basename "${src%.c}").o"
    "$davecc" "${rt_cflags[@]}" -Ilibc/tests "$src" -o "$obj"
    test_objs+=("$obj")
  done

  exe="$work/libc_runtime_test.exe"
  if [ -n "${LIBC_ARCHIVE:-}" ] && [ -f "$LIBC_ARCHIVE" ]; then
    "$davecc" "${link_cflags[@]}" \
      "${test_objs[@]}" \
      -o "$exe" \
      "$LIBC_ARCHIVE"
  else
    abs_obj="$work/abs.o"
    "$davecc" "${rt_cflags[@]}" libc/abs.c -o "$abs_obj"
    "$davecc" "${link_cflags[@]}" \
      "${test_objs[@]}" "$abs_obj" \
      -o "$exe"
  fi

  run_step "runtime libc tests" env INTERP="$interpreter" EXE="$exe" TARGET="$target" bash -c '
    if [ "$TARGET" = x86_64 ]; then
      "$INTERP" -i "$EXE"
    else
      "$INTERP" "$EXE"
    fi
    test $? -eq 0
  '

  if [ "${LIBC_FULL_RUNTIME_TEST:-0}" = "1" ] && [ "$target" = "x86_64" ]; then
    runtime_srcs=(
      "x86_64 support/setjmp.s"
      "x86_64 support/longjmp.s"
      "x86_64 support/syscall.s"
      "x86_64 support/abs.s"
    )

    runtime_objs=()
    for src in "${runtime_srcs[@]}"; do
      obj="$work/$(basename "${src%.s}").o"
      "$davecc" -target x86_64 -O1 -c -isystem libc/include "$src" -o "$obj"
      runtime_objs+=("$obj")
    done

    test_objs=()
    for src in \
        libc/tests/runtime/main.c \
        libc/tests/runtime/test_core.c; do
      obj="$work/$(basename "${src%.c}").o"
      "$davecc" -target x86_64 -O1 -c -isystem libc/include -Ilibc/tests "$src" -o "$obj"
      test_objs+=("$obj")
    done

    exe="$work/libc_runtime_test.exe"
    "$davecc" -target x86_64 -static -Wl,-e -Wl,main \
      "${test_objs[@]}" "${runtime_objs[@]}" \
      -o "$exe"

    run_step "runtime libc tests (full)" env INTERP="$interpreter" EXE="$exe" bash -c '"$INTERP" -i "$EXE"; test $? -eq 0'
  fi
fi

if [ "$do_runtime" -eq 1 ] && [ "$target" != "x86_64" ] && [ "$target" != "riscv" ] && [ "$target" != "risc-v" ]; then
  echo "SKIP runtime tests (only supported for x86_64 and riscv)"
fi

if [ "$do_runtime" -eq 1 ] && { [ "$target" = "x86_64" ] || [ "$target" = "riscv" ]; } && [ -z "$interpreter" ]; then
  echo "SKIP runtime tests (no interpreter provided)"
fi

if [ "$failed" -ne 0 ]; then
  exit 1
fi

echo "all libc tests passed"
