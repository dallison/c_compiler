// RUN: -std=c++20
// EXPECT_EXIT: 0

// [expr.ref]: naming a reference member designates its referent.  A nested
// class of a class template keeps that when its member template is
// instantiated, so `s.find(key)` calls through the referenced object and
// `auto` deduces the call's result (raw_hash_set's EmplaceDecomposable).
template <class T>
struct Set {
  int n = 0;
  template <class K>
  int find(const K& k) { return n + k; }

  struct Emplace {
    template <class K>
    int operator()(const K& key) const {
      auto r = s.find(key);
      return r;
    }
    Set& s;
  };

  struct Holder {
    Set& s;
    explicit Holder(Set& set) : s(set) {}
    template <class K>
    Set* Address(const K&) const { return &s; }
  };

  int Go() { return Emplace{*this}(3); }
};

int main() {
  Set<int> s;
  s.n = 4;
  if (s.Go() != 7) return 1;
  Set<int>::Holder h(s);
  if (h.Address(1) != &s) return 2;
  return 0;
}
