#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc x86_64" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/lto-inlining"
mkdir -p "$WORK"

cat >"$WORK/add.c" <<'EOF'
int add2(int x) { return x + 2; }
EOF

cat >"$WORK/main.c" <<'EOF'
int add2(int);
__attribute__((noinline)) int caller(void) { return add2(3); }
int main(void) { return caller(); }
EOF

cat >"$WORK/static_a.c" <<'EOF'
static int helper(void) { return 1; }
int from_a(void) { return helper(); }
EOF

cat >"$WORK/static_b.c" <<'EOF'
static int helper(void) { return 2; }
int from_a(void);
int from_b(void) { return helper(); }
int main(void) { return from_a() + from_b(); }
EOF

common=(
  "$DAVECC" -target x86_64 -O2 -nostdinc -nostdlib -static
)

# Without LTO the callee body is not visible, so the call remains.
"${common[@]}" -S "$WORK/main.c" -o "$WORK/main.s"
if ! grep -Eq '[[:space:]]call[[:space:]]+add2' "$WORK/main.s"; then
  echo "expected a call to add2 without LTO" >&2
  cat "$WORK/main.s" >&2
  exit 1
fi

# Combined -flto compile inlines add2 regardless of file order.
for order in add_first main_first; do
  local_s="$WORK/combined_${order}.s"
  if [[ "$order" == add_first ]]; then
    "${common[@]}" -flto -S "$WORK/add.c" "$WORK/main.c" -o "$local_s"
  else
    "${common[@]}" -flto -S "$WORK/main.c" "$WORK/add.c" -o "$local_s"
  fi
  if grep -Eq '[[:space:]]call[[:space:]]+add2' "$local_s"; then
    echo "LTO $order retained a call to add2" >&2
    cat "$local_s" >&2
    exit 1
  fi
  if grep -Eq '^add2:|[[:space:]]\.type[[:space:]]+add2,|[[:space:]]\.global[[:space:]]+add2' "$local_s"; then
    echo "LTO $order still defined add2 after inlining" >&2
    cat "$local_s" >&2
    exit 1
  fi
done

"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/add.c" "$WORK/main.c" \
  -o "$WORK/lto.exe"
set +e
"$INTERPRETER" -i "$WORK/lto.exe"
rc=$?
set -e
if [[ "$rc" -ne 5 ]]; then
  echo "LTO executable returned $rc; expected 5" >&2
  exit 1
fi

# File-scope statics with the same source name must not collide.
"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/static_a.c" "$WORK/static_b.c" \
  -o "$WORK/static.exe"
set +e
"$INTERPRETER" -i "$WORK/static.exe"
rc=$?
set -e
if [[ "$rc" -ne 3 ]]; then
  echo "LTO static-collision executable returned $rc; expected 3" >&2
  exit 1
fi

# Inlined file-scope statics should not be emitted.
"${common[@]}" -flto -S "$WORK/static_a.c" "$WORK/static_b.c" -o "$WORK/static.s"
if grep -Eq 'helper\.lto' "$WORK/static.s"; then
  echo "LTO emitted an inlined static helper" >&2
  cat "$WORK/static.s" >&2
  exit 1
fi

# -c -flto writes IR objects; linking them must not need the sources.
"${common[@]}" -c -flto "$WORK/add.c" -o "$WORK/add.o"
"${common[@]}" -c -flto "$WORK/main.c" -o "$WORK/main.o"
rm -f "$WORK/add.c" "$WORK/main.c"
"${common[@]}" -O2 -Wl,-e -Wl,main "$WORK/add.o" "$WORK/main.o" \
  -o "$WORK/stub.exe"
set +e
"$INTERPRETER" -i "$WORK/stub.exe"
rc=$?
set -e
if [[ "$rc" -ne 5 ]]; then
  echo "LTO stub link returned $rc; expected 5" >&2
  exit 1
fi

# Include guards in a shared header must survive the second TU.
cat >"$WORK/h.h" <<'EOF'
#ifndef H_H
#define H_H
struct Point { int x, y; };
static inline int sum(struct Point p) { return p.x + p.y; }
#endif
EOF

cat >"$WORK/hdr_a.c" <<'EOF'
#include "h.h"
int from_a(void) {
  struct Point p;
  p.x = 3;
  p.y = 4;
  return sum(p);
}
EOF

