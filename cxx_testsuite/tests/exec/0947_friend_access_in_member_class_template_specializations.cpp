// RUN: -std=c++20
// EXPECT_EXIT: 0

// [class.friend]/2, [temp.expl.spec]: a specialization of a member class
// template is a member of the enclosing class even when it is defined at
// namespace scope, so it shares the enclosing class's friendship
// (protobuf's InternalVisibility tokens).
class Meta;

class Token {
  explicit constexpr Token() {}
  friend class Meta;
};

struct Set {
  Set(Token, int* a) : a_(a) {}
  int* a_;
};

class Meta {
 public:
  struct Base {};
  struct Out;
  template <class T> struct Container;
  template <class T> struct Tmpl;
  template <class T> struct Part;
};

template <>
struct Meta::Container<Set> : Meta::Base {
  Set s;
  explicit Container(int* a) : s(Token{}, a) {}
};

struct Meta::Out {
  Set s;
  explicit Out(int* a) : s(Token{}, a) {}
};

template <class T>
struct Meta::Tmpl {
  Set s;
  explicit Tmpl(int* a) : s(Token{}, a) {}
};

template <class T>
struct Meta::Part<T*> {
  Set s;
  explicit Part(int* a) : s(Token{}, a) {}
};

int main() {
  int x = 0;
  Meta::Container<Set> c(&x);
  Meta::Out o(&x);
  Meta::Tmpl<int> t(&x);
  Meta::Part<int*> p(&x);
  return c.s.a_ == &x && o.s.a_ == &x && t.s.a_ == &x && p.s.a_ == &x ? 0 : 1;
}
