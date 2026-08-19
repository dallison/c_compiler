#!/bin/bash
# Exercise the complete emit, import, object, link, and execute module workflow.
set -uo pipefail

DAVECC=""
MODULEDUMP=""
TARGET=""
LIBC=""
INTERPRETER=""
STD_MODULE=""
STD_OBJECT=""
STD_ARTIFACTS=""
SUITE_ROOT=""
declare -a COMPILE_ARGS=()
declare -a INTERP_ARGS=()

usage() {
  echo "usage: $0 --davecc PATH --moduledump PATH --target NAME --libc PATH --interpreter PATH --std-artifacts PATHS" >&2
  exit 2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --moduledump) MODULEDUMP=$2; shift 2 ;;
    --target) TARGET=$2; shift 2 ;;
    --libc) LIBC=$2; shift 2 ;;
    --interpreter) INTERPRETER=$2; shift 2 ;;
    --std-artifacts) STD_ARTIFACTS=$2; shift 2 ;;
    --suite-root) SUITE_ROOT=$2; shift 2 ;;
    --compile-arg) COMPILE_ARGS+=("$2"); shift 2 ;;
    --interp-arg) INTERP_ARGS+=("$2"); shift 2 ;;
    *.dcm|*.o) STD_ARTIFACTS="$STD_ARTIFACTS $1"; shift ;;
    -h|--help) usage ;;
    *) echo "unknown option: $1" >&2; usage ;;
  esac
done

if [ -z "$DAVECC" ] || [ -z "$MODULEDUMP" ] || [ -z "$TARGET" ] ||
   [ -z "$LIBC" ] || [ -z "$INTERPRETER" ] || [ -z "$STD_ARTIFACTS" ]; then
  usage
fi

resolve_runfile() {
  local path=$1
  if [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
    local rooted="${TEST_SRCDIR}/${TEST_WORKSPACE}/${path}"
    if [ -e "$rooted" ]; then
      echo "$rooted"
      return
    fi
  fi
  echo "$path"
}

DAVECC=$(resolve_runfile "$DAVECC")
MODULEDUMP=$(resolve_runfile "$MODULEDUMP")
LIBC=$(resolve_runfile "$LIBC")
INTERPRETER=$(resolve_runfile "$INTERPRETER")
for artifact in $STD_ARTIFACTS; do
  artifact=$(resolve_runfile "$artifact")
  case "$artifact" in
    *.dcm) STD_MODULE=$artifact ;;
    *.o) STD_OBJECT=$artifact ;;
  esac
done
if [ -z "$STD_MODULE" ] || [ -z "$STD_OBJECT" ]; then
  usage
fi
if [ -n "$SUITE_ROOT" ]; then
  SUITE_ROOT=$(resolve_runfile "$SUITE_ROOT")
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  SUITE_ROOT="${TEST_SRCDIR}/${TEST_WORKSPACE}/cxx_testsuite"
else
  SUITE_ROOT="cxx_testsuite"
fi

FIXTURES="$SUITE_ROOT/tests/modules"
work=$(mktemp -d "${TEST_TMPDIR:-/tmp}/cxx-modules.XXXXXX")
if [ -z "${KEEP_MODULE_WORK:-}" ]; then
  trap 'rm -rf "$work"' EXIT
else
  echo "module test work directory: $work" >&2
fi

fail() {
  echo "FAIL: $1" >&2
  if [ -f "$work/command.log" ]; then
    sed 's/^/  /' "$work/command.log" >&2
  fi
  exit 1
}

run() {
  "$@" >"$work/command.log" 2>&1
}

echo "=== cxx_testsuite modules: target=$TARGET ==="

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fmodule-output "$work/hello.dcm" \
  -fdeps-file "$work/hello-deps.json" -fdeps-format=p1689r5 \
  "$FIXTURES/hello.cppm" -o "$work/hello.o" ||
  fail "compile coordinated module artifacts"
grep -Fq '"logical-name": "hello"' "$work/hello-deps.json" ||
  fail "scan provided module dependency"

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xload-module "$work/hello.dcm" ||
  fail "load hello.dcm"
grep -Fq "module hello:" "$work/command.log" ||
  fail "loaded module summary"