cat >"$WORK/hdr_b.c" <<'EOF'
#include "h.h"
int from_a(void);
int main(void) {
  struct Point p;
  p.x = 1;
  p.y = 2;
  return from_a() + sum(p);
}
EOF

"${common[@]}" -flto -I"$WORK" -Wl,-e -Wl,main "$WORK/hdr_a.c" "$WORK/hdr_b.c" \
  -o "$WORK/hdr.exe"
set +e
"$INTERPRETER" -i "$WORK/hdr.exe"
rc=$?
set -e
if [[ "$rc" -ne 10 ]]; then
  echo "LTO include-guard executable returned $rc; expected 10" >&2
  exit 1
fi

# Same-name file-scope typedefs and tags in two .c files.
cat >"$WORK/type_a.c" <<'EOF'
typedef int T;
struct S { int x; };
enum { K = 1 };
int from_a(void) {
  T v = K;
  struct S s;
  s.x = v;
  return s.x;
}
EOF

cat >"$WORK/type_b.c" <<'EOF'
typedef int T;
struct S { int x; };
enum { K = 2 };
int from_a(void);
int main(void) {
  T v = K;
  struct S s;
  s.x = v;
  return from_a() + s.x;
}
EOF

"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/type_a.c" "$WORK/type_b.c" \
  -o "$WORK/type.exe"
set +e
"$INTERPRETER" -i "$WORK/type.exe"
rc=$?
set -e
if [[ "$rc" -ne 3 ]]; then
  echo "LTO file-scope type executable returned $rc; expected 3" >&2
  exit 1
fi

# Tentative globals in two TUs are one object.
cat >"$WORK/tent_a.c" <<'EOF'
int g;
int bump(void) { return ++g; }
EOF

cat >"$WORK/tent_b.c" <<'EOF'
int g;
int bump(void);
int main(void) { return bump() + bump(); }
EOF

"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/tent_a.c" "$WORK/tent_b.c" \
  -o "$WORK/tent.exe"
set +e
"$INTERPRETER" -i "$WORK/tent.exe"
rc=$?
set -e
if [[ "$rc" -ne 3 ]]; then
  echo "LTO tentative executable returned $rc; expected 3" >&2
  exit 1
fi

# -c -flto captures -D in the IR object; sources are not needed at link.
cat >"$WORK/def_a.c" <<'EOF'
int value(void) { return VALUE; }
EOF

cat >"$WORK/def_b.c" <<'EOF'
int value(void);
int main(void) { return value(); }
EOF

"${common[@]}" -c -flto -DVALUE=7 "$WORK/def_a.c" -o "$WORK/def_a.o"
"${common[@]}" -c -flto "$WORK/def_b.c" -o "$WORK/def_b.o"
rm -f "$WORK/def_a.c" "$WORK/def_b.c"
"${common[@]}" -Wl,-e -Wl,main "$WORK/def_a.o" "$WORK/def_b.o" \
  -o "$WORK/def.exe"
set +e
"$INTERPRETER" -i "$WORK/def.exe"
rc=$?
set -e
if [[ "$rc" -ne 7 ]]; then
  echo "LTO -D link returned $rc; expected 7" >&2
  exit 1
fi

# -c -flto captures -I includes in the IR object; sources are not needed at link.
"${common[@]}" -c -flto -I"$WORK" "$WORK/hdr_a.c" -o "$WORK/hdr_a.o"
"${common[@]}" -c -flto -I"$WORK" "$WORK/hdr_b.c" -o "$WORK/hdr_b.o"
rm -f "$WORK/hdr_a.c" "$WORK/hdr_b.c"
"${common[@]}" -Wl,-e -Wl,main "$WORK/hdr_a.o" "$WORK/hdr_b.o" \
  -o "$WORK/hdr_stub.exe"
set +e
"$INTERPRETER" -i "$WORK/hdr_stub.exe"
rc=$?
set -e
if [[ "$rc" -ne 10 ]]; then
  echo "LTO -I link returned $rc; expected 10" >&2
  exit 1
fi

