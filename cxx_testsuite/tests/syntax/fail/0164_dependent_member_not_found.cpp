// RUN: -std=c++20
// A dependent qualified name `R::member` that resolves, at instantiation, to a
// member that does not exist in the substituted type is diagnosed.
// EXPECT: no member named 'nonexistent' in the dependent scope

struct Cfg {
  static const int scale = 1;
};

template <class R>
int f() {
  return R::nonexistent;
}

int main() {
  return f<Cfg>();
}
