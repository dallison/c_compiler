// RUN: -std=c++17
// EXPECT_EXIT: 0

// A functional cast through a template parameter of a nested partial
// specialization (`First(key)`), inside one of its member templates, must
// stay dependent when the enclosing class template is instantiated.  It used
// to bind to the enclosing class's argument (reduced from absl::StrSplit).

#include <map>
#include <string>
#include <utility>

struct Key {
  const char* s;
  Key(const char* p) : s(p) {}
};

template <typename T>
class Outer {
 public:
  template <typename C, typename V, bool B>
  struct Conv {
    C operator()(const Outer&) const { return C(); }
  };
  template <typename C, typename First, typename Second>
  struct Conv<C, std::pair<const First, Second>, true> {
    using iterator = typename C::iterator;
    C operator()(const Outer&) const {
      C m;
      Insert(&m, Key("k"));
      return m;
    }
    template <typename M>
    static iterator Insert(M* m, Key key) {
      return ToIter(m->insert(std::make_pair(First(key.s), Second(""))));
    }
    static iterator ToIter(std::pair<iterator, bool> p) { return p.first; }
  };
  template <typename C>
  operator C() const {
    return Conv<C, typename C::value_type, true>()(*this);
  }
  T t;
};

int main() {
  Outer<int> o;
  o.t = 0;
  std::map<std::string, std::string> m = o;
  if (m.size() != 1) {
    return 1;
  }
  if (!m["k"].empty()) {
    return 2;
  }
  return 0;
}
