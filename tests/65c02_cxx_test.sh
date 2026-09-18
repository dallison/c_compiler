#!/bin/bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
  echo "usage: $0 davecc interpreter rom source ranges_math_test" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
ROM="$ROOT/$3"
SOURCE="$ROOT/$4"
RANGES_MATH_TEST="$ROOT/$5"
WORK="${TEST_TMPDIR:-/tmp}/65c02-cxx"
mkdir -p "$WORK"

# The source dispatches through a secondary base and therefore requires a
# non-zero `this`-adjustor thunk. The driver must also infer static linkage for
# this static-only target.
"$DAVECC" -target 65c02 "$SOURCE" -o "$WORK/test.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/test.exe"
if grep -aq '__davecc_tls_' "$WORK/test.exe"; then
  echo "thread-local runtime linked into a threadless 65C02 program" >&2
  exit 1
fi
for symbol in AlignSize ExpandHeap InitFreeList TakeStartOfFreeBlock \
              exit_functions exit_lock; do
  if strings "$WORK/test.exe" | grep -x "$symbol" >/dev/null; then
    echo "internal runtime symbol exported: $symbol" >&2
    exit 1
  fi
done

cat >"$WORK/hello_iostream.cpp" <<'SRC'
#include <iostream>

int main() {
  for (int i = 0; i < 10; i++) {
    std::cout << "hello world" << std::endl;
  }
  return 0;
}
SRC

"$DAVECC" -target 65c02 "$WORK/hello_iostream.cpp" -o "$WORK/hello_iostream.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/hello_iostream.exe"

# resulta must return the referent address, not a load of the object.
# Otherwise use_facet / cout << int jump through a wild vptr.
cat >"$WORK/reference_return.cpp" <<'SRC'
int value = 42;
int& get_ref() { return value; }
int* pointer = &value;
int*& get_ptr_ref() { return pointer; }

int main() {
  if (get_ref() != 42) {
    return 1;
  }
  if (get_ptr_ref() != &value) {
    return 2;
  }
  return 0;
}
SRC

"$DAVECC" -target 65c02 "$WORK/reference_return.cpp" -o "$WORK/reference_return.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/reference_return.exe"

# A named local class reference must keep the storea that binds it, or a
# later virtual call reads an uninitialized register.
cat >"$WORK/named_ref_virtual.cpp" <<'SRC'
struct Base {
  virtual int f() const { return 1; }
};
struct Derived : Base {
  int f() const override { return 7; }
};

int main() {
  Derived derived;
  Base& ref = derived;
  if (ref.f() != 7) {
    return 1;
  }
  const Base& cref = derived;
  if (cref.f() != 7) {
    return 2;
  }
  return 0;
}
SRC

"$DAVECC" -target 65c02 "$WORK/named_ref_virtual.cpp" -o "$WORK/named_ref_virtual.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/named_ref_virtual.exe"

cat >"$WORK/cout_int.cpp" <<'SRC'
#include <iostream>

int main() {
  std::cout << 42 << '\n';
  return 0;
}
SRC

"$DAVECC" -target 65c02 "$WORK/cout_int.cpp" -o "$WORK/cout_int.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/cout_int.exe"

cat >"$WORK/atoi_vector.cpp" <<'SRC'
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

int main() {
  if (atoi("5") != 5) {
    return 1;
  }
  if (!((unsigned char)'5' <= (unsigned char)'9') ||
      ((unsigned char)'9' <= (unsigned char)'5') || !(-1 <= 0) || (1 <= 0)) {
    return 2;
  }
  std::vector<std::string> values;
  for (int i = 0; i < 10; i++) {
    values.push_back("hello world " + std::to_string(i));
  }
  for (size_t i = 0; i < values.size(); i++) {
    std::cout << values[i] << std::endl;
  }
  _Exit(0);
}
SRC

"$DAVECC" -target 65c02 "$WORK/atoi_vector.cpp" -o "$WORK/atoi_vector.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/atoi_vector.exe"

cat >"$WORK/map_stream.cpp" <<'SRC'
#include <iostream>
#include <map>
#include <string>

int main() {
  std::map<std::string, int> values;
  auto it = values.begin();
  std::cout << it->second << std::endl;
}
SRC

# The nested operator-> call and member load route the loaded value into a
# forward temporary. Verify that 65C02 lowering handles that destination.
"$DAVECC" -target 65c02 -S "$WORK/map_stream.cpp" -o "$WORK/map_stream.s"
if grep -q 'bad_exception' "$WORK/map_stream.s"; then
  echo "unused std::bad_exception RTTI/vtable emitted for <map>" >&2
  exit 1