# C++ template defined in one TU, used in another; link without sources.
cat >"$WORK/tmpl_a.cpp" <<'EOF'
template <typename T>
T add1(T x) { return x + 1; }
int from_a(int x) { return add1(x); }
EOF

cat >"$WORK/tmpl_b.cpp" <<'EOF'
int from_a(int);
int main() { return from_a(4); }
EOF

cxx_common=(
  "$DAVECC" -target x86_64 -O2 -std=c++20 -nostdinc -nostdlib -static
)
"${cxx_common[@]}" -c -flto "$WORK/tmpl_a.cpp" -o "$WORK/tmpl_a.o"
"${cxx_common[@]}" -c -flto "$WORK/tmpl_b.cpp" -o "$WORK/tmpl_b.o"
rm -f "$WORK/tmpl_a.cpp" "$WORK/tmpl_b.cpp"
"${cxx_common[@]}" -Wl,-e -Wl,main "$WORK/tmpl_a.o" "$WORK/tmpl_b.o" \
  -o "$WORK/tmpl.exe"
set +e
"$INTERPRETER" -i "$WORK/tmpl.exe"
rc=$?
set -e
if [[ "$rc" -ne 5 ]]; then
  echo "LTO C++ template link returned $rc; expected 5" >&2
  exit 1
fi

# Whole-program LTO drops unreferenced globals and functions; __attribute__((used))
# keeps a root that nothing calls.
cat >"$WORK/dead.c" <<'EOF'
int dead_g;
int dead_fn(void) { return 9; }
__attribute__((used)) int kept_fn(void) { return 3; }
int main(void) { return 1; }
EOF
"${common[@]}" -flto -S "$WORK/dead.c" -o "$WORK/dead.s"
if grep -Eq 'dead_fn|dead_g' "$WORK/dead.s"; then
  echo "LTO kept an unreferenced symbol" >&2
  cat "$WORK/dead.s" >&2
  exit 1
fi
if ! grep -Eq 'kept_fn' "$WORK/dead.s"; then
  echo "LTO dropped __attribute__((used)) kept_fn" >&2
  cat "$WORK/dead.s" >&2
  exit 1
fi

# Multiple returns, address-taken args, and struct-by-value must inline at IR LTO.
cat >"$WORK/early.c" <<'EOF'
int early(int x) {
  if (x < 0) return -1;
  return x + 1;
}
EOF
cat >"$WORK/early_main.c" <<'EOF'
int early(int);
__attribute__((noinline)) int caller(void) { return early(-2) + early(3); }
int main(void) { return caller(); }
EOF
"${common[@]}" -flto -S "$WORK/early.c" "$WORK/early_main.c" -o "$WORK/early.s"
if grep -Eq '[[:space:]]call[[:space:]]+early' "$WORK/early.s"; then
  echo "LTO retained a call to early" >&2
  cat "$WORK/early.s" >&2
  exit 1
fi
"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/early.c" "$WORK/early_main.c" \
  -o "$WORK/early.exe"
set +e
"$INTERPRETER" -i "$WORK/early.exe"
rc=$?
set -e
if [[ "$rc" -ne 3 ]]; then
  echo "LTO early-return executable returned $rc; expected 3" >&2
  exit 1
fi

cat >"$WORK/addr.c" <<'EOF'
int addr(int x) {
  int *p = &x;
  *p += 3;
  return x;
}
EOF
cat >"$WORK/addr_main.c" <<'EOF'
int addr(int);
__attribute__((noinline)) int caller(void) { return addr(4); }
int main(void) { return caller(); }
EOF
"${common[@]}" -flto -S "$WORK/addr.c" "$WORK/addr_main.c" -o "$WORK/addr.s"
if grep -Eq '[[:space:]]call[[:space:]]+addr' "$WORK/addr.s"; then
  echo "LTO retained a call to addr" >&2
  cat "$WORK/addr.s" >&2
  exit 1
fi
"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/addr.c" "$WORK/addr_main.c" \
  -o "$WORK/addr.exe"
set +e
"$INTERPRETER" -i "$WORK/addr.exe"
rc=$?
set -e
if [[ "$rc" -ne 7 ]]; then
  echo "LTO address-taken executable returned $rc; expected 7" >&2
  exit 1
fi