run "$MODULEDUMP" "$work/hello.dcm" ||
  fail "inspect hello.dcm"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_hello.cpp" \
  -o "$work/use_hello.o" ||
  fail "compile module importer"

run "$DAVECC" -target "$TARGET" -std=c++20 -S \
  -fmodule-file "hello=$work/hello.dcm" "$FIXTURES/use_hello.cpp" \
  -o "$work/use_hello_mapping.s" ||
  fail "compile importer with explicit module mapping"

run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_hello.o" "$work/hello.o" "$LIBC" -o "$work/hello.bin" ||
  fail "link module executable"

"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/hello.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute module program"

run "$DAVECC" -target "$TARGET" -std=c++20 -S \
  -fmodule-file "std=$STD_MODULE" "$FIXTURES/use_std.cpp" \
  -o "$work/use_std_cxx20.s"
if ! grep -Fq "requires C++23" "$work/command.log"; then
  fail "import std was not rejected before C++23"
fi

run "$DAVECC" -target "$TARGET" -std=c++23 -c \
  -fmodule-file "std=$STD_MODULE" "$FIXTURES/use_std.cpp" \
  -o "$work/use_std.o" ||
  fail "compile standard library module importer"

run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_std.o" "$STD_OBJECT" "$LIBC" -o "$work/std.bin" ||
  fail "link standard library module executable"

"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/std.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute standard library module program"

run "$DAVECC" -target "$TARGET" -std=c++23 -c \
  -fmodule-file "std=$STD_MODULE" \
  "$FIXTURES/use_std_containers.cpp" \
  -o "$work/use_std_containers.o" ||
  fail "compile standard container module importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_std_containers.o" "$STD_OBJECT" "$LIBC" \
  -o "$work/std_containers.bin" ||
  fail "link standard container module executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/std_containers.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute standard container module program"

run "$DAVECC" -target "$TARGET" -std=c++23 -c \
  -fmodule-file "std=$STD_MODULE" \
  "$FIXTURES/use_std_utilities.cpp" \
  -o "$work/use_std_utilities.o" ||
  fail "compile standard utility module importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_std_utilities.o" "$STD_OBJECT" "$LIBC" \
  -o "$work/std_utilities.bin" ||
  fail "link standard utility module executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/std_utilities.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute standard utility module program"

run "$DAVECC" -target "$TARGET" -std=c++23 -c \
  -fmodule-file "std=$STD_MODULE" \
  "$FIXTURES/use_std_heavy.cpp" \
  -o "$work/use_std_heavy.o" ||
  fail "compile template-heavy standard module importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_std_heavy.o" "$STD_OBJECT" "$LIBC" \
  -o "$work/std_heavy.bin" ||
  fail "link template-heavy standard module executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/std_heavy.bin" >"$work/command.log" 2>&1
std_heavy_status=$?
[ "$std_heavy_status" -eq 0 ] ||
  fail "execute template-heavy standard module program (status $std_heavy_status)"

run "$DAVECC" -target "$TARGET" -std=c++23 -c \
  -fmodule-file "std=$STD_MODULE" \
  "$FIXTURES/use_std_random.cpp" \
  -o "$work/use_std_random.o" ||
  fail "compile standard random module importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_std_random.o" "$STD_OBJECT" "$LIBC" \
  -o "$work/std_random.bin" ||
  fail "link standard random module executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/std_random.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute standard random module program"

run "$DAVECC" -target "$TARGET" -std=c++23 -c \
  -fmodule-file "std=$STD_MODULE" \
  "$FIXTURES/use_std_locale.cpp" \
  -o "$work/use_std_locale.o" ||
  fail "compile standard locale module importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_std_locale.o" "$STD_OBJECT" "$LIBC" \
  -o "$work/std_locale.bin" ||
  fail "link standard locale module executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/std_locale.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute standard locale module program"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fmodule-output "$work/template_template.dcm" \
  "$FIXTURES/template_template.cppm" -o "$work/template_template.o" ||
  fail "compile template-template parameter module"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fmodule-file "template_template=$work/template_template.dcm" \
  "$FIXTURES/use_template_template.cpp" \
  -o "$work/use_template_template.o" ||
  fail "import template-template parameter module"

