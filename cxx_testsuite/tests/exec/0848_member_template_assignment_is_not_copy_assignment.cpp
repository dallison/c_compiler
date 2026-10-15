// `template <bool O> C& operator=(const C<O>&)` is never the copy assignment
// operator, so the implicit one stays declared and wins for `C<B> = C<B>`
// (protobuf's Printer::ValueImpl).  The template still handles the other
// specialization.
template <bool B>
struct C {
  C() = default;
  explicit C(int v) : v(v) {}
  template <bool O>
  C(const C<O>& that) {
    *this = that;
  }
  template <bool O>
  C& operator=(const C<O>& that) {
    v = that.v + 100;
    return *this;
  }
  int v = 0;
};

struct Plain {
  template <typename T>
  Plain& operator=(const T&) {
    v = 100;
    return *this;
  }
  int v = 0;
};

int main() {
  C<false> a(3), b;
  b = a;
  if (b.v != 3) {
    return 1;
  }
  C<false> copied(a);
  if (copied.v != 3) {
    return 2;
  }
  C<true> other(a);
  C<true> assigned;
  assigned = a;
  if (other.v != 103 || assigned.v != 103) {
    return 3;
  }
  Plain p, q;
  q.v = 7;
  p = q;
  return p.v == 7 ? 0 : 4;
}