cat >"$WORK/sum.c" <<'EOF'
struct S { int a, b; };
int sum(struct S s) { return s.a + s.b; }
EOF
cat >"$WORK/sum_main.c" <<'EOF'
struct S { int a, b; };
int sum(struct S);
__attribute__((noinline)) int caller(void) {
  struct S s;
  s.a = 3;
  s.b = 4;
  return sum(s);
}
int main(void) { return caller(); }
EOF
"${common[@]}" -flto -S "$WORK/sum.c" "$WORK/sum_main.c" -o "$WORK/sum.s"
if grep -Eq '[[:space:]]call[[:space:]]+sum' "$WORK/sum.s"; then
  echo "LTO retained a call to sum" >&2
  cat "$WORK/sum.s" >&2
  exit 1
fi
"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/sum.c" "$WORK/sum_main.c" \
  -o "$WORK/sum.exe"
set +e
"$INTERPRETER" -i "$WORK/sum.exe"
rc=$?
set -e
if [[ "$rc" -ne 7 ]]; then
  echo "LTO struct-by-value executable returned $rc; expected 7" >&2
  exit 1
fi

# C++ this-pointer call across TUs.
cat >"$WORK/this_t.h" <<'EOF'
struct T {
  int x;
  int add(int y);
};
EOF
cat >"$WORK/this_a.cpp" <<'EOF'
#include "this_t.h"
int T::add(int y) { return x + y; }
EOF
cat >"$WORK/this_b.cpp" <<'EOF'
#include "this_t.h"
__attribute__((noinline)) int from_b() {
  T t;
  t.x = 3;
  return t.add(2);
}
int main() { return from_b(); }
EOF
"${cxx_common[@]}" -I"$WORK" -flto -S "$WORK/this_a.cpp" "$WORK/this_b.cpp" \
  -o "$WORK/this.s"
if grep -Eq '[[:space:]]call[[:space:]]+_ZN1T3addEi' "$WORK/this.s"; then
  echo "LTO retained a call to T::add" >&2
  cat "$WORK/this.s" >&2
  exit 1
fi
"${cxx_common[@]}" -I"$WORK" -flto -Wl,-e -Wl,main "$WORK/this_a.cpp" \
  "$WORK/this_b.cpp" -o "$WORK/this.exe"
set +e
"$INTERPRETER" -i "$WORK/this.exe"
rc=$?
set -e
if [[ "$rc" -ne 5 ]]; then
  echo "LTO C++ this executable returned $rc; expected 5" >&2
  exit 1
fi

# Same template instantiated in two TUs merges as one COMDAT.
cat >"$WORK/odr_a.cpp" <<'EOF'
template <typename T>
T add1(T x) { return x + 1; }
int from_a(int x) { return add1(x); }
EOF
cat >"$WORK/odr_b.cpp" <<'EOF'
template <typename T>
T add1(T x) { return x + 1; }
int from_a(int);
int main() { return from_a(4) + add1(0); }
EOF
"${cxx_common[@]}" -flto -S "$WORK/odr_a.cpp" "$WORK/odr_b.cpp" -o "$WORK/odr.s"
if grep -Eq 'add1.*\.lto\.' "$WORK/odr.s"; then
  echo "LTO renamed a template instantiation as if it were TU-local" >&2
  cat "$WORK/odr.s" >&2
  exit 1
fi
"${cxx_common[@]}" -flto -Wl,-e -Wl,main "$WORK/odr_a.cpp" "$WORK/odr_b.cpp" \
  -o "$WORK/odr.exe"
set +e
"$INTERPRETER" -i "$WORK/odr.exe"
rc=$?
set -e
if [[ "$rc" -ne 6 ]]; then
  echo "LTO ODR template executable returned $rc; expected 6" >&2
  exit 1
fi

# Vtables from two TUs merge to one weak ODR copy, not per-TU .lto. names.
cat >"$WORK/vt.h" <<'EOF'
struct Base {
  virtual int f();
  virtual ~Base() {}
};
int call_f(Base* p);
EOF
cat >"$WORK/vt_a.cpp" <<'EOF'
#include "vt.h"
int Base::f() { return 3; }
int call_f(Base* p) { return p->f(); }
int from_a() { Base b; return call_f(&b); }
EOF
cat >"$WORK/vt_b.cpp" <<'EOF'
#include "vt.h"
int from_a();
int main() {
  Base b;
  return from_a() + call_f(&b);
}
EOF
"${cxx_common[@]}" -I"$WORK" -flto -S "$WORK/vt_a.cpp" "$WORK/vt_b.cpp" \
  -o "$WORK/vt.s"
