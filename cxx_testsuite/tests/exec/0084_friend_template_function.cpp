// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// Friend functions declared inside a class template whose signatures depend on
// the template parameters are instantiated per specialization: each gets its
// own substituted signature (and, for inline definitions, its own body), and is
// granted access to the private members of the matching specialization.

template <typename T>
class Box {
  T value;

 public:
  Box(T v) : value(v) {}

  // Hidden-friend idiom: an inline friend operator instantiated per Box<T>.
  friend bool operator==(const Box& a, const Box& b) {
    return a.value == b.value;
  }

  // Inline friend returning the dependent type, distinct per instantiation.
  friend T sum(const Box& a, const Box& b) { return a.value + b.value; }

  // Out-of-line dependent friend: declared here, defined per specialization at
  // namespace scope, and still granted access to Box<T>'s private members.
  friend T unwrap(const Box<T>& b);
};

int unwrap(const Box<int>& b) { return b.value; }
long unwrap(const Box<long>& b) { return b.value; }

int main(void) {
  Box<int> a(2), b(2), c(5);
  Box<long> p(10), q(10), r(20);

  // operator== is instantiated separately for Box<int> and Box<long>.
  if (!(a == b)) return 1;
  if (a == c) return 2;
  if (!(p == q)) return 3;
  if (p == r) return 4;

  // sum returns the per-instantiation dependent type.
  if (sum(a, c) != 7) return 5;
  if (sum(p, r) != 30) return 6;

  // Out-of-line dependent friends access the private member.
  if (unwrap(a) != 2) return 7;
  if (unwrap(r) != 20) return 8;

  return 0;
}
