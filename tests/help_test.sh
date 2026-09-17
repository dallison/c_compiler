#!/bin/bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "usage: $0 <davecc>" >&2
  exit 2
fi

davecc=$1
work="${TEST_TMPDIR:-$(mktemp -d "${TMPDIR:-/tmp}/davecc-help.XXXXXX")}"
mkdir -p "$work"
help="$work/help.txt"

"$davecc" --help >"$help"

fail() {
  echo "$1" >&2
  echo "---- help ----" >&2
  cat "$help" >&2
  exit 1
}

grep -q "^Usage: davecc \[options\] file\.\.\.$" "$help" ||
  fail "missing usage line"

# Every group with options in it must be printed as a heading.
for heading in "Overall options" "Language and standards" \
    "Preprocessor and include paths" "Diagnostics" \
    "Code generation and optimization" "Linking" "C++20 modules" \
    "Source listings" "Compiler developer options"; do
  grep -qF "$heading:" "$help" || fail "missing section '$heading'"
done

# Both compiler options and the driver's own options are documented.
for option in "-c" "-o <file>" "-std <standard>" "-I<dir>" "-W<warning>" \
    "-O<level>" "-flisting-file <file>" "-Xsave-ir" "-l<name>" "-Wl,<arg>" \
    "--gc-sections"; do
  grep -qF "  $option" "$help" || fail "missing option '$option'"
done

grep -q "^Targets for -target:$" "$help" || fail "missing target list"
grep -q "x86_64 (x86-64)" "$help" || fail "missing x86_64 target and aliases"

# Hidden options stay out of the help.
if grep -q -e "-Xemit-module" -e "-Xload-module" -e "(hidden)" "$help"; then
  fail "hidden options should not be listed"
fi

# Nothing runs past the 80-column right margin except an unbreakable word.
if awk 'length($0) > 80 && NF > 1 { print NR": "$0; found = 1 }
        END { exit !found }' "$help"; then
  fail "help text is wider than 80 columns"
fi

# The spellings all produce the same help.
for flag in -h -help; do
  "$davecc" "$flag" >"$work/alt.txt"
  cmp -s "$help" "$work/alt.txt" || fail "$flag differs from --help"
done

echo "help tests passed"
