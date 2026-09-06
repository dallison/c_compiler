#!/bin/bash
# Compilation has to terminate normally.  Each source below once made the
# compiler spin or crash, in most cases while recovering from a construct it
# rejects.  The cases that rely on a construct davecc does not implement should,
# if it becomes supported, be replaced with another unsupported construct in the
# same position rather than dropped.
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <davecc>" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c-error-recovery.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

# The error limit would otherwise stop a spinning parser and hide the defect, so
# it is disabled here and a spin is caught by the timeout instead.  The compiler
# writes straight to the log: a spin produces output far faster than it can be
# usefully buffered.
run_davecc() {
  local src="$1"
  local log="$2"
  local std="$3"
  python3 -c '
import os, signal, subprocess, sys
davecc, src, log, std = sys.argv[1:5]
with open(log, "wb") as output:
    proc = subprocess.Popen(
        [davecc, "-target", "pcode", "-std=" + std, "-fsyntax-only",
         "-error-limit=0", src],
        stdout=output,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    try:
        raise SystemExit(proc.wait(timeout=10))
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid, signal.SIGKILL)
        proc.wait()
        raise SystemExit(124)
' "$DAVECC" "$src" "$log" "$std"
}

# Compile and require an ordinary exit: no hang, no signal.  A rejected
# construct leaves the compiler holding partly built state, so what follows it
# must still be processed without crashing.
expect_terminates() {
  local name="$1"
  local source="$2"
  local std="${3:-c17}"
  local src="$WORK/$name.c"
  local log="$WORK/$name.out"
  printf '%s\n' "$source" >"$src"
  local status=0
  run_davecc "$src" "$log" "$std" || status=$?
  if [[ "$status" -eq 124 ]]; then
    echo "$name: compiler did not terminate" >&2
    head -5 "$log" | sed 's/^/  /' >&2
    exit 1
  fi
  if [[ "$status" -lt 0 || "$status" -gt 1 ]]; then
    echo "$name: compiler exited abnormally with status $status" >&2
    head -5 "$log" | sed 's/^/  /' >&2
    exit 1
  fi
}

expect_diagnosed() {
  local name="$1"
  local source="$2"
  local pattern="${3:-error:}"
  local std="${4:-c17}"
  expect_terminates "$name" "$source" "$std"
  if ! grep -Fq "$pattern" "$WORK/$name.out"; then
    echo "$name: expected diagnostic not found: $pattern" >&2
    head -5 "$WORK/$name.out" | sed 's/^/  /' >&2
    exit 1
  fi
}

# Complex types are supported, but keep the former non-termination positions
# covered so parser recovery changes cannot reintroduce those loops.
expect_terminates complex_declaration_specifier \
  'double _Complex f(void);
int main(void) { return 0; }'

expect_terminates complex_struct_member \
  'struct S { _Complex float d; };'

expect_diagnosed imaginary_declaration_specifier \
  '_Imaginary double x;' \
  "'_Imaginary' types are not supported"

# The GNU spelling of the same specifier must also terminate.
expect_terminates gnu_complex_declaration_specifier \
  '__complex__ double foo (__complex__ double x, __complex__ double y)
{
  return x / y;
}'

# An old-style argument declaration list that reaches end of input before the
# function body: that loop only stopped at the '{' of the body.
expect_diagnosed old_style_arguments_at_end_of_input \
  'int old(a)
int a;'

# Reading a member of an aggregate constexpr object materializes the subobject
# for that member and stores it in the containing object's slot.  The containing
# object here belongs to the symbol and outlives the evaluation, so a subobject
# owned by the evaluation context left the slot dangling and the second read
# followed it.  Two reads are needed: the first frees the subobject, the second
# reuses the slot.  Whether the initializer is accepted does not matter, only
# that the reads do not crash.
expect_terminates constexpr_subobject_read_twice \
  'struct inner { void *p; };
struct outer { struct inner x; };
constexpr struct outer v = { };
static_assert (v.x.p == 0);
static_assert (v.x.p == 0);' \
  c23

# A file-scope assignment is parsed as an implicit-int declaration, which here
# redeclares an object of a different type.  The initializer is encoded for the
# storage of the object being initialized, and the invalid expression beside it
# has an unrelated type, which used to fail an assertion in the encoder.
expect_diagnosed static_initializer_after_type_mismatch \
  'double res;
res = .;'

# An array whose bound comes from its initializer gets an element for every
# index up to the largest designated one.  These indices ask for more elements
# than the compiler can lay out, and each one used to build elements until it
# ran out of memory or time: the first is truncated into a negative index and
# then used unsigned, the second is negative in the source, and the third is
# representable but still names an array of about 16GB.
expect_diagnosed array_designator_index_above_int \
  'static char *name[] = { [0x80000000] = "bar" };' \
  'out of range'

expect_diagnosed array_designator_index_negative \
  'static char *name[] = { [-2147483648] = "bar" };' \
  'outside bounds'

expect_diagnosed array_designator_bound_unrepresentable \
  'static char *name[] = { [0x7ffffff0] = "bar" };' \
  'larger than this compiler can lay out'

# A designator index can be representable and still name a huge array.  The
# initializer tree used to allocate one node per element up to that index.
# Only the designated slot is built now, so this must finish immediately.
expect_terminates array_designator_sparse_large \
  'static char *name[] = { [0x08000000] = "bar" };'

