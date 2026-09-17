#!/bin/bash
# Front-end de-virtualization must emit a direct call for final classes/methods
# and for complete objects, and must keep an indirect call through a non-final
# base pointer or reference.
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "usage: $0 <davecc>" >&2
  exit 2
fi

davecc=$1
work="${TEST_TMPDIR:-$(mktemp -d "${TMPDIR:-/tmp}/davecc-devirt.XXXXXX")}"
mkdir -p "$work"

cat >"$work/devirt.cpp" <<'EOF'
struct Base {
  virtual int id();
  virtual ~Base();
};
struct Mid : Base {
  int id() final override;
};
struct FinalDerived final : Base {
  int id() override;
};

int call_base_ptr(Base* p) { return p->id(); }
int call_mid_ptr(Mid* p) { return p->id(); }
int call_final_ptr(FinalDerived* p) { return p->id(); }
int call_object(FinalDerived v) { return v.id(); }
EOF

if ! "$davecc" -target x86_64 -S -std=c++20 -nostdinc -nostdlib \
  -o "$work/devirt.s" "$work/devirt.cpp"; then
  echo "davecc failed to compile de-virtualization test" >&2
  exit 1
fi

python3 - "$work/devirt.s" <<'PY'
import re
import sys

text = open(sys.argv[1]).read().splitlines()

def function_body(label):
    body = []
    in_fn = False
    for line in text:
        if line.startswith(label + ":"):
            in_fn = True
            continue
        if in_fn:
            if line.startswith(".func_end"):
                break
            if line.startswith("_Z") and line.endswith(":"):
                break
            body.append(line)
    if not in_fn:
        raise SystemExit("missing function %s" % label)
    return body

def calls(body):
    found = []
    for line in body:
        stripped = line.strip()
        if re.search(r"\bcall\b", stripped):
            found.append(stripped)
    return found

def require_direct(label):
    found = calls(function_body(label))
    if not found:
        raise SystemExit("%s: expected a direct call, found none" % label)
    for c in found:
        if re.search(r"call\s+\*", c):
            raise SystemExit("%s: expected a direct call, got %s" % (label, c))

def require_indirect(label):
    found = calls(function_body(label))
    if not any(re.search(r"call\s+\*", c) for c in found):
        raise SystemExit(
            "%s: expected an indirect virtual call, got %s" % (label, found))

require_indirect("_Z13call_base_ptrP4Base")
require_direct("_Z12call_mid_ptrP3Mid")
require_direct("_Z14call_final_ptrP12FinalDerived")
require_direct("_Z11call_object12FinalDerived")
print("devirt_test ok")
PY
