#! /usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 --davecc PATH" >&2
  exit 2
}

DAVECC=
while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    *) usage ;;
  esac
done

if [ -z "$DAVECC" ]; then
  usage
fi

work=$(mktemp -d "${TMPDIR:-/tmp}/davecc-warnings.XXXXXX")
trap 'rm -rf "$work"' EXIT

run_ok() {
  local name=$1
  local source=$2
  shift 2
  : >"$work/$name.out"
  : >"$work/$name.err"
  for opt in -O0 -O1; do
    "$DAVECC" -target x86_64 "$opt" -S "$@" "$source" -o "$work/$name${opt}.s" \
      >>"$work/$name.out" 2>>"$work/$name.err"
  done
}

run_ok_o0() {
  local name=$1
  local source=$2
  shift 2
  : >"$work/$name.out"
  : >"$work/$name.err"
  "$DAVECC" -target x86_64 -O0 -S "$@" "$source" -o "$work/$name-O0.s" \
    >>"$work/$name.out" 2>>"$work/$name.err"
}

expect_warn() {
  local name=$1
  local pattern=$2
  if ! grep -E "$pattern" "$work/$name.err" >/dev/null; then
    echo "missing warning pattern for $name: $pattern" >&2
    echo "--- stderr ---" >&2
    sed 's/^/  /' "$work/$name.err" >&2
    exit 1
  fi
}

expect_no_warn() {
  local name=$1
  local pattern=$2
  if grep -E "$pattern" "$work/$name.err" >/dev/null; then
    echo "unexpected warning pattern for $name: $pattern" >&2
    echo "--- stderr ---" >&2
    sed 's/^/  /' "$work/$name.err" >&2
    exit 1
  fi
}

cat >"$work/unknown.c" <<'EOF'
int main(void) { return 0; }
EOF
run_ok unknown "$work/unknown.c" -Wdefinitely-not-a-warning
expect_warn unknown 'unknown-warning-option'
run_ok unknown_no "$work/unknown.c" -Wno-definitely-not-a-warning
expect_no_warn unknown_no 'unknown-warning-option'

cat >"$work/frontend.c" <<'EOF'
static int helper(void) { return 1; }
int main(void) {
  1 + 2;
label:
  (void)0;
  int later = 1;
  return later;
}
EOF
cat >"$work/oldstyle.c" <<'EOF'
int old(a)
int a;
{
  return 0;
}
int main(void) { return old(1); }
EOF
cat >"$work/implicit.c" <<'EOF'
extern implicit_decl;
int main(void) { return 0; }
EOF
run_ok implicit "$work/implicit.c" -Wall -Wpedantic
expect_warn implicit 'implicit-int'
run_ok oldstyle "$work/oldstyle.c" -Wextra
expect_warn oldstyle 'old-style-definition'
cat >"$work/generated.c" <<'EOF'
int unnamed_param(int) { return 0; }
typedef __builtin_va_list va_list;
int use_builtin(int n, ...) {
  va_list ap;
  __builtin_va_start(ap, n);
  __builtin_va_arg(ap, int);
  __builtin_va_end(ap);
  return 0;
}
int main(void) { return unnamed_param(1) + use_builtin(1, 2); }
EOF
run_ok generated "$work/generated.c" -Weverything
expect_no_warn generated '__invented'
expect_no_warn generated '__builtin_va'
expect_no_warn generated 'unused-parameter'
expect_no_warn generated 'unused-value'
run_ok default_frontend "$work/frontend.c"
expect_no_warn default_frontend 'unused-function'
expect_no_warn default_frontend 'old-style-definition'
expect_no_warn default_frontend 'unused-label'
expect_no_warn default_frontend 'unused-value'
expect_no_warn default_frontend 'declaration-after-statement'
run_ok frontend "$work/frontend.c" -Wall -Wextra -Wpedantic
expect_warn frontend 'unused-function'
expect_warn frontend 'unused-label'
expect_warn frontend 'unused-value'
expect_warn frontend 'declaration-after-statement'
run_ok_o0 werror_unused_value "$work/frontend.c" -Wunused-value -Werror=unused-value
expect_warn werror_unused_value 'error\[unused-value\]'
run_ok wno_error_unused_value "$work/frontend.c" -Wunused-value -Werror -Wno-error=unused-value
expect_warn wno_error_unused_value 'warning\[unused-value\]'
expect_no_warn wno_error_unused_value 'error\[unused-value\]'

cat >"$work/format.c" <<'EOF'
void logf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void logf(const char *fmt, ...) { (void)fmt; }
int main(void) {
  int x = 1;
  logf("");
  logf("%q", 1);
  logf("%s", 1);
  logf(x ? "%d" : "%s");
  return 0;
}
EOF
run_ok format "$work/format.c" -Wall -Wformat-nonliteral -Wformat-security
expect_warn format 'format-zero-length'
expect_warn format 'format-invalid-specifier'
expect_warn format 'format argument 2 has the wrong type'
expect_warn format 'format-nonliteral'
expect_warn format 'format-security'

cat >"$work/conversion.c" <<'EOF'
enum E { EA, EB };
int main(void) {
  unsigned u = 1;
  int i = -1;
  signed char *p = 0;
  unsigned char *up = 0;
  const signed char *cp = 0;
  if (u < i) {
    return 1;
  }
  p = up;
  p = cp;
  i = p;
  switch ((enum E)EA) {
    case EA: break;
    default: break;
  }
  return 0;
}
EOF
run_ok conversion "$work/conversion.c" -Wall -Wextra
expect_warn conversion 'sign-compare'
expect_warn conversion 'pointer-sign'
expect_warn conversion 'discarded-qualifiers'
expect_warn conversion 'int-conversion'
expect_warn conversion 'switch-enum'
run_ok everything_conversion "$work/conversion.c" -Weverything
expect_warn everything_conversion 'sign-compare'
expect_warn everything_conversion 'pointer-sign'
expect_warn everything_conversion 'discarded-qualifiers'
expect_warn everything_conversion 'int-conversion'
expect_warn everything_conversion 'switch-enum'
expect_warn everything_conversion 'conversion'
run_ok no_everything_conversion "$work/conversion.c" -Weverything -Wno-everything
expect_no_warn no_everything_conversion 'sign-compare'
expect_no_warn no_everything_conversion 'pointer-sign'
expect_no_warn no_everything_conversion 'discarded-qualifiers'
expect_no_warn no_everything_conversion 'int-conversion'
expect_no_warn no_everything_conversion 'switch-enum'

cat >"$work/prelex.c" <<'EOF'
#if MISSING_MACRO
int x = 1;
#endif
#pragma unknown_thing
#pragma davecc diagnostic pop
int main(void) {
  int c = 'ab';
  return c == 0;
}
EOF
run_ok prelex "$work/prelex.c" -Wundef -Wunknown-pragmas -Wmultichar
expect_warn prelex 'undef'
expect_warn prelex 'unknown-pragmas'
expect_warn prelex 'multichar'

echo "warning diagnostics ok"