run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_template_template.o" "$work/template_template.o" "$LIBC" \
  -o "$work/template_template.bin" ||
  fail "link template-template parameter module executable"

"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/template_template.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute template-template parameter module"

run "$DAVECC" -target "$TARGET" -std=c++20 -S \
  -fprebuilt-module-path "$work" "$FIXTURES/use_hidden.cpp" \
  -o "$work/use_hidden.s"
if ! grep -Fq "error:" "$work/command.log"; then
  fail "non-exported declaration was visible"
fi
grep -Fq "module_hidden" "$work/command.log" ||
  fail "hidden declaration diagnostic"

run "$DAVECC" -target "$TARGET" -std=c++20 -S \
  -fprebuilt-module-path "$work" "$FIXTURES/import_missing.cpp" \
  -o "$work/import_missing.s"
if ! grep -Fq "error:" "$work/command.log"; then
  fail "missing module import succeeded"
fi
grep -Fq "Cannot import module 'does_not_exist'" "$work/command.log" ||
  fail "missing module diagnostic"
grep -Fq "no prebuilt module file found" "$work/command.log" ||
  fail "missing module detail diagnostic"

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/hello_wrong_name.dcm" \
  "$FIXTURES/wrong_module_name.cppm" ||
  fail "emit mismatched module name archive"
cp "$work/hello.dcm" "$work/hello_good.dcm"
cp "$work/hello_wrong_name.dcm" "$work/hello.dcm"
run "$DAVECC" -target "$TARGET" -std=c++20 -S \
  -fprebuilt-module-path "$work" "$FIXTURES/use_hello.cpp" \
  -o "$work/use_wrong_name.s"
if ! grep -Fq "error:" "$work/command.log"; then
  fail "module name mismatch import succeeded"
fi
grep -Fq "module file declares 'other'" "$work/command.log" ||
  fail "module name mismatch diagnostic"
cp "$work/hello_good.dcm" "$work/hello.dcm"

printf 'not a module\n' >"$work/corrupt.dcm"
if run "$DAVECC" -target "$TARGET" -std=c++20 \
     -Xload-module "$work/corrupt.dcm"; then
  fail "corrupt module load succeeded"
fi

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/extension_probe.dcm" "$FIXTURES/extension_probe.ixx" ||
  fail "compile .ixx module interface"

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/surface.dcm" "$FIXTURES/surface.cppm" ||
  fail "emit surface.dcm"

run "$MODULEDUMP" "$work/surface.dcm" ||
  fail "inspect surface.dcm"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  "$FIXTURES/surface.cppm" -o "$work/surface.o" ||
  fail "compile surface module object"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_surface.cpp" \
  -o "$work/use_surface.o" ||
  fail "compile surface importer"

run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_surface.o" "$work/surface.o" "$LIBC" -o "$work/surface.bin" ||
  fail "link surface executable"

"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/surface.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute surface module program"

run "$DAVECC" -target "$TARGET" -std=c++23 \
  -Xemit-module "$work/template_surface.dcm" \
  "$FIXTURES/template_surface.cppm" ||
  fail "emit template surface module"
run "$DAVECC" -target "$TARGET" -std=c++23 -c \
  "$FIXTURES/template_surface.cppm" -o "$work/template_surface.o" ||
  fail "compile template surface module object"
