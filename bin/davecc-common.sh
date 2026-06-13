#!/usr/bin/env bash

davecc_script_dir() {
  local src=${BASH_SOURCE[0]}
  while [ -L "$src" ]; do
    local dir
    dir=$(cd -P "$(dirname "$src")" >/dev/null 2>&1 && pwd)
    src=$(readlink "$src")
    case "$src" in
      /*) ;;
      *) src="$dir/$src" ;;
    esac
  done
  cd -P "$(dirname "$src")" >/dev/null 2>&1 && pwd
}

DAVECC_BIN_DIR=${DAVECC_BIN_DIR:-$(davecc_script_dir)}
DAVECC_ROOT=${DAVECC_ROOT:-$(cd "$DAVECC_BIN_DIR/.." >/dev/null 2>&1 && pwd)}
if [ -f "$DAVECC_BIN_DIR/davecc-env.sh" ]; then
  source "$DAVECC_BIN_DIR/davecc-env.sh"
fi
DAVECC_LIBEXEC_DIR=${DAVECC_LIBEXEC_DIR:-"$DAVECC_BIN_DIR/../libexec/davecc"}

davecc_find_executable() {
  local name=$1
  shift
  local candidate
  for candidate in "$@"; do
    if [ -n "$candidate" ] && [ -x "$candidate" ]; then
      echo "$candidate"
      return 0
    fi
  done
  if command -v "$name" >/dev/null 2>&1; then
    command -v "$name"
    return 0
  fi
  echo "unable to find executable '$name'" >&2
  return 1
}

davecc_driver() {
  davecc_find_executable davecc \
    "${DAVECC:-}" \
    "$DAVECC_LIBEXEC_DIR/davecc" \
    "$DAVECC_BIN_DIR/davecc" \
    "$DAVECC_ROOT/bazel-bin/davecc"
}

davecc_interpreter() {
  local arch=$1
  local exe_name=$2
  davecc_find_executable "$exe_name" \
    "${DAVECC_INTERPRETER:-}" \
    "$DAVECC_LIBEXEC_DIR/$exe_name" \
    "$DAVECC_LIBEXEC_DIR/davecc-$arch-interpreter" \
    "$DAVECC_BIN_DIR/$exe_name" \
    "$DAVECC_ROOT/bazel-bin/$exe_name"
}

davecc_include_dir() {
  local candidate
  for candidate in \
    "${DAVECC_INCLUDE_DIR:-}" \
    "$DAVECC_ROOT/libc/include" \
    "$DAVECC_BIN_DIR/../include/davecc" \
    "$DAVECC_BIN_DIR/../include"; do
    if [ -n "$candidate" ] && [ -d "$candidate" ]; then
      echo "$candidate"
      return 0
    fi
  done
  echo "unable to find DaveCC libc headers; set DAVECC_INCLUDE_DIR" >&2
  return 1
}

davecc_lib_dir() {
  local candidate
  for candidate in \
    "${DAVECC_LIB_DIR:-}" \
    "$DAVECC_ROOT/bazel-bin/libc" \
    "$DAVECC_BIN_DIR/../lib/davecc"; do
    if [ -n "$candidate" ] && [ -d "$candidate" ]; then
      echo "$candidate"
      return 0
    fi
  done
  echo "unable to find DaveCC libc archives; set DAVECC_LIB_DIR" >&2
  return 1
}

davecc_libc_archive() {
  local archive=$1
  local lib_dir
  lib_dir=$(davecc_lib_dir)
  if [ ! -f "$lib_dir/$archive" ]; then
    echo "unable to find libc archive '$archive' in $lib_dir" >&2
    return 1
  fi
  echo "$lib_dir/$archive"
}

davecc_6502_rom() {
  local candidate
  for candidate in \
    "${DAVECC_6502_ROM:-}" \
    "$DAVECC_ROOT/bazel-bin/6502_support/6502rom.exe" \
    "$(davecc_lib_dir 2>/dev/null)/6502rom.exe"; do
    if [ -n "$candidate" ] && [ -f "$candidate" ]; then
      echo "$candidate"
      return 0
    fi
  done
  echo "unable to find 65c02 ROM; set DAVECC_6502_ROM" >&2
  return 1
}

davecc_should_link_libc() {
  local arg
  for arg in "$@"; do
    case "$arg" in
      -c|-S|-shared|-nostdlib)
        return 1
        ;;
      *.a|*/libc*.a)
        return 1
        ;;
    esac
  done
  return 0
}

davecc_has_arg() {
  local needle=$1
  shift
  local arg
  for arg in "$@"; do
    if [ "$arg" = "$needle" ]; then
      return 0
    fi
  done
  return 1
}

davecc_compile_target() {
  local target=$1
  local libc_archive=$2
  local needs_main_entry=$3
  shift 3

  local driver include_dir
  driver=$(davecc_driver)
  include_dir=$(davecc_include_dir)

  local args=("-target" "$target" "-isystem" "$include_dir")
  if davecc_should_link_libc "$@"; then
    if ! davecc_has_arg "-static" "$@"; then
      args+=("-static")
    fi
    if [ "$needs_main_entry" = "yes" ]; then
      args+=("-Wl,-e" "-Wl,main")
    fi
  fi

  args+=("$@")
  if davecc_should_link_libc "$@"; then
    args+=("$(davecc_libc_archive "$libc_archive")")
  fi

  exec "$driver" "${args[@]}"
}

davecc_run_target() {
  local arch=$1
  local exe_name=$2
  local force_interpret=$3
  shift 3

  local interp
  interp=$(davecc_interpreter "$arch" "$exe_name")

  local args=()
  if [ "$force_interpret" = "yes" ] &&
     ! davecc_has_arg "-i" "$@" &&
     ! davecc_has_arg "--interpret" "$@" &&
     ! davecc_has_arg "-n" "$@" &&
     ! davecc_has_arg "--native" "$@"; then
    args+=("-i")
  fi
  args+=("$@")

  exec "$interp" "${args[@]}"
}
