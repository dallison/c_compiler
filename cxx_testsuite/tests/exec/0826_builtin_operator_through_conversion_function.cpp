// With no user-declared operator, a class operand of a built-in operator is
// converted by its conversion function ([over.match.oper]/7).  Subspace
// indexes with `bcb_->sizes[ccb_->num_buffers - 1]` on a std::atomic<int>.
// Relational operators only use the rewritten `operator<=>` form when one
// exists.
#include <atomic>
#include <compare>
#include <cstdint>

struct Count {
  int v;
  operator int() const { return v; }
};

template <class T>
struct Wrapper {
  T v;
  operator T() const { return v; }
};

struct Flag {
  bool on;
  operator bool() const { return on; }
};

struct Ordered {
  int v;
  auto operator<=>(const Ordered&) const = default;
};

struct Control {
  std::atomic<int> num_buffers;
};
struct Sizes {
  uint64_t sizes[4];
};

int main() {
  Count c{5};
  if (c + 1 != 6 || c - 1 != 4 || c * 2 != 10 || 20 / c != 4) return 1;
  if ((c & 4) != 4 || (c << 1) != 10 || -c != -5 || !(c == 5)) return 2;
  if (!(c < 6) || !(4 < c) || !(c >= 5) || c > 5 || c != 5) return 3;

  Wrapper<long> w{7};
  long sum = w + 1;
  if (sum != 8 || !(w > 6)) return 4;

  Flag f{true};
  if (f + 1 != 2) return 5;

  Ordered a{1}, b{2};
  if (!(a < b) || b <= a || a == b) return 6;

  Control control;
  control.num_buffers = 3;
  Sizes sizes{{10, 20, 30, 40}};
  Control* ccb = &control;
  Sizes* bcb = &sizes;
  uint64_t size = bcb->sizes[ccb->num_buffers - 1];
  if (size != 30) return 7;
  if (ccb->num_buffers == 0 || !(ccb->num_buffers < 4)) return 8;
  return 0;
}