if grep -Eq '__davecc_vtbl_.*\.lto\.' "$WORK/vt.s"; then
  echo "LTO renamed a vtable as if it were TU-local" >&2
  cat "$WORK/vt.s" >&2
  exit 1
fi
vtbl_defs=$(grep -cE '^__davecc_vtbl_Base:' "$WORK/vt.s" || true)
if [[ "$vtbl_defs" -ne 1 ]]; then
  echo "LTO emitted $vtbl_defs copies of Base's vtable; expected 1" >&2
  cat "$WORK/vt.s" >&2
  exit 1
fi

# -u / --undefined keeps a otherwise-dead LTO symbol.
cat >"$WORK/undef.c" <<'EOF'
int pulled(void) { return 4; }
int main(void) { return 1; }
EOF
"${common[@]}" -flto -S "$WORK/undef.c" -o "$WORK/undef.s"
if grep -Eq 'pulled' "$WORK/undef.s"; then
  echo "LTO kept pulled without -u" >&2
  cat "$WORK/undef.s" >&2
  exit 1
fi
"${common[@]}" -flto -S -Wl,-u -Wl,pulled "$WORK/undef.c" -o "$WORK/undef_u.s"
if ! grep -Eq 'pulled' "$WORK/undef_u.s"; then
  echo "LTO dropped pulled despite -u" >&2
  cat "$WORK/undef_u.s" >&2
  exit 1
fi

# LTO objects inside a System V .a are extracted at link; unused members DCE.
cat >"$WORK/add.c" <<'EOF'
int add2(int x) { return x + 2; }
EOF
cat >"$WORK/main.c" <<'EOF'
int add2(int);
__attribute__((noinline)) int caller(void) { return add2(3); }
int main(void) { return caller(); }
EOF
cat >"$WORK/unused.c" <<'EOF'
int unused_lib_fn(void) { return 9; }
EOF
"${common[@]}" -c -flto "$WORK/add.c" -o "$WORK/add.o"
"${common[@]}" -c -flto "$WORK/unused.c" -o "$WORK/unused.o"
python3 - "$WORK/libadd.a" "$WORK/add.o" "$WORK/unused.o" <<'PY'
import sys
out, *members = sys.argv[1:]
buf = bytearray(b"!<arch>\n")
for path in members:
    name = path.rsplit("/", 1)[-1].encode("ascii")[:15] + b"/"
    data = open(path, "rb").read()
    hdr = (
        name.ljust(16)
        + b"0".ljust(12)
        + b"0".ljust(6)
        + b"0".ljust(6)
        + b"644".ljust(8)
        + str(len(data)).encode("ascii").ljust(10)
        + b"`\n"
    )
    if len(hdr) != 60:
        raise SystemExit("bad ar header length %d" % len(hdr))
    buf += hdr
    buf += data
    if len(data) % 2:
        buf += b"\n"
open(out, "wb").write(buf)
PY
rm -f "$WORK/add.c" "$WORK/unused.c" "$WORK/add.o" "$WORK/unused.o"
"${common[@]}" -flto -S "$WORK/main.c" "$WORK/libadd.a" -o "$WORK/arch.s"
if grep -Eq '[[:space:]]call[[:space:]]+add2' "$WORK/arch.s"; then
  echo "LTO archive retained a call to add2" >&2
  cat "$WORK/arch.s" >&2
  exit 1
fi
if grep -Eq 'unused_lib_fn' "$WORK/arch.s"; then
  echo "LTO archive kept an unreferenced member" >&2
  cat "$WORK/arch.s" >&2
  exit 1
fi
"${common[@]}" -flto -Wl,-e -Wl,main "$WORK/main.c" "$WORK/libadd.a" \
  -o "$WORK/arch.exe"
set +e
"$INTERPRETER" -i "$WORK/arch.exe"
rc=$?
set -e
if [[ "$rc" -ne 5 ]]; then
  echo "LTO archive executable returned $rc; expected 5" >&2
  exit 1
