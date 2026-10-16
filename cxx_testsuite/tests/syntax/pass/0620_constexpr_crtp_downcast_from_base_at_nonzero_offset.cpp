// RUN: -std=c++20 -fconstexpr-eval=audit

// A CRTP base that is not at offset 0 in the derived class casts `this` back
// to the derived class during constant evaluation, for a local object and for
// a class prvalue.
struct Other {
  short x;
};

template <class D>
struct Interface {
  int tag = 7;
  constexpr const D& derived() const { return static_cast<const D&>(*this); }
  constexpr int first() const { return derived().values[0] + tag - 7; }
};

struct View : Other, Interface<View> {
  int values[2];
};

constexpr bool local_view() {
  View view{{1}, {}, {5, 6}};
  return view.first() == 5;
}
static_assert(local_view());

constexpr View make_view() { return View{{1}, {}, {5, 6}}; }
static_assert(make_view().first() == 5);

int main() { return 0; }
