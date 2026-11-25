#!/bin/sh
# Regression: requires-expression pristine parameter patterns must not survive
# across CompileTranslationUnit.  Before ConceptsReleasePristineParameterPatterns,
# a later TU that recycled a Symbol* address could TypeRecordCopy a poisoned
# TypeRecord (0xa5 fill) and crash in TypeDependentInfoShare.
set -eu

root="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
davecc="$root/$1"
work="$(mktemp -d "${TMPDIR:-/tmp}/concepts-pristine-cross-tu.XXXXXX")"
trap 'rm -rf "$work"' EXIT

cat >"$work/a.cpp" <<'EOF'
template <typename T>
concept Addable = requires(T x) { x + x; };

template <typename T>
  requires Addable<T>
int fa(T t) {
  return static_cast<int>(t + t);
}

int use_a() { return fa(1); }
EOF

cat >"$work/b.cpp" <<'EOF'
template <typename T>
concept Mulable = requires(T x) { x * x; };

template <typename T>
  requires Mulable<T>
int fb(T t) {
  return static_cast<int>(t * t);
}

int use_b() { return fb(2); }
EOF

cat >"$work/main.cpp" <<'EOF'
int use_a();
int use_b();
int main() { return use_a() + use_b(); }
EOF

# Multi-TU compile in one driver invocation — each .cpp is its own
# CompileTranslationUnit.  -fsyntax-only avoids the linker; we only need the
# frontend not to UAF.  Repeat: the bug was intermittent (Symbol* reuse).
i=0
while [ "$i" -lt 8 ]; do
  i=$((i + 1))
  "$davecc" -std=c++20 -O0 -nostdinc -fsyntax-only \
    "$work/a.cpp" "$work/b.cpp" "$work/main.cpp"
done
