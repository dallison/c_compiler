// A member alias template (`IteratorValueAdapter<ForwardIterator>`, absl's
// InlinedVector) used inside a member function template is applied to the
// use's own arguments, not replaced by the class-instantiated alias member.
#include <cstddef>

namespace internal {
template <typename A, typename Iterator>
class IteratorValueAdapter {
 public:
  explicit IteratorValueAdapter(const Iterator& it) : it_(it) {}
  int Sum(size_t n) const {
    int total = 0;
    for (size_t i = 0; i < n; i++) total += it_[i].v;
    return total;
  }

 private:
  Iterator it_;
};
}  // namespace internal

template <typename T, size_t N, typename A = int>
class Vec {
  template <typename Iterator>
  using IteratorValueAdapter = internal::IteratorValueAdapter<A, Iterator>;

 public:
  template <typename ForwardIterator>
  Vec(ForwardIterator first, ForwardIterator last)
      : sum_(IteratorValueAdapter<ForwardIterator>(first).Sum(
            static_cast<size_t>(last - first))) {}
  template <typename ForwardIterator>
  int Again(ForwardIterator first, size_t n) {
    return IteratorValueAdapter<ForwardIterator>(first).Sum(n);
  }
  int sum_;
};

struct Arg {
  int v;
};

int main() {
  Arg args[3] = {{1}, {2}, {4}};
  const Arg* p = args;
  Vec<Arg, 4> v(p, p + 3);
  return v.sum_ == 7 && v.Again(p, 2) == 3 ? 0 : 1;
}
