#!/usr/bin/env bash
# Execute i386 ELF32 test programs under linux/386 via Docker.
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 davecc" >&2
  exit 2
fi

if [[ "$1" = /* ]]; then
  DAVECC="$1"
else
  ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
  DAVECC="$ROOT/$1"
fi

# Colima/Docker on macOS often does not bind-mount host /tmp; use workspace path.
ROOT_DIR="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-.}"
WORK="${ROOT_DIR}/.x86_i386_exec_work"
rm -rf "$WORK"
mkdir -p "$WORK"
trap 'rm -rf "$WORK"' EXIT

if ! command -v docker >/dev/null 2>&1; then
  echo "docker required for i386 execution tests" >&2
  exit 1
fi

cat >"$WORK/_start.s" <<'EOF'
	.text
	.global _start
_start:
	call test_main
	movl %eax, %ebx
	movl $1, %eax
	int $0x80
EOF

cat >"$WORK/regcheck.s" <<'EOF'
	.text
	.global verify_saved_regs
verify_saved_regs:
	movl $0x01020304, %ebx
	movl $0x05060708, %esi
	movl $0x090A0B0C, %edi
	call touch_regs
	cmpl $0x01020304, %ebx
	jne fail
	cmpl $0x05060708, %esi
	jne fail
	cmpl $0x090A0B0C, %edi
	jne fail
	xorl %eax, %eax
	ret
fail:
	movl $1, %eax
	ret
EOF

cat >"$WORK/tests.c" <<'EOF'
int add(int a, int b) { return a + b; }

int callee5(int a, int b, int c, int d, int e) {
  return a + b + c + d + e;
}
int caller5(void) { return callee5(1, 2, 3, 4, 5); }

int inner(int x) { return x + 10; }
int outer(int x) { return inner(x) + inner(x + 1); }

int sdiv(int a, int b) { return a / b; }
int smod(int a, int b) { return a % b; }
int lt(int a, int b) { return a < b; }
int eq(int a, int b) { return a == b; }

struct S1 { char c; };
struct S4 { int x; };
struct S5 { char c; int x; };
struct S8 { long long v; };
struct S12 { int a, b, c; };

int size1(void) { return (int)sizeof(struct S1); }
int size4(void) { return (int)sizeof(struct S4); }
int size5(void) { return (int)sizeof(struct S5); }
int size8(void) { return (int)sizeof(struct S8); }
int size12(void) { return (int)sizeof(struct S12); }

int use4(struct S4 s) { return s.x; }
struct S4 mk4(int v) { struct S4 s; s.x = v; return s; }
int read4(void) { return use4(mk4(42)); }

struct S4 ret4(void) {
  struct S4 s;
  s.x = 10;
  return s;
}
int check_ret4(void) {
  struct S4 s = ret4();
  return s.x;
}

typedef int (*binop_t)(int, int);
int apply(binop_t f, int a, int b) { return f(a, b); }

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
int call_variadic(void) { return sum(3, 10, 20, 30); }

/* Force use of callee-saved EBX/ESI/EDI; regcheck.s verifies they survive. */
int touch_regs(void) {
  volatile int a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, g = 7, h = 8;
  volatile int i = 9, j = 10, k = 11, l = 12, m = 13, n = 14, o = 15, p = 16;
  return a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p;
}

int spill_pressure(int a, int b, int c, int d, int e, int f, int g, int h) {
  volatile int x1 = a, x2 = b, x3 = c, x4 = d, x5 = e, x6 = f, x7 = g, x8 = h;
  return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8;
}

extern int verify_saved_regs(void);

static long long g_ll = 0x123456789ABCDEF0LL;

long long ll_add(long long a, long long b) { return a + b; }
long long ll_sub(long long a, long long b) { return a - b; }
long long ll_mul(long long a, long long b) { return a * b; }
long long ll_sdiv(long long a, long long b) { return a / b; }
long long ll_smod(long long a, long long b) { return a % b; }
unsigned long long ll_udiv(unsigned long long a, unsigned long long b) {
  return a / b;
}
unsigned long long ll_umod(unsigned long long a, unsigned long long b) {
  return a % b;
}
long long ll_neg(long long x) { return -x; }
long long ll_not(long long x) { return ~x; }
long long ll_and(long long a, long long b) { return a & b; }
long long ll_or(long long a, long long b) { return a | b; }
long long ll_xor(long long a, long long b) { return a ^ b; }
long long ll_shl(long long a, int n) { return a << n; }
long long ll_shr(long long a, int n) { return (unsigned long long)a >> n; }
long long ll_sar(long long a, int n) { return a >> n; }
int ll_eq(long long a, long long b) { return a == b; }
int ll_ne(long long a, long long b) { return a != b; }
int ll_lt(long long a, long long b) { return a < b; }
int ll_gt(long long a, long long b) { return a > b; }
int ll_le(long long a, long long b) { return a <= b; }
int ll_ge(long long a, long long b) { return a >= b; }
int ll_ltu(unsigned long long a, unsigned long long b) { return a < b; }
int ll_gtu(unsigned long long a, unsigned long long b) { return a > b; }

long long ll_identity(long long x) { return x; }
long long ll_nested(long long x) {
  return ll_identity(x) + ll_identity(x + 1);
}

long long ll_pick(int cond, long long a, long long b) {
  return cond ? a : b;
}

long long ll_sum(int n, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, n);
  long long total = 0;
  for (int i = 0; i < n; i++) {
    total += __builtin_va_arg(ap, long long);
  }
  __builtin_va_end(ap);
  return total;
}

long long ll_spill(long long a, long long b, long long c, long long d,
                   long long e, long long f, long long g, long long h) {
  volatile long long x1 = a, x2 = b, x3 = c, x4 = d;
  volatile long long x5 = e, x6 = f, x7 = g, x8 = h;
  return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8;
}

long long read_s8(struct S8 s) { return s.v; }
struct S8 mk_s8(long long v) { struct S8 s; s.v = v; return s; }

int test_ll(void) {
  long long local = 0x0F0E0D0C0B0A0908LL;
  long long arr[2];
  arr[0] = 100;
  arr[1] = 200;
  if (g_ll != 0x123456789ABCDEF0LL) return 20;
  if (local != 0x0F0E0D0C0B0A0908LL) return 21;
  if (ll_add(1LL, 2LL) != 3LL) return 22;
  if (ll_add(0xFFFFFFFFLL, 1LL) != 0x100000000LL) return 23;
  if (ll_sub(0LL, 1LL) != -1LL) return 24;
  if (ll_sub(0x100000000LL, 1LL) != 0xFFFFFFFFLL) return 25;
  if (ll_mul(100000LL, 100000LL) != 10000000000LL) return 26;
  if (ll_sdiv(-7LL, 3LL) != -2LL) return 27;
  if (ll_smod(-7LL, 3LL) != -1LL) return 28;
  if (ll_udiv(7ULL, 3ULL) != 2ULL) return 29;
  if (ll_umod(7ULL, 3ULL) != 1ULL) return 30;
  if (ll_neg(5LL) != -5LL) return 31;
  if (ll_not(0LL) != -1LL) return 32;
  if (ll_and(0xFF00FF00FF00FF00LL, 0x0F0F0F0F0F0F0F0FLL) != 0x0F000F000F000F00LL) return 33;
  if (ll_or(0xF0F0F0F0F0F0F0F0LL, 0x0F0F0F0F0F0F0F0FLL) != -1LL) return 34;
  if (ll_xor(0xAAAAAAAAAAAAAAAAULL, 0xFFFFFFFFFFFFFFFFULL) != 0x5555555555555555ULL) return 35;
  if (ll_shl(1LL, 32) != 0x100000000LL) return 36;
  if (ll_shr(0x8000000000000000ULL, 32) != 0x80000000ULL) return 37;
  if (ll_sar(-1LL, 32) != -1LL) return 38;
  if (ll_shl(1LL, 0) != 1LL) return 39;
  if (ll_shl(1LL, 63) != (long long)0x8000000000000000ULL) return 40;
  if (!ll_eq(5LL, 5LL) || ll_ne(5LL, 6LL) == 0) return 41;
  if (!ll_lt(1LL, 2LL) || !ll_gt(2LL, 1LL)) return 42;
  if (!ll_le(1LL, 1LL) || !ll_ge(2LL, 2LL)) return 43;
  if (!ll_ltu(1ULL, 0x100000000ULL)) return 44;
  if (!ll_gtu(0x100000000ULL, 1ULL)) return 45;
  if ((long long)(unsigned int)0x80000000u != 0x80000000LL) return 46;
  if ((long long)(signed char)-1 != -1LL) return 47;
  if ((int)(long long)0x123456789ABCDEF0LL != (int)0x9ABCDEF0) return 48;
  if (ll_identity(0xDEADBEEFCAFEBABELL) != 0xDEADBEEFCAFEBABELL) return 49;
  if (ll_nested(10LL) != 21LL) return 50;
  if (ll_pick(1, 11LL, 22LL) != 11LL) return 51;
  if (ll_pick(0, 11LL, 22LL) != 22LL) return 52;
  if (ll_sum(3, 10LL, 20LL, 30LL) != 60LL) return 53;
  if (read_s8(mk_s8(42LL)) != 42LL) return 54;
  if (arr[0] + arr[1] != 300LL) return 55;
  { long long x = 10LL; x++; if (x != 11LL) return 56; }
  { long long x = 10LL; x--; if (x != 9LL) return 57; }
  if (ll_spill(1LL, 1LL, 1LL, 1LL, 1LL, 1LL, 1LL, 1LL) != 8LL) return 58;
  return 0;
}

int test_main(void) {
  if (add(3, 4) != 7) return 1;
  if (caller5() != 15) return 2;
  if (outer(5) != 31) return 3;
  if (sdiv(-7, 3) != -2) return 4;
  if (smod(-7, 3) != -1) return 5;
  if (!lt(1, 2) || !eq(3, 3)) return 6;
  /* S5 is 8 bytes (char + tail padding before int). */
  if (size1() != 1 || size4() != 4 || size5() != 8) return 7;
  if (size8() != 8 || size12() != 12) return 8;
  if (read4() != 42) return 9;
  if (check_ret4() != 10) return 10;
  if (apply(add, 10, 5) != 15) return 11;
  if (call_variadic() != 60) return 12;
  if (verify_saved_regs() != 0) return 13;
  if (spill_pressure(1, 1, 1, 1, 1, 1, 1, 1) != 8) return 14;
  return test_ll();
}
EOF

cp "${ROOT_DIR}/libc/cdiv.c" "$WORK/cdiv.c"

cat >"$WORK/cross.c" <<'EOF'
int add(int, int);
int cross_main(void) { return add(3, 4); }
EOF

cat >"$WORK/_start_cross.s" <<'EOF'
	.text
	.global _start
_start:
	call cross_main
	movl %eax, %ebx
	movl $1, %eax
	int $0x80
EOF

"$DAVECC" -target x86 -nostdinc -c -o "$WORK/tests.o" "$WORK/tests.c"
"$DAVECC" -target x86 -nostdinc -c -o "$WORK/cross.o" "$WORK/cross.c"

docker_run() {
  local extra_link="$1"
  local expect="$2"
  docker run --rm --platform linux/386 \
    -v "$WORK:/work" -w /work alpine:3.20 sh -c "
      apk add --no-cache binutils gcc musl-dev >/dev/null
      gcc -c _start.s -o _start.o
      gcc -c regcheck.s -o regcheck.o
      gcc -fno-stack-protector -c cdiv.c -o cdiv.o
      gcc -nostdlib -static -Wl,-e,_start -o test.elf _start.o regcheck.o tests.o cdiv.o ${extra_link}
      ./test.elf; echo exit:\$?
    " | tail -1 | sed 's/exit://'
}

main_exit="$(docker_run "" 0)"
[[ "$main_exit" == "0" ]] || {
  echo "main exec test failed: exit=$main_exit" >&2
  exit 1
}

cross_exit="$(docker run --rm --platform linux/386 \
  -v "$WORK:/work" -w /work alpine:3.20 sh -c "
    apk add --no-cache binutils gcc musl-dev >/dev/null
    gcc -c _start_cross.s -o _start.o
    gcc -c regcheck.s -o regcheck.o
    gcc -nostdlib -static -Wl,-e,_start -o cross.elf \
      _start.o cross.o regcheck.o tests.o cdiv.o
    ./cross.elf; echo exit:\$?
  " | tail -1 | sed 's/exit://')"
[[ "$cross_exit" == "7" ]] || {
  echo "cross-TU exec test failed: expected exit 7, got $cross_exit" >&2
  exit 1
}

# GNU i386 struct-return ABI interop: GCC caller -> davecc callee.
cat >"$WORK/davecc_sret.c" <<'EOF'
struct S4 { int x; };
struct S4 davecc_mk4(int v) { struct S4 s; s.x = v; return s; }
EOF

cat >"$WORK/gcc_calls_davecc.c" <<'EOF'
struct S4 { int x; };
struct S4 davecc_mk4(int v);
int gcc_calls_davecc(void) {
  struct S4 s = davecc_mk4(77);
  return s.x;
}
EOF

cat >"$WORK/_start_gcc_calls_davecc.s" <<'EOF'
	.text
	.global _start
_start:
	call gcc_calls_davecc
	movl %eax, %ebx
	movl $1, %eax
	int $0x80
EOF

"$DAVECC" -target x86 -nostdinc -c -o "$WORK/davecc_sret.o" "$WORK/davecc_sret.c"
"$DAVECC" -target x86 -nostdinc -S -o "$WORK/davecc_sret.s" "$WORK/davecc_sret.c"
grep -q 'ret \$4' "$WORK/davecc_sret.s" || {
  echo "davecc struct-return callee missing ret \$4" >&2
  exit 1
}

gcc_calls_davecc_exit="$(docker run --rm --platform linux/386 \
  -v "$WORK:/work" -w /work alpine:3.20 sh -c "
    apk add --no-cache binutils gcc musl-dev >/dev/null
    gcc -c _start_gcc_calls_davecc.s -o _start.o
    gcc -fno-stack-protector -c gcc_calls_davecc.c -o gcc_calls_davecc.o
    gcc -nostdlib -static -Wl,-e,_start -o gcc_calls_davecc.elf \
      _start.o gcc_calls_davecc.o davecc_sret.o
    ./gcc_calls_davecc.elf; echo exit:\$?
  " | tail -1 | sed 's/exit://')"
[[ "$gcc_calls_davecc_exit" == "77" ]] || {
  echo "GCC-caller/davecc-sret interop failed: expected exit 77, got $gcc_calls_davecc_exit" >&2
  exit 1
}

# GNU i386 struct-return ABI interop: davecc caller -> GCC callee.
cat >"$WORK/gcc_sret.c" <<'EOF'
struct S4 { int x; };
struct S4 gcc_mk4(int v) { struct S4 s; s.x = v; return s; }
EOF

cat >"$WORK/davecc_calls_gcc.c" <<'EOF'
struct S4 { int x; };
struct S4 gcc_mk4(int v);
int davecc_calls_gcc(void) {
  struct S4 s = gcc_mk4(88);
  return s.x;
}
EOF

cat >"$WORK/_start_davecc_calls_gcc.s" <<'EOF'
	.text
	.global _start
_start:
	call davecc_calls_gcc
	movl %eax, %ebx
	movl $1, %eax
	int $0x80
EOF

"$DAVECC" -target x86 -nostdinc -c -o "$WORK/davecc_calls_gcc.o" "$WORK/davecc_calls_gcc.c"
"$DAVECC" -target x86 -nostdinc -S -o "$WORK/davecc_calls_gcc.s" "$WORK/davecc_calls_gcc.c"
grep -Eq 'addl \$c, %esp|addl \$12, %esp' "$WORK/davecc_calls_gcc.s" || {
  echo "davecc struct-return caller missing adjusted stack cleanup" >&2
  exit 1
}

davecc_calls_gcc_exit="$(docker run --rm --platform linux/386 \
  -v "$WORK:/work" -w /work alpine:3.20 sh -c "
    apk add --no-cache binutils gcc musl-dev >/dev/null
    gcc -c _start_davecc_calls_gcc.s -o _start.o
    gcc -fno-stack-protector -c gcc_sret.c -o gcc_sret.o
    gcc -nostdlib -static -Wl,-e,_start -o davecc_calls_gcc.elf \
      _start.o davecc_calls_gcc.o gcc_sret.o
    ./davecc_calls_gcc.elf; echo exit:\$?
  " | tail -1 | sed 's/exit://')"
[[ "$davecc_calls_gcc_exit" == "88" ]] || {
  echo "davecc-caller/GCC-sret interop failed: expected exit 88, got $davecc_calls_gcc_exit" >&2
  exit 1
}

# GNU i386 long long interop: GCC caller -> davecc callee.
cat >"$WORK/davecc_ll.c" <<'EOF'
long long davecc_ll_add(long long a, long long b) { return a + b; }
EOF

cat >"$WORK/gcc_calls_davecc_ll.c" <<'EOF'
long long davecc_ll_add(long long, long long);
int gcc_calls_davecc_ll(void) {
  return (int)davecc_ll_add(0x100000000LL, 7LL);
}
EOF

cat >"$WORK/_start_gcc_calls_davecc_ll.s" <<'EOF'
	.text
	.global _start
_start:
	call gcc_calls_davecc_ll
	movl %eax, %ebx
	movl $1, %eax
	int $0x80
EOF

"$DAVECC" -target x86 -nostdinc -c -o "$WORK/davecc_ll.o" "$WORK/davecc_ll.c"
gcc_calls_davecc_ll_exit="$(docker run --rm --platform linux/386 \
  -v "$WORK:/work" -w /work alpine:3.20 sh -c "
    apk add --no-cache binutils gcc musl-dev >/dev/null
    gcc -c _start_gcc_calls_davecc_ll.s -o _start.o
    gcc -fno-stack-protector -c cdiv.c -o cdiv.o
    gcc -fno-stack-protector -c gcc_calls_davecc_ll.c -o gcc_calls_davecc_ll.o
    gcc -nostdlib -static -Wl,-e,_start -o gcc_calls_davecc_ll.elf \
      _start.o gcc_calls_davecc_ll.o davecc_ll.o cdiv.o
    ./gcc_calls_davecc_ll.elf; echo exit:\$?
  " | tail -1 | sed 's/exit://')"
[[ "$gcc_calls_davecc_ll_exit" == "7" ]] || {
  echo "GCC-caller/davecc long long interop failed: expected exit 7, got $gcc_calls_davecc_ll_exit" >&2
  exit 1
}

# davecc caller -> GCC long long callee.
cat >"$WORK/gcc_ll.c" <<'EOF'
long long gcc_ll_identity(long long x) { return x; }
EOF

cat >"$WORK/davecc_calls_gcc_ll.c" <<'EOF'
long long gcc_ll_identity(long long);
int davecc_calls_gcc_ll(void) {
  return (int)gcc_ll_identity(0x100000005LL);
}
EOF

cat >"$WORK/_start_davecc_calls_gcc_ll.s" <<'EOF'
	.text
	.global _start
_start:
	call davecc_calls_gcc_ll
	movl %eax, %ebx
	movl $1, %eax
	int $0x80
EOF

"$DAVECC" -target x86 -nostdinc -c -o "$WORK/davecc_calls_gcc_ll.o" "$WORK/davecc_calls_gcc_ll.c"
davecc_calls_gcc_ll_exit="$(docker run --rm --platform linux/386 \
  -v "$WORK:/work" -w /work alpine:3.20 sh -c "
    apk add --no-cache binutils gcc musl-dev >/dev/null
    gcc -c _start_davecc_calls_gcc_ll.s -o _start.o
    gcc -fno-stack-protector -c gcc_ll.c -o gcc_ll.o
    gcc -nostdlib -static -Wl,-e,_start -o davecc_calls_gcc_ll.elf \
      _start.o davecc_calls_gcc_ll.o gcc_ll.o
    ./davecc_calls_gcc_ll.elf; echo exit:\$?
  " | tail -1 | sed 's/exit://')"
[[ "$davecc_calls_gcc_ll_exit" == "5" ]] || {
  echo "davecc-caller/GCC long long interop failed: expected exit 5, got $davecc_calls_gcc_ll_exit" >&2
  exit 1
}

echo "x86 i386 exec tests passed (main exit 0, cross-TU exit 7, GCC sret interop 77/88, long long interop 7/5)"