fi

# -g -flto keeps source locations through IR objects after the .c is gone.
cat >"$WORK/dbg.c" <<'EOF'
int add2(int x) { return x + 2; }
int main(void) { return add2(3); }
EOF
"${common[@]}" -c -flto -g "$WORK/dbg.c" -o "$WORK/dbg.o"
rm -f "$WORK/dbg.c"
"${common[@]}" -g -S "$WORK/dbg.o" -o "$WORK/dbg.s"
if ! grep -Eq 'dbg\.c' "$WORK/dbg.s"; then
  echo "LTO -g assembly lost the original source filename" >&2
  cat "$WORK/dbg.s" >&2
  exit 1
fi

# Known-type de-virtualization after cross-TU inlining of a virtual call.
cat >"$WORK/dv_base.h" <<'EOF'
struct Base {
  virtual int f();
};
int call_f(Base* p);
EOF
cat >"$WORK/dv_a.cpp" <<'EOF'
#include "dv_base.h"
int Base::f() { return 1; }
int call_f(Base* p) { return p->f(); }
EOF
cat >"$WORK/dv_b.cpp" <<'EOF'
#include "dv_base.h"
struct Derived : Base {
  int f() override { return 7; }
};
int main() {
  Derived d;
  return call_f(&d);
}
EOF
"${cxx_common[@]}" -I"$WORK" -flto -S "$WORK/dv_a.cpp" "$WORK/dv_b.cpp" \
  -o "$WORK/dv.s"
if grep -Eq 'call[[:space:]]+\*' "$WORK/dv.s"; then
  echo "LTO kept an indirect virtual call after known-type de-virtualization" >&2
  cat "$WORK/dv.s" >&2
  exit 1
fi
if grep -Eq '[[:space:]]call[[:space:]]+_ZN4Base1fEv' "$WORK/dv.s"; then
  echo "LTO de-virtualized to Base::f instead of Derived::f" >&2
  cat "$WORK/dv.s" >&2
  exit 1
fi

# Whole-program unique overrider: noinline call through Base* with one impl.
cat >"$WORK/wpd_a.cpp" <<'EOF'
struct Base {
  virtual int f();
};
int Base::f() { return 4; }
__attribute__((noinline)) int call_f(Base* p) { return p->f(); }
EOF
cat >"$WORK/wpd_b.cpp" <<'EOF'
struct Base {
  virtual int f();
};
int call_f(Base* p);
int main() {
  Base b;
  return call_f(&b);
}
EOF
"${cxx_common[@]}" -flto -S "$WORK/wpd_a.cpp" "$WORK/wpd_b.cpp" -o "$WORK/wpd.s"
if grep -Eq 'call[[:space:]]+\*' "$WORK/wpd.s"; then
  echo "LTO unique-overrider left an indirect virtual call" >&2
  cat "$WORK/wpd.s" >&2
  exit 1
fi
if ! grep -Eq '[[:space:]]call[[:space:]]+_ZN4Base1fEv|movq \$4, %rax' "$WORK/wpd.s"; then
  echo "LTO unique-overrider did not emit Base::f or its inlined body" >&2
  cat "$WORK/wpd.s" >&2
  exit 1
fi

# Multiple overriders must not be collapsed in a noinline indirect call.
cat >"$WORK/wpd2_a.cpp" <<'EOF'
struct Base { virtual int f(); };
int Base::f() { return 1; }
__attribute__((noinline)) int call_f(Base* p) { return p->f(); }
EOF
cat >"$WORK/wpd2_b.cpp" <<'EOF'
struct Base { virtual int f(); };
struct Derived : Base { int f() override; };
int Derived::f() { return 2; }
int call_f(Base* p);
int main() {
  Derived d;
  return call_f(&d);
}
EOF
"${cxx_common[@]}" -flto -S "$WORK/wpd2_a.cpp" "$WORK/wpd2_b.cpp" -o "$WORK/wpd2.s"
if ! grep -Eq 'call[[:space:]]+\*' "$WORK/wpd2.s"; then
  echo "LTO collapsed a virtual call that has multiple overriders" >&2
  cat "$WORK/wpd2.s" >&2
  exit 1
fi

