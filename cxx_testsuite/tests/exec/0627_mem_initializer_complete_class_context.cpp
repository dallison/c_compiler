// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Limits {
  int low;
  int high;
  constexpr Limits(int l, int h) noexcept : low(l), high(h) {}
};

class Plain {
 public:
  Plain() : limits_(defaults()), scale_(factor() * 2) {}
  static constexpr Limits defaults() noexcept { return Limits(1, 100); }
  static int factor() { return 3; }
  int high() const { return limits_.high; }
  int scale() const { return scale_; }

 private:
  Limits limits_;
  int scale_;
};

template <class T, class A = int>
class Holder {
 public:
  constexpr Holder() noexcept : Holder(A()) {}
  constexpr explicit Holder(const A& a) noexcept
      : alloc_(a), limits_(defaults()) {}
  template <class U>
    requires(sizeof(U) > 0)
  explicit Holder(U value) : alloc_(static_cast<A>(value)), limits_(defaults()) {}
  static constexpr Limits defaults() noexcept { return Limits(8, 64); }
  int low() const { return limits_.low; }
  A alloc() const { return alloc_; }

 private:
  A alloc_;
  Limits limits_;
};

int main() {
  Plain plain;
  if (plain.high() != 100 || plain.scale() != 6) {
    return 1;
  }
  Holder<long> holder;
  if (holder.low() != 8 || holder.alloc() != 0) {
    return 2;
  }
  Holder<long> converted(5L);
  if (converted.low() != 8 || converted.alloc() != 5) {
    return 3;
  }
  return 0;
}
