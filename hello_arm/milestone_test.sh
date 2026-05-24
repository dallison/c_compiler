#!/bin/bash
set -uo pipefail

ARM="$1"
DYNAMIC="$2"
STATIC="$3"

cd "$(dirname "$DYNAMIC")"

run_expect() {
  local expected=$1
  shift
  set +e
  "$@"
  local status=$?
  set -e
  test "$status" -eq "$expected"
}

run_expect 99 "$ARM" hello_dynamic.exe
run_expect 99 env LD_BIND_NOW=1 "$ARM" hello_dynamic.exe
run_expect 99 env LD_BIND_NOW=1 "$ARM" hello_dynamic.exe
run_expect 42 "$ARM" "$STATIC"
