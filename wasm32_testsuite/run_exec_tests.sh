#!/bin/bash
#
# Differential execution tests for the wasm32 backend.
#
# Each test is a self-contained C program whose exit status is its answer.
# The same program is built twice, once by the host clang and once by davecc
# for wasm32, and the two answers have to agree.  Comparing against a second
# compiler rather than a recorded value means a test cannot silently bless
# whatever the backend currently does.
#
# A test is either a single .c file or a directory of them that make up one
# program, which is how the cross-object cases are written.
#
# A test under tests/hosted is a different animal: it links against the
# wasm32 libc and runs as a WASI command, so what it prints is checked as
# well as what it returns.  Everything else is freestanding and is reached
# by calling main directly.
#
# Usage: run_exec_tests.sh [davecc] [test.c|test_dir ...]

set -u

DAVECC=${1:-bazel-bin/davecc}
shift 2>/dev/null || true

if [ $# -eq 0 ]; then
  set -- "$(dirname "$0")"/tests/*.c "$(dirname "$0")"/tests/link/*/ \
         "$(dirname "$0")"/tests/hosted/*.c
fi

ARCHIVIST=${ARCHIVIST:-bazel-bin/archivist}

for tool in wasm-validate wasmtime clang; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "run_exec_tests.sh: $tool is not installed" >&2
    exit 2
  fi
done

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

pass=0
fail=0

for source in "$@"; do
  case "$source" in
    */hosted/*)
      name=$(basename "$source" .c)
      if ! clang -w -o "$work/$name.native" "$source" -lm 2>/dev/null; then
        echo "SKIP $name: the host compiler rejected it"
        continue
      fi
      want_output=$("$work/$name.native")
      want=$?

      if ! error=$("$DAVECC" -target wasm32 "$source" -o "$work/$name.wasm" \
                     2>&1); then
        echo "FAIL $name: $(echo "$error" | head -1)"
        fail=$((fail + 1))
        continue
      fi
      if ! error=$(wasm-validate "$work/$name.wasm" 2>&1); then
        echo "FAIL $name: invalid module: $(echo "$error" | head -1)"
        fail=$((fail + 1))
        continue
      fi
      got_output=$(wasmtime run "$work/$name.wasm" 2>/dev/null)
      got=$?
      if [ "$got" != "$want" ]; then
        echo "FAIL $name: exit want $want, got $got"
        fail=$((fail + 1))
      elif [ "$got_output" != "$want_output" ]; then
        echo "FAIL $name: output differs"
        diff <(echo "$want_output") <(echo "$got_output") | head -8
        fail=$((fail + 1))
      else
        pass=$((pass + 1))
      fi
      continue
      ;;
  esac

  # A 'lib' subdirectory holds units that reach the link through an archive
  # rather than by being named, which is what exercises lazy pull-in.
  library=()
  if [ -d "$source" ]; then
    name=$(basename "${source%/}")
    sources=("$source"/*.c)
    if [ -d "$source/lib" ]; then
      library=("$source"/lib/*.c)
    fi
  else
    name=$(basename "$source" .c)
    sources=("$source")
  fi

  # The reference build has to see the same archive structure, because a
  # member nothing needs is meant not to be linked and may well not even
  # resolve if it were.
  native_library=()
  if [ ${#library[@]} -gt 0 ]; then
    native_members=()
    for unit in "${library[@]}"; do
      object="$work/$name.native.$(basename "$unit" .c).o"
      if ! clang -w -c -o "$object" "$unit" 2>/dev/null; then
        echo "SKIP $name: the host compiler rejected it"
        continue 2
      fi
      native_members+=("$object")
    done
    rm -f "$work/libnative$name.a"
    ar rcs "$work/libnative$name.a" "${native_members[@]}" 2>/dev/null
    native_library=("$work/libnative$name.a")
  fi

  # The +"..." form is how an empty array expands to nothing rather than to
  # an unbound variable, which the bash that ships with macOS still needs.
  if ! clang -w -o "$work/$name.native" "${sources[@]}" \
         ${native_library[@]+"${native_library[@]}"} 2>/dev/null; then
    echo "SKIP $name: the host compiler rejected it"
    continue
  fi
  "$work/$name.native"
  want=$?

  objects=()
  broken=""
  for unit in "${sources[@]}"; do
    object="$work/$name.$(basename "$unit" .c).o"
    if ! error=$("$DAVECC" -target wasm32 -c "$unit" -o "$object" 2>&1); then
      echo "FAIL $name: $(echo "$error" | head -1)"
      broken=yes
      break
    fi
    # An object is itself a valid module, so checking it separates a bad
    # encoding from a bad link.
    if ! error=$(wasm-validate "$object" 2>&1); then
      echo "FAIL $name: invalid object: $(echo "$error" | head -1)"
      broken=yes
      break
    fi
    objects+=("$object")
  done
  archive=()
  if [ -z "$broken" ] && [ ${#library[@]} -gt 0 ]; then
    members=()
    for unit in "${library[@]}"; do
      object="$work/$name.lib.$(basename "$unit" .c).o"
      if ! error=$("$DAVECC" -target wasm32 -c "$unit" -o "$object" 2>&1); then
        echo "FAIL $name: $(echo "$error" | head -1)"
        broken=yes
        break
      fi
      members+=("$object")
    done
    rm -f "$work/lib$name.a"
    if [ -z "$broken" ] &&
       ! error=$("$ARCHIVIST" rc "$work/lib$name.a" "${members[@]}" 2>&1); then
      echo "FAIL $name: archive: $(echo "$error" | head -1)"
      broken=yes
    fi
    archive=("$work/lib$name.a")
  fi
  if [ -n "$broken" ]; then
    fail=$((fail + 1))
    continue
  fi

  if ! error=$("$DAVECC" -target wasm32 "${objects[@]}" \
                 ${archive[@]+"${archive[@]}"} -o "$work/$name.wasm" 2>&1); then
    echo "FAIL $name: link: $(echo "$error" | head -1)"
    fail=$((fail + 1))
    continue
  fi

  if ! error=$(wasm-validate "$work/$name.wasm" 2>&1); then
    echo "FAIL $name: invalid module: $(echo "$error" | head -1)"
    fail=$((fail + 1))
    continue
  fi

  # These programs are freestanding, so there is no _start to run and main
  # is called directly.  It always takes the three arguments C allows it,
  # whether or not it was written with them, because a wasm call has to
  # match the signature exactly.
  got=$(wasmtime run --invoke main "$work/$name.wasm" 0 0 0 2>/dev/null)
  # A process exit status is only the low eight bits, so mask what wasm
  # returned the same way before comparing.
  if [ "$((got & 255))" = "$want" ]; then
    pass=$((pass + 1))
  else
    echo "FAIL $name: want $want, got $got"
    fail=$((fail + 1))
  fi
done

echo "wasm32 exec tests: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