fi

cat >"$WORK/init_array_alignment.cpp" <<'SRC'
unsigned char marker = 1;

struct Initializer {
  Initializer() { marker = 7; }
};

Initializer initializer;

int main() {
  return marker == 7 ? 0 : 1;
}
SRC

# Keep an odd-sized .data payload before the pointer-aligned .init_array.  Both
# the section virtual address and PT_LOAD file span must include the padding.
"$DAVECC" -target 65c02 "$WORK/init_array_alignment.cpp" \
  -o "$WORK/init_array_alignment.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/init_array_alignment.exe"

cat >"$WORK/aggregate_reference_frame.cpp" <<'SRC'
#include <map>
#include <ranges>
#include <vector>

int main() {
  std::vector<double> values;
  std::map<int, double> by_key;
  std::ranges::iota_view<int, int> source(0, 13);
  std::ranges::iota_view<int, int> target;
  target = static_cast<std::ranges::iota_view<int, int>&&>(source);
  if (target.begin() == target.end() || *target.begin() != 0) {
    return 1;
  }
  return values.empty() && by_key.empty() ? 0 : 2;
}
SRC

# References occupy pointer-sized frame slots even when their aggregate
# referents have a different size.
"$DAVECC" -target 65c02 -O2 "$WORK/aggregate_reference_frame.cpp" \
  -o "$WORK/aggregate_reference_frame.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/aggregate_reference_frame.exe"

cat >"$WORK/aggregate_reference_load.cpp" <<'SRC'
#include <vector>

// Points into itself, so a relocation that reads the wrong source shows up as
// a wrong payload rather than as a wrong pointer.
struct Holder {
  char buf[8];
  char* data;
  unsigned size;
  Holder(char c) { data = buf; buf[0] = c; size = 1; }
  Holder(Holder&& other) {
    data = buf;
    size = other.size;
    buf[0] = other.buf[0];
  }
  Holder(const Holder& other) {
    data = buf;
    size = other.size;
    buf[0] = other.buf[0];
  }
  Holder& operator=(const Holder&) = default;
};

int main() {
  std::vector<Holder> values;
  values.push_back(Holder('x'));
  values.push_back(Holder('y'));  // Reallocates, relocating the first element.
  if (values.size() != 2) return 1;
  if (values[0].data != values[0].buf) return 2;
  if (values[0].size != 1 || values[0].buf[0] != 'x') return 3;
  if (values[1].size != 1 || values[1].buf[0] != 'y') return 4;
  return 0;
}
SRC

# Reading a reference whose referent is an aggregate must load the pointer the
# reference holds.  Taking the address of the reference's own slot instead fed
# vector's relocation a stack cell as the element to move from.
"$DAVECC" -target 65c02 -O2 "$WORK/aggregate_reference_load.cpp" \
  -o "$WORK/aggregate_reference_load.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/aggregate_reference_load.exe"

cat >"$WORK/transform_pipe_return.cpp" <<'SRC'
#include <map>
#include <ranges>
#include <vector>

int main() {
  std::vector<double> values;
  std::map<int, double> by_key;
  auto samples = std::views::iota(0, 13) |
                 std::views::transform([](int step) { return step * 15; });
  int count = 0;
  int sum = 0;
  for (int value : samples) {
    ++count;
    sum += value;
  }
  if (!values.empty() || !by_key.empty()) return 1;
  return count == 13 && sum == 1170 ? 0 : 2;
}
SRC

# Keep the transform view's concrete return type through the pipe operator;
# auto deduction previously produced a view whose copied base had a zero bound.
"$DAVECC" -target 65c02 -O2 "$WORK/transform_pipe_return.cpp" \
  -o "$WORK/transform_pipe_return.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/transform_pipe_return.exe"

# Run the same ranges and floating-point regression used by every other
# executable backend.
"$DAVECC" -target 65c02 -O2 "$RANGES_MATH_TEST" \
  -o "$WORK/ranges_math.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/ranges_math.exe"

cat >"$WORK/printf_local_labels.cpp" <<'SRC'
#include <cstdio>

int main() {
  std::printf("%d", 1);
  std::printf("%zu", sizeof(int));
  std::printf("%f", 0.5);
  return 0;
}
SRC

# Specialized printf archive members contain identically named internal
# control-flow labels. Those labels must remain local to each object.
"$DAVECC" -target 65c02 -O2 "$WORK/printf_local_labels.cpp" \
  -o "$WORK/printf_local_labels.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/printf_local_labels.exe"
