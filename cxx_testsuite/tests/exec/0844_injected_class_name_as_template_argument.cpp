// `using const_iterator = It<Splitter>;` inside `Splitter` names the current
// instantiation through the injected class name (absl's SplitIterator).  The
// member bodies must construct `It<Splitter<int, char>>`, and the hidden
// friends of the `It<Splitter>` that `Traits<const_iterator>` instantiates
// while the pattern is parsed must not stand in for the real ones.
template <typename T>
struct Traits {
  using value_type = typename T::value_type;
};

template <typename S>
class It {
 public:
  using value_type = int;
  enum State { kInit, kEnd };
  It(State s, const S* owner) : pos_(s == kEnd ? owner->size() : 0) {}
  explicit It(int p) : pos_(p) {}
  It& operator++() {
    ++pos_;
    return *this;
  }
  int operator*() const { return pos_; }
  friend bool operator==(const It& a, const It& b) { return a.pos_ == b.pos_; }
  friend bool operator!=(const It& a, const It& b) { return !(a == b); }

 private:
  int pos_;
};

template <typename D, typename P>
class Splitter {
 public:
  using const_iterator = It<Splitter>;
  using value_type = typename Traits<const_iterator>::value_type;
  int size() const { return 3; }
  const_iterator begin() const { return {const_iterator::kInit, this}; }
  const_iterator end() const { return {const_iterator::kEnd, this}; }
  const_iterator at(int p) const { return const_iterator(p); }
};

int main() {
  Splitter<int, char> s;
  int sum = 0;
  for (Splitter<int, char>::value_type v : s) {
    sum += v;
  }
  for (auto it = s.begin(); it != s.end(); ++it) {
    sum += *it;
  }
  if (sum != 6) {
    return 1;
  }
  return *s.at(2) == 2 && s.at(3) == s.end() ? 0 : 2;
}
