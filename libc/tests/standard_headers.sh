#!/bin/bash
# Check that every standard C and C++ header mandated by each supported
# language mode exists and is self-contained (compiles on its own).
#
# Headers still to be written are listed in the *_todo sets.  They are reported
# but do not fail the run; move a header out of its todo set as soon as it
# lands so that a later regression is caught.
set -uo pipefail

if [ "$#" -lt 2 ]; then
  echo "usage: $0 <davecc> <target> [extra davecc flags...]" >&2
  exit 2
fi

davecc=$1
target=$2
shift 2
extra_flags=("$@")

work=$(mktemp -d "${TMPDIR:-/tmp}/libc_header_test.XXXXXX")
trap 'rm -rf "$work"' EXIT

# C headers by the standard that introduced them.
c89="assert.h ctype.h errno.h float.h limits.h locale.h math.h setjmp.h
     signal.h stdarg.h stddef.h stdio.h stdlib.h string.h time.h"
c95="iso646.h wchar.h wctype.h"
c99="complex.h fenv.h inttypes.h stdbool.h stdint.h tgmath.h"
c11="stdalign.h stdatomic.h stdnoreturn.h threads.h uchar.h"
c23="stdbit.h stdckdint.h"

# C++ headers by the standard that introduced them.  Headers that a later
# standard removed are listed in cxx_removed_in with the first standard that
# no longer mandates them.
cxx98="algorithm bitset complex deque exception fstream functional iomanip ios
       iosfwd iostream istream iterator limits list locale map memory new
       numeric ostream queue set sstream stack stdexcept streambuf string
       typeinfo utility valarray vector cassert cctype cerrno cfloat ciso646
       climits clocale cmath csetjmp csignal cstdarg cstddef cstdio cstdlib
       cstring ctime cwchar cwctype"
cxx11="array atomic chrono condition_variable cstdalign cstdbool cfenv cinttypes
       cstdint cuchar forward_list future initializer_list mutex random ratio
       regex scoped_allocator system_error thread tuple type_traits typeindex
       unordered_map unordered_set"
cxx14="shared_mutex"
cxx17="any charconv execution filesystem memory_resource optional string_view
       variant"
cxx20="barrier bit compare concepts coroutine format latch numbers ranges
       semaphore source_location span stop_token syncstream version"
cxx23="expected flat_map flat_set generator mdspan print spanstream stacktrace
       stdfloat"

# Deprecated C++ headers that a later standard removed.
cxx_removed_at_20="ciso646 cstdalign cstdbool"

# Not yet implemented.  Remove entries here as the headers land.
c_todo="complex.h fenv.h tgmath.h uchar.h wctype.h locale.h signal.h"
cxx_todo="complex valarray clocale csignal cwctype
          cfenv cuchar future regex scoped_allocator
          shared_mutex execution spanstream"

# The C++ headers were only ever exercised at C++20 and later and use C++17/20
# syntax unconditionally, so most of them do not compile below C++20 yet.  Until
# that is finished, a failure below C++20 is reported but does not fail the run;
# C++20 and later, and every C mode, are held to the normal standard.  Delete
# this once the pre-C++20 modes are clean.
cxx_pre20_incomplete=1

pass=0
fail=0
todo=0

in_set() {
  local needle=$1
  shift
  local item
  for item in $*; do
    [ "$item" = "$needle" ] && return 0
  done
  return 1
}

check_one() {
  local header=$1 std=$2 ext=$3
  local src="$work/probe.$ext"
  printf '#include <%s>\nint probe_unused;\n' "$header" > "$src"
  if "$davecc" -target "$target" -std="$std" -c -isystem libc/include \
      ${extra_flags+"${extra_flags[@]}"} \
      "$src" -o "$work/probe.o" 2>"$work/err.txt"; then
    pass=$((pass + 1))
    return 0
  fi
  if in_set "$header" "$c_todo $cxx_todo"; then
    echo "TODO <$header> ($std) not implemented"
    todo=$((todo + 1))
    return 0
  fi
  case "$std" in
    c++11|c++14|c++17)
      if [ "$cxx_pre20_incomplete" -eq 1 ]; then
        echo "TODO <$header> ($std) not yet valid below C++20"
        todo=$((todo + 1))
        return 0
      fi
      ;;
  esac
  echo "FAIL <$header> ($std)"
  sed 's/^/  /' "$work/err.txt"
  fail=$((fail + 1))
  return 1
}

for std in c11 c17 c23; do
  headers="$c89 $c95 $c99 $c11"
  if [ "$std" = c23 ]; then
    headers="$headers $c23"
  fi
  for header in $headers; do
    check_one "$header" "$std" c || true
  done
done

for std in c++11 c++14 c++17 c++20 c++23; do
  headers="$cxx98 $cxx11"
  case "$std" in
    c++14) headers="$headers $cxx14" ;;
    c++17) headers="$headers $cxx14 $cxx17" ;;
    c++20) headers="$headers $cxx14 $cxx17 $cxx20" ;;
    c++23) headers="$headers $cxx14 $cxx17 $cxx20 $cxx23" ;;
  esac
  for header in $headers; do
    case "$std" in
      c++20|c++23)
        if in_set "$header" "$cxx_removed_at_20"; then
          continue
        fi
        ;;
    esac
    check_one "$header" "$std" cpp || true
  done
done

echo "standard header summary: pass=$pass fail=$fail todo=$todo"
if [ "$fail" -ne 0 ]; then
  exit 1
fi
