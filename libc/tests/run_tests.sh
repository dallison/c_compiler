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

if [ "$do_runtime" -eq 1 ] && { [ "$target" = "x86_64" ] || [ "$target" = "aarch64" ] || [ "$target" = "arm" ] || [ "$target" = "riscv" ] || [ "$target" = "risc-v" ]; } && [ -n "$interpreter" ]; then
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
    aarch64)
      rt_cflags=(-target aarch64 -O0 -c -isystem libc/include -Ilibc)
      link_cflags=(-target aarch64 -O0 -static -Wl,-e -Wl,main)
      ;;
    arm)
      rt_cflags=(-target arm -O0 -c -isystem libc/include -Ilibc)
      link_cflags=(-target arm -O0 -static -Wl,-e -Wl,main)
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
    if [ "$TARGET" = x86_64 ] || [ "$TARGET" = aarch64 ] || [ "$TARGET" = arm ]; then
      "$INTERP" -i "$EXE"
    else
      "$INTERP" "$EXE"
    fi
    test $? -eq 0
  '

  if [ "$target" = "x86_64" ] || [ "$target" = "aarch64" ] || [ "$target" = "arm" ] || [ "$target" = "riscv" ]; then
    tls_exe="$work/tls_local_exec.exe"
    tls_obj="$work/tls_local_exec.o"
    "$davecc" "${rt_cflags[@]}" libc/tests/runtime/tls_local_exec.c -o "$tls_obj"
    "$davecc" "${link_cflags[@]}" "$tls_obj" -o "$tls_exe"
    run_step "runtime tls local-exec test" env INTERP="$interpreter" EXE="$tls_exe" TARGET="$target" bash -c '
      if [ "$TARGET" = riscv ]; then "$INTERP" "$EXE"; else "$INTERP" -i "$EXE"; fi
      test $? -eq 0
    '

    thread_exe="$work/thread_tls_isolation.exe"
    thread_obj="$work/thread_tls_isolation.o"
    threads_obj="$work/threads.o"
    cxx_tls_obj="$work/cxx_tls.o"
    cxx_tls_stubs_obj="$work/cxx_tls_stubs.o"
    runtime_sources=("aarch64 support/syscall.s")
    if [ "$target" = "x86_64" ]; then
      runtime_sources=("x86_64 support/syscall.s" "x86_64 support/abs.s")
    elif [ "$target" = "arm" ]; then
      runtime_sources=("arm support/syscall.s")
    elif [ "$target" = "riscv" ]; then
      runtime_sources=("RISCV_support/eh_transfer.s" "libc/syscall.c")
    fi
    runtime_objs=()
    for src in "${runtime_sources[@]}"; do
      obj="$work/rt_$(basename "${src%.s}").o"
      "$davecc" "${rt_cflags[@]}" "$src" -o "$obj"
      runtime_objs+=("$obj")
    done
    "$davecc" "${rt_cflags[@]}" libc/tests/runtime/thread_tls_isolation.c -o "$thread_obj"
    "$davecc" "${rt_cflags[@]}" libc/threads.c -o "$threads_obj"
    "$davecc" "${rt_cflags[@]}" libc/cxx_tls.c -o "$cxx_tls_obj"
    "$davecc" "${rt_cflags[@]}" libc/cxx_tls_stubs.c -o "$cxx_tls_stubs_obj"
    thread_libc_objs=()
    if [ -z "${LIBC_ARCHIVE:-}" ] || [ ! -f "$LIBC_ARCHIVE" ]; then
      for src in \
          libc/malloc.c \
          libc/free.c \
          libc/realloc.c \
          libc/calloc.c \
          libc/guest_heap.c \
          libc/memset.c \
          libc/errno.c \
          libc/posix.c; do
        obj="$work/thread_libc_$(basename "${src%.c}").o"
        "$davecc" "${rt_cflags[@]}" "$src" -o "$obj"
        thread_libc_objs+=("$obj")
      done
    fi
    if [ -n "${LIBC_ARCHIVE:-}" ] && [ -f "$LIBC_ARCHIVE" ]; then
      "$davecc" "${link_cflags[@]}" "$thread_obj" "$threads_obj" "$cxx_tls_obj" \
        "$cxx_tls_stubs_obj" "${runtime_objs[@]}" "$LIBC_ARCHIVE" -o "$thread_exe"
    else
      "$davecc" "${link_cflags[@]}" "$thread_obj" "$threads_obj" "$cxx_tls_obj" \
        "$cxx_tls_stubs_obj" "${thread_libc_objs[@]}" "${runtime_objs[@]}" \
        -o "$thread_exe"
    fi
    run_step "runtime thread tls isolation test" env INTERP="$interpreter" EXE="$thread_exe" TARGET="$target" bash -c '
      if [ "$TARGET" = riscv ]; then "$INTERP" "$EXE"; else "$INTERP" -i "$EXE"; fi
      test $? -eq 0
    '

    link_thread_test() {
      local name=$1
      local src=$2
      local exe="$work/${name}.exe"
      local obj="$work/${name}.o"
      "$davecc" "${rt_cflags[@]}" "$src" -o "$obj"
      if [ -n "${LIBC_ARCHIVE:-}" ] && [ -f "$LIBC_ARCHIVE" ]; then
        "$davecc" "${link_cflags[@]}" "$obj" "$threads_obj" "$cxx_tls_obj" \
          "$cxx_tls_stubs_obj" "${runtime_objs[@]}" "$LIBC_ARCHIVE" -o "$exe"
      else
        "$davecc" "${link_cflags[@]}" "$obj" "$threads_obj" "$cxx_tls_obj" \
          "$cxx_tls_stubs_obj" "${thread_libc_objs[@]}" "${runtime_objs[@]}" \
          -o "$exe"
      fi
      run_step "runtime ${name} test" env INTERP="$interpreter" EXE="$exe" TARGET="$target" bash -c '
        if [ "$TARGET" = riscv ]; then "$INTERP" "$EXE"; else "$INTERP" -i "$EXE"; fi
        test $? -eq 0
      '
    }

    link_thread_libc_test() {
      local name=$1
      local src=$2
      local exe="$work/${name}.exe"
      local obj="$work/${name}.o"
      "$davecc" "${rt_cflags[@]}" "$src" -o "$obj"
      if [ -n "${LIBC_ARCHIVE:-}" ] && [ -f "$LIBC_ARCHIVE" ]; then
        "$davecc" "${link_cflags[@]}" "$obj" "$threads_obj" "$cxx_tls_obj" \
          "$cxx_tls_stubs_obj" "${runtime_objs[@]}" -o "$exe" "$LIBC_ARCHIVE"
      else
        "$davecc" "${link_cflags[@]}" "$obj" "$threads_obj" "$cxx_tls_obj" \
          "$cxx_tls_stubs_obj" "${thread_libc_objs[@]}" "${runtime_objs[@]}" \
          -o "$exe"
      fi
      run_step "runtime ${name} test" env INTERP="$interpreter" EXE="$exe" TARGET="$target" bash -c '
        if [ "$TARGET" = riscv ]; then "$INTERP" "$EXE"; else "$INTERP" -i "$EXE"; fi
        test $? -eq 0
      '
    }

    link_thread_test thread_join_negative libc/tests/runtime/thread_join_negative.c
    link_thread_test thread_thrd_exit libc/tests/runtime/thread_thrd_exit.c

    link_thread_libc_test thread_heap_stress libc/tests/runtime/thread_heap_stress.c
    link_thread_libc_test thread_errno_isolation libc/tests/runtime/thread_errno_isolation.c
  fi

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
  elif [ "$target" = "x86_64" ]; then
    runtime_objs=()
    for src in \
        "x86_64 support/setjmp.s" \
        "x86_64 support/longjmp.s" \
        "x86_64 support/eh_transfer.s" \
        "x86_64 support/syscall.s" \
        "x86_64 support/abs.s"; do
      obj="$work/rt_$(basename "${src%.s}").o"
      "$davecc" "${rt_cflags[@]}" "$src" -o "$obj"
      runtime_objs+=("$obj")
    done
    "$davecc" "${link_cflags[@]}" \
      "${test_objs[@]}" "${runtime_objs[@]}" \
      libc/eh_frame.c \
      -o "$exe"
  else
    abs_obj="$work/abs.o"
    "$davecc" "${rt_cflags[@]}" libc/abs.c -o "$abs_obj"
    "$davecc" "${link_cflags[@]}" \
      "${test_objs[@]}" "$abs_obj" \
      -o "$exe"
  fi

  run_step "runtime libc tests" env INTERP="$interpreter" EXE="$exe" TARGET="$target" bash -c '
    if [ "$TARGET" = x86_64 ] || [ "$TARGET" = aarch64 ] || [ "$TARGET" = arm ]; then
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
      "x86_64 support/eh_transfer.s"
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
      libc/eh_frame.c \
      -o "$exe"

    run_step "runtime libc tests (full)" env INTERP="$interpreter" EXE="$exe" bash -c '"$INTERP" -i "$EXE"; test $? -eq 0'
  fi
fi

if [ "$do_runtime" -eq 1 ] && [ "$target" != "x86_64" ] && [ "$target" != "aarch64" ] && [ "$target" != "arm" ] && [ "$target" != "riscv" ] && [ "$target" != "risc-v" ]; then
  echo "SKIP runtime tests (only supported for x86_64, aarch64, arm and riscv)"
fi

if [ "$do_runtime" -eq 1 ] && { [ "$target" = "x86_64" ] || [ "$target" = "aarch64" ] || [ "$target" = "arm" ] || [ "$target" = "riscv" ]; } && [ -z "$interpreter" ]; then
  echo "SKIP runtime tests (no interpreter provided)"
fi

if [ "$failed" -ne 0 ]; then
  exit 1
fi

echo "all libc tests passed"