run "$DAVECC" -target "$TARGET" -std=c++23 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_template_surface.cpp" \
  -o "$work/use_template_surface.o" ||
  fail "compile template surface importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_template_surface.o" "$work/template_surface.o" "$LIBC" \
  -o "$work/template_surface.bin" ||
  fail "link template surface executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/template_surface.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute template surface module program"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/template_parameter_objects.dcm" \
  "$FIXTURES/template_parameter_objects.cppm" ||
  fail "emit template parameter object module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  "$FIXTURES/template_parameter_objects.cppm" \
  -o "$work/template_parameter_objects.o" ||
  fail "compile template parameter object module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  -fprebuilt-module-path "$work" \
  "$FIXTURES/use_template_parameter_objects.cpp" \
  -o "$work/use_template_parameter_objects.o" ||
  fail "compile template parameter object importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_template_parameter_objects.o" \
  "$work/template_parameter_objects.o" "$LIBC" \
  -o "$work/template_parameter_objects.bin" ||
  fail "link template parameter object executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/template_parameter_objects.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute template parameter object module program"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/reflection_roundtrip.dcm" \
  "$FIXTURES/reflection_roundtrip.cppm" ||
  fail "emit reflection round-trip module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  "$FIXTURES/reflection_roundtrip.cppm" \
  -o "$work/reflection_roundtrip.o" ||
  fail "compile reflection round-trip module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  -fprebuilt-module-path "$work" \
  "$FIXTURES/use_reflection_roundtrip.cpp" \
  -o "$work/use_reflection_roundtrip.o" ||
  fail "compile reflection round-trip importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_reflection_roundtrip.o" \
  "$work/reflection_roundtrip.o" "$LIBC" \
  -o "$work/reflection_roundtrip.bin" ||
  fail "link reflection round-trip executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/reflection_roundtrip.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute reflection round-trip module program"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/trivial_union_lifetime.dcm" \
  "$FIXTURES/trivial_union_lifetime.cppm" ||
  fail "emit trivial union lifetime module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  "$FIXTURES/trivial_union_lifetime.cppm" \
  -o "$work/trivial_union_lifetime.o" ||
  fail "compile trivial union lifetime module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  -fprebuilt-module-path "$work" \
  "$FIXTURES/use_trivial_union_lifetime.cpp" \
  -o "$work/use_trivial_union_lifetime.o" ||
  fail "compile trivial union lifetime importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_trivial_union_lifetime.o" \
  "$work/trivial_union_lifetime.o" "$LIBC" \
  -o "$work/trivial_union_lifetime.bin" ||
  fail "link trivial union lifetime executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/trivial_union_lifetime.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute trivial union lifetime module program"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/erroneous_value_state.dcm" \
  "$FIXTURES/erroneous_value_state.cppm" ||
  fail "emit erroneous value-state module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  "$FIXTURES/erroneous_value_state.cppm" \
  -o "$work/erroneous_value_state.o" ||
  fail "compile erroneous value-state module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  -fprebuilt-module-path "$work" \
  "$FIXTURES/use_erroneous_value_state.cpp" \
  -o "$work/use_erroneous_value_state.o" ||
  fail "compile erroneous value-state importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_erroneous_value_state.o" \
  "$work/erroneous_value_state.o" "$LIBC" \
  -o "$work/erroneous_value_state.bin" ||
  fail "link erroneous value-state executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/erroneous_value_state.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute erroneous value-state module program"

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/reachability.dcm" "$FIXTURES/reachability.cppm" ||
  fail "emit reachability.dcm"
run "$MODULEDUMP" --symbols "$work/reachability.dcm" ||
  fail "inspect reachability.dcm"
if grep -Fq "unrelated_implementation_detail" "$work/command.log"; then
  fail "unrelated declaration leaked into reachability.dcm"
fi
if grep -Fq "private_implementation_detail" "$work/command.log"; then
  fail "private fragment declaration leaked into reachability.dcm"
fi
grep -Fq "add_offset" "$work/command.log" ||
  fail "reachable hidden helper missing from reachability.dcm"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  "$FIXTURES/reachability.cppm" -o "$work/reachability.o" ||
  fail "compile reachability module object"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_reachability.cpp" \
  -o "$work/use_reachability.o" ||
  fail "compile reachability importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_reachability.o" "$work/reachability.o" "$LIBC" \
  -o "$work/reachability.bin" ||
  fail "link reachability executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/reachability.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute reachability module program"

run "$DAVECC" -target "$TARGET" -std=c++20 -S \
  -fprebuilt-module-path "$work" "$FIXTURES/use_reachability_hidden.cpp" \
  -o "$work/use_reachability_hidden.s"
if ! grep -Fq "error:" "$work/command.log"; then
  fail "reachable-only declaration entered importer lookup"
fi

if run "$DAVECC" -target "$TARGET" -std=c++20 \
     -Xemit-module "$work/invalid_internal_reachability.dcm" \
     "$FIXTURES/invalid_internal_reachability.cppm"; then
  fail "internal-linkage exposure was accepted"