# The same jump in a range designator must not clone the initializer once per
# index in the range.
expect_diagnosed array_designator_range_too_large \
  'static int a[] = { [0 ... 0x08000000] = 1 };' \
  'larger than this compiler can lay out' \
  gnu17

# Positional initializers after a designated hole still target the next index
# (a[5] then a[6]), without filling the hole with INodes.
expect_terminates array_designator_sparse_then_positional \
  'int a[] = { [5] = 1, 2 };
int recovery_anchor(void) { return a[6]; }'

# An operand that is itself an expression is parsed by recursive descent, and
# the parser cannot tell how much stack is left, so beyond a fixed nesting depth
# the input is rejected.  These nest far past that depth and used to exhaust the
# stack.  The second one never closes its parentheses, so the depth is reached
# while the parser is already recovering.
expect_diagnosed expression_paren_nesting_limit \
  "$(python3 -c 'print("int x = " + "(" * 10000 + "1" + ")" * 10000 + ";")')" \
  'nests more than'

expect_terminates expression_paren_nesting_limit_unclosed \
  "$(python3 -c 'print("int x = " + "(" * 10000 + "1;")')"

# A member declaration can define another struct, and each definition's member
# list costs stack for the same reason, so the depth is capped there too.  The
# second one closes none of its definitions, so the cap is reached with every
# enclosing member list still open.
expect_diagnosed struct_definition_nesting_limit \
  "$(python3 -c '
n = 10000
print("".join("struct s%d {" % i for i in range(n)) + " int x;" + "} x;" * n)')" \
  'nest more than'

expect_terminates struct_definition_nesting_limit_unclosed \
  "$(python3 -c '
print("".join("struct s%d {" % i for i in range(10000)) + " int x;")')"

# Unclosed nested structs at end of input used to report "Missing }" and
# "Expected semicolon" once per level.  After the first of each, further
# copies at EOF are dropped.
expect_terminates unclosed_nested_structs_few_diagnostics \
  "$(python3 -c '
print("".join("struct s%d {" % i for i in range(20)) + " int x;")')"
nerr=$(grep -c '^error:' "$WORK/unclosed_nested_structs_few_diagnostics.out" || true)
if [[ "$nerr" -gt 4 ]]; then
  echo "unclosed_nested_structs_few_diagnostics: $nerr diagnostics, expected at most 4" >&2
  head -8 "$WORK/unclosed_nested_structs_few_diagnostics.out" | sed 's/^/  /' >&2
  exit 1
fi

# A comment that no line closes.  Looking for the close read past the end of
# the input, where waiting for the lexer to report end of file could not
# succeed: the lexer had not yet consumed the line being tokenized.
expect_terminates unterminated_comment \
  'int main(void) { return 0; }
/* the input ends inside this comment'

# `else` is a statement keyword, not a type.  Classifying it as a type made
# "Type expected" recovery stop on the same token forever inside a member list.
expect_diagnosed else_in_struct_member_list \
  'struct S { else };
int recovery_anchor(void) { return 42; }' \
  'Type expected'

# The same token in a namespace body, and a punctuator that does not start a
# declaration, must not spin in the namespace-declaration loop.
expect_diagnosed else_in_namespace_body \
  'namespace { else }
int recovery_anchor() { return 42; }' \
  'Expected semicolon' \
  c++17

expect_terminates punctuator_in_namespace_body \
  'namespace { int x; << else }
int recovery_anchor() { return 42; }' \
  c++17

# A missing semicolon after a junk initializer must not skip the function's
# closing brace and treat the next function as nested (which used to report
# "Function definition not allowed here" and then miss the closer).
expect_diagnosed local_decl_does_not_skip_function_close \
  'int f(void) { int x = @ }
int recovery_anchor(void) { return 42; }' \
  'primary expression expected'

if grep -Fq 'Function definition not allowed' "$WORK/local_decl_does_not_skip_function_close.out"; then
  echo "local_decl_does_not_skip_function_close: skipped '}' and nested the next function" >&2
  exit 1
fi

expect_diagnosed paren_after_struct_tag \
  'struct(S {});
int recovery_anchor() { return 42; }' \
  'Missing close parenthesis' \
  c++17

expect_diagnosed noexcept_without_paren_in_array \
  'unsigned [ noexcept x;
int recovery_anchor() { return 42; }' \
  'Expected ( after noexcept' \
  c++17

expect_terminates return_in_array_bound_statement_expr \
  'int old(a) int([({{ a; { return a; }'

expect_diagnosed old_style_extra_close_paren \
  'int old(a) ) int a; { return 0; }' \
  'Expected semicolon'

expect_terminates identifier_list_without_comma \
  'void g(vo id) { }
int recovery_anchor(void) { return 42; }'

expect_diagnosed unknown_type_as_parameter \
  'int f(inat x) { return 0; }
int recovery_anchor(void) { return 42; }'

# `class;` with no name and no body is not an anonymous aggregate.  Injecting
# its members used to dereference a null type.
expect_diagnosed anonymous_class_without_body \
  'struct S { class; };
int recovery_anchor() { return 42; }' \
  'must have a definition' \
  c++17

expect_terminates friend_recovery_then_class_semicolon \
  'struct S { te]pl-te<class T> friend  &&^= class; };
int recovery_anchor() { return 42; }' \
  c++20

expect_terminates friend_recovery_template_soup \
  'struct S { templ&ate<cl typeid ass T> friendend class; },;
int recovery_anchor() { return 42; }' \
  c++26

echo "c error recovery ok"
