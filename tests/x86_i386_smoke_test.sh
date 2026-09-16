#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 davecc [workdir]" >&2
  exit 2
fi

if [[ "$1" = /* ]]; then
  DAVECC="$1"
else
  ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
  DAVECC="$ROOT/$1"
fi
WORK="${2:-${TEST_TMPDIR:-/tmp}/x86-i386-smoke}"
mkdir -p "$WORK"

FORBIDDEN='(%r[0-9]+|%r1[0-9]|%r1[0-5]d|movq |pushq |popq |cmplq|cmpq |addq |subq |movzbq|movsbq|bsrq|bsfq|rolq|rorq|cqo|cltq|movl_xmm|%rip)'

check_no_forbidden() {
  local f="$1"
  if rg -n "$FORBIDDEN" "$f" >/dev/null 2>&1; then
    echo "forbidden x86-64 pattern in $f:" >&2
    rg -n "$FORBIDDEN" "$f" >&2 || true
    exit 1
  fi
}

# Verify conventional i386 frame: args at 8(%ebp), 12(%ebp), ...
check_frame_semantics() {
  local asm="$1"
  grep -q 'pushl %ebp' "$asm"
  grep -q 'movl %esp, %ebp' "$asm"
  grep -q '8(%ebp)' "$asm"
}

assemble_and_check() {
  local src="$1" obj="$2" asm="$3"
  "$DAVECC" -target x86 -nostdinc -c -o "$obj" "$src"
  "$DAVECC" -target x86 -nostdinc -S -o "$asm" "$src"
  check_no_forbidden "$asm"
  file "$obj" | grep -q 'ELF 32-bit.*Intel 80386'
  if command -v readelf >/dev/null 2>&1; then
    readelf -r "$obj" 2>/dev/null | grep -Eq 'R_386_32|R_386_PC32|There are no relocations|\.rel\.' || {
      echo "expected R_386 relocations in $obj" >&2
      readelf -r "$obj" >&2 || true
      exit 1
    }
  fi
}

cat >"$WORK/add.c" <<'EOF'
int add(int a, int b) { return a + b; }
int use(void) { return add(3, 4); }
EOF

cat >"$WORK/call5.c" <<'EOF'
int callee(int a, int b, int c, int d, int e) {
  return a + b + c + d + e;
}
int caller(void) { return callee(1, 2, 3, 4, 5); }
EOF

cat >"$WORK/divmod.c" <<'EOF'
int sdiv(int a, int b) { return a / b; }
int udiv(unsigned a, unsigned b) { return a / b; }
int smod(int a, int b) { return a % b; }
EOF

cat >"$WORK/compare.c" <<'EOF'
int lt(int a, int b) { return a < b; }
int eq(int a, int b) { return a == b; }
int neq(int a, int b) { return a != b; }
EOF

cat >"$WORK/global.c" <<'EOF'
extern int g;
int read_g(void) { return g; }
int write_g(int v) { g = v; return g; }
EOF

cat >"$WORK/float.c" <<'EOF'
float fadd(float a, float b) { return a + b; }
double dadd(double a, double b) { return a + b; }
float callf(float x) { return fadd(x, 1.0f); }
EOF

cat >"$WORK/varargs.c" <<'EOF'
int sum(int n, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, n);
  int total = 0;
  for (int i = 0; i < n; i++) {
    total += __builtin_va_arg(ap, int);
  }
  __builtin_va_end(ap);
  return total;
}
int call_sum(void) { return sum(3, 10, 20, 30); }
EOF

cat >"$WORK/struct4.c" <<'EOF'
struct S4 { int x; };
struct S4 mk(int v) { struct S4 s; s.x = v; return s; }
int use4(struct S4 s) { return s.x; }
EOF

cat >"$WORK/structs.c" <<'EOF'
struct S1 { char c; };
struct S5 { char c; int x; };
struct S8 { long long v; };
struct S12 { int a, b, c; };
int size1(void) { return (int)sizeof(struct S1); }
int size5(void) { return (int)sizeof(struct S5); }
int size8(void) { return (int)sizeof(struct S8); }
int size12(void) { return (int)sizeof(struct S12); }
EOF

cat >"$WORK/indirect.c" <<'EOF'
typedef int (*binop_t)(int, int);
int add(int a, int b) { return a + b; }
int apply(binop_t f, int a, int b) { return f(a, b); }
int use_indirect(void) { return apply(add, 3, 4); }
EOF

cat >"$WORK/spill.c" <<'EOF'
int pressure(int a, int b, int c, int d, int e, int f, int g, int h) {
  volatile int x1 = a, x2 = b, x3 = c, x4 = d, x5 = e, x6 = f, x7 = g, x8 = h;
  return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8;
}
EOF

cat >"$WORK/forward.c" <<'EOF'
int callee(int x) { return x + 1; }
int forward(int x) { return callee(x); }
EOF

assemble_and_check "$WORK/add.c" "$WORK/add.o" "$WORK/add.s"
assemble_and_check "$WORK/call5.c" "$WORK/call5.o" "$WORK/call5.s"
assemble_and_check "$WORK/divmod.c" "$WORK/divmod.o" "$WORK/divmod.s"
assemble_and_check "$WORK/compare.c" "$WORK/compare.o" "$WORK/compare.s"
assemble_and_check "$WORK/global.c" "$WORK/global.o" "$WORK/global.s"
assemble_and_check "$WORK/float.c" "$WORK/float.o" "$WORK/float.s"
assemble_and_check "$WORK/varargs.c" "$WORK/varargs.o" "$WORK/varargs.s"
assemble_and_check "$WORK/struct4.c" "$WORK/struct4.o" "$WORK/struct4.s"
assemble_and_check "$WORK/structs.c" "$WORK/structs.o" "$WORK/structs.s"
assemble_and_check "$WORK/indirect.c" "$WORK/indirect.o" "$WORK/indirect.s"
assemble_and_check "$WORK/spill.c" "$WORK/spill.o" "$WORK/spill.s"
assemble_and_check "$WORK/forward.c" "$WORK/forward.o" "$WORK/forward.s"

check_frame_semantics "$WORK/add.s"
check_frame_semantics "$WORK/call5.s"
grep -Eq '8\(%ebp\)|12\(%ebp\)|16\(%ebp\)|20\(%ebp\)|24\(%ebp\)' "$WORK/call5.s"
grep -q 'idivl\|divl' "$WORK/divmod.s"
grep -q 'set' "$WORK/compare.s"
grep -Eq 'addl \$c, %eax|8\(%ebp\).*12\(%ebp\)' "$WORK/varargs.s" || grep -q 'addl \$12, %eax' "$WORK/varargs.s"
grep -Eq 'addl \$4, %eax|8\(%ebp\)' "$WORK/struct4.s"
grep -q 'ret \$4' "$WORK/struct4.s"
grep -q 'call \*' "$WORK/indirect.s" || grep -q 'ff d0' "$WORK/indirect.o" 2>/dev/null || objdump -d "$WORK/indirect.o" | grep -q 'ff d0'
grep -q 'pushl %ebp' "$WORK/spill.s"
# Non-leaf forwarding with no locals still allocates an 8-byte frame body.
grep -Eq 'subl \$8, %esp|addl \$-8, %esp' "$WORK/forward.s"

# Scalar long long: compile and emit 32-bit pair arithmetic (adcl/sbbl).
cat >"$WORK/wide_int.c" <<'EOF'
long long wide_add(long long a, long long b) { return a + b; }
long long wide_sub(long long a, long long b) { return a - b; }
EOF
assemble_and_check "$WORK/wide_int.c" "$WORK/wide_int.o" "$WORK/wide_int.s"
grep -q 'adcl' "$WORK/wide_int.s"
grep -q 'sbbl' "$WORK/wide_int.s"

# x86_64 regression: compile-only smoke
X64_WORK="${TEST_TMPDIR:-/tmp}/x86_64-regression-smoke"
mkdir -p "$X64_WORK"
cat >"$X64_WORK/t.c" <<'EOF'
int f(int a) { return a + 1; }
EOF
"$DAVECC" -target x86_64 -nostdinc -c -o "$X64_WORK/t.o" "$X64_WORK/t.c"

# Driver path for standalone i386 assembly sources.
cat >"$WORK/input.s" <<'EOF'
	.text
	.global asm_identity
asm_identity:
	movl 4(%esp), %eax
	ret
EOF
"$DAVECC" -target x86 -nostdinc -c -o "$WORK/input.o" "$WORK/input.s"
file "$WORK/input.o" | grep -q 'ELF 32-bit.*Intel 80386'

echo "x86 i386 smoke test passed"