fi
grep -Fq "exposes internal-linkage declaration 'internal_helper'" \
  "$work/command.log" ||
  fail "internal-linkage exposure diagnostic"

if run "$DAVECC" -target "$TARGET" -std=c++20 \
     -Xemit-module "$work/invalid_private_reachability.dcm" \
     "$FIXTURES/invalid_private_reachability.cppm"; then
  fail "private-fragment exposure was accepted"
fi
grep -Fq "reaches private-fragment declaration 'private_helper'" \
  "$work/command.log" ||
  fail "private-fragment exposure diagnostic"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fdeps-file "$work/partitioned-deps.json" \
  -fdeps-format=p1689r5 -fdeps-scan-only \
  "$FIXTURES/partitioned.cppm" -o "$work/partitioned-scan.o" ||
  fail "scan partitioned module dependencies"
grep -Fq '"logical-name": "partitioned:detail"' \
  "$work/partitioned-deps.json" ||
  fail "scan interface partition dependency"
grep -Fq '"logical-name": "partitioned:impl"' \
  "$work/partitioned-deps.json" ||
  fail "scan internal partition dependency"

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/partitioned-detail.dcm" \
  "$FIXTURES/partitioned_detail.cppm" ||
  fail "emit interface partition"
run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/partitioned-impl.dcm" \
  "$FIXTURES/partitioned_impl.cpp" ||
  fail "emit internal partition"
run "$DAVECC" -target "$TARGET" -std=c++20 \
  -fprebuilt-module-path "$work" \
  -Xemit-module "$work/partitioned.dcm" "$FIXTURES/partitioned.cppm" ||
  fail "emit partitioned primary interface"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  "$FIXTURES/partitioned_detail.cppm" -o "$work/partitioned_detail.o" ||
  fail "compile interface partition object"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  "$FIXTURES/partitioned_impl.cpp" -o "$work/partitioned_impl.o" ||
  fail "compile internal partition object"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/partitioned.cppm" \
  -o "$work/partitioned.o" ||
  fail "compile partitioned primary object"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_partitioned.cpp" \
  -o "$work/use_partitioned.o" ||
  fail "compile partitioned importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_partitioned.o" "$work/partitioned.o" \
  "$work/partitioned_detail.o" "$work/partitioned_impl.o" "$LIBC" \
  -o "$work/partitioned.bin" ||
  fail "link partitioned executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/partitioned.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute partitioned module program"

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/private_dependency.dcm" \
  "$FIXTURES/private_dependency.cppm" ||
  fail "emit private dependency"
run "$DAVECC" -target "$TARGET" -std=c++20 \
  -fprebuilt-module-path "$work" \
  -Xemit-module "$work/reexport_middle.dcm" \
  "$FIXTURES/reexport_middle.cppm" ||
  fail "emit middle reexport"
run "$DAVECC" -target "$TARGET" -std=c++20 \
  -fprebuilt-module-path "$work" \
  -Xemit-module "$work/reexport_top.dcm" "$FIXTURES/reexport_top.cppm" ||
  fail "emit transitive reexport"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_reexport_top.cpp" \
  -o "$work/use_reexport_top.o" ||
  fail "compile transitive reexport importer"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  "$FIXTURES/private_dependency.cppm" -o "$work/private_dependency.o" ||
  fail "compile private dependency object"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_reexport_top.o" "$work/private_dependency.o" "$LIBC" \
  -o "$work/reexport_top.bin" ||
  fail "link transitive reexport executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/reexport_top.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute transitive reexport program"

run "$DAVECC" -target "$TARGET" -std=c++20 -c -fmodule-header \
  -fmodule-name '"header_unit.hpp"' \
  -fmodule-output "$work/header_unit.hpp.dcm" \
  -fdeps-file "$work/header-unit-deps.json" -fdeps-format=p1689r5 \
  "$FIXTURES/header_unit.hpp" -o "$work/header_unit.o" ||
  fail "compile coordinated header-unit artifacts"
grep -Fq 'header_unit.hpp' "$work/header-unit-deps.json" ||
  fail "scan provided header unit"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_header_unit.cpp" \
  -o "$work/use_header_unit.o" ||
  fail "compile header-unit importer"
if run "$DAVECC" -target "$TARGET" -std=c++20 -S \
     -fprebuilt-module-path "$work" \
     "$FIXTURES/use_header_unit_internal.cpp" \
     -o "$work/use_header_unit_internal.s"; then
  fail "header-unit internal-linkage declaration was visible"
fi
grep -Fq "header_unit_internal_value" "$work/command.log" ||
  fail "header-unit internal-linkage diagnostic"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_header_unit.o" "$work/header_unit.o" "$LIBC" \
  -o "$work/header_unit.bin" ||
  fail "link header-unit executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/header_unit.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute header-unit program"

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -fprebuilt-module-path "$work" \
  -Xemit-module "$work/not_reexporting.dcm" \
  "$FIXTURES/not_reexporting.cppm" ||
  fail "emit module with private dependency"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/not_reexporting.cppm" \
  -o "$work/not_reexporting.o" ||
  fail "compile private dependency wrapper object"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_not_reexporting.cpp" \
  -o "$work/use_not_reexporting.o" ||
  fail "compile private dependency importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_not_reexporting.o" "$work/not_reexporting.o" \
  "$work/private_dependency.o" "$LIBC" -o "$work/not_reexporting.bin" ||
  fail "link private dependency executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/not_reexporting.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute private dependency program"

run "$DAVECC" -target "$TARGET" -std=c++20 -S \
  -fprebuilt-module-path "$work" \
  "$FIXTURES/use_private_dependency_name.cpp" \
  -o "$work/use_private_dependency_name.s"
if ! grep -Fq "error:" "$work/command.log"; then
  fail "non-reexported dependency name was visible"
fi

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/identity_iface.dcm" "$FIXTURES/identity_iface.cppm" ||
  fail "emit identity_iface.dcm"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/identity_impl.cpp" \
  -o "$work/identity_impl.o" ||
  fail "compile identity implementation unit"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  "$FIXTURES/identity_iface.cppm" -o "$work/identity_iface.o" ||
  fail "compile identity interface object"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_identity_iface.cpp" \
  -o "$work/use_identity_iface.o" ||
  fail "compile identity importer"

run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_identity_iface.o" "$work/identity_iface.o" \
  "$work/identity_impl.o" "$LIBC" -o "$work/identity_iface.bin" ||
  fail "link identity executable"

"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/identity_iface.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute identity module program"

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/alpha.dcm" "$FIXTURES/alpha.cppm" ||
  fail "emit alpha.dcm"
run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/beta.dcm" "$FIXTURES/beta.cppm" ||
  fail "emit beta.dcm"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  "$FIXTURES/alpha.cppm" -o "$work/alpha.o" ||
  fail "compile alpha object"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  "$FIXTURES/beta.cppm" -o "$work/beta.o" ||
  fail "compile beta object"
run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_alpha_beta.cpp" \
  -o "$work/use_alpha_beta.o" ||
  fail "compile alpha_beta importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_alpha_beta.o" "$work/alpha.o" "$work/beta.o" "$LIBC" \
  -o "$work/alpha_beta.bin" ||
  fail "link alpha_beta executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} "$work/alpha_beta.bin" \
  >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute alpha_beta module program"

run "$DAVECC" -target "$TARGET" -std=c++20 \
  -Xemit-module "$work/export_conflict.dcm" \
  "$FIXTURES/export_conflict.cppm" ||
  fail "emit export_conflict.dcm"

run "$DAVECC" -target "$TARGET" -std=c++20 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_export_conflict.cpp" \
  -o "$work/use_export_conflict.o" ||
  fail "compile export_conflict importer"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/deleted_reason.dcm" "$FIXTURES/deleted_reason.cppm" ||
  fail "emit deleted function reason module"
if run "$DAVECC" -target "$TARGET" -std=c++26 -c \
     -fprebuilt-module-path "$work" "$FIXTURES/use_deleted_reason.cpp" \
     -o "$work/use_deleted_reason.o"; then
  fail "imported deleted function call succeeded"
fi
grep -Fq "use available() instead" "$work/command.log" ||
  fail "imported deleted function reason diagnostic"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/static_assert_message.dcm" \
  "$FIXTURES/static_assert_message.cppm" ||
  fail "emit generated static_assert message module"
if run "$DAVECC" -target "$TARGET" -std=c++26 -c \
     -fprebuilt-module-path "$work" "$FIXTURES/use_static_assert_message.cpp" \
     -o "$work/use_static_assert_message.o"; then
  fail "imported failing static_assert succeeded"
fi
grep -Fq "module-generated assertion" "$work/command.log" ||
  fail "imported generated static_assert message diagnostic"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/placeholder_variables.dcm" \
  "$FIXTURES/placeholder_variables.cppm" ||
  fail "emit placeholder variables module"
if run "$DAVECC" -target "$TARGET" -std=c++26 -c \
     -fprebuilt-module-path "$work" "$FIXTURES/use_placeholder_variables.cpp" \
     -o "$work/use_placeholder_variables.o"; then
  fail "ambiguous imported placeholder member use succeeded"
fi
grep -Fq "name-independent declaration '_' is ambiguous" "$work/command.log" ||
  fail "imported placeholder member ambiguity diagnostic"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/expansion_statements.dcm" \
  "$FIXTURES/expansion_statements.cppm" ||
  fail "emit expansion statements module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_expansion_statements.cpp" \
  -o "$work/use_expansion_statements.o" ||
  fail "instantiate imported expansion statement"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_expansion_statements.o" "$LIBC" \
  -o "$work/expansion_statements.bin" ||
  fail "link imported expansion statement executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/expansion_statements.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute imported expansion statement"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/contracts.dcm" \
  "$FIXTURES/contracts.cppm" ||
  fail "emit contracts module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_contracts.cpp" \
  -o "$work/use_contracts.o" ||
  fail "instantiate imported contracts"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_contracts.o" "$LIBC" \
  -o "$work/contracts.bin" ||
  fail "link imported contracts executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/contracts.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute imported contracts"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/constexpr_exceptions.dcm" \
  "$FIXTURES/constexpr_exceptions.cppm" ||
  fail "emit constexpr exceptions module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_constexpr_exceptions.cpp" \
  -o "$work/use_constexpr_exceptions.o" ||
  fail "evaluate imported constexpr exceptions"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_constexpr_exceptions.o" "$LIBC" \
  -o "$work/constexpr_exceptions.bin" ||
  fail "link imported constexpr exceptions executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/constexpr_exceptions.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute imported constexpr exceptions"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/reflection.dcm" \
  "$FIXTURES/reflection.cppm" ||
  fail "emit reflection module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_reflection.cpp" \
  -o "$work/use_reflection.o" ||
  fail "instantiate imported reflection values"

run "$DAVECC" -target "$TARGET" -std=c++26 \
  -Xemit-module "$work/meta_synthesis.dcm" \
  "$FIXTURES/meta_synthesis.cppm" ||
  fail "emit meta_synthesis module"
run "$DAVECC" -target "$TARGET" -std=c++26 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_meta_synthesis.cpp" \
  -o "$work/use_meta_synthesis.o" ||
  fail "instantiate imported meta_synthesis values"

run "$DAVECC" -target "$TARGET" -std=c++29 \
  -Xemit-module "$work/token_injection.dcm" \
  "$FIXTURES/token_injection.cppm" ||
  fail "emit token injection module"
run "$DAVECC" -target "$TARGET" -std=c++29 -c \
  "$FIXTURES/token_injection.cppm" -o "$work/token_injection.o" ||
  fail "compile token injection module object"
run "$DAVECC" -target "$TARGET" -std=c++29 -c \
  -fprebuilt-module-path "$work" "$FIXTURES/use_token_injection.cpp" \
  -o "$work/use_token_injection.o" ||
  fail "compile token injection importer"
run "$DAVECC" -target "$TARGET" -static \
  ${COMPILE_ARGS[@]+"${COMPILE_ARGS[@]}"} \
  "$work/use_token_injection.o" "$work/token_injection.o" "$LIBC" \
  -o "$work/token_injection.bin" ||
  fail "link token injection module executable"
"$INTERPRETER" ${INTERP_ARGS[@]+"${INTERP_ARGS[@]}"} \
  "$work/token_injection.bin" >"$work/command.log" 2>&1
[ "$?" -eq 0 ] || fail "execute token injection module program"

echo "ok module emit/import/link/execute"
